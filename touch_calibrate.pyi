# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Tuple

import lvgl as lv

style: lv.style_t = ...


class Tpcal_point(object):

    def __init__(self, x, y, name):
        ...

    def __repr__(self):
        ...


class Tpcal(object):

    def __init__(self, touch_count=500):
        ...

    def show_text(self, txt):
        ...

    def show_circle(self):
        ...

    def calibrate_clicked(self, x, y):
        ...

    def check(self):
        ...

    def calibrate(self, points) -> Tuple[(int, int, int, int)]:
        ...


# Run calibration
def run():
    ...

