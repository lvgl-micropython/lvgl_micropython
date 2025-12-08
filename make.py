#!/usr/bin/env python3
# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import sys
import builder
import shutil

from argparse import ArgumentParser

if sys.platform.startswith('win'):
    raise RuntimeError('compiling on windows is not supported at this time')

SCRIPT_DIR = os.path.abspath(os.path.dirname(sys.argv[0]))
MPY_DIR = os.path.join(SCRIPT_DIR, 'micropython')


argParser = ArgumentParser(prefix_chars='-')
argParser.add_argument(
    '--custom-board-path',
    dest='custom_board_path',
    default=None,
    action='store'
)
args, extra_args = argParser.parse_known_args(sys.argv[1:])

custom_board_path = args.custom_board_path

if custom_board_path is not None:
    if not os.path.exists(custom_board_path):
        raise RuntimeError(
            'Supplied custom board path does not exist.'
        )

    has_toml = False
    for file in os.listdir(custom_board_path):
        if file.endswith('.toml'):
            extra_args.insert(0, f'--toml={os.path.join(custom_board_path, file)}')
            break


argParser = ArgumentParser(prefix_chars='-')
argParser.add_argument(
    '--toml',
    dest='toml',
    help='use a toml file to setup the build.',
    action='store',
    default=None
)
args, extra_args = argParser.parse_known_args(extra_args)


if args.toml is not None:
    toml_path = args.toml
    build_path = os.path.join(SCRIPT_DIR, 'build')
    if not os.path.exists(build_path):
        os.mkdir(build_path)

    driver_out_path = os.path.join(build_path, 'display.py')

    from builder import toml_reader

    build_command = toml_reader.run(toml_path, driver_out_path)

    for arg in extra_args:
        if arg not in build_command:
            build_command.append(arg)

    extra_args = build_command

argParser = ArgumentParser(prefix_chars='-')
argParser.add_argument(
    'target',
    help='build target',
    choices=[
        'esp32', 'windows', 'stm32', 'unix', 'rp2',
        'renesas-ra', 'nrf', 'mimxrt', 'samd', 'macOS', 'raspberry_pi'
    ],
    action='store',
    nargs=1
)


args1, extra_args = argParser.parse_known_args(extra_args)
target = args1.target[0]


argParser = ArgumentParser(prefix_chars='-mscLBFDIVE')

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

if target in ('windows', 'unix', 'macOS', 'raspberry_pi'):
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
        'Display name is the name of the directory under '
        'api_drivers/common_api_drivers/display'
    ),
    action='append',
    default=[]
)

argParser.add_argument(
    'INDEV',
    dest='indevs',
    help=(
        'indev ic model or path (absolue) to indev driver. '
        'Indev name is the name of the source file under '
        'api_drivers/common_api_drivers/indev (without the ".py")'
    ),
    action='append',
    default=[]
)

argParser.add_argument(
    'EXPANDER',
    dest='expanders',
    help=(
        'io expander ic model or path (absolute) to an io expander driver. '
        'The model is the name of the source file under '
        'api_drivers/common_api_drivers/io_expander (without the ".py")'
    ),
    action='append',
    default=[]
)

argParser.add_argument(
    'IMU',
    dest='imus',
    help=(
        'imu/mems sensor ic model or path (absolute) to an imu/mems sensor '
        'driver. The model is the name of the source file under '
        'api_drivers/common_api_drivers/imu_sensor (without the ".py")'
    ),
    action='append',
    default=[]
)

argParser.add_argument(
    '--no-scrub',
    dest='no_scrub',
    default=False,
    action='store_true'
)


args2, extra_args = argParser.parse_known_args(extra_args)

lv_cflags = args2.lv_cflags
clean = args2.clean
board = args2.board
frozen_manifest = args2.frozen_manifest
displays = args2.displays
indevs = args2.indevs
expanders = args2.expanders
imus = args2.imus
builder.DO_NOT_SCRUB_BUILD_FOLDER = args2.no_scrub

if imus:
    os.environ['FUSION'] = "1"


if frozen_manifest is not None:
    frozen_manifest = os.path.abspath(frozen_manifest)
    if not os.path.exists(frozen_manifest):
        raise RuntimeError('Frozen manifest does not exist.')


if lv_cflags is None:
    lv_cflags = ''


extra_args.append(f'FROZEN_MANIFEST="{SCRIPT_DIR}/build/manifest.py"')
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

    if 'EXTRA_CFLAGS' not in os.environ:
        os.environ['EXTRA_CFLAGS'] = f'-Wno-unterminated-string-initialization'
    elif '-Wno-unterminated-string-initialization' not in os.environ['EXTRA_CFLAGS']:
        os.environ['EXTRA_CFLAGS'] += f' -Wno-unterminated-string-initialization'

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
    elif target.lower() == 'raspberry_pi':
        from builder import raspberry_pi as mod
        target = 'unix'
    elif target.lower() == 'windows':
        from builder import windows as mod
    else:
        import builder as mod

    get_submodules()

    # Custom card management (classic format without TOML)
    if custom_board_path is not None and args.toml is None:
        from pathlib import Path

        custom_board_path = Path(custom_board_path).resolve()  # normalize
        board_name = custom_board_path.name
        dst_path = f"lib/micropython/ports/{target}/boards/{board_name}"

        import re
        errors = []
        board_dir = Path(custom_board_path)
        
        # Partition file will be checked further
        required_files = {
            "board.json": board_dir / "board.json",
            "mpconfigboard.h": board_dir / "mpconfigboard.h", 
            "mpconfigboard.cmake": board_dir / "mpconfigboard.cmake",
            "sdkconfig.board": board_dir / "sdkconfig.board"
        }
        
        missing_files = [name for name, path in required_files.items() if not path.exists()]
        errors.extend(f"Missing file: {name}" for name in missing_files)
        
        partitions_file = ''
        partitions_path = ''

        if "sdkconfig.board" not in missing_files:
            sdkconfig_path = required_files["sdkconfig.board"]
            content = sdkconfig_path.read_text()
            
            checks = [
                (r'CONFIG_PARTITION_TABLE_CUSTOM=y', "Missing CONFIG_PARTITION_TABLE_CUSTOM=y"),
                (r'CONFIG_ESPTOOLPY_FLASHSIZE_\d+MB=y', "No CONFIG_ESPTOOLPY_FLASHSIZE_*MB=y found"),
            ]
            
            for pattern, error_msg in checks:
                if not re.search(pattern, content):
                    errors.append(error_msg)
            
            match = re.search(r'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME\s*=\s*"([^"]+)"', content)
            if match:
                partitions_file = Path(match.group(1)).name
                partitions_path = board_dir / partitions_file
                
                if not partitions_path.exists():
                    errors.append(f"Declared partitions file '{partitions_file}' not found")
                
            else:
                errors.append("CONFIG_PARTITION_TABLE_CUSTOM_FILENAME not found")
        
        if errors:
            error_lines = "\n".join(f"  • {e}" for e in errors)
            raise RuntimeError(f"Invalid board '{board_name}':\n{error_lines}")
        
        print(f"✓ Valid custom board: {board_name}")
        
       # Move the partition table file to "ports/esp32/"" for proper MicroPython integration.

        esp32_port_dir = Path(f"lib/micropython/ports/{target}")
        target_partitions_path = esp32_port_dir / partitions_file
        
        esp32_port_dir.mkdir(parents=True, exist_ok=True)
        
        shutil.copy2(partitions_path, target_partitions_path)
        print(f"✓ Copied partitions file to: {target_partitions_path}")

        dst_path = Path(f"lib/micropython/ports/{target}/boards/{board_name}")
        dst_path.parent.mkdir(parents=True, exist_ok=True)
        
        if dst_path.exists():
            shutil.rmtree(dst_path)
        
        shutil.copytree(board_dir, dst_path)
        
        final_board_name = board
        if board is None:
            board = board_name
        elif board != board_name:
            print(f"Warning: Board name '{board}' differs from folder name '{board_name}'")
            new_dst = Path(f"lib/micropython/ports/{target}/boards/{board}")
            if dst_path.exists():
                shutil.rmtree(dst_path)
            shutil.copytree(board_dir, new_dst)
            final_board_name = board

    mod.custom_board_path = custom_board_path

    extra_args, lv_cflags, board = mod.parse_args(extra_args, lv_cflags, board)
    extra_args = mod.build_commands(
        target, extra_args, SCRIPT_DIR, lv_cflags, board)

    if clean:
        print('Cleaning build....')
        mod.force_clean(True)
    else:
        mod.clean()

    if not os.path.exists('lib/micropython/mpy_cross/build/mpy-cross'):
        print('Compiling mpy-cross....')
        mod.mpy_cross()

    mod.submodules()

    print('Generating build files....')
    set_mp_version(target.lower())

    mod.build_manifest(
        target, SCRIPT_DIR, False, displays,
        indevs, expanders, imus, frozen_manifest
    )

    create_lvgl_header()

    print('Compiling....')
    mod.compile(*extra_args)
