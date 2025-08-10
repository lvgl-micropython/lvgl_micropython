

py_names = {
    'va_list': '*args',
    'void *': 'Any',
    'void *[]': 'Any',
    'char *[]': 'List[str]',
    'uint8_t *': 'Union[str, List[int]]',
    'uint16_t *': 'List[int]',
    'uint32_t *': 'List[int]',
    'uint64_t *': 'List[int]',
    'int8_t *': 'List[int]',
    'int16_t *': 'List[int]',
    'int32_t *': 'List[int]',
    'int64_t *': 'List[int]',
    'const uint8_t *': 'str',
    'const uint16_t *': 'List[int]',
    'const uint32_t *': 'List[int]',
    'const uint64_t *': 'List[int]',
    'const uint8_t **': 'List[bytes]',
    'const uint16_t **': 'List[List[int]]',
    'const uint32_t **': 'List[List[int]]',
    'const uint64_t **': 'List[List[int]]',
    'const int8_t *': 'List[int]',
    'const int16_t *': 'List[int]',
    'const int32_t *': 'List[int]',
    'const int64_t *': 'List[int]',
    'const int8_t **': 'List[List[int]]',
    'const int16_t **': 'List[List[int]]',
    'const int32_t **': 'List[List[int]]',
    'const int64_t **': 'List[List[int]]',
    'const void *': 'Any',
    'bool': 'bool',
    'char *': 'str',
    'char **': 'List[str]',
    'const char *': 'str',
    'const char **': 'List[str]',
    'uint8_t': 'int',
    'uint16_t': 'int',
    'uint32_t': 'int',
    'uint64_t': 'int',
    'unsigned': 'int',
    'unsigned int': 'int',
    'unsigned char': 'str',
    'const unsigned char': 'str',
    'unsigned char *': 'str',
    'unsigned char **': 'List[str]',
    'const unsigned char *': 'str',
    'const unsigned char **':  'List[str]',
    'unsigned short': 'int',
    'unsigned long': 'int',
    'unsigned long int': 'int',
    'unsigned long long': 'int',
    'unsigned long long int': 'int',

    'const unsigned short': 'int',
    'const unsigned long': 'int',
    'const unsigned long int': 'int',
    'const unsigned long long': 'int',
    'const unsigned long long int': 'int',


    'int8_t': 'int',
    'int16_t': 'int',
    'int32_t': 'int',
    'int64_t': 'int',
    'size_t': 'int',
    'int': 'int',
    'char': 'str',

    'short': 'int',
    'long': 'int',
    'long int': 'int',
    'long long': 'int',
    'long long int': 'int',

    'const short': 'int',
    'const long': 'int',
    'const long int': 'int',
    'const long long': 'int',
    'const long long int': 'int',

    'short *': 'List[int]',
    'long *': 'List[int]',
    'long int *': 'List[int]',
    'long long *': 'List[int]',
    'long long int *': 'List[int]',

    'const short *': 'List[int]',
    'const long *': 'List[int]',
    'const long int *': 'List[int]',
    'const long long *': 'List[int]',
    'const long long int *': 'List[int]',

    'void': 'None',
    'float': 'float',
}

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
        for key, value in list(kwargs.items()):
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

            elif key == 'name' and value is not None:
                pointer = ''
                while value.endswith('*'):
                    pointer += '*'
                    value = value[:-1]

                value = value.strip()
                kwargs['pointer'] = pointer
                kwargs[key] = value

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

    def __init__(self, ctype, pointer=''):
        self.ctype = ctype
        self.pointer = pointer
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


var_template = '{name}: {type} = ...'


class Var(BaseJSON):

    def __init__(self, ctype: str, type: "json_types", name: str, quals: list[str], pointer: str = ''):
        super().__init__(ctype, pointer)

        self.type = type
        self.name = name
        self.quals = quals

    @property
    def py_name(self):
        name = self.name
        if self.pointer:
            name += ' ' + self.pointer

        name = py_names.get(name, clean(self.name))

        return name

    def definition(self):
        return var_template.format(name=clean(self.name), type=self.type.py_name)


class Typedef(BaseJSON):

    def __init__(self, ctype: str, name: str, type: "json_types", storage: list[str], pointer: str = ''):
        super().__init__(ctype, pointer)

        self.name = name
        self.type = type
        self.storage = storage

    @property
    def py_name(self):
        name = self.name
        if self.pointer:
            name += ' ' + self.pointer

        name = py_names.get(name, clean(self.name))

        return name



class Type(BaseJSON):

    def __init__(self, ctype: str, name: str, pointer: str = ''):
        super().__init__(ctype, pointer)

        self.name = name

    @property
    def py_name(self):
        name = self.name
        if self.pointer:
            name += ' ' + self.pointer

        name = py_names.get(name, clean(self.name))

        return name


struct_type_template = '''
class _{cls_name}_type(TypedDict):
{fields}
'''

widget_template = '''
class {cls_name}({parent_cls}):
    {classes}

    def __init__(self, parent: obj, /):
       ...
       
    {properties}
    {methods}
'''


struct_template = '''
class {cls_name}({parent_cls}):
{classes}
        
    def __init__(self, values: _{cls_name}_type = dict(), /):
       ...

{properties}
{methods}
'''

property_get_set_template = '''
    @property
    def {prop_name}(self) -> "{type}":
        ...
        
    @{prop_name}.setter
    def {prop_name}(self, value: "{type}"):
        ...
'''

property_get_template = '''
    @property
    def {prop_name}(self) -> "{type}":
        ...
'''

property_set_template = '''
    def {prop_name}(self, value: "{type}"):
        ...
    
    {prop_name} = property(fset={prop_name})
'''


class Struct(BaseJSON):

    def __init__(self, ctype: str, name: str | None, members: list["Field"], pointer: str = ''):
        super().__init__(ctype, pointer)

        self.name = name
        self.members = members
        self.methods = []
        self.parent_cls = 'object'

        self.is_obj = False
        self.enums = []

    def add_enum(self, enum):
        self.enums.append(enum)

    def is_enum_compatable(self, enum):
        common_name = None

        for member in enum.members:
            if common_name is None:
                common_name = member.py_name
            else:
                name = member.py_name.split('_')
                c_name = common_name.split('_')

                for i in range(min(len(name), len(c_name))):
                    if name[i] != c_name[i]:
                        break
                else:
                    raise RuntimeError

                common_name = '_'.join(name[:i + 1])

        if common_name.lower().startswith(clean(self.name)[:-2]):
            return True

        return False

    def definition(self, name=None):

        if name is None:
            if self.is_obj:
                name = clean(self.name)[:-2]
            else:
                name = clean(self.name)

        field_types = []

        field_classes = []

        field_type_dicts = []
        properties = []

        for field in self.members:
            if isinstance(field.type, (Struct, Union)):
                field_type_dict, field_def = field.type.definition(field.name)
                field_type_dicts.append(field_type_dict)

                field_def = '\n'.join('    ' + line for line in field_def.split('\n'))
                field_def += '\n    \n'
                field_def += f'{field.name} = {field.name}()'

                field_classes.append(field_def)

                type_name = f'_{field.name}_type'
                field_types.append(f'    {field.name}: {type_name}')
            else:
                field_types.append(f'    {field.name}: "{field.type.py_name}"')

                if hasattr(field.type, 'quals') and 'const' in field.type.quals:
                    properties.append(property_get_template.format(prop_name=field.name, type=field.type.py_name))
                else:
                    properties.append(property_get_set_template.format(prop_name=field.name, type=field.type.py_name))

        methods = '\n\n'.join(method for method in self.methods)

        methods = '\n'.join('    ' + line for line in methods.split('\n'))

        if not field_types:
            field_types.append('    pass')

        field_type_dicts.append(struct_type_template.format(cls_name=name, fields='\n'.join(field_types)))

        field_type_dicts = '\n\n'.join(field_type_dicts)
        properties = '\n\n'.join(properties)
        classes = '\n\n'.join(field_classes)

        if self.is_obj:
            return '', widget_template.format(cls_name=name, parent_cls=self.parent_cls, classes=classes, properties=properties, methods=methods)

        res = struct_template.format(cls_name=name, parent_cls=self.parent_cls, classes=classes, properties=properties, methods=methods)
        return field_type_dicts, res

    def add_method(self, func):

        if (
            func.name.endswith('_create') and
            func.params[0].type.name == 'lv_obj_t'
        ):
            self.is_obj = True
            if self.name != 'lv_obj_t':
                self.parent_cls = 'obj'
        else:
            self.methods.append(func)

        func.params.pop(0)

    def is_method(self, func):
        name = clean(self.name[:-2])

        if not func.name.startswith(name):
            return False

        if not func.params:
            return False

        param = func.params[0]

        if param.type.name != self.name:
            return False

        return True

    @property
    def py_name(self):
        if self.is_obj:
            return clean(self.name)[:-2]
        else:
            return clean(self.name)


class Union(Struct):
    pass


class Field(BaseJSON):
    def __init__(self, ctype: str, type: "json_types", name: str, quals: list[str], bits: int, pointer: str = ''):
        super().__init__(ctype, pointer)

        self.type = type
        self.name = name
        self.quals = quals
        self.bits = bits
        self.pointer = ''


func_template = '''
def {func_name}({args}) -> "{type}":
    ...
'''


class Func(BaseJSON):

    def __init__(self, ctype: str, quals: list[str], type: "json_types",
                 name: str, params: list["Param"], pointer: str = ''):

        super().__init__(ctype, pointer)

        self.quals = quals
        self.type = type
        self.name = name
        self.params = params

    def definition(self, common_name=None):

        if common_name is None:
            args = []
            name = clean(self.name)
        else:
            args = ['self']
            name = clean(self.name).replace(common_name, '', 1)[1:]

        for param in self.params:
            args.append(f'{param.name}: "{param.type.py_name}"')

        return func_template.format(
            func_name=name,
            args=', '.join(args),
            type=self.type.py_name
        )


class Param(BaseJSON):

    def __init__(self, ctype: str, type: "json_types",
                 name: str, quals: list[str], index: int, pointer: str = ''):

        super().__init__(ctype, pointer)

        self.type = type
        self.name = name
        self.quals = quals
        self.index = index


class Ellipsis(BaseJSON):

    def __init__(self, ctype: str, pointer: str = '', **_):
        super().__init__(ctype, pointer)

        self.name = ''

    @property
    def py_name(self):
        return '*args'


class EnumMember(BaseJSON):

    def __init__(self, ctype: str, name: str, pointer: str = ''):
        super().__init__(ctype, pointer)

        self.name = name

    @property
    def py_name(self):
        return get_py_name(self.name)


class Enum(BaseJSON):

    def __init__(self, ctype: str, name: str | None, members: list["EnumMember"], pointer: str = ''):
        super().__init__(ctype, pointer)

        self.name = name
        self.members = members

    @property
    def py_name(self):
        if self.name is not None:
            return clean(self.name)[:-2]
        return ''


class Callback(BaseJSON):
    def __init__(self, ctype: str, quals: list[str], type: "json_types",
                 name: str, params: list["Param"], pointer: str = ''):

        super().__init__(ctype, pointer)

        self.quals = quals
        self.type = type
        self.name = name
        self.params = params

    @property
    def py_name(self):
        return 'callable'


class Array(BaseJSON):

    def __init__(self, ctype: str, type: "json_types", dim: int, pointer: str = ''):
        super().__init__(ctype, pointer)
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
    def py_name(self):
        name = self.name
        if self.pointer:
            name += ' ' + self.pointer

        name = py_names.get(name, clean(self.name))

        name = f'"{name}"'
        type_ = self.type

        while isinstance(type_, Array):
            type_ = type_.type
            name = f'list[{name}]'

        return name


json_types = Array | Struct | Union | Type | Callback


if __name__ == '__main__':
    import json

    with open(r'C:\Users\drsch\PycharmProjects\lvgl_micropython\gen\api_gen\first_dump.json', 'r') as f:
        new_data = json.loads(f.read())


    data = {}

    for item in new_data:
        if item['ctype'] not in data:
            data[item['ctype']] = []

        data[item['ctype']].append(item)

    if 'union' not in data:
        data['union'] = []

    found = []

    for typedef in data['typedef'][:]:
        for struct in data['struct']:
            if struct['name'] == typedef['type']['name']:
                struct['name'] = typedef['name']
                found.append(typedef['name'])
                break
        else:
            for union in data['union']:
                if union['name'] == typedef['type']['name']:
                    union['name'] = typedef['name']
                    found.append(typedef['name'])
                    break
            else:
                for enum in data['enum']:
                    if enum['name'] == typedef['type']['name']:
                        enum['name'] = typedef['name']
                        found.append(typedef['name'])
                        break

    from copy import deepcopy

    for typedef in data['typedef']:
        if typedef['type']['ctype'] == 'struct' and typedef['name'] not in found:
            struct = deepcopy(typedef['type'])
            struct['name'] = typedef['name']
            data['struct'].append(struct)

        if typedef['type']['ctype'] == 'union' and typedef['name'] not in found:
            union = deepcopy(typedef['type'])
            union['name'] = typedef['name']
            data['union'].append(union)

    def iter_members(type_, objs):
        for itm in objs:
            for member in itm['members']:
                m_type = member['type']
                while m_type['ctype'] == 'array':
                    m_type = m_type['type']

                if m_type['name'] == type_['type']['name']:
                    if m_type['ctype'] in ('struct', 'union'):
                        member['type'] = deepcopy(type_['type'])
                        member['type']['name'] = type_['name']
                    else:
                        m_type['name'] = type_['name']


    def iter_params(type_, objs):
        for itm in objs:
            for param in itm['params']:
                p_type = param['type']
                while p_type['ctype'] == 'array':
                    p_type = p_type['type']

                if p_type['name'] == type_['type']['name']:
                    if p_type['ctype'] in ('struct', 'union'):
                        param['type'] = deepcopy(type_['type'])
                        param['type']['name'] = type_['name']
                    else:
                        p_type['name'] = type_['name']


    for typedef in data['typedef']:
        iter_members(typedef, data['struct'])
        iter_members(typedef, data['union'])
        iter_params(typedef, data['func'])
        iter_params(typedef, data['callback'])


    structs = []
    unions = []
    funcs = []
    vars = []

    for struct in data['struct']:
        structs.append(Struct(**struct))

    for union in data['union']:
        unions.append(Union(**union))

    for func in data['func']:
        func = Func(**func)
        for struct in structs:
            if struct.is_method(func):
                struct.add_method(func)
                break
        else:
            for union in unions:
                if union.is_method(func):
                    union.add_method(func)
                    break
            else:
                funcs.append(func)
    for var in data['var']:
        vars.append(Var(**var))

    for struct in structs:
        field_type, struct = struct.definition()
        if field_type:
            print(field_type)
        print(struct)
        print()

    for union in unions:
        field_type, union = union.definition()
        if field_type:
            print(field_type)
        print(union)
        print()

    for func in funcs:
        print(func.definition())
        print()

    for var in vars:
        print(var.definition())

'''
{
    'ctype': 'struct', 
    'name': 'lv_draw_triangle_dsc_t', 
    'members': [
        {
            'ctype': 'field', 
            'type': {
                'name': 'lv_draw_dsc_base_t', 
                'ctype': 'type'
            }, 
            'name': 'base', 
            'quals': [], 
            'bits': None
        }, 
        {
            'ctype': 'field', 
            'type': {
                'ctype': 'array', 
                'type': {
                    'name': 'lv_point_precise_t', 
                    'ctype': 'type'
                }, 
                'dim': 3
            }, 
            'name': 'p', 
            'quals': [], 
            'bits': None
        }, {'ctype': 'field', 'type': {'name': 'lv_color_t', 'ctype': 'type'}, 'name': 'color', 'quals': [], 'bits': None}, {'ctype': 'field', 'type': {'name': 'lv_opa_t', 'ctype': 'type'}, 'name': 'opa', 'quals': [], 'bits': None}, {'ctype': 'field', 'type': {'name': 'lv_grad_dsc_t', 'ctype': 'type'}, 'name': 'grad', 'quals': [], 'bits': None}]}
'''