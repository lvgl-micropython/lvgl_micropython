# This file is to be given as "make USER_C_MODULES=..." when building Micropython port

find_package(Python3 REQUIRED COMPONENTS Interpreter)

get_filename_component(BINDING_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)

set(LVGL_HEADER "${BINDING_DIR}/build/lvgl_header.h")
set(FAKE_LIBC_PATH "${BINDING_DIR}/gen/fake_libc")

separate_arguments(LV_CFLAGS_ENV UNIX_COMMAND $ENV{LV_CFLAGS})
separate_arguments(SECOND_BUILD_ENV UNIX_COMMAND $ENV{SECOND_BUILD})

list(APPEND LV_CFLAGS_ENV
    -std=c11
    -Wno-incompatible-pointer-types
    -I${BINDING_DIR}/lib
    -I${BINDING_DIR}/lib/lvgl
)

list(APPEND LV_CFLAGS ${LV_CFLAGS_ENV})
list(APPEND LV_CFLAGS
    -DMICROPY_FLOAT=1
)

list(APPEND LV_PP_CFLAGS ${LV_CFLAGS_ENV})
list(APPEND LV_PP_CFLAGS
    -E
    -I{FAKE_LIBC_PATH}
    -Wno-unused-function
    -DPYCPARSER
)


# file(GLOB_RECURSE LVGL_HEADERS ${BINDING_DIR}/lib/lvgl/src/*.h ${BINDING_DIR}/lib/lv_conf.h)

# this MUST be an execute_process because of the order in which cmake does things
# if add_custom_command is used it errors becasue add_custom_command doesn't
# actually run before the lv_mp.c file gets added to the source list. That causes
# the compilation to error because the source file doesn't exist. It needs to
# exist before it gets added to the source list and this is the only way I have
# found to go about doing it.

if(${SECOND_BUILD_ENV} EQUAL "0")
    execute_process(
        COMMAND
            ${CMAKE_C_COMPILER} ${LV_PP_CFLAGS} ${LVGL_HEADER}  >> ${CMAKE_BINARY_DIR}/lv_mp.pp
        WORKING_DIRECTORY 
            ${CMAKE_CURRENT_LIST_DIR}
        RESULT_VARIABLE mpy_pp_result
        OUTPUT_VARIABLE mpy_pp_output
    )
    
    if(${mpy_pp_result} GREATER "0")
        message("OUTPUT: ${mpy_pp_output}")
        message("RESULT: ${mpy_pp_result}")
        message(FATAL_ERROR "Failed to generate ${CMAKE_BINARY_DIR}/lv_mp.pp")
    endif()

    execute_process(
        COMMAND
            ${Python3_EXECUTABLE} ${BINDING_DIR}/gen/$ENV{GEN_SCRIPT}_api_gen_mpy.py --cflags="${LV_PP_CFLAGS}" --output=${CMAKE_BINARY_DIR}/lv_mp.c --pp_file=${CMAKE_BINARY_DIR}/lv_mp.pp --board=$ENV{LV_PORT} --module_name=lvgl --module_prefix=lv --metadata=${CMAKE_BINARY_DIR}/lv_mp.c.json --header_file=${LVGL_HEADER}
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_LIST_DIR}

        RESULT_VARIABLE mpy_result
        OUTPUT_VARIABLE mpy_output
    )

    if(${mpy_result} GREATER "0")
        message("OUTPUT: ${mpy_output}")
        message("RESULT: ${mpy_result}")
       message(FATAL_ERROR "Failed to generate ${CMAKE_BINARY_DIR}/lv_mp.c")
    endif()
endif()

# file(WRITE ${CMAKE_BINARY_DIR}/lv_mp.c ${mpy_output})

file(GLOB_RECURSE LVGL_SOURCES ${BINDING_DIR}/lib/lvgl/src/*.c)
list(APPEND LVGL_SOURCES
    ${BINDING_DIR}/ext_mod/lvgl/mem_core.c
)

add_library(lvgl_interface INTERFACE)

target_sources(lvgl_interface INTERFACE ${LVGL_SOURCES})
target_compile_options(lvgl_interface INTERFACE ${LV_CFLAGS})

set(LVGL_MPY_INCLUDES
    ${BINDING_DIR}/lib/micropython
    ${BINDING_DIR}/lib
    ${BINDING_DIR}/lib/lvgl
)

add_library(usermod_lvgl INTERFACE)
target_sources(usermod_lvgl INTERFACE ${CMAKE_BINARY_DIR}/lv_mp.c)
target_include_directories(usermod_lvgl INTERFACE ${LVGL_MPY_INCLUDES})
target_link_libraries(usermod_lvgl INTERFACE lvgl_interface)
target_link_libraries(usermod INTERFACE usermod_lvgl)
