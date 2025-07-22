from .. import DynamicLoader, DynamicAttribute


_dyanamic_loader = DynamicLoader(__name__)


if __name__ == '__main__':

    def module_decl(py_member_name: str, c_member_name: str, /) -> str:
        ...

    def definition(c_member_name: str, /) -> str:
        ...

    def str_module_decl(py_member_name: str, c_member_name: str, /) -> str:
        ...


