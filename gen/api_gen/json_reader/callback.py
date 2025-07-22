from typing import TYPE_CHECKING

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types
    from .func import Param


class Callback(BaseJSON):
    def __init__(self, ctype: str, quals: list[str], type: "json_types",
                 name: str, params: list["Param"]):

        super().__init__(ctype)

        self.quals = quals
        self.type = type
        self.name = name
        self.params = params

    @property
    def c_to_mp_byref(self):
        if self.name is None:
            return ''

        return converters.c_to_mp_byref[self.name]

    @property
    def c_to_mp(self):
        return ''

    @property
    def mp_to_c(self):
        return ''

    @property
    def c_to_py(self):
        return ''
