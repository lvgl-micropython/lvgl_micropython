# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import sys
import subprocess


SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
PROJECT_PATH = os.path.abspath(os.path.join(SCRIPT_PATH, '..'))

while 'lib' not in os.listdir(PROJECT_PATH):
    PROJECT_PATH = os.path.abspath(os.path.join(PROJECT_PATH, '..'))

PYCPARSER_PATH = os.path.join(PROJECT_PATH, 'lib', 'pycparser')
FAKE_LIB_C_PATH = os.path.join(SCRIPT_PATH, 'fake_libc')

sys.path.insert(0, os.path.abspath(PYCPARSER_PATH))


if __name__ == '__main__':

    sys.path.append(os.path.abspath(os.path.join(SCRIPT_PATH, '..')))

    import api_gen

    import argparse

    # Argument parsing
    argParser = argparse.ArgumentParser()

    argParser.add_argument(
        '-I',
        '--include',
        dest='include',
        help='Preprocesor include path',
        metavar='<Include Path>',
        action='append',
        default=[]
    )
    argParser.add_argument(
        '-D',
        '--define',
        dest='define',
        help='Define preprocessor macro',
        metavar='<Macro Name>',
        action='append',
        default=[]
    )
    argParser.add_argument(
        '--metadata',
        dest='metadata',
        help='Optional file to emit metadata (introspection)',
        metavar='<MetaData File Name>',
        action='store',
        default=None

    )
    argParser.add_argument(
        '--output',
        dest='output',
        help='Output file path',
        metavar='<Output path>',
        action='store',
        default=None
    )
    argParser.add_argument(
        '--debug',
        dest='debug',
        help='enable debugging output',
        action='store_true',
        default=False
    )
    argParser.add_argument(
        '--header_file',
        dest='header',
        action='store',
        default=None
    )

    args, unknownargs = argParser.parse_known_args()

    if args.output is None:
        output = os.path.join(SCRIPT_PATH, 'output.pp')
    else:
        output = args.output

    if args.header is None:
        header = os.path.join(PROJECT_PATH, 'lib', 'lvgl', 'lvgl.h')
    else:
        header = args.header

    parser = api_gen.__main__(
        header, output, args.include,
        args.define, *unknownargs,
        debug=args.debug
    )

    from api_gen import templates

    print(templates.module(
        header,
        parser.enum_definitions,
        parser.struct_definitions,
        parser.union_definitions,
        parser.function_definitions,
        parser.variable_definitions,
        parser.module_decls
    ))

    from api_gen.json_reader import converters
    import json

    print()
    print(json.dumps(converters.mp_to_c, indent=4))
    print()
    print()
    print(json.dumps(converters.c_to_mp, indent=4))

    sys.exit()


from . import patch_pycparser  # NOQA

from . import pp_to_json
from . import json_reader


def get_preprocessor_output(output, input_header, includes_, defines, *addl_args):
    # this block of code is used to handle the generation of the preprocessor
    # output. Since pycparser has the ability to call the compiler internally
    # there is no need to do it from cmake. Data doesn't need to be going in
    # and out of cmake like that when it can all be done in a single location.
    # it makes things easier.

    # when compiling under Windows we want to set up a build system that
    # points to all the right things to allow pycparser to work correctly
    # when generating the preprocessor output. I have not yet fully determined
    # the best way to handle the pyMSVC dependency as it is not needed for
    # folks running any other OS except Windows.

    pp_file = output.rsplit('.', 1)[0] + '.pp'

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

    else:
        include_path_env_key = 'C_INCLUDE_PATH'
        cpp_cmd = [
            'gcc', '-std=c11', '-E', '-Wno-incompatible-pointer-types',
        ]
        output_pp = f' >> "{pp_file}"'

    if include_path_env_key not in os.environ:
        os.environ[include_path_env_key] = ''

    os.environ[include_path_env_key] = (
        f'{FAKE_LIB_C_PATH}{os.pathsep}{os.environ[include_path_env_key]}'
    )

    if 'PATH' not in os.environ:
        os.environ['PATH'] = ''

    os.environ['PATH'] = (
        f'{FAKE_LIB_C_PATH}{os.pathsep}{os.environ["PATH"]}'
    )

    cpp_cmd.extend([f'-D{define}' for define in defines])
    cpp_cmd.extend(['-DPYCPARSER', '-E', f'-I{FAKE_LIB_C_PATH}'])
    cpp_cmd.extend([f'-I{include}' for include in includes_])
    cpp_cmd.append(f'"{input_header}"')

    if sys.platform.startswith('win'):
        cpp_cmd.insert(len(cpp_cmd) - 2, output_pp)
    else:
        cpp_cmd.append(output_pp)

    cpp_cmd.extend(list(addl_args))

    cpp_cmd = ' '.join(cpp_cmd)

    p = subprocess.Popen(
        cpp_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=os.environ,
        shell=True
    )
    out, err_ = p.communicate()
    exit_code = p.returncode

    if not os.path.exists(pp_file):
        sys.stdout.write(out.decode('utf-8').strip() + '\n')
        sys.stdout.write('EXIT CODE: ' + str(exit_code) + '\n')
        sys.stderr.write(err_.decode('utf-8').strip() + '\n')
        sys.stdout.flush()
        sys.stderr.flush()

        raise RuntimeError('Unknown Failure')

    return pp_file


def __main__(input_header, output_file, includes, defines, *compiler_options, debug=False):
    pp_file = get_preprocessor_output(output_file, input_header, includes,
                                      defines, *compiler_options)


    from pycparser.c_generator import CGenerator
    generator = CGenerator()

    with open(r'C:\Users\drsch\PycharmProjects\lvgl_micropython\gen\api_gen\lvgl.pp', 'r') as f:
        data = f.read()

    print(generator.visit(data))
    raise RuntimeError

    json_data = pp_to_json.JSONGenerator.parse(input_header, f.read(), False)
    return json_reader.CGenerator.parse(json_data)
    