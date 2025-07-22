

MP_DEFINE_EXCEPTION(LvError, Exception)
MP_DEFINE_EXCEPTION(LvReferenceError, LvError)
MP_DEFINE_EXCEPTION(LvPointerError, LvError)

#define _RAISE_ERROR_MSG(msg)  msg

#define RAISE_ValueError(msg, ...)           mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_TypeError(msg, ...)            mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_OSError(msg, ...)              mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_SyntaxError(msg, ...)          mp_raise_msg_varg(&mp_type_SyntaxError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_Exception(msg, ...)            mp_raise_msg_varg(&mp_type_Exception, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_IndexError(msg, ...)           mp_raise_msg_varg(&mp_type_IndexError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_KeyError(msg, ...)             mp_raise_msg_varg(&mp_type_KeyError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_NotImplementedError(msg, ...)  mp_raise_msg_varg(&mp_type_NotImplementedError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_RuntimeError(msg, ...)         mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_AttributeError(msg, ...)       mp_raise_msg_varg(&mp_type_AttributeError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)

#define RAISE_LvError(msg, ...)              mp_raise_msg_varg(&mp_type_LvError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_LvReferenceError(msg, ...)     mp_raise_msg_varg(&mp_type_LvReferenceError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_LvPointerError(msg, ...)       mp_raise_msg_varg(&mp_type_LvPointerError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)

