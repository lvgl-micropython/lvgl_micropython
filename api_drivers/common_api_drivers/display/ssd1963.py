"""



// Delay between some initialisation commands
#define TFT_INIT_DELAY 0x80 // Not used unless commandlist invoked

// Generic commands used by TFT_eSPI.cpp
#define TFT_NOP     0x00
#define TFT_SWRST   0x01

#define TFT_INVOFF  0x20
#define TFT_INVON   0x21

#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29

#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C

#define TFT_RAMRD   0x2E
#define TFT_IDXRD   0xDD // ILI9341 only, indexed control register read

#define TFT_MADCTL  0x36
#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_RGB 0x00

#ifdef TFT_RGB_ORDER
  #if (TFT_RGB_ORDER == 1)
    #define TFT_MAD_COLOR_ORDER TFT_MAD_RGB
  #else
    #define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
  #endif
#else
  #define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
#endif


#if defined (SSD1963_480_DRIVER)

  writecommand(0xE2);   //PLL multiplier, set PLL clock to 120M
  writedata(0x23);      //N=0x36 for 6.5M, 0x23 for 10M crystal
  writedata(0x02);
  writedata(0x54);
  writecommand(0xE0);   // PLL enable
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);   // software reset

  delay(100);

  writecommand(0xE6);   //PLL setting for PCLK, depends on resolution
  writedata(0x01);
  writedata(0x1F);
  writedata(0xFF);

  writecommand(0xB0);   //LCD SPECIFICATION
  writedata(0x20);
  writedata(0x00);
  writedata(0x01);    //Set HDP 479
  writedata(0xDF);
  writedata(0x01);    //Set VDP 271
  writedata(0x0F);
  writedata(0x00);

  writecommand(0xB4);   //HSYNC
  writedata(0x02);    //Set HT  531
  writedata(0x13);
  writedata(0x00);    //Set HPS 8
  writedata(0x08);
  writedata(0x2B);    //Set HPW 43
  writedata(0x00);    //Set LPS 2
  writedata(0x02);
  writedata(0x00);

  writecommand(0xB6);   //VSYNC
  writedata(0x01);    //Set VT  288
  writedata(0x20);
  writedata(0x00);    //Set VPS 4
  writedata(0x04);
  writedata(0x0c);    //Set VPW 12
  writedata(0x00);    //Set FPS 2
  writedata(0x02);

  writecommand(0xBA);
  writedata(0x0F);    //GPIO[3:0] out 1

  writecommand(0xB8);
  writedata(0x07);      //GPIO3=input, GPIO[2:0]=output
  writedata(0x01);    //GPIO0 normal

  writecommand(0x36);   //rotation
  writedata(0x21 | TFT_MAD_COLOR_ORDER);

  writecommand(0xF0);   //pixel data interface
  writedata(0x00);      //8-bit bus

  delay(1);

  writecommand(0xB8);
  writedata(0x0f);    //GPIO is controlled by host GPIO[3:0]=output   GPIO[0]=1  LCD ON  GPIO[0]=1  LCD OFF
  writedata(0x01);    //GPIO0 normal

  writecommand(0xBA);
  writedata(0x01);    //GPIO[0] out 1 --- LCD display on/off control PIN

  writecommand(0x2A);
  writedata(0);
  writedata(0);
  writedata((271 & 0xFF00)>>8);
  writedata(271 & 0xFF);

  writecommand(0x2B);
  writedata(0);
  writedata(0);
  writedata((479 & 0xFF00)>>8);
  writedata(479 & 0xFF);

  writecommand(0x2C);

  writecommand(0x29);   //display on

  writecommand(0xBE);   //set PWM for B/L
  writedata(0x06);
  writedata(0xf0);
  writedata(0x01);
  writedata(0xf0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xd0);
  writedata(0x0d);

  writecommand(0x2C);


          param_buf[0] = 0x23
        param_buf[1] = 0x02
        param_buf[2] = 0x54
        self.set_params(0xE2, param_mv[:3])

        param_buf[0] = 0x01
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        param_buf[0] = 0x03
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        self.set_params(0x01)

        time.sleep_ms(100)
        param_buf[0] = 0x01
        param_buf[1] = 0x1F
        param_buf[2] = 0xFF
        self.set_params(0xE6, param_mv[:3])

        param_buf[0] = 0x20
        param_buf[1] = 0x00
        param_buf[2] = 0x01
        param_buf[3] = 0xDF
        param_buf[4] = 0x01
        param_buf[5] = 0x0F
        param_buf[6] = 0x00
        self.set_params(0xB0, param_mv[:7])

        param_buf[0] = 0x02
        param_buf[1] = 0x13
        param_buf[2] = 0x00
        param_buf[3] = 0x08
        param_buf[4] = 0x2B
        param_buf[5] = 0x00
        param_buf[6] = 0x02
        param_buf[7] = 0x00
        self.set_params(0xB4, param_mv[:8])

        param_buf[0] = 0x01
        param_buf[1] = 0x20
        param_buf[2] = 0x00
        param_buf[3] = 0x04
        param_buf[4] = 0x0c
        param_buf[5] = 0x00
        param_buf[6] = 0x02
        self.set_params(0xB6, param_mv[:7])

        param_buf[0] = 0x0F
        self.set_params(0xBA, param_mv[:1])

        param_buf[0] = 0x07
        param_buf[1] = 0x01
        self.set_params(0xB8, param_mv[:2])

        param_buf[0] = 0x21 | TFT_MAD_COLOR_ORDER
        self.set_params(0x36, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(0xF0, param_mv[:1])

        time.sleep_ms(1)
        param_buf[0] = 0x0f
        param_buf[1] = 0x01
        self.set_params(0xB8, param_mv[:2])

        param_buf[0] = 0x01
        self.set_params(0xBA, param_mv[:1])

        param_buf[0] = 0
        param_buf[1] = 0
        param_buf[2] = (271 & 0xFF00
        param_buf[3] = 271 & 0xFF
        self.set_params(0x2A, param_mv[:4])

        param_buf[0] = 0
        param_buf[1] = 0
        param_buf[2] = (479 & 0xFF00
        param_buf[3] = 479 & 0xFF
        self.set_params(0x2B, param_mv[:4])

        self.set_params(0x2C)

        self.set_params(0x29)

        param_buf[0] = 0x06
        param_buf[1] = 0xf0
        param_buf[2] = 0x01
        param_buf[3] = 0xf0
        param_buf[4] = 0x00
        param_buf[5] = 0x00
        self.set_params(0xBE, param_mv[:6])

        param_buf[0] = 0x0d
        self.set_params(0xd0, param_mv[:1])

        self.set_params(0x2C)

#elif defined (SSD1963_800_DRIVER)

  writecommand(0xE2);   //PLL multiplier, set PLL clock to 120M
  writedata(0x1E);      //N=0x36 for 6.5M, 0x23 for 10M crystal
  writedata(0x02);
  writedata(0x54);
  writecommand(0xE0);   // PLL enable
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);   // software reset

  delay(100);

  writecommand(0xE6);   //PLL setting for PCLK, depends on resolution
  writedata(0x03);
  writedata(0xFF);
  writedata(0xFF);

  writecommand(0xB0);   //LCD SPECIFICATION
  writedata(0x20);
  writedata(0x00);
  writedata(0x03);    //Set HDP 799
  writedata(0x1F);
  writedata(0x01);    //Set VDP 479
  writedata(0xDF);
  writedata(0x00);

  writecommand(0xB4);   //HSYNC
  writedata(0x03);    //Set HT  928
  writedata(0xA0);
  writedata(0x00);    //Set HPS 46
  writedata(0x2E);
  writedata(0x30);    //Set HPW 48
  writedata(0x00);    //Set LPS 15
  writedata(0x0F);
  writedata(0x00);

  writecommand(0xB6);   //VSYNC
  writedata(0x02);    //Set VT  525
  writedata(0x0D);
  writedata(0x00);    //Set VPS 16
  writedata(0x10);
  writedata(0x10);    //Set VPW 16
  writedata(0x00);    //Set FPS 8
  writedata(0x08);

  writecommand(0xBA);
  writedata(0x0F);    //GPIO[3:0] out 1

  writecommand(0xB8);
  writedata(0x07);      //GPIO3=input, GPIO[2:0]=output
  writedata(0x01);    //GPIO0 normal

  writecommand(0x36);   //rotation
  writedata(0x21 | TFT_MAD_COLOR_ORDER);

  writecommand(0xF0);   //pixel data interface
  writedata(0x00);      //8-bit bus

  delay(1);

  writecommand(0xB8);
  writedata(0x0f);    //GPIO is controlled by host GPIO[3:0]=output   GPIO[0]=1  LCD ON  GPIO[0]=1  LCD OFF
  writedata(0x01);    //GPIO0 normal

  writecommand(0xBA);
  writedata(0x01);    //GPIO[0] out 1 --- LCD display on/off control PIN

  writecommand(0x2A);
  writedata(0);
  writedata(0);
  writedata((479 & 0xFF00)>>8);
  writedata(479 & 0xFF);

  writecommand(0x2B);
  writedata(0);
  writedata(0);
  writedata((799 & 0xFF00)>>8);
  writedata(799 & 0xFF);

  writecommand(0x2C);

  writecommand(0x29);   //display on

  writecommand(0xBE);   //set PWM for B/L
  writedata(0x06);
  writedata(0xf0);
  writedata(0x01);
  writedata(0xf0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xd0);
  writedata(0x0d);

  writecommand(0x2C);



          param_buf[0] = 0x1E
        param_buf[1] = 0x02
        param_buf[2] = 0x54
        self.set_params(0xE2, param_mv[:3])

        param_buf[0] = 0x01
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        param_buf[0] = 0x03
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        self.set_params(0x01)

        time.sleep_ms(100)
        param_buf[0] = 0x03
        param_buf[1] = 0xFF
        param_buf[2] = 0xFF
        self.set_params(0xE6, param_mv[:3])

        param_buf[0] = 0x20
        param_buf[1] = 0x00
        param_buf[2] = 0x03
        param_buf[3] = 0x1F
        param_buf[4] = 0x01
        param_buf[5] = 0xDF
        param_buf[6] = 0x00
        self.set_params(0xB0, param_mv[:7])

        param_buf[0] = 0x03
        param_buf[1] = 0xA0
        param_buf[2] = 0x00
        param_buf[3] = 0x2E
        param_buf[4] = 0x30
        param_buf[5] = 0x00
        param_buf[6] = 0x0F
        param_buf[7] = 0x00
        self.set_params(0xB4, param_mv[:8])

        param_buf[0] = 0x02
        param_buf[1] = 0x0D
        param_buf[2] = 0x00
        param_buf[3] = 0x10
        param_buf[4] = 0x10
        param_buf[5] = 0x00
        param_buf[6] = 0x08
        self.set_params(0xB6, param_mv[:7])

        param_buf[0] = 0x0F
        self.set_params(0xBA, param_mv[:1])

        param_buf[0] = 0x07
        param_buf[1] = 0x01
        self.set_params(0xB8, param_mv[:2])

        param_buf[0] = 0x21 | TFT_MAD_COLOR_ORDER
        self.set_params(0x36, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(0xF0, param_mv[:1])

        time.sleep_ms(1)
        param_buf[0] = 0x0f
        param_buf[1] = 0x01
        self.set_params(0xB8, param_mv[:2])

        param_buf[0] = 0x01
        self.set_params(0xBA, param_mv[:1])

        param_buf[0] = 0
        param_buf[1] = 0
        param_buf[2] = (479 & 0xFF00
        param_buf[3] = 479 & 0xFF
        self.set_params(0x2A, param_mv[:4])

        param_buf[0] = 0
        param_buf[1] = 0
        param_buf[2] = (799 & 0xFF00
        param_buf[3] = 799 & 0xFF
        self.set_params(0x2B, param_mv[:4])

        self.set_params(0x2C)

        self.set_params(0x29)

        param_buf[0] = 0x06
        param_buf[1] = 0xf0
        param_buf[2] = 0x01
        param_buf[3] = 0xf0
        param_buf[4] = 0x00
        param_buf[5] = 0x00
        self.set_params(0xBE, param_mv[:6])

        param_buf[0] = 0x0d
        self.set_params(0xd0, param_mv[:1])

        self.set_params(0x2C)

#elif defined (SSD1963_800ALT_DRIVER)

  writecommand(0xE2);   //PLL multiplier, set PLL clock to 120M
  writedata(0x23);      //N=0x36 for 6.5M, 0x23 for 10M crystal
  writedata(0x02);
  writedata(0x04);
  writecommand(0xE0);   // PLL enable
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);   // software reset

  delay(100);

  writecommand(0xE6);   //PLL setting for PCLK, depends on resolution
  writedata(0x04);
  writedata(0x93);
  writedata(0xE0);

  writecommand(0xB0);   //LCD SPECIFICATION
  writedata(0x00);  // 0x24
  writedata(0x00);
  writedata(0x03);    //Set HDP 799
  writedata(0x1F);
  writedata(0x01);    //Set VDP 479
  writedata(0xDF);
  writedata(0x00);

  writecommand(0xB4);   //HSYNC
  writedata(0x03);    //Set HT  928
  writedata(0xA0);
  writedata(0x00);    //Set HPS 46
  writedata(0x2E);
  writedata(0x30);    //Set HPW 48
  writedata(0x00);    //Set LPS 15
  writedata(0x0F);
  writedata(0x00);

  writecommand(0xB6);   //VSYNC
  writedata(0x02);    //Set VT  525
  writedata(0x0D);
  writedata(0x00);    //Set VPS 16
  writedata(0x10);
  writedata(0x10);    //Set VPW 16
  writedata(0x00);    //Set FPS 8
  writedata(0x08);

  writecommand(0xBA);
  writedata(0x05);    //GPIO[3:0] out 1

  writecommand(0xB8);
  writedata(0x07);      //GPIO3=input, GPIO[2:0]=output
  writedata(0x01);    //GPIO0 normal

  writecommand(0x36);   //rotation
  writedata(0x21 | TFT_MAD_COLOR_ORDER);    // -- Set rotation

  writecommand(0xF0);   //pixel data interface
  writedata(0x00);      //8-bit bus

  delay(10);

  writecommand(0x2A);
  writedata(0);
  writedata(0);
  writedata((479 & 0xFF00)>>8);
  writedata(479 & 0xFF);

  writecommand(0x2B);
  writedata(0);
  writedata(0);
  writedata((799 & 0xFF00)>>8);
  writedata(799 & 0xFF);

  writecommand(0x2C);

  writecommand(0x29);   //display on

  writecommand(0xBE);   //set PWM for B/L
  writedata(0x06);
  writedata(0xF0);
  writedata(0x01);
  writedata(0xF0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xD0);
  writedata(0x0D);

  writecommand(0x2C);


          param_buf[0] = 0x23
        param_buf[1] = 0x02
        param_buf[2] = 0x04
        self.set_params(0xE2, param_mv[:3])

        param_buf[0] = 0x01
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        param_buf[0] = 0x03
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        self.set_params(0x01)

        time.sleep_ms(100)
        param_buf[0] = 0x04
        param_buf[1] = 0x93
        param_buf[2] = 0xE0
        self.set_params(0xE6, param_mv[:3])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        param_buf[2] = 0x03
        param_buf[3] = 0x1F
        param_buf[4] = 0x01
        param_buf[5] = 0xDF
        param_buf[6] = 0x00
        self.set_params(0xB0, param_mv[:7])

        param_buf[0] = 0x03
        param_buf[1] = 0xA0
        param_buf[2] = 0x00
        param_buf[3] = 0x2E
        param_buf[4] = 0x30
        param_buf[5] = 0x00
        param_buf[6] = 0x0F
        param_buf[7] = 0x00
        self.set_params(0xB4, param_mv[:8])

        param_buf[0] = 0x02
        param_buf[1] = 0x0D
        param_buf[2] = 0x00
        param_buf[3] = 0x10
        param_buf[4] = 0x10
        param_buf[5] = 0x00
        param_buf[6] = 0x08
        self.set_params(0xB6, param_mv[:7])

        param_buf[0] = 0x05
        self.set_params(0xBA, param_mv[:1])

        param_buf[0] = 0x07
        param_buf[1] = 0x01
        self.set_params(0xB8, param_mv[:2])

        param_buf[0] = 0x21 | TFT_MAD_COLOR_ORDER
        self.set_params(0x36, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(0xF0, param_mv[:1])

        time.sleep_ms(10)
        param_buf[0] = 0
        param_buf[1] = 0
        param_buf[2] = (479 & 0xFF00
        param_buf[3] = 479 & 0xFF
        self.set_params(0x2A, param_mv[:4])

        param_buf[0] = 0
        param_buf[1] = 0
        param_buf[2] = (799 & 0xFF00
        param_buf[3] = 799 & 0xFF
        self.set_params(0x2B, param_mv[:4])

        self.set_params(0x2C)

        self.set_params(0x29)

        param_buf[0] = 0x06
        param_buf[1] = 0xF0
        param_buf[2] = 0x01
        param_buf[3] = 0xF0
        param_buf[4] = 0x00
        param_buf[5] = 0x00
        self.set_params(0xBE, param_mv[:6])

        param_buf[0] = 0x0D
        self.set_params(0xD0, param_mv[:1])

        self.set_params(0x2C)

#elif defined (SSD1963_800BD_DRIVER) // Copied from Buy Display code

  writecommand(0xE2);   //PLL multiplier, set PLL clock to 120M
  writedata(0x23);      //N=0x36 for 6.5M, 0x23 for 10M crystal
  writedata(0x02);
  writedata(0x54);

  writecommand(0xE0);   // PLL enable
  writedata(0x01);

  delay(10);

  writecommand(0xE0);
  writedata(0x03);

  delay(10);

  writecommand(0x01);   // software reset

  delay(100);

  writecommand(0xE6);   //PLL setting for PCLK, depends on resolution
  writedata(0x03);
  writedata(0x33);
  writedata(0x33);

  writecommand(0xB0);    //LCD SPECIFICATION
  writedata(0x20);
  writedata(0x00);
  writedata(799 >> 8);   //Set HDP 799
  writedata(799 & 0xFF);
  writedata(479 >> 8);   //Set VDP 479
  writedata(479 & 0xFF);
  writedata(0x00);

  writecommand(0xB4);   //HSYNC
  writedata(0x04);      //Set HT
  writedata(0x1F);
  writedata(0x00);      //Set HPS
  writedata(0xD2);
  writedata(0x00);      //Set HPW
  writedata(0x00);      //Set LPS
  writedata(0x00);
  writedata(0x00);

  writecommand(0xB6);   //VSYNC
  writedata(0x02);    //Set VT
  writedata(0x0C);
  writedata(0x00);    //Set VPS
  writedata(0x22);
  writedata(0x00);    //Set VPW
  writedata(0x00);    //Set FPS
  writedata(0x00);

  writecommand(0xB8);
  writedata(0x0F);      //GPIO3=input, GPIO[2:0]=output
  writedata(0x01);      //GPIO0 normal

  writecommand(0xBA);
  writedata(0x01);    //GPIO[0] out 1 --- LCD display on/off control PIN

  writecommand(0x36);   //rotation
  writedata(0x21 | TFT_MAD_COLOR_ORDER);      //set to rotate

	//writecommand(0x003A); //Set the current pixel format for RGB image data
	//writedata(0x0050);    //16-bit/pixel

  writecommand(0xF0);   //pixel data interface
  writedata(0x00);      //000 = 8-bit bus, 011 = 16-bit, 110 = 9-bit

  writecommand(0xBC);
  writedata(0x40);     //contrast value
  writedata(0x80);     //brightness value
  writedata(0x40);     //saturation value
  writedata(0x01);     //Post Processor Enable


  delay(10);

  writecommand(0x29);   //display on

  writecommand(0xBE);   //set PWM for B/L
  writedata(0x06);
  writedata(0x80);
  writedata(0x01);
  writedata(0xF0);
  writedata(0x00);
  writedata(0x00);

  writecommand(0xD0);
  writedata(0x0D);

#endif


        param_buf[0] = 0x23
        param_buf[1] = 0x02
        param_buf[2] = 0x54
        self.set_params(0xE2, param_mv[:3])

        param_buf[0] = 0x01
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        param_buf[0] = 0x03
        self.set_params(0xE0, param_mv[:1])

        time.sleep_ms(10)
        self.set_params(0x01)

        time.sleep_ms(100)
        param_buf[0] = 0x03
        param_buf[1] = 0x33
        param_buf[2] = 0x33
        self.set_params(0xE6, param_mv[:3])

        param_buf[0] = 0x20
        param_buf[1] = 0x00
        param_buf[2] = 799 >> 8
        param_buf[3] = 799 & 0xFF
        param_buf[4] = 479 >> 8
        param_buf[5] = 479 & 0xFF
        param_buf[6] = 0x00
        self.set_params(0xB0, param_mv[:7])

        param_buf[0] = 0x04
        param_buf[1] = 0x1F
        param_buf[2] = 0x00
        param_buf[3] = 0xD2
        param_buf[4] = 0x00
        param_buf[5] = 0x00
        param_buf[6] = 0x00
        param_buf[7] = 0x00
        self.set_params(0xB4, param_mv[:8])

        param_buf[0] = 0x02
        param_buf[1] = 0x0C
        param_buf[2] = 0x00
        param_buf[3] = 0x22
        param_buf[4] = 0x00
        param_buf[5] = 0x00
        param_buf[6] = 0x00
        self.set_params(0xB6, param_mv[:7])

        param_buf[0] = 0x0F
        param_buf[1] = 0x01
        self.set_params(0xB8, param_mv[:2])

        param_buf[0] = 0x01
        self.set_params(0xBA, param_mv[:1])

        param_buf[0] = 0x21 | TFT_MAD_COLOR_ORDER
        self.set_params(0x36, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(0xF0, param_mv[:1])

        param_buf[0] = 0x40
        param_buf[1] = 0x80
        param_buf[2] = 0x40
        param_buf[3] = 0x01
        self.set_params(0xBC, param_mv[:4])

        time.sleep_ms(10)
        self.set_params(0x29)

        param_buf[0] = 0x06
        param_buf[1] = 0x80
        param_buf[2] = 0x01
        param_buf[3] = 0xF0
        param_buf[4] = 0x00
        param_buf[5] = 0x00
        self.set_params(0xBE, param_mv[:6])

        param_buf[0] = 0x0D
        self.set_params(0xD0, param_mv[:1])
"""