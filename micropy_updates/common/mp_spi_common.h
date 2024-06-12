
#ifndef __MP_SPI_COMMON_H__
    #define __MP_SPI_COMMON_H__

    typedef enum _mp_spi_state_t {
        MP_SPI_STATE_STOPPED,
        MP_SPI_STATE_STARTED,
        MP_SPI_STATE_SENDING
    } mp_spi_state_t;

    typedef struct _machine_hw_spi_bus_obj_t {
        uint8_t host;
        mp_obj_t sck;
        mp_obj_t mosi;
        mp_obj_t miso;
        int16_t active_devices;
        mp_spi_state_t state;
        void *user_data;
    } machine_hw_spi_bus_obj_t;


    typedef struct _machine_hw_spi_obj_t {
        mp_obj_base_t base;
        uint32_t baudrate;
        uint8_t polarity;
        uint8_t phase;
        uint8_t bits;
        uint8_t firstbit;
        bool active;
        mp_obj_t cs;
        machine_hw_spi_bus_obj_t *spi_bus;
        void *user_data;
    } machine_hw_spi_obj_t;

#endif /* __MP_SPI_COMMON_H__ */



user_data = (spi_device_handle_t) spi_device;

user_data = spi_inst_t *const spi_inst;
