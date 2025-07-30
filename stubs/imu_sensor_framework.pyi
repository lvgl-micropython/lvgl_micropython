import fusion


class IMUSensorFramework:
    _device = ...
    _fusion: fusion.Fusion
    _delay_between_samples: int
    _roll: float
    _pitch: float
    _yaw: float

    def __init__(self, device, declination_adjustment: float = 0.0, delay_between_samples: int = 100):
        ...

    @property
    def roll(self) -> float:
        ...

    @property
    def pitch(self) -> float:
        ...

    @property
    def yaw(self) -> float:
        ...

    def _get_gyrometer(self) -> list[float]:
        raise NotImplementedError

    def _get_accelerometer(self) -> list[float]:
        raise NotImplementedError

    def _get_magnetometer(self) -> list[float]:
        raise NotImplementedError

    def calibrate(self, sample_count: int = 5) -> bool:
        ...

    def read(self) -> tuple[float, float, float]:
        ...
