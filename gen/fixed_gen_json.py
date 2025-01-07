# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import sys
import shutil
import subprocess
import tempfile

base_path = os.path.abspath(os.path.dirname(__file__))

project_path = os.path.abspath(os.path.join(base_path, '..', 'lib', 'lvgl'))
docs_path = os.path.join(project_path, 'docs')
sys.path.insert(0, docs_path)


import create_fake_lib_c  # NOQA
import pycparser_monkeypatch  # NOQA
import pycparser  # NOQA


temp_directory = tempfile.mkdtemp(suffix='.lvgl_json')


def dummy_list():
    return []


def dummy_doc(_):
    return None


pycparser_monkeypatch.get_enum_item_docs = dummy_doc
pycparser_monkeypatch.get_enum_docs = dummy_doc
pycparser_monkeypatch.get_func_docs = dummy_doc
pycparser_monkeypatch.get_var_docs = dummy_doc
pycparser_monkeypatch.get_union_docs = dummy_doc
pycparser_monkeypatch.get_struct_docs = dummy_doc
pycparser_monkeypatch.get_typedef_docs = dummy_doc
pycparser_monkeypatch.get_macro_docs = dummy_doc
pycparser_monkeypatch.get_macros = dummy_list


def run(lvgl_config_path, target_header, filter_private):

    pycparser_monkeypatch.FILTER_PRIVATE = filter_private

    lvgl_path = project_path
    lvgl_src_path = os.path.join(lvgl_path, 'src')
    temp_lvgl = os.path.join(temp_directory, 'lvgl')
    target_header_base_name = (
        os.path.splitext(os.path.split(target_header)[-1])[0]
    )

    os.mkdir(temp_lvgl)
    shutil.copytree(lvgl_src_path, os.path.join(temp_lvgl, 'src'))
    shutil.copyfile(os.path.join(lvgl_path, 'lvgl.h'), os.path.join(temp_lvgl, 'lvgl.h'))

    pp_file = os.path.join(temp_directory, target_header_base_name + '.pp')

    src = lvgl_config_path
    dst = os.path.join(temp_directory, 'lv_conf.h')
    shutil.copyfile(src, dst)

    include_dirs = [temp_directory, project_path]

    if sys.platform.startswith('darwin'):
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

    fake_libc_path = create_fake_lib_c.run(temp_directory)

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

    shutil.rmtree(temp_directory)

    return ast
