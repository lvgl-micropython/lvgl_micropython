# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const

_WIDTH = const(480)
_HEIGHT = const(320)

import lcd_bus  # NOQA
# this next section is the setup for the SPIBus.
# How this works is like this.
# I recommend using the hardware pins for SPI. This is going to
# get you the best speeds possible. This is due to the design of the MCU.

# this is a required setting. You MUST supply a value here.
_DC_PIN = const(11)

# if you leave the `host` at its default or set it to -1 then this must be
# supplied. This is the pin that sends data to the display.
_MOSI_PIN = const(13)

# There is an option that can be used to save making a pin connection.
# in most cses the display is only going to have to receive data to it.
# So the SPI bus only has to transmit. If you set the `tx_only` parameter to
# True this pin will be changed to -1. If you have the `host` left at its
# default or it is set to -1 that means you want the host assignment picked
# automatically. If that is the case then you will need to set this to the
# pin that it is assigned to for the host you are going to want to use.
# If the host is set to -1 or it is left at its default even if even if you have
# `tx_only` to True you will need to set this. Don't worry you will not have
# to make a connection from the display to this pin if you are using `tx_only`.
_MISO_PIN = const(12)

# if you leave the `host` at its default or set it to -1 then this must be
# supplied.
_SCLK_PIN = const(14)

# This is the pin that selects the display so it can receive data. This pin
# is optional and can be set to -1. If this pin is set to -1 the display MUST
# be the only device attached to the bus other than the MCU. The CS pin on the
# display side will need to be pulled either high or low depending on the
# specification for the display
_CS_PIN = const(15)

_FREQ = const(80000000)

# these pins get set Automatically if you are using hardware SPI pins and you
# have qud_spi set to True. You can set them manually if you are using
# something other than the hardware pins.
_WP_PIN = const(-1)
_HD_PIN = const(-1)


display_bus = lcd_bus.SPIBus(
    dc=_DC_PIN,
    host=1,
    sck=_SCLK_PIN,
    freq=_FREQ,
    mosi=_MOSI_PIN,
    miso=_MISO_PIN,
    cs=_CS_PIN,
    wp=_WP_PIN,
    hd=_HD_PIN,
    quad_spi=False,
    tx_only=False,
    cmd_bits=8,
    param_bits=8,
    dc_low_on_data=False,
    sio_mode=False,
    lsb_first=False,
    cs_high_active=False,
    spi_mode=0
)


import st7796  # NOQA

# reset pin, this pin is used to reset the display.
# If there is no reset pin for your display set this to -1.
_RESET_PIN = const(43)

# power pin, this pin is used to power on the display.
# If there is no power pin for your display set this to -1.
_POWER_PIN = const(44)

# backlight pin, this pin is used to turn on and off the
# backlight to the display.
# If there is no backlight pin for your display set this to -1.
_BACKLIGHT_PIN = const(45)


# some displays have a bezel that covers a small portion of the viewable area
# of the display. This is to offset the display data so it is not covered by
# the bezel. Keep in mind that you will have to adjust the width and height in
# order to compensate for the right and bottom edges of the display.
_OFFSET_X = const(0)
_OFFSET_Y = const(0)


import lvgl as lv  # NOQA


display = st7796.ST7796(
    data_bus=display_bus,
    display_width=_WIDTH,
    display_height=_HEIGHT,
    # we are going to let the driver handle the allocation of the frame buffers
    frame_buffer1=None,
    frame_buffer2=None,
    reset_pin=_RESET_PIN,
    reset_state=st7796.STATE_HIGH,
    power_pin=_POWER_PIN,
    power_on_state=st7796.STATE_HIGH,
    backlight_pin=_BACKLIGHT_PIN,
    backlight_on_state=st7796.STATE_HIGH,
    offset_x=_OFFSET_X,
    offset_y=_OFFSET_Y,
    color_space=lv.COLOR_FORMAT.RGB565,
    rgb565_byte_swap=True
)

# Remember you are going to need to power the display
# on if you have a power pin connected. If you set this to True and you have
# the power pin set to -1 do not worry it will simply return doing nothing.
# it is ideal to have this in place no matter what so if you change the display
# to one that does have the power pin the only thing you are going
# to need to change is the pin number
display.set_power(True)

# The old drivers used to inot the display automatically. I have decided against
# doing that to save memory. So the init must be done at some point before
# you create any LVGL objects.
display.init()

display.set_backlight(100)


# you can use the built in handling for calling the task_handler and updating
# the time in LVGL. I personally don't like using this because of it using an
# ISR which uses up CPU time. I also don't like it because it has to schedule
# a task using the scheduler so the display update is able t take pplace
# outside of an ISR. The issue with the scheduler is it will run when it is
# able to. so there is a big unknown as to when it will actually run.

# import task_handler  # NOQA

# task_handler.TaskHandler()

# This is my prefered method of updating the display. This has time keeping
# that is accurate to the nanosecond.

import time
time_passed = 1000000

while True:
    start_time = time.time_ns()
    time.sleep_ns(1000000)
    lv.tick_inc(int(time_passed // 1000000))
    time_passed -= int(time_passed // 1000000) * 1000000
    lv.task_handler()
    end_time = time.time_ns()
    time_passed += time.ticks_diff(end_time, start_time)
