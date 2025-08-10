// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "../../generic/src/spi_bus.c"

/*


#if defined(MICROPY_HW_SPI1_SCK)
static SPI_HandleTypeDef SPIHandle1 = {.Instance = NULL};
#endif
#if defined(MICROPY_HW_SPI2_SCK)
static SPI_HandleTypeDef SPIHandle2 = {.Instance = NULL};
#endif
#if defined(MICROPY_HW_SPI3_SCK)
static SPI_HandleTypeDef SPIHandle3 = {.Instance = NULL};
#endif
#if defined(MICROPY_HW_SPI4_SCK)
static SPI_HandleTypeDef SPIHandle4 = {.Instance = NULL};
#endif
#if defined(MICROPY_HW_SPI5_SCK)
static SPI_HandleTypeDef SPIHandle5 = {.Instance = NULL};
#endif
#if defined(MICROPY_HW_SPI6_SCK)
static SPI_HandleTypeDef SPIHandle6 = {.Instance = NULL};
#endif
#if defined(MICROPY_HW_SUBGHZSPI_ID)
static SPI_HandleTypeDef SPIHandleSubGhz = {.Instance = NULL};
#endif








#if defined(HAL_DMA_MODULE_ENABLED)
    must set macro USE_HAL_SPI_REGISTER_CALLBACKS to 1U

    typedef void (*pSPI_CallbackTypeDef)(SPI_HandleTypeDef *hspi)

    typedef enum {
        HAL_SPI_TX_COMPLETE_CB_ID,
        HAL_SPI_RX_COMPLETE_CB_ID,
        HAL_SPI_TX_RX_COMPLETE_CB_ID,
        HAL_SPI_TX_HALF_COMPLETE_CB_ID,
        HAL_SPI_RX_HALF_COMPLETE_CB_ID,
        HAL_SPI_TX_RX_HALF_COMPLETE_CB_ID,
        HAL_SPI_ERROR_CB_ID,
        HAL_SPI_ABORT_CB_ID,
        HAL_SPI_SUSPEND_CB_ID,
        HAL_SPI_MSPINIT_CB_ID,
        HAL_SPI_MSPDEINIT_CB_ID
    } HAL_SPI_CallbackIDTypeDef;

    HAL_StatusTypeDef HAL_SPI_RegisterCallback(SPI_HandleTypeDef *hspi, HAL_SPI_CallbackIDTypeDef CallbackID, pSPI_CallbackTypeDef pCallback)
    HAL_StatusTypeDef HAL_SPI_UnRegisterCallback(SPI_HandleTypeDef *hspi, HAL_SPI_CallbackIDTypeDef CallbackID)

    if (HAL_SPI_STATE_READY == hspi->State)
*/