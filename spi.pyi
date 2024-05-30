from typing import Optional, Final, Any, Callable, Union


def get_dma_buffer(size_in: int, /) -> memoryview:
    ...


def free_dma_buffer(buf_in: memoryview, /) -> None:
    ...


class Bus(object):

    def __init__(self):
        ...

    def get_host(
        self,
        *,
        host: int,
        mosi: Optional[int] = None,
        miso: Optional[int] = None,
        sck: Optional[int] = None,
        wp: Optional[int] = None,
        hd: Optional[int] = None,
        data4: Optional[int] = None,
        data5: Optional[int] = None,
        data6: Optional[int] = None,
        data7: Optional[int] = None,
        dual: bool = False
    ) -> int:
        ...

    def deinit(self) -> None:
        ...

class Device(object):

    MSB: Final[int] = 0
    LSB: Final[int] = 1

    def __init__(
        self,
        *,
        spi_bus: Bus,
        freq: int,
        cs: int = -1,
        polarity: int = 0,
        phase: int = 0,
        firstbit: int = MSB,
        bits: int = 8,
        three_wire: bool = False,
        cs_high_active: bool = False,
        half_duplex: bool = False,
        clock_as_cs: bool = False,
        queue_size: int = 5
    ):
        ...

    def comm(
        self,
        *,
        tx_data: Optional[memoryview] = None,
        rx_data: Optional[memoryview] = None,
        cmd: Optional[int] = None,
        cmd_bits: Optional[int] = None,
        addr: Optional[int] = None,
        addr_bits: Optional[int] = None,
        callback: Optional[Callable[["Device", Any], None]] = None
    ) -> None:
        ...

    def register_trans_start_cb(
        self,
        callback: Union[Callable[["Device", Any], None], None],
        user_data: Optional[Any] = None
    ) -> None:
        ...

    def register_trans_end_cb(
        self,
        callback: Union[Callable[["Device", Any], None], None],
        user_data: Optional[Any] = None
    ) -> None:
        ...

    def get_bus(self) -> Bus:
        ...

    def get_host(self) -> int:
        ...

    def deinit(self) -> None:
        ...


del Callable
del Union
del Any
del Optional
del Final
