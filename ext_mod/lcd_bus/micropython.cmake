# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.

add_library(usermod_lcd_bus INTERFACE)

set(PORT_DIR ${CMAKE_CURRENT_LIST_DIR}/$ENV{LV_PORT})
set(LCD_INCLUDES ${CMAKE_CURRENT_LIST_DIR} ${PORT_DIR}/include)

set(LCD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/lcd_types.c
    ${CMAKE_CURRENT_LIST_DIR}/modlcd_bus.c
    ${PORT_DIR}/src/i2c_bus.c
    ${PORT_DIR}/src/i80_bus.c
    ${PORT_DIR}/src/spi_bus.c
    ${PORT_DIR}/src/rgb_bus.c
    ${PORT_DIR}/src/sdl_bus.c
    ${PORT_DIR}/src/dsi_bus.c
)


if(ESP_PLATFORM)
    # gets esp_lcd include paths
    idf_component_get_property(ESP_LCD_INCLUDES esp_lcd INCLUDE_DIRS)
    idf_component_get_property(ESP_LCD_DIR esp_lcd COMPONENT_DIR)

    # sets the include paths into INCLUDES variable
    if(ESP_LCD_INCLUDES)
        list(TRANSFORM ESP_LCD_INCLUDES PREPEND ${ESP_LCD_DIR}/)
        list(APPEND LCD_INCLUDES ${ESP_LCD_INCLUDES})
    endif()

    list(APPEND LCD_SOURCES
        ${PORT_DIR}/src/rgb565_dither.c
        ${PORT_DIR}/src/rgb_bus_rotation.c
    )

endif(ESP_PLATFORM)

# Add our source files to the lib
target_sources(usermod_lcd_bus INTERFACE ${LCD_SOURCES})

# Add include directories.
target_include_directories(usermod_lcd_bus INTERFACE ${LCD_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_lcd_bus)
