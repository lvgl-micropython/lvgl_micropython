# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.

add_definitions(-DMICROPY_PY_THREAD_GIL=0)

add_library(usermod_freertos INTERFACE)

set(MOD_INCLUDES ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/include)

# gets esp_lcd include paths
idf_component_get_property(FREERTOS_INCLUDES freertos INCLUDE_DIRS)
idf_component_get_property(FREERTOS_DIR freertos COMPONENT_DIR)

# sets the include paths into INCLUDES variable
if(FREERTOS_INCLUDES)
    list(TRANSFORM FREERTOS_INCLUDES PREPEND ${FREERTOS_DIR}/)
    list(APPEND MOD_INCLUDES ${FREERTOS_INCLUDES})
endif()

set(FREERTOS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/freertos_mod.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_atomic.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_event_groups.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_idf_additions.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_portmacro.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_projdefs.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_queue.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_semphr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_stream_buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_message_buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_task.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_task_notify.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mpy_timers.c
)

# Add our source files to the lib
target_sources(usermod_freertos INTERFACE ${FREERTOS_SOURCES})

# Add include directories.
target_include_directories(usermod_freertos INTERFACE ${MOD_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_freertos)
