include(${CMAKE_CURRENT_LIST_DIR}/lcd_bus/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/lvgl/micropython.cmake)

if(ESP_PLATFORM)
    include(${CMAKE_CURRENT_LIST_DIR}/esp32_additions/micropython.cmake)
endif(ESP_PLATFORM)
