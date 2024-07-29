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
