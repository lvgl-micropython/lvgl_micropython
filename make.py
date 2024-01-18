import os
import sys
import builder

from argparse import ArgumentParser


SCRIPT_DIR = os.path.abspath(os.path.dirname(sys.argv[0]))
MPY_DIR = os.path.join(SCRIPT_DIR, 'micropython')

argParser = ArgumentParser(prefix_chars='-')
argParser.add_argument(
    'target',
    help='build target',
    choices=[
        'esp32', 'windows', 'stm32', 'unix', 'rp2', 'esp8266', 'teensy',
        'teensy', 'zephyr', 'renesas-ra', 'nrf', 'mimxrt', 'samd',
        'webassembly'
    ],
    action='store',
    nargs=1
)


args1, extra_args = argParser.parse_known_args(sys.argv[1:])
argParser = ArgumentParser(prefix_chars='mcsLBF')

argParser.add_argument(
    'mpy_cross',
    dest='mpy_cross',
    help='compile mpy_cross',
    action='store_true'
)
argParser.add_argument(
    'submodules',
    dest='submodules',
    help='build submodules',
    action='store_true'
)

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

args2, extra_args = argParser.parse_known_args(extra_args)

target = args1.target[0]
lv_cflags = args2.lv_cflags
clean = args2.clean
submodules = args2.submodules
mpy_cross = args2.mpy_cross
board = args2.board
frozen_manifest = args2.frozen_manifest

extra_args.append(f'FROZEN_MANIFEST="{SCRIPT_DIR}/build/manifest.py"')

if lv_cflags is not None:
    lv_cflags = lv_cflags.replace('"', '')


def get_submodules():
    # if not os.path.exists(os.path.join(SCRIPT_DIR, 'micropython/mpy-cross')):
    #     builder.get_micropython()
    # if not os.path.exists(os.path.join(SCRIPT_DIR, 'lvgl/lvgl.h')):
    #     builder.get_lvgl()
    # if not os.path.exists(os.path.join(SCRIPT_DIR, 'libs/pycparser/pycparser')):
    #     builder.get_pycparser()
    pass


if __name__ == '__main__':
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
    else:
        import builder as mod

    extra_args, lv_cflags, board = mod.parse_args(extra_args, lv_cflags, board)
    mod.build_commands(target, extra_args, SCRIPT_DIR, lv_cflags, board)

    if submodules:
        get_submodules()
        mod.submodules()

    if mpy_cross:
        mod.mpy_cross()

    if clean:
        mod.clean()

    mod.build_manifest(target, SCRIPT_DIR, frozen_manifest)

    mod.compile()

