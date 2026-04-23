# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import sys
from .unix import (
    parse_args as _parse_args,
    build_commands as _build_commands,
    build_manifest as _build_manifest,
    force_clean as _force_clean,
    clean as _clean,
    compile as _compile,
    mpy_cross as _mpy_cross
)

from . import unix
from . import spawn


unix.REAL_PORT = 'macOS'

if not os.path.exists('micropy_updates/originals/macOS'):
    os.makedirs('micropy_updates/originals/macOS')


def parse_args(extra_args, lv_cflags, board):
    return _parse_args(extra_args, lv_cflags, board)


def build_commands(not_sure, extra_args, script_dir, lv_cflags, board):
    return _build_commands(not_sure, extra_args, script_dir, lv_cflags, board)


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
    _build_manifest(target, script_dir, lvgl_api, displays,
                    indevs, expanders, imus, frozen_manifest)


def clean():
    _clean()


def force_clean(clean_mpy_cross):
    _force_clean(clean_mpy_cross)


def build_sdl(_):
    pass


unix.build_sdl = build_sdl
is_arm = os.uname().machine == "arm64"


def submodules():
    if is_arm:
        path = '/opt/homebrew/opt'
    else:
        path = '/usr/local/opt'

    if not os.path.exists(f'{path}/libffi'):
        raise RuntimeError('libffi needs to be installed using Homebrew.')

    ldflags = [f'-L{path}/libffi/lib']
    cflags = [f'-I{path}/libffi/include']

    if not os.path.exists(f'{path}/sdl2'):
        raise RuntimeError('sdl2 needs to be installed using Homebrew.')

    ldflags.append(f'-L{path}/sdl2/lib')
    cflags.append(f'-I{path}/sdl2/include')

    os.environ['LDFLAGS'] = ' '.join(ldflags)
    os.environ['CFLAGS'] = ' '.join(cflags)

    berkeley_db = os.path.abspath('lib/micropython/lib/berkeley-db-1.xx/README')

    if not os.path.exists(berkeley_db):
        return_code, _ = unix.spawn(unix.submodules_cmd)

        if return_code != 0:
            sys.exit(return_code)


def compile(*args):  # NOQA
    _compile(*args)


def mpy_cross():
    _mpy_cross()
