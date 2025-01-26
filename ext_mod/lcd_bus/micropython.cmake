# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.

add_library(usermod_lcd_bus INTERFACE)

set(LCD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/common/lcd_framebuf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/common/modlcd_bus.c
    ${CMAKE_CURRENT_LIST_DIR}/src/common/rgb565_dither.c
    ${CMAKE_CURRENT_LIST_DIR}/src/common/sw_rotate.c
    ${CMAKE_CURRENT_LIST_DIR}/src/common/sw_rotate_task_common.c
)

set(LCD_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/include
)


if(ESP_PLATFORM)
    list(APPEND LCD_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include/esp32)

    list(APPEND LCD_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/esp32/i2c_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/esp32/spi_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/esp32/i80_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/esp32/rgb_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/esp32/lcd_bus_utils.c
        ${CMAKE_CURRENT_LIST_DIR}/src/esp32/lcd_types.c
        ${CMAKE_CURRENT_LIST_DIR}/src/esp32/sw_rotate_task.c
    )

    # gets esp_lcd include paths
    idf_component_get_property(ESP_LCD_INCLUDES esp_lcd INCLUDE_DIRS)
    idf_component_get_property(ESP_LCD_DIR esp_lcd COMPONENT_DIR)

    # sets the include paths into INCLUDES variable
    if(ESP_LCD_INCLUDES)
        list(TRANSFORM ESP_LCD_INCLUDES PREPEND ${ESP_LCD_DIR}/)
        list(APPEND LCD_INCLUDES ${ESP_LCD_INCLUDES})
    endif()

else()
    list(APPEND LCD_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/include/other_mcus)

    list(APPEND LCD_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/other_mcus/i2c_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/other_mcus/spi_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/other_mcus/i80_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/other_mcus/rgb_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/src/other_mcus/lcd_bus_utils.c
        ${CMAKE_CURRENT_LIST_DIR}/src/other_mcus/lcd_types.c
        ${CMAKE_CURRENT_LIST_DIR}/src/other_mcus/sw_rotate_task.c
    )

endif(ESP_PLATFORM)


# Add our source files to the lib
target_sources(usermod_lcd_bus INTERFACE ${LCD_SOURCES})

# Add include directories.
target_include_directories(usermod_lcd_bus INTERFACE ${LCD_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_lcd_bus)
