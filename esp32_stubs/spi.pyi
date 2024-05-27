from typing import Optional, Final, Any, Callable


def get_dma_buffer(size_in: int, /) -> memoryview:
    ...

def free_dma_buffer(buf_in: memoryview, /) -> None:

    ...

class Bus(object):

    def __init__(self):
        ...

    def get_host(
        self,
        host: int,
        /,
        mosi: Optional[int] = None,
        miso: Optional[int] = None,
        sck: Optional[int] = None,
        wp: Optional[int] = None,
        hd: Optional[int] = None,
        data4: Optional[int] = None,
        data5: Optional[int] = None,
        data6: Optional[int] = None,
        data7: Optional[int] = None,
        dual: Optional[bool] = False
    ) -> int:
        ...

    def deinit(self) -> None:
        ...

class Device(object):

    MSB: Final[int] = 0
    LSB: Final[int] = 1

    def __init__(
        self,
        spi_bus: Bus,
        baudrate: int,
        /,
        cs: int = -1,
        polarity: int = 0,
        phase: int = 0,
        bits: int = 8,
        firstbit: int = MSB,
        three_wire: bool = False,
        cs_active_pos: bool = False,
        half_duplex: bool = False,
        clock_as_cs: bool = False,
        queue_size: int = 5
    ):
        ...

    def read(self, read_buf: memoryview, /) -> None:
        ...

    def write(self, write_buf: memoryview, /) -> None:
        ...

    def write_read(self, write_buf: memoryview, read_buf: memoryview, /) -> None:
        ...

    def register_trans_start_cb(self, callback: Callable, user_data: Optional[Any] = None) -> None:
        ...

    def register_trans_end_cb(self, callback: Callable, user_data: Optional[Any] = None) -> None:
        ...

    def get_bus(self) -> Bus:
        ...

    def get_host(self) -> int:
        ...

    def deinit(self) -> None:
        ...
