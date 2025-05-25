# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import sys
from . import spawn
from . import generate_manifest
from . import update_mphalport

board = None
board_variant = None


def parse_args(extra_args, lv_cflags, brd):
    global board
    board = brd

    return extra_args, lv_cflags, board


mpy_cross_cmd = ['make', '-C', 'lib/micropython/mpy-cross']
stm32_cmd = [
    'make',
    '',
    f'-j {os.cpu_count()}',
    '-C',
    'lib/micropython/ports/stm32',
    'LV_PORT=stm32'
]

clean_cmd = []
compile_cmd = []
submodules_cmd = []


def build_commands(_, extra_args, script_dir, lv_cflags, brd):

    if lv_cflags:
        lv_cflags += ' -DLV_USE_TINY_TTF=0'
    else:
        lv_cflags = '-DLV_USE_TINY_TTF=0'

    stm32_cmd.append(f'USER_C_MODULES="{script_dir}/ext_mod"')

    # stm32_cmd.extend(extra_args)

    if lv_cflags:
        stm32_cmd.insert(6, f'LV_CFLAGS="{lv_cflags}"')

    if board is not None:
        if lv_cflags:
            stm32_cmd.insert(7, f'BOARD={brd}')
        else:
            stm32_cmd.insert(6, f'BOARD={brd}')

    clean_cmd.extend(stm32_cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(stm32_cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(stm32_cmd[:])
    submodules_cmd[1] = 'submodules'
    return extra_args


def build_manifest(
    target,
    script_dir,
    lvgl_api,
    displays,
    indevs,
    expanders,
    imus,
    frozen_manifest
):
    update_mphalport(target)
    
    manifest_path = 'lib/micropython/ports/stm32/boards/manifest.py'

    generate_manifest(script_dir, lvgl_api, manifest_path, displays,
                      indevs, expanders, imus, frozen_manifest)


def force_clean(clean_mpy_cross):
    if clean_mpy_cross:
        cross_clean = mpy_cross_cmd[:]
        cross_clean.insert(1, 'clean')
        spawn(cross_clean)

    spawn(clean_cmd)


def clean():
    force_clean(False)


def submodules():
    stm32lib_path = 'lib/micropython/lib/stm32lib'
    if not os.path.exists(os.path.join(stm32lib_path, 'CMSIS')):
        print()
        print('collecting STM32 lib')

        ret_code, _ = spawn(
            [
                ['cd', stm32lib_path],
                ['git', 'submodule', 'update', '--init']
            ]
        )
        if ret_code != 0:
            sys.exit(ret_code)

        return_code, _ = spawn(submodules_cmd)
        if return_code != 0:
            sys.exit(return_code)


def compile(*args):  # NOQA
    import shutil

    src_path = 'micropy_updates/stm32'
    dst_path = 'lib/micropython/ports/stm32'

    for file in os.listdir(src_path):
        src_file = os.path.join(src_path, file)
        dst_file = os.path.join(dst_path, file)
        shutil.copyfile(src_file, dst_file)

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
            f'lib/micropython/ports/stm32/'
            f'build-{board}_{board_variant}/firmware.dfu'
        )
        dst = f'build/lvgl_micropy_{board}_{board_variant}.dfu'
    else:
        src = f'lib/micropython/ports/stm32/build-{board}/firmware.dfu'
        dst = f'build/lvgl_micropy_{board}.dfu'

    shutil.copyfile(src, dst)

    print(f'compiled binary is {os.path.abspath(dst)}')


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd, cmpl=True)
    if return_code != 0:
        sys.exit(return_code)
