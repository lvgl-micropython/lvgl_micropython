from . import get_py_name, BaseJSON
from .. import templates
from . import converters


class EnumMember(BaseJSON):

    def __init__(self, ctype: str, name: str):
        super().__init__(ctype)

        self.name = name

    @property
    def module_decl(self):
        if self.name.startswith('LV_SYMBOL'):
            return templates.enum.str_module_decl(get_py_name(self.name), self.name)
        else:
            return templates.enum.module_decl(get_py_name(self.name), self.name)

    @property
    def definition(self):
        if self.name.startswith('LV_SYMBOL'):
            return templates.enum.definition(self.name)
        else:
            return ''


class Enum(BaseJSON):

    def __init__(self, ctype: str, name: str | None, members: list["EnumMember"]):
        super().__init__(ctype)

        self._name = name
        self.members = members

        if name is not None:
            if name not in converters.mp_to_c:
                converters.mp_to_c[name] = f'({name})mp_obj_get_int'

            if name not in converters.c_to_mp:
                converters.c_to_mp[name] = 'mp_obj_new_int'

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):

        if self._name is not None:
            del converters.mp_to_c[self._name]
            del converters.c_to_mp[self._name]

        if value is not None:
            converters.mp_to_c[value] = f'({value})mp_obj_get_int'
            converters.c_to_mp[value] = 'mp_obj_new_int'

        self._name = value

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
        if self.name is None:
            return ''

        return 'int'

    @property
    def module_decl(self):
        res = [member.module_decl for member in self.members]
        return res

    @property
    def definition(self):
        res = [member.definition for member in self.members if member.definition]

        if res:
            return '\n'.join(res)
        else:
            return ''

