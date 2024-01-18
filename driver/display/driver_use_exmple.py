import heap_caps
from micropython import const

_WIDTH = const(480)
_HEIGHT = const(320)
_BYTES_PER_PIXEL = const(2)  # RGB565
_BUFFER_SIZE = const(int(_WIDTH * _HEIGHT * _BYTES_PER_PIXEL / 10))


def memory_allocation_failure(user_data, function_name, caps, size):

    print('Frme buffer allocation failed:', user_data, function_name, caps, size)


# we want to allocate the frame buffers as soon as the application starts.
# this is really important to do if wanting to secure space
# in the internal memory. It is even more important to do if you are
# using an ESP32 and want to use DMA memory and also internal memory.
# There is only a really small amount of DMA memory available.

# allocate the buffer in DMA section of internal memory
# DMA memory will allow for the best performance using 2 buffers
# and the internal SRAM is a lot faster than the external SPIRAM

# you can also set an optional callback if the allocation fails.
heap_caps.register_failed_alloc_callback(memory_allocation_failure, 'frame buffer 1')
frame_buf1 = heap_caps.malloc(_BUFFER_SIZE, heap_caps.CAP_DMA | heap_caps.CAP_INTERNAL)

heap_caps.register_failed_alloc_callback(memory_allocation_failure, 'frame buffer 2')
frame_buf2 = heap_caps.malloc(_BUFFER_SIZE, heap_caps.CAP_DMA | heap_caps.CAP_INTERNAL)


import lcd_bus  # NOQA
# this next section is the setup for the SPIBus.
# How this works is like this.
# I recommend using the hardware pins for SPI. This is going to
# get you the best speeds possible. This is due to the design of the MCU.

# this is a required setting. You MUST supply a value here.
_DC_PIN = const(11)

# This setting sets the size of the transfer that can be done.
# If you are NOT using DMA memory for the buffers then you do not set this
# at all or set this to `lcd_bus.SPIBus.MAXIMUM_BUFFER_SIZE`
_MAX_TRANSFER_SZ = const(_BUFFER_SIZE)

# the default for the host is -1. if this is set to -1 the host will be picked
# based on the pin numbers supplied. If the spi pin numbers are all set to -1
# then the pin numbers that are assigned to the host at the hardware level will
# be used. Setting the host and making the connections to the hardware pins
# is the easiest thing to do.
_SPI_HOST = const(lcd_bus.SPIBus.HOST2)

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

# set this to True if you are using quad_spi
_QUAD_SPI = False

# Set this to True if you do not have the miso line connected.
_TX_ONLY = False

# this is the default value and only needs to be changed if the datasheet
# for the display says to
_CMD_BITS = const(8)

# this is the default value and only needs to be changed if the datasheet
# for the display says to
_PARAM_BITS = const(8)

# this is the default value and only needs to be changed if the datasheet
# for the display says to
_DC_LOW_ON_DATA = False

# this is the default value and only needs to be changed if the datasheet
# for the display says to
_SIO_MODE = const()

# this is the default value and only needs to be changed if the datasheet
# for the display says to
_LSB_FIRST = False

# this is the default value and only needs to be changed if the datasheet
# for the display says to
_CS_HIGH_ACTIVE = False

# this is the default value and only needs to be changed if the datasheet
# for the display says to
_SPI_MODE = const(0)


display_bus = lcd_bus.SPIBus(
    dc=_DC_PIN,
    max_transfer_sz=_MAX_TRANSFER_SZ,
    host=_SPI_HOST,
    mosi=_MOSI_PIN,
    miso=_MISO_PIN,
    sclk=_SCLK_PIN,
    cs=_CS_PIN,
    freq=_FREQ,
    wp=_WP_PIN,
    hd=_HD_PIN,
    quad_spi=_QUAD_SPI,
    tx_only=_TX_ONLY,
    cmd_bits=_CMD_BITS,
    param_bits=_PARAM_BITS,
    dc_low_on_data=_DC_LOW_ON_DATA,
    sio_mode=_SIO_MODE,
    lsb_first=_LSB_FIRST,
    cs_high_active=_CS_HIGH_ACTIVE,
    spi_mode=_SPI_MODE
)


import st7796  # NOQA

# reset pin, this pin is used to reset the display.
# If there is no reset pin for your display set this to -1.
_RESET_PIN = const(43)

# this is the direction the pin needs to be set to in order for the display
# to perform the reset. STATE_HIGH is the default setting
# valid choices are STATE_HIGH and STATE_LOW
_RESET_STATE = const(st7796.STATE_HIGH)


# power pin, this pin is used to power on the display.
# If there is no power pin for your display set this to -1.

_POWER_PIN = const(44)

# this is the direction the pin needs to be set to in order for the display
# to power on. STATE_HIGH is the default setting
# valid choices are STATE_HIGH and STATE_LOW
_POWER_ON_STATE = const(st7796.STATE_HIGH)

# backlight pin, this pin is used to turn on and off the
# backlight to the display.
# If there is no backlight pin for your display set this to -1.
_BACKLIGHT_PIN = const(45)

# this is the direction the pin needs to be set to in order for the
# backlight to turn on and off. STATE_HIGH is the default setting
# valid choices are STATE_HIGH, STATE_LOW and STATE_PWM.
# STATE_PWM can only be used if there is a transistor in the circuit that is
# used to supply backlight power. Check your datacheet for the display prior
# to using STATE_PWM.
_BACKLIGHT_ON_STATE = const(st7796.STATE_PWM)

# some displays have a bezel that covers a small portion of the viewable area
# of the display. This is to offset the display data so it is not covered by
# the bezel. Keep in mind that you will have to adjust the width and height in
# order to compensate for the right and bottom edges of the display.
_OFFSET_X = const(0)
_OFFSET_Y = const(0)

display = st7796.ST7796(
    data_bus=display_bus,
    display_width=_WIDTH,
    display_height=_HEIGHT,
    frame_buffer1=frame_buf1,
    frame_buffer2=frame_buf2,
    reset_pin=_RESET_PIN,
    reset_state=_RESET_STATE,
    power_pin=_POWER_PIN,
    power_on_state=_POWER_ON_STATE,
    backlight_pin=_BACKLIGHT_PIN,
    backlight_on_state=_BACKLIGHT_ON_STATE,
    offset_x=_OFFSET_X,
    offset_y=_OFFSET_Y
)

# Remember you are going to need to power the display
# on if you have a power pin connected. If you set this to True and you have
# the power pin set to -1 do not worry it will simply return doing nothing.
# it is ideal to have this in place no matter what so if you change the display
# to one that does have the power pin the only thing you are going
# to need to change is the pin number
display.power = True

# The old drivers used to inot the display automatically. I have decided against
# doing that to save memory. So the init must be done at some point before
# you create any LVGL objects.

display.init()

# you can set the orientation any time after the display can be constructed.
# It doesn't matter if it is before calling init or after.
display.orientation = st7796.LANDSCAPE

import lvgl as lv  # NOQA

# place your code here that is going to create your UI


# Now turn the backlight on. If you do not have PWM you can use 0 for off or
# False for off and any number that is not 0 for on or True for on.
# if you do have PWM set then 100 is full brightness and 0 if off.
# I recommend using the 0-100 scale no matter what state is set to.
# you will see why I have the setting of the backlight in a function call.
# NOTE: Fractional numbers are allowed when setting the backlight.
def set_backlight(value):

    import time

    curr_value = display.backlight
    # backlight pin not supplied

    if curr_value == -1:
        return

    if value > curr_value:
        increment = 1
    else:
        increment = -1

    while curr_value != value:
        curr_value += increment
        display.backlight = value
        time.sleep_ms(1)

        # this is here in case there is anything that needs to be
        # updated in the UI. Maybe an opening animation.
        update_lvgl()


# I removed the use of having tick_inc and task_handler called.
# I found this to be something that causes a lot of headache and it also
# degrades the performance due to it using an ISR and scheduling a callback
# to have task_handler called. It caused the display update to be slow.
# because that has been removed you will now need to provide your own loop
# I recommend using the loop code below and adding any thing that needs
# to be done to it.

# this is how I go about doing a nice ramp up with the backlight (if supported)

import time  # NOQA

# time keeping variables
start_time = time.ticks_us()
left_over = 0


# I set the updating into a function so I would not have duplicated code for
# when the backlight is ramping up and when the loop is running
def update_lvgl():
    global start_time
    global left_over

    # I am using microsecond resolution when updating but due to LVGL only
    # supporting millisecond timing I have to check and see if enough
    # microseconds have passed before calling the task handler.
    # the update happens every 1024 microseconds which is 24 microseconds
    # past 1 millisecond. I am doing this so I m able to use simple bit shifting
    # and avoid having to do any floating point math.

    curr_time = time.ticks_us()
    new_amount = time.ticks_diff(start_time, curr_time) + left_over
    if new_amount < 0:
        return

    new_amount = abs(new_amount)
    left_over = new_amount & 0x400
    # 1024 in binary is 0100 0000 0000
    new_amount >>= 10
    new_amount = abs(new_amount)
    if new_amount:
        # adjustment to the left over for the 24 microseconds.
        # we want to keep the time keeping in sync so 24 extra microseconds for
        # each millisecond that has passed needs to be adjusted for
        left_over -= 24 * new_amount
        lv.tick_inc(new_amount)
        lv.task_handler()

        start_time = curr_time


set_backlight(100)

while True:
    update_lvgl()
    # any code that needs to be run inside the loop gets done here
