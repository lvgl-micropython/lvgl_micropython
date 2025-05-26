

#include "fusion.h"
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

#include <math.h>


#define FUSION_PI 3.14159265358979323846f
#define FUSION_UNUSED(x) ((void)x)

#define FUSION_RADIANS(degree) (degree) * FUSION_PI / 180.0f
#define FUSION_DEGREES(radian) (radian) * 180.0f / FUSION_PI

#define FUSION_MAX(item1, item2) (item1) >= (item2) ? (item1) : (item2)
#define FUSION_MIN(item1, item2) (item1) <= (item2) ? (item1) : (item2)


static mp_obj_t fusion_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_declination
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_declination, MP_ARG_OBJ  | MP_ARG_KW_ONLY,  {.u_obj = mp_const_none } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                    MP_ARRAY_SIZE(make_new_args), make_new_args, args);

    // create new object
    mp_fusion_obj_t *self = m_new_obj(mp_fusion_obj_t);
    self->base.type = &mp_fusion_type;

    self->beta = 0.6045997880780725842169464404f;

    if (args[ARG_declination].u_obj == mp_const_none) {
        self->declination = 0.0f;
    } else {
        self->declination = mp_obj_get_float_to_f(args[ARG_declination].u_obj);
    }

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t calibrate(mp_obj_t self_in, mp_obj_t getxyz, mp_obj_t stopfunc)
{

    mp_fusion_obj_t *self = MP_OBJ_TO_PTR(self_in);

    float mag_max[3];
    float mag_min[3];

    mp_obj_tuple_t *magxyz = MP_OBJ_TO_PTR(mp_call_function_n_kw(getxyz, 0, 0, NULL));

    float item;
    for (uint8_t i=0;i<3;i++) {
        item = (float)mp_obj_get_float(magxyz->items[i]);
        mag_max[i] = item;
        mag_min[i] = item;
    }

    bool stop = (bool)mp_obj_get_int_truncated(mp_call_function_n_kw(stopfunc, 0, 0, NULL));

    while (!stop) {
        magxyz = MP_OBJ_TO_PTR(mp_call_function_n_kw(getxyz, 0, 0, NULL));

        for (uint8_t i=0;i<3;i++) {
            item = (float)mp_obj_get_float(magxyz->items[i]);
            mag_max[i] = FUSION_MAX(mag_max[i], item);
            mag_min[i] = FUSION_MIN(mag_min[i], item);
        }

        stop = (bool)mp_obj_get_int_truncated(mp_call_function_n_kw(stopfunc, 0, 0, NULL));
    }

    for (uint8_t i=0;i<3;i++) {
        self->mag_bias[i] = (mag_min[i] + mag_max[i]) / 2.0f;
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_3(calibrate_obj, calibrate);


static float delta_T(mp_fusion_obj_t *self)
{
    float res;
    uint32_t ts = mp_hal_ticks_us();

    if (self->start_ts == 0) {
        res = 0.0001f;
    } else {
        res = (float)(ts - self->start_ts) * 0.000001f;
    }

    self->start_ts = ts;
    return res;
}


mp_obj_t calculate(mp_fusion_obj_t *self, float accel[3], float gyro[3], float *mag)
{
    mp_obj_t tuple[3] = {
        mp_const_none,
        mp_const_none,
        mp_const_none
    };

    float roll;
    float pitch;
    float yaw;

    float ax = accel[0];
    float ay = accel[1];
    float az = accel[2];

    // Normalise accelerometer measurement
    float norm = sqrtf((ax * ax) + (ay * ay) + (az * az));

    if (norm == 0.0f) return mp_obj_new_tuple(3, tuple);  // handle NaN

    norm = 1.0f / norm;  // use reciprocal for division

    ax *= norm;
    ay *= norm;
    az *= norm;

    float mx;
    float my;
    float mz;

    if (mag != NULL) {
        mx = mag[0] - self->mag_bias[0];
        my = mag[1] - self->mag_bias[1];
        mz = mag[2] - self->mag_bias[2];

        // Normalise magnetometer measurement
        norm = sqrtf((mx * mx) + (my * my) + (mz * mz));

        if (norm == 0.0f) return mp_obj_new_tuple(3, tuple);  // handle NaN

        norm = 1.0f / norm;  // use reciprocal for division

        mx *= norm;
        my *= norm;
        mz *= norm;
    } else {
        mx = 0.0f;
        my = 0.0f;
        mz = 0.0f;
    }

    float gx = FUSION_RADIANS(gyro[0]);
    float gy = FUSION_RADIANS(gyro[1]);
    float gz = FUSION_RADIANS(gyro[2]);

    float q1 = self->q[0];
    float q2 = self->q[1];
    float q3 = self->q[2];
    float q4 = self->q[3];

    // Auxiliary variables to avoid repeated arithmetic
    float _2q1 = 2.0f * q1;
    float _2q2 = 2.0f * q2;
    float _2q3 = 2.0f * q3;
    float _2q4 = 2.0f * q4;

    float q1sq = q1 * q1;
    float q2sq = q2 * q2;
    float q3sq = q3 * q3;
    float q4sq = q4 * q4;

    float s1;
    float s2;
    float s3;
    float s4;

    if (mag != NULL) {
        float q12 = q1 * q2;
        float q13 = q1 * q3;
        float q24 = q2 * q4;
        float q34 = q3 * q4;

        // Reference direction of Earth's magnetic field
        float _2q1mx = _2q1 * mx;
        float _2q1my = _2q1 * my;
        float _2q1mz = _2q1 * mz;
        float _2q2mx = _2q2 * mx;

        float hx = (mx * q1sq) - (_2q1my * q4) + (_2q1mz * q3) + (mx * q2sq) + (_2q2 * my) * q3 + (_2q2 * mz * q4) - (mx * q3sq) - (mx * q4sq);
        float hy = (_2q1mx * q4) + (my * q1sq) - (_2q1mz * q2) + (_2q2mx * q3) - (my * q2sq) + (my * q3sq) + (_2q3 * mz * q4) - (my * q4sq);

        float _2bx = sqrtf((hx * hx) + (hy * hy));
        float _2bz = (-_2q1mx * q3) + (_2q1my * q2) + (mz * q1sq) + (_2q2mx * q4) - (mz * q2sq) + (_2q3 * my * q4) - (mz * q3sq) + (mz * q4sq);
        float _4bx = 2.0f * _2bx;
        float _4bz = 2.0f * _2bz;

        float temp1 = (2.0f * q24) - (2.0f * q13) - ax;
        float temp2 = (2.0f * q12) + (2.0f * q34) - ay;
        float temp3 = (_2bx * (0.5f - q3sq - q4sq)) + (_2bz * (q34 - q13)) - mx;
        float temp4 = (_2bx * ((q2 * q3) - (q1 * q4))) + (_2bz * (q12 + q34)) - my;
        float temp5 = (_2bx * (q13 + q24)) + (_2bz * (0.5f - q2sq - q3sq)) - mz;
        float temp6 = 1.0f - (2.0f * q2sq) - (2 * q3sq) - az;
        float temp7 = 4.0f * temp6;
        float temp8 = _2bx * q3;
        float temp9 = _2bx * q4;
        float temp10 = _2bz * q4;
        float temp11 = _2bz * q1;
        float temp12 = _2bz * q2;
        float temp13 = _2bx * q2;
        float temp14 = _2bz * q3;
        float temp15 = _2bx * q1;

        // Gradient descent algorithm corrective step
        s1 = (-_2q3 * temp1) + (_2q2 * temp2) - (temp14 * temp3) + ((temp9 + temp12) * temp4) + (temp8 * temp5);
        s2 = (_2q4 * temp1) + (_2q1 * temp2) - (q2 * temp7) + (temp10 * temp3) + ((temp8 + temp11) * temp4) + ((temp9 - (_4bz * q2)) * temp5);
        s3 = (-_2q1 * temp1) + (_2q4 * temp2) - (q3 * temp7) + ((-_4bx * q3 - temp11) * temp3) + ((temp13 + temp10) * temp4) + ((temp15 - (_4bz * q3)) * temp5);
        s4 = (_2q2 * temp1) + (_2q3 * temp2) + (((-_4bx * q4) + temp12) * temp3) + ((-temp15 + temp14) * temp4) + (temp13 * temp5);
    } else {
        float _4q1 = _2q1 + _2q1;
        float _4q2 = _2q2 + _2q2;
        float _4q3 = _2q3 + _2q3;

        float _8q2 = _4q2 + _4q2;
        float _8q3 = _4q3 + _4q3;

        // Gradient decent algorithm corrective step
        s1 = (_4q1 * q3sq) + (_2q3 * ax) + (_4q1 * q2sq) - (_2q2 * ay);
        s2 = (_4q2 * q4sq) - (_2q4 * ax) + (4 * q1sq * q2) - (_2q1 * ay) - _4q2 + (_8q2 * q2sq) + (_8q2 * q3sq) + (_4q2 * az);
        s3 = (4 * q1sq * q3) + (_2q1 * ax) + (_4q3 * q4sq) - (_2q4 * ay) - _4q3 + (_8q3 * q2sq) + (_8q3 * q3sq) + (_4q3 * az);
        s4 = (4 * q2sq * q4) - (_2q2 * ax) + (4 * q3sq * q4) - (_2q3 * ay);
    }

    norm = 1.0f / sqrtf((s1 * s1) + (s2 * s2) + (s3 * s3) + (s4 * s4));  // normalise step magnitude
    s1 *= norm;
    s2 *= norm;
    s3 *= norm;
    s4 *= norm;

    float beta = self->beta;

    // Compute rate of change of quaternion
    float qDot1 = 0.5f * ((-q2 * gx) - (q3 * gy) - (q4 * gz)) - (beta * s1);
    float qDot2 = 0.5f * ((q1 * gx) + (q3 * gz) - (q4 * gy)) - (beta * s2);
    float qDot3 = 0.5f * ((q1 * gy) - (q2 * gz) + (q4 * gx)) - (beta * s3);
    float qDot4 = 0.5f * ((q1 * gz) + (q2 * gy) - (q3 * gx)) - (beta * s4);

    // Integrate to yield quaternion
    float delta_t = delta_T(self);
    q1 += qDot1 * delta_t;
    q2 += qDot2 * delta_t;
    q3 += qDot3 * delta_t;
    q4 += qDot4 * delta_t;

    norm = 1.0f / sqrtf((q1 * q1) + (q2 * q2) + (q3 * q3) + (q4 * q4));  // normalise quaternion

    q1 *= norm;
    q2 *= norm;
    q3 *= norm;
    q4 *= norm;

    q1sq = q1 * q1;
    q2sq = q2 * q2;
    float s3sq = s3 * s3;
    q4sq = q4 * q4;

    self->q[0] = q1;
    self->q[1] = q2;
    self->q[2] = q3;
    self->q[3] = q4;

    pitch = FUSION_DEGREES(-asinf(2.0f * ((q2 * q4) - (q1 * q3))));
    roll = FUSION_DEGREES(atan2f(2.0f * ((q1 * q2) + (q3 * q4)), q1sq - q2sq - s3sq + q4sq));

    if (mag != NULL) {
        yaw = FUSION_DEGREES(atan2f(2.0f * ((q2 * q3) + (q1 * q4)), q1sq + q2sq - s3sq - q4sq));
        yaw += self->declination;
    } else {
        yaw = 0.0f;
    }

    tuple[0] = mp_obj_new_float((mp_float_t)roll);
    tuple[1] = mp_obj_new_float((mp_float_t)pitch);
    tuple[2] = mp_obj_new_float((mp_float_t)yaw);

    return mp_obj_new_tuple(3, tuple);
}




mp_obj_t update(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_accel, ARG_gyro, ARG_mag };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,  MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_accel, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_gyro,  MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_mag,   MP_ARG_OBJ, { .u_obj = mp_const_none } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_fusion_obj_t *self = (mp_fusion_obj_t *)args[ARG_self].u_obj;

    mp_obj_t ret;

    float accel[3];
    float gyro[3];

    mp_obj_tuple_t *t_accel = MP_OBJ_TO_PTR(args[ARG_accel].u_obj);
    mp_obj_tuple_t *t_gyro = MP_OBJ_TO_PTR(args[ARG_gyro].u_obj);

    for(uint8_t i=0;i<3;i++) {
        accel[i] = mp_obj_get_float_to_f(t_accel->items[i]);
        gyro[i] = mp_obj_get_float_to_f(t_gyro->items[i]);
    }

    if (args[ARG_mag].u_obj != mp_const_none) {
        float *mag = (float *)malloc(sizeof(float) * 3);
        mp_obj_tuple_t *mag_t = MP_OBJ_TO_PTR(args[ARG_mag].u_obj);

        for(uint8_t i=0;i<3;i++) {
            mag[i] = mp_obj_get_float_to_f(mag_t->items[i]);
        }

        ret = calculate(self, accel, gyro, mag);
        free(mag);
    } else {
        ret = calculate(self, accel, gyro, NULL);
    }

    return ret;
}


static MP_DEFINE_CONST_FUN_OBJ_KW(update_obj, 3, update);


static const mp_rom_map_elem_t fusion_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_calibrate), MP_ROM_PTR(&calibrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_update),    MP_ROM_PTR(&update_obj)    },
};


MP_DEFINE_CONST_DICT(fusion_locals_dict, fusion_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_fusion_type,
    MP_QSTR_Fusion,
    MP_TYPE_FLAG_NONE,
    make_new, fusion_make_new,
    locals_dict, (mp_obj_dict_t *)&fusion_locals_dict
);


static const mp_rom_map_elem_t fusion_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_fusion) },
    { MP_ROM_QSTR(MP_QSTR_Fusion),   MP_ROM_PTR(&mp_fusion_type)     }
};


static MP_DEFINE_CONST_DICT(fusion_globals, fusion_globals_table);


const mp_obj_module_t mp_module_fusion = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&fusion_globals,
};


MP_REGISTER_MODULE(MP_QSTR_fusion, mp_module_fusion);
