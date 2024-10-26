# Copyright (C) 2024  Kevin G Schlosser
# Code that is written by the above named is done under the GPL license
# and that license is able to be viewed in the LICENSE file in the root
# of this project.

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
