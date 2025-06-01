#include <stdint.h>
#include <inttypes.h>

#include "allocate_buffers.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/misc.h"


int allocate_buffers(uint8_t num_buffs, size_t buf_size, uint8_t *buf1, uint8_t *buf2)
{
    buf1 = m_malloc(buf_size);
    if (buf1 == NULL) {
        return -1;
    } else if (num_buffs == 2) {
        buf2 = m_malloc(buf_size);
        if (buf2 == NULL) {
            m_free(buf1);
            buf1 = NULL;
            return -1;
        } else {
            return 2;
        }
    }

    return 1;
}


void free_buffers(uint8_t *buf1, uint8_t *buf2)
{
    if (buf1 != NULL) {
        m_free(buf1);
        buf1 = NULL;
    }

    if (buf2 != NULL) {
        m_free(buf2);
        buf2 = NULL;
    }
}


bool allocate_framebuffer(void *buf, size_t size, uint32_t caps)
{
    ((void)caps);
    buf = m_malloc(size);
    if (buf == NULL) {
        return false;
    } else {
        return true;
    }
}


void free_framebuffer(void *buf)
{
    if (buf != NULL) m_free(buf);
}