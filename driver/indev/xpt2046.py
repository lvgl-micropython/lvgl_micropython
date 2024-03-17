from micropython import const
import machine
import pointer_framework


_CMD_X_READ = const(0x20)
_CMD_Y_READ = const(0xA0)
_MAX_RAW_COORD = const((1 << 12) - 1)


class XPT2046(pointer_framework.PointerDriver):

    def __init__(
        self,
        miso,
        mosi,
        clk,
        cs,
        host,
        freq=5000000,
        touch_cal=None
    ):
        super().__init__(touch_cal=touch_cal)
        self._buf = bytearray(2)
        self._mv = memoryview(self._buf)

        self._spi = machine.SPI(
            host,
            baudrate=freq,
            sck=machine.Pin(clk, machine.Pin.OUT),
            mosi=machine.Pin(mosi, machine.Pin.OUT),
            miso=machine.Pin(miso, machine.Pin.IN, pull=machine.Pin.PULL_UP)
        )

        self.cs = machine.Pin(cs, machine.Pin.OUT)
        self.cs.value(1)

    def _get_coords(self):
        buf = self._buf
        mv = self._mv

        x_vals = []
        y_vals = []
        self.cs.value(0)
        for i in range(0, 3):
            xy = []

            for cmd in (_CMD_X_READ, _CMD_Y_READ):
                buf[0] = 0x01
                buf[1] = cmd

                self._spi.write_readinto(mv, mv)

                value = (buf[0] << 4) + (buf[1] >> 4)

                if value == _MAX_RAW_COORD:
                    value = 0
                xy.append(value)

            x_vals.append(xy[0])
            y_vals.append(xy[1])

        self.cs.value(1)

        if 0 in x_vals or 0 in y_vals:
            return None

        x_vals.remove(max(x_vals))
        x_vals.remove(min(x_vals))

        y_vals.remove(max(y_vals))
        y_vals.remove(min(y_vals))

        return self.PRESSED, x_vals[0], y_vals[0]
