# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.
add_library(usermod_spi3wire INTERFACE)

set(SPI3WIRE_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
)

set(SPI3WIRE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/spi3wire.c
    ${CMAKE_CURRENT_LIST_DIR}/src/esp_lcd_panel_io_3wire_spi.c
)


# Add our source files to the lib
target_sources(usermod_spi3wire INTERFACE ${SPI3WIRE_SOURCES})

# Add include directories.
target_include_directories(usermod_spi3wire INTERFACE ${SPI3WIRE_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_spi3wire)
