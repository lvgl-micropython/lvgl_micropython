from micropython import const  # NOQA
import micropython  # NOQA
import machine  # NOQA
import pointer_framework
import time
import array


_INT_ON_PD0_BIT = const(0x01)
_INT_OFF_PD0_BIT = const(0x00)

_VREF_ON_PD1_BIT = const(0x02)
_VREF_OFF_PD1_BIT = const(0x00)

_CMD_X_READ = const(0xD0)
_CMD_Y_READ = const(0x90)
_MIN_RAW_COORD = const(10)
_MAX_RAW_COORD = const(4090)

_Z_VALUE_1 = const(0xB0)
_Z_VALUE_2 = const(0xC0)
_Y_POSITION = const(0x90)
_X_POSITION = const(0xD0)
_BATTERY = const(0xA6)
_AUX_IN = const(0xE6)
_TEMP0 = const(0x86)
_TEMP1 = const(0xF6)


class XPT2046(pointer_framework.PointerDriver):

    def __init__(
        self,
        spi_bus,
        cs=None,
        interrupt=None,
        vref_on=False,
        press_threshold=400,
        touch_cal=None
    ):
        super().__init__(touch_cal=touch_cal)
        self._trans_buf = bytearray(2)
        self._trans_mv = memoryview(self._trans_buf)

        self._recv_buf = bytearray(2)
        self._recv_mv = memoryview(self._recv_buf)

        self._press_threshold = press_threshold
        self.__read_last = False
        self.__int_running = False

        if interrupt is not None:
            self._on_schedule_ref = self._on_schedule
            self.__last_coords = array.array('i', [-1, 0, 0])

            PD0_BIT = _INT_ON_PD0_BIT
            interrupt = machine.Pin(interrupt, machine.Pin.IN)

            interrupt.irq(
                trigger=machine.Pin.IRQ_FALLING,
                handler=self._on_interrupt
            )

            self._set_mode_event()
        else:
            PD0_BIT = _INT_OFF_PD0_BIT

        self._interrupt = interrupt

        if vref_on:
            PD1_BIT = _VREF_ON_PD1_BIT
        else:
            PD1_BIT = _VREF_OFF_PD1_BIT

        PD_BITS = PD1_BIT | PD0_BIT

        self._Z_VALUE_1 = _Z_VALUE_1 | PD_BITS
        self._Z_VALUE_2 = _Z_VALUE_2 | PD_BITS
        self._Y_POSITION = _Y_POSITION | PD_BITS
        self._X_POSITION = _X_POSITION | PD_BITS
        # self._BATTERY = const(0xA6)
        # self._AUX_IN = const(0xE6)
        # self._TEMP0 = const(0x86)
        # self._TEMP1 = const(0xF6)
        self._spi = spi_bus
        if cs is not None:
            self.__cs = machine.Pin(cs, machine.Pin.OUT)
            self.__cs.value(1)
        else:
            self.__cs = None

    def _read_reg(self, reg):
        self._trans_buf[0] = reg
        self._recv_buf[0] = 0x00
        self._recv_buf[1] = 0x00

        if self.__cs is not None:
            self.__cs.value(0)

        self._spi.write_readinto(self._trans_mv, self._recv_mv)

        if self.__cs is not None:
            self.__cs.value(1)

        return (self._recv_buf[0] << 8) | self._recv_buf[1]

    def _on_schedule(self, *_):
        self.__read_last = True
        self.read()

    def _on_interrupt(self, pin):
        if not pin.value() and not self.__int_running:
            self.__int_running = True

            coords = self._get_coords()
            if coords is not None:
                (
                    self.__last_coords[0],
                    self.__last_coords[1],
                    self.__last_coords[2]
                ) = coords
                micropython.schedule(self._on_schedule_ref, None)

            time.sleep_ms(100)
        elif pin.value() and self.__int_running:
            time.sleep_ms(100)  # Debounce rising edge
            self.__int_running = False  # Unlock interrupt

    def _get_coords(self):
        if self.__read_last and not self.__int_running:
            self.__read_last = False
            if self.__last_coords[0] == -1:
                return None

            state = self.__last_coords[0]
            x = self.__last_coords[1]
            y = self.__last_coords[2]

            self.__last_coords[0] = -1
        else:
            z1 = self._read_reg(self._Z_VALUE_1)
            z2 = self._read_reg(self._Z_VALUE_2)

            z = (z1 >> 3) + (_MAX_RAW_COORD - (z2 >> 3))

            if z < self._press_threshold:
                return None

            # dump first reading
            self._read_reg(self._X_POSITION)

            x_points = []
            y_points = []

            for _ in range(3):
                x = self._read_reg(self._X_POSITION) >> 3
                y = self._read_reg(self._Y_POSITION) >> 3

                if (
                    (_MIN_RAW_COORD <= x <= _MAX_RAW_COORD) or
                    (_MIN_RAW_COORD <= y <= _MAX_RAW_COORD)
                ):
                    x = int((x / float(_MAX_RAW_COORD)) * self._orig_width)
                    y = int((y / float(_MAX_RAW_COORD)) * self._orig_height)

                    x_points.append(x)
                    y_points.append(y)

            if x_points and y_points:
                x = int(sum(x_points) / len(x_points))
                y = int(sum(y_points) / len(y_points))

                state = self.PRESSED
            else:
                return None

        return state, x, y
