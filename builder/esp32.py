import os
import sys
from argparse import ArgumentParser
from . import spawn
from . import generate_manifest
from . import update_mphalport


def get_partition_file_name(otp):
    if 'Running cmake in directory ' in otp:
        build_path = otp.split('Running cmake in directory ', 1)[-1]
    else:
        build_path = otp.split('Running ninja in directory ', 1)[-1]

    build_path = build_path.split('\n', 1)[0]

    target_file = os.path.join(build_path, 'sdkconfig')

    with open(target_file, 'r') as f:
        file = f.read()

    for i, line in enumerate(file.split('\n')):
        if (
            line.startswith('CONFIG_PARTITION_TABLE_CUSTOM_FILENAME') or
            line.startswith('CONFIG_PARTITION_TABLE_FILENAME')
        ):
            return line.split('=', 1)[-1].replace('"', '')


PARTITION_HEADER = '''\
# Notes: the offset of the partition table itself is set in
# $IDF_PATH/components/partition_table/Kconfig.projbuild.
# Name,   Type, SubType, Offset,  Size, Flags
'''


class Partition:

    def __init__(self, file_path):
        self.file_path = file_path
        self._saved_data = None
        self.csv_data = self.read_csv()
        last_partition = self.csv_data[-1]

        self.total_space = last_partition[-2] + last_partition[-3]

    def revert_to_original(self):
        with open(self.file_path, 'w') as f:
            f.write(self._saved_data)

    def get_app_size(self) -> int:
        for part in self.csv_data:
            if part[1] == 'app':
                return part[4]

    def set_app_size(self, size):
        next_offset = 0
        app_size = 0

        for i, part in enumerate(self.csv_data):
            if next_offset == 0:
                next_offset = part[3]

            if part[3] != next_offset:
                part[3] = next_offset  # NOQA

            if part[1] == 'app':
                factor = ((part[4] + size) / 4096.0) + 1
                part[4] = int(int(factor) * 4096)  # NOQA
                app_size += part[4]
            elif app_size != 0:
                part[4] = self.total_space - next_offset  # NOQA

            next_offset += part[4]

        if next_offset > self.total_space:
            raise RuntimeError(
                f'Board does not have enough space, overflow of '
                f'{next_offset - self.total_space} bytes ({self.file_path})\n'
            )

    def save(self):
        otp = []

        def convert_to_hex(itm):
            if isinstance(itm, int):
                itm = hex(itm)
            return itm

        for line in self.csv_data:
            otp.append(','.join(convert_to_hex(item) for item in line))

        with open(self.file_path, 'w') as f:
            f.write(PARTITION_HEADER)
            f.write('\n'.join(otp))

    def read_csv(self):
        with open(self.file_path, 'r') as f:
            csv_data = f.read()

        self._saved_data = csv_data

        csv_data = [
            line.strip()
            for line in csv_data.split('\n')
            if line.strip() and not line.startswith('#')
        ]

        def convert_to_int(elem):
            if elem.startswith('0'):
                elem = int(elem, 16)
            return elem

        for j, line in enumerate(csv_data):
            line = [convert_to_int(item.strip()) for item in line.split(',')]
            csv_data[j] = line

        return csv_data


def get_espidf():
    cmd = [
        'git',
        'submodule',
        'update',
        '--init',
        '--recursive',
        '--',
        'lib/esp-idf'
    ]

    result, _ = spawn(cmd)
    if result != 0:
        sys.exit(result)


board_variant = None
skip_partition_resize = False
partition_size = None


def parse_args(extra_args, lv_cflags, board):
    global board_variant
    global skip_partition_resize
    global partition_size

    esp_argParser = ArgumentParser(prefix_chars='-B')

    esp_argParser.add_argument(
        '--skip-partition-resize',
        dest='skip_partition_resize',
        help='clean the build',
        action='store_true'
    )

    esp_argParser.add_argument(
        'BOARD_VARIANT',
        dest='board_variant',
        default='',
        action='store'
    )

    esp_argParser.add_argument(
        '--partition-size',
        dest='partition_size',
        default=-1,
        type=int,
        action='store'
    )

    esp_args, extra_args = esp_argParser.parse_known_args(extra_args)

    skip_partition_resize = esp_args.skip_partition_resize
    partition_size = esp_args.partition_size

    if board is None:
        board = 'ESP32_GENERIC'

    board_variant = esp_args.board_variant

    if lv_cflags is None:
        lv_cflags = '-DLV_KCONFIG_IGNORE=1'
    else:
        lv_cflags += ' -DLV_KCONFIG_IGNORE=1'

    return extra_args, lv_cflags, board


mpy_cross_cmd = ['make', '-C', 'lib/micropython/mpy-cross']
esp_cmd = [
    'make',
    '',
    f'-j {os.cpu_count()}',
    '-C',
    f'lib/micropython/ports/esp32'
]
clean_cmd = []
compile_cmd = []
submodules_cmd = []


def build_commands(_, extra_args, __, lv_cflags, board):
    esp_cmd.extend([
        f'LV_CFLAGS="{lv_cflags}"',
        f'LV_PORT=esp32',
        f'BOARD={board}',
        'USER_C_MODULES=../../../../../micropython.cmake'
    ])

    esp_cmd.extend(extra_args)

    clean_cmd.extend(esp_cmd[:])
    clean_cmd[1] = 'clean'

    compile_cmd.extend(esp_cmd[:])
    compile_cmd.pop(1)

    submodules_cmd.extend(esp_cmd[:])
    submodules_cmd[1] = 'submodules'

    if board_variant:
        clean_cmd.insert(8, f'BOARD_VARIANT={board_variant}')
        compile_cmd.insert(7, f'BOARD_VARIANT={board_variant}')
        submodules_cmd.insert(8, f'BOARD_VARIANT={board_variant}')

    mpconfigport_path = 'lib/micropython/ports/esp32/mpconfigport.h'

    with open(mpconfigport_path, 'rb') as f:
        data = f.read().decode('utf-8')

    if '#define MICROPY_PY_MACHINE_I2S (0)' not in data:
        data = data.replace(
            '#ifndef MICROPY_PY_MACHINE_I2S',
            '#define MICROPY_PY_MACHINE_I2S (0)\n'
            '#ifndef MICROPY_PY_MACHINE_I2S'
        )
        with open(mpconfigport_path, 'wb') as f:
            f.write(data.encode('utf-8'))


def get_idf_version():
    if 'ESP_IDF_VERSION' in os.environ:
        return os.environ['ESP_IDF_VERSION']


def build_manifest(target, script_dir, frozen_manifest):
    update_mphalport(target)

    with open(f'lib/micropython/ports/esp32/boards/sdkconfig.base', 'r') as f:
        sdkconfig_base = f.read()

    if 'CONFIG_FREERTOS_INTERRUPT_BACKTRACE=n' not in sdkconfig_base:
        sdkconfig_base += '\nCONFIG_FREERTOS_INTERRUPT_BACKTRACE=n\n'
        sdkconfig_base += 'CONFIG_FREERTOS_IDLE_TASK_STACKSIZE=4096\n'

        with open(
            f'lib/micropython/ports/esp32/boards/sdkconfig.base', 'w'
        ) as f:
            f.write(sdkconfig_base)

    idf_version = get_idf_version()
    manifest_path = 'lib/micropython/ports/esp32/boards/manifest.py'

    manifest_files = [
        f'{script_dir}/driver/display/display_driver_framework.py',
        f'{script_dir}/driver/fs_driver.py',
        f'{script_dir}/utils/lv_utils.py',
    ]
    generate_manifest(manifest_path, frozen_manifest, *manifest_files)

    network_common_path = 'lib/micropython/ports/esp32/network_common.c'

    with open(network_common_path, 'rb') as f:
        network_common = f.read()

    if idf_version == '5.1':
        network_common = network_common.replace(
            b'_Static_assert(WIFI_AUTH_MAX == 10',
            b'_Static_assert(WIFI_AUTH_MAX == 11'
        )
    elif idf_version == '5.0':
        network_common = network_common.replace(
            b'_Static_assert(WIFI_AUTH_MAX == 11',
            b'_Static_assert(WIFI_AUTH_MAX == 10'
        )

    with open(network_common_path, 'wb') as f:
        f.write(network_common)


def clean():
    if 'deploy' in clean_cmd:
        clean_cmd.remove('deploy')

    spawn(clean_cmd)


def setup_idf_environ():
    idf_ver = get_idf_version()
    env = None

    if idf_ver is None or idf_ver not in ('5.0', '5.1'):
        idf_path = 'lib/esp-idf'

        if os.path.exists(os.path.join(idf_path, 'export.sh')):
            cmds = [['cd', idf_path]]

            if sys.platform.startswith('win'):
                cmds.append(['export'])
                cmds.append(['set'])
            else:
                cmds.append(['. ./export.sh'])
                cmds.append(['printenv'])

            result, output = spawn(cmds, out_to_screen=False)

            if result != 0:
                print(output)
                sys.exit(result)

            output = [line for line in output.split('\n') if '=' in line]
            env = {
                line.split('=', 1)[0]: line.split('=', 1)[1]
                for line in output
            }
        else:
            args = sys.argv[:]

            if 'submodules' not in args:
                args.insert(2, 'submodules')

            args = " ".join(args)

            print('ESP-IDF version 5.0.x or 5.1.x is needed to compile')
            print('Please rerun the build using the command below...')
            print(f'"{sys.executable} {args}"')
            raise RuntimeError

    return env


def submodules():
    idf_ver = get_idf_version()
    if idf_ver is None or idf_ver not in ('5.0', '5.1'):
        idf_path = 'lib/esp-idf'
        if not os.path.exists(os.path.join(idf_path, 'export.sh')):
            print('collecting ESP-IDF v5.1')
            print('this might take a bit...')
            print()
            get_espidf()
            print()

        cmds = [['cd', idf_path]]

        if sys.platform.startswith('win'):
            cmds.append(['install', 'all'])
        else:
            cmds.append(['./install.sh', 'all'])

        print('setting up ESP-IDF v5.1')
        print('this might take a bit...')
        print()

        result, _ = spawn(cmds)
        if result != 0:
            sys.exit(result)

    if 'deploy' in submodules_cmd:
        submodules_cmd.remove('deploy')

    return_code, _ = spawn(submodules_cmd)
    if return_code != 0:
        sys.exit(return_code)


def compile():  # NOQA
    env = setup_idf_environ()

    if 'deploy' in compile_cmd:
        if not skip_partition_resize:
            compile_cmd.remove('deploy')
            deploy = True
        else:
            deploy = False
    else:
        deploy = False

    ret_code, output = spawn(compile_cmd, env=env)
    if ret_code != 0:
        if (
            'partition is too small ' not in output or
            skip_partition_resize
        ):
            sys.exit(ret_code)

        sys.stdout.write('\n\033[31;1m***** Resizing Partition *****\033[0m\n')
        sys.stdout.flush()

        partition_file_name = get_partition_file_name(output)
        partition_file_name = (
            os.path.join('lib/micropython/ports/esp32', partition_file_name)
        )
        partition = Partition(partition_file_name)

        if partition_size != -1:
            overflow_amount = partition_size - partition.get_app_size()
        else:
            end = output.split('(overflow ', 1)[-1]
            overflow_amount = int(end.split(')', 1)[0], 16)

        partition.set_app_size(overflow_amount)
        partition.save()

        sys.stdout.write(
            '\n\033[31;1m***** Running build again *****\033[0m\n\n'
        )
        sys.stdout.flush()

        if deploy:
            compile_cmd.append('deploy')

        ret_code, output = spawn(compile_cmd, env=env)

        if ret_code != 0:
            sys.exit(ret_code)

    elif not skip_partition_resize:
        if 'Project build complete.' in output:

            sys.stdout.write(
                '\n\033[31;1m***** Resizing Partition *****\033[0m\n'
            )
            sys.stdout.flush()

            partition_file_name = get_partition_file_name(output)
            partition_file_name = os.path.join(
                'lib/micropython/ports/esp32',
                partition_file_name
            )
            partition = Partition(partition_file_name)

            if partition_size != -1:
                part_size = partition.get_app_size()
                size_diff = abs(part_size - partition_size)

                if part_size < partition_size:
                    size_diff = -size_diff
            else:
                size_diff = output.rsplit('application')[1]
                size_diff = int(
                    size_diff.split('(', 1)[-1].split('remaining')[0].strip()
                )

            partition.set_app_size(-size_diff)
            partition.save()

            sys.stdout.write(
                '\n\033[31;1m***** Running build again *****\033[0m\n\n'
            )
            sys.stdout.flush()

            if deploy:
                compile_cmd.append('deploy')

            ret_code, output = spawn(compile_cmd, env=env)

            if ret_code != 0:
                sys.exit(ret_code)

    if 'Running cmake in directory ' in output:
        build_path = output.split('Running cmake in directory ', 1)[-1]
    else:
        build_path = output.split('Running ninja in directory ', 1)[-1]

    build_path = build_path.split('\n', 1)[0]

    print()
    print('COMPILED BINARIES')
    print(f'  BOOTLOADER:  {build_path}/bootloader/bootloader.bin')
    print(f'  FIRMWARE:    {build_path}/firmware.bin')
    print(f'  MICROPYTHON: {build_path}/micropython.bin')


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd)
    if return_code != 0:
        sys.exit(return_code)
