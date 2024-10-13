#ifndef __THREADING_EVENT_H__
    #define __THREADING_EVENT_H__

     xEventGroupCreateStatic( *pxEventGroupBuffer)

    typedef struct _mp_event_group_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    } mp_event_group_t;


    typedef struct _mp_obj_threading_event_t {
        mp_obj_base_t base;
        mp_event_group_t event;
    } mp_obj_threading_event_t;


    extern const mp_obj_type_t mp_type_threading_event_t;


#endif













