# Create an INTERFACE library for our C module.

add_library(usermod_lcd_bus INTERFACE)

if(ESP_PLATFORM)
    set(LCD_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/esp32_include
    )

    set(LCD_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/modlcd_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/lcd_types.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/i2c_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/spi_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/i80_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/rgb_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32_src/spi_3wire.c
    )

    # gets esp_io_expander include paths
    idf_component_get_property(ESP_IO_EXPANDER_INCLUDES esp_io_expander INCLUDE_DIRS)
    idf_component_get_property(ESP_IO_EXPANDER_DIR esp_io_expander COMPONENT_DIR)

    if(ESP_IO_EXPANDER_INCLUDES)
        list(TRANSFORM ESP_IO_EXPANDER_INCLUDES PREPEND ${ESP_IO_EXPANDER_DIR}/)
        list(APPEND LCD_INCLUDES ${ESP_IO_EXPANDER_INCLUDES})
    endif()

    # gets esp_lcd_panel_io_additions include paths
    idf_component_get_property(ESP_IO_ADDITIONS_INCLUDES esp_lcd_panel_io_additions INCLUDE_DIRS)
    idf_component_get_property(ESP_IO_ADDITIONS_DIR esp_lcd_panel_io_additions COMPONENT_DIR)

    if(ESP_IO_ADDITIONS_INCLUDES)
        list(TRANSFORM ESP_IO_ADDITIONS_INCLUDES PREPEND ${ESP_IO_ADDITIONS_DIR}/)
        list(APPEND LCD_INCLUDES ${ESP_IO_ADDITIONS_INCLUDES})
    endif()

    # gets esp_lcd include paths
    idf_component_get_property(ESP_LCD_INCLUDES esp_lcd INCLUDE_DIRS)
    idf_component_get_property(ESP_LCD_DIR esp_lcd COMPONENT_DIR)

    # sets the include paths into INCLUDES variable
    if(ESP_LCD_INCLUDES)
        list(TRANSFORM ESP_LCD_INCLUDES PREPEND ${ESP_LCD_DIR}/)
        list(APPEND LCD_INCLUDES ${ESP_LCD_INCLUDES})
    endif()

else()
    set(LCD_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/common_include
        ${CMAKE_CURRENT_LIST_DIR}/sdl_bus
    )

    set(LCD_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/lcd_types.c
        ${CMAKE_CURRENT_LIST_DIR}/modlcd_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/i2c_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/spi_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/i80_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/common_src/rgb_bus.c
        ${CMAKE_CURRENT_LIST_DIR}/sdl_bus/sdl_bus.c
    )

endif(ESP_PLATFORM)


# Add our source files to the lib
target_sources(usermod_lcd_bus INTERFACE ${LCD_SOURCES})

# Add include directories.
target_include_directories(usermod_lcd_bus INTERFACE ${LCD_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_lcd_bus)
