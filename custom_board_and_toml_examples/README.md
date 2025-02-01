

### *Custom Boards*
___________________


I added the ability to provide a path to a custom board. There are a few requirememnts for 
this to work properly. The best bet is to look at the files in the `custom_board_and_toml_examples/MY_CUSTOM_BOARD` 
folder to see what files are needed and to also get an idea of the kind of information that 
is in the files. 

***NOTE***: There are only 2 options that get used when supplying a custom board. The first one 
            is the build target and the second is the `--custom-board-path` command. ALL others 
            are ignored. The reason they are ignored is because you have the ability to set things
            in the files for the custom board.


The path needs to point to the folder that holds the board specification files. Here is a list of required files.

* `board.json`: This file outlines what the board is. At a minimum the file needs to contain the following.
                ```
                {
                    "mcu": "{MCU}"
                }
                ```
                where `{MCU}` is one of the follwing:

  * esp32
  * esp32s2
  * esp32s3
  * esp32c3
  * esp32c6

* `sdkconfig.board`: This file contains all of the ESP-IDF specific config settings. If you don't know 
                     what needs to be set in here then please ask me for assistance.
* `mpconfigboard.h`: MicroPython config settings. If you don't know what needs to be set in here then
                     please ask me for assistance.
* `mpconfigboard.cmake`: Build script. At a minimum the following should be in the build script.
                         `{MCU}` is replaced with one of the options from the list of MCU's above.
                         `{BOARD_CONATINING_FOLDER}` if the name of the folder these files are located in.
```
set(IDF_TARGET {MCU})

set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    ${SDKCONFIG_IDF_VERSION_SPECIFIC}
    boards/{BOARD_CONATINING_FOLDER}/sdkconfig.board
)
```

* `partition.csv`: This file dictates what the partitions are supposed to be on the ESP32. As for assistance
                   If you do not know how to create one of these.

***NOTE***: The `.toml` file in the custom board example is NOT a requirement. I do strongly suggest using it
            since it will tie everything together. You can specify all of the display and indev bits and pieces.
            see [TOML Example](#toml-example) for further information.


### *TOML Example*
__________________

Here is an example of what you would put inside of the `.toml` file. 
I will go over each section below.

    [MCU.esp32]
    BOARD = "ESP32_GENERIC_S3"
    BOARD_VARIANT = "SPIRAM_OCT"
    octal_flash = true
    flash_size = 16
    enable_jtag_repl = 'n'
    enable_cdc_repl = 'n'
    enable_uart_repl = 'y'
    uart_repl_bitrate = 115200

    [I80Bus.display_bus]
    data0 = 9
    data1 = 46
    data2 = 3
    data3 = 8
    data4 = 18
    data5 = 17
    data6 = 16
    data7 = 15
    dc = 0
    wr = 47
    cs = -1
    freq = 20000000

    [I2C.Bus.i2c_bus]
    host = 0
    scl = 5
    sda = 6
    freq = 100000

    [I2C.Device.indev_device]
    bus = "i2c_bus"
    dev_id = "ft6x36.I2C_ADDR"
    reg_bits = "ft6x36.BITS"

    [ST7796.display]
    data_bus = "display_bus"
    display_width = 320
    display_height = 480
    backlight_pin = 45
    color_byte_order = "st7789.BYTE_ORDER_BGR"
    color_space = "lv.COLOR_FORMAT.RGB565"
    rgb565_byte_swap = true
    
    [ST7796.display.init]
    params = []
    
    [FT6x36.indev]
    device = "indev_device"
    
    [display.set_color_inversion]
    params = [true]
    
    [display.set_rotation]
    params = ["lv.DISPLAY_ROTATION._90"]
    
    [display.set_backlight]
    params = [100]
    
    [task_handler.TaskHandler]
    params=[]



* `[MCU.{target}]`: `{target}` is the build target you want to use. In the example above we are using `esp32`
                    The parameters that immediatly follow are almost the same as what you would use for build commands
                    when entering them from the command line. There are a few rules for how those commands get enetered.

  * options that star with `--` need to have the `--` removed and all `-`'s in the name need to be change to `_`.
  * options are cas esensitive
  * options that take a string value need to be wrapped in double quotes (`"value"`)
  * options that do not take any value *MUST* have the value set to `true`
  * options like `DISPLAY` and `INDEV` which are able to be repeated cannot be repeated (yet). That means you can onmly 
    supply a single display or indev. You only need to supply the `DISPLAY` and `INDEV` if you are not wanting to automatically
    build the startup script.


    [MCU.esp32]
    BOARD = "ESP32_GENERIC_S3"
    BOARD_VARIANT = "SPIRAM_OCT"
    octal_flash = true
    flash_size = 16
    enable_jtag_repl = 'n'
    enable_cdc_repl = 'n'
    enable_uart_repl = 'y'
    uart_repl_bitrate = 115200

* `[{display bus}.{variable name}]`: `[I80Bus.display_bus]`, That heading section gets turnmed into `display_bus = lcd_bus.I80Bus(...)`.
                                     The variable name you will use when passing the display bus instance to the display driver. I will 
                                     go into that more later on. The list of options below this heading are the parameters that get passed.
                                     These options MUST match the names of the parameters for the class being used.
* `[I2C.Bus.{variable name}]` and `[I2C.Device.{variable_name}]`