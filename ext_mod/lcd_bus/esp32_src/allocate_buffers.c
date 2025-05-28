#include "esp_heap_caps.h"


int allocate_buffers(uint8_t num_buffs, size_t buf_size, uint8_t *buf1, uint8_t *buf2)
{
    buf1 = heap_caps_calloc(1, buf_size, MALLOC_CAP_DMA);
    if (buf1 == NULL) {
        buf1 = heap_caps_calloc(1, buf_size, MALLOC_CAP_DEFAULT);
    } else if (num_buffs == 2) {
        buf2 = heap_caps_calloc(1, buf_size, MALLOC_CAP_DMA);
        if (buf2 != NULL) {
            return 2;
        }
    }
    if (buf1) return -1;

    return 1;
}


void free_buffers(uint8_t *buf1, uint8_t *buf2)
{
    if (buf1 != NULL) {
        heap_caps_free(buf1);
        buf1 = NULL;
    }

    if (buf2 != NULL) {
        heap_caps_free(buf2);
        buf2 = NULL;
    }
}