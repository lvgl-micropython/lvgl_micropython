#include "py/obj.h"

#ifndef __FUSION_H__
    #define __FUSION_H__

    typedef struct {
        mp_obj_base_t base;

        uint32_t start_ts;
        float mag_bias[3];
        float beta;  // 0.6045997880780725842169464404f
        float declination;
        float q[4];

    } mp_fusion_obj_t;


    extern const mp_obj_type_t mp_fusion_type;

#endif
