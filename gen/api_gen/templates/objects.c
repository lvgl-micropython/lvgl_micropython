#define LV_OBJ_T /*#~0~#*/

typedef struct mp_lv_obj_type_t {
    const lv_obj_class_t *lv_obj_class;
    const mp_obj_type_t *mp_obj_type;
} mp_lv_obj_type_t;

static const mp_lv_obj_type_t mp_lv_/*#~1~#*/_type;
static const mp_lv_obj_type_t *mp_lv_obj_types[];

static inline const mp_obj_type_t *get_BaseObj_type()
{
    return mp_lv_/*#~1~#*/_type.mp_obj_type;
}

MP_DEFINE_EXCEPTION(LvReferenceError, Exception)
    """