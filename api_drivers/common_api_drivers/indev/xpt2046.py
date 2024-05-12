from micropython import const
import machine
import pointer_framework


_CMD_X_READ = const(0xD0)
_CMD_Y_READ = const(0x90)
_MIN_RAW_COORD = const(10)
_MAX_RAW_COORD = const(4090)


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
        self._trans_buf = bytearray(3)
        self._trans_mv = memoryview(self._trans_buf)

        self._recv_buf = bytearray(3)
        self._recv_mv = memoryview(self._recv_buf)

        self._spi = machine.SPI(
            host + 1,
            baudrate=freq,
            sck=machine.Pin(clk, machine.Pin.OUT),
            mosi=machine.Pin(mosi, machine.Pin.OUT),
            miso=machine.Pin(miso, machine.Pin.IN, pull=machine.Pin.PULL_UP)
        )

        self.cs = machine.Pin(cs, machine.Pin.OUT)
        self.cs.value(1)

    def _get_coords(self):
        x_vals = []
        y_vals = []

        for _ in range(0, 3):
            self.cs.value(0)
            self._trans_buf[0] = _CMD_X_READ
            self._recv_buf[0] = 0x00
            self._recv_buf[1] = 0x00
            self._recv_buf[2] = 0x00

            self._spi.write_readinto(self._trans_mv, self._recv_mv)
            x = (self._recv_buf[1] * 256 + self._recv_buf[2]) >> 3

            self._trans_buf[0] = _CMD_Y_READ
            self._recv_buf[0] = 0x00
            self._recv_buf[1] = 0x00
            self._recv_buf[2] = 0x00

            self._spi.write_readinto(self._trans_mv, self._recv_mv)
            y = (self._recv_buf[1] * 256 + self._recv_buf[2]) >> 3

            if (
                (_MIN_RAW_COORD < x < _MAX_RAW_COORD) or
                (_MIN_RAW_COORD < y < _MAX_RAW_COORD)
            ):
                return None

            x_vals.append(x)
            y_vals.append(y)
            self.cs.value(1)

        if 0 in x_vals or 0 in y_vals:
            return None

        x_vals.remove(max(x_vals))
        x_vals.remove(min(x_vals))

        y_vals.remove(max(y_vals))
        y_vals.remove(min(y_vals))

        return self.PRESSED, x_vals[0], y_vals[0]
