# Copyright (C) 2024  Kevin G Schlosser
# Code that is written by the above named is done under the GPL license
# and that license is able to be viewed in the LICENSE file in the root
# of this project.

from typing import Optional, Any, Callable, Union, ClassVar
from array import array


class SPI(object):
    MSB: ClassVar[int] = ...
    LSB: ClassVar[int] = ...

    def __init__(
        self,
        id: int,
        baudrate: int,
        /,
        *,
        polarity: int=0,
        phase: int=0,
        bits: int=8,
        firstbit: int=MSB,
        sck: Union[str, int]=None,
        mosi: Union[str, int]=None,
        miso: Union[str, int]=None,
        cs: Union[str, int]=None,
    ):
        ...

    def deinit(self) -> None:
        ...

    def read(self, nbytes: int, write: int=0x00, /) -> bytes:
        ...

    def readinto(self, buf: Union[memoryview, bytes, bytearray, array], write: int=0x00, /) -> None:
        ...

    def write(self, buf: Union[memoryview, bytes, bytearray, array], /) -> None:
        ...

    def write_readinto(
        self,
        write_buf: Union[memoryview, bytes, bytearray, array],
        read_buf: Union[memoryview, bytes, bytearray, array],
        /
    ) -> None:
        ...


del ClassVar
del Callable
del Union
del Any
del Optional
del array
