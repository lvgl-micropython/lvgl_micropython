# Copyright (c) 2024 - 2025 Kevin G. Schlosser
import machine
import lcd_utils


class Pin(object):
    IN = 0x00
    OUT = 0x01
    OPEN_DRAIN = 0x02

    IRQ_RISING = 0x01
    IRQ_FALLING = 0x02

    PULL_UP = 0x01
    PULL_DOWN = 0x02

    _int_pin = None
    _reg_int_pins = []

    _device = None

    @classmethod
    def _int_cb(cls, _):
        for ext_pin in cls._reg_int_pins:
            ext_pin._interrupt_cb()  # NOQA

    def _interrupt_cb(self):
        if self._irq is None:
            # sanity check
            return

        value = self.value()
        if value == self._irq_input_state:
            return

        self._irq_input_state = value

        handler, trigger = self._irq

        if value and trigger & self.IRQ_RISING:
            handler(self)

        elif not value and trigger & self.IRQ_FALLING:
            handler(self)

    @classmethod
    def set_int_pin(cls, pin_num, pull=-1):
        if cls._int_pin is not None:
            raise ValueError('Interrupt pin has already been set')

        try:
            cls._int_cb(None)
        except NotImplementedError:
            raise NotImplementedError
        except:  # NOQA
            pass

        if pull == -1:
            int_pin = machine.Pin(pin_num, machine.Pin.IN)
        else:
            int_pin = machine.Pin(pin_num, machine.Pin.IN, pull=pull)

        int_pin.irq(
            cls._int_cb,
            machine.Pin.IRQ_FALLING
        )
        cls._int_pin = int_pin

    @classmethod
    def set_device(cls, device):
        if cls._device is not None:
            raise ValueError('device has already been set')

        cls._device = device

    def __init__(self, id, mode=-1, pull=-1, value=-1):  # NOQA
        if Pin._device is None:
            raise RuntimeError('The expander device has not been set')

        self._id = id
        self._mode = 0
        self._buf = bytearray(2)
        self._mv = memoryview(self._buf)

        self._adc = None
        self._pwm = None
        self._irq = None
        self._irq_input_state = 0

        self.init(mode, pull, value)

    def init(self, mode=-1, pull=-1, value=-1):
        if mode != -1:
            self._set_dir(mode)
            self._mode = mode

        if pull is None or pull != -1:
            self._set_pull(pull)

        if value != -1:
            self._set_level(value)

    def irq(self, handler, trigger=IRQ_RISING | IRQ_FALLING):
        if self._mode == self.IN:
            if self.__class__._int_pin is None:
                raise ValueError(
                    'You need to call Pin.set_int_pin(pin_num) '
                    'before setting the pins IRQ'
                )

            self._irq_input_state = self.value()
            self._set_irq(handler, trigger=trigger)

            if handler is None:
                self._irq = None
                if self in self.__class__._reg_int_pins:
                    self.__class__._reg_int_pins.remove(self)
            else:
                if self not in self.__class__._reg_int_pins:
                    self.__class__._reg_int_pins.append(self)

                self._irq = (handler, trigger)

    def value(self, x=-1):
        if x != -1:
            self._set_level(x)
        else:
            return self._get_level()

    def __call__(self, x=-1):
        if x != -1:
            self._set_level(x)
        else:
            return self._get_level()

    def off(self):
        self.value(0)

    def on(self):
        self.value(1)

    def low(self):
        self.value(0)

    def high(self):
        self.value(1)

    def mode(self, mode=-1):
        if mode == -1:
            return self._mode

        self.init(mode=mode)

    def _set_irq(self, handler, trigger):
        raise NotImplementedError

    def _set_pull(self, pull):
        raise NotImplementedError

    def _set_dir(self, direction):
        raise NotImplementedError

    def _set_level(self, level):
        raise NotImplementedError

    def _get_level(self):
        raise NotImplementedError


class PWM:
    def __init__(self, pin: Pin, freq: int = -1, duty=-1):
        if pin._pwm is not None:  # NOQA
            raise RuntimeError('you can only start PWM a single time for a pin')

        if pin._adc is not None:  # NOQA
            raise RuntimeError('You cannot set a pin to be both PWM and ADC at the same time')

        if pin._irq is not None:  # NOQA
            raise RuntimeError('You cannot have a pin set to be an output and an input at the same time')

        self._pin = pin
        self.init(freq, duty=duty)
        pin._pwm = self

    def init(self, freq=-1, duty=-1):
        raise NotImplementedError

    def deinit(self):
        self._pin._pwm = None

    def freq(self, freq=-1):
        raise NotImplementedError

    def duty(self, duty=-1):
        raise NotImplementedError


class ADC:
    # 511
    WIDTH_9BIT = 0
    # 1023
    WIDTH_10BIT = 1
    # 2047
    WIDTH_11BIT = 2
    # 4095
    WIDTH_12BIT = 3
    # 8191
    WIDTH_13BIT = 4

    # 1.1V
    ATTN_0DB = 0
    # 1.5V
    ATTN_2_5DB = 1
    # 2.2V
    ATTN_6DB = 2
    # 3.9V
    ATTN_11DB = 3

    _min_read = 0
    _max_read = 0

    def __init__(self, pin: Pin, atten=-1):
        if pin._adc is not None:  # NOQA
            raise RuntimeError('you can only start ADC a single time for a pin')

        if pin._pwm is not None:  # NOQA
            raise RuntimeError('You cannot set a pin to be both ADC and PWM at the same time')

        if pin._irq is not None:  # NOQA
            raise RuntimeError('You cannot have a pin set to be an output and an input at the same time')

        self._pin = pin

        self._width = 0
        self._atten = 0

        self.init(atten=atten)

        pin._adc = self

    def init(self, atten=-1):
        self._init(atten=atten)
        self.atten(atten)

    def _init(self, atten):
        raise NotImplementedError

    def deinit(self):
        self._pin._adc = None

    def read_uv(self):
        value = self.read()

        if self._width == self.WIDTH_9BIT:
            old_max = 511.0
        elif self._width == self.WIDTH_10BIT:
            old_max = 1023.0
        elif self._width == self.WIDTH_11BIT:
            old_max = 2047.0
        elif self._width == self.WIDTH_12BIT:
            old_max = 4095.0
        else:  # self.WIDTH_13BIT
            old_max = 8191.0

        if self._atten == self.ATTN_0DB:
            new_max = 1.1
        elif self._atten == self.ATTN_2_5DB:
            new_max = 1.5
        elif self._atten == self.ATTN_6DB:
            new_max = 2.2
        else:  # self.ATTN_11DB
            new_max = 3.9

        value = lcd_utils.remap(value, 0.0, old_max, 0.0, new_max)

        return value

    def atten(self, atten=-1):
        if atten == -1:
            return self._atten
        else:
            self._atten = atten

    def width(self, width=-1):
        if width != -1:
            self._width = width

    def read(self):
        value = self._read()

        if self._width == self.WIDTH_9BIT:
            new_max = 511
        elif self._width == self.WIDTH_10BIT:
            new_max = 1023
        elif self._width == self.WIDTH_11BIT:
            new_max = 2047
        elif self._width == self.WIDTH_12BIT:
            new_max = 4095
        else:  # self.WIDTH_13BIT
            new_max = 8191

        value = lcd_utils.remap(value, self._min_read,
                                self._max_read, 0, new_max)

        return value

    def _read(self):
        raise NotImplementedError
