
#ifndef __PORT_TYPES_H__
    #define __PORT_TYPES_H__
    #include "sdkconfig.h"

    // esp-idf includes
    #include "esp_lcd_panel_io.h"


    typedef enum {
        LCD_OK = ESP_OK,
        LCD_FAIL = ESP_FAIL,
        LCD_ERR_NO_MEM = ESP_ERR_NO_MEM,
        LCD_ERR_INVALID_ARG = ESP_ERR_INVALID_ARG,
        LCD_ERR_INVALID_STATE = ESP_ERR_INVALID_STATE,
        LCD_ERR_INVALID_SIZE = ESP_ERR_INVALID_SIZE,
        LCD_ERR_NOT_SUPPORTED = ESP_ERR_NOT_SUPPORTED
    } mp_lcd_err_t;

    void isr_callback(mp_obj_t cb);
    bool bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);


#endif