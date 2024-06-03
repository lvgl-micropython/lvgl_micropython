from micropython import const  # NOQA
import micropython  # NOQA
import machine  # NOQA
import pointer_framework
import time


_CMD_X_READ = const(0xD0)  # 12 bit resolution
_CMD_Y_READ = const(0x90)  # 12 bit resolution

_MIN_RAW_COORD = const(10)
_MAX_RAW_COORD = const(4090)


class XPT2046(pointer_framework.PointerDriver):

    confidence = 5
    margin = 50

    def __init__(
        self,
        spi_bus,
        touch_cal=None
    ):
        super().__init__(touch_cal=touch_cal)
        self._trans_buf = bytearray(3)
        self._trans_mv = memoryview(self._trans_buf)

        self._recv_buf = bytearray(3)
        self._recv_mv = memoryview(self._recv_buf)

        self.__confidence = max(min(self.confidence, 25), 3)
        self.__points = [[0, 0] for _ in range(self.__confidence)]

        margin = max(min(self.margin, 100), 1)
        self.__margin = margin * margin

        self._spi = spi_bus

    def _read_reg(self, reg):
        self._trans_buf[0] = reg
        self._recv_buf[0] = 0x00
        self._recv_buf[1] = 0x00
        self._recv_buf[2] = 0x00

        self._spi.write_readinto(self._trans_mv, self._recv_mv)

        return ((self._recv_buf[1] << 8) | self._recv_buf[2]) >> 3

    def _get_coords(self):
        points = self.__points
        count = 0
        timeout = 5000000
        start_time = time.ticks_ns()
        while timeout > 0:
            if count == self.__confidence:
                break

            sample = self._get_raw()  # get a touch
            if sample is not None:
                points[count][0], points[count][1] = sample  # put in buff
                count += 1

            end_time = time.ticks_ns()
            timeout -= time.ticks_diff(end_time, start_time)
            start_time = end_time

        if count:
            meanx = sum([points[i][0] for i in range(count)]) // count
            meany = sum([points[i][1] for i in range(count)]) // count
            dev = sum([(points[i][0] - meanx) ** 2 + (points[i][1] - meany) ** 2 for i in range(count)]) / count

            if dev <= self.__margin:
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
        if x > _MIN_RAW_COORD and y < _MAX_RAW_COORD:  # touch pressed?
            return x, y
        else:
            return None
