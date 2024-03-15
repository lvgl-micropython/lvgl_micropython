'''




#define CMD_DISPLAY_OFF         0xAE
#define CMD_DISPLAY_ON          0xAF

#define CMD_SET_DISP_START_LINE 0x40
#define CMD_SET_PAGE            0xB0

#define CMD_SET_COLUMN_UPPER    0x10
#define CMD_SET_COLUMN_LOWER    0x00

#define CMD_SET_ADC_NORMAL      0xA0
#define CMD_SET_ADC_REVERSE     0xA1

#define CMD_SET_DISP_NORMAL     0xA6
#define CMD_SET_DISP_REVERSE    0xA7

#define CMD_SET_ALLPTS_NORMAL   0xA4
#define CMD_SET_ALLPTS_ON       0xA5
#define CMD_SET_BIAS_9          0xA2
#define CMD_SET_BIAS_7          0xA3

#define CMD_RMW                 0xE0
#define CMD_RMW_CLEAR           0xEE
#define CMD_INTERNAL_RESET      0xE2
#define CMD_SET_COM_NORMAL      0xC0
#define CMD_SET_COM_REVERSE     0xC8
#define CMD_SET_POWER_CONTROL   0x28
#define CMD_SET_RESISTOR_RATIO  0x20
#define CMD_SET_VOLUME_FIRST    0x81
#define CMD_SET_VOLUME_SECOND   0x00
#define CMD_SET_STATIC_OFF      0xAC
#define CMD_SET_STATIC_ON       0xAD
#define CMD_SET_STATIC_REG      0x00
#define CMD_SET_BOOSTER_FIRST   0xF8
#define CMD_SET_BOOSTER_234     0x00
#define CMD_SET_BOOSTER_5       0x01
#define CMD_SET_BOOSTER_6       0x03
#define CMD_NOP                 0xE3
#define CMD_TEST                0xF0



{
    LV_DRV_DISP_RST(1);
    LV_DRV_DELAY_MS(10);
    LV_DRV_DISP_RST(0);
    LV_DRV_DELAY_MS(10);
    LV_DRV_DISP_RST(1);
    LV_DRV_DELAY_MS(10);

    LV_DRV_DISP_SPI_CS(0);

    st7565_command(CMD_SET_BIAS_7);
    st7565_command(CMD_SET_ADC_NORMAL);
    st7565_command(CMD_SET_COM_NORMAL);
    st7565_command(CMD_SET_DISP_START_LINE);
    st7565_command(CMD_SET_POWER_CONTROL | 0x4);
    LV_DRV_DELAY_MS(50);

    st7565_command(CMD_SET_POWER_CONTROL | 0x6);
    LV_DRV_DELAY_MS(50);

    st7565_command(CMD_SET_POWER_CONTROL | 0x7);
    LV_DRV_DELAY_MS(10);

    st7565_command(CMD_SET_RESISTOR_RATIO | 0x6); // Defaulted to 0x26 (but could also be between 0x20-0x27 based on display's specs)

    st7565_command(CMD_DISPLAY_ON);
    st7565_command(CMD_SET_ALLPTS_NORMAL);

    /*Set brightness*/
    st7565_command(CMD_SET_VOLUME_FIRST);
    st7565_command(CMD_SET_VOLUME_SECOND | (0x18 & 0x3f));
'''