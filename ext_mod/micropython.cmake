include(${CMAKE_CURRENT_LIST_DIR}/lcd_bus/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/lvgl/micropython.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/lcd_utils/micropython.cmake)

set(MICROPY_MULTICORE_THREAD $ENV{MICROPY_MULTICORE_THREADING})

if(MICROPY_MULTICORE_THREAD)
      include(${CMAKE_CURRENT_LIST_DIR}/threading/micropython.cmake)
endif(MICROPY_MULTICORE_THREAD)
