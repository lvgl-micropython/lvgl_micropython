from typing import TYPE_CHECKING

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types
    from .struct import Field


class Union(BaseJSON):
    anonymous_typedefs = []
    anonymous_decls = []

    def __init__(self, ctype: str, name: str | None, members: list["Field"]):
        super().__init__(ctype)

        self.name = name
        self.members = members

        if self.name is not None:
            if self.name not in converters.c_to_mp:
                converters.c_to_mp[self.name] = f'mp_read_{self.name}'
            if self.name not in converters.mp_to_c:
                converters.mp_to_c[self.name] = f'mp_write_{self.name}'

    @property
    def c_to_mp_byref(self):
        if self.name is None:
            return ''

        return converters.c_to_mp_byref[self.name]

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

    @property
    def definition(self):
        set_attrs = []
        get_attrs = []

        for field in self.members:
            get_attrs.append(field.c_to_mp)
            set_attrs.append(field.mp_to_c)

        if get_attrs or set_attrs:
            attr_func = templates.struct.attr_func(
                self.name,
                '\n'.join(get_attrs),
                '\n'.join(set_attrs)
                )
            attr_func_decl = templates.struct.attr_func_decl(self.name)
        else:
            attr_func = ''
            attr_func_decl = ''

        return templates.struct.definition(
            self.name,
            clean(self.name),
            attr_func,
            attr_func_decl
        )

    @property
    def module_decl(self):
        return [templates.struct.module_decl(clean(self.name), self.name)]


