# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import ffi  # NOQA
import os
import ffilib  # NOQA


libc = ffilib.libc()

fcntl_l = libc.func("i", "fcntl", "iil")
fcntl_s = libc.func("i", "fcntl", "iip")
ioctl_l = libc.func("i", "ioctl", "iil")


def fcntl(fd, op, arg=0):
    if type(arg) is int:
        r = fcntl_l(fd, op, arg)
        return r
    else:
        r = fcntl_s(fd, op, arg)
        return r


def ioctl(fd, op, arg):
    if type(arg) is int:
        r = ioctl_l(fd, op, arg)
        return r
    else:
        r = ioctl_s(fd, op, arg)
        return r
