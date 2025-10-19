import time

import lcd_bus
import machine
from micropython import const


# display settings
_WIDTH = const(170)
_HEIGHT = const(320)
_BL = const(15)
_RST = const(14)
_DC = const(6)

_MOSI = const(4)
_SCK = const(5)
_HOST = const(1)  # SPI2

_LCD_CS = const(7)
_LCD_FREQ = const(80_000_000)

# touch settings
_TOUCH_CS = const(18)
_TOUCH_FREQ = const(500_000)
_TOUCH_SDA = const(18)
_TOUCH_SCL = const(8)
_TOUCH_RST = const(21)

spi_bus = machine.SPI.Bus(
    host=_HOST,
    mosi=_MOSI,
    sck=_SCK
)

display_bus = lcd_bus.SPIBus(
    spi_bus=spi_bus,
    freq=_LCD_FREQ,
    dc=_DC,
    cs=_LCD_CS,
)

import st7789  # NOQA
import lvgl as lv  # NOQA

lv.init()

display = st7789.ST7789(
    data_bus=display_bus,
    display_width=_WIDTH,
    display_height=_HEIGHT,
    backlight_pin=_BL,
    reset_pin=_RST,
    reset_state=st7789.STATE_LOW,
    backlight_on_state=st7789.STATE_PWM,
    color_space=lv.COLOR_FORMAT.RGB565,
    color_byte_order=st7789.BYTE_ORDER_RGB,
    rgb565_byte_swap=True,
    offset_x=35
)

display.set_power(True)
display.init()
display.set_backlight(10)

scrn = lv.screen_active()
scrn.set_style_bg_color(lv.color_hex(0x000000), 0)

import i2c # NOQA
import cst816s  # NOQA
import task_handler  # NOQA

i2c_bus = i2c.I2C.Bus(host=0,
                      scl=_TOUCH_SCL,
                      sda=_TOUCH_SDA,
                      freq=_TOUCH_FREQ,
                      use_locks=False)

touch_dev = i2c.I2C.Device(bus=i2c_bus,
                           dev_id=cst816s.I2C_ADDR,
                           reg_bits=cst816s.BITS)


indev = cst816s.CST816S(touch_dev, reset_pin=_TOUCH_RST)

eventnum = 0
numpressed = 0

labelhw = lv.label(scrn)
labelhw.set_text('HELLO WORLD!')
labelhw.align(lv.ALIGN.CENTER, 0, -100)

btn1 = lv.button(scrn)
btn1.set_size(90, 30)

labelbtn1 = lv.label(btn1)
labelbtn1.set_text('----')

btn1.align_to(labelhw, lv.ALIGN.OUT_BOTTOM_MID, 0, 10)

btn2 = lv.button(scrn)
btn2.set_size(90, 30)

labelbtn2 = lv.label(btn2)
labelbtn2.set_text('press me')
btn2.align_to(btn1, lv.ALIGN.OUT_BOTTOM_MID, 0, 10)

def btn2event(event):
    global numpressed
    numpressed += 1
    labelbtn2.set_text(f'pressed {numpressed}')

btn2.add_event_cb(btn2event, lv.EVENT.PRESSED, None)

def time_cb(t):
    global eventnum
    eventnum += 1
    labelbtn1.set_text(f'count {eventnum}')

timer = lv.timer_create(time_cb,
                        1000,
                        None)
timer.set_repeat_count(-1)


while True:
    lv.tick_inc(5)
    lv.timer_handler_run_in_period(5)
    time.sleep_ms(5)  # Delay for a short perio
