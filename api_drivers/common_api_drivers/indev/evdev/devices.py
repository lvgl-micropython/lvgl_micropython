# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const
import struct
import os
import select

_INT = "i"
_INT2 = "ii"
_INT5 = "iiiii"
_SHORT = "h"
_USHORT = "H"
_SHORT4 = "hhhh"

_SIZEOF_INT2 = const(0x8)
_IOC_NRBITS = const(0x8)
_IOC_TYPEBITS = const(0x8)
_IOC_SIZEBITS = const(0xE)
_IOC_DIRBITS = const(0x2)
_IOC_NRMASK = const(0xFF)
_IOC_TYPEMASK = const(0xFF)
_IOC_SIZEMASK = const(0x3FFF)
_IOC_DIRMASK = const(0x3)
_IOC_NRSHIFT = const(0x0)
_IOC_TYPESHIFT = const(0x8)
_IOC_SIZESHIFT = const(0x10)
_IOC_DIRSHIFT = const(0x1E)
_IOC_NONE = const(0x0)
_IOC_WRITE = const(0x1)
_IOC_READ = const(0x2)
_EVIOCGVERSION = const(0x80044501)
_EVIOCGID = const(0x80084502)
_EVIOCGREP = const(0x80084503)
_EVIOCSREP = const(0x40084503)
_EVIOCGKEYCODE = const(0x80084504)
_EVIOCSKEYCODE = const(0x40084504)
_EVIOCGKEY = const(0x80084505)
_EVIOCGNAME = const(0x80FF4506)
_EVIOCGPHYS = const(0x80FF4507)
_EVIOCGUNIQ = const(0x80FF4508)
_EVIOCRMFF = const(0x40044581)
_EVIOCSGAIN = const(0x40004582)
_EVIOCSAUTOCENTER = const(0x40004583)
_EVIOCGEFFECTS = const(0x80044584)
_EVIOCGRAB = const(0x40044590)
_EV_FMT = "llHHi"
_EV_SIZE = const(0x18)


def _IOC_TYPECHECK(t):
    if isinstance(t, int):
        return t

    size = 0

    for char in t:
        if char == 'i':
            size += 4
        elif char == 'h':
            size += 2
        elif size == 'H':
            size += 2

    return size


def _IOC(d, t, nr, size):
    return (
        (d << _IOC_DIRSHIFT) |
        (t << _IOC_TYPESHIFT) |
        (nr << _IOC_NRSHIFT) |
        (size << _IOC_SIZESHIFT)
    )


def _IOR(t, nr, size):
    return _IOC(_IOC_READ, t, nr, _IOC_TYPECHECK(size))


def _IOW(t, nr, size):
    return _IOC(_IOC_WRITE, t, nr, _IOC_TYPECHECK(size))


# these take parameters.
def EVIOCGBIT(evtype, length=255):
    return _IOC(_IOC_READ, 69, 0x20 + evtype, length)  # get event bits */


def EVIOCGABS(a):
    return _IOR(69, 0x40 + a, _INT5)  # get abs value/limits */


def EVIOCGSW(length):
    return _IOC(_IOC_READ, 69, 0x1b, length)  # get all switch states */


def EVIOCGLED(length):
    return _IOC(_IOC_READ, 69, 0x19, length)  # get all LEDs */


_EV_SYN = const(0x00)
_EV_KEY = const(0x01)
_EV_REL = const(0x02)
_EV_ABS = const(0x03)
_EV_MSC = const(0x04)
_EV_SW = const(0x05)
_EV_LED = const(0x11)
_EV_SND = const(0x12)
_EV_REP = const(0x14)
_EV_FF = const(0x15)
_EV_PWR = const(0x16)
_EV_FF_STATUS = const(0x17)


import ffi  # NOQA
import ffilib  # NOQA

libc = ffilib.libc()
_ioctl_s = libc.func("i", "ioctl", "iip")


def _ioctl(fd, op, arg):
    r = _ioctl_s(fd, op, arg)
    return r


class Features(object):
    """Contains a set of base features. May be actual set as returned by a
    feature request, or a desired set to find.
    """
    NAMES = {
        _EV_SYN: "Sync",
        _EV_KEY: "Keys or Buttons",
        _EV_REL: "Relative Axes",
        _EV_ABS: "Absolute Axes",
        _EV_MSC: "Miscellaneous",
        _EV_SW: "Switches",
        _EV_LED: "Leds",
        _EV_SND: "Sound",
        _EV_REP: "Repeat",
        _EV_FF: "Force Feedback",
        _EV_PWR: "Power Management",
        _EV_FF_STATUS: "Force Feedback Status",
    }

    def __init__(self, bits=0):
        self._bits = bits

    def has_keys(self):
        return (self._bits >> _EV_KEY) & 1

    def has_leds(self):
        return (self._bits >> _EV_LED) & 1

    def has_sound(self):
        return (self._bits >> _EV_SND) & 1

    def has_relative_axes(self):
        return (self._bits >> _EV_REL) & 1

    def has_absolute_axes(self):
        return (self._bits >> _EV_ABS) & 1

    def has_misc(self):
        return (self._bits >> _EV_MSC) & 1

    def has_switches(self):
        return (self._bits >> _EV_SW) & 1

    def has_repeat(self):
        return (self._bits >> _EV_REP) & 1

    def has_forcefeedback(self):
        return (self._bits >> _EV_FF) & 1

    def has_forcefeedback_status(self):
        return (self._bits >> _EV_FF_STATUS) & 1

    def has_power(self):
        return (self._bits >> _EV_PWR) & 1

    def _make_set(self):
        featureset = set()
        bits = self._bits
        for bit in (
            _EV_KEY,
            _EV_REL,
            _EV_ABS,
            _EV_MSC,
            _EV_SW,
            _EV_LED,
            _EV_SND,
            _EV_REP,
            _EV_FF,
            _EV_PWR,
            _EV_FF_STATUS
        ):
            if (bits >> bit) & 1:
                featureset.add(bit)
        return featureset

    def __str__(self):
        s = []
        bits = self._bits
        for bit, name in self.NAMES.items():
            if (bits >> bit) & 1:
                s.append(name)
        return ", ".join(s)


class Event(object):

    def __init__(self, data):
        self.type, self.code, self.value = struct.unpack(_EV_FMT, data)[2:]

    def __str__(self):
        return (
            "event_type: 0x%x, event_code: 0x%x, event value: 0x%x" %
            (self.type, self.code, self.value)
        )


# base class for event devices. Subclass this for your specific device.
class EventDevice(object):

    def __init__(self, name, filename):
        filename = f'/dev/input/{filename}'

        self._evdev = open(filename, 'rb')
        self._fd = self._evdev.fileno()

        self.name = name

        data = self._buf = bytearray(_EV_SIZE)
        mv = self._mv = memoryview(data)

        _ioctl(self._fd, _EVIOCGVERSION, mv[:4])
        self.driver_version = struct.unpack(_INT, bytes(data[:4]))[0]

        _ioctl(self._fd, _EVIOCGID, mv[:8])
        (
            self.bus_id,
            self.vendor_id,
            self.product_id,
            self.version_id
        ) = struct.unpack(_SHORT4, mv[:8])

        _ioctl(self._fd, EVIOCGBIT(0), mv[:4])
        caps = struct.unpack(_INT, bytes(data[:4]))[0]
        self.features = Features(caps)

        self._poll = select.poll()
        self._poll.register(self._fd)

    def __str__(self):
        return (
            "%s: bus=0x%x, vendor=0x%x, product=0x%x, version=0x%x\n   %s" %
            (self.name, self.bus_id, self.vendor_id,
             self.product_id, self.version_id, self.features)
        )

    def read(self):
        if not self._poll.poll()[0][1] & select.POLLIN:
            return None

        # Read and parse evdev mouse data
        # 2e 16 e9 63 00 00 00 00 17 95 0a 00 00 00 00 00 02 00 00 00 fd ff ff ff  # NOQA
        # |          16 bytes long system time           |type |code |   value
        data = struct.unpack(_EV_FMT, self._evdev.read(_EV_SIZE))
        if data:
            return Event(data)

    @staticmethod
    def get_device(name):
        for dev_name, filename in EventDevice.get_device_names():
            if name in dev_name:
                return EventDevice(dev_name, filename)

        raise IOError("Input device %r not found." % (name,))

    @staticmethod
    def get_device_names():
        names = []

        data = bytearray(256)
        mv = memoryview(data)

        for i in range(0, 31):
            files = os.listdir('/dev/input')
            event_filename = f'event{i}'
            mouse_filename = f'mouse{i}'
            joystick_filename = f'j{i}'

            for filename in (event_filename, mouse_filename, joystick_filename):
                if filename in files:
                    file = f'/dev/input/{filename}'
                    try:
                        evdev = open(file, 'rb')
                    except (OSError, IOError):  # probably no permissions
                        pass
                    else:
                        fd = evdev.fileno()
                        try:
                            _ioctl(fd, _EVIOCGNAME, mv)
                            name = ''.join(
                                bytes(char).decode('utf-8') for char in data if
                                bytes(char) != b'\x00'
                            )
                            names.append((name, filename))
                            for j in range(len(data)):
                                data[i] = 0
                        finally:
                            evdev.close()
        return names

    def close(self):
        if self._evdev is not None:
            self._evdev.close()
            self._fd = None
            self._evdev = None
            self.name = ""
