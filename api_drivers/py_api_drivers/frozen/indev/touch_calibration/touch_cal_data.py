import lcd_utils
import struct
import sys

try:
    from esp32 import NVS  # NOQA
except ImportError:
    import os

    class NVS:

        def __init__(self, name):
            self.name = name
            try:
                self._file = open(name, 'rb')
            except OSError:
                open(name, 'w').close()
                self._file = open(name, 'rb')

            self._data_to_commit = None

        def get_blob(self, key, buf):
            key = key.encode('utf-8')

            buf_len = len(buf)
            self._file.seek(0)
            data = self._file.read()
            if key not in data:
                raise OSError

            data = data.split(key)[1]
            data_len = int(data[0])
            if data_len < buf_len:
                raise OSError

            for i in range(buf_len):
                buf[i] = data[i + 1]

        def set_blob(self, key, buf):
            key = key.encode('utf-8')

            self._file.seek(0)
            data = self._file.read()

            if key in data:
                start_data, end_data = data.split(key)
                data_len = int(end_data[0])
                end_data = end_data[data_len + 1:]

                data = (
                    start_data +
                    key +
                    bytes(bytearray([len(buf)])) +
                    bytes(buf) +
                    end_data
                )
            else:
                data += (
                    key +
                    bytes(bytearray([len(buf)])) +
                    bytes(buf)
                )

            self._data_to_commit = data

        def erase(self, key):
            key = key.encode('utf-8')

            self._file.seek(0)
            data = self._file.read()

            if key not in data:
                raise OSError

            start_data, end_data = data.split(key)
            data_len = int(end_data[0])
            end_data = end_data[data_len + 1:]

            self._data_to_commit = start_data + end_data

        def commit(self):
            if self._data_to_commit is None:
                return

            self._file.close()
            with open(self.name, 'wb') as f:
                f.write(self._data_to_commit)

            self._file = open(self.name, 'rb')

            self._data_to_commit = None


class TouchCalData(object):

    def __init__(self, name):
        self._config = NVS(name)

        blob = bytearray(26)
        mv = memoryview(blob)
        try:
            self._config.get_blob("ts_config", mv)
        except OSError as err:
            sys.print_exception(err)

            self._alphaX = None
            self._betaX = None
            self._deltaX = None
            self._alphaY = None
            self._betaY = None
            self._deltaY = None
            self._mirror_x = None
            self._mirror_y = None
        else:
            (
                alphaX, betaX, deltaX, alphaY, betaY, deltaY, mirror_x, mirror_y
            ) = struct.unpack("<IIIIIIBB", blob)

            self._alphaX = round(lcd_utils.int_float_converter(alphaX), 7)
            self._betaX = round(lcd_utils.int_float_converter(betaX), 7)
            self._deltaX = round(lcd_utils.int_float_converter(deltaX), 7)
            self._alphaY = round(lcd_utils.int_float_converter(alphaY), 7)
            self._betaY = round(lcd_utils.int_float_converter(betaY), 7)
            self._deltaY = round(lcd_utils.int_float_converter(deltaY), 7)
            self._mirror_x = bool(mirror_x)
            self._mirror_y = bool(mirror_y)

        self._is_dirty = False

    def save(self):
        if self._is_dirty:
            if None in (
                self._alphaX,
                self._betaX,
                self._deltaX,
                self._alphaY,
                self._betaY,
                self._deltaY,
                self._mirror_x,
                self._mirror_y
            ):
                self._config.erase('ts_config')

            else:
                alphaX = lcd_utils.int_float_converter(self._alphaX)
                betaX = lcd_utils.int_float_converter(self._betaX)
                deltaX = lcd_utils.int_float_converter(self._deltaX)
                alphaY = lcd_utils.int_float_converter(self._alphaY)
                betaY = lcd_utils.int_float_converter(self._betaY)
                deltaY = lcd_utils.int_float_converter(self._deltaY)

                blob = struct.pack(
                    '<IIIIIIBB',
                    alphaX, betaX, deltaX, alphaY, betaY, deltaY,
                    int(self._mirror_x), int(self._mirror_y)
                )

                blob = bytearray(blob)
                mv = memoryview(blob)

                self._config.set_blob("ts_config", mv)

            self._config.commit()

    @property
    def mirrorX(self):
        return self._mirror_x

    @mirrorX.setter
    def mirrorX(self, value):
        if value is None:
            self._mirror_x = None
        else:
            self._mirror_x = bool(value)

        self._is_dirty = True

    @property
    def mirrorY(self):
        return self._mirror_y

    @mirrorY.setter
    def mirrorY(self, value):
        if value is None:
            self._mirror_y = None
        else:
            self._mirror_y = bool(value)

        self._is_dirty = True

    @property
    def alphaX(self):
        return self._alphaX

    @alphaX.setter
    def alphaX(self, value):
        if value is None:
            self._alphaX = value
        else:
            self._alphaX = round(value, 7)

        self._is_dirty = True

    @property
    def betaX(self):
        return self._betaX

    @betaX.setter
    def betaX(self, value):
        if value is None:
            self._betaX = value
        else:
            self._betaX = round(value, 7)

        self._is_dirty = True

    @property
    def deltaX(self):
        return self._deltaX

    @deltaX.setter
    def deltaX(self, value):
        if value is None:
            self._deltaX = value
        else:
            self._deltaX = round(value, 7)

        self._is_dirty = True

    @property
    def alphaY(self):
        return self._alphaY

    @alphaY.setter
    def alphaY(self, value):
        if value is None:
            self._alphaY = value
        else:
            self._alphaY = round(value, 7)

        self._is_dirty = True

    @property
    def betaY(self):
        return self._betaY

    @betaY.setter
    def betaY(self, value):
        if value is None:
            self._betaY = value
        else:
            self._betaY = round(value, 7)

        self._is_dirty = True

    @property
    def deltaY(self):
        return self._deltaY

    @deltaY.setter
    def deltaY(self, value):
        if value is None:
            self._deltaY = value
        else:
            self._deltaY = round(value, 7)

        self._is_dirty = True

    def reset(self):
        self.alphaX = None
        self.betaX = None
        self.deltaX = None
        self.alphaY = None
        self.betaY = None
        self.deltaY = None
        self._mirror_x = None
        self._mirror_y = None
        self._is_dirty = True
        self.save()
