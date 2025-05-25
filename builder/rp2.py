# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import sys
from argparse import ArgumentParser
from . import spawn
from . import generate_manifest
from . import update_mphalport


board_variant = None
board = None


def parse_args(extra_args, lv_cflags, brd):
    global board_variant
    global board

    board = brd

    if board == 'WEACTSTUDIO':
        rp2_argParser = ArgumentParser(prefix_chars='-B')

        rp2_argParser.add_argument(
            'BOARD_VARIANT',
            dest='board_variant',
            default='',
            action='store'
        )
        rp2_args, extra_args = rp2_argParser.parse_known_args(extra_args)

        board_variant = rp2_args.board_variant

    else:
        for arg in extra_args:
            if arg.startswith('BOARD_VARIANT'):
                raise RuntimeError(f'BOARD_VARIANT not supported by "{board}"')

    return extra_args, lv_cflags, board


mpy_cross_cmd = ['make', '-C', 'lib/micropython/mpy-cross']
rp2_cmd = [
    'make',
    '',
    f'-j {os.cpu_count()}',
    '-C',
    'lib/micropython/ports/rp2',
    'LV_PORT=rp2',
    'USER_C_MODULES=../../../../ext_mod/micropython.cmake',
    'SECOND_BUILD=0'
]

clean_cmd = []
compile_cmd = []
submodules_cmd = []


def build_commands(_, extra_args, __, lv_cflags, brd):
    # rp2_cmd.extend(extra_args)

    if lv_cflags:
        rp2_cmd.insert(6, f'LV_CFLAGS="{lv_cflags}"')

    if brd is not None:
        if lv_cflags:
            rp2_cmd.insert(7, f'BOARD={brd}')
            if board_variant:
                rp2_cmd.insert(8, f'BOARD_VARIANT={board_variant}')
        else:
            rp2_cmd.insert(6, f'BOARD={brd}')
            if board_variant:
                rp2_cmd.insert(7, f'BOARD_VARIANT={board_variant}')

    clean_cmd.extend(rp2_cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(rp2_cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(rp2_cmd[:])
    submodules_cmd[1] = 'submodules'

    return extra_args


def build_manifest(
    target, script_dir, lvgl_api, displays, indevs, expanders, imus, frozen_manifest
):
    update_mphalport(target)
    
    manifest_path = 'lib/micropython/ports/rp2/boards/manifest.py'

    generate_manifest(
        script_dir, lvgl_api, manifest_path,
        displays, indevs, expanders, imus, frozen_manifest
    )


def force_clean(clean_mpy_cross):
    if clean_mpy_cross:
        cross_clean = mpy_cross_cmd[:]
        cross_clean.insert(1, 'clean')
        spawn(cross_clean)

    spawn(clean_cmd)


def clean():
    force_clean(False)


def submodules():
    pico_dsk_path = os.path.abspath('lib/micropython/lib/pico-sdk')

    if not os.path.exists(
        os.path.join(pico_dsk_path, 'pico_sdk_init.cmake')
    ):
        print()
        print('collecting PICO-SDK')

        ret_code, _ = spawn([
            ['cd', pico_dsk_path],
            ['git', 'submodule', 'update', '--init']
        ])

        if ret_code != 0:
            sys.exit(ret_code)

        return_code, _ = spawn(submodules_cmd)
        if return_code != 0:
            sys.exit(return_code)


def compile(*args):  # NOQA
    import shutil

    src_path = 'micropy_updates/rp2'
    dst_path = 'lib/micropython/ports/rp2'

    for file in os.listdir(src_path):
        src_file = os.path.join(src_path, file)
        dst_file = os.path.join(dst_path, file)
        shutil.copyfile(src_file, dst_file)

    if 'PICO_SDK_PATH' not in os.environ:
        os.environ['PICO_SDK_PATH'] = (
            f'{os.getcwd()}/lib/micropython/lib/pico-sdk'
        )

    cmd_ = compile_cmd[:]
    cmd_.extend(list(args))

    return_code, _ = spawn(cmd_, cmpl=True)
    if return_code != 0:
        sys.exit(return_code)

    os.remove('build/lvgl_header.h')

    for f in os.listdir('build'):
        if f.startswith('lvgl'):
            continue

        os.remove(os.path.join('build', f))

    if board_variant:
        src = (
            f'lib/micropython/ports/rp2/'
            f'build-{board}_{board_variant}/firmware.uf2'
        )
        dst = f'build/lvgl_micropy_{board}_{board_variant}.uf2'
    else:
        src = f'lib/micropython/ports/rp2/build-{board}/firmware.uf2'
        dst = f'build/lvgl_micropy_{board}.uf2'

    shutil.copyfile(src, dst)

    print(f'compiled binary is {os.path.abspath(dst)}')


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd, cmpl=True)
    if return_code != 0:
        sys.exit(return_code)
