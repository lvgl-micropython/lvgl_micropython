# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Optional, Union
from typing import TYPE_CHECKING


if TYPE_CHECKING:
    import array


_BUFFER_TYPE = Union[bytearray, bytes, memoryview, array.array]


class I2C(object):
    class Bus(object):

        def __init__(
            self,
            host: Union[str, int],
            scl: Union[str, int],
            sda: Union[str, int],
            freq: int = 4000000,
            use_locks: bool = False
        ):
            ...

        def __enter__(self) -> "I2C.Bus":
            ...

        def __exit__(self, exc_type, exc_val, exc_tb) -> None:
            ...

        def scan(self) -> list:
            ...

        def start(self) -> None:
            ...

        def stop(self) -> None:
            ...

        def readinto(self, buf: _BUFFER_TYPE,  nack: bool = True) -> None:
            ...

        def write(self, buf: _BUFFER_TYPE) -> None:
            ...

        def readfrom(self, addr: int, nbytes: int, stop: bool = True) -> bytes:
            ...

        def readfrom_into(self, addr: int, buf: _BUFFER_TYPE, stop: bool = True) -> None:
            ...

        def writeto(self, addr: int, buf: _BUFFER_TYPE, stop: bool = True) -> None:
            ...

        def writevto(self, addr: int, vector: list, stop: bool = True) -> None:
            ...

        def readfrom_mem(self, addr: int, memaddr: int, nbytes: int, addrsize: int = 8) -> bytes:
            ...

        def readfrom_mem_into(self, addr: int, memaddr: int, buf: _BUFFER_TYPE, addrsize: int = 8) -> None:
            ...

        def writeto_mem(self, addr: int, memaddr: int, buf: _BUFFER_TYPE, addrsize: int = 8) -> None:
            ...


    class Device(object):
        _bus: "I2C.Bus" = ...
        dev_id: int = ...
        _reg_bits: int = ...

        def __init__(self, bus: "I2C.Bus", dev_id: int, reg_bits: int = 8):
            ...

        def write_readinto(self, write_buf: _BUFFER_TYPE, read_buf: _BUFFER_TYPE) -> None:
            ...

        def read_mem(self, memaddr: int, num_bytes: Optional[int] = None, buf: Optional[_BUFFER_TYPE] = None) -> Optional[bytes]:
            ...

        def write_mem(self, memaddr: int, buf: _BUFFER_TYPE):
            ...

        def read(self, nbytes: Optional[int] = None, buf: Optional[_BUFFER_TYPE] = None, stop: bool=True) -> Optional[bytes]:
            ...

        def write(self, buf: _BUFFER_TYPE, stop: bool=True) -> None:
            ...
