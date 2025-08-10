
from _machine import *
import _machine
from _mpy_shed import AnyReadableBuf, AnyWritableBuf
from typing import Sequence, ClassVar


del I2C
del SPI

class I2C:

    class Bus(_machine.I2C):
        def __init__(self, *, host: int | str, scl: int | str,
                     sda: int | str, queue_size: int | None = None):
            ...

    class Device:
        def __init__(self, *, i2c_bus: "I2C.Bus", addr: int, freq: int = 400000, timeout: int = 50000):
            ...

        def set_mem_addr_size(self, num_bits: int, /) -> None:
            ...

        def readinto(self, buf: AnyWritableBuf, stop: bool = True, /) -> None:
            ...

        def write(self, buf: AnyWritableBuf, stop: bool = True, /) -> int:
            ...

        def readfrom(self, nbytes: int, stop: bool = True, /) -> bytes:
            ...

        def readfrom_into(self, buf: AnyWritableBuf, stop: bool = True, /) -> None:
            ...

        def writeto(self, buf: AnyReadableBuf, stop: bool = True, /) -> int:
            ...

        def writevto(self, vector: Sequence[AnyReadableBuf], stop: bool = True, /) -> int:
            ...

        def readfrom_mem(self, memaddr: int, nbytes: int, /) -> bytes:
            ...

        def readfrom_mem_into(self, memaddr: int, buf: AnyWritableBuf, /) -> None:
            ...

        def writeto_mem(self, memaddr: int, buf: AnyReadableBuf, /) -> None:
            ...

        def get_bus(self) -> "I2C.Bus":
            ...


class SPI:
    MSB: ClassVar[int] = ...
    LSB: ClassVar[int] = ...

    class Bus:
        def __init__(
            self, *,
            host: int,
            sck: int | str,
            mosi: int | str,
            miso: int | str,
            dual_pins: tuple[int | str, int | str] | None = None,
            quad_pins: tuple[int | str, int | str, int | str, int | str] | None = None,
            octal_pins: tuple[int | str, int | str, int | str, int | str, int | str, int | str, int | str, int | str] | None = None
        ):
            ...

    class Device:

        def __init__(self, *, spi_bus: "SPI.Bus", freq: int, cs: int | str, polarity: int = 0,
                     phase: int = 0, bits: int = 8, firstbit: int = _machine.SPI.MSB, dual: bool = False,
                     quad: bool = False, octal: bool = False, cs_high_active: bool = False):
            ...

        read = _machine.SPI.read
        readinto = _machine.SPI.readinto
        write = _machine.SPI.write
        write_readinto = _machine.SPI.write_readinto

        def readfrom_mem_into(self, memaddr: int, buf: AnyWritableBuf, /) -> None:
            ...

        def writeto_mem(self, memaddr: int, buf: AnyReadableBuf, /) -> None:
            ...

        def readfrom_mem(self, memaddr: int, nbytes: int, /) -> bytes:
            ...

        def set_mem_addr_size(self, num_bits: int, /) -> None:
            ...

        def get_bus(self) -> "SPI.Bus":
            ...

