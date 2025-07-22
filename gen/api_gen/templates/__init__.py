# Copyright (c) 2024 - 2025 Kevin G. Schlosser


class DynamicAttribute:

    def __init__(self, data):
        self._data = data

    def __get__(self, instance, owner):
        return Template(self._data)


class DynamicLoader:

    def __init__(self, name):
        import sys

        mod = sys.modules[name]

        import os  # NOQA

        self.__module_path__ = os.path.dirname(mod.__file__)

        for key, value in mod.__dict__.items():

            if key.startswith('__') and key.endswith('__'):
                setattr(self, key, value)

        self.__original__module__ = mod

        sys.modules[name] = self

    def __getattr__(self, item):
        if item in self.__dict__:
            return self.__dict__[item]

        if hasattr(self.__original__module__, item):
            return getattr(self.__original__module__, item)

        import os

        if item + '.c' in os.listdir(self.__module_path__):
            value = Template(file_name=os.path.join(self.__module_path__, item + '.c'))
            setattr(self, item, value)
            return value

        raise AttributeError(item)

    @staticmethod
    def c_name(*args) -> str:
        return '_'.join(list(args))


class _DynamicLoader(DynamicLoader):
    pass


_dynamic_loader = _DynamicLoader(__name__)


class Template(str):

    def __init__(self, value='', file_name=''):
        pass

    def __new__(cls, value='', file_name=''):
        if file_name:
            with open(file_name, 'r') as f:
                value = f.read()

            value = value.replace('{', '{{').replace('}', '}}')
            value = value.replace('/*#~', '{').replace('~#*/', '}')

        return super().__new__(cls, value)

    def __call__(self, *args):
        return self.format(*args)


from . import func as _func
from . import struct as _struct
from . import enum as _enum
from . import array as _array
from . import var as _var

_dynamic_loader.func = _func
_dynamic_loader.struct = _struct
_dynamic_loader.enum = _enum
_dynamic_loader.array = _array
_dynamic_loader.var = _var


if __name__ == '__main__':
    func = _func
    struct = _struct
    enum = _enum
    array = _array
    var = _var

    def module(include_header: str, str_enums: str, structs: str, unions: str,
               functions: str, variables: str, module_decls: str) -> str:
        ...




del _func
del _struct
del _enum
del _array
del _var
