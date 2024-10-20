import os
import shutil
from .unix import (
    parse_args as _parse_args,
    build_commands as _build_commands,
    build_manifest as _build_manifest,
    force_clean as _force_clean,
    clean as _clean,
    build_sdl as _build_sdl,
    submodules as _submodules,
    compile as _compile,
    mpy_cross as _mpy_cross
)


from . import unix


unix.REAL_PORT = 'macOS'

if not os.path.exists('micropy_updates/originals/macOS'):
    os.makedirs('micropy_updates/originals/macOS')


def parse_args(extra_args, lv_cflags, board):
    return _parse_args(extra_args, lv_cflags, board)


def build_commands(not_sure, extra_args, script_dir, lv_cflags, board):
    return _build_commands(not_sure, extra_args, script_dir, lv_cflags, board)


def build_manifest(not_sure, script_dir, lvgl_api, displays, indevs, frozen_manifest):
    _build_manifest(not_sure, script_dir, lvgl_api, displays, indevs, frozen_manifest)


def clean():
    _clean()


def force_clean(clean_mpy_cross):
    _force_clean(clean_mpy_cross)


def build_sdl(addl_commands):
    dst = f'lib/micropython/ports/unix/build-{unix.variant}/SDL'

    if os.path.exists(os.path.join(dst, 'libSDL2-2.0.0.dylib')):
        return

    _build_sdl(addl_commands.strip())


unix.build_sdl = build_sdl


def submodules():
    _submodules()


def compile(*args):  # NOQA
    _compile(*args)

    src = f'lib/micropython/ports/unix/build-{unix.variant}/SDL/libSDL2-2.0.0.dylib'
    dst = f'build/libSDL2-2.0.0.dylib'
    if os.path.exists(src):
        shutil.copyfile(src, dst)


def mpy_cross():
    _mpy_cross()


