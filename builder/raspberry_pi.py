# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os

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


if not os.path.exists('micropy_updates/originals/raspberry_pi'):
    os.makedirs('micropy_updates/originals/raspberry_pi')


unix.REAL_PORT = 'raspberry_pi'


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


def build_sdl(addl_commands):
    if addl_commands.startswith('"') and addl_commands.endswith('"'):
        addl_commands = addl_commands[1:-1]

    if has_neon():
        addl_commands += ' CFLAGS="-mfpu=neon"'

    _build_sdl(addl_commands.strip())


unix.build_sdl = build_sdl


def submodules():
    _submodules()


def compile(*args):  # NOQA
    _compile(*args)


def mpy_cross():
    _mpy_cross()


def has_neon():
    """
    Features : half thumb fastmult vfp edsp neon vfpv3
    tls vfpv4 idiva idivt vfpd32 lpae evtstrm crc32
    """

    res = False

    with os.popen('cat /proc/cpuinfo') as file:
        for line in file:
            if not line.startswith('Features'):
                continue

            res = ' neon ' in line

            if res:
                break

    return res
