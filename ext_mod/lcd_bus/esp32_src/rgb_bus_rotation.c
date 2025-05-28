// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/gc.h"
    #include "py/stackctrl.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "freertos/event_groups.h"
    #include "freertos/idf_additions.h"
    #include "rom/ets_sys.h"
    #include "esp_system.h"
    #include "esp_cpu.h"

    #include "esp_lcd_panel_ops.h"

    #include "rgb_bus.h"
    #include "rgb565_dither.h"

    #include <string.h>




#endif
