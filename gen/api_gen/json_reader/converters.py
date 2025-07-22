
mp_to_c = {
    'mp_obj_t': '(mp_obj_t)',
    'va_list': None,
    'void *': 'mp_to_ptr',
    'const uint8_t *': 'mp_to_ptr',
    'const void *': 'mp_to_ptr',
    'bool': 'mp_obj_is_true',
    'char *': '(char *)convert_from_str',
    'char **': 'mp_write_ptr_C_Pointer',
    'const char *': 'convert_from_str',
    'const char **': 'mp_write_ptr_C_Pointer',
    'lv_obj_t *': 'mp_to_lv',
    'uint8_t': '(uint8_t)mp_obj_get_int',
    'uint16_t': '(uint16_t)mp_obj_get_int',
    'uint32_t': '(uint32_t)mp_obj_get_int',
    'uint64_t': '(uint64_t)mp_obj_get_ull',
    'unsigned': '(unsigned)mp_obj_get_int',
    'unsigned int': '(unsigned int)mp_obj_get_int',
    'unsigned char': '(unsigned char)mp_obj_get_int',
    'unsigned short': '(unsigned short)mp_obj_get_int',
    'unsigned long': '(unsigned long)mp_obj_get_int',
    'unsigned long int': '(unsigned long int)mp_obj_get_int',
    'unsigned long long': '(unsigned long long)mp_obj_get_ull',
    'unsigned long long int': '(unsigned long long int)mp_obj_get_ull',
    'int8_t': '(int8_t)mp_obj_get_int',
    'int16_t': '(int16_t)mp_obj_get_int',
    'int32_t': '(int32_t)mp_obj_get_int',
    'int64_t': '(int64_t)mp_obj_get_ull',
    'size_t': '(size_t)mp_obj_get_int',
    'int': '(int)mp_obj_get_int',
    'char': '(char)mp_obj_get_int',
    'short': '(short)mp_obj_get_int',
    'long': '(long)mp_obj_get_int',
    'long int': '(long int)mp_obj_get_int',
    'long long': '(long long)mp_obj_get_ull',
    'long long int': '(long long int)mp_obj_get_ull',
    'float': '(float)mp_obj_get_float',
}

c_to_mp = {
    'mp_obj_t': '(mp_obj_t)',
    'va_list': None,
    'void *': 'ptr_to_mp',
    'const uint8_t *': 'ptr_to_mp',
    'const void *': 'ptr_to_mp',
    'bool': 'convert_to_bool',
    'char *': 'convert_to_str',
    'char **': 'mp_read_ptr_C_Pointer',
    'const char *': 'convert_to_str',
    'const char **': 'mp_read_ptr_C_Pointer',
    'lv_obj_t *': 'lv_to_mp',
    'uint8_t': 'mp_obj_new_int_from_uint',
    'uint16_t': 'mp_obj_new_int_from_uint',
    'uint32_t': 'mp_obj_new_int_from_uint',
    'uint64_t': 'mp_obj_new_int_from_ull',
    'unsigned': 'mp_obj_new_int_from_uint',
    'unsigned int': 'mp_obj_new_int_from_uint',
    'unsigned char': 'mp_obj_new_int_from_uint',
    'unsigned short': 'mp_obj_new_int_from_uint',
    'unsigned long': 'mp_obj_new_int_from_uint',
    'unsigned long int': 'mp_obj_new_int_from_uint',
    'unsigned long long': 'mp_obj_new_int_from_ull',
    'unsigned long long int': 'mp_obj_new_int_from_ull',
    'int8_t': 'mp_obj_new_int',
    'int16_t': 'mp_obj_new_int',
    'int32_t': 'mp_obj_new_int',
    'int64_t': 'mp_obj_new_int_from_ll',
    'size_t': 'mp_obj_new_int_from_uint',
    'int': 'mp_obj_new_int',
    'char': 'mp_obj_new_int',
    'short': 'mp_obj_new_int',
    'long': 'mp_obj_new_int',
    'long int': 'mp_obj_new_int',
    'long long': 'mp_obj_new_int_from_ll',
    'long long int': 'mp_obj_new_int_from_ll',
    'float': 'mp_obj_new_float',
}


c_to_mp_byref = {}


c_mp_type = {
    'mp_obj_t': 'lv_obj_t *',
    'va_list': None,
    'void *': 'void *',
    'const uint8_t *': 'void *',
    'const void *': 'void *',
    'bool': 'bool',
    'char *': 'char *',
    'char **': 'char **',
    'const char *': 'char *',
    'const char **': 'char **',
    'lv_obj_t *': 'lv_obj_t *',
    'uint8_t': 'int',
    'uint16_t': 'int',
    'uint32_t': 'int',
    'uint64_t': 'int',
    'unsigned': 'int',
    'unsigned int': 'int',
    'unsigned char': 'int',
    'unsigned short': 'int',
    'unsigned long': 'int',
    'unsigned long int': 'int',
    'unsigned long long': 'int',
    'unsigned long long int': 'int',
    'int8_t': 'int',
    'int16_t': 'int',
    'int32_t': 'int',
    'int64_t': 'int',
    'size_t': 'int',
    'int': 'int',
    'char': 'int',
    'short': 'int',
    'long': 'int',
    'long int': 'int',
    'long long': 'int',
    'long long int': 'int',
    'void': None,
    'float': 'float',
}

c_to_py_type = {
    'lv_obj_t': 'obj_t',
    'lv_obj_t *': 'obj_t',
    'va_list': '*args',
    'void *': 'Any',
    'void *[]': 'Any',
    'char *[]': 'List[str]',
    'uint8_t *': 'memoryview',
    'uint16_t *': 'List[int]',
    'uint32_t *': 'List[int]',
    'uint64_t *': 'List[int]',
    'int8_t *': 'List[int]',
    'int16_t *': 'List[int]',
    'int32_t *': 'List[int]',
    'int64_t *': 'List[int]',
    'const uint8_t *': 'str',
    'const uint16_t *': 'List[int]',
    'const uint32_t *': 'List[int]',
    'const uint64_t *': 'List[int]',
    'const uint8_t **': 'List[bytes]',
    'const uint16_t **': 'List[List[int]]',
    'const uint32_t **': 'List[List[int]]',
    'const uint64_t **': 'List[List[int]]',
    'const int8_t *': 'List[int]',
    'const int16_t *': 'List[int]',
    'const int32_t *': 'List[int]',
    'const int64_t *': 'List[int]',
    'const int8_t **': 'List[List[int]]',
    'const int16_t **': 'List[List[int]]',
    'const int32_t **': 'List[List[int]]',
    'const int64_t **': 'List[List[int]]',
    'const void *': 'Any',
    'bool': 'bool',
    'char *': 'str',
    'char **': 'List[str]',
    'const char *': 'str',
    'const char **': 'List[str]',
    'uint8_t': 'int',
    'uint16_t': 'int',
    'uint32_t': 'int',
    'uint64_t': 'int',
    'unsigned': 'int',
    'unsigned int': 'int',
    'unsigned char': 'str',
    'const unsigned char': 'str',
    'unsigned char *': 'str',
    'unsigned char **': 'List[str]',
    'const unsigned char *': 'str',
    'const unsigned char **':  'List[str]',
    'unsigned short': 'int',
    'unsigned long': 'int',
    'unsigned long int': 'int',
    'unsigned long long': 'int',
    'unsigned long long int': 'int',

    'const unsigned short': 'int',
    'const unsigned long': 'int',
    'const unsigned long int': 'int',
    'const unsigned long long': 'int',
    'const unsigned long long int': 'int',


    'int8_t': 'int',
    'int16_t': 'int',
    'int32_t': 'int',
    'int64_t': 'int',
    'size_t': 'int',
    'int': 'int',
    'char': 'str',

    'short': 'int',
    'long': 'int',
    'long int': 'int',
    'long long': 'int',
    'long long int': 'int',

    'const short': 'int',
    'const long': 'int',
    'const long int': 'int',
    'const long long': 'int',
    'const long long int': 'int',

    'short *': 'List[int]',
    'long *': 'List[int]',
    'long int *': 'List[int]',
    'long long *': 'List[int]',
    'long long int *': 'List[int]',

    'const short *': 'List[int]',
    'const long *': 'List[int]',
    'const long int *': 'List[int]',
    'const long long *': 'List[int]',
    'const long long int *': 'List[int]',

    'void': 'None',
    'float': 'float',
}


# Add native array supported types
# These types would be converted automatically to/from array type.
# Supported array (pointer) types are signed/unsigned int: 8bit, 16bit, 32bit and 64bit.
def register_int_ptr_type(convertor, *types):
    for ptr_type in types:
        for qualified_ptr_type in [ptr_type, 'const '+ptr_type]:
            mp_to_c[qualified_ptr_type] = f'mp_array_to_{convertor}'
            c_to_mp[qualified_ptr_type] = f'mp_array_from_{convertor}'
            c_mp_type[qualified_ptr_type] = 'void*'


register_int_ptr_type('u8ptr',
                      'unsigned char *', 'uint8_t *')

# char* is considered as string, not int ptr!

# register_int_ptr_type('i8ptr',
#                       'char *', 'int8_t *')

register_int_ptr_type('u16ptr',
                      'unsigned short *', 'uint16_t *')

register_int_ptr_type('i16ptr',
                      'short *', 'int16_t *')

register_int_ptr_type('u32ptr',
                      'uint32_t *', 'unsigned *', 'unsigned int *',
                      'unsigned long *', 'unsigned long int *', 'size_t *')

register_int_ptr_type('i32ptr',
                      'int32_t *', 'signed *', 'signed int *',
                      'signed long *', 'signed long int *', 'long *',
                      'long int *', 'int *')

register_int_ptr_type('u64ptr',
                      'int64_t *', 'signed long long *',
                      'long long *', 'long long int *')

register_int_ptr_type('i64ptr',
                      'uint64_t *', 'unsigned long long *',
                      'unsigned long long int *')