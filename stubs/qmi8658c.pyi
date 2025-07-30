from typing import Final

import i2c
import imu_sensor_framework  # NOQA



def _decode_settings(value: int) -> tuple[int, int]:
    ...


def _encode_setting(rnge: int, rate: int) -> int:
    ...


ACCEL_RANGE_2: Final[int]
ACCEL_RANGE_4: Final[int]
ACCEL_RANGE_8: Final[int]
ACCEL_RANGE_16: Final[int]

GYRO_RANGE_16: Final[int]
GYRO_RANGE_32: Final[int]
GYRO_RANGE_64: Final[int]
GYRO_RANGE_128: Final[int]
GYRO_RANGE_256: Final[int]
GYRO_RANGE_512: Final[int]
GYRO_RANGE_1024: Final[int]
GYRO_RANGE_2048: Final[int]

ACCEL_RATE_8000_HZ: Final[int]
ACCEL_RATE_4000_HZ: Final[int]
ACCEL_RATE_2000_HZ: Final[int]
ACCEL_RATE_1000_HZ: Final[int]
ACCEL_RATE_500_HZ: Final[int]
ACCEL_RATE_250_HZ: Final[int]
ACCEL_RATE_125_HZ: Final[int]
ACCEL_RATE_62_HZ: Final[int]
ACCEL_RATE_31_HZ: Final[int]
ACCEL_RATE_LP_128_HZ: Final[int]
ACCEL_RATE_LP_21_HZ: Final[int]
ACCEL_RATE_LP_11_HZ: Final[int]
ACCEL_RATE_LP_3_HZ: Final[int]


GYRO_RATE_8000_HZ: Final[int]
GYRO_RATE_4000_HZ: Final[int]
GYRO_RATE_2000_HZ: Final[int]
GYRO_RATE_1000_HZ: Final[int]
GYRO_RATE_500_HZ: Final[int]
GYRO_RATE_250_HZ: Final[int]
GYRO_RATE_125_HZ: Final[int]
GYRO_RATE_62_HZ: Final[int]
GYRO_RATE_31_HZ: Final[int]

I2C_ADDR: Final[int]
BITS: Final[int]


class QMI8658C(imu_sensor_framework.IMUSensorFramework):
    _device: i2c.I2C.Device

    _tx_buf: bytearray
    _tx_mv: memoryview
    _rx_buf: bytearray
    _rx_mv: memoryview

    _accel_range: int
    _accel_rate: int

    _gyro_range: int
    _gyro_rate: int


    def _read_reg(self, reg: int) -> int:
        ...

    def _write_reg(self, reg: int, data: int) -> None:
        ...

    def __init__(self, device: i2c.I2C.Device, delay_between_samples: int = 100):
        ...

    @property
    def accel_range(self) -> int:
        ...

    @accel_range.setter
    def accel_range(self, value: int) -> None:
        ...

    @property
    def accel_rate(self) -> int:
        ...

    @accel_rate.setter
    def accel_rate(self, value: int) -> None:
        ...

    @property
    def gyro_range(self) -> int:
        ...

    @gyro_range.setter
    def gyro_range(self, value: int) -> None:
        ...

    @property
    def gyro_rate(self) -> int:
        ...

    @gyro_rate.setter
    def gyro_rate(self, value: int) -> None:
        ...

    @property
    def timestamp(self) -> int:
        ...

    @property
    def temperature(self) -> float:
        """Chip temperature"""
        ...

    def _get_accelerometer(self) -> tuple[int, int, int]:
        ...

    def _get_gyrometer(self) -> tuple[int, int, int]:
        ...

    def _get_magnetometer(self) -> None:  # NOQA
        ...
