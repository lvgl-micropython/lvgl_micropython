from micropython import const  # NOQA
import micropython  # NOQA
import machine  # NOQA
import pointer_framework
import time


_CMD_X_READ = const(0xD1)  # 12 bit resolution
_CMD_Y_READ = const(0x91)  # 12 bit resolution
_CMD_Z1_READ = const(0xB1)  # 12 bit resolution
_CMD_Z2_READ = const(0xC1)  # 12 bit resolution


_MIN_RAW_COORD = const(10)
_MAX_RAW_COORD = const(4090)


class XPT2046(pointer_framework.PointerDriver):

    confidence = 5
    z_thresh = 400

    def __init__(
        self,
        spi_bus,
        touch_cal=None
    ):
        super().__init__(touch_cal=touch_cal)
        self._trans_buf = bytearray(2)
        self._trans_mv = memoryview(self._trans_buf)

        self._recv_buf = bytearray(2)
        self._recv_mv = memoryview(self._recv_buf)

        self.__confidence = max(min(self.confidence, 25), 3)
        self.__points = [[0, 0] for _ in range(self.__confidence)]

        self._spi = spi_bus

    def _read_reg(self, reg):
        self._trans_buf[0] = reg
        self._recv_buf[0] = 0x00
        self._recv_buf[1] = 0x00

        self._spi.write_readinto(self._trans_mv, self._recv_mv)

        return ((self._recv_buf[0] << 8) | self._recv_buf[1]) >> 3

    def _get_coords(self):
        points = self.__points
        count = 0

        z1 = self._read_reg(_CMD_Z1_READ)
        z2 = self._read_reg(_CMD_Z2_READ)

        z = z1 + (_MAX_RAW_COORD - z2)
        if z >= self.z_thresh:
            self._read_reg(_CMD_X_READ)

            while count < self.__confidence:
                sample = self._get_raw()  # get a touch
                if sample is not None:
                    points[count][0], points[count][1] = sample  # put in buff
                    count += 1

            meanx = sum([points[i][0] for i in range(count)]) // count
            meany = sum([points[i][1] for i in range(count)]) // count

            x, y = self._normalize(meanx, meany)
            return self.PRESSED, x, y

        return None

    def _normalize(self, x, y):
        x = pointer_framework._remap(x, _MIN_RAW_COORD, _MAX_RAW_COORD, 0, self._orig_width)
        y = pointer_framework._remap(y, _MIN_RAW_COORD, _MAX_RAW_COORD, 0, self._orig_height)

        return x, y

    def _get_raw(self):
        x = self._read_reg(_CMD_X_READ)
        y = self._read_reg(_CMD_Y_READ)
        if _MAX_RAW_COORD >= x >= _MIN_RAW_COORD and _MAX_RAW_COORD >= y >= _MIN_RAW_COORD:
            return x, y
        else:
            return None
