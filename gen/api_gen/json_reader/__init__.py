import json


NOT_ALLOWED = ('False', 'None', 'True', 'and', 'as', 'assert', 'break', 'class',
               'continue', 'def', 'del', 'elif', 'else', 'except', 'finally',
               'for', 'from', 'global', 'if', 'import', 'in', 'is', 'lambda',
               'nonlocal', 'not', 'or', 'pass', 'raise', 'return', 'try',
               'while', 'with', 'yield')


def clean(name, kwlist=NOT_ALLOWED):
    if name in kwlist:
        result = "_%s" % name
    else:
        result = name

    result = result.strip()
    result = result.replace(' ', '_')
    result = result.replace('*', '_ptr')
    result = result.replace('lv_', '', 1)
    return result


class JSONMeta(type):

    def __call__(cls, **kwargs):
        for key, value in kwargs.items():
            if isinstance(value, dict):
                try:
                    ctype = value['ctype']
                except KeyError:
                    print(kwargs)
                    raise
                cls_name = ctype.replace('_', ' ').title().replace(' ', '')
                kwargs[key] = globals()[cls_name](**value)
            elif isinstance(value, list):
                for i, item in enumerate(value):
                    if isinstance(item, dict):
                        ctype = item['ctype']
                        cls_name = ctype.replace('_', ' ').title().replace(' ', '')
                        value[i] = globals()[cls_name](**item)

        instance = super().__call__(**kwargs)

        for key, value in kwargs.items():
            if isinstance(value, BaseJSON):
                value.parent = instance
            elif isinstance(value, list):
                for item in value:
                    if isinstance(item, BaseJSON):
                        item.parent = instance

        return instance


class BaseJSON(metaclass=JSONMeta):

    def __init__(self, ctype):
        self.ctype = ctype
        self.cls = None
        self._parent = None

    def process(self):
        pass

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

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

    def __str__(self):
        res = [f'{self.__class__.__name__}(']

        for key, value in self.__dict__.items():
            if key.startswith('_'):
                continue

            if key in ('anonymous_typedefs', 'anonymous_decls', 'cls'):
                continue

            if isinstance(value, list):
                if value:
                    res.append(f'    {key}=[')
                    for item in value:
                        if isinstance(item, BaseJSON):
                            item = str(item).split('\n')
                            item = '\n'.join('        ' + line for line in item)
                            res.append(item + ',')
                        else:
                            res.append(f'        {repr(item)},')
                    res.append('    ],')
                else:
                    res.append(f'    {key}=[],')
            elif isinstance(value, BaseJSON):
                value = str(value)
                value = value.split('\n')
                value = '\n    '.join(value)
                res.append(f'    {key}={value},')
            else:
                res.append(f'    {key}={repr(value)},')

        res = '\n'.join(res)
        res = res[:-1] + '\n)'
        return res

def get_py_name(name):
    name = name.replace('lv_', '')
    name = name.replace('LV_', '')
    name = name.replace('ENUM_', '')
    return name


from .type import Type
from .func import Func, Param, GroupedFunc, Ellipsis
from .struct import Struct, Field
from .union import Union
from .array import Array
from .typedef import Typedef
from .var import Var
from .enum import Enum, EnumMember
from .callback import Callback


from typing import TYPE_CHECKING, Union as _Union

if TYPE_CHECKING:
    json_types = _Union[Array, Struct, Union, Type, Callback]


class CGenerator:

    def __init__(self):
        self.typedefs = {}
        self.used_typedefs = set()
        self.module_decls = []
        self.struct_definitions = []
        self.union_definitions = []
        self.function_definitions = []
        self.variable_definitions = []
        self.enum_definitions = []

    def _iter_json(self, dct, is_base=False):
        for key, value in list(dct.items()):
            if isinstance(value, dict):
                self._iter_json(value)
            elif isinstance(value, list):
                for itm_ in value:
                    if isinstance(itm_, dict):
                        self._iter_json(itm_)

            elif isinstance(value, str):
                if value in self.typedefs:
                    if is_base:
                        pass
                    else:
                        dct.clear()
                        dct['ctype'] = 'type'

                    dct['name'] = self.typedefs[value]['name']
                    self.used_typedefs.add(value)
                    continue
                elif value.endswith('*'):
                    tmp_value, pointer = value.rsplit(' ', 1)

                    if tmp_value in self.typedefs:
                        dct.clear()
                        dct['ctype'] = 'type'
                        dct['name'] = self.typedefs[tmp_value]['name'] + ' ' + pointer
                        self.used_typedefs.add(tmp_value)
                        continue

    def _read(self, json_data: dict):
        sorted_api = self._sort(json_data)

        for typedef in sorted_api['typedef'][:]:
            type_name = typedef['type']['name']

            if type_name == typedef['name']:
                sorted_api['typedef'].remove(typedef)
                continue

            if type_name is None:
                continue

            if type_name.startswith('_lv') or type_name.startswith('lv_'):
                self.typedefs[type_name] = typedef

        for k, v in sorted_api.items():
            if k == 'typedef':
                continue

            for item in v:
                self._iter_json(item, is_base=True)

        for typedef in self.used_typedefs:
            if self.typedefs[typedef]['type']['ctype'] in ('union', 'struct', 'enum'):
                if self.typedefs[typedef]['type']['members']:
                    sorted_api[self.typedefs[typedef]['type']['ctype']].insert(0, self.typedefs[typedef]['type'])

            del self.typedefs[typedef]

        for typedef in sorted_api['typedef']:
            typedef_type_name = typedef['type']['name']

            if typedef_type_name in self.typedefs:
                continue

            sorted_api['typedef'].remove(typedef)

        sorted_api['union'] = []

        for typedef in sorted_api['typedef'][:]:
            if typedef['type']['ctype'] in ('struct', 'enum', 'union'):
                sorted_api['typedef'].remove(typedef)

                typedef['type']['name'] = typedef['name']
                sorted_api[typedef['type']['ctype']].append(typedef['type'])

        return sorted_api

    def _sort(self, data):
        res = {}

        for itm_ in data:
            ctype = itm_['ctype']
            if ctype not in res:
                res[ctype] = []

            res[ctype].append(itm_)

        return res

    @staticmethod
    def _get_obj(py_data, target_name):
        for obj_ in py_data['struct']:
            if obj_.name == target_name:
                return obj_

        for obj_ in py_data['union']:
            if obj_.name == target_name:
                return obj_

        for obj_ in py_data['enum']:
            if obj_.name == target_name:
                return obj_

        for obj_ in py_data['typedef']:
            if obj_.name == target_name:
                return obj_

        for obj_ in py_data['callback']:
            if obj_.name == target_name:
                return obj_

        return None

    def process(self, py_data):

        for obj in py_data['struct']:
            for field in obj.members:
                field.process()
                field.type.cls = self._get_obj(py_data, field.type.name)

        for obj in py_data['union']:
            for field in obj.members:
                field.process()
                field.type.cls = self._get_obj(py_data, field.type.name)

        for obj in py_data['callback']:
            for param in obj.params:
                param.type.cls = self._get_obj(py_data, param.type.name)

            obj.type.cls = self._get_obj(py_data, obj.type.name)

        for obj in py_data['func']:
            for param in obj.params:
                param.type.cls = self._get_obj(py_data, param.type.name)

            obj.type.cls = self._get_obj(py_data, obj.type.name)

        for obj in py_data['grouped_func']:
            for param in obj.params:
                param.type.cls = self._get_obj(py_data, param.type.name)

            obj.type.cls = self._get_obj(py_data, obj.type.name)

    @staticmethod
    def parse(json_data: dict):
        self = CGenerator()
        sorted_api = self._read(json_data)

        from .func import group_functions

        py_json_objs = {}

        for type_name, values in sorted_api.items():
            py_json_objs[type_name] = []

            for json_obj in values:
                json_type = json_obj['ctype']
                json_type = json_type.replace('_', '').title().replace(' ', '')

                inst = globals()[json_type](**json_obj)
                py_json_objs[type_name].append(inst)

        py_json_objs = group_functions(py_json_objs)

        for i, json_obj in enumerate(py_json_objs['grouped_func']):
            json_type = json_obj['ctype']
            json_type = json_type.replace('_', ' ').title().replace(' ', '')

            inst = globals()[json_type](**json_obj)
            py_json_objs['grouped_func'][i] = inst

        module_decls = self.module_decls
        struct_definitions = self.struct_definitions
        union_definitions = self.union_definitions
        function_definitions = self.function_definitions
        variable_definitions = self.variable_definitions
        enum_definitions = self.enum_definitions


        self.process(py_json_objs)

        # for key, value in py_json_objs.items():
        #     print('********')
        #     print(key)
        #     print('********')
        #     for item in value:
        #         print(str(item))
        #
        # print()
        # print()

        for enum in py_json_objs['enum']:
            module_decls.extend(enum.module_decl)
            enum_definitions.append(enum.definition)

        for struct in py_json_objs['struct']:
            struct_definitions.append(struct.definition)
            module_decls.extend(struct.module_decl)

        for union in py_json_objs['union']:
            union_definitions.append(union.definition)
            module_decls.extend(union.module_decl)

        for function in py_json_objs['grouped_func']:
            function_definitions.append(function.definition)
            module_decls.extend(function.module_decl)

        for variable in py_json_objs['var']:
            variable_definitions.append(variable.definition)
            module_decls.extend(variable.module_decl)

        self.module_decls = ',\n'.join(str(item) for item in module_decls if item)
        self.struct_definitions = '\n'.join(str(item) for item in struct_definitions if item)
        self.union_definitions = '\n'.join(str(item) for item in union_definitions if item)
        self.function_definitions = '\n'.join(str(item) for item in function_definitions if item)
        self.variable_definitions = '\n'.join(str(item) for item in variable_definitions if item)
        self.enum_definitions = '\n'.join(str(item) for item in enum_definitions if item)

        return self


if __name__ == '__main__':
    import json
    import pp_to_json

    with open(r'lvgl.pp', 'r') as f:
        _data = f.read()

    s_api, s_api_p = pp_to_json.pp_to_json(_data, r'../../lib/lvgl/lvgl.h')

    gf = group_functions(s_api)

    for item in s_api['typedef']:
        ctype = item['ctype'].replace('_', ' ').title().replace(' ', '')
        instance = globals()[ctype](**item)

    for item in s_api['enum']:
        ctype = item['ctype'].replace('_', ' ').title().replace(' ', '')
        instance = globals()[ctype](**item)

    out_funcs = []
    out_def = []

    for key, _data in gf.items():
        if not isinstance(key, int):
            continue

        ctype = _data['ctype'].replace('_', ' ').title().replace(' ', '')
        instance = globals()[ctype](**_data)
        out_funcs.append(instance.definition)
        out_def.extend(instance.module_decl)

    print('\n\n'.join(out_funcs))
    print()
    print()
    print(',\n'.join(out_def))

    # print(json.dumps(gf, indent=4))

