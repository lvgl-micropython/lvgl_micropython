# Copyright (c) 2026 Tim C for Adafruit Industries

from micropython import const  # NOQA
import micropython  # NOQA
import pointer_framework
import time


I2C_ADDR = 0x48
BITS = 8

_MEASURE_X = const(0x0C)
_MEASURE_Y = const(0x0D)
_MEASURE_Z1 = const(0x0E)
_MEASURE_Z2 = const(0x0F)
_MEASURE_TEMP0 = const(0x00)

_POWERDOWN_IRQON = const(0x00)
_ADON_IRQOFF = const(0x01)

_ADC_12BIT = const(0x00)

_MIN_RAW_COORD = const(10)
_MAX_RAW_COORD = const(4090)


class TSC2007(pointer_framework.PointerDriver):

    touch_threshold = 250
    sample_threshold = 100

    def __init__(
        self,
        device,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        self._device = device

        self._tx_buf = bytearray(1)
        self._tx_mv = memoryview(self._tx_buf)

        self._rx_buf = bytearray(2)
        self._rx_mv = memoryview(self._rx_buf)

        # Put controller in a known state (power down with IRQ enabled).
        self._command(_MEASURE_TEMP0, _POWERDOWN_IRQON, _ADC_12BIT)

        super().__init__(
            touch_cal=touch_cal, startup_rotation=startup_rotation, debug=debug
        )

    def _command(self, func, pwr, res):
        self._tx_buf[0] = (func << 4) | (pwr << 2) | (res << 1)

        self._device.write(self._tx_mv[:1])
        time.sleep_us(500)  # NOQA

        self._device.read(buf=self._rx_mv[:2])

        return (self._rx_buf[0] << 4) | (self._rx_buf[1] >> 4)

    def _read_touch(self):
        z1 = self._command(_MEASURE_Z1, _ADON_IRQOFF, _ADC_12BIT)
        z2 = self._command(_MEASURE_Z2, _ADON_IRQOFF, _ADC_12BIT)

        if z1 == 0:
            self._command(_MEASURE_TEMP0, _POWERDOWN_IRQON, _ADC_12BIT)
            return None

        z = z1 + ((_MAX_RAW_COORD + 6) - z2)
        if z < self.touch_threshold:
            self._command(_MEASURE_TEMP0, _POWERDOWN_IRQON, _ADC_12BIT)
            return None

        x1 = self._command(_MEASURE_X, _ADON_IRQOFF, _ADC_12BIT)
        y1 = self._command(_MEASURE_Y, _ADON_IRQOFF, _ADC_12BIT)
        x2 = self._command(_MEASURE_X, _ADON_IRQOFF, _ADC_12BIT)
        y2 = self._command(_MEASURE_Y, _ADON_IRQOFF, _ADC_12BIT)

        # Put the controller back in low-power pen IRQ mode.
        self._command(_MEASURE_TEMP0, _POWERDOWN_IRQON, _ADC_12BIT)

        if abs(x1 - x2) > self.sample_threshold:
            return None

        if abs(y1 - y2) > self.sample_threshold:
            return None

        x = (x1 + x2) // 2
        y = (y1 + y2) // 2

        if x == 0xFFF or y == 0xFFF:
            return None

        return x, y

    def _normalize(self, x, y):
        x = pointer_framework.remap(
            x, _MIN_RAW_COORD, _MAX_RAW_COORD, 0, self._orig_width
        )
        y = pointer_framework.remap(
            y, _MIN_RAW_COORD, _MAX_RAW_COORD, 0, self._orig_height
        )

        return x, y

    def _get_coords(self):
        coords = self._read_touch()

        if coords is None:
            return None

        x, y = self._normalize(*coords)

        return self.PRESSED, x, y
