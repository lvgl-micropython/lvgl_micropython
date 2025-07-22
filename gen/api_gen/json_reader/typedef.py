from typing import TYPE_CHECKING

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types


class Typedef(BaseJSON):

    def __init__(self, ctype: str, name: str, type: "json_types", storage: list[str]):
        super().__init__(ctype)

        self.name = name
        self.type = type
        self.storage = storage

        if self.type is None:
            print(name)
            raise RuntimeError

        if self.name is not None:
            if self.name not in converters.mp_to_c:
                converters.mp_to_c[self.name] = self.type.mp_to_c

            if self.name not in converters.c_to_mp:
                converters.c_to_mp[self.name] = self.type.c_to_mp

    @property
    def c_to_mp(self):
        if self.name is None:
            return ''

        return converters.c_to_mp[self.name]

    @property
    def mp_to_c(self):
        if self.name is None:
            return ''

        return converters.mp_to_c[self.name]

    @property
    def c_to_py(self):
        return ''
