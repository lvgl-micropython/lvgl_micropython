import display_driver_framework
from micropython import const  # NOQA
import machine
import time
import sys

import lvgl as lv  # NOQA


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


_WRITE_CMD   =  const(0x02)
_READ_CMD    =  const(0x03)
_WRITE_COLOR =  const(0x32)


_RASET  = const(0x2B)
_CASET  = const(0x2A)
_RAMWR  = const(0x2C)
_RAMWRC = const(0x3C)
_MADCTL = const(0x36)

_MADCTL_MH  = const(0x04)  # Refresh 0=Left to Right, 1=Right to Left
_MADCTL_BGR = const(0x08)  # BGR color order
_MADCTL_ML  = const(0x10)  # Refresh 0=Top to Bottom, 1=Bottom to Top

_MADCTL_MV = const(0x20)  # 0=Normal, 1=Row/column exchange
_MADCTL_MX = const(0x40)  # 0=Left to Right, 1=Right to Left
_MADCTL_MY = const(0x80)  # 0=Top to Bottom, 1=Bottom to Top

CHUNK_SIZE = 10

cmd_RAMWR = _RAMWR & 0xff
cmd_RAMWR <<= 8
cmd_RAMWR |= _WRITE_COLOR << 24

cmd_RAMWRC = _RAMWRC & 0xff
cmd_RAMWRC <<= 8
cmd_RAMWRC |= _WRITE_COLOR << 24



class NV3041A(display_driver_framework.DisplayDriver):

    _ORIENTATION_TABLE = (
        0,
        _MADCTL_MV,
        _MADCTL_MX | _MADCTL_MY,
        _MADCTL_MV | _MADCTL_MX | _MADCTL_MY
    )

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        print("NV3041A initializing...")

    def init(self, type=None):  # NOQA
        if type is None:
            mod_name = f'_{self.__class__.__name__.lower()}_init'
        else:
            mod_name = f'_{self.__class__.__name__.lower()}_init_type{type}'

        mod = __import__(mod_name)
        mod.init(self)
        del sys.modules[mod_name]


    def set_params(self, cmd, params=None):
        cmd = cmd & 0xff 
        cmd <<= 8
        cmd |= _WRITE_CMD << 24
        self._data_bus.tx_param(cmd, params)

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int):
        
        param_buf = self._param_buf  # NOQA

        # Column addresses
        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        param_buf[2] = (x2 >> 8) & 0xFF
        param_buf[3] = x2 & 0xFF
        self.set_params(_CASET, self._param_mv)

        # Page addresses
        param_buf[0] = (y1 >> 8) & 0xFF
        param_buf[1] = y1 & 0xFF
        param_buf[2] = (y2 >> 8) & 0xFF
        param_buf[3] = y2 & 0xFF
        self.set_params(_RASET, self._param_mv)

        '''
        if y1 == 0:
            return _RAMWR
        return _RAMWRC
        '''
       
    #@micropython.native
    def _flush_cb(self, _, area, color_p):

        """
        Flush callback for LVGL
        """

        x1 = area.x1
        x2 = area.x2

        y1 = area.y1
        y2 = area.y2

        #cmd = self._set_memory_location(x1, y1, x2, y2)
        self._set_memory_location(x1, y1, x2, y2)

        width = x2 - x1 + 1
        height = y2 - y1 + 1      
        size = width * height * lv.color_format_get_size(self._color_space)

        data_view = color_p.__dereference__(size)


        #chunk_size = 1024 * CHUNK_SIZE
        chunk_size = 10240
        for i in range(0, size, chunk_size):
            chunk = data_view[i:i + chunk_size]
            if i == 0:
                # 1 _RAMWR
                self._data_bus.tx_color(cmd_RAMWR, chunk, x1, y1, x2, y2)
            else:
                # other _RAMWRC
                self._data_bus.tx_color(cmd_RAMWRC, chunk, x1, y1, x2, y2)
