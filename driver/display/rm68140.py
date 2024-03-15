'''


// Delay between some initialisation commands
#define TFT_INIT_DELAY 0x80 // Not used unless commandlist invoked


// Generic commands used by TFT_eSPI.cpp
#define TFT_NOP     0x00
#define TFT_SWRST   0x01

#define TFT_SLPIN   0x10
#define TFT_SLPOUT  0x11

#define TFT_INVOFF  0x20
#define TFT_INVON   0x21

#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29

#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C

#define TFT_RAMRD   0x2E

#define TFT_MADCTL  0x36

#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_RGB 0x00
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_SS  0x02
#define TFT_MAD_GS  0x01

#define TFT_IDXRD   0x00 // ILI9341 only, indexed control register read


    writecommand(TFT_SLPOUT);
    delay(20);

    writecommand(0xD0);
    writedata(0x07);
    writedata(0x42);
    writedata(0x18);

    writecommand(0xD1);
    writedata(0x00);
    writedata(0x07);
    writedata(0x10);

    writecommand(0xD2);
    writedata(0x01);
    writedata(0x02);

    writecommand(0xC0);
    writedata(0x10);
    writedata(0x3B);
    writedata(0x00);
    writedata(0x02);
    writedata(0x11);

    writecommand(0xC5);
    writedata(0x03);

    writecommand(0xC8);
    writedata(0x00);
    writedata(0x32);
    writedata(0x36);
    writedata(0x45);
    writedata(0x06);
    writedata(0x16);
    writedata(0x37);
    writedata(0x75);
    writedata(0x77);
    writedata(0x54);
    writedata(0x0C);
    writedata(0x00);

    writecommand(TFT_MADCTL);
    writedata(0x0A);

    writecommand(0x3A);
    writedata(0x55);

    writecommand(TFT_CASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0x3F);

    writecommand(TFT_PASET);
    writedata(0x00);
    writedata(0x00);
    writedata(0x01);
    writedata(0xDF);

    delay(120);
    writecommand(TFT_DISPON);

    delay(25);





            time.sleep_ms(20)
        self.set_params(TFT_SLPOUT)

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
        self.set_params(0xC0, param_mv[:5])

        param_buf[0] = 0x03
        self.set_params(0xC5, param_mv[:1])

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

        param_buf[0] = 0x0A
        self.set_params(TFT_MADCTL, param_mv[:1])

        param_buf[0] = 0x55
        self.set_params(0x3A, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        param_buf[2] = 0x01
        param_buf[3] = 0x3F
        self.set_params(TFT_CASET, param_mv[:4])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        param_buf[2] = 0x01
        param_buf[3] = 0xDF
        self.set_params(TFT_PASET, param_mv[:4])

        time.sleep_ms(120)
        self.set_params(TFT_DISPON)

        time.sleep_ms(25)

'''