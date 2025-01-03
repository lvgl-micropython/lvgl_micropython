if(ESP_PLATFORM)
    include(${CMAKE_CURRENT_LIST_DIR}/esp32_components.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/spi3wire/micropython.cmake)
endif(ESP_PLATFORM)

include(${CMAKE_CURRENT_LIST_DIR}/lcd_bus/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/lvgl/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/lcd_utils/micropython.cmake)



