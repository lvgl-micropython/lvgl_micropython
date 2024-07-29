import esp32  # NOQA
import lcd_utils
import struct

import nvs  # NOQA


class TouchCalData(object):

    def __init__(self, name):
        self._config = esp32.NVS(name)

        blob = bytearray(24)
        try:
            self._config.get_blob("ts_config", blob)
            (
                alphaX, betaX, deltaX, alphaY, betaY, deltaY
            ) = struct.unpack("<IIIIII", blob)

            self._alphaX = round(lcd_utils.binary_to_float(alphaX), 7)
            self._betaX = round(lcd_utils.binary_to_float(betaX), 7)
            self._deltaX = round(lcd_utils.binary_to_float(deltaX), 7)
            self._alphaY = round(lcd_utils.binary_to_float(alphaY), 7)
            self._betaY = round(lcd_utils.binary_to_float(betaY), 7)
            self._deltaY = round(lcd_utils.binary_to_float(deltaY), 7)

        except OSError:
            self._alphaX = None
            self._betaX = None
            self._deltaX = None
            self._alphaY = None
            self._betaY = None
            self._deltaY = None

        self._is_dirty = False

    def save(self):
        if self._is_dirty:
            if None in (
                self._alphaX,
                self._betaX,
                self._deltaX,
                self._alphaY,
                self._betaY,
                self._deltaY
            ):
                self._config.erase('ts_config')

            else:
                alphaX = lcd_utils.float_to_binary(self._alphaX)
                betaX = lcd_utils.float_to_binary(self._betaX)
                deltaX = lcd_utils.float_to_binary(self._deltaX)
                alphaY = lcd_utils.float_to_binary(self._alphaY)
                betaY = lcd_utils.float_to_binary(self._betaY)
                deltaY = lcd_utils.float_to_binary(self._deltaY)

                blob = struct.pack(
                    '<IIIIII',
                    alphaX, betaX, deltaX, alphaY, betaY, deltaY
                )

                self._config.set_blob("ts_config", blob)

            self._config.commit()

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
        self.save()

