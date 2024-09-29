#!/usr/bin/env python3

import os
import sys
import builder

from argparse import ArgumentParser

if sys.platform.startswith('win'):
    raise RuntimeError('compiling on windows is not supported at this time')

SCRIPT_DIR = os.path.abspath(os.path.dirname(sys.argv[0]))
MPY_DIR = os.path.join(SCRIPT_DIR, 'micropython')

argParser = ArgumentParser(prefix_chars='-')
argParser.add_argument(
    'target',
    help='build target',
    choices=[
        'esp32', 'windows', 'stm32', 'unix', 'rp2',
        'renesas-ra', 'nrf', 'mimxrt', 'samd', 'macOS'
    ],
    action='store',
    nargs=1
)


args1, extra_args = argParser.parse_known_args(sys.argv[1:])
target = args1.target[0]

argParser = ArgumentParser(prefix_chars='mscLBFDIV')

argParser.add_argument(
    'clean',
    dest='clean',
    help='clean the build',
    action='store_true'
)

argParser.add_argument(
    'LV_CFLAGS',
    dest='lv_cflags',
    help='additional flags that gets passed to the compiler for LVGL',
    action='store',
    default=None
)

if target in ('windows', 'unix'):
    argParser.add_argument(
        'VARIANT',
        dest='board',
        help='optional, board to compile for.',
        action='store',
        default=None
    )
else:
    argParser.add_argument(
        'BOARD',
        dest='board',
        help='optional, board to compile for.',
        action='store',
        default=None
    )

argParser.add_argument(
    'FROZEN_MANIFEST',
    dest='frozen_manifest',
    help='path to manifest file',
    action='store',
    default=None
)

argParser.add_argument(
    'DISPLAY',
    dest='displays',
    help=(
        'display name or path (absolute) to display driver. '
        'Display name is the name of the source file under '
        'driver/display (without the ".py")'
    ),
    action='append',
    default=[]
)

argParser.add_argument(
    'INDEV',
    dest='indevs',
    help=(
        'indev device name or path (absolue) to indev driver. '
        'Indev name is the name of the source file under '
        'driver/indev (without the ".py")'
    ),
    action='append',
    default=[]
)

args2, extra_args = argParser.parse_known_args(extra_args)

lv_cflags = args2.lv_cflags
clean = args2.clean
board = args2.board
frozen_manifest = args2.frozen_manifest
displays = args2.displays
indevs = args2.indevs


if lv_cflags is None:
    lv_cflags = ''


argParser = ArgumentParser(prefix_chars='-')
argParser.add_argument(
    '--LVGL_API',
    dest='lvgl_api',
    help=(
        'Sets the API to be used. If this flag gets set '
        'then the api that is used is the same as what the C API is for LVGL.'
    ),
    action='store_true',
    default=False
)

args3, extra_args = argParser.parse_known_args(extra_args)

lvgl_api = args3.lvgl_api

extra_args.append(f'FROZEN_MANIFEST="{SCRIPT_DIR}/build/manifest.py"')

if lvgl_api:
    extra_args.append(f'GEN_SCRIPT=lvgl')
else:
    extra_args.append(f'GEN_SCRIPT=python')


if lv_cflags is not None:
    lv_cflags = lv_cflags.replace('"', '')


def get_submodules():

    if not os.path.exists(
        os.path.join(SCRIPT_DIR, 'lib/micropython/mpy-cross')
    ):
        builder.get_micropython()
    if not os.path.exists(
        os.path.join(SCRIPT_DIR, 'lib/lvgl/lvgl.h')
    ):
        builder.get_lvgl()
    if not os.path.exists(os.path.join(
        SCRIPT_DIR, 'lib/pycparser/pycparser')
    ):
        builder.get_pycparser()


def create_lvgl_header():
    header_path = f'{SCRIPT_DIR}/build/lvgl_header.h'

    with open(header_path, 'w') as f:
        f.write(
            f'#include "{SCRIPT_DIR}/lib/lvgl/lvgl.h"\n'
        )
        f.write(
            f'#include "{SCRIPT_DIR}/lib/lvgl/src/lvgl_private.h"\n'
        )


if __name__ == '__main__':

    from builder import set_mp_version

    if sys.platform.startswith('win'):
        from builder import setup_windows_build

        setup_windows_build()

    if target.lower() == 'esp32':
        from builder import esp32 as mod
    elif target.lower() == 'unix':
        from builder import unix as mod
    elif target.lower() == 'rp2':
        from builder import rp2 as mod
    elif target.lower() == 'stm32':
        from builder import stm32 as mod
    elif target.lower() == 'nrf':
        from builder import nrf as mod
    elif target.lower() == 'renesas-ra':
        from builder import renesas as mod
    elif target.lower() == 'macos':
        from builder import macOS as mod
        target = 'unix'
    elif target.lower() == 'windows':
        from builder import windows as mod
    else:
        import builder as mod

    extra_args, lv_cflags, board = mod.parse_args(extra_args, lv_cflags, board)
    extra_args = mod.build_commands(target, extra_args, SCRIPT_DIR, lv_cflags, board)

    if clean:
        print('Cleaning build....')
        mod.force_clean(True)
    else:
        mod.clean()

    get_submodules()

    mod.submodules()

    if not os.path.exists('lib/micropython/mpy_cross/build/mpy-cross'):
        print('Compiling mpy-cross....')
        mod.mpy_cross()

    print('Generating build files....')
    set_mp_version(target.lower())

    mod.build_manifest(
        target, SCRIPT_DIR, lvgl_api, displays, indevs, frozen_manifest
    )
    create_lvgl_header()

    print('Compiling....')
    mod.compile(*extra_args)
