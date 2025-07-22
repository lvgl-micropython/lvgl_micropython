from typing import TYPE_CHECKING

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types


class Array(BaseJSON):

    def __init__(self, ctype: str, type: "json_types", dim: int):
        super().__init__(ctype)
        self.type = type
        self.dim = dim

    @property
    def name(self):
        type_ = self.type

        while isinstance(type_, Array):
            type_ = type_.type

        return type_.name

    @name.setter
    def name(self, value):
        type_ = self.type

        while isinstance(type_, Array):
            type_ = type_.type

        type_.name = value

    @property
    def c_to_mp(self):
        return ''

    @property
    def mp_to_c(self):
        return ''

    @property
    def c_to_py(self):
        return ''

    @property
    def module_decl(self):
        return []

    @property
    def definition(self):
        return ''
