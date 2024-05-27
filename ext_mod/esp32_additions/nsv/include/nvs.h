

#ifndef __NVS_H__
    #define __NVS_H__

    #include "py/runtime.h"
    #include "py/mperrno.h"
    #include "mphalport.h"
    #include "nvs_flash.h"
    #include "nvs.h"


    typedef struct _mp_nvs_obj_t {
        mp_obj_base_t base;
        nvs_handle_t ns;
        const char *ns_name;
    } mp_nvs_obj_t;

    extern const mp_obj_type_t mp_nvs_type;

#endif

