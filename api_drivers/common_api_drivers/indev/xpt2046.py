from micropython import const  # NOQA
import micropython  # NOQA
import machine  # NOQA
import pointer_framework
import _thread
import spi as _spi


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
        freq=1000000,
        cs=-1,
        interrupt=-1,
        vref_on=False,
        press_threshold=400,
        touch_cal=None
    ):
        super().__init__(touch_cal=touch_cal)
        self._trans_mv = _spi.get_dma_buffer(1)
        self._recv_mv = _spi.get_dma_buffer(2)

        self._on_schedule_ref = self._on_schedule
        self._press_threshold = press_threshold

        if interrupt != -1:
            PD0_BIT = _INT_ON_PD0_BIT
            interrupt = machine.Pin(interrupt, machine.Pin.IN)
            interrupt.irq(
                trigger=machine.Pin.IRQ_FALLING,
                handler=self._on_interrupt
            )

            self._set_mode_event()  # NOQA
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

        self._spi = _spi.Device(
            spi_bus=spi_bus,
            freq=freq,
            cs=cs
        )

        self._lock = _thread.allocate_lock()

    def _read_reg(self, reg):
        self._lock.acquire()

        def _callback(*_):
            self._lock.release()

        self._recv_mv[0] = 0x00
        self._recv_mv[1] = 0x00
        self._spi.comm(
            cmd=reg,
            cmd_bits=8,
            rx_data=self._recv_mv,
            callback=_callback
        )

        self._lock.acquire()
        self._lock.release()
        return (self._recv_mv[0] << 8) | self._recv_mv[1]

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

        for _ in range(3):
            x = self._read_reg(self._X_POSITION) >> 3
            y = self._read_reg(self._Y_POSITION) >> 3

            if (
                (_MIN_RAW_COORD <= x <= _MAX_RAW_COORD) or
                (_MIN_RAW_COORD <= y <= _MAX_RAW_COORD)
            ):
                x = int((x / float(_MAX_RAW_COORD)) * self._orig_width)  # NOQA
                y = int((y / float(_MAX_RAW_COORD)) * self._orig_height)  # NOQA

                x_points.append(x)
                y_points.append(y)

        if x_points and y_points:
            x = int(sum(x_points) / len(x_points))
            y = int(sum(y_points) / len(y_points))

            return self.PRESSED, x, y

        return None
