'''

#define HX8357_NOP     0x00
#define HX8357_SWRESET 0x01
#define HX8357_RDDID   0x04
#define HX8357_RDDST   0x09

#define HX8357_RDPOWMODE  0x0A
#define HX8357_RDMADCTL  0x0B
#define HX8357_RDCOLMOD  0x0C
#define HX8357_RDDIM  0x0D
#define HX8357_RDDSDR  0x0F

#define HX8357_SLPIN   0x10
#define HX8357_SLPOUT  0x11

#define HX8357_INVOFF  0x20
#define HX8357_INVON   0x21
#define HX8357_DISPOFF 0x28
#define HX8357_DISPON  0x29

#define HX8357_CASET   0x2A
#define HX8357_PASET   0x2B
#define HX8357_RAMWR   0x2C
#define HX8357_RAMRD   0x2E

#define HX8357_TEON  0x35
#define HX8357_TEARLINE  0x44
#define HX8357_MADCTL  0x36
#define HX8357_COLMOD  0x3A

#define HX8357_SETOSC 0xB0
#define HX8357_SETPWR1 0xB1
#define HX8357_SETRGB 0xB3
#define HX8357D_SETCOM  0xB6

#define HX8357D_SETCYC  0xB4
#define HX8357D_SETC 0xB9

#define HX8357D_SETSTBA 0xC0

#define HX8357_SETPANEL  0xCC

#define HX8357D_SETGAMMA 0xE0



// Configure HX8357D display

    // setextc
    writecommand(HX8357D_SETC);
    writedata(0xFF);
    writedata(0x83);
    writedata(0x57);
    delay(300);

    // setRGB which also enables SDO
    writecommand(HX8357_SETRGB);
    writedata(0x80);  //enable SDO pin!
//  writedata(0x00);  //disable SDO pin!
    writedata(0x0);
    writedata(0x06);
    writedata(0x06);

    writecommand(HX8357D_SETCOM);
    writedata(0x25);  // -1.52V

    writecommand(HX8357_SETOSC);
    writedata(0x68);  // Normal mode 70Hz, Idle mode 55 Hz

    writecommand(HX8357_SETPANEL); //Set Panel
    writedata(0x05);  // BGR, Gate direction swapped

    writecommand(HX8357_SETPWR1);
    writedata(0x00);  // Not deep standby
    writedata(0x15);  //BT
    writedata(0x1C);  //VSPR
    writedata(0x1C);  //VSNR
    writedata(0x83);  //AP
    writedata(0xAA);  //FS

    writecommand(HX8357D_SETSTBA);
    writedata(0x50);  //OPON normal
    writedata(0x50);  //OPON idle
    writedata(0x01);  //STBA
    writedata(0x3C);  //STBA
    writedata(0x1E);  //STBA
    writedata(0x08);  //GEN

    writecommand(HX8357D_SETCYC);
    writedata(0x02);  //NW 0x02
    writedata(0x40);  //RTN
    writedata(0x00);  //DIV
    writedata(0x2A);  //DUM
    writedata(0x2A);  //DUM
    writedata(0x0D);  //GDON
    writedata(0x78);  //GDOFF

    writecommand(HX8357D_SETGAMMA);
    writedata(0x02);
    writedata(0x0A);
    writedata(0x11);
    writedata(0x1d);
    writedata(0x23);
    writedata(0x35);
    writedata(0x41);
    writedata(0x4b);
    writedata(0x4b);
    writedata(0x42);
    writedata(0x3A);
    writedata(0x27);
    writedata(0x1B);
    writedata(0x08);
    writedata(0x09);
    writedata(0x03);
    writedata(0x02);
    writedata(0x0A);
    writedata(0x11);
    writedata(0x1d);
    writedata(0x23);
    writedata(0x35);
    writedata(0x41);
    writedata(0x4b);
    writedata(0x4b);
    writedata(0x42);
    writedata(0x3A);
    writedata(0x27);
    writedata(0x1B);
    writedata(0x08);
    writedata(0x09);
    writedata(0x03);
    writedata(0x00);
    writedata(0x01);

    writecommand(HX8357_COLMOD);
    writedata(0x55);  // 16-bit

    writecommand(HX8357_MADCTL);
    writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER);

    writecommand(HX8357_TEON);  // TE off
    writedata(0x00);

    writecommand(HX8357_TEARLINE);  // tear line
    writedata(0x00);
    writedata(0x02);

    writecommand(HX8357_SLPOUT);  //Exit Sleep
    delay(150);

    writecommand(HX8357_DISPON);  // display on
    delay(50);

// End of HX8357D display configuration



        time.sleep_ms(300)
        param_buf[0] = 0xFF
        param_buf[1] = 0x83
        param_buf[2] = 0x57
        self.set_params(HX8357D_SETC, param_mv[:3])

        param_buf[0] = 0x80
        self.set_params(HX8357_SETRGB, param_mv[:1])

        param_buf[0] = 0x0
        param_buf[1] = 0x06
        param_buf[2] = 0x06
        param_buf[3] = 0x25
        self.set_params(HX8357D_SETCOM, param_mv[:4])

        param_buf[0] = 0x68
        self.set_params(HX8357_SETOSC, param_mv[:1])

        param_buf[0] = 0x05
        self.set_params(HX8357_SETPANEL, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x15
        param_buf[2] = 0x1C
        param_buf[3] = 0x1C
        param_buf[4] = 0x83
        param_buf[5] = 0xAA
        self.set_params(HX8357_SETPWR1, param_mv[:6])

        param_buf[0] = 0x50
        param_buf[1] = 0x50
        param_buf[2] = 0x01
        param_buf[3] = 0x3C
        param_buf[4] = 0x1E
        param_buf[5] = 0x08
        self.set_params(HX8357D_SETSTBA, param_mv[:6])

        param_buf[0] = 0x02
        param_buf[1] = 0x40
        param_buf[2] = 0x00
        param_buf[3] = 0x2A
        param_buf[4] = 0x2A
        param_buf[5] = 0x0D
        param_buf[6] = 0x78
        self.set_params(HX8357D_SETCYC, param_mv[:7])

        param_buf[0] = 0x02
        param_buf[1] = 0x0A
        param_buf[2] = 0x11
        param_buf[3] = 0x1d
        param_buf[4] = 0x23
        param_buf[5] = 0x35
        param_buf[6] = 0x41
        param_buf[7] = 0x4b
        param_buf[8] = 0x4b
        param_buf[9] = 0x42
        param_buf[10] = 0x3A
        param_buf[11] = 0x27
        param_buf[12] = 0x1B
        param_buf[13] = 0x08
        param_buf[14] = 0x09
        param_buf[15] = 0x03
        param_buf[16] = 0x02
        param_buf[17] = 0x0A
        param_buf[18] = 0x11
        param_buf[19] = 0x1d
        param_buf[20] = 0x23
        param_buf[21] = 0x35
        param_buf[22] = 0x41
        param_buf[23] = 0x4b
        param_buf[24] = 0x4b
        param_buf[25] = 0x42
        param_buf[26] = 0x3A
        param_buf[27] = 0x27
        param_buf[28] = 0x1B
        param_buf[29] = 0x08
        param_buf[30] = 0x09
        param_buf[31] = 0x03
        param_buf[32] = 0x00
        param_buf[33] = 0x01
        self.set_params(HX8357D_SETGAMMA, param_mv[:34])

        param_buf[0] = 0x55
        self.set_params(HX8357_COLMOD, param_mv[:1])

        param_buf[0] = TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_COLOR_ORDER
        self.set_params(HX8357_MADCTL, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(HX8357_TEON, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x02
        self.set_params(HX8357_TEARLINE, param_mv[:2])

        time.sleep_ms(150)
        self.set_params(HX8357_SLPOUT)

        time.sleep_ms(50)
        self.set_params(HX8357_DISPON)
'''