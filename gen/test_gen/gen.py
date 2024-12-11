# -*- coding: utf-8 -*-

import sys

try:
    from pycparser import c_ast  # NOQA
except ImportError:
    sys.stderr.write(
        '\nThe pycparser library is missing, '
        'please run "pip install pycparser" to install it.\n'
    )
    sys.stderr.flush()
    sys.exit(-500)

from pycparser.c_generator import CGenerator

generator = CGenerator()

BASIC_TYPES = [
    'float',
    'double',
    'long',
    'ulong',
    'unsigned long',
    'long double',
    'signed long double',
    'unsigned long double',
    'long long',
    'signed long long',
    'unsigned long long',
    'int',
    'uint',
    'signed int',
    'unsigned int',
    'long int',
    'signed long int',
    'unsigned long int',
    'short'
    'ushort',
    'signed short',
    'unsigned short',
    'void',
    'char',
    'uchar',
    'signed char',
    'unsigned char',
    'bool'
]

STDLIB_TYPES = [
    'size_t',
    'uint8_t',
    'uint16_t',
    'uint32_t',
    'uint64_t',
    'int8_t',
    'int16_t',
    'int32_t',
    'int64_t',
    'va_list',
    'uintptr_t',
    'intptr_t',
]


def _repr(obj):
    if isinstance(obj, list):
        if len(obj) >= 1:
            if isinstance(obj[0], str):
                res = ', '.join(repr(item) for item in obj)
            else:
                res = [
                    '\n'.join(f'  {line}' for line in _repr(e).split('\n'))
                    for e in obj
                ]
                res = ',\n'.join(res)
                res = f'\n{res}\n'
        else:
            res = ''

        return f'[{res}]'
    else:
        return repr(obj)


def Node__repr__(self):
    """ Generates a python representation of the current node
    """

    if isinstance(self, Constant):
        return repr(self.value)

    result = f'{self.__class__.__name__}('
    res = []

    if isinstance(self, IdentifierType):
        return f"'{' '.join(self.new_name)}'"

    for name in self.__slots__[:-2]:
        if name == 'body':
            dta = f"{name}=''"
        elif 'name' in name and hasattr(self, 'new_name'):
            dta = f'{name}={_repr(self.new_name)}'
        else:
            dta = f'{name}={_repr(getattr(self, name))}'

        res.append(dta)

    try:
        res.append(f"parent='{self.parent.__class__.__name__}'")
    except AttributeError:
        res.append('parent=?')

    res = ',\n'.join(res)

    if isinstance(self, c_ast.FileAST) or len(self.__slots__[:-2]) > 1 or res.count('\n'):
        res = '\n'.join(f'  {line}' for line in res.split('\n'))
        result += f'\n{res}\n)'
    else:
        result += f'{res})'

    return result


c_ast._repr = _repr
setattr(c_ast.Node, '__repr__', Node__repr__)


def filter_node(n):
    if hasattr(n, 'coord') and n.coord is not None:
        if 'fake_libc_include' in n.coord.file:
            return True
        if '_private.h' in n.coord.file:
            return True

    try:
        name = n.new_name
    except AttributeError:
        pass
    else:
        if isinstance(name, str) and name.startswith('_'):
            return True

    return False


def strip_lv(name):
    if not isinstance(name, str):
        return name

    if name.lower().startswith('lv_') or name.lower().startswith('_lv_'):
        name = name[3:]

    return name


arraydecl_types = set()


class ArrayDecl(c_ast.ArrayDecl):
    '''
    as a function argument
        Decl(
          name='props',
          quals=['const'],
          align=[],
          storage=[],
          funcspec=[],
          type=ArrayDecl(
            type=TypeDecl(
              declname='props',
              quals=['const'],
              align=None,
              type=IdentifierType(names=['lv_style_prop_t'])
            ),
            dim=None,
            dim_quals=[]
          ),
          init=None,
          bitsize=None
        ),


    as the field in a struct/union
    Decl(
          name='stops',
          quals=[],
          align=[],
          storage=[],
          funcspec=[],
          type=ArrayDecl(
            type=TypeDecl(
              declname='stops',
              quals=[],
              align=None,
              type=IdentifierType(names=['lv_gradient_stop_t'])
            ),
            dim=Constant(
              type='int',
              value='2'
            ),
            dim_quals=[]
          ),
          init=None,
          bitsize=None
        ),

    '''

    def __init__(self, type, dim, dim_quals, coord=None):
        self._parent = None
        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(type, dim, dim_quals, coord)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    def process(self):
        """
        if parent is None:
            arraydecl_types: {}
        else:
            arraydecl_types: {'TypeDecl', 'PtrDecl', 'ArrayDecl'}
        """

    def __repr__(self):
        arraydecl_types.add(self.type.__class__.__name__)
        return super().__repr__()


class Constant(c_ast.Constant):
    '''

    Constant(
        type='int',
        value='8'
    ),

    '''

    def __init__(self, type, value, coord=None):
        self._parent = None

        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(type, value, coord)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


decl_types = set()


class Decl(c_ast.Decl):
    '''
    Decl(
      name='mp_lv_init_gc',
      quals=[],
      align=[],
      storage=['extern'],
      funcspec=[],
      type=FuncDecl(
        args=None,
        type=TypeDecl(
          declname='mp_lv_init_gc',
          quals=[],
          align=None,
          type=IdentifierType(names=['void'])
        )
      ),
      init=None,
      bitsize=None
    )
    '''

    def __init__(self, name, quals, align, storage, funcspec, type, init, bitsize, coord=None):
        self._parent = None

        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(name, quals, align, storage, funcspec, type, init, bitsize, coord)
        # funcspec not used
        # init not used
        # align is not used
        # storage is not used
        # bitsize is used in struct/union fields

    @property
    def new_name(self):
        return strip_lv(self.name)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    def process(self):
        """
        if parent is None:
            decl_types: {'TypeDecl', 'Enum', 'FuncDecl', 'PtrDecl', 'Struct'}
        else:
            decl_types: {'TypeDecl', 'PtrDecl', 'Enum', 'Struct', 'FuncDecl', 'ArrayDecl'}
        """

    def __repr__(self):
        decl_types.add(self.type.__class__.__name__)
        return super().__repr__()


class EllipsisParam(c_ast.EllipsisParam):

    def __init__(self, coord=None):
        self._parent = None
        super().__init__(coord)
        # used only in function parameters

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


class Enum(c_ast.Enum):
    '''
    anonymous
    Decl(
      name=None,
      quals=[],
      align=[],
      storage=[],
      funcspec=[],
      type=Enum(
        name=None,
        values=EnumeratorList(
          enumerators=[
            Enumerator(
              name='ENUM_LV_LOG_LEVEL_TRACE',
              value=Constant(
                type='int',
                value='0'
              )
            )
          ]
        )
      ),
      init=None,
      bitsize=None
    )

    anonymous that is typedef
    Typedef(
      name='lv_anim_enable_t',
      quals=[],
      storage=['typedef'],
      type=TypeDecl(
        declname='lv_anim_enable_t',
        quals=[],
        align=None,
        type=Enum(
          name=None,
          values=EnumeratorList(
            enumerators=[
              Enumerator(
                name='LV_ANIM_OFF',
                value=None
              ),
              Enumerator(
                name='LV_ANIM_ON',
                value=None
              )
            ]
          )
        )
      )
    )

    named enum

    Decl(
      name=None,
      quals=[],
      align=[],
      storage=[],
      funcspec=[],
      type=Enum(
        name='_lv_align_t',
        values=EnumeratorList(
          enumerators=[
            Enumerator(
              name='LV_ALIGN_DEFAULT',
              value=Constant(
                type='int',
                value='0'
              )
            ),
            ...

    the enum above is also a forward declaration for the following typedef
    Typedef(
      name='lv_align_t',
      quals=[],
      storage=['typedef'],
      type=TypeDecl(
        declname='lv_align_t',
        quals=[],
        align=None,
        type=IdentifierType(names=['uint8_t'])
      )
)
    '''

    def __init__(self, name, values, coord=None):
        self._parent = None

        for value in values or []:
            try:
                value.parent = self
            except AttributeError:
                pass

        super().__init__(name, values, coord)

    @property
    def new_name(self):
        return strip_lv(self.name)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


class Enumerator(c_ast.Enumerator):
    '''
    Enumerator(
        name='LV_ALIGN_DEFAULT',
        value=Constant(
            type='int',
            value='0'
        )
    ),
    '''

    def __init__(self, name, value, coord=None):
        self._parent = None
        super().__init__(name, value, coord)

    @property
    def new_name(self):
        return strip_lv(self.name)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


class EnumeratorList(c_ast.EnumeratorList):

    def __init__(self, enumerators, coord=None):
        self._parent = None
        super().__init__(enumerators, coord)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


class FileAST(c_ast.FileAST):

    def __init__(self, ext, coord=None):
        self._parent = None
        super().__init__(ext, coord)

        for n in ext:
            if filter_node(n):
                continue

            print(n)

        print()
        print()

        print('decl_types:', decl_types)
        print('funcdecl_types:', funcdecl_types)
        print('funcdef_types:', funcdef_types)
        print('ptrdecl_types:', ptrdecl_types)
        print('typedecl_types:', typedecl_types)
        print('typedef_types:', typedef_types)
        print('typename_types:', typename_types)
        print('arraydecl_types:', arraydecl_types)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


funcdecl_types = set()


class FuncDecl(c_ast.FuncDecl):
    '''
    Decl(
        name='lv_version_info',
        quals=['const'],
        align=[],
        storage=['static'],
        funcspec=['inline'],
        type=FuncDecl(
            args=ParamList(
                params=[
                    Typename(
                        name=None,
                        quals=[],
                        align=None,
                        type=TypeDecl(
                            declname=None,
                            quals=[],
                            align=None,
                            type=IdentifierType(names=['void'])
                        )
                    )
                ]
            ),
            type=PtrDecl(
                quals=[],
                type=TypeDecl(
                    declname='lv_version_info',
                    quals=['const'],
                    align=None,
                    type=IdentifierType(names=['char'])
                )
            )
        )
    )
        
    '''

    def __init__(self, args, type, coord=None):
        self._parent = None

        for arg in args or []:
            try:
                arg.parent = self
            except AttributeError:
                pass

        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(args, type, coord)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    def process(self):
        """
        if parent is None:
            funcdecl_types: {}
        else:
            funcdecl_types: {'TypeDecl', 'PtrDecl'}
        """

    @property
    def decl(self):

        mp_params = []

        mp_conversions = []

        for arg in self.args or []:
            if isinstance(arg, Decl):
                lv_param_name = arg.name
                mp_param_name = lv_param_name + '_in'
                mp_param = 'mp_obj_t ' + mp_param_name
                mp_params.append(mp_param)

                ptr = ''
                param_type = arg.type
                while isinstance(param_type, PtrDecl):
                    ptr += '*'
                    param_type = param_type.type

                if (
                    isinstance(param_type, TypeDecl) and
                    isinstance(param_type.type, IdentifierType)
                ):
                    param_type = str(param_type.type)
                    if ptr:
                        param_type += ' ' + ptr + lv_param_name
                    else:
                        param_type += ' ' + ptr + lv_param_name



        self.type.declname

    @property
    def type_name(self):
        return self.type.type_name

    def __repr__(self):
        funcdecl_types.add(self.type.__class__.__name__)
        return super().__repr__()


funcdef_types = set()


class FuncDef(c_ast.FuncDef):

    def __init__(self, decl, param_decls, body, coord=None):
        self._parent = None
        super().__init__(decl, param_decls, body, coord)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    def process(self):
        """
        if parent is None:
            funcdef_types: {'Decl'}
        else:
            funcdef_types: {'Decl'}
        """

    def __repr__(self):
        funcdef_types.add(self.decl.__class__.__name__)
        return super().__repr__()


class IdentifierType(c_ast.IdentifierType):

    def __init__(self, names, coord=None):
        self._parent = None
        super().__init__(names, coord)

    @property
    def new_name(self):
        return [strip_lv(name) for name in self.names]

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


class ParamList(c_ast.ParamList):

    def __init__(self, params, coord=None):
        self._parent = None

        super().__init__(params, coord)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


ptrdecl_types = set()


class PtrDecl(c_ast.PtrDecl):

    def __init__(self, quals, type, coord=None):
        self._parent = None

        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(quals, type, coord)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    def process(self):
        """
        if parent is None:
            ptrdecl_types: {}
        else:
            ptrdecl_types: {'TypeDecl', 'FuncDecl', 'PtrDecl'}
        """

    @property
    def type_name(self):
        type_name = self.type.type_name

        if not isinstance(self.type, FuncDecl):
            if type_name.emdswith('*'):
                type_name += '*'
            else:
                type_name += '*'

        return type_name

    def __repr__(self):
        ptrdecl_types.add(self.type.__class__.__name__)
        return super().__repr__()


class Struct(c_ast.Struct):

    def __init__(self, name, decls, coord=None):
        self._parent = None

        for decl in decls or []:

            try:
                decl.parent = self
            except AttributeError:
                pass

        super().__init__(name, decls, coord)

    @property
    def new_name(self):
        return strip_lv(self.name)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


typedecl_types = set()


class TypeDecl(c_ast.TypeDecl):

    def __init__(self, declname, quals, align, type, coord=None):
        self._parent = None

        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(declname, quals, align, type, coord)

    @property
    def new_name(self):
        return strip_lv(self.declname)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    @property
    def type_name(self):



    def process(self):
        """
        if parent is None:
            typedecl_types: {}
        else:
            typedecl_types: {'IdentifierType', 'Enum', 'Union', 'Struct'}
        """

    def __repr__(self):
        typedecl_types.add(self.type.__class__.__name__)
        return super().__repr__()


typedef_types = set()


class Typedef(c_ast.Typedef):

    def __init__(self, name, quals, storage, type, coord=None):
        self._parent = None

        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(name, quals, storage, type, coord)

    @property
    def new_name(self):
        return strip_lv(self.name)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    def process(self):
        """
        if parent is None:
            typedef_types: {'TypeDecl', 'PtrDecl'}
        else:
            typedef_types: {'TypeDecl', 'PtrDecl'}
        """

    def __repr__(self):
        typedef_types.add(self.type.__class__.__name__)
        return super().__repr__()


typename_types = set()


class Typename(c_ast.Typename):

    def __init__(self, name, quals, align, type, coord=None):
        self._parent = None

        try:
            type.parent = self
        except AttributeError:
            pass

        super().__init__(name, quals, align, type, coord)

    @property
    def new_name(self):
        return strip_lv(self.name)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    def process(self):

        self.name = f'arg_{self.parent.arg_count}_in'
        self.type_name = ''

        if isinstance(self.type, TypeDecl):

            if str(self.type.type) == 'void':
                self.name = ''
                self.type_name = 'void'


        type_ = self.type
        while isinstance(type_, PtrDecl):



            TypeDecl(
                declname='free_cnt',
                quals=[],
                align=None,
                type='size_t',
                parent='Decl'
            )

            self.type.


        self.parent.arg_count += 1


        """
        if parent is None: 
            typename_types: {}
        else:
            typename_types: {'TypeDecl', 'PtrDecl'}
            
            FuncDecl.args
            Always used when the function parameter is not named.
            
            
            
        """

    def __repr__(self):
        typename_types.add(self.type.__class__.__name__)
        return super().__repr__()


class Union(c_ast.Union):

    def __init__(self, name, decls, coord=None):
        self._parent = None

        for decl in decls or []:
            try:
                decl.parent = self
            except AttributeError:
                pass

        super().__init__(name, decls, coord)

    @property
    def new_name(self):
        return strip_lv(self.name)

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value


for cls in (
    ArrayDecl,
    Constant,
    Decl,
    EllipsisParam,
    Enum,
    Enumerator,
    EnumeratorList,
    FileAST,
    FuncDecl,
    FuncDef,
    IdentifierType,
    ParamList,
    PtrDecl,
    Struct,
    TypeDecl,
    Typedef,
    Typename,
    Union
):
    cls_name = cls.__name__
    setattr(getattr(sys.modules['pycparser.c_parser'], 'c_ast'), cls_name, cls)


import os
import sys
import subprocess

import create_fake_lib_c  # NOQA
import pycparser  # NOQA

def run():
    target_header = r'C:\Users\drsch\PycharmProjects\lvgl_micropython\lib\lvgl\lvgl.h'
    lvgl_config_path = r'C:\Users\drsch\PycharmProjects\lvgl_micropython\lib'

    lvgl_path = r'C:\Users\drsch\PycharmProjects\lvgl_micropython\lib\lvgl'
    lvgl_src_path = os.path.join(lvgl_path, 'src')

    target_header_base_name = (
        os.path.splitext(os.path.split(target_header)[-1])[0]
    )

    pp_file = target_header_base_name + '.pp'
    include_dirs = [lvgl_path, lvgl_config_path]

    if sys.platform.startswith('win'):
        try:
            import pyMSVC  # NOQA
        except ImportError:
            sys.stderr.write(
                '\nThe pyMSVC library is missing, '
                'please run "pip install pyMSVC" to install it.\n'
            )
            sys.stderr.flush()
            sys.exit(-500)

        env = pyMSVC.setup_environment()  # NOQA
        print(env)

        cpp_cmd = ['cl', '/std:c11', '/nologo', '/P']
        output_pp = f'/Fi"{pp_file}"'
        include_path_env_key = 'INCLUDE'

    elif sys.platform.startswith('darwin'):
        include_path_env_key = 'C_INCLUDE_PATH'
        cpp_cmd = [
            'clang', '-std=c11', '-E', '-DINT32_MIN=0x80000000',
        ]
        output_pp = f' >> "{pp_file}"'
    else:
        include_path_env_key = 'C_INCLUDE_PATH'
        cpp_cmd = [
            'gcc', '-std=c11', '-E', '-Wno-incompatible-pointer-types',
        ]
        output_pp = f' >> "{pp_file}"'

    fake_libc_path = create_fake_lib_c.run(os.getcwd())

    if include_path_env_key not in os.environ:
        os.environ[include_path_env_key] = ''

    os.environ[include_path_env_key] = (
        f'{fake_libc_path}{os.pathsep}{os.environ[include_path_env_key]}'
    )

    if 'PATH' not in os.environ:
        os.environ['PATH'] = ''

    os.environ['PATH'] = (
        f'{fake_libc_path}{os.pathsep}{os.environ["PATH"]}'
    )

    cpp_cmd.extend([
        '-DLV_LVGL_H_INCLUDE_SIMPLE',
        '-DLV_CONF_INCLUDE_SIMPLE',
        '-DLV_USE_DEV_VERSION'
    ])

    cpp_cmd.extend(['-DPYCPARSER', f'"-I{fake_libc_path}"'])
    cpp_cmd.extend([f'"-I{item}"' for item in include_dirs])
    cpp_cmd.append(f'"{target_header}"')

    if sys.platform.startswith('win'):
        cpp_cmd.insert(len(cpp_cmd) - 2, output_pp)
    else:
        cpp_cmd.append(output_pp)

    cpp_cmd = ' '.join(cpp_cmd)

    p = subprocess.Popen(
        cpp_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=os.environ,
        shell=True
    )
    out, err = p.communicate()
    exit_code = p.returncode

    if not os.path.exists(pp_file):
        sys.stdout.write(out.decode('utf-8').strip() + '\n')
        sys.stdout.write('EXIT CODE: ' + str(exit_code) + '\n')
        sys.stderr.write(err.decode('utf-8').strip() + '\n')
        sys.stdout.flush()
        sys.stderr.flush()

        raise RuntimeError('Unknown Failure')

    with open(pp_file, 'r') as f:
        pp_data = f.read()

    cparser = pycparser.CParser()
    ast = cparser.parse(pp_data, target_header)


run()
