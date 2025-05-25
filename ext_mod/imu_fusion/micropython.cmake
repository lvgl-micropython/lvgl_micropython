# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.
add_library(usermod_fusion INTERFACE)

set(FUSION_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/include
)

set(FUSION_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/fusion.c
)


# Add our source files to the lib
target_sources(usermod_fusion INTERFACE ${FUSION_SOURCES})

# Add include directories.
target_include_directories(usermod_fusion INTERFACE ${FUSION_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_fusion)
