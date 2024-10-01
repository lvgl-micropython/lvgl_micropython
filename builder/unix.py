import os
import sys
import shutil
from . import spawn
from . import generate_manifest
from . import update_mphalport
from argparse import ArgumentParser


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
heap_size = 4194304
sdl_flags = ''


def parse_args(extra_args, lv_cflags, board):
    global heap_size
    global sdl_flags

    unix_argParser = ArgumentParser(prefix_chars='-S')

    unix_argParser.add_argument(
        '--heap-size',
        dest='heap_size',
        help="heap size to use in bytes. Default is 8MiB (8,388,608 bytes). "
             "Must be more than 100KiB (102,104 bytes)",
        default=(2 ** 20) * 8,
        type=int,
        action='store'
    )

    unix_argParser.add_argument(
        'SDL_FLAGS',
        dest='sdl_flags',
        help='flags to pass to the SDL2 compiler',
        default='',
        action='store'
    )
    unix_args, extra_args = unix_argParser.parse_known_args(extra_args)

    if unix_args.heap_size < 102400:
        raise RuntimeError('heap size is too low, must be >= 102,104 bytes')

    heap_size = unix_args.heap_size
    sdl_flags = unix_args.sdl_flags

    return extra_args, lv_cflags, board


variant = None
SCRIPT_PATH = ''


def build_commands(_, extra_args, script_dir, lv_cflags, board):
    global variant

    if board is None:
        board = 'standard'

    variant = board

    unix_cmd.append(f'{script_dir}/lib/micropython/ports/unix')

    if board:
        unix_cmd.append(f'VARIANT={board}')

    unix_cmd.extend([
        f'LV_CFLAGS="{lv_cflags}"',
        f'LV_PORT=unix',
        f'USER_C_MODULES="{script_dir}/ext_mod"',
        (
            '"CFLAGS_EXTRA='
            '-Wno-sign-compare '
            '-Wno-unused-function '
            '-Wno-double-promotion '
            '-Wno-unused-command-line-argument '
            '-Wno-missing-field-initializers"'
            # 'export CPPFLAGS="-I/opt/homebrew/opt/libffi/include"'
        )
    ])

    # unix_cmd.extend(extra_args)

    clean_cmd.extend(unix_cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(unix_cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(unix_cmd[:])
    submodules_cmd[1] = 'submodules'
    return extra_args


def build_manifest(
    target, script_dir, lvgl_api, displays, indevs, frozen_manifest
):
    global SCRIPT_PATH

    SCRIPT_PATH = script_dir

    update_mphalport(target)

    manifest_path = 'lib/micropython/ports/unix/variants/manifest.py'

    generate_manifest(
        script_dir, lvgl_api, manifest_path, displays,
        indevs, frozen_manifest
    )


def force_clean(clean_mpy_cross):
    if clean_mpy_cross:
        cross_clean = mpy_cross_cmd[:]
        cross_clean.insert(1, 'clean')
        spawn(cross_clean)

    spawn(clean_cmd)


def clean():
    path = f'lib/micropython/ports/unix/build-{variant}'
    if not os.path.exists(path):
        return

    for file in os.listdir(path):
        if file == 'SDL':
            continue

        file = os.path.join(path, file)
        try:
            if os.path.isdir(file):
                shutil.rmtree(file)
            else:
                os.remove(file)
        except OSError:
            pass


def _run(c, spinner=False, cmpl=False):
    res, _ = spawn(c, spinner=spinner, cmpl=cmpl)
    if res != 0:
        sys.exit(res)


def build_sdl():
    dst = f'lib/micropython/ports/unix/build-{variant}/SDL'

    if not os.path.exists(dst):
        os.makedirs(dst)
    elif os.path.exists(os.path.join(dst, 'libSDL2.a')):
        return

    cwd = os.getcwd()
    os.chdir(dst)
    cmd_ = [
        [
            f'cmake -DSDL_STATIC=ON -DSDL_SHARED=OFF '
            f'-DCMAKE_BUILD_TYPE=Release {sdl_flags} {SCRIPT_PATH}/lib/SDL'
        ],
        [f'cmake --build . --config Release --parallel {os.cpu_count()}']
    ]

    res, _ = spawn(cmd_, cmpl=True)
    if res != 0:
        sys.exit(res)

    os.chdir(cwd)


def submodules():
    if not os.path.exists('lib/SDL/include'):
        print()
        print('collecting SDL2')
        cmd_ = [
            'git',
            'submodule',
            'update',
            '--init',
            '--',
            f'lib/SDL'
        ]
        _run(cmd_)

    cmd_ = ['cd lib/SDL && git checkout release-2.30.2']
    _run(cmd_)

    return_code, _ = spawn(submodules_cmd)
    if return_code != 0:
        sys.exit(return_code)


def copy_updated_files():
    src_path = 'micropy_updates/unix'
    dst_path = 'lib/micropython/ports/unix'

    for file in os.listdir(src_path):
        src = os.path.join(src_path, file)
        dst = os.path.join(dst_path, file)

        shutil.copyfile(src, dst)


def read_file(path):
    with open(path, 'rb') as f:
        return f.read().decode('utf-8')


def write_file(path, data):
    with open(path, 'wb') as f:
        f.write(data.encode('utf-8'))


def update_makefile():
    makefile_path = 'lib/micropython/ports/unix/Makefile'

    data = read_file(makefile_path)

    if 'machine_timer.c' not in data:
        data = data.replace(
            'SRC_C += \\\n',
            'SRC_C += \\\n\tmachine_timer.c\\\n'
        )

        write_file(makefile_path, data)


def update_modmachine():
    modmachine_path = 'lib/micropython/ports/unix/modmachine.c'

    data = read_file(modmachine_path)

    if 'MICROPY_PY_MACHINE_EXTRA_GLOBALS' not in data:
        data += (
            '\n#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \\\n'
            '    { MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&machine_timer_type) }\n'  # NOQA
        )

        write_file(modmachine_path, data)


def update_main():
    main_path = 'lib/micropython/ports/unix/main.c'

    data = read_file(main_path)

    if 'modmachine.h' not in data:
        data = data.replace(
            '#include "input.h"',
            '#include "input.h"\n#include "modmachine.h"'
        )

    if 'machine_timer_deinit_all()' not in data:
        data = data.replace(
            '#if MICROPY_PY_SYS_ATEXIT',
            'machine_timer_deinit_all();\n    #if MICROPY_PY_SYS_ATEXIT'
        )

    if 'lcd_bus_deinit_sdl()' not in data:
        data = data.replace(
            '#if MICROPY_PY_SYS_ATEXIT',
            'lcd_bus_deinit_sdl();\n    #if MICROPY_PY_SYS_ATEXIT'
        )

    if 'lcd_bus_init_sdl()' not in data:
        data = data.replace(
            'mp_init();',
            'mp_init();\n    lcd_bus_init_sdl();'
        )

    data = data.split('\n')

    for i, line in enumerate(data):
        if line.startswith('long heap_size ='):
            data[i] = f'long heap_size = {heap_size};'
            break

    data = '\n'.join(data)

    write_file(main_path, data)


def update_mpconfigvariant_common():
    mpconfigvariant_common_path = (
        'lib/micropython/ports/unix/variants/mpconfigvariant_common.h'
    )
    data = read_file(mpconfigvariant_common_path)

    if (
        '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)' in
        data
    ):
        data = data.replace(
            '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)',
            '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (0)'
        )

    if '#define MICROPY_MEM_STATS              (1)' in data:
        data = data.replace(
            '#define MICROPY_MEM_STATS              (1)',
            '#define MICROPY_MEM_STATS              (0)'
        )

    macros = (
        '#define MICROPY_SCHEDULER_DEPTH              (128)'
        '#define MICROPY_STACK_CHECK              (0)'
    )

    for macro in macros:
        if macro not in data:
            data += '\n\n'
            data += macro + '\n'

    write_file(mpconfigvariant_common_path, data)


def compile(*args):  # NOQA

    update_makefile()
    update_modmachine()
    update_main()
    update_mpconfigvariant_common()
    copy_updated_files()

    build_sdl()

    cmd_ = compile_cmd[:]
    cmd_.extend(list(args))

    return_code, _ = spawn(cmd_)
    if return_code != 0:
        sys.exit(return_code)

    os.remove('build/lvgl_header.h')

    for f in os.listdir('build'):
        if f.startswith('lvgl'):
            continue

        if f.startswith('SDLPointer'):
            continue

        if f == 'manifest.py' or not f.endswith('.py'):
            os.remove(os.path.join('build', f))

    src = f'lib/micropython/ports/unix/build-{variant}/micropython'
    dst = f'build/lvgl_micropy_unix'
    shutil.copyfile(src, dst)

    print(f'compiled binary is {os.path.abspath(dst)}')


def mpy_cross():
    _cmd = [
        'make -C lib/micropython/mpy-cross'
    ]

    res, _ = spawn(_cmd, cmpl=True)
    if res:
        sys.exit(res)
