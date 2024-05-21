from typing import Union, Final, Literal
from enum import IntEnum



TYPE_I8: Final[int] = 0x11
TYPE_U8: Final[int] = 0x01
TYPE_I16: Final[int] = 0x12
TYPE_U16: Final[int] = 0x02
TYPE_I32: Final[int] = 0x14
TYPE_U32: Final[int] = 0x04
TYPE_I64: Final[int] = 0x18
TYPE_U64: Final[int] = 0x08
TYPE_STR: Final[int] = 0x21
TYPE_BLOB: Final[int] = 0x42
TYPE_FLOAT: Final[int] = 0xFE


class NVS:

    def __init__(self, name: str, /):
        ...

    def set(self, type: int, key: str, value: Union[int, float, str, bytes, memoryview, bytearray]) -> None:
        ...

    def get(self, type: int, key: str) ->  Union[int, float, str, bytes]:
        ...

    def erase(self, key: str) -> None:
        ...

    def commit(self) -> None:
        ...
