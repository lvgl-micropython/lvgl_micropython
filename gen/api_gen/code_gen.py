# MIT license; Copyright (c) 2021 Amir Gonnen
# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# TODO
# - Array conversion improvements:
#   - return custom iterable object instead of Blob when converting to array
#   - check array dim on conversion
# - On print extensions, print the reflected internal representation of
#   the object (worth the extra ROM?)
# - Verify that when mp_obj is given it is indeed the right type (mp_lv_obj_t).
#   Report error if not. can be added to mp_to_lv.
# - Implement inheritance instead of embed base methods (how? seems it's not
#   supported, see https://github.com/micropython/micropython/issues/1159)
# - When converting mp to ptr (and vice versa), verify that types are
#   compatible. Now all pointers are casted to void*.


import collections
import copy
import functools
import json
import inspect
import sys
import os
import argparse
import subprocess
import re


def memoize(func):
    @functools.lru_cache(maxsize=1000000)
    def memoized(*args_, **kwargs):
        return func(*args_, **kwargs)
    return memoized


def eprint(*args_, **kwargs):
    print(*args_, file=sys.stderr, **kwargs)


script_path = os.path.dirname(os.path.abspath(__file__))
project_path = os.path.abspath(os.path.join(script_path, '..'))
pycparser_path = os.path.join(project_path, 'lib', 'pycparser')


sys.path.insert(0, os.path.abspath(pycparser_path))
from pycparser import c_parser, c_ast, c_generator  # NOQA
import pycparser  # NOQA


fake_libc_path = os.path.join(script_path, 'fake_libc')


# ---------------- monkey patch code for pycparser -------------------------
# this code cleans up the output when logging the objects created by pycparser
# It makes it easier to read

def _repr(obj):
    if isinstance(obj, list):
        if len(obj) >= 1:
            if isinstance(obj[0], str):
                res = ', '.join(repr(itm) for itm in obj)
            else:
                res = ['\n'.join(f'  {line}'
                       for line in _repr(e).split('\n')) for e in obj]

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
    result = f'{self.__class__.__name__}('
    res = []

    for n in self.__slots__[:-2]:
        dta = f'{n}={_repr(getattr(self, n))}'
        res.append(dta)

    res = ',\n'.join(res)

    if (
        isinstance(self, c_ast.FileAST) or
        len(self.__slots__[:-2]) > 1 or
        res.count('\n')
    ):
        res = '\n'.join(f'  {line}' for line in res.split('\n'))
        result += f'\n{res}\n)'
    else:
        result += f'{res})'

    return result


#
# Argument parsing
#
argParser = argparse.ArgumentParser()

argParser.add_argument('-I', '--include', dest='include',
                       help='Preprocesor include path',
                       metavar='<Include Path>', action='append', default=[])

argParser.add_argument('-D', '--define', dest='define',
                       help='Define preprocessor macro', metavar='<Macro Name>',
                       action='append', default=[])

argParser.add_argument('--module_name', dest='module_name',
                       help='Module name', metavar='<Module name string>',
                       action='store')

argParser.add_argument('--module_prefix', dest='module_prefix',
                       help='Module prefix that starts every function name',
                       metavar='<Prefix string>', action='store')

argParser.add_argument('--metadata', dest='metadata',
                       help='Optional file to emit metadata (introspection)',
                       metavar='<MetaData File Name>', action='store')

argParser.add_argument('--board', dest='board', help='Board or OS',
                       metavar='<Board or OS>', action='store', default='')

argParser.add_argument('--output', dest='output',
                       help='Output file path', metavar='<Output path>',
                       action='store')

argParser.add_argument('--debug', dest='debug',
                       help='enable debugging output', action='store_true')

argParser.add_argument('--header_file', dest='header',
                       action='store', default=None)

args, unknownargs = argParser.parse_known_args()

module_name = args.module_name
module_prefix = args.module_prefix if args.module_prefix else args.module_name
input_header = args.header
DEBUG = args.debug

lvgl_path = os.path.dirname(input_header)
private_header = os.path.join(lvgl_path, 'src', 'lvgl_private.h')

lv_config_path = os.path.abspath(os.path.join(lvgl_path, '..', 'lv_conf.h'))
gen_json_path = os.path.join(lvgl_path, 'scripts/gen_json')

sys.path.insert(0, gen_json_path)

original_nodes = {}


for key, value in c_ast.__dict__.items():
    if inspect.isclass(value):
        original_nodes[key] = value


import fixed_gen_json  # NOQA


json_ast = fixed_gen_json.run(
    lv_config_path,
    os.path.join(project_path, 'build', 'lvgl_header.h'),
    False
)

lvgl_json = json_ast.to_dict()


for key, value in original_nodes.items():
    setattr(c_ast, key, value)


c_ast._repr = _repr
setattr(c_ast.Node, '__repr__', Node__repr__)


pp_file = args.output.rsplit('.', 1)[0] + '.pp'

if DEBUG:
    log_file = open(os.path.join(os.path.dirname(__file__), 'log.txt'), 'w')


def LOG(*ags):
    if DEBUG:
        ags = ' '.join(str(ag) for ag in ags)
        log_file.write(ags + '\n')
        log_file.flush()


# this block of code is used to handle the generation of the preprocessor
# output. Since pycparser has the ability to call the compiler internally
# there is no need to do it from cmake. Data doesn't need to be going in
# and out of cmake like that when it can all be done in a single location.
# it makes things easier.

# This function only gets used when compiling under Windows for Windows.
# It collects SDL2 and reorganizes the folder structure so the SDK include path
# will be in alignment with what is used in Unix. The include macro for SDL2
# will not need to be changed if compiling under Windows.
def get_sdl2():
    import zipfile
    import io
    import os
    import sys

    path = os.getcwd()
    url = 'https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-devel-2.26.5-VC.zip'  # NOQA

    def get_path(n: str, pth: str) -> str:
        for file in os.listdir(pth):
            file = os.path.join(pth, file)

            if file.endswith(n):
                return file

            if os.path.isdir(file):
                res = get_path(name, file)
                if res:
                    return res

    import requests  # NOQA

    stream = io.BytesIO()

    with requests.get(url, stream=True) as r:
        r.raise_for_status()

        content_length = int(r.headers['Content-Length'])
        chunks = 0
        print()
        sys.stdout.write('\r' + str(chunks) + '/' + str(content_length))
        sys.stdout.flush()

        for chunk in r.iter_content(chunk_size=1024):
            stream.write(chunk)
            chunks += len(chunk)
            sys.stdout.write('\r' + str(chunks) + '/' + str(content_length))
            sys.stdout.flush()

    print()
    stream.seek(0)
    zf = zipfile.ZipFile(stream)

    for z_item in zf.infolist():
        for ext in ('.h', '.dll', '.lib'):
            if not z_item.filename.endswith(ext):
                continue

            zf.extract(z_item, path=path)
            break

    include_path = get_path('include', path)
    lib_path = get_path('lib\\x64', path)
    dll_path = get_path('SDL2.dll', lib_path)

    sdl_include_path = os.path.split(include_path)[0]
    if not os.path.exists(os.path.join(sdl_include_path, 'SDL2')):
        os.rename(include_path, os.path.join(sdl_include_path, 'SDL2'))

    zf.close()
    stream.close()

    return os.path.abspath(sdl_include_path), dll_path


# when compiling under Windows we want to set up a build system that
# points to all the right things to allow pycparser to work correctly
# when generating the preprocessor output. I have not yet fully determined
# the best way to handle the pyMSVC dependency as it is not needed for
# folks running any other OS except Windows.
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
    cpp_cmd = ['cl', '/std:c11', '/nologo', '/P']
    output_pp = f'/Fi"{pp_file}"'
    include_path_env_key = 'INCLUDE'

# elif sys.platform.startswith('darwin'):
#     include_path_env_key = 'C_INCLUDE_PATH'
#     cpp_cmd = [
#         'clang', '-std=c11', '-E', '-DINT32_MIN=0x80000000',
#     ]
#     output_pp = f' >> "{pp_file}"'
else:
    include_path_env_key = 'C_INCLUDE_PATH'
    cpp_cmd = [
        'gcc', '-std=c11', '-E', '-Wno-incompatible-pointer-types',
    ]
    output_pp = f' >> "{pp_file}"'


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

# cpp_cmd.extend(
#     [
#         '-DLV_LVGL_H_INCLUDE_SIMPLE',
#         '-DLV_CONF_INCLUDE_SIMPLE',
#         '-DLV_USE_DEV_VERSION'
#     ]
# )


cpp_cmd.extend([f'-D{define}' for define in args.define])
cpp_cmd.extend(['-DPYCPARSER', '-E', f'-I{fake_libc_path}'])
cpp_cmd.extend([f'-I{include}' for include in args.include])
cpp_cmd.append(f'"{input_header}"')


if sys.platform.startswith('win'):
    cpp_cmd.insert(len(cpp_cmd) - 2, output_pp)
else:
    cpp_cmd.append(output_pp)


cpp_cmd.extend(unknownargs)

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


#
# AST parsing helper functions
#
@memoize
def remove_declname(ast_):
    if hasattr(ast_, 'declname'):
        ast_.declname = None
    if isinstance(ast_, tuple):
        remove_declname(ast_[1])
        return
    for i, c1 in enumerate(ast_.children()):
        child = ast_.children()[i]
        remove_declname(child)


@memoize
def add_default_declname(ast_, name_):
    if hasattr(ast_, 'declname'):
        if ast_.declname is None:
            ast_.declname = name_

    if isinstance(ast_, tuple):
        add_default_declname(ast_[1], name_)
        return

    for i, c1 in enumerate(ast_.children()):
        child = ast_.children()[i]
        add_default_declname(child, name_)


@memoize
def convert_array_to_ptr(ast_):
    if hasattr(ast_, 'type') and isinstance(ast_.type, c_ast.ArrayDecl):
        if hasattr(ast_.type, 'quals'):
            quals = ast_.type.quals
        else:
            quals = []

        ast_.type = c_ast.PtrDecl(quals, ast_.type.type)

    if isinstance(ast_, tuple):
        return convert_array_to_ptr(ast_[1])

    for i, c1 in enumerate(ast_.children()):
        child = ast_.children()[i]
        convert_array_to_ptr(child)


@memoize
def remove_quals(ast_):
    if hasattr(ast_, 'quals'):
        ast_.quals = []

    if hasattr(ast_, 'dim_quals'):
        ast_.dim_quals = []

    if isinstance(ast_, tuple):
        return remove_quals(ast_[1])

    for i, c1 in enumerate(ast.children()):
        child = ast_.children()[i]

        # Don't remove quals which change function prototype
        if not isinstance(child, c_ast.FuncDecl):
            remove_quals(child)


@memoize
def remove_explicit_struct(ast_):
    if (
        isinstance(ast_, c_ast.TypeDecl) and
        isinstance(ast_.type, c_ast.Struct)
    ):
        explicit_struct_name = ast_.type.name

        if explicit_struct_name:
            if explicit_struct_name in explicit_structs:
                ast_.type = c_ast.IdentifierType(
                    [explicit_structs[explicit_struct_name]]
                )

            elif explicit_struct_name in structs:
                ast_.type = c_ast.IdentifierType([explicit_struct_name])

    if isinstance(ast_, tuple):
        return remove_explicit_struct(ast_[1])

    for i, c1 in enumerate(ast_.children()):
        child = ast_.children()[i]
        remove_explicit_struct(child)


@memoize
def get_type(arg, **kwargs):
    if isinstance(arg, str):
        return arg

    remove_quals_arg = 'remove_quals' in kwargs and kwargs['remove_quals']
    arg_ast = copy.deepcopy(arg)
    remove_explicit_struct(arg_ast)

    if remove_quals_arg:
        remove_quals(arg_ast)

    return gen.visit(arg_ast)


@memoize
def get_name(type_):
    if isinstance(type_, c_ast.Decl):
        return type_.name

    if (
        isinstance(type_, c_ast.Struct) and
        type_.name and
        type_.name in explicit_structs
    ):
        return explicit_structs[type_.name]

    if isinstance(type_, c_ast.Struct):
        return type_.name

    if isinstance(type_, c_ast.TypeDecl):
        return type_.declname

    if isinstance(type_, c_ast.IdentifierType):
        return type_.names[0]

    if isinstance(type_, c_ast.FuncDecl):
        return type_.type.declname

    if isinstance(type_, (c_ast.PtrDecl, c_ast.ArrayDecl)):
        return get_type(type_, remove_quals=True)

    return gen.visit(type_)


# Create a function prototype AST from a function AST
@memoize
def function_prototype(func):
    bare_func = copy.deepcopy(func)
    remove_declname(bare_func)

    ptr_decl = c_ast.PtrDecl(quals=[], type=bare_func.type)
    func_proto = c_ast.Typename(name=None, quals=[], align=[], type=ptr_decl)

    return func_proto


#
# module specific text patterns
# IGNORECASE and "lower" are used to match both function and enum names
#
base_obj_name = 'obj'
base_obj_type = f'{module_prefix}_{base_obj_name}_t'
lv_ext_pattern = (
    re.compile(f'^{module_prefix}_([^_]+)_ext_t')
)
lv_obj_pattern = (
    re.compile(f'^{module_prefix}_([^_]+)', re.IGNORECASE)
)
lv_func_pattern = (
    re.compile(f'^{module_prefix}_(.+)', re.IGNORECASE)
)
create_obj_pattern = (
    re.compile(f'^{module_prefix}_(.+)_create$')
)
lv_method_pattern = (
    re.compile(f'^{module_prefix}_[^_]+_(.+)', re.IGNORECASE)
)
lv_base_obj_pattern = (
    re.compile(f'^(struct _){{0,1}}{module_prefix}_{base_obj_name}_t( [*]){{0,1}}')  # NOQA
)
lv_str_enum_pattern = (
    re.compile(f'^_?{module_prefix.upper()}_STR_(.+)')
)
lv_callback_type_pattern = (
    re.compile(f'({module_prefix}_){{0,1}}(.+)_cb(_t){{0,1}}')  # NOQA
)
lv_global_callback_pattern = (
    re.compile('.*g_cb_t')
)
lv_func_returns_array = (
    re.compile('.*_array$')
)
lv_enum_name_pattern = (
    re.compile(f'^(ENUM_){{0,1}}({module_prefix.upper()}_){{0,1}}(.*)')  # NOQA
)


# Prevent identifier names which are Python reserved words (add underscore in such case)
def sanitize(
    id_,
    kwlist=['False', 'None', 'True', 'and', 'as', 'assert',  'break', 'class',
            'continue', 'def', 'del', 'elif',  'else', 'finally', 'if', 'raise',
            'for', 'from', 'except', 'global', 'import', 'in', 'is', 'lambda',
            'nonlocal', 'not', 'or', 'pass',  'return', 'try', 'while', 'with',
            'yield']
):

    if id_ in kwlist:
        result = "_%s" % id_
    else:
        result = id_

    result = result.strip()
    result = result.replace(' ', '_')
    result = result.replace('*', '_ptr')

    return result


@memoize
def simplify_identifier(id_):
    match_result = lv_func_pattern.match(id_)

    if match_result:
        return match_result.group(1)

    return id_


def ctor_name_from_obj_name(obj_name_):
    return f'{module_prefix}_{obj_name_}_create'


def is_method_of(func_name_, obj_name_):
    n = f'{module_prefix}_{obj_name_}_'.lower()
    return func_name_.lower().startswith(n)


def method_name_from_func_name(func_name_):
    res = lv_method_pattern.match(func_name_).group(1)
    if res != "del":
        return res

    # del is a reserved name, don't use it
    return "delete"


def get_enum_name(enum_):
    match_result = lv_enum_name_pattern.match(enum_)
    if match_result:
        return match_result.group(3)

    return enum_


def str_enum_to_str(str_enum):
    res = lv_str_enum_pattern.match(str_enum).group(1)
    return f'{module_prefix.upper()}_{res}'


def is_obj_ctor(func):
    # ctor name must match pattern
    if not create_obj_pattern.match(func.name):
        return False

    # ctor must return a base_obj type
    type_ = get_type(func.type.type, remove_quals=True)
    if not lv_base_obj_pattern.match(type_):
        return False

    # ctor must receive (at least) one base obj parameters
    args_ = func.type.args.params
    if len(args_) < 1:
        return False

    type_ = get_type(args_[0].type, remove_quals=True)
    if not lv_base_obj_pattern.match(type_):
        return False

    return True


def is_global_callback(arg_type):
    arg_type_str = get_name(arg_type.type)
    return lv_global_callback_pattern.match(arg_type_str)


def get_py_type(type_name):

    def _get_best_match(tn):
        best_match = ''

        for objnme in obj_names:
            if tn.startswith(objnme):
                if len(objnme) > len(best_match):
                    best_match = objnme

        return best_match

    type_name = type_name.replace('const lv_', 'lv_')
    type_name = type_name.replace('const _lv_', '_lv_').strip()

    if type_name in json_c_to_py_types:
        return json_c_to_py_types[type_name]

    new_type_name = type_name
    lvobj = False

    if new_type_name.startswith('lv_'):
        new_type_name = new_type_name[3:]
        lvobj = True
    elif new_type_name.startswith('_lv_'):
        new_type_name = new_type_name[3:]
        lvobj = True

    if new_type_name.count('*') > 1:
        new_type_name = new_type_name.split('*', 1)[0].strip()

        if new_type_name in json_c_to_py_types:
            new_type_name = json_c_to_py_types[new_type_name]

        ntn = _get_best_match(new_type_name)
        if ntn:
            new_type_name = ntn

        if lvobj:
            new_type_name = f'"{new_type_name}"'

        new_type_name = f'List[{new_type_name}]'

        json_c_to_py_types[type_name] = new_type_name
        return new_type_name

    if new_type_name.count('*') == 1:
        new_type_name = new_type_name.split('*', 1)[0].strip()
        if new_type_name in json_c_to_py_types:
            return json_c_to_py_types[new_type_name]

        ntn = _get_best_match(new_type_name)
        if ntn:
            new_type_name = ntn

        if lvobj:
            new_type_name = f'"{new_type_name}"'

        json_c_to_py_types[type_name] = new_type_name
        return new_type_name

    if '[' in new_type_name:
        new_type_name = new_type_name.split('[', 1)[0].strip()
        if new_type_name in json_c_to_py_types:
            new_type_name = json_c_to_py_types[new_type_name]

        ntn = _get_best_match(new_type_name)
        if ntn:
            new_type_name = ntn

        if lvobj:
            new_type_name = f'"{new_type_name}"'

        new_type_name = f'List[{new_type_name}]'

        json_c_to_py_types[type_name] = new_type_name
        return new_type_name

    ntn = _get_best_match(new_type_name)
    if ntn:
        new_type_name = ntn

    if lvobj:
        new_type_name = f'"{new_type_name}"'

    json_c_to_py_types[type_name] = new_type_name
    return new_type_name


#
# Initialization, data structures, helper functions
#

# We consider union as a struct, for simplicity
def is_struct(type_):
    return isinstance(type_, (c_ast.Struct, c_ast.Union))


obj_metadata = collections.OrderedDict()
func_metadata = collections.OrderedDict()
callback_metadata = collections.OrderedDict()
struct_metadata = collections.OrderedDict()
variable_metadata = collections.OrderedDict()
constant_metadata = collections.OrderedDict()

func_prototypes = {}

parser = c_parser.CParser()
gen = c_generator.CGenerator()

with open(pp_file, 'r') as f:
    pp_data = f.read()

cparser = pycparser.CParser()
ast = cparser.parse(pp_data, input_header)

forward_struct_decls = {}

for item in ast.ext[:]:
    if (
        isinstance(item, c_ast.Decl) and
        item.name is None and
        isinstance(item.type, c_ast.Struct) and
        item.type.name is not None
    ):
        if item.type.decls is None:
            forward_struct_decls[item.type.name] = [item]
        else:
            if item.type.name in forward_struct_decls:
                decs = forward_struct_decls[item.type.name]
                if len(decs) == 2:
                    decl, td = decs

                    td.type.type.decls = item.type.decls[:]

                    ast.ext.remove(decl)
                    ast.ext.remove(item)
    elif (
        isinstance(item, c_ast.Typedef) and
        isinstance(item.type, c_ast.TypeDecl) and
        item.name and
        item.type.declname and
        item.name == item.type.declname and
        isinstance(item.type.type, c_ast.Struct) and
        item.type.type.decls is None
    ):
        if item.type.type.name in forward_struct_decls:
            forward_struct_decls[item.type.type.name].append(item)


# ast_file = open(os.path.join(os.path.dirname(__file__), 'ast.txt'), 'w')
# ast_file.write(str(ast))
# ast_file.close()

pp_cmd = cpp_cmd

# the stdout code below is to override output to stdout from the print
# statements. This will output to a file instead of stdout. This is done
# because of how cmake works and it removing all of the semicolons when the
# output gets stored in a cmake variable and then written to a file using cmake

import sys  # NOQA


class STDOut:

    def __init__(self):
        self._stdout = sys.stdout
        self._file = open(args.output, 'w')

        sys.stdout = self  # NOQA
        self._write_to_file = None

    def set_out_file(self, file):
        self._write_to_file = file

    def write(self, data):
        self._file.write(data)

        if self._write_to_file is not None:
            self._write_to_file.write(data)
            self._write_to_file.flush()

    def flush(self):
        self._file.flush()

    def close(self):
        try:
            self._file.close()
        except:  # NOQA
            pass
        else:
            sys.stdout = self._stdout  # NOQA

    def __getattr__(self, attr):
        if attr in self.__dict__:
            return self.__dict__[attr]

        return getattr(self._stdout, attr)


stdout = STDOut()

_old_excepthook = sys.excepthook


def my_excepthook(exc_type, exc_value, tb):
    stdout.close()
    return _old_excepthook(exc_type, exc_value, tb)


sys.excepthook = my_excepthook

# Types and structs

# and not (hasattr(x.type, 'declname') and lv_base_obj_pattern.match(x.type.declname))]
typedefs = [x.type for x in ast.ext if isinstance(x, c_ast.Typedef)]
# print('/* %s */' % str(typedefs))

synonym = {}
for t in typedefs:
    if (
        isinstance(t, c_ast.TypeDecl) and
        isinstance(t.type, c_ast.IdentifierType)
    ):
        if t.declname != t.type.names[0]:
            synonym[t.declname] = t.type.names[0]

    if isinstance(t, c_ast.TypeDecl) and isinstance(t.type, c_ast.Struct):
        if t.declname != t.type.name:
            synonym[t.declname] = t.type.name

struct_typedefs = [typedef for typedef in typedefs if is_struct(typedef.type)]
structs_without_typedef = collections.OrderedDict(
    (decl.type.name, decl.type)
    for decl in ast.ext
    if hasattr(decl, 'type') and is_struct(decl.type)
)

# for typedefs that referenced to a forward declaration struct,
# replace it with the real definition.
for typedef in struct_typedefs:
    # None means it's a forward declaration
    if typedef.type.decls is None:
        struct_name = typedef.type.name
        # check if it's found in `structs_without_typedef`.
        # It actually has the typedef. Replace type with it.
        if typedef.type.name in structs_without_typedef:
            typedef.type = structs_without_typedef[struct_name]


structs = collections.OrderedDict((typedef.declname, typedef.type)
                                  for typedef in struct_typedefs
                                  if typedef.declname and typedef.type.decls)

# This is for struct without typedef
structs.update(structs_without_typedef)
explicit_structs = collections.OrderedDict((typedef.type.name, typedef.declname)
                                           for typedef in struct_typedefs
                                           if typedef.type.name)

opaque_structs = collections.OrderedDict(
    (typedef.declname, c_ast.Struct(name=typedef.declname, decls=[]))
    for typedef in typedefs
    if isinstance(typedef.type, c_ast.Struct) and typedef.type.decls is None
)

structs.update({k: v for k, v in opaque_structs.items() if k not in structs})

# Functions and objects

func_defs = [x.decl for x in ast.ext if isinstance(x, c_ast.FuncDef)]
func_decls = [x for x in ast.ext
              if isinstance(x, c_ast.Decl) and
              isinstance(x.type, c_ast.FuncDecl)]

all_funcs = func_defs + func_decls

# functions that start with underscore are usually internal
funcs = [f for f in all_funcs if not f.name.startswith('_')]
obj_ctors = [func for func in funcs if is_obj_ctor(func)]

for obj_ctor in obj_ctors:
    funcs.remove(obj_ctor)

obj_names = [create_obj_pattern.match(ctor.name).group(1) for ctor in obj_ctors]


def has_ctor(obj_name_):
    return (ctor_name_from_obj_name(obj_name_) in
            [ctor.name for ctor in obj_ctors])


def get_ctor(obj_name_):
    global obj_ctors
    return next(ctor for ctor in obj_ctors
                if ctor.name == ctor_name_from_obj_name(obj_name_))


def get_methods(obj_name_):
    global funcs
    return [func for func in funcs
            if is_method_of(func.name, obj_name_) and
            not func.name == ctor_name_from_obj_name(obj_name_)]


@memoize
def noncommon_part(member_name_, stem_name):
    common_part = os.path.commonprefix([member_name_, stem_name])
    n = len(common_part) - 1
    while n > 0 and member_name_[n] != '_':
        n -= 1

    return member_name_[n + 1:]


@memoize
def get_first_arg(func):
    if not func.type.args:
        return None

    if not len(func.type.args.params) >= 1:
        return None

    if not func.type.args.params[0].type:
        return None

    return func.type.args.params[0].type


@memoize
def get_first_arg_type(func):
    first_arg = get_first_arg(func)
    if not first_arg:
        return None

    if not first_arg.type:
        return None

    return get_type(first_arg.type, remove_quals=True)


def get_base_struct_name(struct_name_):
    if struct_name_.endswith('_t'):
        return struct_name_[:-2]

    return struct_name_


# "struct function" starts with struct name (without _t), and their first argument is a pointer to the struct
# Need also to take into account struct functions of aliases of current struct.
@memoize
def get_struct_functions(struct_name_):
    global funcs

    if not struct_name_:
        return []

    reverse_aliases = [alias for alias in struct_aliases
                       if struct_aliases[alias] == struct_name_]
    res = []

    if struct_name_ in structs or len(reverse_aliases) > 0:
        simp_struct_name = simplify_identifier(struct_name_)

        for func in funcs:
            simp_func_name = simplify_identifier(func.name)
            uncommon_name = noncommon_part(simp_func_name, simp_struct_name)
            if uncommon_name != simp_func_name:
                if get_first_arg_type(func) == struct_name_:
                    res.append(func)

    if struct_name_ in struct_aliases:
        res.extend(get_struct_functions(struct_aliases[struct_name_]))

    return res


@memoize
def is_struct_function(func):
    return func in get_struct_functions(get_first_arg_type(func))

# is_static_member returns true if function does not receive the obj as the first argument
# and the object is not a struct function


@memoize
def is_static_member(func, obj_type=base_obj_type):
    first_arg = get_first_arg(func)

    if first_arg and isinstance(first_arg, c_ast.ArrayDecl):
        # Arrays cannot have non static members
        return True

    if is_struct_function(func):
        return False

    first_arg_type = get_first_arg_type(func)
    return first_arg_type is None or first_arg_type != obj_type


# All object should inherit directly from base_obj, and not according to lv_ext,
# as disccussed on https://github.com/littlevgl/lv_binding_micropython/issues/19
parent_obj_names = {child_name: base_obj_name
                    for child_name in obj_names
                    if child_name != base_obj_name}

parent_obj_names[base_obj_name] = None


# Parse Enums

enum_defs = [x for x in ast.ext
             if hasattr(x, 'type') and isinstance(x.type, c_ast.Enum)]

enum_defs += [x.type for x in ast.ext
              if hasattr(x, 'type') and
              hasattr(x.type, 'type') and
              isinstance(x.type, c_ast.TypeDecl) and
              isinstance(x.type.type, c_ast.Enum)]


# Enum member access functions.
def get_enum_members(obj_name_):
    if obj_name_ not in enums:
        return []

    return [e_mem_name for e_mem_name, _ in enums[obj_name_].items()]


def get_enum_member_name(enum_member):
    if enum_member[0].isdigit():
        # needs to be a valid attribute name
        enum_member = '_' + enum_member

    return enum_member


def get_enum_value(obj_name_, enum_member):
    return enums[obj_name_][enum_member]


# parse function pointers
func_typedefs = collections.OrderedDict(
    (t.name, t) for t in ast.ext
    if isinstance(t, c_ast.Typedef) and
    isinstance(t.type, c_ast.PtrDecl) and
    isinstance(t.type.type, c_ast.FuncDecl)
)

# Global blobs
blobs = collections.OrderedDict(
    (decl.name, decl.type.type) for decl in ast.ext
    if isinstance(decl, c_ast.Decl) and
    'extern' in decl.storage and
    hasattr(decl, 'type') and
    isinstance(decl.type, c_ast.TypeDecl) and
    not decl.name.startswith('_')
)

blobs['_nesting'] = parser.parse('extern int _nesting;').ext[0].type.type

int_constants = []


#
# Type convertors
#
class MissingConversionException(ValueError):
    pass


mp_to_lv = {
    'mp_obj_t': '(mp_obj_t)',
    'va_list': None,
    'void *': 'mp_to_ptr',
    'const uint8_t *': 'mp_to_ptr',
    'const void *': 'mp_to_ptr',
    'bool': 'mp_obj_is_true',
    'char *': '(char*)convert_from_str',
    'char **': 'mp_write_ptr_C_Pointer',
    'const char *': 'convert_from_str',
    'const char **': 'mp_write_ptr_C_Pointer',
    '%s_obj_t *' % module_prefix: 'mp_to_lv',
    'uint8_t': '(uint8_t)mp_obj_get_int',
    'uint16_t': '(uint16_t)mp_obj_get_int',
    'uint32_t': '(uint32_t)mp_obj_get_int',
    'uint64_t': '(uint64_t)mp_obj_get_ull',
    'unsigned': '(unsigned)mp_obj_get_int',
    'unsigned int': '(unsigned int)mp_obj_get_int',
    'unsigned char': '(unsigned char)mp_obj_get_int',
    'unsigned short': '(unsigned short)mp_obj_get_int',
    'unsigned long': '(unsigned long)mp_obj_get_int',
    'unsigned long int': '(unsigned long int)mp_obj_get_int',
    'unsigned long long': '(unsigned long long)mp_obj_get_ull',
    'unsigned long long int': '(unsigned long long int)mp_obj_get_ull',
    'int8_t': '(int8_t)mp_obj_get_int',
    'int16_t': '(int16_t)mp_obj_get_int',
    'int32_t': '(int32_t)mp_obj_get_int',
    'int64_t': '(int64_t)mp_obj_get_ull',
    'size_t': '(size_t)mp_obj_get_int',
    'int': '(int)mp_obj_get_int',
    'char': '(char)mp_obj_get_int',
    'short': '(short)mp_obj_get_int',
    'long': '(long)mp_obj_get_int',
    'long int': '(long int)mp_obj_get_int',
    'long long': '(long long)mp_obj_get_ull',
    'long long int': '(long long int)mp_obj_get_ull',
    'float': '(float)mp_obj_get_float',
}

lv_to_mp = {
    'mp_obj_t': '(mp_obj_t)',
    'va_list': None,
    'void *': 'ptr_to_mp',
    'const uint8_t *': 'ptr_to_mp',
    'const void *': 'ptr_to_mp',
    'bool': 'convert_to_bool',
    'char *': 'convert_to_str',
    'char **': 'mp_read_ptr_C_Pointer',
    'const char *': 'convert_to_str',
    'const char **': 'mp_read_ptr_C_Pointer',
    '%s_obj_t *' % module_prefix: 'lv_to_mp',
    'uint8_t': 'mp_obj_new_int_from_uint',
    'uint16_t': 'mp_obj_new_int_from_uint',
    'uint32_t': 'mp_obj_new_int_from_uint',
    'uint64_t': 'mp_obj_new_int_from_ull',
    'unsigned': 'mp_obj_new_int_from_uint',
    'unsigned int': 'mp_obj_new_int_from_uint',
    'unsigned char': 'mp_obj_new_int_from_uint',
    'unsigned short': 'mp_obj_new_int_from_uint',
    'unsigned long': 'mp_obj_new_int_from_uint',
    'unsigned long int': 'mp_obj_new_int_from_uint',
    'unsigned long long': 'mp_obj_new_int_from_ull',
    'unsigned long long int': 'mp_obj_new_int_from_ull',
    'int8_t': 'mp_obj_new_int',
    'int16_t': 'mp_obj_new_int',
    'int32_t': 'mp_obj_new_int',
    'int64_t': 'mp_obj_new_int_from_ll',
    'size_t': 'mp_obj_new_int_from_uint',
    'int': 'mp_obj_new_int',
    'char': 'mp_obj_new_int',
    'short': 'mp_obj_new_int',
    'long': 'mp_obj_new_int',
    'long int': 'mp_obj_new_int',
    'long long': 'mp_obj_new_int_from_ll',
    'long long int': 'mp_obj_new_int_from_ll',
    'float': 'mp_obj_new_float',
}

lv_mp_type = {
    'mp_obj_t': '%s*' % base_obj_type,
    'va_list': None,
    'void *': 'void*',
    'const uint8_t *': 'void*',
    'const void *': 'void*',
    'bool': 'bool',
    'char *': 'char*',
    'char **': 'char**',
    'const char *': 'char*',
    'const char **': 'char**',
    '%s_obj_t *' % module_prefix: '%s*' % base_obj_type,
    'uint8_t': 'int',
    'uint16_t': 'int',
    'uint32_t': 'int',
    'uint64_t': 'int',
    'unsigned': 'int',
    'unsigned int': 'int',
    'unsigned char': 'int',
    'unsigned short': 'int',
    'unsigned long': 'int',
    'unsigned long int': 'int',
    'unsigned long long': 'int',
    'unsigned long long int': 'int',
    'int8_t': 'int',
    'int16_t': 'int',
    'int32_t': 'int',
    'int64_t': 'int',
    'size_t': 'int',
    'int': 'int',
    'char': 'int',
    'short': 'int',
    'long': 'int',
    'long int': 'int',
    'long long': 'int',
    'long long int': 'int',
    'void': None,
    'float': 'float',
}

json_c_to_py_types = {
    'mp_obj_t': f'"{base_obj_type}"',
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

lv_to_mp_byref = {}
lv_to_mp_funcptr = {}


# Add native array supported types
# These types would be converted automatically to/from array type.
# Supported array (pointer) types are signed/unsigned int: 8bit, 16bit, 32bit and 64bit.
def register_int_ptr_type(convertor, *types):
    for ptr_type in types:
        for qualified_ptr_type in [ptr_type, 'const '+ptr_type]:
            mp_to_lv[qualified_ptr_type] = f'mp_array_to_{convertor}'
            lv_to_mp[qualified_ptr_type] = f'mp_array_from_{convertor}'
            lv_mp_type[qualified_ptr_type] = 'void*'


register_int_ptr_type('u8ptr', 'unsigned char *', 'uint8_t *')

# char* is considered as string, not int ptr!
'''
register_int_ptr_type('i8ptr',
        'char *',
        'int8_t *')
'''

register_int_ptr_type('u16ptr', 'unsigned short *', 'uint16_t *')
register_int_ptr_type('i16ptr', 'short *', 'int16_t *')

register_int_ptr_type('u32ptr', 'uint32_t *', 'unsigned *',
                      'unsigned int *', 'unsigned long *',
                      'unsigned long int *', 'size_t *')

register_int_ptr_type('i32ptr', 'int32_t *', 'signed *',
                      'signed int *', 'signed long *', 'signed long int *',
                      'long *', 'long int *', 'int *')

register_int_ptr_type('u64ptr', 'int64_t *',
                      'signed long long *', 'long long *', 'long long int *')

register_int_ptr_type('i64ptr', 'uint64_t *',
                      'unsigned long long *', 'unsigned long long int *')


from . import templates  # NOQA


c_name = templates.c_name
pointer = templates.pointer
const_pointer = templates.const_pointer
const = templates.const
write_case_cb = templates.write_case_cb
read_case_cb = templates.read_case_cb
write_case_array = templates.write_case_array
read_case_array = templates.read_case_array
write_case = templates.write_case
read_case = templates.read_case


input_headers = [input_header, private_header]
includes = '\n'.join('#include "%s"' % header for header in input_headers)

gen_objs = ', '.join([f'{objname}({parent_obj_names[objname]})' for objname in obj_names])
build_args = ' '.join(sys.argv)

# Emit Header
print(templates.headers(build_args, pp_cmd, gen_objs, module_name, includes))

# Enable objects, if supported
if len(obj_names) > 0:
    print(templates.objects(base_obj_type, base_obj_name))


# Emit Mpy helper functions
print(templates.helper_functions)


# Add regular enums with integer values
enums = collections.OrderedDict()
for enum_def in enum_defs:
    # Skip stdatomic.h memory_order, no bindings needed.
    if (
        isinstance(enum_def, c_ast.TypeDecl) and
        enum_def.declname == 'memory_order'
    ):
        continue

    # eprint("--> %s" % enum_def)
    while hasattr(enum_def.type, 'name') and not enum_def.type.values:
        enum_def = next(e for e in enum_defs
                        if hasattr(e.type, 'name') and
                        e.type.name == enum_def.type.name and
                        e.type.values)

    member_names = [member.name for member in enum_def.type.values.enumerators
                    if not member.name.startswith('_')]

    enum_name = os.path.commonprefix(member_names)

    # remove suffix
    enum_name = "_".join(enum_name.split("_")[:-1])

    enum = collections.OrderedDict()
    for member in enum_def.type.values.enumerators:
        if member.name.startswith('_'):
            continue

        if len(enum_name) > 0:
            member_name = member.name[len(enum_name)+1:]
        else:
            member_name = member.name

        if member_name[0].isdigit():
            member_name = '_' + member_name

        if len(enum_name) > 0 and get_enum_name(enum_name) != 'ENUM':
            enum[member_name] = templates.MP_ROM_INT(member.name)
        else:
            int_constants.append(member.name)

    if len(enum) > 0:
        if len(get_enum_name(enum_name)) > 0:
            prev_enum = enums.get(enum_name)
            if prev_enum:
                prev_enum.update(enum)
            else:
                enums[enum_name] = enum


tmp_enums = [enum for enum in enums
             if len(enums[enum]) == 1 and
             enum.startswith('ENUM')]

for enum in tmp_enums:
    int_constants.append('%s_%s' % (enum, next(iter(enums[enum]))))
    enum_name = c_name(enum,  list(enums[enum].keys())[0])

    meta_key = enum_name.replace('ENUM_', '')
    meta_key = meta_key.replace('LV_', '')

    constant_metadata[meta_key] = (
        dict(py_type='int', c_type=enum_name.replace('ENUM_', ''))
    )

    del enums[enum]


# Add special string enums
print('''
/*
 * LVGL string constants
 */
''')

for enum_def in enum_defs:
    if not enum_def.type.values:
        continue
    member_names = [str_enum_to_str(member.name) for member in enum_def.type.values.enumerators
                    if lv_str_enum_pattern.match(member.name)]

    enum_name = os.path.commonprefix(member_names)
    # remove suffix
    enum_name = "_".join(enum_name.split("_")[:-1])
    enum = collections.OrderedDict()
    if enum_name:
        for member in enum_def.type.values.enumerators:
            full_name = str_enum_to_str(member.name)
            member_name = full_name[len(enum_name)+1:]

            print(templates.MP_DEFINE_STR_OBJ(full_name, full_name))

            enum[member_name] = f'&mp_{full_name}'
        if len(enum) > 0:
            if enum_name in enums:
                enums[enum_name].update(enum)
            else:
                enums[enum_name] = enum


# eprint('--> enums: \n%s' % enums)


# eprint(',\n'.join(sorted('%s : %s' % (name, get_type(blobs[name])) for name in blobs)))

#
# Callbacks helper functions
#

def decl_to_callback(decl_):
    if not hasattr(decl_, 'type'):
        return None

    if (
        isinstance(decl_.type, c_ast.PtrDecl) and
        isinstance(decl_.type.type, c_ast.FuncDecl)
    ):
        return decl_.name, decl_.type.type

    if isinstance(decl_.type, c_ast.FuncDecl):
        return decl_.name, decl_.type

    if (
        isinstance(decl_.type, c_ast.TypeDecl) and
        hasattr(decl_.type.type, 'names')
    ):
        func_typedef_name = decl_.type.type.names[0]
        while func_typedef_name in synonym:
            func_typedef_name = synonym[func_typedef_name]

        if func_typedef_name in func_typedefs:
            return decl_.name, func_typedefs[func_typedef_name].type.type


def get_user_data_accessors(containing_struct, containing_struct_name=None):
    if (
        not containing_struct_name and
        containing_struct and
        containing_struct.name
    ):
        containing_struct_name = containing_struct.name

    if not containing_struct_name:
        return None, None

    base_struct_name = get_base_struct_name(containing_struct_name)
    getter_name = f'{base_struct_name}_get_user_data'
    setter_name = f'{base_struct_name}_set_user_data'

    struct_functions = get_struct_functions(containing_struct_name)

    getters = [s for s in struct_functions
               if s.name == getter_name and
               len(s.type.args.params) == 1]

    setters = [s for s in struct_functions
               if s.name == setter_name and
               len(s.type.args.params) == 2]

    if getters and setters:
        return getters[0], setters[0]
    else:
        return None, None


def get_user_data(func, containing_struct=None, containing_struct_name=None):
    args_ = func.args.params

    user_data_found = False
    user_data = 'None'
    if len(args_) > 0 and isinstance(args_[0].type, c_ast.PtrDecl):
        struct_arg_type_name = get_type(args_[0].type.type, remove_quals=True)

        if containing_struct_name and struct_arg_type_name != containing_struct_name:
            return None, None, None

        if not containing_struct:
            try_generate_type(args_[0].type)

            if struct_arg_type_name in structs:
                containing_struct = structs[struct_arg_type_name]

        if containing_struct:
            flatten_struct_decls = flatten_struct(containing_struct.decls)
            user_data = 'user_data'

            user_data_decls = [decl_.name for decl_ in flatten_struct_decls]
            user_data_found = user_data in user_data_decls

            if not user_data_found and lvgl_json is not None:
                containing_struct_j = next([struct for struct in lvgl_json["structures"]
                                            if struct["name"] == struct_arg_type_name], None)

                fwd_decl = next([fwd_decl for fwd_decl in lvgl_json["forward_decls"]
                                 if fwd_decl["name"] == struct_arg_type_name], None)

                if (
                    containing_struct_j is None
                    and struct_arg_type_name.startswith("lv_")
                    and fwd_decl is not None
                ):
                    struct_arg_type_name_with_underscore = "_" + struct_arg_type_name
                    containing_struct_j = next([struct for struct in lvgl_json["structures"]
                                                if struct["name"] == struct_arg_type_name_with_underscore], None)

                if containing_struct_j is not None:
                    user_data_found = any(user_data == field["name"]
                                          for field in containing_struct_j["fields"])

    if not user_data_found:
        user_data = None

    return user_data, *get_user_data_accessors(containing_struct, containing_struct_name)


#
# Generate structs when needed
#
generated_structs = collections.OrderedDict()
generated_struct_functions = collections.OrderedDict()
struct_aliases = collections.OrderedDict()
callbacks_used_on_structs = []


def flatten_struct(struct_decls):
    result = []

    if not struct_decls:
        return result

    for decl_ in struct_decls:
        if is_struct(decl_.type):
            result.extend(flatten_struct(decl_.type.decls))
        else:
            result.append(decl_)

    return result


def try_generate_struct(struct_name_, struct):
    if struct_name_ in generated_structs:
        return None

    sanitized_struct_name = sanitize(struct_name_)

    # Starting generating a struc
    generated_structs[struct_name_] = False

    if struct_name_ in mp_to_lv:
        return mp_to_lv[struct_name_]

    if struct.decls is None:
        if struct_name_ == struct.name:
            return None

        if struct.name not in structs:
            return None

        return try_generate_type(structs[struct.name])

    flatten_struct_decls = flatten_struct(struct.decls)

    # Go over fields and try to generate type convertors for each
    write_cases = []
    read_cases = []

    meta_key_ = get_py_type(struct_name_).replace('"', '')

    struct_metadata[meta_key_] = dict(
        class_attributes=collections.OrderedDict(),
        attributes=collections.OrderedDict(),
        py_type='class',
        c_type=struct_name_,
        methods=collections.OrderedDict()
    )

    attribute_meta = struct_metadata[meta_key_]['attributes']

    for decl_ in flatten_struct_decls:
        converted = try_generate_type(decl_.type)
        type_name = get_type(decl_.type, remove_quals=True)
        decl_name = decl_.name

        # Handle the case of nested struct
        if not converted and is_struct(decl_.type.type):
            parent_name = struct_name_
            child_name = decl_.type.declname  # NOQA
            type_name = '%s_%s_t' % (parent_name[:-2], child_name)
            print(templates.typedef_typeof(parent_name, child_name, type_name))

            try_generate_struct(type_name, decl_.type.type)

        # print('==> %s %s: %s' % (type_name, str(type_name in mp_to_lv), decl))

        if (
            (type_name not in mp_to_lv or not mp_to_lv[type_name]) or
            (type_name not in lv_to_mp or not lv_to_mp[type_name])
        ):
            if type_name in generated_structs:
                print(f"/* Already started generating {type_name}! skipping field '{decl_name}' */")
                continue

            raise MissingConversionException(f'Missing conversion to {type_name} when generating struct {struct_name_}.{get_name(decl_)}')

        mp_to_lv_convertor = mp_to_lv[type_name]

        if type_name in lv_to_mp_byref:
            lv_to_mp_convertor = lv_to_mp_byref[type_name]
        else:
            lv_to_mp_convertor = lv_to_mp[type_name]

        if isinstance(decl_.type, c_ast.PtrDecl):
            # needed when field is const. casting to void overrides it
            cast = '(void*)'
        else:
            cast = ''

        callback = decl_to_callback(decl_)
        field = sanitize(decl_.name)

        if callback:
            # print("/* %s callback %s */" % (gen.visit(decl), callback))
            func_name_, arg_type = callback
            user_data, _, _ = get_user_data(arg_type, containing_struct=struct,
                                            containing_struct_name=struct_name_)

            if callback not in callbacks_used_on_structs:
                callbacks_used_on_structs.append(callback + (struct_name_,))

            # Emit callback forward decl.
            tmp_user_data = [user_data_decl.name
                             for user_data_decl in flatten_struct_decls]

            lv_callback = c_name(struct_name_, func_name_, 'callback')

            if user_data in tmp_user_data:
                full_user_data = f'data->{user_data}'
                full_user_data_ptr = f'&{full_user_data}'
                print(templates.cb_decl(get_type(arg_type.type, remove_quals=False),
                                        lv_callback, gen.visit(arg_type.args)))
            else:
                if not user_data:
                    gen_func_error(decl_, f"Missing 'user_data' as a field of the first parameter of the callback function '{lv_callback}'")
                else:
                    gen_func_error(decl_, f"Missing 'user_data' member in struct '{struct_name_}'")

                full_user_data = 'NULL'
                full_user_data_ptr = full_user_data
                lv_callback = 'NULL'

            user_data = full_user_data_ptr
            write_cases.append(write_case_cb(field, type_name, cast, decl_name,
                                             lv_callback, struct_name_, user_data))

            user_data = full_user_data
            funcptr = lv_to_mp_funcptr[type_name]
            read_cases.append(read_case_cb(field, type_name, funcptr, cast, decl_name,
                                           lv_callback, struct_name_, user_data))
        else:
            # Only allow write to non-const members

            if hasattr(decl_.type, 'quals') and 'const' in decl_.type.quals:
                is_writeable = False
            else:
                is_writeable = True

            # Arrays must be handled by memcpy, otherwise we would get "assignment to expression with array type" error

            attribute_meta[decl_name] = dict(py_type=get_py_type(type_name), c_type=type_name,
                                             is_writeable=is_writeable, is_readable=True)

            if isinstance(decl_.type, c_ast.ArrayDecl):
                size = f'{templates.sizeof(gen.visit(decl_.type.type))} * {gen.visit(decl_.type.dim)}'

                if is_writeable:
                    write_cases.append(write_case_array(field, type_name,
                                                        mp_to_lv_convertor,
                                                        cast, decl_name, size))

                read_cases.append(read_case_array(field, type_name,
                                                  lv_to_mp_convertor,
                                                  cast, decl_name))
            else:
                if is_writeable:
                    write_cases.append(write_case(field, type_name,
                                                  mp_to_lv_convertor,
                                                  cast, decl_name))

                read_cases.append(read_case(field, type_name,
                                            lv_to_mp_convertor,
                                            cast, decl_name))

    print(templates.struct(sanitized_struct_name=sanitized_struct_name,
                           struct_name=struct_name_,
                           struct_tag='struct ' if struct_name_ in structs_without_typedef.keys() else '',
                           write_cases='\n'.join(write_cases),
                           read_cases='\n'.join(read_cases)))

    lv_to_mp[struct_name_] = templates.mp_read(sanitized_struct_name)
    lv_to_mp_byref[struct_name_] = templates.mp_read_byref(sanitized_struct_name)
    mp_to_lv[struct_name_] = templates.mp_write(sanitized_struct_name)
    lv_to_mp[pointer(struct_name_)] = templates.mp_read_ptr(sanitized_struct_name)
    mp_to_lv[pointer(struct_name_)] = templates.mp_write_ptr(sanitized_struct_name)
    lv_to_mp[const_pointer(struct_name_)] = templates.mp_read_ptr(sanitized_struct_name)
    mp_to_lv[const_pointer(struct_name_)] = templates.mp_write_ptr(sanitized_struct_name)

    simple_struct_name = simplify_identifier(sanitized_struct_name)
    lv_mp_type[struct_name_] = simple_struct_name
    lv_mp_type[pointer(struct_name_)] = simple_struct_name
    lv_mp_type[const_pointer(struct_name_)] = simple_struct_name

    # Completed generating a struct
    generated_structs[struct_name_] = True

    return struct_name_


#
# Generate Array Types when needed
#
def try_generate_array_type(type_ast):
    arr_name = get_name(type_ast)

    if arr_name in mp_to_lv:
        return mp_to_lv[arr_name]

    if hasattr(type_ast, 'dim') and type_ast.dim:
        dim = gen.visit(type_ast.dim)
    else:
        dim = None

    element_type = get_type(type_ast.type, remove_quals=True)
    qualified_element_type = gen.visit(type_ast.type)

    if not mp_to_lv.get(element_type, None):
        try_generate_type(type_ast.type)

        if not mp_to_lv.get(element_type, None):
            raise MissingConversionException(f'Missing conversion to {element_type} '
                                             f'while generating array type conversion')

    type_ptr_ast = c_ast.PtrDecl(quals=[], type=type_ast.type)
    element_type_ptr = get_type(type_ptr_ast, remove_quals=True)

    if not mp_to_lv.get(element_type_ptr, None):
        try_generate_type(type_ptr_ast)

        if not mp_to_lv.get(element_type_ptr, None):
            raise MissingConversionException(f'Missing conversion to {element_type_ptr} '
                                             f'while generating array type conversion')

    array_convertor_suffix = arr_name
    for args_ in (
        (' ', '_'),
        ('*', 'ptr'),
        ('+', 'plus'),
        ('-', 'minus'),
        ('[', '__'),
        (']', '__'),
        ('(', '__'),
        (')', '__'),
        ('/', '_div_')
    ):
        array_convertor_suffix = array_convertor_suffix.replace(*args_)

    arr_to_c_convertor_name = templates.mp_arr_to(array_convertor_suffix)
    arr_to_mp_convertor_name = templates.mp_arr_from(array_convertor_suffix)

    if element_type in structs_without_typedef.keys():
        s_decl = 'struct '
    else:
        s_decl = ''

    print(templates.array_conv(arr_name, s_decl, element_type, arr_to_c_convertor_name,
                               '//TODO check dim!' if dim else '', mp_to_lv[element_type]))

    if dim:
        print(templates.array_conv_dim(arr_to_mp_convertor_name, qualified_element_type,
                                       lv_to_mp[element_type], dim))
    else:
        print(templates.array_conv_no_dim(arr_to_mp_convertor_name, qualified_element_type,
                                          lv_to_mp[element_type_ptr]))

    mp_to_lv[arr_name] = arr_to_c_convertor_name
    mp_to_lv[const(arr_name)] = arr_to_c_convertor_name
    lv_to_mp[arr_name] = arr_to_mp_convertor_name
    lv_to_mp[const(arr_name)] = arr_to_mp_convertor_name
    lv_mp_type[arr_name] = arr_to_c_convertor_name
    lv_mp_type[const(arr_name)] = const(arr_to_c_convertor_name)

    new_array_name = get_py_type(arr_name)
    json_c_to_py_types[arr_to_c_convertor_name] = new_array_name

    return arr_to_c_convertor_name


# Generate types from typedefs when needed
def get_arg_name(arg):
    if isinstance(arg, c_ast.PtrDecl) or isinstance(arg, c_ast.FuncDecl):
        return get_arg_name(arg.type)

    if hasattr(arg, 'declname'):
        return arg.declname

    if hasattr(arg, 'name'):
        return name

    return 'unnamed_arg'


def try_generate_type(type_ast):
    if isinstance(type_ast, str):
        raise SyntaxError('Internal error! try_generate_type argument is a string.')

    # Handle the case of a pointer
    if isinstance(type_ast, c_ast.TypeDecl):
        return try_generate_type(type_ast.type)

    type_ = get_name(type_ast)
    if isinstance(type_ast, c_ast.Enum):
        mp_to_lv[type_] = mp_to_lv['int']
        mp_to_lv[pointer(type_)] = mp_to_lv['int *']
        lv_to_mp[type_] = lv_to_mp['int']
        lv_to_mp[pointer(type_)] = lv_to_mp['int *']
        lv_mp_type[type_] = lv_mp_type['int']
        lv_mp_type[pointer(type_)] = lv_mp_type['int *']
        return mp_to_lv[type_]

    if type_ in mp_to_lv:
        return mp_to_lv[type_]

    if isinstance(type_ast, c_ast.ArrayDecl) and try_generate_array_type(type_ast):
        return mp_to_lv[type_]

    if isinstance(type_ast, (c_ast.PtrDecl, c_ast.ArrayDecl)):
        type_ = get_name(type_ast.type.type)
        ptr_type = get_type(type_ast, remove_quals=True)

        if type_ in structs:
            try_generate_struct(type_, structs[type_])

        if (
            isinstance(type_ast.type, c_ast.TypeDecl) and
            isinstance(type_ast.type.type, c_ast.Struct) and
            type_ast.type.type.name in structs
        ):
            try_generate_struct(type_, structs[type_ast.type.type.name])

        if isinstance(type_ast.type, c_ast.FuncDecl):
            if isinstance(type_ast.type.type.type, c_ast.TypeDecl):
                type_ = type_ast.type.type.type.declname

            func_ptr_name = templates.func_ptr_name(type_)

            i = 1
            # Make sure func_ptr_name is unique
            while func_ptr_name in generated_funcs:
                func_ptr_name = f'{templates.func_ptr_name(type_)}_{i}'
                i += 1

            func = c_ast.Decl(name=func_ptr_name, quals=[], align=[], storage=[],
                              funcspec=[], type=type_ast.type, init=None, bitsize=None)
            try:
                print(templates.func_ptr_macro(func_ptr_name))
                gen_mp_func(func, None)

                print(templates.func_ptr_def(func_ptr_name))
                lv_to_mp_funcptr[ptr_type] = func_ptr_name

                lv_to_mp[ptr_type] = templates.mp_lv(func_ptr_name)
                lv_mp_type[ptr_type] = 'function pointer'

            except MissingConversionException as err_:
                gen_func_error(func, err_)

        if ptr_type not in mp_to_lv:
            mp_to_lv[ptr_type] = mp_to_lv['void *']

        if ptr_type not in lv_to_mp:
            lv_to_mp[ptr_type] = lv_to_mp['void *']

        if ptr_type not in lv_mp_type:
            lv_mp_type[ptr_type] = 'void*'

        return mp_to_lv[ptr_type]

    if type_ in structs:
        if try_generate_struct(type_, structs[type_]):
            return mp_to_lv[type_]

    for new_type_ast in [x for x in typedefs if get_arg_name(x) == type_]:
        new_type = get_type(new_type_ast, remove_quals=True)

        if (
            isinstance(new_type_ast, c_ast.TypeDecl) and
            isinstance(new_type_ast.type, c_ast.Struct) and
            not new_type_ast.type.decls
        ):
            if hasattr(new_type_ast.type, 'name'):
                explicit_struct_name = new_type_ast.type.name
            else:
                explicit_struct_name = new_type_ast.type.names[0]  # NOQA

        else:
            explicit_struct_name = new_type

        if type_ == explicit_struct_name:
            continue

        if explicit_struct_name in structs:
            if try_generate_struct(new_type, structs[explicit_struct_name]):
                if explicit_struct_name == new_type:
                    struct_aliases[new_type] = type_

        if type_ != new_type and try_generate_type(new_type_ast):
            mp_to_lv[type_] = mp_to_lv[new_type]
            type_ptr = pointer(type_)

            new_type_ptr = pointer(new_type)
            if new_type_ptr in mp_to_lv:
                mp_to_lv[type_ptr] = mp_to_lv[new_type_ptr]

            if new_type in lv_to_mp:
                lv_to_mp[type_] = lv_to_mp[new_type]
                lv_mp_type[type_] = lv_mp_type[new_type]
                if new_type in lv_to_mp_funcptr:
                    lv_to_mp_funcptr[type_] = lv_to_mp_funcptr[new_type]

                if new_type in lv_to_mp_byref:
                    lv_to_mp_byref[type_] = lv_to_mp_byref[new_type]

                if new_type_ptr in lv_to_mp:
                    lv_to_mp[type_ptr] = lv_to_mp[new_type_ptr]

                if new_type_ptr in lv_mp_type:
                    lv_mp_type[type_ptr] = lv_mp_type[new_type_ptr]

            return mp_to_lv[type_]

    return None


# Helper structs
def create_helper_struct(struct_str):
    struct_str_ast = parser.parse(struct_str).ext[0].type
    struct_name_ = get_name(struct_str_ast)

    structs[struct_name_] = struct_str_ast.type

    try:
        try_generate_struct(struct_name_, struct_str_ast.type)
    except MissingConversionException as err_:
        print(f'/* Helper structs NOT generated:\n    {repr(err_)}\n*/')


print(templates.helper_struct)
create_helper_struct(templates.helper_struct.split('\n', 2)[-1])


#
# Emit C callback functions
#
generated_callbacks = collections.OrderedDict()


def build_callback_func_arg(arg, index, func, func_name_=None):  # NOQA
    arg_type = get_type(arg.type, remove_quals=False)

    # needed when field is const. casting to void overrides it
    if isinstance(arg.type, c_ast.PtrDecl):
        cast = '(void*)'
    else:
        cast = ''

    if arg_type == 'char *':
        converter = 'ptr_to_mp'
    else:
        arg_type = get_type(arg.type, remove_quals=True)

        if not lv_to_mp.get(arg_type, None):
            try_generate_type(arg.type)

            if not lv_to_mp.get(arg_type, None):
                raise MissingConversionException(
                    f"Callback: Missing conversion to {arg_type}"
                )

        converter = lv_to_mp[arg_type]

    arg_metadata = dict(c_type=arg_type, py_type=get_py_type(arg_type))

    if arg.name:
        arg_metadata['name'] = arg.name
    else:
        arg_metadata['name'] = None

    callback_metadata[func_name_]['args'].append(arg_metadata)

    return templates.cb_arg_conv(index, converter, cast)


def gen_callback_func(func, func_name_=None, user_data_argument=False):
    if func_name_ in generated_callbacks:
        return

    args_ = func.args.params
    enumerated_args = []
    for i, arg in enumerate(args_):
        arg_name = f'arg{i}'
        new_arg = c_ast.Decl(name=arg_name, quals=arg.quals, align=[], storage=[],
                             funcspec=[], type=copy.deepcopy(arg.type), init=None,
                             bitsize=None)

        t_ = new_arg
        while hasattr(t_, 'type'):
            if hasattr(t_.type, 'declname'):
                t_.type.declname = arg_name

            t_ = t_.type

        enumerated_args.append(new_arg)

    if not func_name_:
        func_name_ = get_arg_name(func.type)

    callback_metadata[func_name_] = dict(py_type='Callable', c_type=None, args=[])

    if isinstance(func.type, c_ast.TypeDecl) and func.type.declname:
        callback_metadata[func_name_]['c_type'] = func.type.declname

    if is_global_callback(func):
        full_user_data = 'MP_STATE_PORT(mp_lv_user_data)'
    else:
        user_data, user_data_getter, _ = get_user_data(func, func_name_)

        if (
            user_data_argument and
            len(args_) > 0 and
            gen.visit(args_[-1].type) == 'void *'
        ):
            full_user_data = 'arg%d' % (len(args_) - 1)
        elif user_data:
            full_user_data = 'arg0->%s' % user_data

            if (
                len(args_) < 1 or
                hasattr(args_[0].type.type, 'names') and
                lv_base_obj_pattern.match(args_[0].type.type.names[0])
            ):
                raise MissingConversionException(
                    "Callback: First argument of callback function must be lv_obj_t"
                )

        elif user_data_getter:
            full_user_data = '%s(arg0)' % user_data_getter.name
        else:
            full_user_data = None

        if not full_user_data:
            raise MissingConversionException(
                f'Callback: user_data NOT FOUND! {gen.visit(func)}'
            )

    return_type = get_type(func.type, remove_quals=False)
    if (
        return_type != 'void' and
        (return_type not in mp_to_lv or not mp_to_lv[return_type])
    ):
        try_generate_type(func.type)

        if return_type not in mp_to_lv or not mp_to_lv[return_type]:
            raise MissingConversionException(
                f'Callback return value: Missing conversion to {return_type}'
            )

    callback_metadata[func_name_]['c_rtype'] = return_type
    callback_metadata[func_name_]['py_rtype'] = get_py_type(return_type)

    if return_type == 'void':
        return_value = ''
        return_value_assignment = ''
    else:
        return_value = f' {mp_to_lv[return_type]}(callback_result)'
        return_value_assignment = 'mp_obj_t callback_result = '

    func_args = ', '.join(gen.visit(arg) for arg in enumerated_args)

    build_args_ = '\n    '.join(
        build_callback_func_arg(arg, i, func, func_name_=func_name_)
        for i, arg in enumerate(args_))

    print(templates.callback_func(sanitize(func_name_), gen.visit(func), func_args,
                                  len(args_), return_type, return_value,
                                  return_value_assignment, build_args_, full_user_data))

    generated_callbacks[func_name_] = True


# Emit Mpy function definitions
generated_funcs = collections.OrderedDict()


def build_mp_func_arg(arg, index, func, obj_name_):  # NOQA
    if isinstance(arg, c_ast.EllipsisParam):
        raise MissingConversionException("Cannot convert ellipsis param")

    fixed_arg = copy.deepcopy(arg)
    convert_array_to_ptr(fixed_arg)

    if not fixed_arg.name:
        fixed_arg.name = f"arg{index}"
        add_default_declname(fixed_arg, fixed_arg.name)

    callback = decl_to_callback(arg)

    if func.type.args:
        args_ = func.type.args.params
    else:
        args_ = []

    if callback:
        # Callback is supported in two modes:
        # 1) last argument is a void* user_data which is passed to callback
        # 2) first argument is a struct with user_data field,
        #    which is passed to callback

        callback_name, arg_type = callback

        try:
            user_data_argument = False
            full_user_data = None
            user_data_getter = None
            user_data_setter = None

            if (
                len(args_) > 0 and
                gen.visit(args_[-1].type) == 'void *' and
                args_[-1].name == 'user_data'
            ):
                callback_name = c_name(func.name, callback_name)
                full_user_data = '&user_data'
                user_data_argument = True
                first_arg = ''
            else:
                first_arg = args_[0]

                if hasattr(first_arg.type.type, 'type'):
                    struct_name_ = get_name(first_arg.type.type.type)
                else:
                    struct_name_ = get_name(first_arg.type.type)

                callback_name = c_name(struct_name_, callback_name)
                user_data, user_data_getter, user_data_setter = (
                    get_user_data(arg_type, callback_name)
                )

                if is_global_callback(arg_type):
                    full_user_data = '&MP_STATE_PORT(mp_lv_user_data)'
                else:
                    if user_data:
                        full_user_data = f'&{first_arg.name}->{user_data}'
                    elif user_data_getter and user_data_setter:
                        # uses getter/setter instead
                        full_user_data = 'NULL'

                    if index == 0:
                        raise MissingConversionException(
                            f"Callback argument '{gen.visit(arg)}' cannot be the "
                            f"first argument! We assume the first argument "
                            f"contains the user_data"
                        )

                    if not full_user_data:
                        raise MissingConversionException(
                            f"Callback function '{gen.visit(arg)}' must receive a "
                            f"struct pointer with user_data member as its first argument!"
                        )
            # eprint("--> callback_metadata= %s_%s" % (struct_name, callback_name))
            gen_callback_func(arg_type, '%s' % callback_name, user_data_argument)

            if (
                isinstance(arg_type, c_ast.FuncDecl) and
                isinstance(arg_type.type, c_ast.TypeDecl) and
                arg_type.type.declname
            ):
                arg_metadata = dict(py_type='Callable',
                                    function=callback_metadata[callback_name],
                                    c_type=arg_type.type.declname)
            else:
                arg_metadata = dict(py_type='Callable',
                                    function=callback_metadata[callback_name],
                                    c_type=None)

            if arg.name:
                arg_metadata['name'] = arg.name
            else:
                arg_metadata['name'] = None

            if user_data_getter and user_data_setter:
                containing_struct = first_arg.name
            else:
                containing_struct = "NULL"

            if user_data_getter:
                user_data_getter_name = user_data_getter.name
            else:
                user_data_getter_name = 'NULL'

            if user_data_setter:
                user_data_setter_name = user_data_setter.name
            else:
                user_data_setter_name = 'NULL'

            res = templates.func_arg_conv_1(fixed_arg.name, index, sanitize(callback_name),
                                            full_user_data, containing_struct,
                                            user_data_getter_name, user_data_setter_name)

            return res, arg_metadata

        except MissingConversionException as err_:
            gen_func_error(arg, err_)

    if not hasattr(arg, 'type'):
        raise MissingConversionException(
            f'Cannot convert function argument {repr(arg)}'
        )

    arg_type = get_type(arg.type, remove_quals=True)

    if arg_type not in mp_to_lv or not mp_to_lv[arg_type]:
        try_generate_type(arg.type)

        if arg_type not in mp_to_lv or not mp_to_lv[arg_type]:
            raise MissingConversionException(
                f'Missing conversion to {arg_type}'
            )

    arg_metadata = dict(py_type=get_py_type(arg_type), c_type=arg_type)

    if arg.name:
        arg_metadata['name'] = arg.name
    else:
        arg_metadata['name'] = None

    if 'const' in arg.quals:
        # allow conversion from non const to const, sometimes requires cast
        cast = f'({gen.visit(fixed_arg.type)})'
    else:
        cast = ''

    res = templates.func_arg_conv_2(gen.visit(fixed_arg), cast,
                                    mp_to_lv[arg_type], index)

    return res, arg_metadata


def emit_func_obj(func_obj_name, func_name_, param_count, func_ptr, is_static):

    if is_static:
        builtin_macro = 'MP_DEFINE_CONST_LV_FUN_OBJ_STATIC_VAR'
    else:
        builtin_macro = 'MP_DEFINE_CONST_LV_FUN_OBJ_VAR'

    print(templates.func_obj_decl(func_name_, func_obj_name, func_ptr,
                                  param_count, builtin_macro))


def gen_mp_func(func, obj_name_):
    if func.name in generated_funcs:
        print(f'// WARNING: {func.name} was declared more than once!')
        return

    # starting to generate the function
    generated_funcs[func.name] = False

    func_md = dict(args=[], c_type=func.name)

    if obj_name_:
        func_md['py_type'] = 'method'
        func_md['scope'] = obj_name_
    else:
        func_md['py_type'] = 'function'
        func_md['scope'] = 'global'

    if func.type.args:
        args_ = func.type.args.params
    else:
        args_ = []

    enumerated_args = enumerate(args_)

    # Handle the case of a single function argument which is "void"
    if (
        len(args_) == 1 and
        get_type(args_[0].type, remove_quals=True) == "void"
    ):
        param_count = 0
        args_ = []
    else:
        param_count = len(args_)

    # If func prototype matches an already generated func,
    # reuse it and only emit func obj that points to it.
    prototype_str = gen.visit(function_prototype(func))
    if prototype_str in func_prototypes:
        original_func = func_prototypes[prototype_str]
        if generated_funcs[original_func.name] is True:
            print(f'// Reusing {original_func.name} for {func.name}')
            emit_func_obj(func.name, original_func.name, param_count,
                          func.name, is_static_member(func, base_obj_type))

            func_md['c_rtype'] = func_metadata[original_func.name]['c_rtype']
            func_md['py_rtype'] = func_metadata[original_func.name]['py_rtype']

            func_md['args'] = func_metadata[original_func.name]['args']

            # completed generating the function
            generated_funcs[func.name] = True
            func_metadata[func.name] = func_md
            return

    func_prototypes[prototype_str] = func

    # user_data argument must be handled first, if it exists
    try:
        i = [(arg.name if hasattr(arg, 'name') else None)
             for arg in args].index('user_data')

        if i > 0:
            # convert enumerate to list
            enumerated_args = [(i, arg) for i, arg in enumerated_args]
            enumerated_args[0], enumerated_args[i] = (
                enumerated_args[i], enumerated_args[0]
            )
    except ValueError:
        pass

    return_type = get_type(func.type.type, remove_quals=False)
    qualified_return_type = gen.visit(func.type.type)
    if (
        isinstance(func.type.type, c_ast.PtrDecl) and
        lv_func_returns_array.match(func.name)
    ):
        try_generate_array_type(func.type.type)

    if return_type == "void":
        build_result = ""
        build_return_value = "mp_const_none"

        func_md['py_rtype'] = 'None'
        func_md['c_rtype'] = 'void'
    else:
        if not lv_to_mp.get(return_type, None):
            try_generate_type(func.type.type)

            if not lv_to_mp.get(return_type, None):
                raise MissingConversionException(
                    f'Missing conversion from {return_type}'
                )

        build_result = f'{qualified_return_type} _res = '

        if isinstance(func.type.type, c_ast.PtrDecl):
            # needed when field is const. casting to void overrides it
            cast = '(void*)'
        else:
            cast = ''

        build_return_value = f'{lv_to_mp[return_type]}({cast}_res)'

        func_md['py_rtype'] = get_py_type(return_type)
        func_md['c_rtype'] = return_type

    build_args_ = []

    for i, arg in enumerated_args:
        if (
            isinstance(arg, c_ast.EllipsisParam) or
            not isinstance(arg.type, c_ast.TypeDecl) or
            not isinstance(arg.type.type, c_ast.IdentifierType) or
            'void' not in arg.type.type.names
        ):
            # Handle the case of 'void' param which should be ignored
            ba, arg_metadata = build_mp_func_arg(arg, i, func, obj_name_)
            build_args_.append(ba)
            func_md['args'].append(arg_metadata)

    # Handle the case of 'void' param which should be ignored
    build_args_ = "\n    ".join(build_args_)

    send_args = ", ".join((arg.name
                           if (hasattr(arg, 'name') and arg.name)
                           else ("arg%d" % i))
                          for i, arg in enumerate(args_))

    print(templates.extension_def(module_name, gen.visit(func), func.name, build_args_,
                                  build_result, prototype_str, send_args, build_return_value))

    emit_func_obj(func.name, func.name, param_count, func.name,
                  is_static_member(func, base_obj_type))

    # completed generating the function
    generated_funcs[func.name] = True
    func_metadata[func.name] = func_md


def gen_func_error(method, err_):
    global funcs

    if isinstance(method, c_ast.Node):
        func = gen.visit(method)
    else:
        func = method

    print(templates.func_error(err_, func))

    try:
        funcs.remove(method)
    except:  # NOQA
        pass


# Emit Mpy objects definitions
enum_referenced = collections.OrderedDict()


def gen_obj_methods(obj_name_):
    global enums

    if len(obj_names) > 0 and obj_name_ == base_obj_name:
        helper_members = [templates.cast_member]
    else:
        helper_members = []

    members = [templates.member(sanitize(
        method_name_from_func_name(method.name)), method.name)
               for method in get_methods(obj_name_)]

    meta_update = {}

    for method in get_methods(obj_name_):
        meta_update[method_name_from_func_name(method.name)] = (
            func_metadata[method.name])

    obj_metadata[obj_name_]['members'].update(meta_update)

    # add parent methods
    parent_members = []
    if (
        obj_name_ in parent_obj_names and
        parent_obj_names[obj_name_] is not None
    ):
        # parent_members += gen_obj_methods(parent_obj_names[obj_name])
        if 'members' in obj_metadata[parent_obj_names[obj_name_]]:
            key_ = 'members'
        else:
            key_ = 'class_attributes'

        obj_metadata[obj_name_]['members'].update(
            obj_metadata[parent_obj_names[obj_name_]][key_])

    # add enum members

    enum_members = [templates.enum_member(sanitize(
        get_enum_member_name(enum_member_name)),
        get_enum_value(obj_name_, enum_member_name))
        for enum_member_name in get_enum_members(obj_name_)]

    meta_update = {}

    for enum_member_name in get_enum_members(obj_name):
        if enum_member_name.startswith('_'):
            enum_c_type = (
                obj_metadata[obj_name_]['c_type'] + enum_member_name)
        else:
            enum_c_type = (
                obj_metadata[obj_name_]['c_type'] + '_' + enum_member_name)

        meta_update[get_enum_member_name(enum_member_name)] = dict(
            c_type=enum_c_type,
            py_type='int'
        )

    obj_metadata[obj_name_]['members'].update(meta_update)

    # add enums that match object name
    obj_enums = [enum_name_ for enum_name_ in enums.keys()
                 if is_method_of(enum_name_, obj_name_)]

    enum_types = [templates.enum_type_member(
        sanitize(method_name_from_func_name(enum_name_)), enum_name_)
        for enum_name_ in obj_enums]

    meta_update = {}
    for enum_name_ in obj_enums:
        if enum_name_.startswith('_'):
            enum_c_type = obj_metadata[obj_name_]['c_type'] + enum_name_
        else:
            enum_c_type = obj_metadata[obj_name_]['c_type'] + '_' + enum_name_

        meta_update[method_name_from_func_name(enum_name_)] = (
            dict(c_type=enum_c_type, py_type='int')
        )

    obj_metadata[obj_name_]['members'].update(meta_update)

    for enum_name_ in obj_enums:
        meth_name = method_name_from_func_name(enum_name_)

        obj_metadata[obj_name_]['classes'][meth_name] = (
            copy.deepcopy(obj_metadata[obj_name_]['members'][meth_name]))

        del obj_metadata[obj_name_]['members'][meth_name]

        obj_metadata[obj_name_]['classes'][meth_name].update(
            copy.deepcopy(obj_metadata[enum_name_]))
        enum_referenced[enum_name_] = True

        obj_metadata[obj_name_]['classes'][meth_name]['c_type'] = 'enum'
        obj_metadata[obj_name_]['classes'][meth_name]['class_attributes'] = (
            obj_metadata[obj_name_]['classes'][meth_name]['members'])

        del obj_metadata[obj_name_]['classes'][meth_name]['members']

    return members + parent_members + enum_members + enum_types + helper_members


def gen_obj(obj_name_):
    is_obj = has_ctor(obj_name_)
    should_add_base_methods = is_obj and obj_name_ != 'obj'

    obj_metadata[obj_name_] = dict(py_type='class', c_type=obj_name_,
                                   members=collections.OrderedDict(),
                                   classes=collections.OrderedDict(),
                                   methods=collections.OrderedDict(),
                                   class_attributes=collections.OrderedDict())

    if should_add_base_methods:
        obj_metadata[obj_name_]['parent_class'] = 'obj'
    else:
        obj_metadata[obj_name_]['parent_class'] = 'object'

    # Generate object methods
    for method in get_methods(obj_name_):
        try:
            gen_mp_func(method, obj_name_)
        except MissingConversionException as err_:
            gen_func_error(method, err_)

    # Generate object construction function, if needed
    if is_obj:
        ctor_func = get_ctor(obj_name_)

        try:
            gen_mp_func(ctor_func, obj_name_)
        except MissingConversionException as err_:
            gen_func_error(ctor_func, err_)

    if has_ctor(obj_name_):
        ctor = templates.ctor(obj_name_, ctor_func.name)  # NOQA
    else:
        ctor = ''

    locals_dict_entries = ",\n    ".join(gen_obj_methods(obj_name_))

    if is_obj:
        make_new = f'make_new, {obj_name_}_make_new,'
        binary_op = 'binary_op, mp_lv_obj_binary_op,'
        buffer = 'buffer, mp_lv_obj_get_buffer,'
        lv_class = f'&lv_{obj_name_}_class'
    else:
        make_new = ''
        binary_op = ''
        buffer = ''
        lv_class = 'NULL'

    if parent_obj_names.get(obj_name_, None):
        parent_ = f'parent, &mp_lv_{parent_obj_names[obj_name]}_type_base,'
    else:
        parent_ = ''

    print(templates.object_def(module_name, obj_name_, locals_dict_entries, ctor,
                               make_new, binary_op, buffer, parent_, lv_class))

    tmp_metadata = list(obj_metadata[obj_name]['members'].items())

    for meta_func_name, meta_func in tmp_metadata:
        if 'scope' not in meta_func:
            continue

        if obj_metadata[obj_name]['parent_class'] == meta_func['scope']:
            del obj_metadata[obj_name]['members'][meta_func_name]


# Generate Enum objects
for enum_name in list(enums.keys()):
    gen_obj(enum_name)


# Generate all other objects. Generate parent objects first
generated_obj_names = collections.OrderedDict()
for obj_name in obj_names:
    if obj_name in parent_obj_names:
        parent_obj_name = parent_obj_names[obj_name]
    else:
        parent_obj_name = None

    while (
        parent_obj_name is not None and
        parent_obj_name not in generated_obj_names
    ):
        gen_obj(parent_obj_name)
        generated_obj_names[parent_obj_name] = True

        if parent_obj_name in parent_obj_names:
            parent_obj_name = parent_obj_names[parent_obj_name]
        else:
            parent_obj_name = None

    if obj_name not in generated_obj_names:
        gen_obj(obj_name)
        generated_obj_names[obj_name] = True


# Generate structs which contain function members
# First argument of a function could be it's parent struct
# Need to make sure these structs are generated *before* struct-functions are
# Otherwise we will not know of all the structs when generating struct-functions
def try_generate_structs_from_first_argument():
    for func in funcs:
        if func.name in generated_funcs:
            continue

        if func.type.args:
            args_ = func.type.args.params
        else:
            args_ = []

        if len(args_) < 1:
            continue

        arg_type = get_type(args_[0].type, remove_quals=True)

        if not mp_to_lv.get(arg_type, None):
            try:
                try_generate_type(args_[0].type)
            except MissingConversionException as err_:
                print(f'// {arg_type} not generated: {err_}')


# Generate globals
def gen_global(global_name_, global_type_ast):
    global_type = get_type(global_type_ast, remove_quals=True)

    if global_type not in generated_structs:
        wrapped_type = lv_mp_type[global_type]

        if not wrapped_type:
            raise MissingConversionException(
                f'Missing conversion to {wrapped_type} when '
                f'generating global {global_name_}'
            )

        global_type = sanitize(f'_lv_mp_{wrapped_type}_wrapper')
        custom_struct_str = f"""
typedef struct {{
    {wrapped_type} value;
}} {global_type};
        """

        if global_type not in generated_structs:
            print(f'// Global struct wrapper for {wrapped_type}')
            print(custom_struct_str)

            try_generate_struct(
                global_type, parser.parse(custom_struct_str).ext[0].type.type)

    ct = global_name_

    if '_class' in ct:
        ct += '_t'

    variable_metadata[get_py_type(global_name_).replace('"', '')] = (
        dict(py_type=get_py_type(global_type), c_type=global_name_))

    print(templates.global_def(
        module_name, global_name_, global_type, gen.visit(global_type_ast)))


generated_globals = []
for global_name in blobs:
    try:
        gen_global(global_name, blobs[global_name])
        generated_globals.append(global_name)

    except MissingConversionException as exp:
        gen_func_error(global_name, exp)


#
# Generate struct-functions
#
# eprint("/* Generating struct-functions */")
try_generate_structs_from_first_argument()


def generate_struct_functions(struct_list):
    for struct_name_ in struct_list:
        if not generated_structs[struct_name_]:
            continue

        sanitized_struct_name = sanitize(struct_name_)

        struct_funcs = get_struct_functions(struct_name_)

        # clone list because we are changing it in the loop.
        for struct_func in struct_funcs[:]:
            try:
                if struct_func.name not in generated_funcs:
                    gen_mp_func(struct_func, struct_name_)

                    meta_key_ = get_py_type(
                        struct_name_).replace('"', '')

                    if meta_key_ not in struct_metadata:
                        struct_metadata[meta_key_] = (
                            dict(class_attributes=collections.OrderedDict(),
                                 attributes=collections.OrderedDict(),
                                 py_type='class', c_type=struct_name_,
                                 methods=collections.OrderedDict()))

                    san_struct_name = sanitize(
                        noncommon_part(struct_func.name, struct_name_))

                    struct_metadata[meta_key_]['methods'][san_struct_name] = (
                        copy.deepcopy(func_metadata[struct_func.name]))

            except MissingConversionException as err_:
                gen_func_error(struct_func, err_)
                struct_funcs.remove(struct_func)

        if struct_name_ not in structs or structs[struct_name_].decls:
            if struct_name_ in structs_without_typedef.keys():
                struct_tag = 'struct '
            else:
                struct_tag = ''

            struct_size_attr = templates.size_member(struct_tag, struct_name_)

            meta_key_ = get_py_type(struct_name_).replace('"', '')

            struct_metadata[meta_key_]['class_attributes']['__SIZE__'] = (
                dict(c_type='int', py_type='int'))
        else:
            struct_size_attr = ''

        functions = ',\n'.join(templates.func_member(
            sanitize(noncommon_part(f.name, struct_name_)), func_.name)
                               for func_ in struct_funcs)

        print(templates.size_attr(
            sanitized_struct_name, struct_size_attr, functions))

        generated_struct_functions[struct_name_] = True


generate_struct_functions(list(generated_structs.keys()))


# Generate all module functions (not including method functions
# which were already generated)
print("""
/*
 *
 * Global Module Functions
 *
 */
""")

# eprint("/* Generating global module functions /*")
module_funcs = [func for func in funcs if func.name not in generated_funcs]

# clone list because we are changing it in the loop.
for module_func in module_funcs[:]:
    if module_func.name in generated_funcs:
        # generated_funcs could change inside the loop so need to recheck.
        continue

    try:
        gen_mp_func(module_func, None)

        # A new function can create new struct with new function structs
        new_structs = [s for s in generated_structs
                       if s not in generated_struct_functions]

        if new_structs:
            generate_struct_functions(new_structs)

    except MissingConversionException as err:
        gen_func_error(module_func, err)
        module_funcs.remove(module_func)


functions_not_generated = [func.name for func in funcs
                           if func.name not in generated_funcs]

if len(functions_not_generated) > 0:
    print("""
/*
 * Functions not generated:
 * {funcs}
 *
 */

""".format(funcs="\n * ".join(functions_not_generated)))


# Generate callback functions
# for func_typedef in func_typedefs:
#     func = func_typedef.type.type
#     try:
#         gen_callback_func(func)
#     except MissingConversionException as exp:
#         gen_func_error(func, exp)
#         func_name = get_arg_name(func.type)
#         lv_to_mp[func_name] = lv_to_mp['void *']
#         mp_to_lv[func_name] = mp_to_lv['void *']


for (func_name, func, struct_name) in callbacks_used_on_structs:
    try:
        gen_callback_func(func)
        # struct_metadata[struct_name]['methods'][func_name] = (
        #     copy.deepcopy(callback_metadata[func.name]))

    except MissingConversionException as err:
        gen_func_error(func, err)
        # func_name = get_arg_name(func.type)
        # lv_to_mp[func_name] = lv_to_mp['void *']
        # mp_to_lv[func_name] = mp_to_lv['void *']


# Emit Mpy Module definition
print(f"""

/*
 * {sanitize(module_name)} module definitions
 */

static const mp_rom_map_elem_t {sanitize(module_name)}_globals_table[] = {{
    {{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_{sanitize(module_name)}) }},
{'\n'.join(f'    {{ MP_ROM_QSTR(MP_QSTR_{sanitize(o)}), '
           f'       MP_ROM_PTR(&mp_lv_{sanitize(o)}_type_base) }},' for o in obj_names)}
{'\n'.join(f'    {{ MP_ROM_QSTR(MP_QSTR_{sanitize(simplify_identifier(f.name))}), '
           f'       MP_ROM_PTR(&mp_{f.name}_mpobj) }},' for f in module_funcs)}
{'\n'.join(f'    {{ MP_ROM_QSTR(MP_QSTR_{sanitize(get_enum_name(enum_name))}), '
           f'       MP_ROM_PTR(&mp_lv_{enum_name}_type_base) }},' 
           for enum_name in enums.keys() if enum_name not in enum_referenced)}
{'\n'.join(f'    {{ MP_ROM_QSTR(MP_QSTR_{sanitize(simplify_identifier(struct_name))}), '
           f'       MP_ROM_PTR(&mp_{sanitize(struct_name)}_type) }},'
           for struct_name in generated_structs if generated_structs[struct_name])}
{'\n'.join(f'    {{ MP_ROM_QSTR(MP_QSTR_{sanitize(simplify_identifier(struct_aliases[struct_name]))}), '
           f'       MP_ROM_PTR(&mp_{sanitize(struct_name)}_type) }},' for struct_name in struct_aliases.keys())}
{'\n'.join(f'    {{ MP_ROM_QSTR(MP_QSTR_{sanitize(simplify_identifier(global_name))}), '
           f'       MP_ROM_PTR(&mp_{global_name}) }},' for global_name in generated_globals)}
{'\n'.join(f'    {{ MP_ROM_QSTR(MP_QSTR_{sanitize(get_enum_name(int_constant))}), '
           f'       MP_ROM_PTR(MP_ROM_INT({int_constant})) }},' for int_constant in int_constants)}
#ifdef LV_OBJ_T
    {{ MP_ROM_QSTR(MP_QSTR_LvReferenceError), MP_ROM_PTR(&mp_type_LvReferenceError) }},
#endif // LV_OBJ_T
}};
""")


print(f"""
static MP_DEFINE_CONST_DICT (
    mp_module_{module_name}_globals,
    {module_name}_globals_table
);

const mp_obj_module_t mp_module_{module_name} = {{
    .base = {{ &mp_type_module }},
    .globals = (mp_obj_dict_t*)&mp_module_{module_name}_globals
}};

MP_REGISTER_MODULE(MP_QSTR_{module_name}, mp_module_{module_name});

""")


# Add an array of all object types
if len(obj_names) > 0:
    print(f'''
static const mp_lv_obj_type_t *mp_lv_obj_types[] = {{
    {',\n    '.join('&mp_lv_%s_type' % obj_name for obj_name in obj_names)},
    NULL
}};
    ''')


# Save Metadata File, if specified.
def _iter_metadata(d, indent=0):
    idt = ' ' * indent
    for key_, value_ in d.items():
        if isinstance(value_, dict):
            LOG(f"{idt}'{key_}': {{")
            _iter_metadata(value_, indent + 4)
            LOG(f'{idt}}},')
        elif (
            isinstance(value_, list) and
            value_ and
            isinstance(value_[0], dict)
        ):
            LOG(f"{idt}'{key_}': [")
            extra_idt = ' ' * (indent + 4)
            for item_ in value_:
                LOG(f'{extra_idt}{{')
                _iter_metadata(item_, indent + 8)
                LOG(f'{extra_idt}}},')
            LOG(f"{idt}],")
        else:
            LOG(f"{idt}'{key_}': {repr(value_)},")


for obj_name in obj_names:
    parent = obj_metadata[obj_name]['parent_class']

    obj_metadata[obj_name]['methods'].update(
        copy.deepcopy(obj_metadata[obj_name]['members']))

    del obj_metadata[obj_name]['members']

    if parent not in obj_metadata:
        continue

    for enum_name in list(obj_metadata[obj_name]['classes']):
        if enum_name in obj_metadata[parent]['classes']:
            del obj_metadata[obj_name]['classes'][enum_name]


for name, metadata in list(variable_metadata.items()):
    if not metadata['c_type'].endswith('class'):
        continue

    obj_metadata[f'{name}_class'] = dict(
        class_attributes=collections.OrderedDict(),
        attributes=collections.OrderedDict(),
        py_type='class',
        c_type=metadata['c_type'] + '_t',
        methods=collections.OrderedDict()
    )

    obj_names.append(f'{name}_class')
    del variable_metadata[name]


if args.metadata:

    metadata = collections.OrderedDict()
    metadata['objects'] = {obj_name: obj_metadata[obj_name]
                           for obj_name in obj_names}

    metadata['functions'] = {simplify_identifier(f.name): func_metadata[f.name]
                             for f in module_funcs}

    metadata['enums'] = {get_enum_name(enum_name): obj_metadata[enum_name]
                         for enum_name in enums.keys()
                         if enum_name not in enum_referenced}

    metadata['structs'] = {simplify_identifier(struct_name): struct_metadata[simplify_identifier(struct_name)]
                           for struct_name in generated_structs
                           if simplify_identifier(struct_name) in struct_metadata}

    # metadata['structs'] += [simplify_identifier(struct_aliases[struct_name]) for struct_name in struct_aliases.keys()]
    metadata['variables'] = {get_py_type(global_name).replace('"', ''): variable_metadata[get_py_type(global_name).replace('"', '')]
                             for global_name in generated_globals
                             if get_py_type(global_name).replace('"', '') in variable_metadata}

    metadata['int_constants'] = {get_enum_name(int_constant): constant_metadata[get_enum_name(int_constant)]
                                 for int_constant in int_constants}

    _iter_metadata(metadata)

    with open(args.metadata, 'w') as metadata_file:
        json.dump(metadata, metadata_file, indent=4)

    build_path = os.path.split(args.metadata)[0]

    api_json_path = os.path.join(build_path, 'lvgl_api.json')

    with open(api_json_path, 'w') as metadata_file:
        json.dump(lvgl_json, metadata_file, indent=4)

    import stub_gen  # NOQA

    stub_gen.run(args.metadata, api_json_path)

stdout.close()
