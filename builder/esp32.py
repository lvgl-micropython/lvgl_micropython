import os
import shutil
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
            if part[1] in ('app', 'factory'):
                return part[4]

    def set_app_size(self, size):
        next_offset = 0
        app_size = 0

        for i, part in enumerate(self.csv_data):
            if next_offset == 0:
                next_offset = part[3]

            if part[3] != next_offset:
                part[3] = next_offset  # NOQA

            if part[1] in ('app', 'factory'):
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
        [
            'git', 'submodule', 'update', '--init',
            f'--jobs {os.cpu_count()}', '--', 'lib/esp-idf'
        ],
        ['cd', 'lib/esp-idf'],
        [
            'git', 'submodule', 'update', '--init',
            f'--jobs {os.cpu_count()}', '--',
            'components/bt/host/nimble/nimble',
            'components/esp_wifi',
            'components/esptool_py/esptool',
            'components/lwip/lwip',
            'components/mbedtls/mbedtls',
            'components/bt/controller/lib_esp32',
            'components/bt/controller/lib_esp32c3_family'
        ]
    ]
    print()
    print('collecting ESP-IDF v5.2.0')
    print('this might take a while...')
    result, _ = spawn(cmd, spinner=True)
    if result != 0:
        sys.exit(result)


board_variant = None
board = None
skip_partition_resize = False
partition_size = None
flash_size = '0'
oct_flash = False

DEBUG = False
deploy = False
PORT = None
BAUD = 460800
ccache = False
disable_OTG = True


def parse_args(extra_args, lv_cflags, brd):
    global board
    global board_variant
    global skip_partition_resize
    global partition_size
    global flash_size
    global oct_flash
    global DEBUG
    global PORT
    global BAUD
    global deploy
    global ccache
    global disable_OTG

    board = brd

    if board is None:
        board = 'ESP32_GENERIC'

    if board in ('ESP32_GENERIC_S2', 'ESP32_GENERIC_S3'):
        esp_argParser = ArgumentParser(prefix_chars='-')
        esp_argParser.add_argument(
            '--USB-OTG',
            dest='usb_otg',
            default=False,
            action='store_true'
        )
        esp_args, extra_args = esp_argParser.parse_known_args(extra_args)

        disable_OTG = not esp_args.usb_otg

    esp_argParser = ArgumentParser(prefix_chars='BPd')
    esp_argParser.add_argument(
        'BAUD',
        dest='baud',
        default=460800,
        type=int,
        action='store'
    )
    esp_argParser.add_argument(
        'PORT',
        dest='port',
        default=None,
        action='store'
    )
    esp_argParser.add_argument(
        'deploy',
        dest='deploy',
        default=False,
        action='store_true'
    )
    esp_args, extra_args = esp_argParser.parse_known_args(extra_args)

    BAUD = esp_args.baud
    PORT = esp_args.port
    deploy = esp_args.deploy

    if board in ('ESP32_GENERIC', 'ESP32_GENERIC_S3'):
        esp_argParser = ArgumentParser(prefix_chars='B')
        esp_argParser.add_argument(
            'BOARD_VARIANT',
            dest='board_variant',
            default='',
            action='store'
        )
        esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
        board_variant = esp_args.board_variant
    else:
        for arg in extra_args:
            if arg.startswith('BOARD_VARIANT'):
                raise RuntimeError(f'BOARD_VARIANT not supported by "{board}"')

    if board_variant in ('SPIRAM', 'SPIRAM_OCT'):
        if board == 'ESP32_GENERIC_S2':
            esp_argParser = ArgumentParser(prefix_chars='-')

            esp_argParser.add_argument(
                '--flash-size',
                dest='flash_size',
                help='flash size',
                choices=('2', '4'),
                default='4',
            )
            esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
            flash_size = esp_args.flash_size

        elif board == 'ESP32_GENERIC':
            esp_argParser = ArgumentParser(prefix_chars='-')

            esp_argParser.add_argument(
                '--flash-size',
                dest='flash_size',
                help='flash size',
                choices=('4', '8', '16'),
                default='4',
            )
            esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
            flash_size = esp_args.flash_size

        elif board == 'ESP32_GENERIC_S3':
            esp_argParser = ArgumentParser(prefix_chars='-')

            esp_argParser.add_argument(
                '--octal-flash',
                help='octal spi flash',
                dest='oct_flash',
                action='store_true'
            )

            esp_argParser.add_argument(
                '--flash-size',
                dest='flash_size',
                help='flash size',
                choices=('4', '8', '16', '32'),
                default='8',
            )
            esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
            flash_size = esp_args.flash_size
            oct_flash = esp_args.oct_flash

    esp_argParser = ArgumentParser(prefix_chars='-')

    esp_argParser.add_argument(
        '--skip-partition-resize',
        dest='skip_partition_resize',
        help='clean the build',
        action='store_true'
    )

    esp_argParser.add_argument(
        '--partition-size',
        dest='partition_size',
        default=-1,
        type=int,
        action='store'
    )
    esp_argParser.add_argument(
        '--debug',
        dest='debug',
        default=False,
        action='store_true'
    )
    esp_argParser.add_argument(
        '--ccache',
        dest='ccache',
        default=False,
        action='store_true'
    )

    esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
    skip_partition_resize = esp_args.skip_partition_resize
    partition_size = esp_args.partition_size
    ccache = esp_args.ccache
    DEBUG = esp_args.debug

    if lv_cflags:
        lv_cflags += ' -DLV_KCONFIG_IGNORE=1'
    else:
        lv_cflags = '-DLV_KCONFIG_IGNORE=1'

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
SCRIPT_DIR = ''


def build_commands(_, extra_args, script_dir, lv_cflags, ___):
    global SCRIPT_DIR
    SCRIPT_DIR = script_dir

    clean_cmd.extend(esp_cmd[:])
    clean_cmd[1] = 'clean'
    clean_cmd.append(f'BOARD={board}')

    submodules_cmd.extend(esp_cmd[:])
    submodules_cmd[1] = 'submodules'
    submodules_cmd.append(f'BOARD={board}')

    esp_cmd.extend([
        'SECOND_BUILD=0',
        f'LV_CFLAGS="{lv_cflags}"',
        f'LV_PORT=esp32',
        f'BOARD={board}',
        'USER_C_MODULES=../../../../../ext_mod/micropython.cmake'
    ])

    esp_cmd.extend(extra_args)

    compile_cmd.extend(esp_cmd[:])
    compile_cmd.pop(1)

    if board_variant:
        clean_cmd.append(f'BOARD_VARIANT={board_variant}')
        compile_cmd.insert(7, f'BOARD_VARIANT={board_variant}')
        submodules_cmd.append(f'BOARD_VARIANT={board_variant}')


def get_idf_path():
    if 'IDF_PATH' in os.environ:
        idf_path = os.environ['IDF_PATH']
        if not os.path.exists(idf_path):
            idf_path = None
    else:
        idf_path = None

    return idf_path


cached_idf_version = None


def has_correct_idf():
    global cached_idf_version

    idf_path = get_idf_path()

    if cached_idf_version is None and idf_path:
        exit_code, data = spawn(
            ['python3', f'{idf_path}/tools/idf.py', '--version'],
            out_to_screen=False
        )
        if not exit_code:
            version = data.split('v')[-1].split('-')[0]
            if version:
                cached_idf_version = version

    return cached_idf_version is not None and cached_idf_version == '5.2.0'


def build_manifest(
    target, script_dir, lvgl_api, displays, indevs, frozen_manifest
):
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

    manifest_path = 'lib/micropython/ports/esp32/boards/manifest.py'

    generate_manifest(
        script_dir, lvgl_api, manifest_path,
        displays, indevs, frozen_manifest, 'esp32/touch_cal_data.py'
    )


def clean(clean_mpy_cross):
    env, cmds = setup_idf_environ()

    if clean_mpy_cross:
        cross_clean = mpy_cross_cmd[:]
        cross_clean.insert(1, 'clean')
        cross_clean = cmds[:] + [cross_clean]
        spawn(cross_clean, env=env)

    cmds.append(clean_cmd)

    spawn(cmds, env=env)


def get_clean_environment():
    env = {
        k: v for k, v in os.environ.items()
        if not k.startswith('IDF')
    }
    if 'PATH' in env:
        env['PATH'] = os.pathsep.join(
            item for item in env['PATH'].split(os.pathsep)
            if 'espressif' not in item and 'esp-idf' not in item
        )

    return env


def environ_helper(idf_path):
    env = get_clean_environment()

    py_path = os.path.split(sys.executable)[0]
    idf_path = os.path.abspath(idf_path)
    idf_tools_path = os.path.join(idf_path, 'tools')

    env['PATH'] = (
        py_path + os.pathsep +
        os.pathsep + idf_tools_path +
        os.pathsep + env.get('PATH', '')
    )
    env['IDF_PATH'] = idf_path

    for key, value in env.items():
        os.environ[key] = value

    if 'GITHUB_RUN_ID' in os.environ:
        if sys.platform.startswith('win'):
            env_cmds = [
                ['echo', f"{py_path}", '|', 'Out-File',
                 '-Append', '-FilePath', '$env:GITHUB_PATH',
                 '-Encoding', 'utf8'],
                ['echo', f"{idf_path}", '|', 'Out-File',
                 '-Append', '-FilePath', '$env:GITHUB_PATH',
                 '-Encoding', 'utf8'],
                ['echo', f"{idf_tools_path}", '|', 'Out-File',
                 '-Append', '-FilePath', '$env:GITHUB_PATH',
                 '-Encoding', 'utf8']
            ]
        else:
            env_cmds = [
                ['echo', f"{py_path}", '>>', '$GITHUB_PATH'],
                ['echo', f"{idf_path}", '>>', '$GITHUB_PATH'],
                ['echo', f"{idf_tools_path}", '>>', '$GITHUB_PATH']
            ]

        spawn(env_cmds, out_to_screen=False)

    return env


IDF_ENVIRON_SET = False


def setup_idf_environ():
    global IDF_ENVIRON_SET
    # There were some modifications made with how the environment gets set up
    # @cheops put quite a bit of time in to research the best solution
    # and also with the testing of the code.
    if IDF_ENVIRON_SET or (not IDF_ENVIRON_SET and has_correct_idf()):
        env = os.environ
        IDF_ENVIRON_SET = True
    elif not IDF_ENVIRON_SET:
        print('Getting ESP-IDF build Environment')
        idf_path = 'lib/esp-idf'

        if not os.path.exists(os.path.join(idf_path, 'export.sh')):
            args = sys.argv[:]

            if 'submodules' not in args:
                args.insert(2, 'submodules')

            args = " ".join(args)

            sys.stderr.write('ESP-IDF version 5.2.0 is needed to compile\n')
            sys.stderr.write(
                'Please rerun the build using the command below...\n'
            )
            sys.stderr.write(f'"{sys.executable} {args}"\n\n')
            sys.stderr.flush()
            sys.exit(-1)

        environ_helper(idf_path)

        if 'GITHUB_RUN_ID' in os.environ:
            cmds = [
                [f'export "IDF_PATH={idf_path}"'],
                ['cd', idf_path],
                ['. ./export.sh'],
                ['printenv']
            ]
        else:
            cmds = [
                [f'cd {idf_path}'],
                [f'. ./export.sh'],
                ['printenv']
            ]

        result, output = spawn(cmds, out_to_screen=False)

        if result != 0:
            sys.stderr.write('********* ERROR **********\n')
            sys.stderr.flush()
            print(output)
            sys.exit(result)

        output = [line for line in output.split('\n') if '=' in line]

        temp_env = {
            line.split('=', 1)[0]: line.split('=', 1)[1]
            for line in output
        }

        for key, value in temp_env.items():
            os.environ[key] = value

        env = os.environ
        IDF_ENVIRON_SET = True
    else:
        # this is a sanity check and should never actually run
        env = os.environ

    if 'GITHUB_RUN_ID' in os.environ:
        idf_path = os.path.abspath(env["IDF_PATH"])
        cmds = [
            [f'export "IDF_PATH={idf_path}"'],
            ['cd', f'{idf_path}'],
            ['. ./export.sh'],
            [f'cd {SCRIPT_DIR}']
        ]
    else:
        cmds = []

    return env, cmds


def submodules():
    if has_correct_idf():
        idf_path = os.environ['IDF_PATH']
    else:
        idf_path = 'lib/esp-idf'
        if not os.path.exists(os.path.join(idf_path, 'export.sh')):
            get_espidf()

    cmds = [
        [f'export "IDF_PATH={os.path.abspath(idf_path)}"'],
        ['cd', idf_path],
        ['./install.sh', 'all']
    ]

    print('setting up ESP-IDF v5.2.0')
    print('this might take a while...')
    env = {k: v for k, v in os.environ.items()}
    env['IDF_PATH'] = os.path.abspath(idf_path)

    result, _ = spawn(cmds, env=env)
    if result != 0:
        sys.exit(result)

    env, cmds = setup_idf_environ()
    cmds.append(submodules_cmd)

    return_code, _ = spawn(cmds, env=env)
    if return_code != 0:
        sys.exit(return_code)


def compile():  # NOQA
    global PORT

    env, cmds = setup_idf_environ()

    if ccache:
        env['IDF_CCACHE_ENABLE'] = '1'

    if (
        board in ('ESP32_GENERIC', 'ESP32_GENERIC_S2', 'ESP32_GENERIC_S3') and
        board_variant in ('SPIRAM', 'SPIRAM_OCT')
    ):
        base_config = [
            'CONFIG_ESPTOOLPY_FLASHMODE_QIO=y',
            'CONFIG_ESPTOOLPY_FLASHFREQ_80M=y',
            'CONFIG_ESPTOOLPY_AFTER_NORESET=y',
            'CONFIG_PARTITION_TABLE_CUSTOM=y',
            'CONFIG_ESPTOOLPY_FLASHSIZE_2MB=n',
            'CONFIG_ESPTOOLPY_FLASHSIZE_4MB=n',
            'CONFIG_ESPTOOLPY_FLASHSIZE_8MB=n',
            'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=n',
            'CONFIG_ESPTOOLPY_FLASHSIZE_32MB=n',
            f'CONFIG_ESPTOOLPY_FLASHSIZE_{flash_size}MB=y',
            'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME='
            f'"partitions-{flash_size}MiB.csv"',
        ]

        if DEBUG:
            base_config.extend([
                'CONFIG_BOOTLOADER_LOG_LEVEL_NONE=n',
                'CONFIG_BOOTLOADER_LOG_LEVEL_ERROR=n',
                'CONFIG_BOOTLOADER_LOG_LEVEL_WARN=n',
                'CONFIG_BOOTLOADER_LOG_LEVEL_INFO=n',
                'CONFIG_BOOTLOADER_LOG_LEVEL_DEBUG=y',
                'CONFIG_BOOTLOADER_LOG_LEVEL_VERBOSE=n',
                'CONFIG_LCD_ENABLE_DEBUG_LOG=y',
                'CONFIG_HAL_LOG_LEVEL_NONE=n',
                'CONFIG_HAL_LOG_LEVEL_ERROR=n',
                'CONFIG_HAL_LOG_LEVEL_WARN=n',
                'CONFIG_HAL_LOG_LEVEL_INFO=n',
                'CONFIG_HAL_LOG_LEVEL_DEBUG=y',
                'CONFIG_HAL_LOG_LEVEL_VERBOSE=n',
                'CONFIG_LOG_MAXIMUM_LEVEL_ERROR=n',
                'CONFIG_LOG_MAXIMUM_LEVEL_WARN=n',
                'CONFIG_LOG_MAXIMUM_LEVEL_INFO=n',
                'CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y',
                'CONFIG_LOG_MAXIMUM_LEVEL_VERBOSE=n',
                'CONFIG_LOG_DEFAULT_LEVEL_NONE=n',
                'CONFIG_LOG_DEFAULT_LEVEL_ERROR=n',
                'CONFIG_LOG_DEFAULT_LEVEL_WARN=n',
                'CONFIG_LOG_DEFAULT_LEVEL_INFO=n',
                'CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y',
                'CONFIG_LOG_DEFAULT_LEVEL_VERBOSE=n',
            ])

        base_config.append('')

        board_config_path = f'build/sdkconfig.board'
        with open(board_config_path, 'w') as f:
            f.write('\n'.join(base_config))

        config_settings = []
        if board == 'ESP32_GENERIC_S3':
            if oct_flash and flash_size in ('16', '32'):
                if flash_size == '32':
                    config_settings.extend([
                        'CONFIG_ESPTOOLPY_FLASHMODE_QIO=n',
                        'CONFIG_ESPTOOLPY_FLASHMODE_DOUT=y'
                    ])

                config_settings.append('CONFIG_ESPTOOLPY_OCT_FLASH=y')

        with open(board_config_path, 'a') as f:
            f.write('\n'.join(config_settings))

        mpconfigboard_cmake_path = (
            'lib/micropython/ports/esp32/boards/'
            f'{board}/mpconfigboard.cmake'
        )

        with open(mpconfigboard_cmake_path, 'rb') as f:
            data = f.read().decode('utf-8')

        sdkconfig = (
            'set(SDKCONFIG_DEFAULTS ${SDKCONFIG_DEFAULTS} '
            '../../../../build/sdkconfig.board)'
        )

        if sdkconfig not in data:
            data += '\n' + sdkconfig + '\n'

            with open(mpconfigboard_cmake_path, 'wb') as f:
                f.write(data.encode('utf-8'))

    if board in ('ESP32_GENERIC_S2', 'ESP32_GENERIC_S3') and disable_OTG:

        mphalport_path = 'lib/micropython/ports/esp32/mphalport.c'

        with open(mphalport_path, 'rb') as f:
            data = f.read().decode('utf-8')

        data = data.replace(
            '#elif CONFIG_USB_OTG_SUPPORTED',
            '#elif MP_USB_OTG'
        )

        with open(mphalport_path, 'wb') as f:
            f.write(data.encode('utf-8'))

        main_path = 'lib/micropython/ports/esp32/main.c'

        with open(main_path, 'rb') as f:
            data = f.read().decode('utf-8')

        data = data.replace(
            '#elif CONFIG_USB_OTG_SUPPORTED',
            '#elif MP_USB_OTG'
        )

        with open(main_path, 'wb') as f:
            f.write(data.encode('utf-8'))

        mpconfigboard_path = (
            f'lib/micropython/ports/esp32/boards/{board}/mpconfigboard.h'
        )

        with open(mpconfigboard_path, 'rb') as f:
            data = f.read().decode('utf-8')

        if 'MP_USB_OTG' not in data:
            data += (
                '\n'
                '#ifndef MP_USB_OTG\n'
                '#define MP_USB_OTG    (0)\n'
                '#endif'
            )

            with open(mpconfigboard_path, 'wb') as f:
                f.write(data.encode('utf-8'))

    src_path = 'micropy_updates/esp32'
    dst_path = 'lib/micropython/ports/esp32'

    for file in os.listdir(src_path):
        src_file = os.path.join(src_path, file)
        dst_file = os.path.join(dst_path, file)
        shutil.copyfile(src_file, dst_file)

    mpconfigport_path = 'lib/micropython/ports/esp32/mpconfigport.h'

    with open(mpconfigport_path, 'rb') as f:
        data = f.read().decode('utf-8')

    if '#define MICROPY_BLUETOOTH_NIMBLE            (0)' not in data:
        data = data.replace(
            '#define MICROPY_BLUETOOTH_NIMBLE            (1)',
            '#define MICROPY_BLUETOOTH_NIMBLE            (0)'
        )
        with open(mpconfigport_path, 'wb') as f:
            f.write(data.encode('utf-8'))

    # if '#define MICROPY_PY_MACHINE_I2S (0)' not in data:
    #     data = data.replace(
    #         '#ifndef MICROPY_PY_MACHINE_I2S',
    #         '#define MICROPY_PY_MACHINE_I2S (0)\n'
    #         '#ifndef MICROPY_PY_MACHINE_I2S'
    #     )
    #     with open(mpconfigport_path, 'wb') as f:
    #         f.write(data.encode('utf-8'))

    if not sys.platform.startswith('win'):
        cmds.append(compile_cmd)
    else:
        cmds = compile_cmd

    ret_code, output = spawn(cmds, env=env, cmpl=True)
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

        compile_cmd[4] = 'SECOND_BUILD=1'
        ret_code, output = spawn(cmds, env=env, cmpl=True)

        if ret_code != 0:
            sys.exit(ret_code)

    elif not skip_partition_resize:
        if 'build complete' in output:
            partition_file_name = get_partition_file_name(output)
            partition_file_name = os.path.join(
                'lib/micropython/ports/esp32',
                partition_file_name
            )
            partition = Partition(partition_file_name)

            remaining = output.rsplit('application')[-1]
            remaining = int(
                remaining.split('(', 1)[-1].split('remaining')[0].strip()
            )

            if remaining > 4096 or partition_size != -1:
                sys.stdout.write(
                    '\n\033[31;1m***** Resizing Partition *****\033[0m\n'
                )
                sys.stdout.flush()

            if partition_size != -1:
                part_size = partition.get_app_size()
                resize = abs(part_size - partition_size)

                if part_size < partition_size:
                    resize = -resize

                partition.set_app_size(-resize)
                partition.save()

                sys.stdout.write(
                    '\n\033[31;1m***** Running build again *****\033[0m\n\n'
                )
                sys.stdout.flush()

            elif remaining > 4096:
                partition.set_app_size(-remaining)
                partition.save()

                sys.stdout.write(
                    '\n\033[31;1m***** Running build again *****\033[0m\n\n'
                )
                sys.stdout.flush()

            if remaining > 4096 or partition_size != -1:
                compile_cmd[4] = 'SECOND_BUILD=1'

                ret_code, output = spawn(cmds, env=env, cmpl=True)

                if ret_code != 0:
                    sys.exit(ret_code)

    if 'To flash, run this command:' in output:
        output = output.split('To flash, run this command:')[-1].strip()
        output = output.split('\n')[0]

        python_path, output = output.split('python ', 1)
        python_path += 'python'

        esp_tool_path, output = output.split('esptool.py ', 1)
        esp_tool_path += 'esptool.py'
        esp_tool_path = esp_tool_path.replace(
            '../../../',
            os.getcwd() + '/lib/'
        )

        out_cmd = []

        for file in (
            'bootloader.bin',
            'partition-table.bin',
            'micropython.bin'
        ):
            arg, output = output.split('build-', 1)
            output = 'build-' + output
            path, output = output.split(file, 1)
            output = output.strip()
            path += file
            out_cmd.append(arg.strip())
            out_cmd.append(
                os.path.abspath('lib/micropython/ports/esp32/' + path)
            )

        out_cmd = ' '.join(out_cmd)

        build_name = f'build-{board}'

        os.remove('build/lvgl_header.h')

        for f in os.listdir('build'):
            if f.startswith('lvgl'):
                continue

            os.remove(os.path.join('build', f))

        if board_variant:
            build_name += f'-{board_variant}'

        build_bin_file = f'build/lvgl_micropy_{build_name[6:]}-{flash_size}'
        if oct_flash:
            build_bin_file += '_OCTFLASH'

        build_bin_file += '.bin'
        build_bin_file = os.path.abspath(build_bin_file)

        cmd = f'{python_path} {esp_tool_path} {out_cmd}'
        cmd = cmd.replace('write_flash', f'merge_bin -o {build_bin_file}')
        cmd = cmd.replace('--flash_freq 80m ', '')
        cmd = cmd.replace('-p (PORT) ', '')
        cmd = cmd.replace('-b 460800 ', '')
        cmd = cmd.replace('--before default_reset ', '')
        cmd = cmd.replace('--after no_reset ', '')

        # if not sys.platform.startswith('win'):
        #     cmds = [
        #         # [f'export "IDF_PATH={os.path.abspath(env["IDF_PATH"])}"'],
        #         # ['cd', 'lib/esp-idf'],
        #         # ['. ./export.sh'],
        #         # ['cd ../..'],
        #         cmd
        #     ]
        # else:
        cmds = [cmd]

        result, _ = spawn(cmds, env=env)
        if result:
            sys.exit(result)

        cmd = f'{python_path} {esp_tool_path} {out_cmd}'
        cmd = cmd.split('write_flash', 1)[0]
        cmd += f'write_flash 0x0 {build_bin_file}'

        if deploy:
            python_env_path = os.path.split(os.path.split(python_path)[0])[0]
            python_version = (
                os.path.split(python_env_path)[-1].split('_')[1][2:]
            )
            site_packages = os.path.join(
                python_env_path,
                f'lib/python{python_version}/site-packages'
            )
            sys.path.insert(0, site_packages)

            from esptool.targets import CHIP_DEFS  # NOQA
            from esptool.util import FatalError  # NOQA
            from serial.tools import list_ports

            cmd = cmd.replace('-b 460800', f'-b {BAUD}')

            def get_port_list():
                pts = sorted(p.device for p in list_ports.comports())
                if sys.platform.startswith('linux'):
                    serial_path = '/dev/serial/by_id'
                    if os.path.exists(serial_path):
                        pts_alt = [
                            os.path.join(serial_path, fle)
                            for fle in os.listdir(serial_path)
                        ]
                        pts = pts_alt + pts

                return pts

            def find_esp32(chip):
                found_ports = []
                for prt in get_port_list():
                    chip_class = CHIP_DEFS[chip]
                    try:
                        _esp = chip_class(prt, 115200, False)
                    except (FatalError, OSError):
                        continue

                    try:
                        _esp.connect('no_reset', 2)
                    except (FatalError, OSError):
                        pass
                    else:
                        found_ports.append(port)

                    if _esp and _esp._port:  # NOQA
                        _esp._port.close()  # NOQA

                return found_ports

            if PORT is None:

                ports = find_esp32(cmd.split('--chip ', 1)[-1].split(' ', 1)[0])
                if len(ports) > 1:
                    query = []
                    for i, port in enumerate(ports):
                        query.append(str(i + 1) + ': ' + port)
                    query.append('')
                    query.append('Which ESP32? :')

                    res = input('\n'.join(query))
                    res = int(res) - 1
                else:
                    res = 0

                PORT = ports[res]

            cmd = cmd.replace('-p (PORT)', f'-p "{PORT}"')

            erase_flash = (
                f'"{python_path}" "{esp_tool_path}" '
                f'-p "{PORT}" -b 460800 erase_flash'
            )

            result, _ = spawn(erase_flash)
            if result != 0:
                sys.exit(result)

            result, _ = spawn(cmd)

        else:
            print()
            print()
            print('To flash firmware:')
            print('Replace "(PORT)" with the serial port for your esp32')
            print('and run the commands.')
            print()

            print(
                python_path, esp_tool_path, '-p (PORT) -b 460800 erase_flash'
            )

            print()
            print(cmd.replace('-b 460800', '-b 921600'))
            print()


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd)
    if return_code != 0:
        sys.exit(return_code)
