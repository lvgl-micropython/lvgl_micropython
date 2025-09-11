# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.

add_library(usermod_lcd_bus INTERFACE)


set(LCD_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/common/include
    ${CMAKE_CURRENT_LIST_DIR}/$ENV{LV_PORT}/include
)

file(GLOB LCD_SOURCES ${CMAKE_CURRENT_LIST_DIR}/$ENV{LV_PORT}/src/*.c)
list(APPEND LCD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/common/src/modlcd_bus.c
    ${CMAKE_CURRENT_LIST_DIR}/common/src/lcd_types.c
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

endif(ESP_PLATFORM)

# Add our source files to the lib
target_sources(usermod_lcd_bus INTERFACE ${LCD_SOURCES})

# Add include directories.
target_include_directories(usermod_lcd_bus INTERFACE ${LCD_INCLUDES})

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_lcd_bus)
