# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Optional

# this class is used as a template for writing the mechanism that is
# used to store the touch screen calibration data. All properties and functions
# seen in this class need to exist in the class that is written to store the
# touch calibration data.

# For more information on how to do this see the touch calibration for the ESP32

class TouchCalData(object):

    def __init__(self, name):
        ...
    def save(self):
        ...

    @property
    def left(self) -> Optional[int]:
        ...

    @left.setter
    def left(self, value: int):
        ...

    @property
    def right(self) -> Optional[int]:
        ...

    @right.setter
    def right(self, value: int):
        ...

    @property
    def top(self) -> Optional[int]:
        ...

    @top.setter
    def top(self, value: int):
        ...

    @property
    def bottom(self) -> Optional[int]:
        ...

    @bottom.setter
    def bottom(self, value: int):
        ...

    def reset(self):
        ...
