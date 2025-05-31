#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef __ALLOCATE_BUFFERS_H__
    #define __ALLOCATE_BUFFERS_H__

    int allocate_buffers(uint8_t num_buffs, size_t buf_size, uint8_t *buf1, uint8_t *buf2);
    void free_buffers(uint8_t *buf1, uint8_t *buf2);

    bool allocate_framebuffer(void *buf, size_t size, uint32_t caps);
    void free_framebuffer(void *buf);

#endif