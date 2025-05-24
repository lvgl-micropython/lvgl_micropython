import fusion
import time


class IMUSensorFramework:
    # delay between samples is nanosecond resolution. 1000 nanoseconds = 1 millisecond

    def __init__(self, device, declination_adjustment=0.0, delay_between_samples=100):
        self._device = device
        self._fusion = fusion.Fusion(declination=declination_adjustment)
        self._delay_between_samples = delay_between_samples
        self._roll = 0.0
        self._pitch = 0.0
        self._yaw = 0.0

    @property
    def roll(self):
        return self._roll

    @property
    def pitch(self):
        return self._pitch

    @property
    def yaw(self):
        return self._yaw

    def _get_gyrometer(self):
        raise NotImplementedError

    def _get_accelerometer(self):
        raise NotImplementedError

    def _get_magnetometer(self):
        raise NotImplementedError

    def calibrate(self, sample_count=5):
        ts = [time.ticks_ns()]
        count = [0]

        def _stop_func():
            if count[0] == sample_count:
                return True

            now_ts = time.ticks_ns()
            diff = time.ticks_diff(now_ts, ts[0])
            remaining = self._delay_between_samples - diff
            if remaining > 0:
                time.sleep_ns(remaining)

            ts[0] = time.ticks_ns()
            count[0] += 1
            return False

        self._fusion.calibrate(self._get_magnetometer, _stop_func)

    def read(self):
        accel = self._get_accelerometer()
        gyro = self._get_gyrometer()

        try:
            mag = self._get_magnetometer()
        except NotImplementedError:
            mag = None

        self._roll, self._pitch, self._yaw = self._fusion.update(accel, gyro, mag)

        return self._roll, self._pitch, self._yaw
