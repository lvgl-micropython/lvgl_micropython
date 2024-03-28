include(${CMAKE_CURRENT_LIST_DIR}/lcd_bus/micropython.cmake)

if(ESP_PLATFORM)
    include(${CMAKE_CURRENT_LIST_DIR}/esp_i2c/micropython.cmake)
endif(ESP_PLATFORM)

