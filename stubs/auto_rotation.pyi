# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv
import imu_sensor_framework


class AutoRotation:
    _device: imu_sensor_framework.IMUSensorFramework
    _timer = lv.timer_t
    _lock_rotation: bool
    _last_rotation: int
    _last_free_rotation: int
    _adjustment: float


    def __init__(self, device: imu_sensor_framework.IMUSensorFramework,
                 delay: int, lock_rotation: bool = False, adjustment: float = 0.0):
        ...

    @property
    def adjustment(self) -> float:
        ...

    @adjustment.setter
    def adjustment(self, value: float) -> None:
        ...

    @property
    def lock_rotation(self) -> bool:
        ...

    @lock_rotation.setter
    def lock_rotation(self, value: bool) -> None:
        ...

    def __del__(self) -> None:
        ...

    def _timer_cb(self, _) -> None:
        ...
