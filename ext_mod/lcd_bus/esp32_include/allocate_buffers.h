#ifndef __ALLOCATE_BUFFERS_H__:
    #define __ALLOCATE_BUFFERS_H__

    int allocate_buffers(uint8_t num_buffs, size_t buf_size, uint8_t *buf1, uint8_t *buf2);
    void free_buffers(uint8_t *buf1, uint8_t *buf2);


#endif