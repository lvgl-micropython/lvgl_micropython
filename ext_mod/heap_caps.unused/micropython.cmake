# Create an INTERFACE library for our C module.

add_library(usermod_heap_caps INTERFACE)

set(INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
)

if(ESP_PLATFORM)
    set(SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/heap_caps.c
    )
else()
    set(SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/common_src/heap_caps.c
    )
endif(ESP_PLATFORM)

# Add our source files to the lib
target_sources(usermod_heap_caps INTERFACE ${SOURCES})

# Add include directories.
target_include_directories(usermod_heap_caps INTERFACE ${INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_heap_caps)
