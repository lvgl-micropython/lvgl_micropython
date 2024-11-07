// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "thread_event.h"


#ifndef __THREADING_EVENT_H__
    #define __THREADING_EVENT_H__


    typedef struct _threading_event_handle_t {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        uint8_t flag:
    } threading_event_handle_t;

    struct _mp_event_t {
        EventGroupHandle_t handle;
    };

    extern const mp_obj_type_t mp_type_threading_event_t;



void mp_create_event(mp_event_t *event);


#endif
pthread_mutex_lock( &mutex );
pthread_cond_wait( & cond, & mutex );
pthread_mutex_unlock( & mutex );
pthread_cond_signal( &cond );

pthread_cond_broadcast

pthread_cond_signal
int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime);







