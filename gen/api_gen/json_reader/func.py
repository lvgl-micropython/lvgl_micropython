from typing import TYPE_CHECKING, Tuple

from . import clean, BaseJSON
from .. import templates
from . import converters

if TYPE_CHECKING:
    from . import json_types


class Func(BaseJSON):

    def __init__(self, ctype: str, quals: list[str], type: "json_types",
                 name: str, params: list[str]):

        super().__init__(ctype)

        self.quals = quals
        self.type = type
        self.name = name
        self.params = params

    @property
    def c_to_mp(self):
        return ''

    @property
    def mp_to_c(self):
        return ''

    @property
    def c_to_py(self):
        return ''


class Param(BaseJSON):

    def __init__(self, ctype: str, type: "json_types",
                 name: str, quals: list[str], index: int):

        super().__init__(ctype)

        self.type = type
        self.name = name
        self.quals = quals
        self.index = index

    @property
    def c_to_mp(self):
        if self.type.name in converters.c_to_mp:
            return converters.c_to_mp[self.type.name]

        return ''

    @property
    def mp_to_c(self):
        if self.type.name in converters.mp_to_c:
            return converters.mp_to_c[self.type.name]

        return ''

    @property
    def c_to_py(self):
        return ''

    @property
    def is_callback(self):
        from .callback import Callback

        return isinstance(self.type, Callback)


class Ellipsis(BaseJSON):

    def __init__(self, ctype: str, **_):
        super().__init__(ctype)

        self.name = ''

    @property
    def c_to_mp(self):
        return ''

    @property
    def mp_to_c(self):
        return ''

    @property
    def c_to_py(self):
        return '*args'


class GroupedFunc(BaseJSON):
    def __init__(self, ctype: str,  type: "json_types", quals: list[str],
                 params: list["Param"], signature: Tuple[str, tuple[str]],
                 param_names: list[str], func_names: list[str], name: str):

        super().__init__(ctype)

        self.type = type
        self.quals = quals
        self.params = params
        self.signature = signature
        self.param_names = param_names
        self.func_names = func_names
        self.name = name
        self.param_names = param_names
        self.func_names = func_names

    @property
    def module_decl(self):
        res = []

        for c_function in self.func_names:
            res.append(
                templates.func.module_decl(clean(c_function), c_function))

        return res

    @property
    def definition(self):
        arg_count = len(self.signature[1])
        arg_converters = []

        for i, param in enumerate(self.params):
            arg_converters.append(
                templates.func.arg_converter(self.signature[1][i],
                                             self.param_names[i],
                                             param.mp_to_c, i))

        if self.signature[0] == 'void':
            return_type = '    '
            return_converter = 'mp_const_none'
        else:
            return_type = f'    {self.signature[0]} _ret = '
            return_converter = f'{self.type.c_to_mp}(_ret)'

        c_function_call = templates.func.c_func_call(self.signature[0],
                                                     ', '.join(self.signature[1]),
                                                     ', '.join(self.param_names))

        mp_func_decls = []

        for c_function in self.func_names:
            mp_func_decls.append(
                templates.func.MP_DEFINE_CONST_C_FUN_OBJ(c_function, arg_count,
                                                         self.name))

        return templates.func.definition(self.name, '\n'.join(arg_converters),
                                   return_type, c_function_call,
                                   return_converter, '\n'.join(mp_func_decls))


def group_functions(json_objs):
    func_signatures = {}
    count = 0

    json_objs['grouped_func'] = []

    for func in json_objs['func']:
        if 'const' in func.quals:
            const = 'const '
        else:
            const = ''

        ret_type = f'{const}{func.type.name}'

        params = []
        param_names = []

        for j, param in enumerate(func.params):
            try:
                quals = param.quals[:]

                suffix = ''

                type_ = param.type

                while type_.ctype == 'array':
                    dim = type_.dim
                    if dim == -1:
                        dim = ''
                    else:
                        dim = str(dim)

                    suffix += f'[{dim}]'

                    type_ = type_.type

                quals.append(type_.name)

                if suffix:
                    quals.append(suffix)

                params.append(' '.join(quals))
                param_names.append(f'arg{j}')

            except KeyError:
                print(func)
                raise

        key = (ret_type, tuple(params))

        if key not in func_signatures:
            func_signatures[key] = count

            json_objs['grouped_func'].append(dict(
                ctype='grouped_func',
                type=func.type,
                quals=func.quals,
                params=func.params,
                signature=key,
                param_names=param_names,
                func_names=[],
                name=f'grouped_function_{count}'
            ))
            count += 1

        index = func_signatures[key]
        json_objs['grouped_func'][index]['func_names'].append(func.name)

    return json_objs
