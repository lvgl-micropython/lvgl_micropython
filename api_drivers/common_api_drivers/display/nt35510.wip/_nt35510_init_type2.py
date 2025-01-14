# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import _nt35510_init


def init(self):
    _nt35510_init.init1(self)

    self._param_buf[0] = 0x6B
    self.set_params(0xB500, self._param_mv[:1])

    _nt35510_init.init2(self)
