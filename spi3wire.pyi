# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Optional

class Spi3Wire:

    def __init__(
        self,
        scl: int,
        sda: int,
        cs: int,
        freq: int,
        spi_mode: int = 0,
        use_dc_bit: bool = True,
        dc_zero_on_data: bool = False,
        lsb_first: bool = False,
        cs_high_active: bool = False,
        del_keep_cs_inactive: bool = False,
    ):
        ...

    def __del__(self):
        ...

    def init(self, cmd_bits: int, param_bits: int) -> None:
        ...

    def deinit(self) -> None:
        ...

    def tx_param(self, cmd: int, params: Optional[memoryview]=None):
        ...
