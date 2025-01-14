# Copyright (c) 2024 - 2025 Kevin G. Schlosser

'''

// Configure HX8357C display

    writecommand(0xB9); // Enable extension command
    writedata(0xFF);
    writedata(0x83);
    writedata(0x57);
    delay(50);

    writecommand(0xB6); //Set VCOM voltage
    writedata(0x2C);    //0x52 for HSD 3.0"

    writecommand(0x11); // Sleep off
    delay(200);

    writecommand(0x35); // Tearing effect on
    writedata(0x00);    // Added parameter

    writecommand(0x3A); // Interface pixel format
    writedata(0x55);    // 16 bits per pixel

    //writecommand(0xCC); // Set panel characteristic
    //writedata(0x09);    // S960>S1, G1>G480, R-G-B, normally black

    //writecommand(0xB3); // RGB interface
    //writedata(0x43);
    //writedata(0x00);
    //writedata(0x06);
    //writedata(0x06);

    writecommand(0xB1); // Power control
    writedata(0x00);
    writedata(0x15);
    writedata(0x0D);
    writedata(0x0D);
    writedata(0x83);
    writedata(0x48);


    writecommand(0xC0); // Does this do anything?
    writedata(0x24);
    writedata(0x24);
    writedata(0x01);
    writedata(0x3C);
    writedata(0xC8);
    writedata(0x08);

    writecommand(0xB4); // Display cycle
    writedata(0x02);
    writedata(0x40);
    writedata(0x00);
    writedata(0x2A);
    writedata(0x2A);
    writedata(0x0D);
    writedata(0x4F);

    writecommand(0xE0); // Gamma curve
    writedata(0x00);
    writedata(0x15);
    writedata(0x1D);
    writedata(0x2A);
    writedata(0x31);
    writedata(0x42);
    writedata(0x4C);
    writedata(0x53);
    writedata(0x45);
    writedata(0x40);
    writedata(0x3B);
    writedata(0x32);
    writedata(0x2E);
    writedata(0x28);

    writedata(0x24);
    writedata(0x03);
    writedata(0x00);
    writedata(0x15);
    writedata(0x1D);
    writedata(0x2A);
    writedata(0x31);
    writedata(0x42);
    writedata(0x4C);
    writedata(0x53);
    writedata(0x45);
    writedata(0x40);
    writedata(0x3B);
    writedata(0x32);

    writedata(0x2E);
    writedata(0x28);
    writedata(0x24);
    writedata(0x03);
    writedata(0x00);
    writedata(0x01);

    writecommand(0x36); // MADCTL Memory access control
    writedata(0x48);
    delay(20);

    writecommand(0x21); //Display inversion on
    delay(20);

    writecommand(0x29); // Display on

    delay(120);

// End of HX8357C

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework


_SETVCOM = const(0xB6)
_SLPOUT = const(0x11)
_TEON = const(0x35)
_SETPOWER = const(0xB1)
_SETCYC = const(0xB4)
_SETSTBA = const(0xC0)
_SETGAMMA = const(0xE0)
_MADCTL = const(0x36)
_INVON = const(0x21)
_DISPON = const(0x29)
_COLMOD = const(0x3A)


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class HX8357C(display_driver_framework.DisplayDriver):

_SWRESET = const(0x01)
time.sleep_ms(100)

_FRMCTR3 = const(0xB9)


_SETRGB = const(0xB3) x80\x00\x06\x06   (0x00, 0x00, 0x06 (7 frame?)
_SETCOM = const(0xB6)  x25
_SETOSC = const(0xB0)  x68
_SETPANEL = const(0xCC)  x05
_SETPWR1 = const(0xB1)  x00\x15\x1C\x1C\x83\xAA
_SETSTBA = const(0xC0)  x50\x50\x01\x3C\x1E\x08
_SETCYC = const(0xB4)  x02\x40\x00\x2A\x2A\x0D\x78
_SETGAMMA = const(0xE0)  x02\x0A\x11\x1d\x23\x35\x41\x4b\x4b\x42\x3A\x27\x1B\x08\x09\x03\x02\x0A\x11\x1d\x23\x35\x41\x4b\x4b\x42\x3A\x27\x1B\x08\x09\x03\x00\x01
_COLMOD = const(0x3A)  x55
_MADCTL = const(0x36)  xC0   (default = 0x00)
_TEON = const(0x35)   x00
_TEARLINE = const(0x44) x00\x02
_SLPOUT = const(0x11)

_DISPON = const(0x29)


    b"\xB9\x83\xFF\x83\x57\xFF"  # _SETC and delay 500ms
    b"\xB3"  # _SETRGB 0x80 enables SDO pin (0x00 disables)
    b"\xB6"  # _SETCOM -1.52V
    b"\xB0"  # _SETOSC Normal mode 70Hz, Idle mode 55 Hz
    b"\xCC"  # _SETPANEL BGR, Gate direction swapped
    b"\xB1"  # _SETPWR1 Not deep standby BT VSPR VSNR AP
    b"\xC0"  # _SETSTBA OPON normal OPON idle STBA GEN
    b"\xB4"  # _SETCYC NW 0x02 RTN DIV DUM DUM GDON GDOFF
    b"\xE0"  # _SETGAMMA
    b"\x3A"  # _COLMOD 16 bit
    b"\x36"  # _MADCTL
    b"\x35"  # _TEON TW off
    b"\x44"  # _TEARLINE
    b"\x11"  # _SLPOUT and delay 150 ms
    delay(150)
    b"\x36\x01\xA0"
    b"\x29    x32"  # _DISPON and delay 50 ms
    delay(50)
)


    def init(self):
        param_buf = bytearray(14)
        param_mv = memoryview(param_buf)

        self.set_params(_SLPOUT)

        time.sleep_ms(20)  # NOQA

        param_buf[0] = 0x07
        param_buf[1] = 0x42
        param_buf[2] = 0x18
        self.set_params(0xD0, param_mv[:3])

        param_buf[0] = 0x00
        param_buf[1] = 0x07
        param_buf[2] = 0x10
        self.set_params(0xD1, param_mv[:3])

        param_buf[0] = 0x01
        param_buf[1] = 0x02
        self.set_params(0xD2, param_mv[:2])

        param_buf[0] = 0x10
        param_buf[1] = 0x3B
        param_buf[2] = 0x00
        param_buf[3] = 0x02
        param_buf[4] = 0x11
        self.set_params(_PWR1, param_mv[:5])

        param_buf[0] = 0x08
        self.set_params(_VCMPCTL, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x32
        param_buf[2] = 0x36
        param_buf[3] = 0x45
        param_buf[4] = 0x06
        param_buf[5] = 0x16
        param_buf[6] = 0x37
        param_buf[7] = 0x75
        param_buf[8] = 0x77
        param_buf[9] = 0x54
        param_buf[10] = 0x0C
        param_buf[11] = 0x00
        self.set_params(0xC8, param_mv[:12])



        param_buf[0] = 0x00
        param_buf[1] = 0x00
        param_buf[2] = 0x01
        param_buf[3] = 0x3F
        self.set_params(_CASET, param_mv[:4])

        param_buf[3] = 0xDF
        self.set_params(_RASET, param_mv[:4])

        time.sleep_ms(120)  # NOQA
        self.set_params(_DISPON)
        time.sleep_ms(25)  # NOQA

        display_driver_framework.DisplayDriver.init(self)




    _SETEXTC = const(0xB9)


        time.sleep_ms(50)
        param_buf[0] = 0xFF
        param_buf[1] = 0x83
        param_buf[2] = 0x57
        self.set_params(_SETEXTC, param_mv[:3])

        param_buf[0] = 0x2C
        self.set_params(_SETVCOM, param_mv[:1])

        time.sleep_ms(200)
        self.set_params(_SLPOUT)

        param_buf[0] = 0x00
        self.set_params(_TEON, param_mv[:1])


        param_buf[0] = 0x35
        self.set_params(0x3A, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x15
        param_buf[2] = 0x0D
        param_buf[3] = 0x0D
        param_buf[4] = 0x83
        param_buf[5] = 0x48
        self.set_params(_SETPOWER, param_mv[:6])

        param_buf[0] = 0x24
        param_buf[1] = 0x24
        param_buf[2] = 0x01
        param_buf[3] = 0x3C
        param_buf[4] = 0xC8
        param_buf[5] = 0x08
        self.set_params(_SETSTBA, param_mv[:6])



        param_buf[0] = 0x02
        param_buf[1] = 0x40
        param_buf[2] = 0x00
        param_buf[3] = 0x2A
        param_buf[4] = 0x2A
        param_buf[5] = 0x0D
        param_buf[6] = 0x4F
        self.set_params(_SETCYC, param_mv[:7])

        param_buf[0] = 0x00
        param_buf[1] = 0x15
        param_buf[2] = 0x1D
        param_buf[3] = 0x2A
        param_buf[4] = 0x31
        param_buf[5] = 0x42
        param_buf[6] = 0x4C
        param_buf[7] = 0x53
        param_buf[8] = 0x45
        param_buf[9] = 0x40
        param_buf[10] = 0x3B
        param_buf[11] = 0x32
        param_buf[12] = 0x2E
        param_buf[13] = 0x28
        self.set_params(_SETGAMMA, param_mv[:14])

        time.sleep_ms(20)


        param_buf[0] = (
            self._madctl(
                self._color_byte_order,
                display_driver_framework._ORIENTATION_TABLE  # NOQA
            )
        )
        self.set_params(_MADCTL, param_mv[:1])

        color_size = lv.color_format_get_size(self._color_space)
        if color_size == 2:  # NOQA
            pixel_format = 0x55
        elif color_size == 3:
            pixel_format = 0x77
        else:
            raise RuntimeError(
                f'{self.__class__.__name__} IC only supports '
                'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
            )

        param_buf[0] = pixel_format
        self.set_params(_COLMOD, param_mv[:1])


        param_buf[0] = 0x24
        param_buf[1] = 0x03
        param_buf[2] = 0x00
        param_buf[3] = 0x15
        param_buf[4] = 0x1D
        param_buf[5] = 0x2A
        param_buf[6] = 0x31
        param_buf[7] = 0x42
        param_buf[8] = 0x4C
        param_buf[9] = 0x53
        param_buf[10] = 0x45
        param_buf[11] = 0x40
        param_buf[12] = 0x3B
        param_buf[13] = 0x32
        param_buf[14] = 0x2E
        param_buf[15] = 0x28
        param_buf[16] = 0x24
        param_buf[17] = 0x03
        param_buf[18] = 0x00
        param_buf[19] = 0x01
        param_buf[20] = 0x48
        self.set_params(_MADCTL, param_mv[:21])

        time.sleep_ms(20)
        self.set_params(_INVON)
        self.set_params(_DISPON)
        time.sleep_ms(120)
'''