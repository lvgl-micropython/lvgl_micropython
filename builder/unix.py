import os
import sys
from . import spawn
from . import generate_manifest
from . import update_mphalport


mpy_cross_cmd = []
clean_cmd = []
unix_cmd = [
    'make',
    '',
    f'-j {os.cpu_count()}',
    '-C',
]

compile_cmd = []
submodules_cmd = []


def parse_args(extra_args, lv_cflags, board):
    return extra_args, lv_cflags, board


variant = None


def build_commands(_, extra_args, script_dir, lv_cflags, board):
    global variant
    variant = board

    if lv_cflags is not None:
        lv_cflags += ' -DMICROPY_SDL=1'
    else:
        lv_cflags = '-DMICROPY_SDL=1'

    unix_cmd.append(f'{script_dir}/lib/micropython/ports/unix')

    if board:
        unix_cmd.append(f'VARIANT={board}')

    unix_cmd.extend([
        f'LV_CFLAGS="{lv_cflags}"',
        f'LV_PORT=unix',
        f'USER_C_MODULES="{script_dir}/ext_mod"'
    ])
    unix_cmd.extend(extra_args)

    clean_cmd.extend(unix_cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(unix_cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(unix_cmd[:])
    submodules_cmd[1] = 'submodules'


def build_manifest(target, script_dir, lvgl_api, displays, indevs, frozen_manifest):
    update_mphalport(target)

    manifest_path = 'lib/micropython/ports/unix/variants/manifest.py'

    manifest_files = [
        f'{script_dir}/api_drivers/common_api_drivers/linux/lv_timer.py'
    ]
    generate_manifest(script_dir, lvgl_api, manifest_path, displays, indevs, frozen_manifest, *manifest_files)


def clean():
    spawn(clean_cmd)


def build_sdl():
    cmd_ = [
        'git',
        'submodule',
        'update',
        '--init',
        '--',
        f'lib/SDL'
    ]

    result, _ = spawn(cmd_, spinner=True)
    if result != 0:
        sys.exit(result)

    def _run(c):
        res, _ = spawn(c)
        if res != 0:
            sys.exit(result)

    cmd_ = ['cd lib/SDL && git checkout release-2.30.x']
    _run(cmd_)

    cmd_ = ['cd lib/SDL && mkdir build']
    _run(cmd_)

    cmd_ = [' '.join([
        'apt-get --yes install'
        'cmake',
        'ninja-build',
        'gnome-desktop-testing',
        'libasound2-dev',
        'libpulse-dev',
        'libaudio-dev',
        'libjack-dev',
        'libsndio-dev',
        'libx11-dev',
        'libxext-dev',
        'libxrandr-dev',
        'libxcursor-dev',
        'libxfixes-dev',
        'libxi-dev',
        'libxss-dev',
        'libxkbcommon-dev',
        'libdrm-dev',
        'libgbm-dev',
        'libgl1-mesa-dev',
        'libgles2-mesa-dev',
        'libegl1-mesa-dev',
        'libdbus-1-dev',
        'libibus-1.0-dev',
        'libudev-dev',
        'fcitx-libs-dev',
        'libpipewire-0.3-dev',
        'libwayland-dev',
        'libdecor-0-dev'
    ])]
    _run(cmd_)

    cmd_ = [
        'cd lib/SDL/build &&'
        'cmake .. -DCMAKE_BUILD_TYPE=Release &&'
        'cmake --build . --config Release --parallel'
    ]
    _run(cmd_)


def submodules():
    if not sys.platform.startswith('linux'):
        raise RuntimeError('Compiling for unix can only be done from Linux')

    if not os.path.exists('lib/SDL/src'):
        build_sdl()

    return_code, _ = spawn(submodules_cmd)
    if return_code != 0:
        sys.exit(return_code)


def compile():  # NOQA
    mpconfigvariant_common_path = (
        'lib/micropython/ports/unix/variants/mpconfigvariant_common.h'
    )

    with open(mpconfigvariant_common_path, 'r') as f:
        mpconfigvariant_common = f.read()

    if (
        '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)' in
        mpconfigvariant_common
    ):
        mpconfigvariant_common = mpconfigvariant_common.replace(
            '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)',
            '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (0)'
        )

        with open(mpconfigvariant_common_path, 'w') as f:
            f.write(mpconfigvariant_common)

    if '#define MICROPY_MEM_STATS              (1)' in mpconfigvariant_common:
        mpconfigvariant_common = mpconfigvariant_common.replace(
            '#define MICROPY_MEM_STATS              (1)',
            '#define MICROPY_MEM_STATS              (0)'
        )

        with open(mpconfigvariant_common_path, 'w') as f:
            f.write(mpconfigvariant_common)

    return_code, _ = spawn(compile_cmd)
    if return_code != 0:
        sys.exit(return_code)

    global variant

    if variant is None:
        variant = 'build-standard'

    src = 'lib/SDL/build'
    dst = f'lib/micropython/ports/unix/{variant}'

    import shutil

    def _iter_path(s):
        for file_name in os.listdir():
            file = os.path.join(s, file_name)
            if os.path.isdir(file):
                _iter_path(file)
            elif not file_name.endswith('.so'):
                continue

            d = os.path.join(dst, file_name)
            print('copying file:', file, '--->', d)
            shutil.copyfile(file, d)

    _iter_path(src)


def mpy_cross():
    pass

