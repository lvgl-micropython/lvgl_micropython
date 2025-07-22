from typing import TYPE_CHECKING

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types


class Var(BaseJSON):
    def __init__(self, ctype: str, type: "json_types", name: str, quals: list[str]):
        super().__init__(ctype)

        self.type = type
        self.name = name
        self.quals = quals

    @property
    def c_to_mp(self):
        return ''

    @property
    def mp_to_c(self):
        return ''

    @property
    def c_to_py(self):
        return ''
