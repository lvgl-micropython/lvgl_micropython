# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Create an INTERFACE library for our C module.

if(ESP_PLATFORM)
    add_library(usermod_led_strip INTERFACE)

    set(LED_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/esp32/include
    )

    set(LED_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/esp32/src/led_strip.c
        ${CMAKE_CURRENT_LIST_DIR}/esp32/src/led_strip_encoder.c
    )

    # Add our source files to the lib
    target_sources(usermod_led_strip INTERFACE ${LED_SOURCES})

    # Add include directories.
    target_include_directories(usermod_led_strip INTERFACE ${LED_INCLUDES})

    # Link our INTERFACE library to the usermod target.
    target_link_libraries(usermod INTERFACE usermod_led_strip)
endif(ESP_PLATFORM)
