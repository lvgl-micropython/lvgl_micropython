import sys
from . import spawn
from . import generate_manifest
from . import update_mphalport


mpy_cross_cmd = []
clean_cmd = []
compile_cmd = []
submodules_cmd = []


def parse_args(extra_args, lv_cflags, board):
    return extra_args, lv_cflags, board


def build_commands(_, extra_args, script_dir, lv_cflags, __):
    if sys.platform.startswith('win'):
        try:
            import pyMSVC

            env = pyMSVC.setup_environment()
            print(env)
        except ImportError:
            pass

        mpy_cross_cmd.extend([
            'msbuild',
            'lib/micropython/mpy-cross/mpy-cross.vcxproj'
        ])
        compile_cmd.extend([
            'msbuild',
            'lib/micropython/ports/windows/micropython.vcxproj',
        ])

    else:
        mpy_cross_cmd.extend(['make', '-C', 'lib/micropython/mpy-cross'])
        clean_cmd.extend([
            'make',
            'clean',
            '-C',
            'lib/micropython/ports/windows'
            'LV_PORT=windows'
        ])

        compile_cmd.extend([
            'make',
            '-C',
            f'lib/micropython/ports/windows'
            f'LV_PORT=windows',
        ])

    if lv_cflags is not None:
        clean_cmd.append(f'LV_CFLAGS="{lv_cflags}"')
        compile_cmd.append(f'LV_CFLAGS="{lv_cflags}"')

    clean_cmd.append(f'USER_C_MODULES="{script_dir}/ext_mod"')
    compile_cmd.append(f'USER_C_MODULES="{script_dir}/ext_mod"')

    clean_cmd.extend(extra_args)
    compile_cmd.extend(extra_args)


def build_manifest(target, script_dir, frozen_manifest):
    update_mphalport(target)
    
    manifest_path = 'lib/micropython/ports/windows/variants/manifest.py'

    manifest_files = [
        f'{script_dir}/driver/display/display_driver_framework.py',
        f'{script_dir}/driver/fs_driver.py',
        f'{script_dir}/utils/lv_utils.py',
    ]
    generate_manifest(manifest_path, frozen_manifest, *manifest_files)


def clean():
    spawn(clean_cmd)


def submodules():
    pass


def compile():  # NOQA
    return_code, _ = spawn(compile_cmd)
    if return_code != 0:
        sys.exit(return_code)


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd)
    if return_code != 0:
        sys.exit(return_code)
