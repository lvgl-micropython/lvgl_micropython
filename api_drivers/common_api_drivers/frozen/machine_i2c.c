// Copyright (c) 2016 Damien P. George
// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/modmachine.h"

#include "driver/i2c.h"
#include "hal/i2c_ll.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


#if MICROPY_PY_MACHINE_I2C || MICROPY_PY_MACHINE_SOFTI2C
    #include "../../../../micropy_updates/common/mp_i2c_common.h"
    #include "sdkconfig.h"

    #define MP_MACHINE_I2C_FLAG_WRITE2   (0x08)

    #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32S3
        #define SCLK_I2C_FREQ XTAL_CLK_FREQ
    #elif CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
        #define SCLK_I2C_FREQ APB_CLK_FREQ
    #else
        #error "unsupported I2C for ESP32 SoC variant"
    #endif

    #define DEFAULT_I2C_TIMEOUT_US (50000) // 50ms


    static void device_deinit_internal(mp_machine_hw_i2c_device_obj_t *self);
    static void bus_deinit_internal(mp_machine_hw_i2c_bus_obj_t *self);

    static mp_machine_hw_i2c_bus_obj_t machine_hw_i2c_obj[I2C_NUM_MAX];


    #define I2C_BUS_LOCK_ACQUIRE(self) xSemaphoreTake(self->lock.handle, portMAX_DELAY)
    #define I2C_BUS_LOCK_DELETE(self) vSemaphoreDelete(self->lock.handle)
    #define I2C_BUS_LOCK_RELEASE(self) xSemaphoreGive(self->lock.handle)
    #define I2C_BUS_LOCK_INIT(self) { \
        self->lock.handle = xSemaphoreCreateBinaryStatic(&self->lock.buffer); \
        xSemaphoreGive(self->lock.handle); \
    }

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        #define I2C_DEBUG_PRINT(...) mp_printf(&mp_plat_print, __VA_ARGS__);
    #else
        #define I2C_DEBUG_PRINT(...)
    #endif

    #define RAISE_VALUE_ERROR(msg, ...) \
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT(msg), __VA_ARGS__)




    // ********************** machine.I2C.Bus ************************

    void mp_machine_hw_i2c_bus_deinit_all(void)
    {
        // we need to copy the existing array to a new one so the order doesn't
        // get all mucked up when objects get removed.

        for (uint8_t i=0;i<I2C_NUM_MAX;i++) {
            if (machine_hw_i2c_obj[i].active == 0) continue;

            uint8_t device_count = machine_hw_i2c_obj[i].device_count;
            mp_machine_hw_i2c_device_obj_t *device_objs[device_count];

            for (uint8_t j=0;j<device_count;j++) {
                device_objs[j] = machine_hw_i2c_obj[i].devices[j];
            }

            for (uint8_t j=0;j<device_count;j++) {
                device_deinit_internal(device_objs[j]);
            }
        }
    }


    void i2c_bus_deinit_internal(mp_machine_hw_i2c_bus_obj_t *self)
    {
        if (self->active == 1) {
            self->active = 0;

            if (self->use_locks == 1) {
                I2C_BUS_LOCK_ACQUIRE(self);
                I2C_BUS_LOCK_RELEASE(self);
                I2C_BUS_LOCK_DELETE(self);
            }
            i2c_driver_delete(self->port);
            self->base.type = NULL;
        }
    }


    static void i2c_bus_init_internal(mp_machine_hw_i2c_bus_obj_t *self,
                                      uint32_t freq, uint32_t timeout_us)
    {
        i2c_bus_deinit_internal(self);

        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = self->sda,
            .sda_pullup_en = (bool)self->pullup,
            .scl_io_num = self->scl,
            .scl_pullup_en = (bool)self->pullup,
            .master.clk_speed = freq
        };

        i2c_param_config(self->port, &conf);

        int timeout = SCLK_I2C_FREQ / 1000000 * timeout_us;

        i2c_set_timeout(self->port,
                (timeout > I2C_LL_MAX_TIMEOUT) ? I2C_LL_MAX_TIMEOUT : timeout);

        i2c_driver_install(self->port, I2C_MODE_MASTER, 0, 0, 0);

        self->active = 1;
    }


    static int i2c_bus_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n,
                                mp_machine_i2c_buf_t *bufs, unsigned int flags)
    {
        mp_machine_hw_i2c_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);

        if (self->active == 1) {
            if (self->use_locks == 1) I2C_BUS_LOCK_ACQUIRE(self);

            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            int data_len = 0;

            if (flags & MP_MACHINE_I2C_FLAG_WRITE1) {
                i2c_master_start(cmd);
                i2c_master_write_byte(cmd, addr << 1, true);
                i2c_master_write(cmd, bufs->buf, bufs->len, true);
                data_len += bufs->len;
                --n;
                ++bufs;
            }

            if (flags & MP_MACHINE_I2C_FLAG_WRITE2) {
                i2c_master_start(cmd);
                i2c_master_write_byte(cmd, addr << 1, true);
                i2c_master_write(cmd, bufs->buf, bufs->len, true);
                data_len += bufs->len;
                --n;
                ++bufs;
            }

            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, addr << 1 | (flags & MP_MACHINE_I2C_FLAG_READ), true);

            for (; n--; ++bufs) {
                if (flags & MP_MACHINE_I2C_FLAG_READ) {
                    i2c_master_read(cmd, bufs->buf, bufs->len,
                                n == 0 ? I2C_MASTER_LAST_NACK : I2C_MASTER_ACK);
                } else {
                    if (bufs->len != 0) {
                        i2c_master_write(cmd, bufs->buf, bufs->len, true);
                    }
                }
                data_len += bufs->len;
            }

            if (flags & MP_MACHINE_I2C_FLAG_STOP) {
                i2c_master_stop(cmd);
            }

            esp_err_t err = i2c_master_cmd_begin(self->port, cmd,
                                    100 * (1 + data_len) / portTICK_PERIOD_MS);
            i2c_cmd_link_delete(cmd);

            if (self->use_locks == 1) I2C_BUS_LOCK_RELEASE(self);

            if (err == ESP_FAIL) {
                return -MP_ENODEV;
            } else if (err == ESP_ERR_TIMEOUT) {
                return -MP_ETIMEDOUT;
            } else if (err != ESP_OK) {
                return -abs(err);
            }

            return data_len;
        } else {
            return -MP_ENODEV;
        }
    }


    static void i2c_bus_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
    {
        mp_machine_hw_i2c_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
        int h, l;
        i2c_get_period(self->port, &h, &l);
        mp_printf(print, "I2C(%u, scl=%u, sda=%u, freq=%u)",
            self->port, self->scl, self->sda, SCLK_I2C_FREQ / (h + l));
    }


    static mp_obj_t i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                     size_t n_kw, const mp_obj_t *all_args)
    {

        enum { ARG_host, ARG_scl, ARG_sda, ARG_freq,
               ARG_use_locks, ARG_pullup, ARG_timeout };

        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_host,      MP_ARG_INT | MP_ARG_REQUIRED     },
            { MP_QSTR_scl,       MP_ARG_INT | MP_ARG_REQUIRED     },
            { MP_QSTR_sda,       MP_ARG_INT | MP_ARG_REQUIRED     },
            { MP_QSTR_freq,      MP_ARG_INT,  { .u_int = 400000 } },
            { MP_QSTR_use_locks, MP_ARG_BOOL, { .u_bool = false } },
            { MP_QSTR_pullup,    MP_ARG_BOOL, { .u_bool = false } },
            { MP_QSTR_timeout,   MP_ARG_INT,  { .u_int = DEFAULT_I2C_TIMEOUT_US } },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                                  MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        int i2c_id = (int)args[ARG_host].u_int;
        if (!(I2C_NUM_0 <= i2c_id && i2c_id < I2C_NUM_MAX)) {
            RAISE_VALUE_ERROR("I2C(%u) doesn't exist", i2c_id);
        }

        mp_machine_hw_i2c_bus_obj_t *self = (mp_machine_hw_i2c_bus_obj_t *)&machine_hw_i2c_obj[i2c_id];

        if (self->base.type != NULL) {
            RAISE_VALUE_ERROR("I2C host is already in use (%u)", i2c_id);
        }

        self->base.type = &mp_machine_hw_i2c_bus_type;
        self->port = (uint8_t)i2c_id;
        self->active = 0;
        self->device_count = 0;

        self->scl = (gpio_num_t)args[ARG_scl].u_int;
        self->sda = (gpio_num_t)args[ARG_sda].u_int;
        self->freq = (uint32_t)args[ARG_freq].u_int;
        self->timeout_us = (uint32_t)args[ARG_timeout].u_int;
        self->pullup = (uint8_t)args[ARG_pullup].u_bool;
        self->use_locks = (uint8_t)args[ARG_use_locks].u_bool;

        if (self->use_locks == 1) I2C_BUS_LOCK_INIT(self)

        i2c_bus_init_internal(self, self->freq, self->timeout_us);

        return MP_OBJ_FROM_PTR(self);
    }


    static const mp_machine_i2c_p_t i2c_bus_p = {
        .transfer_supports_write1 = true,
        .transfer = i2c_bus_transfer,
    };


    MP_DEFINE_CONST_OBJ_TYPE(
        mp_machine_hw_i2c_bus_type,
        MP_QSTR_Bus,
        MP_TYPE_FLAG_NONE,
        make_new, i2c_bus_make_new,
        print, i2c_bus_print,
        protocol, &i2c_bus_p,
        locals_dict, &mp_machine_i2c_locals_dict
    );


    // ********************** machine.I2C.Device ************************

    static size_t get_memaddr_buf(uint8_t *memaddr_buf, uint32_t memaddr, uint8_t addrsize) {
        size_t memaddr_len = 0;
        if ((addrsize & 7) != 0 || addrsize > 32) {
            RAISE_VALUE_ERROR("invalid mem address size (%u)", addrsize);
        }

        for (int16_t i = addrsize - 8; i >= 0; i -= 8) {
            memaddr_buf[memaddr_len++] = memaddr >> i;
        }

        return memaddr_len;
    }


    static int device_read(mp_machine_hw_i2c_device_obj_t *self, uint16_t addr,
                           uint32_t memaddr, uint8_t addrsize, uint8_t *buf, size_t len)
    {
        if (self->bus == NULL) return -1;

        uint8_t memaddr_buf[4];
        size_t memaddr_len = get_memaddr_buf(&memaddr_buf[0], memaddr, addrsize);

        mp_machine_i2c_buf_t bufs[2] = {
            {.len = memaddr_len, .buf = memaddr_buf},
            {.len = len, .buf = buf},
        };

        return i2c_bus_transfer((mp_obj_base_t *)self->bus, addr, 2, bufs,
               MP_MACHINE_I2C_FLAG_WRITE1 | MP_MACHINE_I2C_FLAG_READ | MP_MACHINE_I2C_FLAG_STOP);
    }


    static int device_write(mp_machine_hw_i2c_device_obj_t *self, uint16_t addr,
                            uint32_t memaddr, uint8_t addrsize, const uint8_t *buf, size_t len)
    {
        if (self->bus == NULL) return -1;

        uint8_t memaddr_buf[4];
        size_t memaddr_len = get_memaddr_buf(&memaddr_buf[0], memaddr, addrsize);

        mp_machine_i2c_buf_t bufs[2] = {
            {.len = memaddr_len, .buf = memaddr_buf},
            {.len = len, .buf = (uint8_t *)buf},
        };

        return i2c_bus_transfer((mp_obj_base_t *)self->bus, addr, 2,
                                 bufs, MP_MACHINE_I2C_FLAG_STOP);
    }


    static int device_readfrom(mp_machine_hw_i2c_device_obj_t *self, uint16_t addr,
                               uint8_t *dest, size_t len, bool stop)
    {
        if (self->bus == NULL) return -1;

        mp_machine_i2c_buf_t buf = {.len = len, .buf = dest};
        return i2c_bus_transfer((mp_obj_base_t *)self->bus, addr, 1,
               &buf, MP_MACHINE_I2C_FLAG_READ | (stop ? MP_MACHINE_I2C_FLAG_STOP : 0));
    }


    static int device_writeto(mp_machine_hw_i2c_device_obj_t *self, uint16_t addr,
                              const uint8_t *src, size_t len, bool stop)
    {
        if (self->bus == NULL) return -1;

        mp_machine_i2c_buf_t buf = {.len = len, .buf = (uint8_t *)src};
        return i2c_bus_transfer((mp_obj_base_t *)self->bus, addr, 1,
                                 &buf, stop ? MP_MACHINE_I2C_FLAG_STOP : 0);
    }


    void device_deinit_internal(mp_machine_hw_i2c_device_obj_t *self)
    {
        if (self->bus == NULL) return;

        uint8_t i = 0;

        for (;i<self->bus->device_count;i++) {
            if (self->bus->devices[i] == self) {
                self->bus->devices[i] = NULL;
                break;
            }
        }

        for (uint8_t j=0;j<self->bus->device_count;j++) {
            self->bus->devices[j - i + 1] = self->bus->devices[j];
        }

        self->bus->device_count--;
        self->bus->devices = m_realloc(self->bus->devices,
            self->bus->device_count * sizeof(mp_machine_hw_i2c_device_obj_t *));

        self->bus = NULL;
    }


    static mp_obj_t i2c_device_deinit(mp_obj_t self_in)
    {
        mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
        device_deinit_internal(self);
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_1(i2c_device_deinit_obj, i2c_device_deinit);


    static mp_obj_t i2c_device_write_readinto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_write_buf, ARG_read_buf };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED },
            { MP_QSTR_write_buf,    MP_ARG_OBJ | MP_ARG_REQUIRED },
            { MP_QSTR_read_buf,     MP_ARG_OBJ | MP_ARG_REQUIRED },
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args,
                         MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

        mp_buffer_info_t write_bufinfo;
        mp_get_buffer_raise(args[ARG_write_buf].u_obj, &write_bufinfo, MP_BUFFER_READ);

        mp_buffer_info_t read_bufinfo;
        mp_get_buffer_raise(args[ARG_write_buf].u_obj, &read_bufinfo, MP_BUFFER_WRITE);

        uint8_t * write_buf = (uint8_t *)write_bufinfo.buf;
        uint32_t memaddr = 0;

        if ((self->reg_bits & 7) != 0 || self->reg_bits > 32) {
            RAISE_VALUE_ERROR("invalid mem address size (%u)", self->reg_bits);
        }

        for (int i=(int)(self->reg_bits / 8);i>-1;i--) {
            memaddr |= (uint32_t)(write_buf[i] << ((~i + (int)(self->reg_bits / 8)) * 8));
        }

        uint8_t memaddr_buf[4];
        size_t memaddr_len = get_memaddr_buf(&memaddr_buf[0], memaddr, self->reg_bits);

        size_t num_bufs = 2;

        unsigned int flags = (
            MP_MACHINE_I2C_FLAG_WRITE1 |
            MP_MACHINE_I2C_FLAG_READ |
            MP_MACHINE_I2C_FLAG_STOP
        );

        mp_machine_i2c_buf_t bufs[3] = {
            {.len = memaddr_len, .buf = memaddr_buf},
            {.len = 0, .buf = NULL},
            {.len = 0, .buf = NULL}
        };

        if ((size_t)(self->reg_bits / 8) < write_bufinfo.len) {
            bufs[1].buf = write_buf + (self->reg_bits / 8);
            bufs[1].len = write_bufinfo.len - (size_t)(self->reg_bits / 8);
            num_bufs += 1;
            flags |= MP_MACHINE_I2C_FLAG_WRITE2;
        }

        bufs[num_bufs - 1].buf = read_bufinfo.buf;
        bufs[num_bufs - 1].len = read_bufinfo.len;

        if (self->bus == NULL) mp_raise_OSError(1);

        int ret = i2c_bus_transfer((mp_obj_base_t *)self->bus,
                                    self->device_id, num_bufs, bufs, flags);

        if (ret < 0) mp_raise_OSError(-ret);
        return mp_const_none;
    }

    static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_device_write_readinto_obj, 3, i2c_device_write_readinto);


    static mp_obj_t i2c_device_read_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_memaddr, ARG_num_bytes, ARG_buf };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED },
            { MP_QSTR_memaddr,      MP_ARG_INT | MP_ARG_REQUIRED },
            { MP_QSTR_num_bytes,    MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1            } },
            { MP_QSTR_buf,          MP_ARG_OBJ | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args,
                         MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

        uint32_t memaddr = (uint32_t)args[ARG_memaddr].u_int;
        int ret;

        if (args[ARG_buf].u_obj != mp_const_none) {
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);

            ret = device_read(self, self->device_id, memaddr, self->reg_bits,
                             (uint8_t *)bufinfo.buf, bufinfo.len);

            if (ret < 0) mp_raise_OSError(-ret);
            return mp_const_none;
        } else {
            int num_bytes = (int)args[ARG_num_bytes].u_int;
            // create the buffer to store data into
            vstr_t vstr;
            vstr_init_len(&vstr, num_bytes);

            // do the transfer
            ret = device_read(self, self->device_id, memaddr, self->reg_bits,
                             (uint8_t *)vstr.buf, vstr.len);

            if (ret < 0) mp_raise_OSError(-ret);
            return mp_obj_new_bytes_from_vstr(&vstr);
        }
    }

    static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_device_read_mem_obj, 2, i2c_device_read_mem);


    static mp_obj_t i2c_device_write_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_memaddr, ARG_buf };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,    MP_ARG_OBJ | MP_ARG_REQUIRED },
            { MP_QSTR_memaddr, MP_ARG_INT | MP_ARG_REQUIRED },
            { MP_QSTR_buf,     MP_ARG_OBJ | MP_ARG_REQUIRED },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args,
                         MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

        uint32_t memaddr = (uint32_t)args[ARG_memaddr].u_int;

        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);

        int ret = device_write(self, self->device_id, memaddr, self->reg_bits,
                              (uint8_t *)bufinfo.buf, bufinfo.len);

        if (ret < 0) mp_raise_OSError(-ret);
        return mp_const_none;
    }

    static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_device_write_mem_obj, 3, i2c_device_write_mem);


    static mp_obj_t i2c_device_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_num_bytes, ARG_buf };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,      MP_ARG_OBJ | MP_ARG_REQUIRED                            },
            { MP_QSTR_num_bytes, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1            } },
            { MP_QSTR_buf,       MP_ARG_OBJ | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args,
                         MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

        int ret;

        if (args[ARG_buf].u_obj != mp_const_none) {
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);

            ret = device_readfrom(self, self->device_id,
                                 (uint8_t *)bufinfo.buf, bufinfo.len, true);

            if (ret < 0) mp_raise_OSError(-ret);

            return mp_const_none;
        } else {
            int num_bytes = (int)args[ARG_num_bytes].u_int;
            // create the buffer to store data into
            vstr_t vstr;
            vstr_init_len(&vstr, num_bytes);

            // do the transfer
            ret = device_readfrom(self, self->device_id,
                                 (uint8_t *)vstr.buf, vstr.len, true);

            if (ret < 0) mp_raise_OSError(-ret);
            return mp_obj_new_bytes_from_vstr(&vstr);
        }
    }

    static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_device_read_obj, 1, i2c_device_read);


    static mp_obj_t i2c_device_write(size_t n_args, const mp_obj_t *pos_args,
                                     mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_buf };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self, MP_ARG_OBJ | MP_ARG_REQUIRED },
            { MP_QSTR_buf, MP_ARG_OBJ | MP_ARG_REQUIRED }
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args,
                         MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);


        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);

        int ret = device_writeto(self, self->device_id,
                                (uint8_t *)bufinfo.buf, bufinfo.len, true);
        if (ret < 0) {
            mp_raise_OSError(-ret);
        }

        return mp_const_none;
    }

    static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_device_write_obj, 2, i2c_device_write);


    mp_obj_t i2c_device_make_new(const mp_obj_type_t *type, size_t n_args,
                                 size_t n_kw, const mp_obj_t *all_args)
    {

        // Parse args
        enum { ARG_bus, ARG_dev_id, ARG_reg_bits };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_bus,      MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED },
            { MP_QSTR_dev_id,   MP_ARG_KW_ONLY | MP_ARG_INT | MP_ARG_REQUIRED },
            { MP_QSTR_reg_bits, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 8   } }
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                                  MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        // create new object
        mp_machine_hw_i2c_device_obj_t *self = m_new_obj(mp_machine_hw_i2c_device_obj_t);
        self->base.type = &mp_machine_hw_i2c_device_type;

        self->bus = (mp_machine_hw_i2c_bus_obj_t *)args[ARG_bus].u_obj;
        self->device_id = (uint16_t)args[ARG_dev_id].u_int;
        self->reg_bits =  (uint8_t)args[ARG_reg_bits].u_int;

        self->bus->device_count++;
        self->bus->devices = m_realloc(self->bus->devices,
                self->bus->device_count * sizeof(mp_machine_hw_i2c_device_obj_t *));

        self->bus->devices[self->bus->device_count - 1] = self;

        return MP_OBJ_FROM_PTR(self);
    }


    static const mp_rom_map_elem_t i2c_device_locals_dict_table[] = {
        { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&i2c_device_write_readinto_obj) },
        { MP_ROM_QSTR(MP_QSTR_read_mem),       MP_ROM_PTR(&i2c_device_read_mem_obj)       },
        { MP_ROM_QSTR(MP_QSTR_write_mem),      MP_ROM_PTR(&i2c_device_write_mem_obj)      },
        { MP_ROM_QSTR(MP_QSTR_read),           MP_ROM_PTR(&i2c_device_read_obj)           },
        { MP_ROM_QSTR(MP_QSTR_write),          MP_ROM_PTR(&i2c_device_write_obj)          },
        { MP_ROM_QSTR(MP_QSTR___del__),        MP_ROM_PTR(&i2c_device_deinit_obj)         }
    };


    MP_DEFINE_CONST_DICT(i2c_device_locals_dict, i2c_device_locals_dict_table);


    MP_DEFINE_CONST_OBJ_TYPE(
        mp_machine_hw_i2c_device_type,
        MP_QSTR_Device,
        MP_TYPE_FLAG_NONE,
        make_new, i2c_device_make_new,
        locals_dict, &i2c_device_locals_dict
    );



    // ********************** machine.I2C ************************

    static const mp_rom_map_elem_t i2c_locals_dict_table[] = {
        { MP_ROM_QSTR(MP_QSTR___name__),  MP_OBJ_NEW_QSTR(MP_QSTR_I2C)             },
        { MP_ROM_QSTR(MP_QSTR_Bus),       (mp_obj_t)&mp_machine_hw_i2c_bus_type    },
        { MP_ROM_QSTR(MP_QSTR_Device),    (mp_obj_t)&mp_machine_hw_i2c_device_type }
    };


    static MP_DEFINE_CONST_DICT(i2c_locals_dict, i2c_locals_dict_table);


    MP_DEFINE_CONST_OBJ_TYPE(
        machine_i2c_type,
        MP_QSTR_I2C,
        MP_TYPE_FLAG_NONE,
        locals_dict, &i2c_locals_dict
    );
#else
    void mp_machine_hw_i2c_bus_deinit_all(void) {}

#endif /* MICROPY_PY_MACHINE_I2C || MICROPY_PY_MACHINE_SOFTI2C */