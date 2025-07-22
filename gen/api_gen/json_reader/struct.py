from typing import TYPE_CHECKING

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types


class Struct(BaseJSON):

    anonymous_typedefs = []
    anonymous_decls = []

    def __init__(self, ctype: str, name: str | None, members: list["Field"]):
        super().__init__(ctype)

        self.name = name
        self.members = members

        if self.name is not None:
            if self.name not in converters.c_to_mp:
                converters.c_to_mp_byref[self.name] = f'mp_read_byref_{self.name}'
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
    def get_user_data(self):
        for param in self.members:
            if param.name == 'user_data':
                return 'user_data'

        return 'NULL'

    @property
    def definition(self):
        set_attrs = []
        get_attrs = []

        for field in self.members:
            get_attrs.append(field.c_to_mp)
            set_attrs.append(field.mp_to_c)

        if get_attrs or set_attrs:
            attr_func = templates.struct.attr_func(self.name, '\n'.join(get_attrs), '\n'.join(set_attrs))
            attr_func_decl = templates.struct.attr_func_decl(self.name)
        else:
            attr_func = ''
            attr_func_decl = ''

        return templates.struct.definition(self.name, clean(self.name), attr_func, attr_func_decl)

    @property
    def module_decl(self):
        return [templates.struct.module_decl(clean(self.name), self.name)]


class Field(BaseJSON):
    def __init__(self, ctype: str, type: "json_types", name: str, quals: list[str], bits: int):
        super().__init__(ctype)

        self.type = type
        self.name = name
        self.quals = quals
        self.bits = bits
        self.pointer = ''

    def process(self):
        from .union import Union
        from .type import Type

        if isinstance(self.type, (Struct, Union)):
            type_name = f'{self.parent.name[:-2]}_{self.name}_t'

            self.parent.anonymous_typedefs.append(
                f'typedef __typeof__( (({self.parent.name}*)(0))->{self.name} ) {type_name};'
            )
            self.type.name = type_name
            self.parent.anonymous_decls.append(self.type)

            self.type = Type(ctype='type', name=type_name)

    @property
    def is_struct(self):
        return isinstance(self.type.cls, Struct)

    @property
    def is_callback(self):
        from .callback import Callback

        return isinstance(self.type, Callback)

    @property
    def c_to_mp(self):
        from .array import Array
        from .type import Type
        from .callback import Callback

        if isinstance(self.type, Array):
            cast = '(void *)'
        else:
            cast = ''

        if isinstance(self.type, Array):
            ret = templates.struct.get_attr_array(clean(self.name), self.type.name,
                                                  self.type.c_to_mp, cast, self.name)

        elif isinstance(self.type, Type):
            ret = templates.struct.get_attr(clean(self.name), self.type.name,
                                            self.type.c_to_mp, cast, self.name)

        elif isinstance(self.type, Callback):

            # ret = templates.struct.get_attr_cb(clean(self.name), self.type, funcptr,
            #                                    cast, self.name, self.struct.name,
            #                                    self.type.name, self.name, self.struct.get_user_data)
            ret = '''\
            case MP_QSTR_{0}:
                // converting from callback {1}
                // NOT YET IMPLIMENTED
                break;'''.format(clean(self.name), self.type.name)
        else:
            print('ERROR:')
            print(str(self.type))
            print()
            return ''

        return ret

    @property
    def mp_to_c(self):
        from .array import Array
        from .type import Type
        from .callback import Callback

        if isinstance(self.type, Array) or '*' in self.type.name:
            cast = '(void*)'
        else:
            cast = ''

        if 'const' in self.quals:
            ret = ''

        elif isinstance(self.type, Array):
            ret = templates.struct.set_attr_array(
                    clean(self.name), self.type, self.type.mp_to_c, cast, self.name,
                    f'sizeof({self.type.type.name}) * {self.type.dim})')

        elif isinstance(self.type, Type):
            ret = templates.struct.set_attr(clean(self.name), self.type.name,
                                            self.type.mp_to_c, cast, self.name)

        elif isinstance(self.type, Callback):
            full_user_data_ptr = '&data->%s' % self.parent.get_user_data

            ret = templates.struct.set_attr_cb(clean(self.name), self.type.name,
                                               cast, self.name, self.name,
                                               self.parent.name, full_user_data_ptr)
        else:
            print('ERROR:')
            print(str(self.type))
            print()
            return ''

        return ret

    @property
    def c_to_py(self):
        return ''
