import os
import sys
from . import spawn
from . import generate_manifest
from . import update_mphalport


def parse_args(extra_args, lv_cflags, board):
    return extra_args, lv_cflags, board


mpy_cross_cmd = ['make', '-C', 'lib/micropython/mpy-cross']
rp2_cmd = [
    'make',
    '',
    f'-j {os.cpu_count()}',
    '-C',
    'lib/micropython/ports/rp2',
    'LV_PORT=rp2',
    'USER_C_MODULES=../../../../micropython.cmake'
]

clean_cmd = []
compile_cmd = []
submodules_cmd = []


def build_commands(_, extra_args, __, lv_cflags, board):
    rp2_cmd.extend(extra_args)

    if lv_cflags is not None:
        rp2_cmd.insert(6, f'LV_CFLAGS="{lv_cflags}"')

    if board is not None:
        if lv_cflags is not None:
            rp2_cmd.insert(7, f'BOARD={board}')
        else:
            rp2_cmd.insert(6, f'BOARD={board}')

    clean_cmd.extend(rp2_cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(rp2_cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(rp2_cmd[:])
    submodules_cmd[1] = 'submodules'


def build_manifest(target, script_dir, frozen_manifest):
    update_mphalport(target)
    
    manifest_path = 'lib/micropython/ports/rp2/boards/manifest.py'

    manifest_files = [
        f'{script_dir}/driver/display/display_driver_framework.py',
        f'{script_dir}/driver/fs_driver.py',
        f'{script_dir}/utils/lv_utils.py',
    ]
    generate_manifest(manifest_path, frozen_manifest, *manifest_files)


def clean():
    spawn(clean_cmd)


def submodules():
    if 'PICO_SDK_PATH' not in os.environ:
        pico_dsk_path = os.path.abspath('lib/micropython/lib/pico-sdk')

        if not os.path.exists(
            os.path.join(pico_dsk_path, 'pico_sdk_init.cmake')
        ):
            ret_code, _ = spawn([
                ['cd', pico_dsk_path],
                ['git', 'submodule', 'init']
            ])

            if ret_code != 0:
                sys.exit(ret_code)

    return_code, _ = spawn(submodules_cmd)
    if return_code != 0:
        sys.exit(return_code)


def compile():  # NOQA
    if 'PICO_SDK_PATH' not in os.environ:
        os.environ['PICO_SDK_PATH'] = f'{os.getcwd()}/lib/micropython/lib/pico-sdk'

    return_code, _ = spawn(compile_cmd)
    if return_code != 0:
        sys.exit(return_code)


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd)
    if return_code != 0:
        sys.exit(return_code)
