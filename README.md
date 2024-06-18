# LVGL binding for Micropython
______________________________

I have tried to make this as simple as possible for paople to use. 
There are some glitches still in it I am sure. If you come across an issue 
please let me know.

I am still in development mode for the unix port. I am writing an SDL driver 
that conforms To the rest of the driver framework. I have started working on 
writing the frameworks for the different indev (input) types that LVGL supports.
The frameworks are written to make it easier to write display and input drivers
for the binding.

<br>

### *New changes*
___________________________

ESP32-ALL
Flash sizes that are able to be used are 4, 8, 16, 32, 64 and 128 across all 
variants of the ESP32. It is up to the user to know what their board is using.

--ota if you want to set the partitions so you can do an over the air update of 
the firmware. I do want to note that this does take up twice as much application 
storage space. This feature applies to any board.


ESP32-S3
2 new command line arguments.
--onboard-mem-speed: allowed values = 120 or 80
--flash-mode: allowed values = QIO, QOUT, DIO, DOUT, OPI, DTR, STR


OK so this is how this works is wanting to use 120,hz speed
* octal spi FLASH, octal spi PSRAM: `--onboard-mem-speed=120 --flash-mode=DTR`
* quad spi FLASH, octal spi PSRAM: `--onboard-mem-speed=120 --flash-mode=STR`
* quad spi FLASH, quad spi PSRAM: `--onboard-mem-speed=120`

There is one other use case. If you have 32mb worth of flash storage you will 
need to set `--flash-mode=DOUT` as well as `--flash-size=32`


ESP32
MicroPython SPI class has been modified
The SPI implimentation in MicroPython for the ESP32 was written so it would fail
if the SPI bus had already been initialized. This would cause an issue due to the
creation order of the display and touch drivers. The display driver needs to be 
initilized before the touch driver does and if the display bus uses SPI the creation 
of the touch driver would fail because the bus would already be started. Another 
issue was the MicroPython SPI driver didn't handle the CS line. This was added for 
ease of use.

The MicroPython SDCard class has been modified
There were limitations placed on using SPI for an SDCard. This was due to the original 
SPI not hanmdling the CS line. Since this has now taken place we needed to modify the SDCard class
to allow for the SPI bus to be shared. The constructor has been modified, the miso, mosi and sclk 
parameters have been removed. A new parameter has been added which is `spi_bus` and this parameter 
takes a `machine.SPI` instance. If you want to share the bus you need to also supply a CS pin using 
the `cs` parameter. All parameters that are used for setting pins now only take integer values, 
`machine.Pin` instances are no longer allowed.

The `lcd_bus.SPIBus` constructor has changed. The miso, mosi, wp, hd and sclk parameters have been removed.
A new parameter has been added which is `spi_bus` and this parameter  takes a `machine.SPI` instance. 
This was done to allow for sharing the SPI bus with other devices like touch panels and SDCard readers. 
You must supply a CS pin in order to share the bus.

The cmd_buts and param_bits paramaters have been removed from all lcd_bus classes. This is now handled 
internally by the display driver.

All touch drivers that are SPI must now be initilized by passing a `machine.SPI` instance. If the touch panel 
is sharing the bus with any other devices then a new `machine.SPI` instance MUST be created with the CS pin given 
for the touch panel. All other pins MUST remain the same across the `machine.SPI` instances. You are allowed to have
different frequencies for each device.

<br>

### *Confirmed working*
_______________________

* Display Bus
  * ESP32 SPI
  * ESP32 RGB
  * ESP32 I8080

* Memory
  * SRAM
  * SRAM DMA
  * PSRAM (SPIRAM)
  * PSRAM (SPIRAM) DMA

* Display IC
  * ST7796
  * ST7789
  * ILI9341
  * SDL
  * RGB
  * ILI9488

* Touch IC
  * XPT2046
  * GT911
  * Mouse
  * FT6x06
  * FT5x06

<br>

## *Build Instructions*
_______________________

I have changed the design of the binding so it is no longer a dependancy of 
MicroPython. Instead MicroPython is now a dependency of the binding. By doing 
this I have simplified the process up updating the MicroPython version. Only 
small changes are now needed to support newer versions of MicroPython.

In order to make this all work I have written a Python script that handles
Building the binding. The only prerequesits are that you have a C compiler 
installed (gcc, clang, msvc) and the necessary support libs.

<br>

### *Requirements*
_________________
compiling for ESP32
  * Ubuntu (Linux): you can install all of these using `apt-get install` 
    * build-essential
    * cmake
    * ninja-build
    * python
    
  * macOS
    * `xcode-select -–install`
    * `brew install cmake`
    * `brew install ninja`
    * `brew install python`


Compiling for RP2
  * Ubuntu (Linux): you can install all of these using `apt-get install` 
    * build-essential
    * cmake
    * ninja-build
    * python
    * gcc-arm-none-eabi 
    * libnewlib-arm-none-eabi
  
  * macOS
    * `command xcode-select–install`
    * `brew install make`
    * `brew install cmake`
    * `brew install ninja`
    * `brew install python`
    * `brew install armmbed/formulae/arm-none-eabi-gcc`

  * Windows
    * Not yet supported


Compiling for STM32:
  * Ubuntu (Linux): you can install all of these using `apt-get install` 
    * gcc-arm-none-eabi 
    * libnewlib-arm-none-eabi: maybe??
    * build-essential
    * ninja-build
    * python
  
  * macOS
    * `command xcode-select–install`
    * `brew install make`
    * `brew install ninja`
    * `brew install python`
    * `brew install armmbed/formulae/arm-none-eabi-gcc`

  * Windows
    * Not yet supported


Compiling for Ubuntu (Linux): you can install all of these using `apt-get install` 

  * build-essential 
  * libffi-dev 
  * pkg-config
  * cmake
  * ninja-build
  * gnome-desktop-testing
  * libasound2-dev
  * libpulse-dev   
  * libaudio-dev
  * libjack-dev
  * libsndio-dev
  * libx11-dev
  * libxext-dev
  * libxrandr-dev
  * libxcursor-dev
  * libxfixes-dev
  * libxi-dev
  * libxss-dev
  * libxkbcommon-dev
  * libdrm-dev
  * libgbm-dev 
  * libgl1-mesa-dev
  * libgles2-mesa-dev
  * libegl1-mesa-dev
  * libdbus-1-dev
  * libibus-1.0-dev
  * libudev-dev
  * fcitx-libs-dev
  * libpipewire-0.3-dev
  * libwayland-dev   
  * libdecor-0-dev


Compiling for macOS 
  * `command xcode-select–install`
  * `brew install libffi` 
  * `brew install ninja`
  * `brew install make`


Compiling for Windows
  * not supported yet

<br>
 
### *Build Target*
__________________

You are also going to need Python >= 3.10 installed for all builds

There is a single entry point for all builds. That is the make.py script in the
root of the repository.

The first argument is positional and it must be one of the following.

  * esp32
  * windows
  * macOS
  * stm32
  * unix
  * rp2 
  * renesas-ra
  * nrf
  * mimxrt
  * samd


<br>

### *Build Options*
________________________

The next few arguments are optional to some degree.

  * submodules\*\*: collects all needed dependencies to perform the build
  * clean: cleans the build environment
  * mpy_cross\*\*: compiles mpy-cross 
               this is not used for all builds. if it is not supported it will do nothing.


**must be run only one time when the build is intially started. after that you will not need 
to add these arguments. There is internal checking that is done to see if the argument needs to 
be carried out. So you can also optionally leave it there if you want. 

<br>

### *Identifying the MCU board*
_______________________________

The next group of options are going to be port specific, some may have them and some may not.

  * BOARD: The MCU to build for. This follows the same symantics as what MIcroPython uses.
  * BOARD_VARIANT: if there is a variation of the board that it to be compiled for.


I will go into specifics for what what boards and variants are available for a specific port a 
little bit further down.

<br>

### *Additional Arguments*
____________________

  * LV_CFLAGS: additional compiler flags that get passed to the LVGL build only.
  * FROZEN_MANIFEST: path to a custom frozen manifest file
  * DISPLAY: this can either be the file name (less the .py) of a display 
             driver that is in the driver/display folder or it can be the absolute
             path to your own custom driver (with the .py extension)
  * INDEV: this can either be the file name (less the .py) of an indev 
           driver that is in the driver/indev folder or it can be the absolute
           path to your own custom driver (with the .py extension)


<br>

### *ESP32 specific options*
____________________________
  * --skip-partition-resize: do not resize the firmware partition
  * --partition-size: set a custom firmware partition size
  * --octal-flash ¹: This is only available for the 16mb flash and the 32mb flash
  * --flash-size ² ³: This is how much flash storage is available.

    Allowed Values are:

    * ESP32-S3: 4, 8, 16 and 32 (default is 8)
    * ESP32-S2: 2 and 4 (default is 4)
    * ESP32: 4, 8 and 16 (default is 4)
    , The default is 8.
  

¹ Available for the ESP32-S3 when `BOARD_VARIANT` is set to `SPIRAM_OCT`<br> 
² Available for the ESP32, ESP32-S2 and ESP32-S3<br>
³ Available only when `BOARD_VARIANT` is set to `SPIRAM` or `SPIRAM_OCT`<br>

<br>

## *Boards & Board Variants*
___________

  * esp32: BOARD=
    * ARDUINO_NANO_ESP32
    * ESP32_GENERIC
      * BOARD_VARIANT=D2WD
      * BOARD_VARIANT=OTA
    * ESP32_GENERIC_C3
    * ESP32_GENERIC_S2
    * ESP32_GENERIC_S3
      * BOARD_VARIANT=SPIRAM_OCT
    * LILYGO_TTGO_LORA32
    * LOLIN_C3_MINI
    * LOLIN_S2_MINI
    * LOLIN_S2_PICO
    * M5STACK_ATOM
    * OLIMEX_ESP32_POE
    * SIL_WESP32
    * UM_FEATHERS2
    * UM_FEATHERS2NEO
    * UM_FEATHERS3
    * UM_NANOS3
    * UM_PROS3
    * UM_TINYPICO
    * UM_TINYS2
    * UM_TINYS3
    * UM_TINYWATCHS3

  * windows: VARIANT=
    * dev
    * stndard
    
  * stm32: BOARD=
    * ADAFRUIT_F405_EXPRESS
    * ARDUINO_GIGA
    * ARDUINO_NICLA_VISION
    * ARDUINO_PORTENTA_H7
    * B_L072Z_LRWAN1
    * B_L475E_IOT01A
    * CERB40
    * ESPRUINO_PICO
    * GARATRONIC_NADHAT_F405
    * GARATRONIC_PYBSTICK26_F411
    * HYDRABUS
    * LEGO_HUB_NO6
    * LEGO_HUB_NO7
    * LIMIFROG
    * MIKROE_CLICKER2_STM32
    * MIKROE_QUAIL
    * NETDUINO_PLUS_2
    * NUCLEO_F091RC
    * NUCLEO_F401RE
    * NUCLEO_F411RE
    * NUCLEO_F412ZG
    * NUCLEO_F413ZH
    * NUCLEO_F429ZI
    * NUCLEO_F439ZI
    * NUCLEO_F446RE
    * NUCLEO_F722ZE
    * NUCLEO_F746ZG
    * NUCLEO_F756ZG
    * NUCLEO_F767ZI
    * NUCLEO_G0B1RE
    * NUCLEO_G474RE
    * NUCLEO_H563ZI
    * NUCLEO_H723ZG
    * NUCLEO_H743ZI
    * NUCLEO_H743ZI2
    * NUCLEO_L073RZ
    * NUCLEO_L152RE
    * NUCLEO_L432KC
    * NUCLEO_L452RE
    * NUCLEO_L476RG
    * NUCLEO_L4A6ZG
    * NUCLEO_WB55
    * NUCLEO_WL55
    * OLIMEX_E407
    * OLIMEX_H407
    * PYBD_SF2
    * PYBD_SF3
    * PYBD_SF6
    * PYBLITEV10
    * PYBV10
    * PYBV11
    * PYBV3
    * PYBV4
    * SPARKFUN_MICROMOD_STM32
    * STM32F411DISC
    * STM32F429DISC
    * STM32F439
    * STM32F4DISC
    * STM32F769DISC
    * STM32F7DISC
    * STM32H573I_DK
    * STM32H7B3I_DK
    * STM32L476DISC
    * STM32L496GDISC
    * USBDONGLE_WB55
    * VCC_GND_F407VE
    * VCC_GND_F407ZG
    * VCC_GND_H743VI

  * unix: VARIANT=
    * coverage
    * minimal
    * nanbox
    * standard
  
  * rp2: BOARD=
    * ADAFRUIT_FEATHER_RP2040
    * ADAFRUIT_ITSYBITSY_RP2040
    * ADAFRUIT_QTPY_RP2040
    * ARDUINO_NANO_RP2040_CONNECT
    * GARATRONIC_PYBSTICK26_RP2040
    * NULLBITS_BIT_C_PRO
    * PIMORONI_PICOLIPO_16MB
    * PIMORONI_PICOLIPO_4MB
    * PIMORONI_TINY2040
    * POLOLU_3PI_2040_ROBOT
    * POLOLU_ZUMO_2040_ROBOT
    * RPI_PICO
    * RPI_PICO_W
    * SIL_RP2040_SHIM
    * SPARKFUN_PROMICRO
    * SPARKFUN_THINGPLUS
    * W5100S_EVB_PICO
    * W5500_EVB_PICO
    * WEACTSTUDIO
      * BOARD_VARIANT=FLASH_2M
      * BOARD_VARIANT=FLASH_4M
      * BOARD_VARIANT=FLASH_8M
      
    * renesas-ra: BOARD=
      * ARDUINO_PORTENTA_C33
      * EK_RA4M1
      * EK_RA4W1
      * EK_RA6M1
      * EK_RA6M2
      * RA4M1_CLICKER
      * VK_RA6M5
      
    * nrf: BOARD=
      * ACTINIUS_ICARUS
      * ARDUINO_NANO_33_BLE_SENSE
      * ARDUINO_PRIMO
      * BLUEIO_TAG_EVIM
      * DVK_BL652
      * EVK_NINA_B1
      * EVK_NINA_B3
      * FEATHER52
      * IBK_BLYST_NANO
      * IDK_BLYST_NANO
      * MICROBIT
      * NRF52840_MDK_USB_DONGLE
      * PARTICLE_XENON
      * PCA10000
      * PCA10001
      * PCA10028
      * PCA10031
      * PCA10040
      * PCA10056
      * PCA10059
      * PCA10090
      * SEEED_XIAO_NRF52
      * WT51822_S4AT 

    * mimxrt: BOARD=
      * ADAFRUIT_METRO_M7
      * MIMXRT1010_EVK
      * MIMXRT1015_EVK
      * MIMXRT1020_EVK
      * MIMXRT1050_EVK
      * MIMXRT1060_EVK
      * MIMXRT1064_EVK
      * MIMXRT1170_EVK
      * OLIMEX_RT1010
      * SEEED_ARCH_MIX
      * TEENSY40
      * TEENSY41

    * samd: BOARD=
      * ADAFRUIT_FEATHER_M0_EXPRESS
      * ADAFRUIT_FEATHER_M4_EXPRESS
      * ADAFRUIT_ITSYBITSY_M0_EXPRESS
      * ADAFRUIT_ITSYBITSY_M4_EXPRESS
      * ADAFRUIT_METRO_M4_EXPRESS
      * ADAFRUIT_TRINKET_M0
      * MINISAM_M4
      * SAMD21_XPLAINED_PRO
      * SEEED_WIO_TERMINAL
      * SEEED_XIAO_SAMD21
      * SPARKFUN_SAMD51_THING_PLUS
      

<br>

## *Build Command Examples*
___________________________

build with submodules and mpy_cross

    python3 make.py esp32 submodules clean mpy_cross BOARD=ESP32_GENERIC_S3 BOARD_VARIANT=SPIRAM_OCT DISPLAY=st7796 INDEV=gt911

build without submodules or mpy_cross

    python3 make.py esp32 clean BOARD=ESP32_GENERIC_S3 BOARD_VARIANT=SPIRAM_OCT DISPLAY=st7796 INDEV=gt911


I always recommend building with the clean command, this will ensure you get a good fresh build.

NOTE:
There is a bug in the ESP32 build. The first time around it will fail saying that 
one of the sumbodules is not available. Run the build again with the submodules 
argument in there and then it will build fine. For the life of me I cam not able to locate
where the issue is stemming from. I will find it eventually.

<br>

I will provide directions on how to use the driver framework and also the drivers that are included
with the binding in the coming weeks.

<br>


SDL fpr Unix is working properly. Make sure you review the requirements needed to compile for unix!!!
The build system compiles the latest version of SDL2 so the list is pretty long for the requirements.

To build for Unix use the following build command

    python3 make.py unix clean DISPLAY=sdl_display INDEV=sdl_pointer


Couple of notes:

  * I recommend making 2 frame buffers as seen in the code example below. This will give you 
    better performance. 
  * **DO NOT** enable LV_USE_DRAW_SDL, I have not written code to allow for it's use (yet).
  * I recommend running `lv.task_handler` once every 5 milliseconds, shorter than that and you 
    will have a lot of CPU time comsumed. Linger than that and your mouse response is not 
    going to be great.



Here is some example code for the unix port


    from micropython import const  # NOQA

    _WIDTH = const(480)
    _HEIGHT = const(320)
    
    _BUFFER_SIZE = _WIDTH * _HEIGHT * 3
    
    import lcd_bus  # NOQA
    
    bus = lcd_bus.SDLBus(flags=0)
    
    buf1 = bus.allocate_framebuffer(_BUFFER_SIZE, 0)
    buf2 = bus.allocate_framebuffer(_BUFFER_SIZE, 0)
    
    import lvgl as lv  # NOQA
    import sdl_display  # NOQA
    
    lv.init()
    
    display = sdl_display.SDLDisplay(
        data_bus=bus,
        display_width=_WIDTH,
        display_height=_HEIGHT,
        frame_buffer1=buf1,
        frame_buffer2=buf2,
        color_space=lv.COLOR_FORMAT.RGB888
    )
    
    display.init()
    
    import sdl_pointer
    
    mouse = sdl_pointer.SDLPointer()
    
    scrn = lv.screen_active()
    scrn.set_style_bg_color(lv.color_hex(0x000000), 0)
    
    slider = lv.slider(scrn)
    slider.set_size(300, 25)
    slider.center()
    
    
    import task_handler
    # the duration needs to be set to 5 to have a good response from the mouse.
    # There is a thread that runs that facilitates double buffering. 
    th = task_handler.TaskHandler(duration=5)



The touch screen drivers will handle the rotation that you set to the display.
There is a single caviat to this. You MUST set up and initilize the display then 
create the touch drivers and after that has been done you can set the rotation.
The touch driver must exist prior to the display rotation being set.

For the ESP32 SOC's there is NVRAM that is available to store data in. That 
data is persistant between restarts of the ESP32. This feature is pur to use to 
store calibration data for the touch screen. In the exmaple below it shows how 
to properly create a display driver and touch driver and how to set the rotation 
and also the calibration storage.


    import lcd_bus
    
    from micropython import const
    
    # display settings
    _WIDTH = const(320)
    _HEIGHT = const(480)
    _BL = const(45)
    _RST = const(4)
    _DC = const(0)
    _WR = const(47)
    _FREQ = const(20000000)
    _DATA0 = const(9)
    _DATA1 = const(46)
    _DATA2 = const(3)
    _DATA3 = const(8)
    _DATA4 = const(18)
    _DATA5 = const(17)
    _DATA6 = const(16)
    _DATA7 = const(15)
    _BUFFER_SIZE = const(30720)
    
    _SCL = const(5)
    _SDA = const(6)
    _TP_FREQ = const(100000)
    
    
    display_bus = lcd_bus.I80Bus(
        dc=_DC,
        wr=_WR,
        freq=_FREQ,
        data0=_DATA0,
        data1=_DATA1,
        data2=_DATA2,
        data3=_DATA3,
        data4=_DATA4,
        data5=_DATA5,
        data6=_DATA6,
        data7=_DATA7
    )
    
    fb1 = display_bus.allocate_framebuffer(_BUFFER_SIZE, lcd_bus.MEMORY_INTERNAL | lcd_bus.MEMORY_DMA)
    fb2 = display_bus.allocate_framebuffer(_BUFFER_SIZE, lcd_bus.MEMORY_INTERNAL | lcd_bus.MEMORY_DMA)
    
    
    import st7796  # NOQA
    import lvgl as lv  # NOQA
    
    lv.init()
    
    display = st7796.ST7796(
        data_bus=display_bus,
        frame_buffer1=fb1,
        frame_buffer2=fb2,
        display_width=_WIDTH,
        display_height=_HEIGHT,
        backlight_pin=_BL,
        # reset=_RST,
        # reset_state=st7796.STATE_LOW,
        color_space=lv.COLOR_FORMAT.RGB565,
        color_byte_order=st7796.BYTE_ORDER_BGR,
        rgb565_byte_swap=True,
    )
    
    import i2c  # NOQA
    import task_handler  # NOQA
    import ft6x36  # NOQA
    import time  # NOQA

    display.init()

    i2c_bus = i2c.I2CBus(scl=_SCL, sda=_SDA, freq=_TP_FREQ, use_locks=False)
    indev = ft6x36.FT6x36(i2c_bus)

    display.invert_colors()

    if not indev.is_calibrated:
        display.set_backlight(100)
        indev.calibrate()

    # you want to rotate the display after the calibration has been done in order
    # to keep the corners oriented properly.
    display.set_rotation(lv.DISPLAY_ROTATION._90)
    
    display.set_backlight(100)
    
    th = task_handler.TaskHandler()

    scrn = lv.screen_active()
    scrn.set_style_bg_color(lv.color_hex(0x000000), 0)
    
    slider = lv.slider(scrn)
    slider.set_size(300, 50)
    slider.center()
    
    label = lv.label(scrn)
    label.set_text('HELLO WORLD!')
    label.align(lv.ALIGN.CENTER, 0, -50)


You are able to force the calibration at any time by calling `indev.calibrate()` 
regardless of what `indev.is_calibrate` returns. This makes it possible to redo 
the calibration by either using a pin that you can check the state of or through
a button in your UI that you provide to the user.

Thank again and enjoy!!

***NOTE***: On ESP32-S3, SPI host 0 and SPI host 1 share a common SPI bus. 
The main Flash and PSRAM are connected to the host 0. It is recommended to use 
SPI host 2 when connecting an SPI device like a display that is going to utilize
the PSRAM for the frame buffer.

