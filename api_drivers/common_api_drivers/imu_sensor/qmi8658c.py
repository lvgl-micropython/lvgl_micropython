from micropython import const  # NOQA
import imu_sensor_framework  # NOQA


_VERSION_REG = const(0x0)
_REVISION_REG = const(0x1)

_TIME_REG = const(0x30)
_TEMP_REG = const(0x33)
_ACCEL_REG = const(0x35)
_GYRO_REG = const(0x3B)

_CONFIG2_REG = const(0x02)
_CONFIG5_REG = const(0x05)
_CONFIG6_REG = const(0x06)
_CONFIG7_REG = const(0x07)

_ACCEL_SETTING_REG = const(0x03)
_GYRO_SETTING_REG = const(0x04)
_ENABLE_REG = const(0x08)

# STANDARD_GRAVITY = 9.80665


def _decode_settings(value):
    rnge = value >> 4
    rate = value & 0xF

    return rnge, rate


def _encode_setting(rnge, rate):
    return ((rnge & 0x7) << 4) | (rate & 0xF)


ACCEL_RANGE_2 = const(0)  # +/- 2g
ACCEL_RANGE_4 = const(0x10)  # +/- 4g
ACCEL_RANGE_8 = const(0x20)  # +/- 8g (default value)
ACCEL_RANGE_16 = const(0x30)  # +/- 16g

GYRO_RANGE_16 = const(0)  # +/- 16 deg/s
GYRO_RANGE_32 = const(0x10)  # +/- 32 deg/s
GYRO_RANGE_64 = const(0x20)  # +/- 64 deg/s
GYRO_RANGE_128 = const(0x30)  # +/- 128 deg/s
GYRO_RANGE_256 = const(0x40)  # +/- 256 deg/s (default value)
GYRO_RANGE_512 = const(0x50)  # +/- 512 deg/s
GYRO_RANGE_1024 = const(0x60)  # +/- 1024 deg/s
GYRO_RANGE_2048 = const(0x70)  # +/- 2048 deg/s

ACCEL_RATE_8000_HZ = const(0)
ACCEL_RATE_4000_HZ = const(1)
ACCEL_RATE_2000_HZ = const(2)
ACCEL_RATE_1000_HZ = const(3)
ACCEL_RATE_500_HZ = const(4)
ACCEL_RATE_250_HZ = const(5)
ACCEL_RATE_125_HZ = const(6)  # (default value)
ACCEL_RATE_62_HZ = const(7)
ACCEL_RATE_31_HZ = const(8)
ACCEL_RATE_LP_128_HZ = const(12)
ACCEL_RATE_LP_21_HZ = const(13)
ACCEL_RATE_LP_11_HZ = const(14)
ACCEL_RATE_LP_3_HZ = const(15)


GYRO_RATE_8000_HZ = const(0)
GYRO_RATE_4000_HZ = const(1)
GYRO_RATE_2000_HZ = const(2)
GYRO_RATE_1000_HZ = const(3)
GYRO_RATE_500_HZ = const(4)
GYRO_RATE_250_HZ = const(5)
GYRO_RATE_125_HZ = const(6)  # (default value)
GYRO_RATE_62_HZ = const(7)
GYRO_RATE_31_HZ = const(8)

I2C_ADDR = 0x6B
BITS = 8


class QMI8658C(imu_sensor_framework.IMUSensorFramework):

    def _read_reg(self, reg):
        self._device.read_mem(reg, self._rx_mv[:1])
        return self._rx_buf[0]

    def _write_reg(self, reg, data):
        self._tx_buf[0] = data
        self._device.write_mem(reg, self._tx_mv[:1])

    def __init__(self, device, delay_between_samples=100):

        super().__init__(device, 0.0, delay_between_samples)
        self._device = device
        self._tx_buf = bytearray(1)
        self._tx_mv = memoryview(self._tx_buf)
        self._rx_buf = bytearray(6)
        self._rx_mv = memoryview(self._rx_buf)

        if self._read_reg(_VERSION_REG) != 0x05:
            raise RuntimeError("Failed to find QMI8658C")

        self._accel_range = ACCEL_RANGE_8
        self._accel_rate = ACCEL_RATE_125_HZ

        self._gyro_range = GYRO_RANGE_256
        self._gyro_rate = GYRO_RATE_125_HZ

        self._write_reg(_CONFIG2_REG, 0x60)
        self._write_reg(_ACCEL_SETTING_REG, _encode_setting(ACCEL_RANGE_8, ACCEL_RATE_125_HZ))
        self._write_reg(_GYRO_SETTING_REG, _encode_setting(GYRO_RANGE_256, GYRO_RATE_125_HZ))

        self._write_reg(_CONFIG5_REG, 0x00)  # No magnetometer
        self._write_reg(_CONFIG6_REG, 0x00)  # Disables Gyroscope And Accelerometer Low-Pass Filter
        self._write_reg(_CONFIG7_REG, 0x00)  # Disables Motion on Demand.
        self._write_reg(_ENABLE_REG, 0x03)  # enable accel and gyro

    @property
    def accel_range(self):
        return self._accel_range

    @accel_range.setter
    def accel_range(self, value):
        self._accel_range = value
        self._write_reg(_ACCEL_SETTING_REG, _encode_setting(value, self._accel_rate))

    @property
    def accel_rate(self):
        return self._accel_rate

    @accel_rate.setter
    def accel_rate(self, value):
        self._accel_rate = value
        self._write_reg(_ACCEL_SETTING_REG, _encode_setting(self._accel_range, value))

    @property
    def gyro_range(self):
        return self._gyro_range

    @gyro_range.setter
    def gyro_range(self, value):
        self._gyro_range = value
        self._write_reg(_ACCEL_SETTING_REG, _encode_setting(value, self._gyro_rate))

    @property
    def gyro_rate(self):
        return self._gyro_rate

    @gyro_rate.setter
    def gyro_rate(self, value):
        self._gyro_rate = value
        self._write_reg(_ACCEL_SETTING_REG, _encode_setting(self._gyro_range, value))

    @property
    def timestamp(self) -> int:
        self._device.read_mem(_TIME_REG, self._rx_mv[:3])
        return self._rx_buf[0] + (self._rx_buf[1] << 8) + (self._rx_buf[2] << 16)

    @property
    def temperature(self) -> float:
        """Chip temperature"""
        self._device.read_mem(_TEMP_REG, self._rx_mv[:2])
        temp = self._rx_buf[0] / 256 + self._rx_buf[1]
        return temp

    def _get_accelerometer(self):
        self._device.read_mem(_ACCEL_REG, self._rx_mv[:6])

        buf = self._rx_buf

        x = buf[0] << 8 | buf[1]
        y = buf[2] << 8 | buf[3]
        z = buf[4] << 8 | buf[5]

        return x, y, z

    def _get_gyrometer(self):
        self._device.read_mem(_GYRO_REG, self._rx_mv[:6])
        buf = self._rx_buf

        x = buf[0] << 8 | buf[1]
        y = buf[2] << 8 | buf[3]
        z = buf[4] << 8 | buf[5]

        return x, y, z

    def _get_magnetometer(self):  # NOQA
        return None
