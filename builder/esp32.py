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
# Name,   Type, SubType, Offset,  Size, Flags'''


OTA_PARTITION_TEMPLATE = f'''\
{PARTITION_HEADER}
nvs,      data, nvs,     0x{{:X}},   0x{{:X}},
otadata,  data, ota,     0x{{:X}},   0x{{:X}},
phy_init, data, phy,     0x{{:X}},   0x{{:X}},
ota_0,    app,  ota_0,   0x{{:X}},   0x{{:X}},
ota_1,    app,  ota_1,   0x{{:X}},   0x{{:X}},
vfs,      data, fat,     0x{{:X}},   0x{{:X}},
'''


PARTITION_TEMPLATE = f'''\
{PARTITION_HEADER}
nvs,      data, nvs,     0x{{:X}},   0x{{:X}},
phy_init, data, phy,     0x{{:X}},   0x{{:X}},
factory,  app,  factory, 0x{{:X}},   0x{{:X}},
vfs,      data, fat,     0x{{:X}},   0x{{:X}},
'''


ARDUINO_NANO_ESP32_PARTITION_TEMPLATE = f'''\
{PARTITION_HEADER}
nvs,      data,      nvs, 0x{{:X}}, 0x{{:X}},
otadata,  data,      ota, 0x{{:X}}, 0x{{:X}},
app0,      app,    ota_0, 0x{{:X}}, 0x{{:X}},
app1,      app,    ota_1, 0x{{:X}}, 0x{{:X}},
ffat,     data,      fat, 0x{{:X}}, 0x{{:X}},
factory,   app,  factory, 0x{{:X}}, 0x{{:X}},
coredump, data, coredump, 0x{{:X}}, 0x{{:X}},
'''


class Partition:
    total_size = 0

    @staticmethod
    def calc_part_size(value):
        factor = (value / 4096.0)
        if int(factor) != factor:
            factor += 1

        return int(int(factor) * 4096)

    @property
    def nvs(self):
        return self._nvs

    @nvs.setter
    def nvs(self, value):
        self._nvs = self.calc_part_size(value)

    @property
    def otadata(self):
        return self._otadata

    @otadata.setter
    def otadata(self, value):
        self._otadata = self.calc_part_size(value)

    @property
    def phy_init(self):
        return self._phy_init

    @phy_init.setter
    def phy_init(self, value):
        self._phy_init = self.calc_part_size(value)

    @property
    def factory(self):
        return self._factory

    @factory.setter
    def factory(self, value):
        self._factory = self.calc_part_size(value)

    @property
    def ota(self):
        return self._ota

    @ota.setter
    def ota(self, value):
        self._ota = self.calc_part_size(value)

    @property
    def vfs(self):
        final_offset = self.calculate_offsets()[-1]
        res = self.total_size - final_offset

        if res < 0:
            self.factory += res - 4096
            final_offset = self.calculate_offsets()[-1]
            res = self.total_size - final_offset

        return res

    def calculate_offsets(self):
        offsets = [0x9000, 0x9000 + self.nvs]
        if self.ota:
            offsets.append(offsets[-1] + self.otadata)
        offsets.append(offsets[-1] + self.phy_init)
        if self.ota:
            offsets.append(offsets[-1] + self.ota)
            offsets.append(offsets[-1] + self.ota)
        else:
            offsets.append(offsets[-1] + self.factory)

        return offsets

    def build(self):
        if self.ota:
            templ = OTA_PARTITION_TEMPLATE
            parts = [
                self.nvs,
                self.otadata,
                self.phy_init,
                self.ota,
                self.ota,
                self.vfs
            ]
        else:
            templ = PARTITION_TEMPLATE
            parts = [
                self.nvs,
                self.phy_init,
                self.factory,
                self.vfs
            ]

        part_data = zip(self.calculate_offsets(), parts)
        part_data = [item for items in part_data for item in items]

        with open(self.file_path, 'w') as f:
            f.write(templ.format(*part_data))

    def __init__(self, file_path):
        self._nvs = 0x6000
        self._ota = 0
        self._otadata = 0x2000
        self._phy_init = 0x1000
        self._factory = 0x180000

        self.file_path = file_path


class Partition2MB(Partition):
    total_size = 0x200000


class Partition4MB(Partition):
    total_size = 0x400000


class Partition8MB(Partition):
    total_size = 0x800000


class Partition16MB(Partition):
    total_size = 0x1000000


class Partition32MB(Partition):
    total_size = 0x2000000


PARTITION_SIZE_MAP = {
    '32': 0x2000000,
    '16': 0x1000000,
    '8': 0x800000,
    '4': 0x400000,
    '2': 0x200000
}


class ARDUINO_NANO_ESP32(Partition16MB):

    def calculate_offsets(self):
        offsets = [0x9000, 0x9000 + self.nvs]  # NOQA
        offsets.append(offsets[-1] + self.otadata)
        offsets.append(offsets[-1] + self.ota)
        offsets.append(offsets[-1] + self.ota)
        offsets.append(offsets[-1] + self.ffat)
        offsets.append(offsets[-1] + self.factory)
        offsets.append(offsets[-1] + self.coredump)

        return offsets

    def build(self):
        templ = ARDUINO_NANO_ESP32_PARTITION_TEMPLATE
        parts = [
            self.nvs,
            self.otadata,
            self.ota,
            self.ota,
            self.ffat,
            self.factory,
            self.coredump
        ]

        part_data = zip(self.calculate_offsets(), parts)
        part_data = [item for items in part_data for item in items]

        with open(self.file_path, 'w') as f:
            f.write(templ.format(*part_data))

    @property
    def ffat(self):
        return self._ffat

    @ffat.setter
    def ffat(self, value):
        self._ffat = self.calc_part_size(value)

    @property
    def coredump(self):
        return self._coredump

    @coredump.setter
    def coredump(self, value):
        self._coredump = self.calc_part_size(value)

    def __init__(self, file_path):
        Partition16MB.__init__(self, file_path)
        self._ffat = 0x960000
        self._coredump = 0x10000
        self._factory = 0x80000
        self._ota = 0x300000
        self._nvs = 0x5000


class ESP32_GENERIC(Partition2MB):
    pass


class ESP32_GENERIC_C3(Partition4MB):
    pass


class ESP32_GENERIC_S2(Partition4MB):
    pass


class ESP32_GENERIC_S3(Partition8MB):
    pass


class LILYGO_TTGO_LORA32(Partition4MB):
    pass


class LOLIN_C3_MINI(Partition4MB):
    pass


class LOLIN_S2_MINI(Partition4MB):
    pass


class LOLIN_S2_PICO(Partition4MB):
    pass


class M5STACK_ATOM(Partition4MB):
    pass


class OLIMEX_ESP32_POE(Partition4MB):
    pass


class SIL_WESP32(Partition16MB):

    def __init__(self, file_path):
        Partition16MB.__init__(self, file_path)
        self.ota = self.factory


class UM_FEATHERS2(Partition16MB):
    pass


class UM_FEATHERS2NEO(Partition4MB):
    pass


class UM_FEATHERS3(Partition16MB):
    pass


class UM_NANOS3(Partition8MB):
    pass


class UM_PROS3(Partition16MB):
    pass


class UM_TINYPICO(Partition4MB):
    pass


class UM_TINYS2(Partition4MB):
    pass


class UM_TINYS3(Partition8MB):
    pass


class UM_TINYWATCHS3(Partition8MB):
    pass


def get_espidf():
    if sys.platform.startswith('win'):
        return

    else:
        cmd = [
            'git',
            'submodule',
            'update',
            '--init',
            '--recursive',
            '--',
            'lib/esp-idf'
        ]

        result, _ = spawn(cmd, spinner=True)
        if result != 0:
            sys.exit(result)


board_variant = None
board = None
skip_partition_resize = False
partition_size = None
flash_size = '0'
oct_flash = False
partition: Partition = None


def parse_args(extra_args, lv_cflags, brd):
    global board
    global board_variant
    global skip_partition_resize
    global partition_size
    global flash_size
    global oct_flash
    global partition

    board = brd

    if board is None:
        board = 'ESP32_GENERIC'

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

    if board not in globals():
        raise RuntimeError(f'Invalid board {board}')

    partition = globals()[board](os.path.abspath('build/partition.csv'))

    if board in (
        'ESP32_GENERIC_S2',
        'ESP32_GENERIC_S3',
        'ESP32_GENERIC',
        'ESP32_GENERIC_C3'
    ):

        if board == 'ESP32_GENERIC':
            default = '2'
        elif board == 'ESP32_GENERIC_S3':
            default = '8'
        else:
            default = '4'
        esp_argParser = ArgumentParser(prefix_chars='-')

        esp_argParser.add_argument(
            '--flash-size',
            dest='flash_size',
            help='flash size',
            choices=('2', '4', '8', '16', '32'),
            default=default,
        )
        esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
        flash_size = esp_args.flash_size

        partition.total_size = PARTITION_SIZE_MAP[flash_size]

    if board == 'ESP32_GENERIC_S3':
        esp_argParser = ArgumentParser(prefix_chars='-')

        esp_argParser.add_argument(
            '--octal-flash',
            help='octal spi flash',
            dest='oct_flash',
            action='store_true'
        )

        esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
        oct_flash = esp_args.oct_flash

    esp_argParser = ArgumentParser(prefix_chars='-')

    esp_argParser.add_argument(
        '--nvs-size',
        dest='nvs_size',
        help='non volitale partition size',
        default=None,
        action='store',
        type=int
    )

    esp_argParser.add_argument(
        '--ota',
        dest='ota',
        help='add ota partitions',
        default=False,
        action='store_true'
    )

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

    esp_args, extra_args = esp_argParser.parse_known_args(extra_args)

    skip_partition_resize = esp_args.skip_partition_resize
    partition_size = esp_args.partition_size
    if partition_size == -1:
        app_size = None
    else:
        app_size = partition_size

    nvs_size = esp_args.nvs_size
    ota = esp_args.ota

    if nvs_size is not None:
        partition.nvs_size = nvs_size

    if ota:
        esp_argParser = ArgumentParser(prefix_chars='-')

        esp_argParser.add_argument(
            '--otadata-size',
            dest='otadata',
            help='otadata partition size',
            default=None,
            action='store',
            type=int
        )

        esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
        otadata = esp_args.otadata

        if otadata is not None:
            partition.otadata = otadata

        if app_size is not None:
            partition.ota = app_size

        if not partition.ota:
            partition.ota = partition.factory
    elif board in ('ARDUINO_NANO_ESP32', 'SIL_WESP32'):
        if app_size is not None:
            partition.ota = app_size
    elif app_size is not None:
        partition.factory = app_size

    if board == 'ARDUINO_NANO_ESP32':
        esp_argParser = ArgumentParser(prefix_chars='-')

        esp_argParser.add_argument(
            '--ffat-size',
            dest='ffat',
            help='ffat partition size',
            default=None,
            action='store',
            type=int
        )

        esp_argParser.add_argument(
            '--factory-size',
            dest='factory',
            help='factory partition size',
            default=None,
            action='store',
            type=int
        )

        esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
        factory = esp_args.factory
        if factory is not None:
            partition.factory = factory

        ffat = esp_args.ffat
        if ffat is not None:
            partition.ffat = ffat

    else:
        esp_argParser = ArgumentParser(prefix_chars='-')

        esp_argParser.add_argument(
            '--phyinit-size',
            dest='phy_init',
            help='phy_init partition size',
            default=None,
            action='store',
            type=int
        )

        esp_args, extra_args = esp_argParser.parse_known_args(extra_args)
        phy_init = esp_args.phy_init
        if phy_init is not None:
            partition.phy_init = phy_init

    if lv_cflags:
        lv_cflags += ' -DLV_KCONFIG_IGNORE=1'
    else:
        lv_cflags = '-DLV_KCONFIG_IGNORE=1'

    if not os.path.exists('build'):
        os.mkdir('build')

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


def build_commands(_, extra_args, __, lv_cflags, ___):
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
        'USER_C_MODULES=../../../../../micropython.cmake'
    ])

    esp_cmd.extend(extra_args)

    compile_cmd.extend(esp_cmd[:])
    compile_cmd.pop(1)

    if board_variant:
        clean_cmd.append(f'BOARD_VARIANT={board_variant}')
        compile_cmd.insert(7, f'BOARD_VARIANT={board_variant}')
        submodules_cmd.append(f'BOARD_VARIANT={board_variant}')


def get_idf_version():
    if 'ESP_IDF_VERSION' in os.environ:
        exit_code, data = spawn(
            ['python3', 'idf.py', '--version'],
            out_to_screen=False
        )
        version = data.split('v')[-1].split('-')[0]
        if version:
            return version


SCRIPT_PATH = ''


def build_manifest(
    target, script_dir, lvgl_api, displays, indevs, frozen_manifest
):
    global SCRIPT_PATH

    SCRIPT_PATH = script_dir

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


def clean():
    env = setup_idf_environ()
    if 'deploy' in clean_cmd:
        clean_cmd.remove('deploy')

    spawn(clean_cmd, env=env)


def setup_idf_environ():
    # There were some modifications made with how the environment gets set up
    # @cheops put quite a bit of time in to research the best solution
    # and also with the testing of the code.

    if sys.platform.startswith('win'):
        return None

    idf_ver = get_idf_version()
    env = None
    if idf_ver is None or idf_ver != '5.0.4':
        idf_path = 'lib/esp-idf'

        if os.path.exists(os.path.join(idf_path, 'export.sh')):

            # this removes any IDF environment variable that may
            # exist if the user has the ESP-IDF installed
            env = {
                k: v for k, v in os.environ.items() if not k.startswith('IDF')
            }
            py_path = os.path.split(sys.executable)[0]
            idf_path = os.path.abspath(idf_path)
            idf_tools_path = os.path.join(idf_path, 'tools')
            env['PATH'] = (
                py_path + os.pathsep +
                os.pathsep + idf_tools_path +
                os.pathsep + env.get('PATH', '')
            )
            env['IDF_PATH'] = idf_path

            cmds = [
                [f'export "IDF_PATH={idf_path}"'],
                ['cd', idf_path],
                ['. ./export.sh'],
                ['printenv']
            ]

            if 'GITHUB_RUN_ID' in env:
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

                spawn(env_cmds, env=env, out_to_screen=False)

            result, output = spawn(cmds, env=env, out_to_screen=False)

            if result != 0:
                print('********* ERROR **********')
                print(output)
                sys.exit(result)

            output = [line for line in output.split('\n') if '=' in line]

            temp_env = {
                line.split('=', 1)[0]: line.split('=', 1)[1]
                for line in output
            }

            if 'PATH' in temp_env:
                env['PATH'] = temp_env['PATH']
            elif 'path' in temp_env:
                env['PATH'] = temp_env['path']

        else:
            args = sys.argv[:]

            if 'submodules' not in args:
                args.insert(2, 'submodules')

            args = " ".join(args)

            print('ESP-IDF version 5.0.4 is needed to compile')
            print('Please rerun the build using the command below...')
            print(f'"{sys.executable} {args}"')
            raise RuntimeError

    return env


def submodules():
    if not sys.platform.startswith('win'):
        idf_ver = get_idf_version()

        if idf_ver is None or idf_ver != '5.0.4':
            idf_path = 'lib/esp-idf'
            if not os.path.exists(os.path.join(idf_path, 'export.sh')):
                print('collecting ESP-IDF v5.0.4')
                print('this might take a bit...')
                print()
                get_espidf()
                print()

            cmds = [
                [f'export "IDF_PATH={os.path.abspath(idf_path)}"'],
                ['cd', idf_path],
                [
                    'git', 'submodule', 'update', '--init',
                    'components/bt/host/nimble/nimble',
                    'components/esp_wifi',
                    'components/esptool_py/esptool',
                    'components/lwip/lwip',
                    'components/mbedtls/mbedtls',
                    'components/bt/controller/lib_esp32',
                    'components/bt/controller/lib_esp32c3_family'
                ], ['./install.sh', 'all']]

            print('setting up ESP-IDF v5.0.4')
            print('this might take a bit...')
            print()

            env = {
                k: v for k, v in os.environ.items() if not k.startswith('IDF')
            }
            env['IDF_PATH'] = os.path.abspath(idf_path)

            result, _ = spawn(cmds, env=env)
            if result != 0:
                sys.exit(result)

        env = setup_idf_environ()

        if 'deploy' in submodules_cmd:
            submodules_cmd.remove('deploy')

        cmds = [
            [f'export "IDF_PATH={os.path.abspath(env["IDF_PATH"])}"'],
            ['cd', 'lib/esp-idf'],
            ['. ./export.sh'],
            ['cd ../..'],
            submodules_cmd
        ]
    else:
        raise RuntimeError('compiling on windows is not supported at this time')

    return_code, _ = spawn(cmds, env=env)
    if return_code != 0:
        sys.exit(return_code)


def build_port_config():
    if board in (
        'ESP32_GENERIC',
        'ESP32_GENERIC_C2',
        'ESP32_GENERIC_S2',
        'ESP32_GENERIC_S3'
    ):
        base_config = [
            'CONFIG_ESPTOOLPY_FLASHMODE_QIO=y',
            'CONFIG_ESPTOOLPY_FLASHFREQ_80M=y',
            'CONFIG_ESPTOOLPY_AFTER_NORESET=y',
            'CONFIG_PARTITION_TABLE_CUSTOM=y',
        ]

        if flash_size == '2':
            base_config.append('CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y')
        elif flash_size == '4':
            base_config.append('CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y')
        elif flash_size == '8':
            base_config.append('CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y')
        elif flash_size == '16':
            base_config.append('CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y')
        elif flash_size == '32':
            base_config.append('CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y')
        else:
            raise RuntimeError(f'unsupported flash size {flash_size}')

        if board == 'ESP32_GENERIC_S3':
            if oct_flash and flash_size in ('16', '32'):
                if flash_size == '32':
                    base_config[0] = 'CONFIG_ESPTOOLPY_FLASHMODE_DOUT=y'

                base_config.append('CONFIG_ESPTOOLPY_OCT_FLASH=y')

        base_config = '\n'.join(base_config)

        if board in ('ESP32_GENERIC', 'ESP32_GENERIC_S3'):
            mpconfigboard_cmake_path = (
                'lib/micropython/ports/esp32/boards/'
                f'{board}/mpconfigboard.cmake'
            )

            with open(mpconfigboard_cmake_path, 'rb') as f:
                data = f.read().decode('utf-8')

            if f'boards/{board}/sdkconfig.board' not in data:
                if board == 'ESP32_GENERIC':
                    data = data.replace(
                        'boards/sdkconfig.spiram',
                        'boards/sdkconfig.spiram\n    '
                        'boards/ESP32_GENERIC/sdkconfig.board'
                    )
                else:
                    data = data.replace(
                        'boards/sdkconfig.spiram_ex',
                        'boards/sdkconfig.spiram_ex\n    '
                        'boards/ESP32_GENERIC_S2/sdkconfig.board'
                    )

                with open(mpconfigboard_cmake_path, 'wb') as f:
                    f.write(data.encode('utf-8'))

            sdkconfig_spiram_path = (
                'lib/micropython/ports/esp32/boards/'
                f'{board}/sdkconfig.board'
            )
            with open(sdkconfig_spiram_path, 'w') as f:
                f.write(base_config)

        else:
            sdkconfig_board_path = (
                'lib/micropython/ports/esp32/'
                f'boards/{board}/sdkconfig.board'
            )
            with open(sdkconfig_board_path, 'w') as f:
                f.write(base_config + '\n')

    sdk_config_path = (
        'lib/micropython/ports/esp32/'
        f'boards/{board}/sdkconfig.board'
    )

    if not os.path.exists(sdk_config_path):
        sdk_config_path = (
            'lib/micropython/ports/esp32/'
            f'boards/sdkconfig.base'
        )

    with open(sdk_config_path, 'r') as f:
        data = f.read().split('\n')

    if data[-1].startswith('CONFIG_PARTITION_TABLE_CUSTOM_FILENAME'):
        data[-1] = (
            f'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME={SCRIPT_PATH}'
            f'/build/partition.csv'
        )
    else:
        data.append(
            f'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME={SCRIPT_PATH}'
            f'/build/partition.csv'
        )

    with open(sdk_config_path, 'w') as f:
        f.write('\n'.join(data))


def update_c_files():
    if board in ('ESP32_GENERIC_S2', 'ESP32_GENERIC_S3'):
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


def compile():  # NOQA
    env = setup_idf_environ()

    build_port_config()
    update_c_files()
    partition.build()

    if 'deploy' in compile_cmd:
        if not skip_partition_resize:
            compile_cmd.remove('deploy')
            deploy = True
        else:
            deploy = False
    else:
        deploy = False

    if not sys.platform.startswith('win'):
        cmds = [
            [f'export "IDF_PATH={os.path.abspath(env["IDF_PATH"])}"'],
            ['cd', 'lib/esp-idf'],
            ['. ./export.sh'],
            ['cd ../..'],
            compile_cmd
        ]
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

        end = output.split('(overflow ', 1)[-1]
        overflow_amount = int(end.split(')', 1)[0], 16)

        if partition.ota:
            partition.ota += overflow_amount
        else:
            partition.factory += overflow_amount

        partition.build()

        sys.stdout.write(
            '\n\033[31;1m***** Running build again *****\033[0m\n\n'
        )
        sys.stdout.flush()

        if deploy:
            compile_cmd.append('deploy')

        compile_cmd[4] = 'SECOND_BUILD=1'
        ret_code, output = spawn(cmds, env=env, cmpl=True)

        if ret_code != 0:
            sys.exit(ret_code)

    elif not skip_partition_resize:
        if 'build complete' in output:
            remaining = output.rsplit('application')[-1]
            remaining = int(
                remaining.split('(', 1)[-1].split('remaining')[0].strip()
            )

            if remaining > 4096:
                sys.stdout.write(
                    '\n\033[31;1m***** Resizing Partition *****\033[0m\n'
                )
                sys.stdout.flush()

                if partition.ota:
                    partition.ota += -remaining
                else:
                    partition.factory += -remaining

                partition.build()

                sys.stdout.write(
                    '\n\033[31;1m***** Running build again *****\033[0m\n\n'
                )
                sys.stdout.flush()

                if deploy:
                    compile_cmd.append('deploy')

                if remaining > 4096 or deploy:
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

        if board_variant:
            build_name += f'-{board_variant}'

        build_bin_file = os.path.abspath(
            f'lib/micropython/ports/esp32/{build_name}'
        )
        build_bin_file = os.path.join(build_bin_file, f'{build_name}.bin')
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

        print()
        print()
        print('To flash firmware:')
        print('Replace "(PORT)" with the serial port for your esp32')
        print('and run the commands.')
        print()
        print(
            python_path, esp_tool_path, '-p (PORT) -b 460800 erase_flash'
        )

        cmd = f'{python_path} {esp_tool_path} {out_cmd}'
        cmd = cmd.split('write_flash', 1)[0]
        cmd += f'write_flash 0x0 {build_bin_file}'

        print()
        print(cmd.replace('-b 460800', '-b 921600'))
        print()


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd)
    if return_code != 0:
        sys.exit(return_code)
