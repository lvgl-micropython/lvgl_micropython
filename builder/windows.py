# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import sys
import os
import uuid
from argparse import ArgumentParser
from . import spawn
from . import generate_manifest
from . import update_mphalport
from . import setup_windows_build


mpy_cross_cmd = []
clean_cmd = []
compile_cmd = []
submodules_cmd = []
heap_size = 4194304


def parse_args(extra_args, lv_cflags, board):
    global heap_size
    unix_argParser = ArgumentParser(prefix_chars='-')

    unix_argParser.add_argument(
        '--heap-size',
        dest='heap_size',
        help="heap size to use in bytes. Default is 4mb (4,194,304 bytes). "
             "Must be more than 100k (102,104 bytes)",
        default=4194304,
        type=int,
        action='store'
    )
    unix_args, extra_args = unix_argParser.parse_known_args(extra_args)

    if unix_args.heap_size < 102400:
        raise RuntimeError('heap size is too low, must be >= 102,104 bytes')

    heap_size = unix_args.heap_size

    return extra_args, lv_cflags, board


variant = None

LV_CFLAGS = ''


def build_commands(_, extra_args, __, lv_cflags, board):
    global variant
    global LV_CFLAGS

    if not board:
        board = 'dev'

    variant = board

    if lv_cflags:
        LV_CFLAGS = lv_cflags

    if sys.platform.startswith('win'):
        env = setup_windows_build()

        mpy_cross_cmd.extend([
            f'"{env.visual_c.msbuild_path}"',
            'lib/micropython/mpy-cross/mpy-cross.vcxproj'
        ])
        compile_cmd.extend([
            f'"{env.visual_c.msbuild_path}"',
            'lib/micropython/ports/windows/micropython.vcxproj',
        ])

        if board:
            compile_cmd.append(f'/p:PyVariant={board}')

    else:
        mpy_cross_cmd.extend(['make', '-C', 'lib/micropython/mpy-cross'])
        clean_cmd.extend([
            'make',
            'clean',
            '-C',
            'lib/micropython/ports/windows'
            'LV_PORT=windows'
        ])

        compile_cmd.extend([
            'make',
            '-C',
            f'lib/micropython/ports/windows'
            f'LV_PORT=windows',
        ])

    # if lv_cflags:
    #     clean_cmd.append(f'LV_CFLAGS="{lv_cflags}"')
    #     compile_cmd.append(f'LV_CFLAGS="{lv_cflags}"')

    # clean_cmd.append(f'USER_C_MODULES="{script_dir}/ext_mod"')
    # compile_cmd.append(f'USER_C_MODULES="{script_dir}/ext_mod"')

    # clean_cmd.extend(extra_args)
    # compile_cmd.extend(extra_args)
    return extra_args


SCRIPT_PATH = ''


def build_manifest(
    target,
    script_dir,
    lvgl_api,
    displays,
    indevs,
    expanders,
    imus,
    frozen_manifest
):
    global SCRIPT_PATH

    SCRIPT_PATH = script_dir

    update_mphalport(target)
    
    manifest_path = 'lib/micropython/ports/windows/variants/manifest.py'

    generate_manifest(script_dir, lvgl_api, manifest_path, displays,
                      indevs, expanders, imus, frozen_manifest)


def force_clean(clean_mpy_cross):
    if clean_mpy_cross:
        cross_clean = mpy_cross_cmd[:]
        cross_clean.insert(1, 'clean')
        spawn(cross_clean)

    spawn(clean_cmd)


def submodules():
    pass


def _run(c, spinner=False, cmpl=False):
    res, _ = spawn(c, spinner=spinner, cmpl=cmpl)
    if res != 0:
        sys.exit(res)


def build_sdl():
    global variant

    if variant is None:
        variant = 'build-standard'
    else:
        variant = f'build-{variant}'

    dst = f'lib/micropython/ports/unix/{variant}/SDL'

    if not os.path.exists(dst):
        os.makedirs(dst)
    elif os.path.exists(os.path.join(dst, 'libSDL2.a')):
        return

    cwd = os.getcwd()
    os.chdir(dst)
    cmd_ = [
        f'cmake -DSDL_STATIC=ON -DSDL_SHARED=OFF '
        f'-DCMAKE_BUILD_TYPE=Release {SCRIPT_PATH}/lib/SDL &&'
        f'cmake --build . --config Release --parallel {os.cpu_count()}'
    ]

    res, _ = spawn(cmd_, cmpl=True)
    if res != 0:
        sys.exit(res)

    os.chdir(cwd)


portable_filter_path_templ = '''\
    <Filter Include="{p}">
      <UniqueIdentifier>{{{guid}}}</UniqueIdentifier>
    </Filter>
'''

portable_filter_include_templ = '''\
    <ClInclude Include="{file_path}">
      <Filter>{p}</Filter>
    </ClInclude>
'''

portable_filter_source_templ = '''\
    <ClCompile Include="{file_path}">
      <Filter>{p}</Filter>
    </ClCompile>
'''

portable_source_templ = '''\
    <ClCompile Include="{file_path}" />
'''

portable_include_templ = '''\
    <ClInclude Include="{file_path}" />
'''



def compile(*args):  # NOQA
    main_path = 'lib/micropython/ports/unix/main.c'
    build_path = f'lib/micropython/ports/windows/build-{variant}'

    with open(main_path, 'rb') as f:
        main = f.read().decode('utf-8').split('\n')

    for i, line in enumerate(main):
        if line.startswith('long heap_size ='):
            main[i] = f'long heap_size = {heap_size};'
            break

    with open(main_path, 'wb') as f:
        f.write('\n'.join(main).encode('utf-8'))

    base_path = os.path.abspath(os.path.dirname(__file__))

    lib_path = os.path.join(SCRIPT_PATH, 'lib')
    lvgl_path = os.path.join(lib_path, 'lvgl')
    lvgl_src_path = os.path.join(lvgl_path, 'src')
    msbuild_lvgl_path = os.path.join(base_path, 'msbuild_lvgl')

    ext_mod_path = os.path.join(SCRIPT_PATH, 'ext_mod')
    lvgl_addon_path = os.path.join(ext_mod_path, 'lvgl_addons')
    lvgl_addon_include = os.path.join(lvgl_addon_path, 'include')
    lvgl_addon_src = os.path.join(lvgl_addon_path, 'src')

    portable_filter_paths = [
        portable_filter_path_templ.format(guid=str(uuid.uuid4()), p=lvgl_path),
        portable_filter_path_templ.format(guid=str(uuid.uuid4()), p=lib_path),
        portable_filter_path_templ.format(
            guid=str(uuid.uuid4()),
            p=lvgl_addon_include
        )
    ]
    portable_filter_includes = [
        portable_filter_include_templ.format(
            p=lvgl_path,
            file_path=os.path.join(lvgl_path, 'lvgl.h')
        ),
        portable_filter_include_templ.format(
            p=lib_path,
            file_path=os.path.join(lib_path, 'lv_conf.h')
        ),
        portable_filter_include_templ.format(
            p=lvgl_addon_include,
            file_path=os.path.join(lvgl_addon_include, 'color_addons.h')
        )
    ]

    portable_filter_sources = [
        portable_filter_source_templ.format(
            p=lvgl_addon_src,
            file_path=os.path.join(lvgl_addon_src, 'color_addons.c')
        )
    ]

    portable_includes = [
        portable_include_templ.format(
            file_path=os.path.join(lvgl_path, 'lvgl.h')
        ),
        portable_include_templ.format(
            file_path=os.path.join(lib_path, 'lv_conf.h')
        ),
        portable_include_templ.format(
            file_path=os.path.join(lvgl_addon_include, 'color_addons.h')
        )
    ]
    portable_sources = [
        portable_source_templ.format(
            file_path=os.path.join(lvgl_addon_src, 'color_addons.c')
        )
    ]

    def _iter_files(p):
        portable_filter_paths.append(
            portable_filter_path_templ.format(guid=str(uuid.uuid4()), p=p)
        )

        for file in os.listdir(p):
            file_path = os.path.join(p, file)
            if os.path.isdir(file_path):
                _iter_files(file_path)
            elif file.endswith('.h'):
                portable_filter_includes.append(
                    portable_filter_include_templ.format(
                        p=p,
                        file_path=file_path
                    )
                )
                portable_includes.append(
                    portable_include_templ.format(file_path=file_path)
                )
            elif file.endswith('.c'):
                portable_filter_sources.append(
                    portable_filter_source_templ.format(
                        p=p,
                        file_path=file_path
                    )
                )
                portable_sources.append(
                    portable_source_templ.format(file_path=file_path)
                )

    _iter_files(lvgl_src_path)

    def _write_file(p, **kwargs):
        with open(p, 'r') as fle:
            data = fle.read()

        if f'{list(kwargs.keys())[0]}' in data:
            with open(p, 'w') as fle:
                fle.write(data.format(**kwargs))

    lvgl_portable_vcxitems_filters_path = (
        os.path.join(msbuild_lvgl_path, 'lvgl_portable.vcxitems.filters')
    )
    _write_file(
        lvgl_portable_vcxitems_filters_path,
        sources=''.join(portable_filter_sources),
        includes=''.join(portable_filter_includes),
        paths=''.join(portable_filter_paths)
    )

    lvgl_portable_vcxitems_path = (
        os.path.join(msbuild_lvgl_path, 'lvgl_portable.vcxitems')
    )
    _write_file(
        lvgl_portable_vcxitems_path,
        sources=''.join(portable_sources),
        includes=''.join(portable_includes)
    )

    lvgl_windows_props_path = (
        os.path.join(msbuild_lvgl_path, 'lvgl_windows.props')
    )
    _write_file(
        lvgl_windows_props_path,
        LVGL_PATH=lvgl_path
    )

    lvgl_windows_vcxproj_path = (
        os.path.join(msbuild_lvgl_path, 'lvgl_windows.vcxproj')
    )
    _write_file(
        lvgl_windows_vcxproj_path,
        includes=portable_include_templ.format(
            file_path=os.path.join(lib_path, 'lv_conf.h')
        )
    )

    lvgl_windows_vcxproj_filters_path = (
        os.path.join(msbuild_lvgl_path, 'lvgl_windows.vcxproj.filters')
    )
    _write_file(
        lvgl_windows_vcxproj_filters_path,
        includes=portable_include_templ.format(
            file_path=os.path.join(lib_path, 'lv_conf.h')
        )
    )

    lcd_bus_path = os.path.join(ext_mod_path, 'lcd_bus')
    lcd_bus_common_include = os.path.join(lcd_bus_path, 'common_include')
    lcd_bus_common_src = os.path.join(lcd_bus_path, 'common_src')
    lcd_bus_sdl = os.path.join(lcd_bus_path, 'sdl_bus')

    sources = [
        r'    <ClCompile Include="$(PyVariantDir)*.c" />',
        fr'    <ClCompile Include="{build_path}\lv_mp.c" />',
        fr'    <ClCompile Include="{lcd_bus_common_include}\*.c" />',
        fr'    <ClCompile Include="{lcd_bus_sdl}\*.c" />',
        fr'    <ClCompile Include="{lcd_bus_path}\*.c" />',
    ]

    lv_h = os.path.abspath("build/lvgl_header.h")

    includes = [
        r'    <ClInclude Include="$(PyBaseDir)ports\windows\msvc\*.h" />',
        fr'    <ClInclude Include="{lv_h}" />',
        fr'    <ClInclude Include="{lcd_bus_common_src}\*.h" />',
        fr'    <ClInclude Include="{lcd_bus_sdl}\*.h" />',
        fr'    <ClInclude Include="{lcd_bus_path}\*.h" />',
    ]
    includes = '\n'.join(includes)
    sources = '\n'.join(sources)

    include_paths = [
        '$(PyIncDirs)',
        '%(AdditionalIncludeDirectories)',
        os.path.abspath('build'),
        lcd_bus_path,
        lcd_bus_common_include,
        lcd_bus_sdl,
        lvgl_addon_include
    ]

    include_paths = ';'.join(include_paths)

    common_props_path = 'lib/micropython/ports/wwindows/msvc/common.props'
    with open(common_props_path, 'r') as f:
        dta = f.read()

    if include_paths not in dta:
        dta = dta.replace(
            '$(PyIncDirs);%(AdditionalIncludeDirectories)',
            include_paths
        )

    macro = '<PreprocessorDefinitions>MP_PORT_UNIX;_USE_MATH_DEFINES;'
    if macro not in dta:
        dta = dta.replace(
            '<PreprocessorDefinitions>_USE_MATH_DEFINES;',
            macro
        )

    link = '<AdditionalDependencies>sdl2.lib;lvgl_windows.lib;Bcrypt.lib;'
    if link not in dta:
        dta = dta.replace('<AdditionalDependencies>Bcrypt.lib;', link)

    with open(common_props_path, 'w') as f:
        f.write(dta)

    micropython_vcxproj_path = (
        'lib/micropython/ports/wwindows/micropython.vcxproj'
    )
    with open(micropython_vcxproj_path, 'r') as f:
        dta = f.read()

    if includes not in dta:
        dta = dta.replace(
            r'    <ClInclude Include="$(PyBaseDir)ports'
            r'\windows\msvc\*.h" />',
            includes
        )

    if sources not in dta:
        dta = dta.replace(
            r'    <ClCompile Include="$(PyVariantDir)*.c" />',
            sources
        )

    with open(micropython_vcxproj_path, 'w') as f:
        f.write(dta)

    cmd = [
        sys.executable,
        'gen/python_api_gen_mpy.py'
    ]

    cmd.extend(LV_CFLAGS.split(' '))
    cmd.extend([
        f'--output={build_path}/lv_mp.c',
        '--include=.',
        '--include=include',
        '--include=lib/lvgl',
        '--board=windows',
        '--module_name=lvgl',
        '--module_prefix=lv',
        f'--metadata={build_path}/lv_mp.c.json',
        f'--header_file={os.path.abspath("build/lvgl_header.h")}'
    ])

    # mpconfigvariant_common_path = (
    #     'lib/micropython/ports/windows/variants/mpconfigvariant_common.h'
    # )
    #
    # with open(mpconfigvariant_common_path, 'r') as f:
    #     mpconfigvariant_common = f.read()
    #
    # if (
    #     '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)' in
    #     mpconfigvariant_common
    # ):
    #     mpconfigvariant_common = mpconfigvariant_common.replace(
    #         '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)',
    #         '#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (0)'
    #     )
    #
    #     with open(mpconfigvariant_common_path, 'w') as f:
    #         f.write(mpconfigvariant_common)
    #
    # if '#define MICROPY_MEM_STATS              (1)' in mpconfigvariant_common:
    #     mpconfigvariant_common = mpconfigvariant_common.replace(
    #         '#define MICROPY_MEM_STATS              (1)',
    #         '#define MICROPY_MEM_STATS              (0)'
    #     )
    #
    #     with open(mpconfigvariant_common_path, 'w') as f:
    #         f.write(mpconfigvariant_common)
    #
    # if '#define MICROPY_SCHEDULER_DEPTH
    # (128)' not in mpconfigvariant_common:
    #     mpconfigvariant_common += '\n\n'
    #     mpconfigvariant_common +=
    #     '#define MICROPY_SCHEDULER_DEPTH              (128)\n'
    #
    #     with open(mpconfigvariant_common_path, 'w') as f:
    #         f.write(mpconfigvariant_common)
    #
    # if '#define MICROPY_STACK_CHECK
    # (0)' not in mpconfigvariant_common:
    #     mpconfigvariant_common += '\n'
    #     mpconfigvariant_common +=
    #     '#define MICROPY_STACK_CHECK              (0)\n'
    #
    #     with open(mpconfigvariant_common_path, 'w') as f:
    #         f.write(mpconfigvariant_common)

    build_sdl()

    return_code, _ = spawn(cmd)
    if return_code != 0:
        sys.exit(return_code)

    return_code, _ = spawn(compile_cmd)
    if return_code != 0:
        sys.exit(return_code)


def mpy_cross():
    return_code, _ = spawn(mpy_cross_cmd, cmpl=True)
    if return_code != 0:
        sys.exit(return_code)
