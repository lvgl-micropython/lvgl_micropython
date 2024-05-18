include(${CMAKE_CURRENT_LIST_DIR}/lcd_bus/micropython.cmake)

if(ESP_PLATFORM)
    include(${CMAKE_CURRENT_LIST_DIR}/expanded_nvs_esp32/micropython.cmake)
endif(ESP_PLATFORM)
