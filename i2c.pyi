from typing import Optional, Union
from typing import TYPE_CHECKING


if TYPE_CHECKING:
    import array


_BUFFER_TYPE = Union[bytearray, bytes, memoryview, array.array]


class I2CBus(object):

    def __init__(
        self,
        scl: int,
        sda: int,
        freq: int = 4000000,
        host: Optional[int] = None,
        use_pullups: bool = False,
        use_locks: bool = False
    ):
        ...

    def __enter__(self) -> "I2CBus":
        ...

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        ...

    def scan(self):
        ...

    def start(self) -> None:
        ...

    def stop(self) -> None:
        ...

    def readinto(self, buf: _BUFFER_TYPE,  nack: bool = True) -> None:
        ...

    def write(self, buf: _BUFFER_TYPE) -> None:
        ...

    def readfrom(self, addr: int, nbytes: int, stop: bool = True) -> bytearray:
        ...

    def readfrom_into(self, addr: int, buf: _BUFFER_TYPE, stop: bool = True) -> None:
        ...

    def writeto(self, addr: int, buf: _BUFFER_TYPE, stop: bool = True) -> None:
        ...

    def readfrom_mem(self, addr: int, memaddr: int, nbytes: int, addrsize: int = 8) -> bytearray:
        ...

    def readfrom_mem_into(self, addr: int, memaddr: int, buf: _BUFFER_TYPE, addrsize: int = 8) -> None:
        ...

    def writeto_mem(self, addr: int, memaddr: int, buf: _BUFFER_TYPE, addrsize: int = 8) -> None:
        ...


class I2CDevice(object):
    _bus: I2CBus = ...
    dev_id: int = ...
    _reg_bits: int = ...

    def __init__(self, bus: I2CBus, dev_id: int, reg_bits: int = 8):
        ...

    def read_mem(self, memaddr: int, num_bytes: Optional[int] = None, buf: Optional[_BUFFER_TYPE] = None) -> Optional[bytearray]:
        ...

    def write_mem(self, memaddr, buf):
        with self._bus:
            self._bus.writeto_mem(
                self.dev_id,
                memaddr,
                buf,
                addrsize=self._reg_bits
            )

    def read(self, nbytes: Optional[int] = None, buf: Optional[_BUFFER_TYPE] = None) -> Optional[bytearray]:
        ...

    def write(self, buf: _BUFFER_TYPE) -> None:
        ...
