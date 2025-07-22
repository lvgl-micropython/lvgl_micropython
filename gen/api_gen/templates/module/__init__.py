from .. import DynamicLoader


_dyanamic_loader = DynamicLoader(__name__)


if __name__ == '__main__':

    def decl(decls: str) -> str:
        ...

    def definition() -> str:
        pass


