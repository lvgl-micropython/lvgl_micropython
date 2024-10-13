include(${CMAKE_CURRENT_LIST_DIR}/lcd_bus/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/lvgl/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/lcd_utils/micropython.cmake)

if(ESP_PLATFORM)
    include(${CMAKE_CURRENT_LIST_DIR}/threading/micropython.cmake)
endif(ESP_PLATFORM)
