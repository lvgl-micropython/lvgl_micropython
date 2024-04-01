'''
#define TFT_NOP     0x00
#define TFT_SWRST   TFT_NOP
#define TFT_CASET   0x15 // SETCOLUMN
#define TFT_PASET   0x75 // SETROW
#define TFT_RAMWR   0x5C // WRITERAM
#define TFT_RAMRD   0x5D // READRAM
#define TFT_IDXRD   TFT_NOP
#define TFT_INVOFF  0xA6 // NORMALDISPLAY
#define TFT_INVON   0xA7 // INVERTDISPLAY


  writecommand(0xFD); // COMMANDLOCK
  writedata(0x12);
  writecommand(0xFD); // COMMANDLOCK
  writedata(0xB1);
  writecommand(0xAE); // DISPLAYOFF
  writecommand(0xB3); // CLOCKDIV
  writedata(0xF1);
  writecommand(0xCA); // MUXRATIO
  writedata(127);
  writecommand(0xA2); // DISPLAYOFFSET
  writedata(0x00);
  writecommand(0xB5); // SETGPIO
  writedata(0x00);
  writecommand(0xAB); // FUNCTIONSELECT
  writedata(0x01);
  writecommand(0xB1); // PRECHARGE
  writedata(0x32);
  writecommand(0xBE); // VCOMH
  writedata(0x05);
  writecommand(0xA6); // NORMALDISPLAY
  writecommand(0xC1); // CONTRASTABC
  writedata(0xC8);
  writedata(0x80);
  writedata(0xC8);
  writecommand(0xC7); // CONTRASTMASTER
  writedata(0x0F);
  writecommand(0xB4); // SETVSL
  writedata(0xA0);
  writedata(0xB5);
  writedata(0x55);
  writecommand(0xB6); // PRECHARGE2
  writedata(0x01);
  writecommand(0xAF); // DISPLAYON



          param_buf[0] = 0x12
        self.set_params(0xFD, param_mv[:1])

        param_buf[0] = 0xB1
        self.set_params(0xFD, param_mv[:1])

        self.set_params(0xAE)

        param_buf[0] = 0xF1
        self.set_params(0xB3, param_mv[:1])

        param_buf[0] = 127
        self.set_params(0xCA, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(0xA2, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(0xB5, param_mv[:1])

        param_buf[0] = 0x01
        self.set_params(0xAB, param_mv[:1])

        param_buf[0] = 0x32
        self.set_params(0xB1, param_mv[:1])

        param_buf[0] = 0x05
        self.set_params(0xBE, param_mv[:1])

        self.set_params(0xA6)

        param_buf[0] = 0xC8
        param_buf[1] = 0x80
        param_buf[2] = 0xC8
        self.set_params(0xC1, param_mv[:3])

        param_buf[0] = 0x0F
        self.set_params(0xC7, param_mv[:1])

        param_buf[0] = 0xA0
        param_buf[1] = 0xB5
        param_buf[2] = 0x55
        self.set_params(0xB4, param_mv[:3])

        param_buf[0] = 0x01
        self.set_params(0xB6, param_mv[:1])

        self.set_params(0xAF)

'''