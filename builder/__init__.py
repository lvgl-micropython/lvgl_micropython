import sys
import os
import subprocess
import threading
import random
import queue

_windows_env = None


def setup_windows_build():

    global _windows_env

    if _windows_env is None:
        import pyMSVC

        env = pyMSVC.setup_environment()
        print(env)

        for key in os.environ.keys():
            if 'COMNTOOLS' in key:
                version = key.replace('COMNTOOLS', '').replace('VS', '')
                break
        else:
            print(os.environ)
            raise RuntimeError('unable to locate common tools version')

        version = 'v' + version

        msbuild_path = env.visual_c.msbuild_path.lower()
        msbuild_path = msbuild_path.split('msbuild', 1)[0][:-1]

        VCTargetsPath = os.path.join(
            msbuild_path,
            'Msbuild',
            'Microsoft',
            'VC',
            version
            )
        os.environ['VCTargetsPath'] = VCTargetsPath + '\\'

        _windows_env = env

    return _windows_env


def set_mp_version(port):
    mpconfigport = f'lib/micropython/ports/{port}/mpconfigport.h'

    with open(mpconfigport, 'rb') as f:
        data = f.read().decode('utf-8')

    if 'MICROPY_BANNER_NAME_AND_VERSION' not in data:
        data += (
            '\n\n#include "genhdr/mpversion.h"'
            '\n\n#define MICROPY_BANNER_NAME_AND_VERSION "LVGL MicroPython '
            '1.23.0 on " MICROPY_BUILD_DATE\n\n'
        )

        with open(mpconfigport, 'wb') as f:
            f.write(data.encode('utf-8'))


def update_mphalport(target):
    if target == 'esp8266':
        mphalport_path = f'lib/micropython/ports/{target}/esp_mphal.h'
    elif target == 'pic16bit':
        mphalport_path = f'lib/micropython/ports/{target}/pic16bit_mphal.h'
    elif target == 'teensy':
        mphalport_path = f'lib/micropython/ports/{target}/teensy_hal.h'
    elif target == 'macOS':
        mphalport_path = f'lib/micropython/ports/unix/mphalport.h'
    elif target == 'windows':
        mphalport_path = f'lib/micropython/ports/{target}/windows_mphal.h'
    else:
        mphalport_path = f'lib/micropython/ports/{target}/mphalport.h'

    if not os.path.exists(mphalport_path):
        raise RuntimeError(mphalport_path)

    with open(mphalport_path, 'rb') as f:
        data = f.read().decode('utf-8')

    if '#ifndef _MPHALPORT_H_' not in data:
        data = (
            f'#ifndef _MPHALPORT_H_\n'
            f'#define _MPHALPORT_H_\n'
            f'{data}\n'
            f'#endif /* _MPHALPORT_H_ */\n'
        )
        with open(mphalport_path, 'wb') as f:
            f.write(data.encode('utf-8'))


def generate_manifest(
    script_dir, lvgl_api, manifest_path, displays,
    indevs, frozen_manifest, *addl_manifest_files
):
    addl_manifest_files = list(addl_manifest_files)

    if not os.path.exists('build'):
        os.mkdir('build')

    manifest_files = [
        f"include('{os.path.abspath(manifest_path)}')"
    ]

    if frozen_manifest is not None:
        manifest_files.append(f"include('{frozen_manifest}')")

    if lvgl_api:
        api_path = f'{script_dir}/api_drivers/lvgl_api_drivers'
    else:
        api_path = f'{script_dir}/api_drivers/py_api_drivers'

    frozen_manifest_files = [
        f'{api_path}/frozen/display/display_driver_framework.py',
        f'{api_path}/frozen/display/rgb_display_framework.py',
        f'{api_path}/frozen/indev/touch_calibration/touch_cal_data.py',
        f'{api_path}/frozen/indev/touch_calibration/touch_calibrate.py',
        f'{api_path}/frozen/indev/_indev_base.py',
        f'{api_path}/frozen/indev/button_framework.py',
        f'{api_path}/frozen/indev/encoder_framework.py',
        f'{api_path}/frozen/indev/keypad_framework.py',
        f'{api_path}/frozen/indev/pointer_framework.py',
        f'{api_path}/fs_driver.py',
        f'{script_dir}/api_drivers/common_api_drivers/frozen/other/i2c.py',
        (
            f'{script_dir}/api_drivers/common_api_drivers/'
            f'frozen/other/io_expander_framework.py'
        ),
        (
            f'{script_dir}/api_drivers/common_api_drivers/'
            f'frozen/other/task_handler.py'
        )
    ]

    for file in frozen_manifest_files:
        if not os.path.exists(file):
            raise RuntimeError(f'File not found "{file}"')

        print(file)
        file_path, file_name = os.path.split(file)
        entry = f"freeze('{file_path}', '{file_name}')"
        manifest_files.append(entry)

    for file in addl_manifest_files:
        if not os.path.exists(file):
            raise RuntimeError(f'File not found "{file}"')

        print(file)
        file_path, file_name = os.path.split(file)
        entry = f"freeze('{file_path}', '{file_name}')"
        manifest_files.append(entry)

    for file in indevs:
        if not os.path.exists(file):
            tmp_file = (
                f'{script_dir}/api_drivers/common_api_drivers/indev/{file}.py'
            )

            if not os.path.exists(tmp_file):
                raise RuntimeError(f'File not found "{file}"')

            if file.startswith('ft'):
                focaltech_touch = (
                    f'{script_dir}/api_drivers/common_api_drivers/'
                    f'indev/focaltech_touch.py'
                )
                print(focaltech_touch)
                directory, file_name = os.path.split(focaltech_touch)
                entry = f"freeze('{directory}', '{file_name}')"
                manifest_files.append(entry)

            file = tmp_file

        directory, file_name = os.path.split(file)
        extension_file = file_name.rsplit('.')[0] + '_extension.py'
        extension = os.path.join(directory, extension_file)

        if os.path.exists(extension):
            print(extension_file)
            entry = f"freeze('{directory}', '{extension_file}')"
            manifest_files.append(entry)

        print(file)
        file_path, file_name = os.path.split(file)
        entry = f"freeze('{file_path}', '{file_name}')"
        manifest_files.append(entry)

    display_paths = []

    for file in displays:
        if not os.path.exists(file):
            tmp_file = (
                f'{script_dir}/api_drivers/common_api_drivers/display/{file}'
            )

            if not os.path.exists(tmp_file):
                raise RuntimeError(f'Display not found "{file}"')

            display_paths.append(tmp_file)
            for file_name in os.listdir(tmp_file):
                if not file_name.endswith('.py'):
                    continue

                print(file_name)

                entry = f"freeze('{tmp_file}', '{file_name}')"
                manifest_files.append(entry)

        print(file)
        file_path, file_name = os.path.split(file)
        entry = f"freeze('{file_path}', '{file_name}')"
        manifest_files.append(entry)

    manifest_files = '\n'.join(manifest_files)

    with open('build/manifest.py', 'w') as f:
        f.write(manifest_files)

    return display_paths


def get_lvgl():
    cmd_ = [
        'git',
        'submodule',
        'update',
        '--init',
        '--depth=1',
        '--',
        f'lib/lvgl'
    ]
    print()
    print('collecting LVGL')
    print('this might take a while...')
    result, _ = spawn(cmd_, spinner=True)
    if result != 0:
        sys.exit(result)


def get_micropython():

    cmd_ = [
        'git',
        'submodule',
        'update',
        '--init',
        '--depth=1',
        '--',
        f'lib/micropython'
    ]
    print()
    print('collecting MicroPython 1.23.0')
    result, _ = spawn(cmd_, spinner=True)
    if result != 0:
        sys.exit(result)

    mkrules_path = 'lib/micropython/py/mkrules.mk'
    with open(mkrules_path, 'rb') as f:
        data = f.read().decode('utf-8')

    pattern = (
        '$(Q)git submodule update --init '
        '$(addprefix $(TOP)/,$(GIT_SUBMODULES))'
    )
    if pattern in data:
        data = data.replace(
            pattern,
            (
                '$(Q)git submodule update --init --depth=1 '
                '$(addprefix $(TOP)/,$(GIT_SUBMODULES))'
            )
        )
        with open(mkrules_path, 'wb') as f:
            f.write(data.encode('utf-8'))


def get_pycparser():
    cmd_ = [
        'git',
        'submodule',
        'update',
        '--init',
        '--depth=1',
        '--',
        f'lib/pycparser'
    ]
    print()
    print('collecting pycparser')
    result, _ = spawn(cmd_)
    if result != 0:
        sys.exit(result)


def _busy_spinner(evnt, spinner_lock):
    if 'GITHUB_RUN_ID' in os.environ:
        while not evnt.is_set():
            with spinner_lock:
                sys.stdout.write('.')
                sys.stdout.flush()
            evnt.wait(2)

        with spinner_lock:
            sys.stdout.write('\n')
            sys.stdout.flush()
    else:
        count = random.randint(1, 25)
        wait = random.randint(10, 100) * 0.001
        chars = '\\|/-'
        char_index = 0

        with spinner_lock:
            sys.stdout.write(chars[char_index] + '\r')
            sys.stdout.flush()

        while not evnt.is_set():
            evnt.wait(wait)
            count -= 1
            char_index += 1
            if char_index == 4:
                char_index = 0

            with spinner_lock:
                sys.stdout.write(f'{chars[char_index]}\r')
                sys.stdout.flush()

            if count == 0:
                count = random.randint(1, 25)
                wait = random.randint(10, 100) * 0.001

        with spinner_lock:
            sys.stdout.write('\r')
            sys.stdout.flush()


def _convert_line(lne):
    try:
        lne = lne.decode('utf-8')
    except UnicodeDecodeError:
        for char in lne:
            if 32 <= char <= 125 or char in (b'\r', b'\n'):
                continue

            lne = lne.replace(char, b'')
        lne = lne.decode('utf-8')

    return lne


def process_output(myproc, out_to_screen, spinner, cmpl, out_queue):
    line = b''
    err_line = b''
    last_line_len = -1
    line_updated = False
    err_updated = False

    os.set_blocking(myproc.stdout.fileno(), False)
    os.set_blocking(myproc.stderr.fileno(), False)

    event = threading.Event()
    spinner_lock = threading.Lock()
    if spinner:
        t = threading.Thread(target=_busy_spinner, args=(event, spinner_lock))
        t.daemon = True
        t.start()
    else:
        t = None

    while True:
        # --- extract line using read(1)
        out = myproc.stdout.read(1)
        while out:
            line_updated = True
            line += out
            if out == b'\n':
                line = _convert_line(line.strip())
                if not line:
                    line = b''
                    break

                out_queue.put(line)

                if out_to_screen:
                    if (
                        cmpl and (
                            line.startswith('[') or
                            line.startswith('CC ') or
                            line.startswith('MPY ') or
                            (
                                line.startswith('--') and
                                len(line) <= 80
                            )
                        )
                    ):
                        if last_line_len != -1:
                            sys.stdout.write('\r')
                            sys.stdout.flush()

                        if len(line) < last_line_len:
                            padding = ' ' * (last_line_len - len(line))
                        else:
                            padding = ''

                        sys.stdout.write(line + padding)
                        sys.stdout.flush()

                        last_line_len = len(line)
                    else:
                        if last_line_len == -1:
                            if spinner:
                                with spinner_lock:
                                    sys.stdout.write('\r' + line + '\n')
                                    sys.stdout.flush()
                            else:
                                sys.stdout.write(line + '\n')
                                sys.stdout.flush()
                        else:
                            if spinner:
                                with spinner_lock:
                                    sys.stdout.write('\r' + line + '\n')
                                    sys.stdout.flush()
                            else:
                                sys.stdout.write('\n' + line + '\n')
                                sys.stdout.flush()

                            last_line_len = -1

                line = b''

            out = myproc.stdout.read(1)

        out = myproc.stderr.read(1)

        while out:
            err_updated = True
            if not spinner and out_to_screen and cmpl and last_line_len != -1:
                sys.stdout.write('\n')
                sys.stdout.flush()
                last_line_len = -1

            err_line += out
            if out == b'\n':
                err_line = _convert_line(err_line.strip())
                if not err_line:
                    err_line = b''
                    break

                out_queue.put(err_line)
                if out_to_screen:
                    if spinner:
                        with spinner_lock:
                            sys.stderr.write('\r' + err_line + '\n')
                            sys.stdout.flush()
                    else:
                        sys.stderr.write(err_line + '\n')
                        sys.stderr.flush()

                err_line = b''

            out = myproc.stderr.read(1)

        if not err_updated and not line_updated:
            if myproc.poll() is not None:
                break
            event.wait(0.1)

        err_updated = False
        line_updated = False

    if t is not None:
        event.set()
        t.join()

    elif out_to_screen and cmpl and last_line_len != -1:
        sys.stdout.write('\n')
        sys.stdout.flush()


def spawn(cmd_, out_to_screen=True, spinner=False, env=None, cmpl=False):

    if env is None:
        env = os.environ

    if isinstance(cmd_[0], str):
        cmd_ = [cmd_[:]]

    cmd_ = list(' '.join(c) for c in cmd_)

    que = queue.Queue()

    p = subprocess.Popen(
        'bash',
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
        shell=True,
        env=env
    )

    os.set_blocking(p.stdout.fileno(), False)
    os.set_blocking(p.stderr.fileno(), False)

    proc_thread = threading.Thread(
        target=process_output,
        args=(p, out_to_screen, spinner, cmpl, que)
    )

    proc_thread.start()
    output_buffer = []

    while cmd_:
        item = cmd_.pop(0)
        # if sys.platform.startswith('darwin'):
        #     if item.startswith('make '):
        #         item = 'g' + item
        #
        #     if ' make ' in item:
        #         item = item.replace(' make ', ' gmake ')

        if 'GITHUB_RUN_ID' in os.environ:
            print(item)

        p.stdin.write(item.encode('utf-8') + b'\n')

    p.stdin.close()

    while proc_thread and proc_thread.is_alive():  # wait for thread to finish
        try:
            line = que.get_nowait()  # or q.get(timeout=.1)
            output_buffer.append(line)
        except queue.Empty:
            pass
        try:
            proc_thread.join(0.025)
        except:  # NOQA
            break

    while True:
        try:
            line = que.get_nowait()  # or q.get(timeout=.1)
            output_buffer.append(line)
        except queue.Empty:
            break

    if not p.stdout.closed:
        p.stdout.close()

    if not p.stderr.closed:
        p.stderr.close()

    output_buffer = '\n'.join(output_buffer)

    if out_to_screen:
        if spinner:
            print(output_buffer)
        elif cmpl:
            sys.stdout.write('\n')
            sys.stdout.flush()

    return p.returncode, output_buffer


def force_clean(clean_mpy_cross):
    if clean_mpy_cross:
        cross_clean = mpy_cross_cmd[:]
        cross_clean.insert(1, 'clean')
        spawn(cross_clean)

    spawn(clean_cmd)


def clean():
    force_clean(False)


def submodules():
    return_code, _ = spawn(submodules_cmd)
    if return_code != 0:
        sys.exit(return_code)


def compile(*args):  # NOQA
    return_code, _ = spawn(compile_cmd)
    if return_code != 0:
        sys.exit(return_code)


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd, cmpl=True)
    if return_code != 0:
        sys.exit(return_code)


def build_manifest(
    target, script_dir, lvgl_api, displays, indevs, frozen_manifest
):
    update_mphalport(target)
    if target == 'teensy':
        manifest_path = f'lib/micropython/ports/{target}/manifest.py'
    else:
        manifest_path = f'lib/micropython/ports/{target}/boards/manifest.py'
    if not os.path.exists(manifest_path):
        raise RuntimeError(f'Unable to locate manifest file "{manifest_path}"')

    generate_manifest(
        script_dir, lvgl_api, manifest_path,
        displays, indevs, frozen_manifest
    )


def parse_args(extra_args, lv_cflags, board):
    return extra_args, lv_cflags, board


mpy_cross_cmd = ['make', '-C', 'lib/micropython/mpy-cross']
cmd = [
    'make',
    '',
    f'-j {os.cpu_count()}',
    '-C'
]
clean_cmd = []
compile_cmd = []
submodules_cmd = []


def build_commands(target, extra_args, script_dir, lv_cflags, board):
    if target == 'samd':
        if lv_cflags:
            lv_cflags += ' -DLV_USE_TINY_TTF=0'
        else:
            lv_cflags = '-DLV_USE_TINY_TTF=0'

    cmd.extend([
        f'lib/micropython/ports/{target}',
        f'LV_PORT={target}',
    ])

    if lv_cflags:
        cmd.append(f'LV_CFLAGS="{lv_cflags}"')

    if board is not None:
        if lv_cflags:
            cmd.insert(5, f'BOARD={board}')
        else:
            cmd.append(f'BOARD={board}')

    cmd.append(f'USER_C_MODULES={script_dir}/ext_mod')
    cmd.extend(extra_args)

    clean_cmd.extend(cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(cmd[:])
    submodules_cmd[1] = 'submodules'
