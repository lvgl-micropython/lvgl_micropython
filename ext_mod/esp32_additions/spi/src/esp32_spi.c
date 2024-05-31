#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../include/esp32_spi_bus.h"
#include "../include/esp32_spi_device.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "py/objarray.h"
#include "py/binary.h"

#include "esp_heap_caps.h"


STATIC mp_obj_t esp32_hw_spi_get_dma_buffer(mp_obj_t size_in)
{
    uint16_t size = (uint16_t)mp_obj_get_int_truncated(size_in);

    void *buf = heap_caps_calloc(1, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (buf == NULL) {
        buf = heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    }

    if (buf == NULL) {
        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to allocate buffer"));
        return mp_const_none;
    }

    mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
    view->typecode |= 0x80; // used to indicate RW buffer

    return MP_OBJ_FROM_PTR(view);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_get_dma_buffer_obj, esp32_hw_spi_get_dma_buffer);


STATIC mp_obj_t esp32_hw_spi_free_dma_buffer(mp_obj_t buf_in)
{
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_RW);
    heap_caps_free(bufinfo.buf);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_free_dma_buffer_obj, esp32_hw_spi_free_dma_buffer);


STATIC const mp_map_elem_t esp32_module_spi_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_spi)                  },
    { MP_ROM_QSTR(MP_QSTR_Bus),             MP_ROM_PTR(&esp32_hw_spi_bus_type)            },
    { MP_ROM_QSTR(MP_QSTR_Device),          MP_ROM_PTR(&esp32_hw_spi_dev_type)            },
    { MP_ROM_QSTR(MP_QSTR_get_dma_buffer),  MP_ROM_PTR(&esp32_hw_spi_get_dma_buffer_obj)  },
    { MP_ROM_QSTR(MP_QSTR_free_dma_buffer), MP_ROM_PTR(&esp32_hw_spi_free_dma_buffer_obj) },
};

STATIC MP_DEFINE_CONST_DICT(esp32_module_spi_globals, esp32_module_spi_globals_table);


const mp_obj_module_t esp32_module_spi = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&esp32_module_spi_globals,
};

MP_REGISTER_MODULE(MP_QSTR_spi, esp32_module_spi);
