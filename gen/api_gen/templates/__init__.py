




class __DynamicLoader:

    class __DynamicAttribute:

        def __init__(self, data):
            self._data = data

        def __get__(self, instance, owner):
            return Template(self._data)

    def __init__(self):
        import sys

        mod = sys.modules[__name__]

        for key, value in mod.__dict__.items():

            if key.startswith('__') and key.endswith('__'):
                setattr(self, key, value)

        self.__original__module__ = mod

        sys.modules[__name__] = self

    def __getattr__(self, item):
        if item in self.__dict__:
            return self.__dict__[item]

        if item + '.c' in os.listdir(BASE_PATH):
            value = Template(file_name=item + '.c')
            setattr(self, item, value)
            return value

        raise AttributeError(item)

    @staticmethod
    def c_name(*args) -> str:
        return '_'.join(list(args))

    MP_DEFINE_STR_OBJ = __DynamicAttribute('MP_DEFINE_STR_OBJ(mp_{0}, {1});')
    MP_ROM_INT = __DynamicAttribute('MP_ROM_INT({0})')
    sizeof = __DynamicAttribute('sizeof({0})')
    pointer = __DynamicAttribute('{0} *')
    const_pointer = __DynamicAttribute('const {0} *')
    const = __DynamicAttribute('const {0}')
    mp_read_byref = __DynamicAttribute('mp_read_byref_{0}')
    mp_read = __DynamicAttribute('mp_read_{0}')
    mp_write = __DynamicAttribute('mp_write_{0}')
    mp_read_ptr = __DynamicAttribute('mp_read_ptr_{0}')
    mp_write_ptr = __DynamicAttribute('mp_write_ptr_{0}')
    typedef_typeof = __DynamicAttribute('typedef __typeof__( (({0}*)(0))->{1} ) {2};')
    mp_lv = __DynamicAttribute('mp_lv_{0}')

    cb_decl = __DynamicAttribute('static {0} {1}({2});')
    cb_arg_conv = __DynamicAttribute('mp_args[{0}] = {1}({2}arg{0});')
    mp_arr_to = __DynamicAttribute('mp_arr_to_{0}')
    mp_arr_from = __DynamicAttribute('mp_arr_from_{0}')

    func_ptr_name = __DynamicAttribute('funcptr_{0}')
    func_ptr_macro = __DynamicAttribute('#define {0} NULL\n')
    func_ptr_def = __DynamicAttribute('static inline mp_obj_t mp_lv_{0}(void *func){{ return mp_lv_funcptr(&mp_{0}_mpobj, func, NULL, MP_QSTR_, NULL); }}\n')

    cast_member = __DynamicAttribute('{ MP_ROM_QSTR(MP_QSTR___cast__), MP_ROM_PTR(&cast_obj_class_method) }')
    member = __DynamicAttribute('{{ MP_ROM_QSTR(MP_QSTR_{0}), MP_ROM_PTR(&mp_{1}_mpobj) }}')
    enum_member = __DynamicAttribute('{{ MP_ROM_QSTR(MP_QSTR_{0}), MP_ROM_PTR({1}) }}')
    enum_type_member = __DynamicAttribute('{{ MP_ROM_QSTR(MP_QSTR_{0}), MP_ROM_PTR(&mp_lv_{1}_type_base) }}')
    size_member = __DynamicAttribute('{{ MP_ROM_QSTR(MP_QSTR___SIZE__), MP_ROM_PTR(MP_ROM_INT(sizeof({0}{1}))) }},')
    func_member = __DynamicAttribute('{{ MP_ROM_QSTR(MP_QSTR_{0}), MP_ROM_PTR(&mp_{1}_mpobj) }}')


__dynamic_loader = __DynamicLoader()


import os  # NOQA

BASE_PATH = os.path.dirname(__file__)


class Template(str):

    def __init__(self, value='', file_name=''):
        pass

    def __new__(cls, value='', file_name=''):
        if file_name:
            with open(os.path.join(BASE_PATH, file_name), 'r') as f:
                value = f.read()

            if '/*#~' in value:
                value = value.replace('{', '{{').replace('}', '}}')
                value = value.replace('/*#~', '{').replace('~#*/', '}')

        super().__init__(cls, value)

    def __call__(self, *args, **kwargs):
        return self.format(*args, **kwargs)


if __name__ == '__main__':
    helper_functions: Template = ...
    objects: Template = ...
    headers: Template = ...
    struct: Template = ...

    MP_ROM_INT: Template = ...
    MP_DEFINE_STR_OBJ: Template = ...
    sizeof: Template = ...
    pointer: Template = ...
    const_pointer: Template = ...
    const: Template = ...
    mp_read_byref: Template = ...
    mp_read: Template = ...
    mp_write: Template = ...
    mp_read_ptr: Template = ...
    mp_write_ptr: Template = ...
    typedef_typeof: Template = ...
    mp_lv: Template = ...

    cb_decl: Template = ...
    cb_arg_conv: Template = ...

    write_case_cb: Template = ...
    write_case: Template = ...
    write_case_array: Template = ...

    read_case_cb: Template = ...
    read_case: Template = ...
    read_case_array: Template = ...

    mp_arr_to: Template = ...
    mp_arr_from: Template = ...

    array_conv_dim: Template = ...
    array_conv_no_dim: Template = ...
    array_conv: Template = ...

    func_ptr_name: Template = ...
    func_ptr_macro: Template = ...
    func_ptr_def: Template = ...

    helper_struct: Template = ...

    cast_member: Template = ...


    def member(method_name: str, method: str, /) -> str:
        ...


    def enum_member(enum_member_name: str, enum_member_value: str, /) -> str:
        ...


    def enum_type_member(name: str, type_: str, /) -> str:
        ...


    def callback_func(func_name: str, func_prototype: str, func_args: str, num_args: int,
                      return_type: str, return_value: str, return_value_assignment: str,
                      build_args: str, user_data: str, /) -> str:
        ...

    def func_arg_conv_1(arg_name: str, index: int, callback_name: str, full_user_data: str,
                        containing_struct: str, user_data_getter: str, user_data_setter: str, /) -> str:
        ...


    def func_arg_conv_2(var_name: str, cast: str, convertor: str, index: int, /) -> str:
        ...


    def func_obj_decl(func_name: str, func_obj_name: str, func_ptr: str,
                      param_count: int, builtin_macro: str, /) -> str:
        ...


    def extension_def(module_name: str, print_func: str, func: str, build_args: str, build_result: str,
                      func_ptr: str, send_args: str, build_return_value: str, /) -> str:
        ...

    def func_error(problem: str, method: str, /) -> str:
        ...

    def ctor(obj: str, ctor_name: str, /) -> str:
        ...

    def object_def(module_name: str, obj: str, locals_dict_entries: str, ctor: str, make_new: str,
                   binary_op: str, buffer: str, parent: str, lv_class: str, /) -> str:
        ...

    def global_def(module_name: str, global_name: str, struct_name: str, cast: str, /) -> str:
        ...

    def size_member(struct_tag: str, struct_name: str, /) -> str:
        ...

    def size_attr(sanitized_struct_name: str, struct_size: str, functions: str, /) -> str:
        ...

    def func_member(name: str, func: str, /) -> str:
        ...


    def c_name(*args) -> str:
        ...

