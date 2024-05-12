from micropython import const
import micropython
import machine
import pointer_framework


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
        miso,
        mosi,
        clk,
        cs,
        host,
        freq=5000000,
        interrupt=-1,
        vref_on=False,
        press_threshold=10,
        touch_cal=None
    ):
        super().__init__(touch_cal=touch_cal)
        self._trans_buf = bytearray(1)
        self._trans_mv = memoryview(self._trans_buf)

        self._recv_buf = bytearray(2)
        self._recv_mv = memoryview(self._recv_buf)

        self._on_schedule_ref = self._on_schedule
        self._press_threshold = press_threshold
        if interrupt != -1:
            PD0_BIT = _INT_ON_PD0_BIT
            interrupt = machine.Pin(interrupt, machine.Pin.IN)
            interrupt.irq(trigger=machine.Pin.IRQ_FALLING, handler=self._on_interrupt)

            self._set_mode_event()
        else:
            interrupt = None
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

        self._spi = machine.SPI(
            host + 1,
            baudrate=freq,
            sck=machine.Pin(clk, machine.Pin.OUT),
            mosi=machine.Pin(mosi, machine.Pin.OUT),
            miso=machine.Pin(miso, machine.Pin.IN, pull=machine.Pin.PULL_UP)
        )

        self.cs = machine.Pin(cs, machine.Pin.OUT)
        self.cs.value(1)

    def _read_reg(self, reg):
        self.cs.value(0)
        self._trans_buf[0] = reg
        self._recv_buf[0] = 0x00
        self._recv_buf[1] = 0x00
        self._spi.write_readinto(self._trans_mv, self._recv_mv)
        self.cs.value(1)
        
        return (self._recv_buf[0] << 8) | self._recv_buf[1]

    def _on_schedule(self, *_):
        self.read()

    def _on_interrupt(self, _):
        micropython.schedule(self._on_schedule_ref, None)

    def _get_coords(self):
        z1 = self._read_reg(self._Z_VALUE_1)
        z2 = self._read_reg(self._Z_VALUE_2)

        z = (z1 >> 3) + (_MAX_RAW_COORD - (z2 >> 3))

        if z < self._press_threshold:
            return None

        # dump first reading
        self._read_reg(self._X_POSITION)

        x_points = []
        y_points = []

        count = 0
        while count != 3:
            x = self._read_reg(self._X_POSITION) >> 3
            y = self._read_reg(self._Y_POSITION) >> 3

            if (
                (_MIN_RAW_COORD < x < _MAX_RAW_COORD) or
                (_MIN_RAW_COORD < y < _MAX_RAW_COORD)
            ):
                continue

            x_points.append(x)
            y_points.append(y)
            count += 1

        x = int(sum(x_points) / len(x_points))
        y = int(sum(y_points) / len(y_points))

        x = int((x / _MAX_RAW_COORD) * self._orig_width)
        y = int((y / _MAX_RAW_COORD) * self._orig_height)

        print(x, y)

        return self.PRESSED, x, y
