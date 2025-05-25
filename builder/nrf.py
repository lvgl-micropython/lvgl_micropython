# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import sys
from . import spawn
from . import generate_manifest
from . import update_mphalport


def parse_args(extra_args, lv_cflags, board):
    return extra_args, lv_cflags, board


mpy_cross_cmd = ['make', '-C', 'lib/micropython/mpy-cross']
nrf_cmd = [
    'make',
    '',
    f'-j {os.cpu_count()}',
    '-C',
    'lib/micropython/ports/nrf',
    'LV_PORT=nrf'
]

clean_cmd = []
compile_cmd = []
submodules_cmd = []


def build_commands(_, extra_args, script_dir, lv_cflags, board):
    if lv_cflags:
        lv_cflags += ' -DLV_USE_TINY_TTF=0'
    else:
        lv_cflags = '-DLV_USE_TINY_TTF=0'

    nrf_cmd.append(f'USER_C_MODULES="{script_dir}/ext_mod"')

    # nrf_cmd.extend(extra_args)

    if lv_cflags:
        nrf_cmd.insert(6, f'LV_CFLAGS="{lv_cflags}"')

    if board is not None:
        if lv_cflags:
            nrf_cmd.insert(7, f'BOARD={board}')
        else:
            nrf_cmd.insert(6, f'BOARD={board}')

    clean_cmd.extend(nrf_cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(nrf_cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(nrf_cmd[:])
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
    manifest_path = 'lib/micropython/ports/nrf/modules/manifest.py'

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
    nrfx_path = f'lib/micropython/libs/nrfx'
    if not os.path.exists(os.path.join(nrfx_path, 'nrfx.h')):
        print()
        print('collecting NRFX')

        ret_code, _ = spawn([
            'git',
            'submodule',
            'update',
            '--init',
            '--',
            nrfx_path
        ])

        if ret_code != 0:
            sys.exit(ret_code)

        return_code, _ = spawn(submodules_cmd)
        if return_code != 0:
            sys.exit(return_code)


def compile(*args):  # NOQA
    cmd_ = compile_cmd[:]
    cmd_.extend(list(args))

    return_code, _ = spawn(cmd_, cmpl=True)
    if return_code != 0:
        sys.exit(return_code)


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd, cmpl=True)
    if return_code != 0:
        sys.exit(return_code)
