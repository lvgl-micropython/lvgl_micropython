from typing import Callable, Tuple


class Fusion:

    def __init__(self, declination: float | None = None):
        ...

    def calibrate(self, mag_xyz: Callable[[], Tuple[float, float, float]], stopfunc: Callable[[], bool], /):
        ...

    def update(self,
        accel: Tuple[float, float, float],
        gyro: Tuple[float, float, float],
        mag: Tuple[float, float, float] | None = None
    ) -> Tuple[float, float, float]:
        ...
