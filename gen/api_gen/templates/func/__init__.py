from .. import Template, DynamicLoader, DynamicAttribute


_dyanamic_loader = DynamicLoader(__name__)


if __name__ == '__main__':

    def c_func_call(return_type: str, arg_types: str, arg_names: str) -> str:
        pass

    def helpers() -> str:
        pass

    def definition(c_func_name: str, arg_converters: str, return_type: str,
             c_function_call: str, return_converter: str, mp_func_decls: str, /) -> str:
        ...

    def MP_DEFINE_CONST_C_FUN_OBJ(mp_func_name: str, arg_count: int, c_func_name: str, /) -> str:
        ...

    def arg_converter(arg_type: str, arg_name: str, arg_converter: str, arg_index: int, /) -> str:
        ...

    def module_decl(py_func_name: str, mp_func: str, /) -> str:
        pass
