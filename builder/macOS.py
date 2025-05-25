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


def is_homebrew_arm(cmd):
    ret_val, output = spawn(cmd, out_to_screen=False)
    if ret_val:
        if cmd[0][0] == '/opt/homebrew/bin/brew':
            raise RuntimeError('Unable to locate homebrew installation')

        return is_homebrew_arm([['/opt/homebrew/bin/brew', 'config']])

    data = {line.split(':', 1)[0].strip(): line.split(':', 1)[1].strip() for
            line in output.split('\n')}

    if 'macOS' not in data:
        raise RuntimeError('Unable to determine Homebrew CPU type')

    if 'arm64' in data['macOS']:
        if 'Rosetta 2' not in data:
            if cmd[0][0] != '/opt/homebrew/bin/brew':
                return is_homebrew_arm([['/opt/homebrew/bin/brew', 'config']])

            raise RuntimeError('Unable to determine Homebrew platform')

        if data['Rosetta 2'] == 'true':
            if cmd[0][0] != '/opt/homebrew/bin/brew':
                return is_homebrew_arm([['/opt/homebrew/bin/brew', 'config']])

            raise RuntimeError('Unable to locate Homebrew for Arm processors.')

        return True, cmd[0][0]

    return False, cmd[0][0]


def submodules():
    is_arm, brew_path = is_homebrew_arm([['brew', 'config']])

    if is_arm:
        ret, out = spawn([[brew_path, 'info', 'libffi']], out_to_screen=False)
        if ret:
            print(out)
            sys.exit(ret)

        if 'Installed\n' not in out:
            print(out)
            raise RuntimeError('libffi is not installed')

        out = out.split('Installed\n', 1)[-1]
        alt_path = out.split('(', 1)[0].strip().split('Cellar', 1)[0]

        if 'export LDFLAGS=' in out:
            out = out.split('export LDFLAGS="', 1)[-1]
            ldflags = out.split('"', 1)[0]
        else:
            ldflags = f'"-L{alt_path}opt/libffi/lib"'

        if 'export CPPFLAGS=' in out:
            out = out.split('export CPPFLAGS="', 1)[-1]
            cflags = out.split('"', 1)[0]
        else:
            cflags = f'"-I{alt_path}opt/libffi/include"'

        ret, out = spawn([[brew_path, 'info', 'sdl2']], out_to_screen=False)

        if ret:
            print(out)
            sys.exit(ret)

        if 'Installed\n' not in out:
            print(out)
            raise RuntimeError('sdl2 is not installed')

        ldflags += f' "-L{alt_path}lib"'
        cflags += f' "-I{alt_path}include"'

        os.environ['LDFLAGS'] = f'{ldflags}'
        os.environ['CFLAGS'] = f'{cflags}'

    berkeley_db = os.path.abspath('lib/micropython/lib/berkeley-db-1.xx/README')

    if not os.path.exists(berkeley_db):
        return_code, _ = unix.spawn(unix.submodules_cmd)

        if return_code != 0:
            sys.exit(return_code)


def compile(*args):  # NOQA
    _compile(*args)


def mpy_cross():
    _mpy_cross()
