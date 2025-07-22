from typing import TYPE_CHECKING

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types


class Type(BaseJSON):

    def __init__(self, ctype: str, name: str):
        super().__init__(ctype)

        if name is None:
            self._name = None
            self.pointer = ''
        else:
            ptr = ''
            while name.endswith('*'):
                ptr += '*'
                name = name[:-1]

            self.pointer = ptr
            self._name = name.strip()

    @property
    def name(self):
        if self.pointer:
            return f'{self._name} {self.pointer}'

        return self._name

    @property
    def c_to_mp(self):
        if self.name is None:
            return ''

        if self.name in converters.c_to_mp:
            return converters.c_to_mp[self.name]

        return ''

    @property
    def mp_to_c(self):
        if self.name is None:
            return ''

        if self.name in converters.mp_to_c:
            return converters.mp_to_c[self.name]

        return ''

    @property
    def c_to_py(self):
        if self.name is None:
            return ''

        return 'int'

