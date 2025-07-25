# MIT license; Copyright (c) 2021 Amir Gonnen
# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# TODO
# - Array conversion improvements:
#   - return custom iterable object instead of Blob when converting to array
#   - check array dim on conversion
# - On print extensions, print the reflected internal representation of the object (worth the extra ROM?)
# - Verify that when mp_obj is given it is indeed the right type (mp_lv_obj_t). Report error if not. can be added to mp_to_lv.
# - Implement inheritance instead of embed base methods (how? seems it's not supported, see https://github.com/micropython/micropython/issues/1159)
# - When converting mp to ptr (and vice versa), verify that types are compatible. Now all pointers are casted to void*.


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
    def memoized(*args, **kwargs):
        return func(*args, **kwargs)
    return memoized


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


# from pudb.remote import set_trace
# set_trace(term_size=(180, 50))

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
    result = f'{self.__class__.__name__}('
    res = []

    for name in self.__slots__[:-2]:
        dta = f'{name}={_repr(getattr(self, name))}'
        res.append(dta)

    res = ',\n'.join(res)

    if isinstance(self, c_ast.FileAST) or len(
        self.__slots__[:-2]
        ) > 1 or res.count('\n'):
        res = '\n'.join(f'  {line}' for line in res.split('\n'))
        result += f'\n{res}\n)'
    else:
        result += f'{res})'

    return result


#
# Argument parsing
#
argParser = argparse.ArgumentParser()
argParser.add_argument('-I', '--include', dest='include', help='Preprocesor include path', metavar='<Include Path>', action='append', default=[])
argParser.add_argument('-D', '--define', dest='define', help='Define preprocessor macro', metavar='<Macro Name>', action='append', default=[])
argParser.add_argument('--module_name', dest='module_name', help='Module name', metavar='<Module name string>', action='store')
argParser.add_argument('--module_prefix', dest='module_prefix', help='Module prefix that starts every function name', metavar='<Prefix string>', action='store')
argParser.add_argument('--metadata', dest='metadata', help='Optional file to emit metadata (introspection)', metavar='<MetaData File Name>', action='store')
argParser.add_argument('--board', dest='board', help='Board or OS', metavar='<Board or OS>', action='store', default='')
argParser.add_argument('--output', dest='output', help='Output file path', metavar='<Output path>', action='store')
argParser.add_argument('--debug', dest='debug', help='enable debugging output', action='store_true')
argParser.add_argument('--header_file', dest='header', action='store', default=None)

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


json_ast = fixed_gen_json.run(lv_config_path, os.path.join(project_path, 'build', 'lvgl_header.h'), False)

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

    def get_path(name: str, p: str) -> str:
        for file in os.listdir(p):
            file = os.path.join(p, file)

            if file.endswith(name):
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
def remove_declname(ast):
    if hasattr(ast, 'declname'):
        ast.declname = None
    if isinstance(ast, tuple):
        remove_declname(ast[1])
        return
    for i, c1 in enumerate(ast.children()):
        child = ast.children()[i]
        remove_declname(child)


@memoize
def add_default_declname(ast, name):
    if hasattr(ast, 'declname'):
        if (ast.declname == None):
            ast.declname = name
    if isinstance(ast, tuple):
        add_default_declname(ast[1], name)
        return
    for i, c1 in enumerate(ast.children()):
        child = ast.children()[i]
        add_default_declname(child, name)


@memoize
def convert_array_to_ptr(ast):
    if hasattr(ast, 'type') and isinstance(ast.type, c_ast.ArrayDecl):
        ast.type = c_ast.PtrDecl(ast.type.quals if hasattr(ast.type, 'quals') else [], ast.type.type)
    if isinstance(ast, tuple):
        return convert_array_to_ptr(ast[1])
    for i, c1 in enumerate(ast.children()):
        child = ast.children()[i]
        convert_array_to_ptr(child)


@memoize
def remove_quals(ast):
    if hasattr(ast,'quals'):
        ast.quals = []
    if hasattr(ast,'dim_quals'):
        ast.dim_quals = []
    if isinstance(ast, tuple):
        return remove_quals(ast[1])
    for i, c1 in enumerate(ast.children()):
        child = ast.children()[i]
        if not isinstance(child, c_ast.FuncDecl): # Don't remove quals which change function prototype
            remove_quals(child)


@memoize
def remove_explicit_struct(ast):
    if isinstance(ast, c_ast.TypeDecl) and isinstance(ast.type, c_ast.Struct):
        explicit_struct_name = ast.type.name
        # eprint('--> replace %s by %s in:\n%s' % (explicit_struct_name, explicit_structs[explicit_struct_name] if explicit_struct_name in explicit_structs else '???', ast))
        if explicit_struct_name:
            if explicit_struct_name in explicit_structs:
                ast.type = c_ast.IdentifierType([explicit_structs[explicit_struct_name]])
            elif explicit_struct_name in structs:
                ast.type = c_ast.IdentifierType([explicit_struct_name])
    if isinstance(ast, tuple):
        return remove_explicit_struct(ast[1])
    for i, c1 in enumerate(ast.children()):
        child = ast.children()[i]
        remove_explicit_struct(child)


@memoize
def get_type(arg, **kwargs):
    if isinstance(arg, str):
        return arg
    remove_quals_arg = 'remove_quals' in kwargs and kwargs['remove_quals']
    arg_ast = copy.deepcopy(arg)
    remove_explicit_struct(arg_ast)
    if remove_quals_arg: remove_quals(arg_ast)
    return gen.visit(arg_ast)


@memoize
def get_name(type):
    if isinstance(type, c_ast.Decl):
        return type.name
    if isinstance(type, c_ast.Struct) and type.name and type.name in explicit_structs:
        return explicit_structs[type.name]
    if isinstance(type, c_ast.Struct):
        return type.name
    if isinstance(type, c_ast.TypeDecl):
        return type.declname
    if isinstance(type, c_ast.IdentifierType):
        return type.names[0]
    if isinstance(type, c_ast.FuncDecl):
        return type.type.declname
    # if isinstance(type, (c_ast.PtrDecl, c_ast.ArrayDecl)) and hasattr(type.type, 'declname'):
    #     return type.type.declname
    if isinstance(type, (c_ast.PtrDecl, c_ast.ArrayDecl)):
        return get_type(type, remove_quals=True)
    else:
        return gen.visit(type)


@memoize
def remove_arg_names(ast):
    if isinstance(ast, c_ast.TypeDecl):
        ast.declname = None
        remove_arg_names(ast.type)
    elif isinstance(ast, c_ast.Decl): remove_arg_names(ast.type)
    elif isinstance(ast, c_ast.FuncDecl): remove_arg_names(ast.args)
    elif isinstance(ast, c_ast.ParamList):
        for param in ast.params: remove_arg_names(param)


# Create a function prototype AST from a function AST
@memoize
def function_prototype(func):
    bare_func = copy.deepcopy(func)
    remove_declname(bare_func)

    ptr_decl = c_ast.PtrDecl(
        quals=[],
        type=bare_func.type)

    func_proto = c_ast.Typename(
            name=None,
            quals=[],
            align=[],
            type=ptr_decl)

    return func_proto


#
# module specific text patterns
# IGNORECASE and "lower" are used to match both function and enum names
#
base_obj_name = 'obj'
base_obj_type = '%s_%s_t' % (module_prefix, base_obj_name)
lv_ext_pattern = re.compile('^{prefix}_([^_]+)_ext_t'.format(prefix=module_prefix))
lv_obj_pattern = re.compile('^{prefix}_([^_]+)'.format(prefix=module_prefix), re.IGNORECASE)
lv_func_pattern = re.compile('^{prefix}_(.+)'.format(prefix=module_prefix), re.IGNORECASE)
create_obj_pattern = re.compile('^{prefix}_(.+)_create$'.format(prefix=module_prefix))
lv_method_pattern = re.compile('^{prefix}_[^_]+_(.+)'.format(prefix=module_prefix), re.IGNORECASE)
lv_base_obj_pattern = re.compile('^(struct _){{0,1}}{prefix}_{base_name}_t( [*]){{0,1}}'.format(prefix=module_prefix, base_name = base_obj_name))
lv_str_enum_pattern = re.compile('^_?{prefix}_STR_(.+)'.format(prefix=module_prefix.upper()))
lv_callback_type_pattern = re.compile('({prefix}_){{0,1}}(.+)_cb(_t){{0,1}}'.format(prefix=module_prefix))
lv_global_callback_pattern = re.compile('.*g_cb_t')
lv_func_returns_array = re.compile('.*_array$')
lv_enum_name_pattern = re.compile('^(ENUM_){{0,1}}({prefix}_){{0,1}}(.*)'.format(prefix=module_prefix.upper()))


# Prevent identifier names which are Python reserved words (add underscore in such case)
def sanitize(id, kwlist =
        ['False', 'None', 'True', 'and', 'as', 'assert', 'break', 'class', 'continue', 'def', 'del', 'elif', 'else',
         'except', 'finally', 'for', 'from', 'global', 'if', 'import', 'in', 'is', 'lambda', 'nonlocal', 'not', 'or',
         'pass', 'raise', 'return', 'try', 'while', 'with', 'yield']):
    if id in kwlist:
        result = "_%s" % id
    else:
        result = id
    result = result.strip()
    result = result.replace(' ','_')
    result = result.replace('*','_ptr')
    return result


@memoize
def simplify_identifier(id):
    match_result = lv_func_pattern.match(id)
    return match_result.group(1) if match_result else id


def obj_name_from_ext_name(ext_name):
    return lv_ext_pattern.match(ext_name).group(1)


def obj_name_from_func_name(func_name):
    return lv_obj_pattern.match(func_name).group(1)


def ctor_name_from_obj_name(obj_name):
    return '{prefix}_{obj}_create'.format(prefix=module_prefix, obj=obj_name)


def is_method_of(func_name, obj_name):
    return func_name.lower().startswith('{prefix}_{obj}_'.format(prefix=module_prefix, obj=obj_name).lower())


def method_name_from_func_name(func_name):
    res = lv_method_pattern.match(func_name).group(1)
    return res if res != "del" else "delete" # del is a reserved name, don't use it


def get_enum_name(enum):
    match_result = lv_enum_name_pattern.match(enum)
    return match_result.group(3) if match_result else enum


def str_enum_to_str(str_enum):
    res = lv_str_enum_pattern.match(str_enum).group(1)
    return ('%s_' % module_prefix.upper()) + res


def is_obj_ctor(func):
    # ctor name must match pattern
    if not create_obj_pattern.match(func.name): return False
    # ctor must return a base_obj type
    if not lv_base_obj_pattern.match(get_type(func.type.type, remove_quals=True)): return False
    # ctor must receive (at least) one base obj parameters
    args = func.type.args.params
    if len(args) < 1: return False
    if not lv_base_obj_pattern.match(get_type(args[0].type, remove_quals=True)): return False
    return True


def is_global_callback(arg_type):
    arg_type_str = get_name(arg_type.type)
    # print('/* --> is_global_callback %s: %s */' % (lv_global_callback_pattern.match(arg_type_str), arg_type_str))
    result = lv_global_callback_pattern.match(arg_type_str)
    return result


def get_py_type(type_name):

    def _get_best_match(tn):
        best_match = ''

        for objnme in obj_names:
            if tn.startswith(objnme):
                if len(objnme) > len(best_match):
                    best_match = objnme

        return best_match

    type_name = type_name.replace('const lv_', 'lv_').replace('const _lv_', '_lv_').strip()

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
def is_struct(type):
    return isinstance(type, c_ast.Struct) or isinstance(type, c_ast.Union)


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
        item.name and item.type.declname and item.name == item.type.declname and
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

    def __getattr__(self, item):
        if item in self.__dict__:
            return self.__dict__[item]

        return getattr(self._stdout, item)


stdout = STDOut()

_old_excepthook = sys.excepthook


def my_excepthook(exc_type, exc_value, tb):
    stdout.close()
    return _old_excepthook(exc_type, exc_value, tb)


sys.excepthook = my_excepthook

# Types and structs

typedefs = [x.type for x in ast.ext if isinstance(x, c_ast.Typedef)] # and not (hasattr(x.type, 'declname') and lv_base_obj_pattern.match(x.type.declname))]
# print('/* %s */' % str(typedefs))
synonym = {}
for t in typedefs:
    if isinstance(t, c_ast.TypeDecl) and isinstance(t.type, c_ast.IdentifierType):
        if t.declname != t.type.names[0]:
            synonym[t.declname] = t.type.names[0]
            # eprint('%s === %s' % (t.declname, t.type.names[0]))
    if isinstance(t, c_ast.TypeDecl) and isinstance(t.type, c_ast.Struct):
        if t.declname != t.type.name:
            synonym[t.declname] = t.type.name
            # eprint('%s === struct %s' % (t.declname, t.type.name))
struct_typedefs = [typedef for typedef in typedefs if is_struct(typedef.type)]
structs_without_typedef = collections.OrderedDict((decl.type.name, decl.type) for decl in ast.ext if hasattr(decl, 'type') and is_struct(decl.type))

# for typedefs that referenced to a forward declaration struct, replace it with the real definition.
for typedef in struct_typedefs:
    if typedef.type.decls is None: # None means it's a forward declaration
        struct_name = typedef.type.name
        # check if it's found in `structs_without_typedef`. It actually has the typedef. Replace type with it.
        if typedef.type.name in structs_without_typedef:
            typedef.type = structs_without_typedef[struct_name]

structs = collections.OrderedDict((typedef.declname, typedef.type) for typedef in struct_typedefs if typedef.declname and typedef.type.decls) # and not lv_base_obj_pattern.match(typedef.declname))
structs.update(structs_without_typedef) # This is for struct without typedef
explicit_structs = collections.OrderedDict((typedef.type.name, typedef.declname) for typedef in struct_typedefs if typedef.type.name) # and not lv_base_obj_pattern.match(typedef.type.name))
opaque_structs = collections.OrderedDict((typedef.declname, c_ast.Struct(name=typedef.declname, decls=[])) for typedef in typedefs if isinstance(typedef.type, c_ast.Struct) and typedef.type.decls == None)
structs.update({k:v for k,v in opaque_structs.items() if k not in structs})
# print('/* --> opaque structs len = %d */' % len(opaque_structs))
# print('/* --> opaque structs  %s */' % ',\n'.join([struct_name for struct_name in opaque_structs]))
# print('/* --> structs:\n%s */' % ',\n'.join(sorted(str(structs[struct_name]) for struct_name in structs if struct_name)))
# print('/* --> structs_without_typedef:\n%s */' % ',\n'.join(sorted(str(structs_without_typedef[struct_name]) for struct_name in structs_without_typedef if struct_name)))
# print('/* --> explicit_structs:\n%s */' % ',\n'.join(sorted(struct_name + " = " + str(explicit_structs[struct_name]) for struct_name in explicit_structs if struct_name)))
# print('/* --> structs without typedef:\n%s */' % ',\n'.join(sorted(str(structs[struct_name]) for struct_name in structs_without_typedef)))

# Functions and objects

func_defs = [x.decl for x in ast.ext if isinstance(x, c_ast.FuncDef)]
func_decls = [x for x in ast.ext if isinstance(x, c_ast.Decl) and isinstance(x.type, c_ast.FuncDecl)]
all_funcs = func_defs + func_decls
funcs = [f for f in all_funcs if not f.name.startswith('_')] # functions that start with underscore are usually internal
# eprint('... %s' % ',\n'.join(sorted('%s' % func.name for func in funcs)))
obj_ctors = [func for func in funcs if is_obj_ctor(func)]


# eprint('CTORS(%d): %s' % (len(obj_ctors), ', '.join(sorted('%s' % ctor.name for ctor in obj_ctors))))
for obj_ctor in obj_ctors:
    funcs.remove(obj_ctor)
obj_names = [create_obj_pattern.match(ctor.name).group(1) for ctor in obj_ctors]


def has_ctor(obj_name):
    return ctor_name_from_obj_name(obj_name) in [ctor.name for ctor in obj_ctors]


def get_ctor(obj_name):
    global obj_ctors
    return next(ctor for ctor in obj_ctors if ctor.name == ctor_name_from_obj_name(obj_name))


def get_methods(obj_name):
    global funcs
    return [func for func in funcs \
            if is_method_of(func.name,obj_name) and \
            (not func.name == ctor_name_from_obj_name(obj_name))]


@memoize
def noncommon_part(member_name, stem_name):
    common_part = os.path.commonprefix([member_name, stem_name])
    n = len(common_part) - 1
    while n > 0 and member_name[n] != '_': n-=1
    return member_name[n+1:]


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
    return get_type(first_arg.type, remove_quals = True)


def get_base_struct_name(struct_name):
    return struct_name[:-2] if struct_name.endswith('_t') else struct_name


# "struct function" starts with struct name (without _t), and their first argument is a pointer to the struct
# Need also to take into account struct functions of aliases of current struct.
@memoize
def get_struct_functions(struct_name):
    global funcs
    if not struct_name:
        return []
    base_struct_name = get_base_struct_name(struct_name)
    # eprint("get_struct_functions %s: %s" % (struct_name, [get_type(func.type.args.params[0].type.type, remove_quals = True) for func in funcs if func.name.startswith(base_struct_name)]))
    # eprint("get_struct_functions %s: %s" % (struct_name, struct_aliases[struct_name] if struct_name in struct_aliases else ""))

    # for func in funcs:
    #     print("/* get_struct_functions: func=%s, struct=%s, noncommon part=%s */" % (simplify_identifier(func.name), simplify_identifier(struct_name),
    #         noncommon_part(simplify_identifier(func.name), simplify_identifier(struct_name))))

    reverse_aliases = [alias for alias in struct_aliases if struct_aliases[alias] == struct_name]
    res = []

    if struct_name in structs or len(reverse_aliases) > 0:
        for func in funcs:
            if noncommon_part(simplify_identifier(func.name), simplify_identifier(struct_name)) != simplify_identifier(func.name):
                if get_first_arg_type(func) == struct_name:
                    res.append(func)

    if struct_name in struct_aliases:
        res.extend(get_struct_functions(struct_aliases[struct_name]))

    return res


@memoize
def is_struct_function(func):
    return func in get_struct_functions(get_first_arg_type(func))

# is_static_member returns true if function does not receive the obj as the first argument
# and the object is not a struct function


@memoize
def is_static_member(func, obj_type=base_obj_type):
    first_arg = get_first_arg(func)
    # print("/* %s : get_first_arg = %s */" % (get_name(func), first_arg))
    if first_arg:
        if isinstance(first_arg, c_ast.ArrayDecl):
            return True # Arrays cannot have non static members
    if is_struct_function(func):
        return False
    first_arg_type = get_first_arg_type(func)
    return (first_arg_type == None) or (first_arg_type != obj_type)


# All object should inherit directly from base_obj, and not according to lv_ext, as disccussed on https://github.com/littlevgl/lv_binding_micropython/issues/19
parent_obj_names = {child_name: base_obj_name for child_name in obj_names if child_name != base_obj_name}
parent_obj_names[base_obj_name] = None

# Populate inheritance hierarchy according to lv_ext structures
# exts = {obj_name_from_ext_name(ext.name): ext for ext in ast.ext if hasattr(ext, 'name') and ext.name is not None and lv_ext_pattern.match(ext.name)}
# for obj_name, ext in exts.items():
#     try:
#         parent_ext_name = ext.type.type.decls[0].type.type.names[0]
#         if lv_ext_pattern.match(parent_ext_name):
#             parent_obj_names[obj_name] = obj_name_from_ext_name(parent_ext_name)
#     except AttributeError:
#         pass

# Parse Enums

enum_defs = [x for x in ast.ext if hasattr(x,'type') and isinstance(x.type, c_ast.Enum)]
enum_defs += [x.type for x in ast.ext if hasattr(x, 'type') and hasattr(x.type, 'type') and isinstance(x.type, c_ast.TypeDecl) and isinstance(x.type.type, c_ast.Enum)]


# Enum member access functions.
def get_enum_members(obj_name):
    global enums
    if not obj_name in enums:
        return []
    return [enum_member_name for enum_member_name, value in enums[obj_name].items()]


def get_enum_member_name(enum_member):
    if enum_member[0].isdigit():
        enum_member = '_' + enum_member # needs to be a valid attribute name
    return enum_member


def get_enum_value(obj_name, enum_member):
    return enums[obj_name][enum_member]

# eprint(enums)

# parse function pointers
func_typedefs = collections.OrderedDict((t.name, t) for t in ast.ext if isinstance(t, c_ast.Typedef) and isinstance(t.type, c_ast.PtrDecl) and isinstance(t.type.type, c_ast.FuncDecl))

# Global blobs
blobs = collections.OrderedDict((decl.name, decl.type.type) for decl in ast.ext \
    if isinstance(decl, c_ast.Decl) \
        and 'extern' in decl.storage \
        and hasattr(decl, 'type') \
        and isinstance(decl.type, c_ast.TypeDecl)
        and not decl.name.startswith('_'))

blobs['_nesting'] = parser.parse('extern int _nesting;').ext[0].type.type

int_constants = []


#
# Type convertors
#
class MissingConversionException(ValueError):
    pass


mp_to_lv = {
    'mp_obj_t'                  : '(mp_obj_t)',
    'va_list'                   : None,
    'void *'                    : 'mp_to_ptr',
    'const uint8_t *'           : 'mp_to_ptr',
    'const void *'              : 'mp_to_ptr',
    'bool'                      : 'mp_obj_is_true',
    'char *'                    : '(char*)convert_from_str',
    'char **'                   : 'mp_write_ptr_C_Pointer',
    'const char *'              : 'convert_from_str',
    'const char **'             : 'mp_write_ptr_C_Pointer',
    '%s_obj_t *'% module_prefix : 'mp_to_lv',
    'uint8_t'                   : '(uint8_t)mp_obj_get_int',
    'uint16_t'                  : '(uint16_t)mp_obj_get_int',
    'uint32_t'                  : '(uint32_t)mp_obj_get_int',
    'uint64_t'                  : '(uint64_t)mp_obj_get_ull',
    'unsigned'                  : '(unsigned)mp_obj_get_int',
    'unsigned int'              : '(unsigned int)mp_obj_get_int',
    'unsigned char'             : '(unsigned char)mp_obj_get_int',
    'unsigned short'            : '(unsigned short)mp_obj_get_int',
    'unsigned long'             : '(unsigned long)mp_obj_get_int',
    'unsigned long int'         : '(unsigned long int)mp_obj_get_int',
    'unsigned long long'        : '(unsigned long long)mp_obj_get_ull',
    'unsigned long long int'    : '(unsigned long long int)mp_obj_get_ull',
    'int8_t'                    : '(int8_t)mp_obj_get_int',
    'int16_t'                   : '(int16_t)mp_obj_get_int',
    'int32_t'                   : '(int32_t)mp_obj_get_int',
    'int64_t'                   : '(int64_t)mp_obj_get_ull',
    'size_t'                    : '(size_t)mp_obj_get_int',
    'int'                       : '(int)mp_obj_get_int',
    'char'                      : '(char)mp_obj_get_int',
    'short'                     : '(short)mp_obj_get_int',
    'long'                      : '(long)mp_obj_get_int',
    'long int'                  : '(long int)mp_obj_get_int',
    'long long'                 : '(long long)mp_obj_get_ull',
    'long long int'             : '(long long int)mp_obj_get_ull',
    'float'                     : '(float)mp_obj_get_float',
}

lv_to_mp = {
    'mp_obj_t'                  : '(mp_obj_t)',
    'va_list'                   : None,
    'void *'                    : 'ptr_to_mp',
    'const uint8_t *'           : 'ptr_to_mp',
    'const void *'              : 'ptr_to_mp',
    'bool'                      : 'convert_to_bool',
    'char *'                    : 'convert_to_str',
    'char **'                   : 'mp_read_ptr_C_Pointer',
    'const char *'              : 'convert_to_str',
    'const char **'             : 'mp_read_ptr_C_Pointer',
    '%s_obj_t *'% module_prefix : 'lv_to_mp',
    'uint8_t'                   : 'mp_obj_new_int_from_uint',
    'uint16_t'                  : 'mp_obj_new_int_from_uint',
    'uint32_t'                  : 'mp_obj_new_int_from_uint',
    'uint64_t'                  : 'mp_obj_new_int_from_ull',
    'unsigned'                  : 'mp_obj_new_int_from_uint',
    'unsigned int'              : 'mp_obj_new_int_from_uint',
    'unsigned char'             : 'mp_obj_new_int_from_uint',
    'unsigned short'            : 'mp_obj_new_int_from_uint',
    'unsigned long'             : 'mp_obj_new_int_from_uint',
    'unsigned long int'         : 'mp_obj_new_int_from_uint',
    'unsigned long long'        : 'mp_obj_new_int_from_ull',
    'unsigned long long int'    : 'mp_obj_new_int_from_ull',
    'int8_t'                    : 'mp_obj_new_int',
    'int16_t'                   : 'mp_obj_new_int',
    'int32_t'                   : 'mp_obj_new_int',
    'int64_t'                   : 'mp_obj_new_int_from_ll',
    'size_t'                    : 'mp_obj_new_int_from_uint',
    'int'                       : 'mp_obj_new_int',
    'char'                      : 'mp_obj_new_int',
    'short'                     : 'mp_obj_new_int',
    'long'                      : 'mp_obj_new_int',
    'long int'                  : 'mp_obj_new_int',
    'long long'                 : 'mp_obj_new_int_from_ll',
    'long long int'             : 'mp_obj_new_int_from_ll',
    'float'                     : 'mp_obj_new_float',
}

lv_mp_type = {
    'mp_obj_t'                  : '%s*' % base_obj_type,
    'va_list'                   : None,
    'void *'                    : 'void*',
    'const uint8_t *'           : 'void*',
    'const void *'              : 'void*',
    'bool'                      : 'bool',
    'char *'                    : 'char*',
    'char **'                   : 'char**',
    'const char *'              : 'char*',
    'const char **'             : 'char**',
    '%s_obj_t *'% module_prefix : '%s*' % base_obj_type,
    'uint8_t'                   : 'int',
    'uint16_t'                  : 'int',
    'uint32_t'                  : 'int',
    'uint64_t'                  : 'int',
    'unsigned'                  : 'int',
    'unsigned int'              : 'int',
    'unsigned char'             : 'int',
    'unsigned short'            : 'int',
    'unsigned long'             : 'int',
    'unsigned long int'         : 'int',
    'unsigned long long'        : 'int',
    'unsigned long long int'    : 'int',
    'int8_t'                    : 'int',
    'int16_t'                   : 'int',
    'int32_t'                   : 'int',
    'int64_t'                   : 'int',
    'size_t'                    : 'int',
    'int'                       : 'int',
    'char'                      : 'int',
    'short'                     : 'int',
    'long'                      : 'int',
    'long int'                  : 'int',
    'long long'                 : 'int',
    'long long int'             : 'int',
    'void'                      : None,
    'float'                     : 'float',
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
            mp_to_lv[qualified_ptr_type] = 'mp_array_to_%s' % (convertor)
            lv_to_mp[qualified_ptr_type] = 'mp_array_from_%s' % (convertor)
            lv_mp_type[qualified_ptr_type] = 'void*'


register_int_ptr_type('u8ptr',
        'unsigned char *',
        'uint8_t *')

# char* is considered as string, not int ptr!
'''
register_int_ptr_type('i8ptr',
        'char *',
        'int8_t *')
'''

register_int_ptr_type('u16ptr',
        'unsigned short *',
        'uint16_t *')

register_int_ptr_type('i16ptr',
        'short *',
        'int16_t *')

register_int_ptr_type('u32ptr',
        'uint32_t *',
        'unsigned *',
        'unsigned int *',
        'unsigned long *',
        'unsigned long int *',
        'size_t *')

register_int_ptr_type('i32ptr',
        'int32_t *',
        'signed *',
        'signed int *',
        'signed long *',
        'signed long int *',
        'long *',
        'long int *',
        'int *')

register_int_ptr_type('u64ptr',
        'int64_t *',
        'signed long long *',
        'long long *',
        'long long int *')

register_int_ptr_type('i64ptr',
        'uint64_t *',
        'unsigned long long *',
        'unsigned long long int *')


#
# Emit Header
#

input_headers = [input_header, private_header]

print ("""
/*
 * Auto-Generated file, DO NOT EDIT!
 *
 * Command line:
 * {cmd_line}
 *
 * Preprocessing command:
 * {pp_cmd}
 *
 * Generating Objects: {objs}
 */

/*
 * Mpy includes
 */

#include <stdlib.h>
#include <string.h>
#include "py/obj.h"
#include "py/objint.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objarray.h"
#include "py/objtype.h"
#include "py/objexcept.h"

/*
 * {module_name} includes
 */

{lv_headers}
""".format(
        module_name = module_name,
        cmd_line=' '.join(sys.argv),
        pp_cmd=pp_cmd,
        objs=", ".join(['%s(%s)' % (objname, parent_obj_names[objname]) for objname in obj_names]),
        lv_headers='\n'.join('#include "%s"' % header for header in input_headers)))

#
# Enable objects, if supported
#

if len(obj_names) > 0:
    print("""
#define LV_OBJ_T {obj_type}

typedef struct mp_lv_obj_type_t {{
    const lv_obj_class_t *lv_obj_class;
    const mp_obj_type_t *mp_obj_type;
}} mp_lv_obj_type_t;

static const mp_lv_obj_type_t mp_lv_{base_obj}_type;
static const mp_lv_obj_type_t *mp_lv_obj_types[];

static const mp_obj_type_t *get_BaseObj_type()
{{
    return mp_lv_{base_obj}_type.mp_obj_type;
}}

MP_DEFINE_EXCEPTION(LvReferenceError, Exception)
    """.format(
            obj_type = base_obj_type,
            base_obj = base_obj_name
        ))

#
# Emit Mpy helper functions
#

print("""
/*
 * Helper functions
 */

#ifndef GENMPY_UNUSED
#ifdef __GNUC__
#define GENMPY_UNUSED __attribute__ ((unused))
#else
#define GENMPY_UNUSED
#endif // __GNUC__
#endif // GENMPY_UNUSED
 
// Custom function mp object

typedef mp_obj_t (*mp_fun_ptr_var_t)(size_t n, const mp_obj_t *, void *ptr);

typedef struct mp_lv_obj_fun_builtin_var_t {
    mp_obj_base_t base;
    mp_uint_t n_args;
    mp_fun_ptr_var_t mp_fun;
    void *lv_fun;
} mp_lv_obj_fun_builtin_var_t;

static mp_obj_t lv_fun_builtin_var_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
static mp_int_t mp_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);

GENMPY_UNUSED static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_type_fun_builtin_var,
    MP_QSTR_function,
    MP_TYPE_FLAG_BINDS_SELF | MP_TYPE_FLAG_BUILTIN_FUN,
    call, lv_fun_builtin_var_call,
    buffer, mp_func_get_buffer
);

GENMPY_UNUSED static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_type_fun_builtin_static_var,
    MP_QSTR_function,
    MP_TYPE_FLAG_BUILTIN_FUN,
    call, lv_fun_builtin_var_call,
    buffer, mp_func_get_buffer
);

static mp_obj_t lv_fun_builtin_var_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_var) ||
           MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_static_var));
    mp_lv_obj_fun_builtin_var_t *self = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, self->n_args, self->n_args, false);
    return self->mp_fun(n_args, args, self->lv_fun);
}

static mp_int_t mp_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    assert(MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_var) ||
           MP_OBJ_IS_TYPE(self_in, &mp_lv_type_fun_builtin_static_var));
    mp_lv_obj_fun_builtin_var_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->lv_fun;
    bufinfo->len = sizeof(self->lv_fun);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

#define MP_DEFINE_CONST_LV_FUN_OBJ_VAR(obj_name, n_args, mp_fun, lv_fun) \\
    const mp_lv_obj_fun_builtin_var_t obj_name = \\
        {{&mp_lv_type_fun_builtin_var}, n_args, mp_fun, lv_fun}

#define MP_DEFINE_CONST_LV_FUN_OBJ_STATIC_VAR(obj_name, n_args, mp_fun, lv_fun) \\
    const mp_lv_obj_fun_builtin_var_t obj_name = \\
        {{&mp_lv_type_fun_builtin_static_var}, n_args, mp_fun, lv_fun}

// Casting

typedef struct mp_lv_struct_t
{
    mp_obj_base_t base;
    void *data;
} mp_lv_struct_t;

static const mp_lv_struct_t mp_lv_null_obj;

#ifdef LV_OBJ_T
static mp_int_t mp_lv_obj_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);
#else
static mp_int_t mp_lv_obj_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags){ return 0; }
#endif

static mp_int_t mp_blob_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);

static mp_obj_t get_native_obj(mp_obj_t mp_obj)
{
    if (!MP_OBJ_IS_OBJ(mp_obj)) return mp_obj;
    const mp_obj_type_t *native_type = ((mp_obj_base_t*)mp_obj)->type;
    if (native_type == NULL)
        return NULL;
    if (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, parent) == NULL ||
        (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, buffer) == mp_blob_get_buffer) ||
        (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, buffer) == mp_lv_obj_get_buffer))
       return mp_obj;
    while (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, parent)) native_type = MP_OBJ_TYPE_GET_SLOT(native_type, parent);
    return mp_obj_cast_to_native_base(mp_obj, MP_OBJ_FROM_PTR(native_type));
}

static mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type);

static mp_obj_t make_new_lv_struct(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args);

static mp_obj_t cast(mp_obj_t mp_obj, const mp_obj_type_t *mp_type)
{
    mp_obj_t res = NULL;
    if (mp_obj == mp_const_none && MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_type, make_new) == &make_new_lv_struct) {
        res = MP_OBJ_FROM_PTR(&mp_lv_null_obj);
    } else if (MP_OBJ_IS_OBJ(mp_obj)) {
        res = get_native_obj(mp_obj);
        if (res){
            const mp_obj_type_t *res_type = ((mp_obj_base_t*)res)->type;
            if (res_type != mp_type){
                if (res_type == &mp_type_dict &&
                    MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_type, make_new) == &make_new_lv_struct)
                        res = dict_to_struct(res, mp_type);
                else res = NULL;
            }
        }
    }
    if (res == NULL) nlr_raise(
        mp_obj_new_exception_msg_varg(
            &mp_type_SyntaxError, MP_ERROR_TEXT("Can't convert %s to %s!"), mp_obj_get_type_str(mp_obj), qstr_str(mp_type->name)));
    return res;
}

// object handling
// This section is enabled only when objects are supported

#ifdef LV_OBJ_T

typedef struct mp_lv_obj_t {
    mp_obj_base_t base;
    LV_OBJ_T *lv_obj;
    LV_OBJ_T *callbacks;
} mp_lv_obj_t;

static LV_OBJ_T *mp_to_lv(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;
    mp_obj_t native_obj = get_native_obj(mp_obj);
    if (MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_obj_get_type(native_obj), buffer) != mp_lv_obj_get_buffer)
        return NULL;
    mp_lv_obj_t *mp_lv_obj = MP_OBJ_TO_PTR(native_obj);
    if (mp_lv_obj->lv_obj == NULL) {
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_LvReferenceError, MP_ERROR_TEXT("Referenced object was deleted!")));
    }
    return mp_lv_obj->lv_obj;
}

static LV_OBJ_T *mp_get_callbacks(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;
    mp_lv_obj_t *mp_lv_obj = MP_OBJ_TO_PTR(get_native_obj(mp_obj));
    if (mp_lv_obj == NULL)
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("'user_data' argument must be either a dict or None!")));
    if (!mp_lv_obj->callbacks) mp_lv_obj->callbacks = mp_obj_new_dict(0);
    return mp_lv_obj->callbacks;
}

static const mp_obj_type_t *get_BaseObj_type();

static void mp_lv_delete_cb(lv_event_t * e)
{
    LV_OBJ_T *lv_obj = e->current_target;
    if (lv_obj){
        mp_lv_obj_t *self = lv_obj->user_data;
        if (self) {
            self->lv_obj = NULL;
        }
    }
}

static mp_obj_t lv_to_mp(LV_OBJ_T *lv_obj)
{
    if (lv_obj == NULL) return mp_const_none;
    mp_lv_obj_t *self = (mp_lv_obj_t*)lv_obj->user_data;
    if (!self)
    {
        // Find the object type
        const mp_obj_type_t *mp_obj_type = get_BaseObj_type();
        const lv_obj_class_t *lv_obj_class = lv_obj_get_class(lv_obj);
        const mp_lv_obj_type_t **iter = &mp_lv_obj_types[0];
        for (; *iter; iter++) {
            if ((*iter)->lv_obj_class == lv_obj_class) {
                mp_obj_type = (*iter)->mp_obj_type;
                break;
            }
        }

        // Create the MP object
        self = m_new_obj(mp_lv_obj_t);
        *self = (mp_lv_obj_t){
            .base = {(const mp_obj_type_t *)mp_obj_type},
            .lv_obj = lv_obj,
            .callbacks = NULL,
        };

        // Register the Python object in user_data
        lv_obj->user_data = self;

        // Register a "Delete" event callback
        lv_obj_add_event_cb(lv_obj, mp_lv_delete_cb, LV_EVENT_DELETE, NULL);
    }
    return MP_OBJ_FROM_PTR(self);
}

static void* mp_to_ptr(mp_obj_t self_in);

static mp_obj_t cast_obj_type(const mp_obj_type_t* type, mp_obj_t obj)
{
    mp_lv_obj_t *self = m_new_obj(mp_lv_obj_t);
    *self = (mp_lv_obj_t){
        .base = {type},
        .lv_obj = mp_to_ptr(obj),
        .callbacks = NULL,
    };
    if (!self->lv_obj) return mp_const_none;
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t cast_obj(mp_obj_t type_obj, mp_obj_t obj)
{
    return cast_obj_type((const mp_obj_type_t *)type_obj, obj);
}

static mp_obj_t make_new(
    const mp_lv_obj_fun_builtin_var_t *lv_obj_var,
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args)
{
    mp_obj_t lv_obj;
    if (n_args == 0 && n_kw == 0) // allow no args, and pass NULL as parent in such case
    {
        const mp_obj_t no_args[] = {mp_const_none};
        lv_obj = mp_call_function_n_kw(MP_OBJ_FROM_PTR(lv_obj_var), 1, 0, no_args);
    }
    else
    {
        lv_obj = mp_call_function_n_kw(MP_OBJ_FROM_PTR(lv_obj_var), n_args, n_kw, args);
    }

    if (!lv_obj) return mp_const_none;

    mp_lv_obj_t *self = MP_OBJ_TO_PTR(lv_obj);
    if (self->base.type != type)
        return cast_obj_type(type, lv_obj);
    return lv_obj;
}

static MP_DEFINE_CONST_FUN_OBJ_2(cast_obj_obj, cast_obj);
static MP_DEFINE_CONST_CLASSMETHOD_OBJ(cast_obj_class_method, MP_ROM_PTR(&cast_obj_obj));

static mp_int_t mp_lv_obj_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    mp_lv_obj_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->lv_obj;
    bufinfo->len = sizeof(self->lv_obj);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

static mp_obj_t mp_lv_obj_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in)
{
    mp_lv_obj_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    mp_lv_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
    switch (op)
    {
        case MP_BINARY_OP_EQUAL:
            return mp_obj_new_bool(lhs->lv_obj == rhs->lv_obj);
        case MP_BINARY_OP_NOT_EQUAL:
            return mp_obj_new_bool(lhs->lv_obj != rhs->lv_obj);
        default:
            return MP_OBJ_NULL;
    }
}

// Register LVGL root pointers
MP_REGISTER_ROOT_POINTER(void *mp_lv_roots);
MP_REGISTER_ROOT_POINTER(void *mp_lv_user_data);

void *mp_lv_roots;

void mp_lv_init_gc()
{
    static bool mp_lv_roots_initialized = false;
    if (!mp_lv_roots_initialized) {
        mp_lv_roots = MP_STATE_VM(mp_lv_roots) = m_new0(lv_global_t, 1);
        mp_lv_roots_initialized = true;
    }
}

#else // LV_OBJ_T

typedef struct mp_lv_obj_type_t {
    mp_obj_type_t *mp_obj_type;
} mp_lv_obj_type_t;

#endif

static mp_obj_t convert_to_bool(bool b)
{
    return b? mp_const_true: mp_const_false;
}

static mp_obj_t convert_to_str(const char *str)
{
    return str? mp_obj_new_str(str, strlen(str)): mp_const_none;
}

static const char *convert_from_str(mp_obj_t str)
{
    if (str == NULL || str == mp_const_none)
        return NULL;

    if (MP_OBJ_IS_TYPE(str, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(str, &mp_type_memoryview)) {
            mp_buffer_info_t buffer_info;
            if (mp_get_buffer(str, &buffer_info, MP_BUFFER_READ)) {
                return buffer_info.buf;
            }
    }

    return mp_obj_str_get_str(str);
}

// struct handling

static mp_lv_struct_t *mp_to_lv_struct(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;
    mp_obj_t native_obj = get_native_obj(mp_obj);
    if ( (!MP_OBJ_IS_OBJ(native_obj)) || (MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_obj_get_type(native_obj), make_new) != &make_new_lv_struct) ) nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Expected Struct object!")));
    mp_lv_struct_t *mp_lv_struct = MP_OBJ_TO_PTR(native_obj);
    return mp_lv_struct;
}

static size_t get_lv_struct_size(const mp_obj_type_t *type)
{
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(MP_OBJ_TYPE_GET_SLOT(type, locals_dict));
    mp_map_elem_t *elem = mp_map_lookup(&self->map, MP_OBJ_NEW_QSTR(MP_QSTR___SIZE__), MP_MAP_LOOKUP);
    if (elem == NULL) {
        return 0;
    } else {
        return (size_t)mp_obj_get_int(elem->value);
    }
}

static mp_obj_t make_new_lv_struct(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args)
{
    if ((!MP_OBJ_IS_TYPE(type, &mp_type_type)) || MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, make_new) != &make_new_lv_struct)
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Argument is not a struct type!")));
    size_t size = get_lv_struct_size(type);
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);
    mp_lv_struct_t *other = (n_args > 0) && (!mp_obj_is_int(args[0])) ? mp_to_lv_struct(cast(args[0], type)): NULL;
    size_t count = (n_args > 0) && (mp_obj_is_int(args[0]))? mp_obj_get_int(args[0]): 1;
    *self = (mp_lv_struct_t){
        .base = {type},
        .data = (size == 0 || (other && other->data == NULL))? NULL: m_malloc(size * count)
    };
    if (self->data) {
        if (other) {
            memcpy(self->data, other->data, size * count);
        } else {
            memset(self->data, 0, size * count);
        }
    }
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t lv_struct_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in)
{
    mp_lv_struct_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    mp_lv_struct_t *rhs = MP_OBJ_TO_PTR(rhs_in);
    switch (op)
    {
        case MP_BINARY_OP_EQUAL:
            return mp_obj_new_bool(lhs->data == rhs->data);
        case MP_BINARY_OP_NOT_EQUAL:
            return mp_obj_new_bool(lhs->data != rhs->data);
        default:
            return MP_OBJ_NULL;
    }
}

static mp_obj_t lv_struct_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    mp_lv_struct_t *self = mp_to_lv_struct(self_in);

    if ((!self) || (!self->data))
        return NULL;
    if (!mp_obj_is_int(index)) {
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Subscript index must be an integer!")));
    }

    const mp_obj_type_t *type = mp_obj_get_type(self_in);
    size_t element_size = get_lv_struct_size(type);
    if (element_size == 0) return mp_const_none;
    size_t element_index = mp_obj_get_int(index);
    void *element_addr = (byte*)self->data + element_size*element_index;

    if (value == MP_OBJ_NULL) {
        memset(element_addr, 0, element_size);
        return self_in;
    }

    mp_lv_struct_t *element_at_index = m_new_obj(mp_lv_struct_t);
    *element_at_index = (mp_lv_struct_t){
        .base = {type},
        .data = element_addr
    };

    if (value != MP_OBJ_SENTINEL){
        mp_lv_struct_t *other = mp_to_lv_struct(cast(value, type));
        if ((!other) || (!other->data))
            return NULL;
        memcpy(element_at_index->data, other->data, element_size);
    }

    return MP_OBJ_FROM_PTR(element_at_index);
}

GENMPY_UNUSED static void *copy_buffer(const void *buffer, size_t size)
{
    void *new_buffer = m_malloc(size);
    memcpy(new_buffer, buffer, size);
    return new_buffer;
}

// Reference an existing lv struct (or part of it)

static mp_obj_t lv_to_mp_struct(const mp_obj_type_t *type, void *lv_struct)
{
    if (lv_struct == NULL) return mp_const_none;
    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);
    *self = (mp_lv_struct_t){
        .base = {type},
        .data = lv_struct
    };
    return MP_OBJ_FROM_PTR(self);
}

static void call_parent_methods(mp_obj_t obj, qstr attr, mp_obj_t *dest)
{
    const mp_obj_type_t *type = mp_obj_get_type(obj);
    while (MP_OBJ_TYPE_HAS_SLOT(type, locals_dict)) {
        // generic method lookup
        // this is a lookup in the object (ie not class or type)
        assert(MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->base.type == &mp_type_dict); // MicroPython restriction, for now
        mp_map_t *locals_map = &MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->map;
        mp_map_elem_t *elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
        if (elem != NULL) {
            mp_convert_member_lookup(obj, type, elem->value, dest);
            break;
        }
        if (MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, parent) == NULL) {
            break;
        }
        // search parents
        type = MP_OBJ_TYPE_GET_SLOT(type, parent);
    }
}

// Convert dict to struct

static mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type)
{
    mp_obj_t mp_struct = make_new_lv_struct(type, 0, 0, NULL);
    mp_obj_t native_dict = cast(dict, &mp_type_dict);
    mp_map_t *map = mp_obj_dict_get_map(native_dict);
    if (map == NULL) return mp_const_none;
    for (uint i = 0; i < map->alloc; i++) {
        mp_obj_t key = map->table[i].key;
        mp_obj_t value = map->table[i].value;
        if (key != MP_OBJ_NULL) {
            mp_obj_t dest[] = {MP_OBJ_SENTINEL, value};
            MP_OBJ_TYPE_GET_SLOT(type, attr)(mp_struct, mp_obj_str_get_qstr(key), dest);
            if (dest[0]) nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Cannot set field %s on struct %s!"), qstr_str(mp_obj_str_get_qstr(key)), qstr_str(type->name)));
        }
    }
    return mp_struct;
}

// Convert mp object to ptr

static void* mp_to_ptr(mp_obj_t self_in)
{
    mp_buffer_info_t buffer_info;
    if (self_in == NULL || self_in == mp_const_none)
        return NULL;

//    if (MP_OBJ_IS_INT(self_in))
//        return (void*)mp_obj_get_int(self_in);

    // If an object is user instance, take it as is so it could be used as user_data
    if (mp_obj_is_instance_type(mp_obj_get_type(self_in))){
        return MP_OBJ_TO_PTR(self_in);
    }

    if (!mp_get_buffer(self_in, &buffer_info, MP_BUFFER_READ)) {
        // No buffer protocol - this is not a Struct or a Blob, it's some other mp object.
        // We only allow setting dict directly, since it's useful to setting user_data for passing data to C.
        // On other cases throw an exception, to avoid a crash later
        if (MP_OBJ_IS_TYPE(self_in, &mp_type_dict))
            return MP_OBJ_TO_PTR(self_in);
        else nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Cannot convert '%s' to pointer!"), mp_obj_get_type_str(self_in)));
    }

    if (MP_OBJ_IS_STR_OR_BYTES(self_in) ||
        MP_OBJ_IS_TYPE(self_in, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(self_in, &mp_type_memoryview))
            return buffer_info.buf;
    else
    {
        void *result;
        if (buffer_info.len != sizeof(result) || buffer_info.typecode != BYTEARRAY_TYPECODE){
            nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Cannot convert %s to pointer! (buffer does not represent a pointer)"), mp_obj_get_type_str(self_in)));
        }
        memcpy(&result, buffer_info.buf, sizeof(result));
        return result;
    }
}

// Blob is a wrapper for void*


static mp_int_t mp_blob_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->data;
    bufinfo->len = sizeof(self->data);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}

static const mp_obj_fun_builtin_var_t mp_lv_dereference_obj;

// Sometimes (but not always!) Blob represents a Micropython object.
// In such cases it's safe to cast the Blob back to the Micropython object
// cast argument is the underlying object type, and it's optional.

static mp_obj_t mp_blob_cast(size_t argc, const mp_obj_t *argv)
{
    mp_obj_t self = argv[0];
    void *ptr = mp_to_ptr(self);
    if (argc == 1) return MP_OBJ_FROM_PTR(ptr);
    mp_obj_t type = argv[1];
    if (!MP_OBJ_IS_TYPE(type, &mp_type_type))
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Cast argument must be a type!")));
    return cast(MP_OBJ_FROM_PTR(ptr), type);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_blob_cast_obj, 1, 2, mp_blob_cast);

static const mp_rom_map_elem_t mp_blob_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___dereference__), MP_ROM_PTR(&mp_lv_dereference_obj) },
    { MP_ROM_QSTR(MP_QSTR___cast__), MP_ROM_PTR(&mp_blob_cast_obj) },
};

static MP_DEFINE_CONST_DICT(mp_blob_locals_dict, mp_blob_locals_dict_table);

static MP_DEFINE_CONST_OBJ_TYPE(
    mp_blob_type,
    MP_QSTR_Blob,
    MP_TYPE_FLAG_NONE,
    binary_op, lv_struct_binary_op,
    locals_dict, &mp_blob_locals_dict,
    buffer, mp_blob_get_buffer
);

static const mp_lv_struct_t mp_lv_null_obj = { {&mp_blob_type}, NULL };

static mp_obj_t ptr_to_mp(void *data)
{
    return lv_to_mp_struct(&mp_blob_type, data);
}

// Cast pointer to struct

static mp_obj_t mp_lv_cast(mp_obj_t type_obj, mp_obj_t ptr_obj)
{
    void *ptr = mp_to_ptr(ptr_obj);
    if (!ptr) return mp_const_none;
    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);
    *self = (mp_lv_struct_t){
        .base = {(const mp_obj_type_t*)type_obj},
        .data = ptr
    };
    return MP_OBJ_FROM_PTR(self);
}

// Cast instance. Can be used in ISR when memory allocation is prohibited

static mp_obj_t mp_lv_cast_instance(mp_obj_t self_in, mp_obj_t ptr_obj)
{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);
    self->data = mp_to_ptr(ptr_obj);
    return self_in;
}

static MP_DEFINE_CONST_FUN_OBJ_2(mp_lv_cast_obj, mp_lv_cast);
static MP_DEFINE_CONST_CLASSMETHOD_OBJ(mp_lv_cast_class_method, MP_ROM_PTR(&mp_lv_cast_obj));

static MP_DEFINE_CONST_FUN_OBJ_2(mp_lv_cast_instance_obj, mp_lv_cast_instance);

// Dereference a struct/blob. This allows access to the raw data the struct holds

static mp_obj_t mp_lv_dereference(size_t argc, const mp_obj_t *argv)
{
    mp_obj_t self_in = argv[0];
    mp_obj_t size_in = argc > 1? argv[1]: mp_const_none;
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);
    size_t size = 0;
    if (size_in == mp_const_none){
        const mp_obj_type_t *type = self->base.type;
        size = get_lv_struct_size(type);
    } else {
        size = (size_t)mp_obj_get_int(size_in);
    }
    if (size == 0) return mp_const_none;
    mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE,
        size, self->data));
    view->typecode |= 0x80; // used to indicate writable buffer
    return MP_OBJ_FROM_PTR(view);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_lv_dereference_obj, 1, 2, mp_lv_dereference);

// Callback function handling
// Callback is either a callable object or a pointer. If it's a callable object, set user_data to the callback.
// Multiple callbacks are kept per object/struct using a dict that associate callback name with callback object
// In case of an lv_obj_t, user_data is mp_lv_obj_t which contains a member "callbacks" for that dict.
// In case of a struct, user_data is a pointer to that dict directly

static mp_obj_t get_callback_dict_from_user_data(void *user_data)
{
    if (user_data){
        mp_obj_t obj = MP_OBJ_FROM_PTR(user_data);
#ifdef LV_OBJ_T
        return
            MP_OBJ_IS_TYPE(obj, &mp_type_dict)? obj: // Handle the case of dict for a struct
            mp_get_callbacks(obj); // Handle the case of mp_lv_obj_t for an lv_obj_t
#else
        return obj;
#endif
    }
    return NULL;
}

typedef void *(*mp_lv_get_user_data)(void *);
typedef void (*mp_lv_set_user_data)(void *, void *);

static void *mp_lv_callback(mp_obj_t mp_callback, void *lv_callback, qstr callback_name,
     void **user_data_ptr, void *containing_struct, mp_lv_get_user_data get_user_data, mp_lv_set_user_data set_user_data)
{
    if (lv_callback && mp_obj_is_callable(mp_callback)) {
        void *user_data = NULL;
        if (user_data_ptr) {
            // user_data is either a dict of callbacks in case of struct, or a pointer to mp_lv_obj_t in case of lv_obj_t
            if (! (*user_data_ptr) ) *user_data_ptr = MP_OBJ_TO_PTR(mp_obj_new_dict(0)); // if it's NULL - it's a dict for a struct
            user_data = *user_data_ptr;
        }
        else if (get_user_data && set_user_data) {
            user_data = get_user_data(containing_struct);
            if (!user_data) {
                user_data = MP_OBJ_TO_PTR(mp_obj_new_dict(0));
                set_user_data(containing_struct, user_data);
            }
        }

        if (user_data) {
            mp_obj_t callbacks = get_callback_dict_from_user_data(user_data);
            mp_obj_dict_store(callbacks, MP_OBJ_NEW_QSTR(callback_name), mp_callback);
        }
        return lv_callback;
    } else {
        return mp_to_ptr(mp_callback);
    }
}

static int _nesting = 0;

// Function pointers wrapper

static mp_obj_t mp_lv_funcptr(const mp_lv_obj_fun_builtin_var_t *mp_fun, void *lv_fun, void *lv_callback, qstr func_name, void *user_data)
{
    if (lv_fun == NULL)
        return mp_const_none;
    if (lv_fun == lv_callback) {
        mp_obj_t callbacks = get_callback_dict_from_user_data(user_data);
        if (callbacks)
            return mp_obj_dict_get(callbacks, MP_OBJ_NEW_QSTR(func_name));
    }
    mp_lv_obj_fun_builtin_var_t *funcptr = m_new_obj(mp_lv_obj_fun_builtin_var_t);
    *funcptr = *mp_fun;
    funcptr->lv_fun = lv_fun;
    return MP_OBJ_FROM_PTR(funcptr);
}

// Missing implementation for 64bit integer conversion

static unsigned long long mp_obj_get_ull(mp_obj_t obj)
{
    if (mp_obj_is_small_int(obj))
        return MP_OBJ_SMALL_INT_VALUE(obj);

    unsigned long long val = 0;
    bool big_endian = !(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
    mp_obj_int_to_bytes_impl(obj, big_endian, sizeof(val), (byte*)&val);
    return val;
}


// Array of natives

typedef struct mp_lv_array_t
{
    mp_lv_struct_t base;
    size_t element_size;
    bool is_signed;
} mp_lv_array_t;

static void mp_lv_array_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{
    mp_lv_array_t *self = MP_OBJ_TO_PTR(self_in);
    size_t element_size = self->element_size;
    bool is_signed = self->is_signed;
    mp_printf(print, "C Array (%sint%d[])", is_signed? "": "u", element_size*8);
}

static mp_obj_t lv_array_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    mp_lv_array_t *self = MP_OBJ_TO_PTR(self_in);

    if ((!self) || (!self->base.data))
        return NULL;
    if (!mp_obj_is_int(index)) {
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Subscript index must be an integer!")));
    }

    size_t element_size = self->element_size;
    size_t element_index = mp_obj_get_int(index);
    void *element_addr = (byte*)self->base.data + element_size*element_index;
    bool is_signed = self->is_signed;
    union {
        long long val;
        unsigned long long uval;
    } element;
    memset(&element, 0, sizeof(element));

    if (value == MP_OBJ_NULL){
        memset(element_addr, 0, element_size);
    }
    else if (value == MP_OBJ_SENTINEL){
        memcpy(&element, element_addr, element_size);
        return is_signed? mp_obj_new_int_from_ll(element.val): mp_obj_new_int_from_ull(element.uval);
    } else {
        if (!mp_obj_is_int(value)) {
            nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Value '%s' must be an integer!"), mp_obj_get_type_str(value)));
        }
        element.uval = mp_obj_get_ull(value);
        memcpy(element_addr, &element, element_size);
    }

    return self_in;
}

static const mp_rom_map_elem_t mp_base_struct_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___cast__), MP_ROM_PTR(&mp_lv_cast_class_method) },
    { MP_ROM_QSTR(MP_QSTR___cast_instance__), MP_ROM_PTR(&mp_lv_cast_instance_obj) },
    { MP_ROM_QSTR(MP_QSTR___dereference__), MP_ROM_PTR(&mp_lv_dereference_obj) },
};

static MP_DEFINE_CONST_DICT(mp_base_struct_locals_dict, mp_base_struct_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_base_struct_type,
    MP_QSTR_Struct,
    MP_TYPE_FLAG_NONE,
    binary_op, lv_struct_binary_op,
    subscr, lv_struct_subscr,
    buffer, mp_blob_get_buffer,
    locals_dict, &mp_base_struct_locals_dict
);

// TODO: provide constructor
static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_array_type,
    MP_QSTR_C_Array,
    MP_TYPE_FLAG_NONE,
    print, mp_lv_array_print,
    binary_op, lv_struct_binary_op,
    subscr, lv_array_subscr,
    buffer, mp_blob_get_buffer,
    locals_dict, &mp_base_struct_locals_dict
);

GENMPY_UNUSED static mp_obj_t mp_array_from_ptr(void *lv_arr, size_t element_size, bool is_signed)
{
    mp_lv_array_t *self = m_new_obj(mp_lv_array_t);
    *self = (mp_lv_array_t){
        { {&mp_lv_array_type}, lv_arr },
        element_size,
        is_signed
    };
    return MP_OBJ_FROM_PTR(self);
}

GENMPY_UNUSED static void *mp_array_to_ptr(mp_obj_t *mp_arr, size_t element_size, GENMPY_UNUSED bool is_signed)
{
    if (MP_OBJ_IS_STR_OR_BYTES(mp_arr) ||
        MP_OBJ_IS_TYPE(mp_arr, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(mp_arr, &mp_type_memoryview)){
            return mp_to_ptr(mp_arr);
    }

    mp_obj_t mp_len = mp_obj_len_maybe(mp_arr);
    if (mp_len == MP_OBJ_NULL) return mp_to_ptr(mp_arr);
    mp_int_t len = mp_obj_get_int(mp_len);
    void *lv_arr = m_malloc(len * element_size);
    byte *element_addr = (byte*)lv_arr;
    mp_obj_t iter = mp_getiter(mp_arr, NULL);
    mp_obj_t item;
    while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
        union {
            long long val;
            unsigned long long uval;
        } element;
        if (!mp_obj_is_int(item)) {
            nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Value '%s' must be an integer!"), mp_obj_get_type_str(item)));
        }
        element.uval = mp_obj_get_ull(item);
        memcpy(element_addr, &element, element_size);
        element_addr += element_size;
    }
    return lv_arr;
}

#define MP_ARRAY_CONVERTOR(name, size, is_signed) \
GENMPY_UNUSED static mp_obj_t mp_array_from_ ## name(void *lv_arr)\
{\
    return mp_array_from_ptr(lv_arr, size, is_signed);\
}\
GENMPY_UNUSED static void *mp_array_to_ ## name(mp_obj_t mp_arr)\
{\
    return mp_array_to_ptr(mp_arr, size, is_signed);\
}

MP_ARRAY_CONVERTOR(u8ptr, 1, false)
MP_ARRAY_CONVERTOR(i8ptr, 1, true)
MP_ARRAY_CONVERTOR(u16ptr, 2, false)
MP_ARRAY_CONVERTOR(i16ptr, 2, true)
MP_ARRAY_CONVERTOR(u32ptr, 4, false)
MP_ARRAY_CONVERTOR(i32ptr, 4, true)
MP_ARRAY_CONVERTOR(u64ptr, 8, false)
MP_ARRAY_CONVERTOR(i64ptr, 8, true)

""")

#
# Add regular enums with integer values
#
enums = collections.OrderedDict()
for enum_def in enum_defs:
    # Skip stdatomic.h memory_order, no bindings needed.
    if isinstance(enum_def, c_ast.TypeDecl) and enum_def.declname == 'memory_order':
        continue

    # eprint("--> %s" % enum_def)
    while hasattr(enum_def.type, 'name') and not enum_def.type.values:
        enum_def  = next(e for e in enum_defs if hasattr(e.type, 'name') and e.type.name == enum_def.type.name and e.type.values)
    member_names = [member.name for member in enum_def.type.values.enumerators if not member.name.startswith('_')]
    enum_name = os.path.commonprefix(member_names)
    enum_name = "_".join(enum_name.split("_")[:-1]) # remove suffix
    enum = collections.OrderedDict()
    for member in enum_def.type.values.enumerators:
        if member.name.startswith('_'):
            continue
        member_name = member.name[len(enum_name)+1:] if len(enum_name) > 0 else member.name
        if member_name[0].isdigit():
            member_name = '_' + member_name
        if len(enum_name) > 0 and get_enum_name(enum_name) != 'ENUM':
            enum[member_name] = 'MP_ROM_INT(%s)' % member.name
        else:
            int_constants.append(member.name)
    if len(enum) > 0:
        if len(get_enum_name(enum_name)) > 0:
            prev_enum = enums.get(enum_name)
            if prev_enum:
                prev_enum.update(enum)
            else:
                enums[enum_name] = enum


for enum in [enum for enum in enums if len(enums[enum]) == 1 and enum.startswith('ENUM')]:
    int_constants.append('%s_%s' % (enum, next(iter(enums[enum]))))
    enum_name = '%s_%s' % (enum,  list(enums[enum].keys())[0])
    constant_metadata[enum_name.replace('ENUM_', '').replace('LV_', '')] = {'py_type': 'int', 'c_type': enum_name.replace('ENUM_', '')}
    del enums[enum]


# Add special string enums
print ('''
/*
 * LVGL string constants
 */
''')

for enum_def in enum_defs:
    if not enum_def.type.values:
        continue
    member_names = [str_enum_to_str(member.name) for member in enum_def.type.values.enumerators if lv_str_enum_pattern.match(member.name)]
    enum_name = os.path.commonprefix(member_names)
    enum_name = "_".join(enum_name.split("_")[:-1]) # remove suffix
    enum = collections.OrderedDict()
    if enum_name:
        for member in enum_def.type.values.enumerators:
            full_name = str_enum_to_str(member.name)
            member_name = full_name[len(enum_name)+1:]
            print('MP_DEFINE_STR_OBJ(mp_%s, %s);' % (full_name, full_name))
            enum[member_name] = '&mp_%s' % full_name
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

def decl_to_callback(decl):
    # print('/* decl_to_callback %s */' % decl)
    if not hasattr(decl, 'type'):
        return None
    if (isinstance(decl.type, c_ast.PtrDecl) and isinstance(decl.type.type, c_ast.FuncDecl)):
        return (decl.name, decl.type.type)
        # print('/* callback: ADDED CALLBACK: %s\n%s */' % (gen.visit(decl.type.type), decl.type.type))
    elif isinstance(decl.type, c_ast.FuncDecl):
        return (decl.name, decl.type)
        # print('/* callback: ADDED CALLBACK: %s\n%s */' % (gen.visit(decl.type.type), decl.type.type))
    elif isinstance(decl.type, c_ast.TypeDecl) and hasattr(decl.type.type,'names'):
        func_typedef_name = decl.type.type.names[0]
        while func_typedef_name in synonym:
            # eprint('decl_to_callback: %s --> %s' % (func_typedef_name, synonym[func_typedef_name]))
            func_typedef_name = synonym[func_typedef_name]
        # print('/* --> callback: TYPEDEF CALLBACK: %s: %s */' % (decl.name if hasattr(decl, 'name') else None, func_typedef_name))
        if func_typedef_name in func_typedefs:
            return (decl.name, func_typedefs[func_typedef_name].type.type)
            # print('/* callback: ADDED CALLBACK: %s\n%s */' % (func_typedef_name, func_typedefs[func_typedef_name]))
    else: return None


def get_user_data_accessors(containing_struct, containing_struct_name = None):
    if not containing_struct_name and containing_struct and containing_struct.name:
        containing_struct_name = containing_struct.name
    if not containing_struct_name:
        return None, None
    base_struct_name = get_base_struct_name(containing_struct_name)
    getter_name = base_struct_name + '_get_user_data'
    setter_name = base_struct_name + '_set_user_data'
    # print('/* struct functions = %s */' % [s.name + ':' + str(len(s.type.args.params)) for s in get_struct_functions(containing_struct_name)])
    # print('/* getter_name = %s */' % getter_name)
    struct_functions = get_struct_functions(containing_struct_name)
    getters = [s for s in struct_functions if s.name == getter_name and len(s.type.args.params) == 1]
    setters = [s for s in struct_functions if s.name == setter_name and len(s.type.args.params) == 2]
    if getters and setters:
        return getters[0], setters[0]
    else:
        return None, None


def get_user_data(func, func_name = None, containing_struct = None, containing_struct_name = None):
    args = func.args.params
    if not func_name: func_name = get_arg_name(func.type)
    # print('/* --> callback: func_name = %s, args = %s */' % (func_name, repr(args)))
    user_data_found = False
    user_data = 'None'
    if len(args) > 0 and isinstance(args[0].type, c_ast.PtrDecl):
        # if isinstance(args[0].type.type.type, c_ast.Struct):
        #     struct_arg_type_name = args[0].type.type.type.name # PtrDecl.TypeDecl.Struct. Needed to omit 'struct' keyword.
        # else:
        #     struct_arg_type_name = get_type(args[0].type.type, remove_quals = True)
        struct_arg_type_name = get_type(args[0].type.type, remove_quals = True)
        # print('/* --> get_user_data: containing_struct_name = %s, struct_arg_type_name = %s */' % (containing_struct_name, struct_arg_type_name))
        if containing_struct_name and struct_arg_type_name != containing_struct_name:
            return None, None, None
        if not containing_struct:
            try_generate_type(args[0].type)
            if struct_arg_type_name in structs:
                containing_struct = structs[struct_arg_type_name]
                # print('/* --> containing_struct = %s */' % containing_struct)
            # if struct_arg_type_name in mp_to_lv:
            #     print('/* --> callback: %s First argument is %s */' % (gen.visit(func), struct_arg_type_name))
        if containing_struct:
            flatten_struct_decls = flatten_struct(containing_struct.decls)
            user_data = 'user_data'
            user_data_found = user_data in [decl.name for decl in flatten_struct_decls]
            # print('/* --> callback: user_data=%s user_data_found=%s containing_struct=%s */' % (user_data, user_data_found, containing_struct))
            if not user_data_found and lvgl_json is not None:
                containing_struct_j = next((struct for struct in lvgl_json["structures"] if struct["name"] == struct_arg_type_name), None)
                if (containing_struct_j is None
                    and struct_arg_type_name.startswith("lv_")
                    and None is not next((fwd_decl for fwd_decl in lvgl_json["forward_decls"] if fwd_decl["name"] == struct_arg_type_name), None)
                ):
                    struct_arg_type_name_with_underscore = "_" + struct_arg_type_name
                    containing_struct_j = next((struct for struct in lvgl_json["structures"] if struct["name"] == struct_arg_type_name_with_underscore), None)
                if containing_struct_j is not None:
                    user_data_found = any(user_data == field["name"] for field in containing_struct_j["fields"])
    return (user_data if user_data_found else None), *get_user_data_accessors(containing_struct, containing_struct_name)


#
# Generate structs when needed
#
generated_structs = collections.OrderedDict()
generated_struct_functions = collections.OrderedDict()
struct_aliases = collections.OrderedDict()
callbacks_used_on_structs = []


def flatten_struct(struct_decls):
    result = []
    if not struct_decls: return result
    for decl in struct_decls:
        if is_struct(decl.type):
            result.extend(flatten_struct(decl.type.decls))
        else:
            result.append(decl)
    return result


def try_generate_struct(struct_name, struct):
    global lv_to_mp
    global mp_to_lv
    if struct_name in generated_structs: return None
    sanitized_struct_name = sanitize(struct_name)
    generated_structs[struct_name] = False # Starting generating a struct
    # print("/* Starting generating %s */" % struct_name)
    if struct_name in mp_to_lv:
        return mp_to_lv[struct_name]
    # print('/* --> try_generate_struct %s: %s\n%s */' % (struct_name, gen.visit(struct), struct))
    if struct.decls is None:
        if struct_name == struct.name:
            return None
        if struct.name not in structs:
            return None
        return try_generate_type(structs[struct.name])
    flatten_struct_decls = flatten_struct(struct.decls)
    # Go over fields and try to generate type convertors for each
    # print('!! %s' % struct)
    # print('!!! %s' % flatten_struct_decls)
    write_cases = []
    read_cases = []

    struct_metadata[get_py_type(struct_name).replace('"', '')] = {
        'class_attributes': collections.OrderedDict(),
        'attributes': collections.OrderedDict(),
        'py_type': 'class',
        'c_type': struct_name,
        'methods': collections.OrderedDict()
    }

    attribute_meta = struct_metadata[get_py_type(struct_name).replace('"', '')]['attributes']

    for decl in flatten_struct_decls:
        # print('/* ==> decl %s: %s */' % (gen.visit(decl), decl))
        converted = try_generate_type(decl.type)
        type_name = get_type(decl.type, remove_quals = True)
        # print('/* --> %s: %s (%s)*/' % (decl.name, type_name, mp_to_lv[type_name] if type_name in mp_to_lv else '---'))
        # Handle the case of nested struct
        if not converted and is_struct(decl.type.type):
            parent_name = struct_name
            child_name = decl.type.declname
            type_name = '%s_%s_t' % (parent_name[:-2], child_name)
            print('typedef __typeof__( (({parent}*)(0))->{child} ) {new_struct};'.format(
                parent = parent_name, child = child_name, new_struct = type_name))
            try_generate_struct(type_name, decl.type.type)

        # print('==> %s %s: %s' % (type_name, str(type_name in mp_to_lv), decl))

        if (type_name not in mp_to_lv or not mp_to_lv[type_name]) or (type_name not in lv_to_mp or not lv_to_mp[type_name]):
            # eprint("[%s] %s or %s : %s" % (isinstance(decl.type,c_ast.PtrDecl), type_name, get_type(decl.type), decl.type))
            if type_name in generated_structs:
                print("/* Already started generating %s! skipping field '%s' */" % (type_name, decl.name))
                continue
            raise MissingConversionException('Missing conversion to %s when generating struct %s.%s' % (type_name, struct_name, get_name(decl)))

        mp_to_lv_convertor = mp_to_lv[type_name]
        lv_to_mp_convertor = lv_to_mp_byref[type_name] if type_name in lv_to_mp_byref else lv_to_mp[type_name]

        cast = '(void*)' if isinstance(decl.type, c_ast.PtrDecl) else '' # needed when field is const. casting to void overrides it

        callback = decl_to_callback(decl)

        if callback:
            # print("/* %s callback %s */" % (gen.visit(decl), callback))
            func_name, arg_type = callback
            user_data, _, _ = get_user_data(arg_type, func_name = func_name, containing_struct = struct, containing_struct_name = struct_name)
            if not callback in callbacks_used_on_structs:
                callbacks_used_on_structs.append(callback + (struct_name,))
            # Emit callback forward decl.
            if user_data in [user_data_decl.name for user_data_decl in flatten_struct_decls]:
                full_user_data = 'data->%s' % user_data
                full_user_data_ptr = '&%s' % full_user_data
                lv_callback = '%s_%s_callback' % (struct_name, func_name)
                print('static %s %s_%s_callback(%s);' % (get_type(arg_type.type, remove_quals = False), struct_name, func_name, gen.visit(arg_type.args)))
            else:
                full_user_data = 'NULL'
                full_user_data_ptr = full_user_data
                lv_callback = 'NULL'
                if not user_data:
                    gen_func_error(decl, "Missing 'user_data' as a field of the first parameter of the callback function '%s_%s_callback'" % (struct_name, func_name))
                else:
                    gen_func_error(decl, "Missing 'user_data' member in struct '%s'" % struct_name)
            write_cases.append('case MP_QSTR_{field}: data->{decl_name} = {cast}mp_lv_callback(dest[1], {lv_callback} ,MP_QSTR_{struct_name}_{field}, {user_data}, NULL, NULL, NULL); break; // converting to callback {type_name}'.
                format(struct_name = struct_name, field = sanitize(decl.name), decl_name = decl.name, lv_callback = lv_callback, user_data = full_user_data_ptr, type_name = type_name, cast = cast))
            read_cases.append('case MP_QSTR_{field}: dest[0] = mp_lv_funcptr(&mp_{funcptr}_mpobj, {cast}data->{decl_name}, {lv_callback} ,MP_QSTR_{struct_name}_{field}, {user_data}); break; // converting from callback {type_name}'.
                format(struct_name = struct_name, field = sanitize(decl.name), decl_name = decl.name, lv_callback = lv_callback, funcptr = lv_to_mp_funcptr[type_name], user_data = full_user_data, type_name = type_name, cast = cast))
        else:
            user_data = None
            # Only allow write to non-const members
            is_writeable = (not hasattr(decl.type, 'quals')) or 'const' not in decl.type.quals
            # Arrays must be handled by memcpy, otherwise we would get "assignment to expression with array type" error

            attribute_meta[sanitize(decl.name)] = {'py_type': get_py_type(type_name), 'c_type': type_name, 'is_writeable': is_writeable, 'is_readable': True}

            if isinstance(decl.type, c_ast.ArrayDecl):
                memcpy_size = 'sizeof(%s)*%s' % (gen.visit(decl.type.type), gen.visit(decl.type.dim))
                if is_writeable:
                    write_cases.append('case MP_QSTR_{field}: memcpy((void*)&data->{decl_name}, {cast}{convertor}(dest[1]), {size}); break; // converting to {type_name}'.
                        format(field = sanitize(decl.name), decl_name = decl.name, convertor = mp_to_lv_convertor, type_name = type_name, cast = cast, size = memcpy_size))
                read_cases.append('case MP_QSTR_{field}: dest[0] = {convertor}({cast}data->{decl_name}); break; // converting from {type_name}'.
                    format(field = sanitize(decl.name), decl_name = decl.name, convertor = lv_to_mp_convertor, type_name = type_name, cast = cast))
            else:
                if is_writeable:
                    write_cases.append('case MP_QSTR_{field}: data->{decl_name} = {cast}{convertor}(dest[1]); break; // converting to {type_name}'.
                        format(field = sanitize(decl.name), decl_name = decl.name, convertor = mp_to_lv_convertor, type_name = type_name, cast = cast))
                read_cases.append('case MP_QSTR_{field}: dest[0] = {convertor}({cast}data->{decl_name}); break; // converting from {type_name}'.
                    format(field = sanitize(decl.name), decl_name = decl.name, convertor = lv_to_mp_convertor, type_name = type_name, cast = cast))
    print('''
/*
 * Struct {struct_name}
 */

static const mp_obj_type_t mp_{sanitized_struct_name}_type;

static void* mp_write_ptr_{sanitized_struct_name}(mp_obj_t self_in)
{{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, &mp_{sanitized_struct_name}_type));
    return ({struct_tag}{struct_name}*)self->data;
}}

#define mp_write_{sanitized_struct_name}(struct_obj) *(({struct_tag}{struct_name}*)mp_write_ptr_{sanitized_struct_name}(struct_obj))

static mp_obj_t mp_read_ptr_{sanitized_struct_name}(void *field)
{{
    return lv_to_mp_struct(&mp_{sanitized_struct_name}_type, field);
}}

#define mp_read_{sanitized_struct_name}(field) mp_read_ptr_{sanitized_struct_name}(copy_buffer(&field, sizeof({struct_tag}{struct_name})))
#define mp_read_byref_{sanitized_struct_name}(field) mp_read_ptr_{sanitized_struct_name}(&field)

static void mp_{sanitized_struct_name}_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);
    GENMPY_UNUSED {struct_tag}{struct_name} *data = ({struct_tag}{struct_name}*)self->data;

    if (dest[0] == MP_OBJ_NULL) {{
        // load attribute
        switch(attr)
        {{
            {read_cases};
            default: call_parent_methods(self_in, attr, dest); // fallback to locals_dict lookup
        }}
    }} else {{
        if (dest[1])
        {{
            // store attribute
            switch(attr)
            {{
                {write_cases};
                default: return;
            }}

            dest[0] = MP_OBJ_NULL; // indicate success
        }}
    }}
}}

static const mp_obj_dict_t mp_{sanitized_struct_name}_locals_dict;

static MP_DEFINE_CONST_OBJ_TYPE(
    mp_{sanitized_struct_name}_type,
    MP_QSTR_{sanitized_struct_name},
    MP_TYPE_FLAG_NONE,
    make_new, make_new_lv_struct,
    binary_op, lv_struct_binary_op,
    subscr, lv_struct_subscr,
    attr, mp_{sanitized_struct_name}_attr,
    locals_dict, &mp_{sanitized_struct_name}_locals_dict,
    buffer, mp_blob_get_buffer,
    parent, &mp_lv_base_struct_type
);
'''.format(
            sanitized_struct_name = sanitized_struct_name,
            struct_name = struct_name,
            struct_tag = 'struct ' if struct_name in structs_without_typedef.keys() else '',
            write_cases = ';\n                '.join(write_cases),
            read_cases  = ';\n            '.join(read_cases),
            ))

    lv_to_mp[struct_name] = 'mp_read_%s' % sanitized_struct_name
    lv_to_mp_byref[struct_name] = 'mp_read_byref_%s' % sanitized_struct_name
    mp_to_lv[struct_name] = 'mp_write_%s' % sanitized_struct_name
    lv_to_mp['%s *' % struct_name] = 'mp_read_ptr_%s' % sanitized_struct_name
    mp_to_lv['%s *' % struct_name] = 'mp_write_ptr_%s' % sanitized_struct_name
    lv_to_mp['const %s *' % struct_name] = 'mp_read_ptr_%s' % sanitized_struct_name
    mp_to_lv['const %s *' % struct_name] = 'mp_write_ptr_%s' % sanitized_struct_name
    lv_mp_type[struct_name] = simplify_identifier(sanitized_struct_name)
    lv_mp_type['%s *' % struct_name] = simplify_identifier(sanitized_struct_name)
    lv_mp_type['const %s *' % struct_name] = simplify_identifier(sanitized_struct_name)
    # print('/* --> struct "%s" generated! */' % (struct_name))
    generated_structs[struct_name] = True # Completed generating a struct
    return struct_name


#
# Generate Array Types when needed
#
def try_generate_array_type(type_ast):
    arr_name = get_name(type_ast)
    if arr_name in mp_to_lv:
        return mp_to_lv[arr_name]
    # print('/* --> try_generate_array_type %s: %s */' % (arr_name, type_ast))
    dim = gen.visit(type_ast.dim) if hasattr(type_ast, 'dim') and type_ast.dim else None
    element_type = get_type(type_ast.type, remove_quals = True)
    qualified_element_type = gen.visit(type_ast.type)
    if element_type not in mp_to_lv or not mp_to_lv[element_type]:
        try_generate_type(type_ast.type)
        if element_type not in mp_to_lv or not mp_to_lv[element_type]:
            raise MissingConversionException('Missing conversion to %s while generating array type conversion' % element_type)
    type_ptr_ast = c_ast.PtrDecl(
        quals=[],
        type=type_ast.type)
    element_type_ptr = get_type(type_ptr_ast, remove_quals = True)
    qualified_element_ptr_type = gen.visit(type_ptr_ast)
    if element_type_ptr not in mp_to_lv or not mp_to_lv[element_type_ptr]:
        try_generate_type(type_ptr_ast)
        if element_type_ptr not in mp_to_lv or not mp_to_lv[element_type_ptr]:
            raise MissingConversionException('Missing conversion to %s while generating array type conversion' % element_type_ptr)
    array_convertor_suffix = arr_name.\
        replace(' ','_').\
        replace('*','ptr').\
        replace('+','plus').\
        replace('-','minus').\
        replace('[','__').\
        replace(']','__').\
        replace('(','__').\
        replace(')','__').\
        replace('/','_div_')
    arr_to_c_convertor_name = 'mp_arr_to_%s' % array_convertor_suffix
    arr_to_mp_convertor_name = 'mp_arr_from_%s' % array_convertor_suffix
    print((('''
/*
 * Array convertors for {arr_name}
 */

GENMPY_UNUSED static {struct_tag}{type} *{arr_to_c_convertor_name}(mp_obj_t mp_arr)
{{
    mp_obj_t mp_len = mp_obj_len_maybe(mp_arr);
    if (mp_len == MP_OBJ_NULL) return mp_to_ptr(mp_arr);
    mp_int_t len = mp_obj_get_int(mp_len);
    {check_dim}
    {struct_tag}{type} *lv_arr = ({struct_tag}{type}*)m_malloc(len * sizeof({struct_tag}{type}));
    mp_obj_t iter = mp_getiter(mp_arr, NULL);
    mp_obj_t item;
    size_t i = 0;
    while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {{
        lv_arr[i++] = {mp_to_lv_convertor}(item);
    }}
    return ({struct_tag}{type} *)lv_arr;
}}
''') + ('''
GENMPY_UNUSED static mp_obj_t {arr_to_mp_convertor_name}({qualified_type} *arr)
{{
    mp_obj_t obj_arr[{dim}];
    for (size_t i=0; i<{dim}; i++){{
        obj_arr[i] = {lv_to_mp_convertor}(arr[i]);
    }}
    return mp_obj_new_list({dim}, obj_arr); // TODO: return custom iterable object!
}}
''' if dim else '''
GENMPY_UNUSED static mp_obj_t {arr_to_mp_convertor_name}({qualified_type} *arr)
{{
    return {lv_to_mp_ptr_convertor}((void*)arr);
}}
''')).format(
        arr_to_c_convertor_name = arr_to_c_convertor_name,
        arr_to_mp_convertor_name = arr_to_mp_convertor_name ,
        arr_name = arr_name,
        type = element_type,
        type_ptr = element_type_ptr,
        struct_tag = 'struct ' if element_type in structs_without_typedef.keys() else '',
        qualified_type = qualified_element_type,
        qualified_ptr_type = qualified_element_ptr_type,
        check_dim = '//TODO check dim!' if dim else '',
        mp_to_lv_convertor = mp_to_lv[element_type],
        lv_to_mp_convertor = lv_to_mp[element_type],
        mp_to_lv_ptr_convertor = mp_to_lv[element_type_ptr],
        lv_to_mp_ptr_convertor = lv_to_mp[element_type_ptr],
        dim = dim if dim else 1,
        ))
    mp_to_lv[arr_name] = arr_to_c_convertor_name
    mp_to_lv['const %s' % arr_name] = arr_to_c_convertor_name
    lv_to_mp[arr_name] = arr_to_mp_convertor_name
    lv_to_mp['const %s' % arr_name] = arr_to_mp_convertor_name
    lv_mp_type[arr_name] = arr_to_c_convertor_name
    lv_mp_type['const %s' % arr_name] = 'const %s' % arr_to_c_convertor_name

    new_array_name = get_py_type(arr_name)
    json_c_to_py_types[arr_to_c_convertor_name] = new_array_name
    return arr_to_c_convertor_name


#
# Generate types from typedefs when needed
#
def get_arg_name(arg):
    if isinstance(arg, c_ast.PtrDecl) or isinstance(arg, c_ast.FuncDecl):
        return get_arg_name(arg.type)
    if hasattr(arg, 'declname'): return arg.declname
    if hasattr(arg, 'name'): return name
    return 'unnamed_arg'


# print("// Typedefs: " + ", ".join(get_arg_name(t) for t in typedefs))


def try_generate_type(type_ast):
    # eprint(' --> try_generate_type %s : %s' % (get_name(type_ast), gen.visit(type_ast)))
    # print('/* --> try_generate_type %s: %s */' % (get_name(type_ast), type_ast))
    if isinstance(type_ast, str): raise SyntaxError('Internal error! try_generate_type argument is a string.')
    # Handle the case of a pointer
    if isinstance(type_ast, c_ast.TypeDecl):
        return try_generate_type(type_ast.type)
    type = get_name(type_ast)
    if isinstance(type_ast, c_ast.Enum):
        mp_to_lv[type] = mp_to_lv['int']
        mp_to_lv['%s *' % type] = mp_to_lv['int *']
        lv_to_mp[type] = lv_to_mp['int']
        lv_to_mp['%s *' % type] = lv_to_mp['int *']
        lv_mp_type[type] = lv_mp_type['int']
        lv_mp_type['%s *' % type] = lv_mp_type['int *']
        return mp_to_lv[type]
    if type in mp_to_lv:
        return mp_to_lv[type]
    if isinstance(type_ast, c_ast.ArrayDecl) and try_generate_array_type(type_ast):
        return mp_to_lv[type]
    if isinstance(type_ast, (c_ast.PtrDecl, c_ast.ArrayDecl)):
        type = get_name(type_ast.type.type)
        ptr_type = get_type(type_ast, remove_quals=True)
        # print('/* --> try_generate_type IS PtrDecl!! %s: %s */' % (type, type_ast))
        if (type in structs):
            try_generate_struct(type, structs[type]) if type in structs else None
        if isinstance(type_ast.type, c_ast.TypeDecl) and isinstance(type_ast.type.type, c_ast.Struct) and (type_ast.type.type.name in structs):
            try_generate_struct(type, structs[type_ast.type.type.name])
        if isinstance(type_ast.type, c_ast.FuncDecl):
            if isinstance(type_ast.type.type.type, c_ast.TypeDecl): type = type_ast.type.type.type.declname
            func_ptr_name = "funcptr_%s" % type

            i = 1
            while func_ptr_name in generated_funcs: # Make sure func_ptr_name is unique
                func_ptr_name = "funcptr_%s_%d" % (type,i)
                i += 1

            func = c_ast.Decl(
                    name=func_ptr_name,
                    quals=[],
                    align=[],
                    storage=[],
                    funcspec=[],
                    type=type_ast.type,
                    init=None,
                    bitsize=None)
            try:
                print("#define %s NULL\n" % func_ptr_name)
                gen_mp_func(func, None)
                print("static mp_obj_t mp_lv_{f}(void *func){{ return mp_lv_funcptr(&mp_{f}_mpobj, func, NULL, MP_QSTR_, NULL); }}\n".format(
                    f=func_ptr_name))
                lv_to_mp_funcptr[ptr_type] = func_ptr_name
                # eprint("/* --> lv_to_mp_funcptr[%s] = %s */" % (ptr_type, func_ptr_name))
                lv_to_mp[ptr_type] = "mp_lv_%s" % func_ptr_name
                lv_mp_type[ptr_type] = 'function pointer'
            except MissingConversionException as exp:
                gen_func_error(func, exp)
        # print('/* --> PTR %s */' % ptr_type)
        if not ptr_type in mp_to_lv: mp_to_lv[ptr_type] = mp_to_lv['void *']
        if not ptr_type in lv_to_mp: lv_to_mp[ptr_type] = lv_to_mp['void *']
        if not ptr_type in lv_mp_type: lv_mp_type[ptr_type] = 'void*'
        return mp_to_lv[ptr_type]
    if type in structs:
        if try_generate_struct(type, structs[type]):
            return mp_to_lv[type]
    for new_type_ast in [x for x in typedefs if get_arg_name(x) == type]:
        new_type = get_type(new_type_ast, remove_quals=True)
        if isinstance(new_type_ast, c_ast.TypeDecl) and isinstance(new_type_ast.type, c_ast.Struct) and not new_type_ast.type.decls:
            explicit_struct_name = new_type_ast.type.name if hasattr(new_type_ast.type, 'name') else new_type_ast.type.names[0]
        else:
            explicit_struct_name = new_type
        if type == explicit_struct_name:
            continue
        # eprint('/* --> typedef: %s --> %s (%s) */' % (type, new_type, new_type_ast))
        if explicit_struct_name in structs:
            if (try_generate_struct(new_type, structs[explicit_struct_name])):
                if explicit_struct_name == new_type:
                    struct_aliases[new_type] = type
        if type != new_type and try_generate_type(new_type_ast):
           # eprint('/* --> try_generate_type TYPEDEF!! %s: %s */' % (type, mp_to_lv[new_type]))
           mp_to_lv[type] = mp_to_lv[new_type]
           type_ptr = '%s *' % type
           new_type_ptr = '%s *' % new_type
           if new_type_ptr in mp_to_lv:
               mp_to_lv[type_ptr] = mp_to_lv[new_type_ptr]
           if new_type in lv_to_mp:
               lv_to_mp[type] = lv_to_mp[new_type]
               lv_mp_type[type] = lv_mp_type[new_type]
               if new_type in lv_to_mp_funcptr:
                   lv_to_mp_funcptr[type] = lv_to_mp_funcptr[new_type]
               if new_type in lv_to_mp_byref:
                   lv_to_mp_byref[type] = lv_to_mp_byref[new_type]
               if new_type_ptr in lv_to_mp:
                   lv_to_mp[type_ptr] = lv_to_mp[new_type_ptr]
               if new_type_ptr in lv_mp_type:
                   lv_mp_type[type_ptr] = lv_mp_type[new_type_ptr]
           # eprint('/* --> %s = (%s) */' % (type, new_type))
           return mp_to_lv[type]
    return None


#
# Helper structs
#
def create_helper_struct(struct_str):
    print(struct_str)
    struct_str_ast = parser.parse(struct_str).ext[0].type
    struct_name = get_name(struct_str_ast)
    # print('/* --> %s: %s */' % (struct_name, struct_str_ast.type))
    structs[struct_name] = struct_str_ast.type
    try:
        try_generate_struct(struct_name, struct_str_ast.type)
    except MissingConversionException as exp:
        print ('/* Helper structs NOT generated:\n    %s\n*/' % (repr(exp)))


print('''
/*
 * Helper Structs
 */
        ''')

create_helper_struct('''
typedef union {
    void*           ptr_val;
    const char*     str_val;
    int             int_val;
    unsigned int    uint_val;
} C_Pointer;
''')


#
# Emit C callback functions
#
generated_callbacks = collections.OrderedDict()


def build_callback_func_arg(arg, index, func, func_name = None):
    arg_type = get_type(arg.type, remove_quals = False)
    cast = '(void*)' if isinstance(arg.type, c_ast.PtrDecl) else '' # needed when field is const. casting to void overrides it

    if arg_type == 'char *':
        converter = 'ptr_to_mp'
    else:
        arg_type = get_type(arg.type, remove_quals = True)
        if arg_type not in lv_to_mp or not lv_to_mp[arg_type]:
            try_generate_type(arg.type)
            if arg_type not in lv_to_mp or not lv_to_mp[arg_type]:
                raise MissingConversionException("Callback: Missing conversion to %s" % arg_type)
        converter = lv_to_mp[arg_type]

    arg_metadata = {'c_type': arg_type, 'py_type': get_py_type(arg_type)}

    if arg.name:
        arg_metadata['name'] = arg.name
    else:
        arg_metadata['name'] = None

    callback_metadata[func_name]['args'].append(arg_metadata)

    return 'mp_args[{i}] = {convertor}({cast}arg{i});'.format(
                convertor = converter,
                i = index, cast = cast)


def gen_callback_func(func, func_name = None, user_data_argument = False):
    global mp_to_lv
    if func_name in generated_callbacks:
        return
    # print('/* --> callback: %s */' % (gen.visit(func)))

    args = func.args.params
    enumerated_args = []
    for i, arg in enumerate(args):
        arg_name = f'arg{i}'
        new_arg = c_ast.Decl(
                    name=arg_name,
                    quals=arg.quals,
                    align=[],
                    storage=[],
                    funcspec=[],
                    type=copy.deepcopy(arg.type),
                    init=None,
                    bitsize=None)
        t = new_arg
        while hasattr(t, 'type'):
            if hasattr(t.type, 'declname'):
                t.type.declname = arg_name
            t = t.type
        enumerated_args.append(new_arg)
        # print(f'/* --> {gen.visit(new_arg)} */')
    if not func_name: func_name = get_arg_name(func.type)
    # print('/* --> callback: func_name = %s */' % func_name)

    callback_metadata[func_name] = {
        'py_type': 'Callable',
        'c_type': None,
        'args': []
    }
    if isinstance(func.type, c_ast.TypeDecl) and func.type.declname:
        callback_metadata[func_name]['c_type'] = func.type.declname

    if is_global_callback(func):
        full_user_data = 'MP_STATE_PORT(mp_lv_user_data)'
    else:
        user_data, user_data_getter, _ = get_user_data(func, func_name)

        if user_data_argument and len(args) > 0 and gen.visit(args[-1].type) == 'void *':
            full_user_data = 'arg%d' % (len(args) - 1)
        elif user_data:
            full_user_data = 'arg0->%s' % user_data
            if len(args) < 1 or hasattr(args[0].type.type, 'names') and lv_base_obj_pattern.match(args[0].type.type.names[0]):
                raise MissingConversionException("Callback: First argument of callback function must be lv_obj_t")

        elif user_data_getter:
            full_user_data = '%s(arg0)' % user_data_getter.name
        else:
            full_user_data = None

        # if full_user_data: print('/* --> callback: %s user_data found!! %s */' %(gen.visit(func), full_user_data))
        # else: print('/* --> callback: full_user_data NOT FOUND !! %s */' % (gen.visit(func)))
        if not full_user_data:
            raise MissingConversionException("Callback: user_data NOT FOUND! %s" % (gen.visit(func)))

    return_type = get_type(func.type, remove_quals = False)
    if return_type != 'void' and (return_type not in mp_to_lv or not mp_to_lv[return_type]):
        try_generate_type(func.type)
        if return_type not in mp_to_lv or not mp_to_lv[return_type]:
            raise MissingConversionException("Callback return value: Missing conversion to %s" % return_type)

    callback_metadata[func_name]['c_rtype'] = return_type
    callback_metadata[func_name]['py_rtype'] = get_py_type(return_type)
    print("""
/*
 * Callback function {func_name}
 * {func_prototype}
 */

GENMPY_UNUSED static {return_type} {func_name}_callback({func_args})
{{
    mp_obj_t mp_args[{num_args}];
    {build_args}
    mp_obj_t callbacks = get_callback_dict_from_user_data({user_data});
    _nesting++;
    {return_value_assignment}mp_call_function_n_kw(mp_obj_dict_get(callbacks, MP_OBJ_NEW_QSTR(MP_QSTR_{func_name})) , {num_args}, 0, mp_args);
    _nesting--;
    return{return_value};
}}
""".format(
        func_prototype = gen.visit(func),
        func_name = sanitize(func_name),
        return_type = return_type,
        func_args = ', '.join([(gen.visit(arg)) for arg in enumerated_args]),
        num_args=len(args),
        build_args="\n    ".join([build_callback_func_arg(arg, i, func, func_name=func_name) for i,arg in enumerate(args)]),
        user_data=full_user_data,
        return_value_assignment = '' if return_type == 'void' else 'mp_obj_t callback_result = ',
        return_value='' if return_type == 'void' else ' %s(callback_result)' % mp_to_lv[return_type]))
    generated_callbacks[func_name] = True


#
# Emit Mpy function definitions
#
generated_funcs = collections.OrderedDict()


def build_mp_func_arg(arg, index, func, obj_name):
    if isinstance(arg, c_ast.EllipsisParam):
        raise MissingConversionException("Cannot convert ellipsis param")
    fixed_arg = copy.deepcopy(arg)
    convert_array_to_ptr(fixed_arg)
    if not fixed_arg.name:
        fixed_arg.name = "arg%d" % index
        add_default_declname(fixed_arg, fixed_arg.name)
    # print('/* --> FIXED ARG: %s */' % repr(fixed_arg))
    callback = decl_to_callback(arg)
    args = func.type.args.params if func.type.args else []
    # print('/* --> ARG: %s */' % arg)
    # print('/* --> FIRST ARG: %s */' % first_arg)
    if callback:
        # Callback is supported in two modes:
        # 1) last argument is a void* user_data which is passed to callback
        # 2) first argument is a struct with user_data field, which is passed to callback

        callback_name, arg_type = callback
        # print('/* --> callback %s ARG TYPE: %s */' % (callback_name, arg_type))

        try:
            user_data_argument = False
            full_user_data = None
            user_data_getter = None
            user_data_setter = None
            if len(args) > 0 and gen.visit(args[-1].type) == 'void *' and args[-1].name == 'user_data':
                callback_name = '%s_%s' % (func.name, callback_name)
                full_user_data = '&user_data'
                user_data_argument = True
            else:
                first_arg = args[0]
                struct_name = get_name(first_arg.type.type.type if hasattr(first_arg.type.type,'type') else first_arg.type.type)
                callback_name = '%s_%s' % (struct_name, callback_name)
                user_data, user_data_getter, user_data_setter = get_user_data(arg_type, callback_name)
                if is_global_callback(arg_type):
                    full_user_data = '&MP_STATE_PORT(mp_lv_user_data)'
                else:
                    if user_data:
                        full_user_data = '&%s->%s' % (first_arg.name, user_data)
                    elif user_data_getter and user_data_setter:
                        full_user_data = 'NULL' # uses getter/setter instead
                    if index == 0:
                       raise MissingConversionException("Callback argument '%s' cannot be the first argument! We assume the first argument contains the user_data" % gen.visit(arg))
                    if not full_user_data:
                        raise MissingConversionException("Callback function '%s' must receive a struct pointer with user_data member as its first argument!" % gen.visit(arg))
            # eprint("--> callback_metadata= %s_%s" % (struct_name, callback_name))
            gen_callback_func(arg_type, '%s' % callback_name, user_data_argument)

            if (
                isinstance(arg_type, c_ast.FuncDecl) and
                isinstance(arg_type.type, c_ast.TypeDecl) and
                arg_type.type.declname
            ):
                arg_metadata = {'py_type': 'Callable', 'function': callback_metadata[callback_name], 'c_type': arg_type.type.declname}
            else:
                arg_metadata = {'py_type': 'Callable', 'function': callback_metadata[callback_name], 'c_type': None}

            if arg.name:
                arg_metadata['name'] = arg.name
            else:
                arg_metadata['name'] = None

            return 'void *{arg_name} = mp_lv_callback(mp_args[{i}], &{callback_name}_callback, MP_QSTR_{callback_name}, {full_user_data}, {containing_struct}, (mp_lv_get_user_data){user_data_getter}, (mp_lv_set_user_data){user_data_setter});'.format(
                i = index,
                arg_name = fixed_arg.name,
                callback_name = sanitize(callback_name),
                full_user_data = full_user_data,
                containing_struct = first_arg.name if user_data_getter and user_data_setter else "NULL",
                user_data_getter = user_data_getter.name if user_data_getter else 'NULL',
                user_data_setter = user_data_setter.name if user_data_setter else 'NULL'), arg_metadata
        except MissingConversionException as exp:
            gen_func_error(arg, exp)
            callback_name = 'NULL'
            full_user_data = 'NULL'
    if not hasattr(arg, 'type'):
        raise MissingConversionException("Cannot convert function argument %s" % repr(arg))
    arg_type = get_type(arg.type, remove_quals = True)
    # print('/* --> arg = %s, arg_type = %s */' %(gen.visit(arg), arg_type))
    if arg_type not in mp_to_lv or not mp_to_lv[arg_type]:
        try_generate_type(arg.type)
        if arg_type not in mp_to_lv or not mp_to_lv[arg_type]:
            raise MissingConversionException('Missing conversion to %s' % arg_type)

    arg_metadata = {'py_type': get_py_type(arg_type), 'c_type': arg_type}

    if arg.name:
        arg_metadata['name'] = arg.name
    else:
        arg_metadata['name'] = None

    cast = ("(%s)" % gen.visit(fixed_arg.type)) if 'const' in arg.quals else "" # allow conversion from non const to const, sometimes requires cast
    return '{var} = {cast}{convertor}(mp_args[{i}]);'.format(
            var = gen.visit(fixed_arg),
            cast = cast,
            convertor = mp_to_lv[arg_type],
            i = index), arg_metadata


def emit_func_obj(func_obj_name, func_name, param_count, func_ptr, is_static):
    print("""
static {builtin_macro}(mp_{func_obj_name}_mpobj, {param_count}, mp_{func_name}, {func_ptr});
    """.format(
            func_obj_name = func_obj_name,
            func_name = func_name,
            func_ptr = func_ptr,
            param_count = param_count,
            builtin_macro='MP_DEFINE_CONST_LV_FUN_OBJ_STATIC_VAR' if is_static else 'MP_DEFINE_CONST_LV_FUN_OBJ_VAR'))


def gen_mp_func(func, obj_name):
    # print('/* gen_mp_func: %s : %s */' % (obj_name, func))
    if func.name in generated_funcs:
        print("""
/*
 * WARNING: %s was declared more than once!
 */
        """ % func.name)
        return
    # print("/* gen_mp_func %s */" % func.name)
    generated_funcs[func.name] = False # starting to generate the function

    func_md = {'args': [], 'c_type': func.name}

    if obj_name:
        func_md['py_type'] = 'method'
        func_md['scope'] = obj_name
    else:
        func_md['py_type'] = 'function'
        func_md['scope'] = 'global'

    args = func.type.args.params if func.type.args else []
    enumerated_args = enumerate(args)

    # Handle the case of a single function argument which is "void"
    if len(args)==1 and get_type(args[0].type, remove_quals = True) == "void":
        param_count = 0
        args = []
    else:
        param_count = len(args)

    # If func prototype matches an already generated func, reuse it and only emit func obj that points to it.
    prototype_str = gen.visit(function_prototype(func))
    if prototype_str in func_prototypes:
        original_func = func_prototypes[prototype_str]
        if generated_funcs[original_func.name] == True:
            print("/* Reusing %s for %s */" % (original_func.name, func.name))
            emit_func_obj(func.name, original_func.name, param_count, func.name, is_static_member(func, base_obj_type))

            func_md['c_rtype'] = func_metadata[original_func.name]['c_rtype']
            func_md['py_rtype'] = func_metadata[original_func.name]['py_rtype']

            func_md['args'] = func_metadata[original_func.name]['args']
            generated_funcs[func.name] = True # completed generating the function
            func_metadata[func.name] = func_md
            return
    func_prototypes[prototype_str] = func

    # user_data argument must be handled first, if it exists
    try:
        i = [(arg.name if hasattr(arg, 'name') else None) for arg in args].index('user_data')
        if i>0:
            enumerated_args = [(i, arg) for i, arg in enumerated_args] # convert enumerate to list
            enumerated_args[0], enumerated_args[i] = enumerated_args[i], enumerated_args[0]
    except ValueError:
        pass

    return_type = get_type(func.type.type, remove_quals = False)
    qualified_return_type = gen.visit(func.type.type)
    if isinstance(func.type.type, c_ast.PtrDecl) and lv_func_returns_array.match(func.name):
        try_generate_array_type(func.type.type)
    # print('/* --> return_type = %s, qualified_return_type = %s\n%s */' % (return_type, qualified_return_type, func.type.type))
    if return_type == "void":
        build_result = ""
        build_return_value = "mp_const_none"

        func_md['py_rtype'] = 'None'
        func_md['c_rtype'] = 'void'
    else:
        if return_type not in lv_to_mp or not lv_to_mp[return_type]:
            try_generate_type(func.type.type)
            if return_type not in lv_to_mp or not lv_to_mp[return_type]:
                raise MissingConversionException("Missing conversion from %s" % return_type)

        build_result = "%s _res = " % qualified_return_type
        cast = '(void*)' if isinstance(func.type.type, c_ast.PtrDecl) else '' # needed when field is const. casting to void overrides it
        build_return_value = "{type}({cast}_res)".format(type = lv_to_mp[return_type], cast = cast)

        func_md['py_rtype'] = get_py_type(return_type)
        func_md['c_rtype'] = return_type

    build_args = []
    for i, arg in enumerated_args:
        if (
            isinstance(arg, c_ast.EllipsisParam) or
            not isinstance(arg.type, c_ast.TypeDecl) or
            not isinstance(arg.type.type, c_ast.IdentifierType) or
            'void' not in arg.type.type.names
        ):  # Handle the case of 'void' param which should be ignored
            ba, arg_metadata = build_mp_func_arg(arg, i, func, obj_name)
            build_args.append(ba)
            func_md['args'].append(arg_metadata)

    print("""
/*
 * {module_name} extension definition for:
 * {print_func}
 */

static mp_obj_t mp_{func}(size_t mp_n_args, const mp_obj_t *mp_args, void *lv_func_ptr)
{{
    {build_args}
    {build_result}(({func_ptr})lv_func_ptr)({send_args});
    return {build_return_value};
}}

 """.format(
        module_name = module_name,
        func=func.name,
        func_ptr=prototype_str,
        print_func=gen.visit(func),
        build_args="\n    ".join(build_args), # Handle the case of 'void' param which should be ignored
        send_args=", ".join([(arg.name if (hasattr(arg, 'name') and arg.name) else ("arg%d" % i)) for i,arg in enumerate(args)]),
        build_result=build_result,
        build_return_value=build_return_value))

    emit_func_obj(func.name, func.name, param_count, func.name, is_static_member(func, base_obj_type))
    generated_funcs[func.name] = True # completed generating the function
    func_metadata[func.name] = func_md

    # print('/* is_struct_function() = %s, is_static_member() = %s, get_first_arg_type()=%s, obj_name = %s */' % (
    #    is_struct_function(func), is_static_member(func, base_obj_type), get_first_arg_type(func), base_obj_type))


def gen_func_error(method, exp):
    global funcs
    print("""
/*
 * Function NOT generated:
 * {problem}
 * {method}
 */
    """.format(method=gen.visit(method) if isinstance(method, c_ast.Node) else method, problem=exp))
    try:
        funcs.remove(method)
    except:
        pass


#
# Emit Mpy objects definitions
#
enum_referenced = collections.OrderedDict()


def gen_obj_methods(obj_name):
    global enums
    helper_members = ["{ MP_ROM_QSTR(MP_QSTR___cast__), MP_ROM_PTR(&cast_obj_class_method) }"] if len(obj_names) > 0 and obj_name == base_obj_name else []
    members = ["{{ MP_ROM_QSTR(MP_QSTR_{method_name}), MP_ROM_PTR(&mp_{method}_mpobj) }}".
                    format(method=method.name, method_name=sanitize(method_name_from_func_name(method.name))) for method in get_methods(obj_name)]
    obj_metadata[obj_name]['members'].update({method_name_from_func_name(method.name): func_metadata[method.name] for method in get_methods(obj_name)})
    # add parent methods
    parent_members = []
    if obj_name in parent_obj_names and parent_obj_names[obj_name] != None:
        # parent_members += gen_obj_methods(parent_obj_names[obj_name])
        if 'members' in obj_metadata[parent_obj_names[obj_name]]:
            key = 'members'
        else:
            key = 'class_attributes'

        obj_metadata[obj_name]['members'].update(obj_metadata[parent_obj_names[obj_name]][key])
    # add enum members
    enum_members = ["{{ MP_ROM_QSTR(MP_QSTR_{enum_member}), MP_ROM_PTR({enum_member_value}) }}".
                    format(enum_member = sanitize(get_enum_member_name(enum_member_name)), enum_member_value = get_enum_value(obj_name, enum_member_name)) for enum_member_name in get_enum_members(obj_name)]

    obj_metadata[obj_name]['members'].update({get_enum_member_name(enum_member_name): {'c_type': obj_metadata[obj_name]['c_type'] + enum_member_name if enum_member_name.startswith('_') else obj_metadata[obj_name]['c_type'] + '_' + enum_member_name, 'py_type': 'int'} for enum_member_name in get_enum_members(obj_name)})

    # add enums that match object name
    obj_enums = [enum_name for enum_name in enums.keys() if is_method_of(enum_name, obj_name)]
    enum_types = ["{{ MP_ROM_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_lv_{enum}_type_base) }}".
                    format(name=sanitize(method_name_from_func_name(enum_name)), enum=enum_name) for enum_name in obj_enums]

    obj_metadata[obj_name]['members'].update({method_name_from_func_name(enum_name): {'c_type': obj_metadata[obj_name]['c_type'] + enum_name if enum_name.startswith('_') else obj_metadata[obj_name]['c_type'] + '_' + enum_name, 'py_type': 'int'} for enum_name in obj_enums})

    for enum_name in obj_enums:
        obj_metadata[obj_name]['classes'][method_name_from_func_name(enum_name)] = copy.deepcopy(obj_metadata[obj_name]['members'][method_name_from_func_name(enum_name)])
        del obj_metadata[obj_name]['members'][method_name_from_func_name(enum_name)]
        obj_metadata[obj_name]['classes'][method_name_from_func_name(enum_name)].update(copy.deepcopy(obj_metadata[enum_name]))
        enum_referenced[enum_name] = True

        obj_metadata[obj_name]['classes'][method_name_from_func_name(enum_name)]['c_type'] = 'enum'
        obj_metadata[obj_name]['classes'][method_name_from_func_name(enum_name)]['class_attributes'] = obj_metadata[obj_name]['classes'][method_name_from_func_name(enum_name)]['members']
        del obj_metadata[obj_name]['classes'][method_name_from_func_name(enum_name)]['members']

    return members + parent_members + enum_members + enum_types + helper_members


def gen_obj(obj_name):
    # eprint('Generating object %s...' % obj_name)
    is_obj = has_ctor(obj_name)
    should_add_base_methods = is_obj and obj_name != 'obj'
    obj_metadata[obj_name] = {'py_type': 'class', 'c_type': obj_name, 'members': collections.OrderedDict(), 'classes': collections.OrderedDict(), 'methods': collections.OrderedDict(), 'class_attributes': collections.OrderedDict()}

    if should_add_base_methods:
        obj_metadata[obj_name]['parent_class'] = 'obj'
    else:
        obj_metadata[obj_name]['parent_class'] = 'object'

    # Generate object methods
    for method in get_methods(obj_name):
        try:
            gen_mp_func(method, obj_name)
        except MissingConversionException as exp:
            gen_func_error(method, exp)

    # Generate object construction function, if needed
    if is_obj:
        ctor_func = get_ctor(obj_name)
        try:
            gen_mp_func(ctor_func, obj_name)
        except MissingConversionException as exp:
            gen_func_error(ctor_func, exp)

    # print([method.name for method in methods])
    ctor = """
static mp_obj_t {obj}_make_new(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args)
{{
    return make_new(&mp_{ctor_name}_mpobj, type, n_args, n_kw, args);
}}
"""

    print("""
/*
 * {module_name} {obj} object definitions
 */
    """.format(
            module_name = module_name,
            obj = obj_name))

    print("""
static const mp_rom_map_elem_t {obj}_locals_dict_table[] = {{
    {locals_dict_entries}
}};

static MP_DEFINE_CONST_DICT({obj}_locals_dict, {obj}_locals_dict_table);

{ctor}

static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_{obj}_type_base,
    MP_QSTR_{obj},
    MP_TYPE_FLAG_NONE,
    {make_new}
    {binary_op}
    attr, call_parent_methods,
    {buffer}
    {parent}
    locals_dict, &{obj}_locals_dict
);

GENMPY_UNUSED static const mp_lv_obj_type_t mp_lv_{obj}_type = {{
#ifdef LV_OBJ_T
    .lv_obj_class = {lv_class},
#endif
    .mp_obj_type = &mp_lv_{obj}_type_base,
}};
    """.format(
            module_name = module_name,
            obj = sanitize(obj_name), base_obj = base_obj_name,
            base_class = '&mp_%s_type' % base_obj_name if should_add_base_methods else 'NULL',
            locals_dict_entries = ",\n    ".join(gen_obj_methods(obj_name)),
            ctor = ctor.format(obj = obj_name, ctor_name = ctor_func.name) if has_ctor(obj_name) else '',
            make_new = 'make_new, %s_make_new,' % obj_name if is_obj else '',
            binary_op = 'binary_op, mp_lv_obj_binary_op,' if is_obj else '',
            buffer = 'buffer, mp_lv_obj_get_buffer,' if is_obj else '',
            parent = 'parent, &mp_lv_%s_type_base,' % parent_obj_names[obj_name] if obj_name in parent_obj_names and parent_obj_names[obj_name] else '',
            lv_class = '&lv_%s_class' % obj_name if is_obj else 'NULL',
            ))

    for meta_func_name, meta_func in list(obj_metadata[obj_name]['members'].items()):
        if 'scope' not in meta_func:
            continue

        if obj_metadata[obj_name]['parent_class'] == meta_func['scope']:
            del obj_metadata[obj_name]['members'][meta_func_name]


#
# Generate Enum objects
#
for enum_name in list(enums.keys()):
    gen_obj(enum_name)


#
# Generate all other objects. Generate parent objects first
#
generated_obj_names = collections.OrderedDict()
for obj_name in obj_names:
    # eprint("--> %s [%s]" % (obj_name, ", ".join([name for name in generated_obj_names])))
    parent_obj_name = parent_obj_names[obj_name] if obj_name in parent_obj_names else None

    while parent_obj_name != None and not parent_obj_name in generated_obj_names:
        gen_obj(parent_obj_name)
        generated_obj_names[parent_obj_name] = True
        parent_obj_name = parent_obj_names[parent_obj_name] if parent_obj_name in parent_obj_names else None

    if not obj_name in generated_obj_names:
        # eprint("--> gen obj %s" % obj_name)
        gen_obj(obj_name)
        generated_obj_names[obj_name] = True


#
# Generate structs which contain function members
# First argument of a function could be it's parent struct
# Need to make sure these structs are generated *before* struct-functions are
# Otherwise we will not know of all the structs when generating struct-functions
#
def try_generate_structs_from_first_argument():
    for func in funcs:
        if func.name in generated_funcs: continue
        args = func.type.args.params if func.type.args else []
        if len(args) < 1: continue
        arg_type = get_type(args[0].type, remove_quals = True)
        if arg_type not in mp_to_lv or not mp_to_lv[arg_type]:
            try:
                try_generate_type(args[0].type)
            except MissingConversionException as e:
                print('''
/*
 * {struct} not generated: {err}
 */
                '''.format(struct=arg_type, err=e))


# eprint("/* Generating globals */")

#
# Generate globals
#
def gen_global(global_name, global_type_ast):
    global_type = get_type(global_type_ast, remove_quals=True)
    generated_global = try_generate_type(global_type_ast)
    # print("/* generated_global = %s */" % generated_global)
    if global_type not in generated_structs:
        wrapped_type = lv_mp_type[global_type]
        if not wrapped_type:
            raise MissingConversionException('Missing conversion to %s when generating global %s' % (wrapped_type, global_name))
        global_type = sanitize("_lv_mp_%s_wrapper" % wrapped_type)
        custom_struct_str = """
typedef struct {{
    {type} value;
}} {name};
        """.format(
            type = wrapped_type,
            name = global_type)
        if global_type not in generated_structs:
            print("/* Global struct wrapper for %s */" % wrapped_type)
            print(custom_struct_str)
            # eprint("%s: %s\n" % (wrapped_type, custom_struct_str))
            try_generate_struct(global_type, parser.parse(custom_struct_str).ext[0].type.type)

    ct = global_name

    if '_class' in ct:
        ct += '_t'
    variable_metadata[get_py_type(global_name).replace('"', '')] = {'py_type': get_py_type(global_type), 'c_type': global_name}

    print("""
/*
 * {module_name} {global_name} global definitions
 */

static const mp_lv_struct_t mp_{global_name} = {{
    {{ &mp_{struct_name}_type }},
    ({cast}*)&{global_name}
}};
    """.format(
            module_name = module_name,
            global_name = global_name,
            struct_name = global_type,
            sanitized_struct_name = sanitize(global_type),
            cast = gen.visit(global_type_ast)))


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
    # print('/* List of structs: %s */' % repr(struct_list))
    for struct_name in struct_list:
        if not generated_structs[struct_name]:
            continue

        sanitized_struct_name = sanitize(struct_name)

        struct_funcs = get_struct_functions(struct_name)
        # print('/* Struct %s contains: %s */' % (struct_name, [f.name for f in struct_funcs]))
        for struct_func in struct_funcs[:]: # clone list because we are changing it in the loop.
            try:
                if struct_func.name not in generated_funcs:
                    gen_mp_func(struct_func, struct_name)

                    if get_py_type(struct_name).replace('"', '') not in struct_metadata:
                        struct_metadata[get_py_type(struct_name).replace('"', '')] = {'class_attributes': collections.OrderedDict(), 'attributes': collections.OrderedDict(), 'py_type': 'class', 'c_type': struct_name, 'methods': collections.OrderedDict()}

                    struct_metadata[get_py_type(struct_name).replace('"', '')]['methods'][sanitize(noncommon_part(struct_func.name, struct_name))] = copy.deepcopy(func_metadata[struct_func.name])
            except MissingConversionException as exp:
                gen_func_error(struct_func, exp)
                struct_funcs.remove(struct_func)

        if struct_name not in structs or structs[struct_name].decls:
            struct_size_attr = '{{ MP_ROM_QSTR(MP_QSTR___SIZE__), MP_ROM_PTR(MP_ROM_INT(sizeof({struct_tag}{struct_name}))) }},'.format(
                struct_name = struct_name,
                struct_tag = 'struct ' if struct_name in structs_without_typedef.keys() else '',
            )
            struct_metadata[get_py_type(struct_name).replace('"', '')]['class_attributes']['__SIZE__'] = {'c_type': 'int', 'py_type': 'int'}
        else:
            struct_size_attr = ''
        print('''
static const mp_rom_map_elem_t mp_{sanitized_struct_name}_locals_dict_table[] = {{
    {struct_size}
    {functions}
}};

static MP_DEFINE_CONST_DICT(mp_{sanitized_struct_name}_locals_dict, mp_{sanitized_struct_name}_locals_dict_table);
        '''.format(
            struct_size = struct_size_attr,
            sanitized_struct_name = sanitized_struct_name,
            functions =  ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_{func}_mpobj) }},\n    '.
                format(name = sanitize(noncommon_part(f.name, struct_name)), func = f.name) for f in struct_funcs]),
        ))

        generated_struct_functions[struct_name] = True


generate_struct_functions(list(generated_structs.keys()))

#
# Generate all module functions (not including method functions which were already generated)
#

print("""
/*
 *
 * Global Module Functions
 *
 */
""")

# eprint("/* Generating global module functions /*")
module_funcs = [func for func in funcs if not func.name in generated_funcs]
for module_func in module_funcs[:]: # clone list because we are changing it in the loop.
    if module_func.name in generated_funcs:
        continue # generated_funcs could change inside the loop so need to recheck.
    try:
        gen_mp_func(module_func, None)
        # A new function can create new struct with new function structs
        new_structs = [s for s in generated_structs if s not in generated_struct_functions]
        if new_structs:
            generate_struct_functions(new_structs)

    except MissingConversionException as exp:
        gen_func_error(module_func, exp)
        module_funcs.remove(module_func)


functions_not_generated = [func.name for func in funcs if func.name not in generated_funcs]
if len(functions_not_generated) > 0:
    print("""
/*
 * Functions not generated:
 * {funcs}
 *
 */

""".format(funcs = "\n * ".join(functions_not_generated)))

#
# Generate callback functions
#

# for func_typedef in func_typedefs:
#     func = func_typedef.type.type
#     try:
#         gen_callback_func(func)
#     except MissingConversionException as exp:
#         gen_func_error(func, exp)
#         func_name = get_arg_name(func.type)
#         lv_to_mp[func_name] = lv_to_mp['void *']
#         mp_to_lv[func_name] = mp_to_lv['void *']


# eprint("/* Generating callback functions */")
for (func_name, func, struct_name) in callbacks_used_on_structs:
    try:
        # print('/* --> gen_callback_func %s */' % func_name)
        gen_callback_func(func, func_name = '%s_%s' % (struct_name, func_name))
        # struct_metadata[struct_name]['methods'][func_name] = copy.deepcopy(callback_metadata[func.name])
    except MissingConversionException as exp:
        gen_func_error(func, exp)
        # func_name = get_arg_name(func.type)
        # lv_to_mp[func_name] = lv_to_mp['void *']
        # mp_to_lv[func_name] = mp_to_lv['void *']

#
# Emit Mpy Module definition
#

# eprint("/* Generating module definition */")
print("""

/*
 * {module_name} module definitions
 */

static const mp_rom_map_elem_t {module_name}_globals_table[] = {{
    {{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_{module_name}) }},
    {objects}
    {functions}
    {enums}
    {structs}
    {struct_aliases}
    {blobs}
    {int_constants}
#ifdef LV_OBJ_T
    {{ MP_ROM_QSTR(MP_QSTR_LvReferenceError), MP_ROM_PTR(&mp_type_LvReferenceError) }},
#endif // LV_OBJ_T
}};
""".format(
        module_name = sanitize(module_name),
        objects = ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{obj}), MP_ROM_PTR(&mp_lv_{obj}_type_base) }},\n    '.
            format(obj = sanitize(o)) for o in obj_names]),
        functions =  ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_{func}_mpobj) }},\n    '.
            format(name = sanitize(simplify_identifier(f.name)), func = f.name) for f in module_funcs]),
        enums = ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_lv_{enum}_type_base) }},\n    '.
            format(name = sanitize(get_enum_name(enum_name)), enum=enum_name) for enum_name in enums.keys() if enum_name not in enum_referenced]),
        structs = ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_{struct_name}_type) }},\n    '.
            format(name = sanitize(simplify_identifier(struct_name)), struct_name = sanitize(struct_name)) for struct_name in generated_structs \
                    if generated_structs[struct_name]]),
        struct_aliases = ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{alias_name}), MP_ROM_PTR(&mp_{struct_name}_type) }},\n    '.
            format(struct_name = sanitize(struct_name), alias_name = sanitize(simplify_identifier(struct_aliases[struct_name]))) for struct_name in struct_aliases.keys()]),
        blobs = ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_{global_name}) }},\n    '.
            format(name = sanitize(simplify_identifier(global_name)), global_name = global_name) for global_name in generated_globals]),
        int_constants = ''.join(['{{ MP_ROM_QSTR(MP_QSTR_{name}), MP_ROM_PTR(MP_ROM_INT({value})) }},\n    '.
            format(name = sanitize(get_enum_name(int_constant)), value = int_constant) for int_constant in int_constants])))


print("""
static MP_DEFINE_CONST_DICT (
    mp_module_{module_name}_globals,
    {module_name}_globals_table
);

const mp_obj_module_t mp_module_{module_name} = {{
    .base = {{ &mp_type_module }},
    .globals = (mp_obj_dict_t*)&mp_module_{module_name}_globals
}};

MP_REGISTER_MODULE(MP_QSTR_{module_name}, mp_module_{module_name});

""".format(
        module_name = module_name,
    ))


# Add an array of all object types
if len(obj_names) > 0:
    print('''
static const mp_lv_obj_type_t *mp_lv_obj_types[] = {{
    {obj_types},
    NULL
}};
    '''.format(obj_types = ',\n    '.join(['&mp_lv_%s_type' % obj_name for obj_name in obj_names])))

# Save Metadata File, if specified.


def _iter_metadata(d, indent=0):
    idt = ' ' * indent
    for key, value in d.items():
        if isinstance(value, dict):
            LOG(f"{idt}'{key}': {{")
            _iter_metadata(value, indent + 4)
            LOG(f'{idt}}},')
        elif isinstance(value, list) and value and isinstance(value[0], dict):
            LOG(f"{idt}'{key}': [")
            extra_idt = ' ' * (indent + 4)
            for item in value:
                LOG(f'{extra_idt}{{')
                _iter_metadata(item, indent + 8)
                LOG(f'{extra_idt}}},')
            LOG(f"{idt}],")
        else:
            LOG(f"{idt}'{key}': {repr(value)},")


for obj_name in obj_names:
    parent = obj_metadata[obj_name]['parent_class']

    obj_metadata[obj_name]['methods'].update(copy.deepcopy(obj_metadata[obj_name]['members']))
    del obj_metadata[obj_name]['members']

    if parent not in obj_metadata:
        continue

    for enum_name in list(obj_metadata[obj_name]['classes']):
        if enum_name in obj_metadata[parent]['classes']:
            del obj_metadata[obj_name]['classes'][enum_name]


for name, metadata in list(variable_metadata.items()):
    if not metadata['c_type'].endswith('class'):
        continue

    obj_metadata[f'{name}_class'] = {
        'class_attributes': collections.OrderedDict(),
        'attributes': collections.OrderedDict(),
        'py_type': 'class',
        'c_type': metadata['c_type'] + '_t',
        'methods': collections.OrderedDict()
    }

    obj_names.append(f'{name}_class')
    del variable_metadata[name]


if args.metadata:

    metadata = collections.OrderedDict()
    metadata['objects'] = {obj_name: obj_metadata[obj_name] for obj_name in obj_names}
    metadata['functions'] = {simplify_identifier(f.name): func_metadata[f.name] for f in module_funcs}
    metadata['enums'] = {get_enum_name(enum_name): obj_metadata[enum_name] for enum_name in enums.keys() if enum_name not in enum_referenced}
    metadata['structs'] = {simplify_identifier(struct_name): struct_metadata[simplify_identifier(struct_name)] for struct_name in generated_structs if simplify_identifier(struct_name) in struct_metadata}
    # metadata['structs'] += [simplify_identifier(struct_aliases[struct_name]) for struct_name in struct_aliases.keys()]
    metadata['variables'] = {get_py_type(global_name).replace('"', ''): variable_metadata[get_py_type(global_name).replace('"', '')] for global_name in generated_globals if get_py_type(global_name).replace('"', '') in variable_metadata}
    metadata['int_constants'] = {get_enum_name(int_constant): constant_metadata[get_enum_name(int_constant)] for int_constant in int_constants}

    # TODO: struct functions
    
    _iter_metadata(metadata)

    with open(args.metadata, 'w') as metadata_file:
        json.dump(metadata, metadata_file, indent=4)

    build_path = os.path.split(args.metadata)[0]

    api_json_path = os.path.join(build_path, 'lvgl_api.json')

    with open(api_json_path, 'w') as metadata_file:
        json.dump(lvgl_json, metadata_file, indent=4)

    import stub_gen

    stub_gen.run(args.metadata, api_json_path)

stdout.close()

