from .. import Template, DynamicLoader, DynamicAttribute


class _DynamicLoader(DynamicLoader):
    attr_func_decl = DynamicAttribute('\n    attr, mp_{0}_attr,')


_dyanamic_loader = _DynamicLoader(__name__)


if __name__ == '__main__':

    def definition(c_struct_name: str, struct_name: str, attr_func: str,
            attr_func_decl: str, /) -> str:
        ...

    def attr_func(c_struct_name: str, struct_attr_getters: str,
                  struct_attr_setters: str, /) -> str:
        ...

    def attr_func_decl(c_struct_name: str, /) -> str:
        ...

    def set_attr_cb(field: str, type_name: str, cast: str, decl_name: str,
                    lv_callback: str, struct_name: str, user_data: str, /) -> str:
        ...

    def get_attr_cb(field: str, type_name: str, funcptr: str, cast: str,
                    decl_name: str,  lv_callback: str, struct_name: str,
                    user_data: str, /) -> str:
        ...

    def set_attr_array(field: str, type_name: str, mp_to_lv_convertor: str,
                       cast: str, decl_name: str, size: str, /) -> str:
        ...

    def get_attr_array(field: str, type_name: str, lv_to_mp_convertor: str,
                       cast: str, decl_name: str, /) -> str:
        ...

    def set_attr(field: str, type_name: str, mp_to_lv_convertor: str, cast: str,
                 decl_name: str, /) -> str:
        ...

    def get_attr(field: str, type_name: str, lv_to_mp_convertor: str, cast: str,
                 decl_name: str, /) -> str:
        ...


    def module_decl(struct_name: str, c_struct_name: str, /) -> str:
        ...