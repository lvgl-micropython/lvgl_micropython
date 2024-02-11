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
    'lib/micropython/ports/unix'
]

compile_cmd = []
submodules_cmd = []


def parse_args(extra_args, lv_cflags, board):
    return extra_args, lv_cflags, board


def build_commands(_, extra_args, script_dir, lv_cflags, __):
    if lv_cflags is not None:
        lv_cflags += ' -DMICROPY_SDL=1'
    else:
        lv_cflags = '-DMICROPY_SDL=1'

    mp_bus_flags = [
        '-DSDL_INCLUDE_PATH=<SDL2/SDL.h>',
        '-DSDL_THREAD_INCLUDE_PATH=<SDL2/SDL_thread.h>',
        '-DSDL_IMAGE_INCLUDE_PATH=<SDL2/SDL_image.h>',
    ]

    mp_bus_flags = ' '.join(mp_bus_flags)

    unix_cmd.extend([
        f'MP_BUS_CFLAGS="{mp_bus_flags}"',
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


def build_manifest(target, script_dir, frozen_manifest):
    update_mphalport(target)

    manifest_path = 'lib/micropython/ports/unix/variants/manifest.py'

    manifest_files = [
        f'{script_dir}/driver/fs_driver.py',
        f'{script_dir}/driver/linux/lv_timer.py',
    ]
    generate_manifest(script_dir, manifest_path, frozen_manifest, *manifest_files)


def clean():
    spawn(clean_cmd)


def submodules():
    if not sys.platform.startswith('linux'):
        raise RuntimeError('Compiling for unix can only be done from Linux')

    release_files = [
        os.path.join('/etc', file)
        for file in os.listdir('/etc')
        if file.endswith('-release')
    ]

    if '/etc/os-release' in release_files:
        release_files.remove('/etc/os-release')
        release_files.insert(0, '/etc/os-release')

    dist_id = None

    for file in release_files:
        with open(file, 'r') as f:
            os_data = f.read().split('\n')

        for line in os_data:
            if line.startswith('ID='):
                dist_id = line.split('=')[-1].replace('"', '')

        if dist_id is not None:
            break

    if dist_id is not None:
        mapping = {
            ('amzn', 'centos', 'fedora', 'rhel'): {
                'package_name': 'SDL2-devel',
                'list': ['yum' 'list' 'installed'],
                'install': ['yum', 'install'],
                },
            ('arch',): {
                'package_name': 'sdl2',
                'list': ['pacman' '-Qe'],
                'install': ['pacman']
            },
            ('debian', 'ubuntu', 'kali', 'raspbian'): {
                'package_name': 'libsdl2-dev',
                'list': ['apt', 'list', '--installed'],
                'install': ['apt', 'install']
            },
            ('fedora',): {
                'package_name': 'SDL2-devel',
                'list': ['dnf' 'list' 'installed'],
                'install': ['dnf', 'install'],
            },
            ('opensuse', 'sles'): {
                'package_name': 'SDL2-devel',
                'list': ['zypper' 'search' '-i'],
                'install': ['zypper', 'install'],
            },
            ('slackware',): {
                'package_name': 'SDL2',
                'list': ['ls', '-l', '/var/log/packages/'],
                'install': ['slackpkg', 'install'],
            }
        }

        values = []

        for key, value in mapping.items():
            if dist_id in key:
                values.append(value)

        for value in values:
            ret_code, result = spawn(value['list'], out_to_screen=False)
            if ret_code != 0:
                continue

            if value['package_name'] not in result:
                value['install'].append(value['package_name'])
                ret_code, _ = spawn(value['install'])
                if ret_code != 0:
                    print(
                        'In order to install SDL2 you '
                        'need to do one of the following:'
                    )
                    print('    Run this script again as a super user')
                    print(
                        '    Manually install the package '
                        'before running this script'
                    )
                    sys.exit(ret_code)

            break

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


def mpy_cross():
    pass

