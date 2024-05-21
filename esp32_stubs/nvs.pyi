from typing import Union, Final


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
        """
        Constructor

        :param name: This is the name you give for a group of
                     data you want to store
        """
        ...

    def set(self, type: int, key: str, value: Union[int, float, str, bytes, memoryview, bytearray]) -> None:
        """
        Stores data.

        :param type: One of TYPE_* constants
        :param key: The name of the data to set
        :param value: The data
        """
        ...

    def get(self, type: int, key: str) ->  Union[int, float, str, bytes]:
        """
        Gets stored data

        :param type: One of TYPE_* constants
        :param key: The name of the data to get
        """
        ...

    def erase(self, key: str) -> None:
        """
        Erases a single entry supplied by `key`.
        """
        ...

    def commit(self) -> None:
        """
        Commits changes made.
        """
        ...

    def close(self) -> None:
        """
        Closes the connection to the NVS.

        This is automatically done if the NVS gets garbage collected.
        """
        ...

    def reset(self) -> None:
        """
        Erases all stored data and their keys.
        """
        ...
