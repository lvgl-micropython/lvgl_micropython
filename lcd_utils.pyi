# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Union

def remap(
    value: Union[float, int],
    old_min: Union[float, int],
    old_max: Union[float, int],
    new_min: Union[float, int],
    new_max: Union[float, int]
) -> Union[float, int]:
    """
    Remaps a value from one one range to another range

    NOTE: If any of the parameters is a `float` than the return value is `float`.
          If all of the parameters are `int` than the return value is `int`.

    :param value: Value to remap
    :param old_min: minimum of the range that value is mapped to
    :param old_max: maximum of the range that value is mapped to
    :param new_min: minimum of the range to map value to
    :param new_max: maximum of the range to map value to
    :return: value that has been mapped to new_min and new_max
    """
    ...


def int_float_converter(value: Union[float, int], /) -> Union[float, int]:
    ...


def spi_mode_to_polarity_phase(mode: int, /) -> tuple[int]:
    """

    :param mode: spi mode, can be 0, 1, 2 or 3
    :type mode: `int`

    :returns: the 2 integer values as `(polarity, phase)`
    :rtype: `tuple`
    """

def spi_polarity_phase_to_mode(polarity: int, phase: int, /) -> int:
    """
    :param polarity: 1 or 0
    :type polarity: `int`

    :param phase: 1 or 0
    :type phase: `int`

    :returns: 0, 1, 2 or 3
    :rtype: `int`
    """