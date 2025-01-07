# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.

add_library(usermod_lcd_utils INTERFACE)

set(LCD_UTILS_INCLUDES)

set(LCD_UTILS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/lcd_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/src/remap.c
    ${CMAKE_CURRENT_LIST_DIR}/src/binary_float.c
)

# Add our source files to the lib
target_sources(usermod_lcd_utils INTERFACE ${LCD_UTILS_SOURCES})

# Add include directories.
target_include_directories(usermod_lcd_utils INTERFACE ${LCD_UTILS_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_lcd_utils)
