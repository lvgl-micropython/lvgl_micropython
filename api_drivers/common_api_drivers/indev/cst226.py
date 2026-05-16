from micropython import const  # NOQA
import pointer_framework
import machine  # NOQA
import time


# These values are used by the lvgl_micropython driver loader.
# The caller creates an I2C device using this address and bus width.
I2C_ADDR = 0x5A
BITS = 8

# Touch reports are read from register 0x00.
# The controller returns a fixed size report buffer.
_TOUCH_DATA_REG = const(0x00)
_TOUCH_DATA_SIZE = const(28)

# Several bytes in a report use 0xAB as a status marker.
# The exact meaning depends on which byte contains it.
_TOUCH_DATA_VALID = const(0xAB)

# The low 7 bits of report byte 5 contain the touch count.
# The high bit is used for a controller specific button condition.
_TOUCH_COUNT_MASK = const(0x7F)
_TOUCH_BUTTON_MASK = const(0x80)

# The chip and project ID are read from a 16 bit command register.
# This read only works correctly while the controller is in command mode.
_ID_REG = const(0xD204)
_CHIP_ID = const(0x00A8)

_CONTROL_REG = const(0xD1)
_ENTER_COMMAND_MODE = const(0x01)
_EXIT_COMMAND_MODE = const(0x09)
_SOFT_RESET_VALUE = const(0x0E)

# Writing a nonzero value disables the controller auto-sleep behavior.
_DIS_AUTO_SLEEP_REG = const(0xFE)


class CST226(pointer_framework.PointerDriver):

    def _read_reg(self, reg, num_bytes):
        # Normal touch data reads use an 8 bit register address.
        self._tx_buf[0] = reg
        self._device.write_readinto(
            self._tx_mv[:1],
            self._rx_mv[:num_bytes]
        )

    def _read_reg16(self, reg, num_bytes):
        # Some CST226 command reads use a 16 bit register address.
        self._tx_buf[0] = reg >> 8
        self._tx_buf[1] = reg & 0xFF

        # For some reason, this must be two separate I2C operations
        self._device.write(self._tx_mv[:2])
        self._device.read(buf=self._rx_mv[:num_bytes])

    def _write_reg(self, reg, value):
        self._tx_buf[0] = reg
        self._tx_buf[1] = value
        self._device.write(self._tx_mv[:2])

    def __init__(
        self,
        device,
        reset_pin=None,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        # Reuse small buffers for all I2C transfers.
        # This avoids allocating new byte arrays each time a touch is sampled.
        self._tx_buf = bytearray(2)
        self._tx_mv = memoryview(self._tx_buf)
        self._rx_buf = bytearray(_TOUCH_DATA_SIZE)
        self._rx_mv = memoryview(self._rx_buf)

        # This is an lvgl_micropython I2C device object, not the raw I2C bus.
        self._device = device

        # Accept either a machine.Pin object or an integer GPIO number.
        if isinstance(reset_pin, int):
            self._reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)
        else:
            self._reset_pin = reset_pin

        # Keep the hardware reset line inactive before beginning setup.
        if self._reset_pin:
            self._reset_pin.value(1)

        self.hw_reset()

        # Read project ID, chip ID
        self._write_reg(_CONTROL_REG, _ENTER_COMMAND_MODE)
        time.sleep_ms(10)  # NOQA
        self._read_reg16(_ID_REG, 4)

        # The controller returns project ID first, then chip ID.
        project_id = self._rx_buf[0] | (self._rx_buf[1] << 8)
        chip_id = self._rx_buf[2] | (self._rx_buf[3] << 8)

        if chip_id != _CHIP_ID:
            raise RuntimeError(
                f'Incorrect chip id ({hex(chip_id)}!={hex(_CHIP_ID)})'
            )

        # Return to normal touch reporting mode and keep the panel awake.
        self._write_reg(_CONTROL_REG, _EXIT_COMMAND_MODE)
        self._write_reg(_DIS_AUTO_SLEEP_REG, 0x01)

        # Let the shared framework register this device as an LVGL pointer.
        super().__init__(
            touch_cal=touch_cal, startup_rotation=startup_rotation, debug=debug
        )

    def hw_reset(self):
        # Boards without a wired reset pin can use the CST226 software reset.
        if self._reset_pin is None:
            self._write_reg(_CONTROL_REG, _SOFT_RESET_VALUE)
            time.sleep_ms(20)  # NOQA
            return

        # Hardware reset timing follows SensorLib
        self._reset_pin(0)
        time.sleep_ms(100)  # NOQA
        self._reset_pin(1)
        time.sleep_ms(100)  # NOQA

    def _get_coords(self):
        # LVGL calls this method repeatedly to poll the current touch state.
        self._read_reg(_TOUCH_DATA_REG, _TOUCH_DATA_SIZE)
        data = self._rx_buf

        # Byte 6 must contain the controller valid report marker.
        # If it does not, ignore the buffer rather than decoding stale data.
        if data[6] != _TOUCH_DATA_VALID:
            return None

        # These values indicate no usable pointer touch for LVGL.
        # The button condition is intentionally ignored by this pointer driver.
        if (
            data[0] == _TOUCH_DATA_VALID or
            data[0] == 0x00 or
            data[5] == _TOUCH_BUTTON_MASK
        ):
            return None

        # Only reports with a reasonable touch count should be decoded.
        # The controller supports multiple touches, but LVGL pointer input uses one.
        touch_count = data[5] & _TOUCH_COUNT_MASK
        if touch_count == 0 or touch_count > 5:
            # Acknowledge the invalid report before returning.
            self._write_reg(_TOUCH_DATA_REG, _TOUCH_DATA_VALID)
            return None

        # The first touch point uses packed 12 bit X and Y coordinates.
        # High 8 bits are stored separately, and low nibbles share byte 3.
        x = (data[1] << 4) | ((data[3] >> 4) & 0x0F)
        y = (data[2] << 4) | (data[3] & 0x0F)

        # LVGL expects the pointer state followed by the X and Y coordinates.
        return self.PRESSED, x, y
