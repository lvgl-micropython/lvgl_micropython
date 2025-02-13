from micropython import const  # NOQA
import machine

from i2c import I2C


I2C_ADDR = const(0x55)
I2C_FREQ = const(100000)

KEYBOARD_BL_PIN = const(9)
I2C_SDA = const(2)
I2C_SCL = const(10)

KB_BRIGHTNESS_FREQ = const(1000)
KB_BRIGHTNESS_RES = const(16)  # Resolution_bits
KB_BRIGHTNESS_BOOT_DUTY = const(0)
KB_BRIGHTNESS_DEFAULT_DUTY = const(32767)  # Alt+B default duty , is duty is zero , use setting duty

LILYGO_KB_BRIGHTNESS_CMD = const(0x01)
LILYGO_KB_ALT_B_BRIGHTNESS_CMD = const(0x02)

rows = [0, 3, 19, 12, 18, 6, 7]
rowCount = len(rows)

cols = [1, 4, 5, 11, 13]
colCount = len(cols)

keys = [([False] * rowCount)[:]] * colCount

lastValue = keys[:]
changedValue = keys[:]
keyboard = [([''] * rowCount)[:]] * colCount
keyboard_symbol = keyboard[:]


keyboard[0][0] = 'q'
keyboard[0][1] = 'w'
keyboard[0][2] = None  # symbol
keyboard[0][3] = 'a'
keyboard[0][4] = None  # ALT
keyboard[0][5] = ' '
keyboard[0][6] = None  # Mic

keyboard[1][0] = 'e'
keyboard[1][1] = 's'
keyboard[1][2] = 'd'
keyboard[1][3] = 'p'
keyboard[1][4] = 'x'
keyboard[1][5] = 'z'
keyboard[1][6] = None  # Left Shift

keyboard[2][0] = 'r'
keyboard[2][1] = 'g'
keyboard[2][2] = 't'
keyboard[2][3] = None  # Right Shift
keyboard[2][4] = 'v'
keyboard[2][5] = 'c'
keyboard[2][6] = 'f'

keyboard[3][0] = 'u'
keyboard[3][1] = 'h'
keyboard[3][2] = 'y'
keyboard[3][3] = None  # Enter
keyboard[3][4] = 'b'
keyboard[3][5] = 'n'
keyboard[3][6] = 'j'

keyboard[4][0] = 'o'
keyboard[4][1] = 'l'
keyboard[4][2] = 'i'
keyboard[4][3] = None  # Backspace
keyboard[4][4] = '$'
keyboard[4][5] = 'm'
keyboard[4][6] = 'k'

keyboard_symbol[0][0] = '#'
keyboard_symbol[0][1] = '1'
keyboard_symbol[0][2] = None
keyboard_symbol[0][3] = '*'
keyboard_symbol[0][4] = None
keyboard_symbol[0][5] = None
keyboard_symbol[0][6] = '0'

keyboard_symbol[1][0] = '2'
keyboard_symbol[1][1] = '4'
keyboard_symbol[1][2] = '5'
keyboard_symbol[1][3] = '@'
keyboard_symbol[1][4] = '8'
keyboard_symbol[1][5] = '7'
keyboard_symbol[1][6] = None

keyboard_symbol[2][0] = '3'
keyboard_symbol[2][1] = '/'
keyboard_symbol[2][2] = '('
keyboard_symbol[2][3] = None
keyboard_symbol[2][4] = '?'
keyboard_symbol[2][5] = '9'
keyboard_symbol[2][6] = '6'

keyboard_symbol[3][0] = '_'
keyboard_symbol[3][1] = ':'
keyboard_symbol[3][2] = ')'
keyboard_symbol[3][3] = None
keyboard_symbol[3][4] = '!'
keyboard_symbol[3][5] = ','
keyboard_symbol[3][6] = ';'

keyboard_symbol[4][0] = '+'
keyboard_symbol[4][1] = '"'
keyboard_symbol[4][2] = '-'
keyboard_symbol[4][3] = None
keyboard_symbol[4][4] = None
keyboard_symbol[4][5] = '.'
keyboard_symbol[4][6] = '\''


BL_state = False
comdata_flag = False
comdata = ''

kb_brightness_duty = KB_BRIGHTNESS_BOOT_DUTY
kb_brightness_setting_duty = KB_BRIGHTNESS_DEFAULT_DUTY

for y in range(rowCount):
    print(rows[y], "as input")

    rows[y] = machine.Pin(rows[y], machine.Pin.IN)


for x in range(colCount):
    print(cols[x], "as input-pullup")
    cols[x] = (machine.Pin(cols[x], machine.Pin.IN, machine.Pin.PULL_UP))


keyboard_BL_PIN = machine.Pin(KEYBOARD_BL_PIN, machine.Pin.OUT)
keyboard_BL_PIN = machine.PWM(keyboard_BL_PIN, freq=KB_BRIGHTNESS_FREQ, duty_u16=0)


i2c_bus = I2C.Bus(
    host=0,
    sda=_I2C_SDA,
    scl=_I2C_SCL,
    freq=_I2C_FREQ
)


i2c = I2C.Device(
    bus=i2c_bus,
    dev_id=_I2C_ADDR,
    reg_bits=8
)


data_out = bytearray(1)
data_mv = memoryview(data_out)


def i2c_send(val):
    if isinstance(val, str):
        val = ord(val)

    data_out[0] = val

    i2c.write(buf=data_mv)


def loop():
    global comdata
    global comdata_flag
    global BL_state

    readMatrix()
    printMatrix()

    # key 3,3 is the enter key
    if keyPressed(3, 3):
        print('enter')
        comdata = 0x0D
        comdata_flag = True

    if keyPressed(4, 3):
        print("backspace")
        comdata = 0x08
        comdata_flag = True

    # Alt+B
    if keyActive(0, 4) and keyPressed(3, 4):
        print("Alt+B")
        # If the software sets the duty cycle to 0, then the value set
        # by the ATL+B register is used to ensure that ALT+B can normally light up the backlight.
        if BL_state:
            BL_state = False
            keyboard_BL_PIN.duty_u16(0)  # turn off
        else:
            BL_state = True
            if kb_brightness_duty == 0:
                print("User set bl duty is zero,use setting duty")
                keyboard_BL_PIN.duty_u16(kb_brightness_setting_duty)
            else:
                print("Duty is not zero ,use user setting bl value")

        comdata_flag = False  # Don't send char

    # Alt+C
    if keyActive(0, 4) and keyPressed(2, 5):
        print("Alt+C")
        comdata = 0x0C
        comdata_flag = True


def onRequest():
    global comdata
    global comdata_flag

    if comdata_flag:
        i2c_send(comdata)
        comdata_flag = False
        print("comdata :", comdata)
    else:
        i2c_send(0x00)


def readMatrix():
    # iterate the columns
    for colIndex in range(colCount):
        # col: set to output to low

        curCol = cols[colIndex]
        curCol.init(machine.Pin.OUT)
        curCol.value(0)

        # row: interate through the rows
        for rowIndex in range(rowCount):
            rowCol = rows[rowIndex]
            rowCol.init(machine.Pin.IN, pull=machine.Pin.PULL_UP)

            time.sleep_ms(1)  # arduino is not fast enought to switch input/output modes so wait 1 ms

            buttonPressed = bool(rowCol.value())

            keys[colIndex][rowIndex] = buttonPressed

            if lastValue[colIndex][rowIndex] != buttonPressed:
                changedValue[colIndex][rowIndex] = True
            else:
                changedValue[colIndex][rowIndex] = False

            lastValue[colIndex][rowIndex] = buttonPressed
            rowCol.init(machine.Pin.IN, pull=machine.Pin.PULL_HOLD)

        # disable the column
        curCol.init(machine.Pin.IN)


def keyPressed(colIndex, rowIndex):
    return changedValue[colIndex][rowIndex] and keys[colIndex][rowIndex] is True


def keyActive(colIndex, rowIndex):
    return keys[colIndex][rowIndex] is True


def isPrintableKey(colIndex, rowIndex):
    return keyboard_symbol[colIndex][rowIndex] is not None or keyboard[colIndex][rowIndex] is not None


def printMatrix():
    global comdata
    global comdata_flag

    for rowIndex in range(rowCount):
        for colIndex in range(colCount):
            # we only want to print if the key is pressed and it is a printable character

            if keyPressed(colIndex, rowIndex) and isPrintableKey(colIndex, rowIndex):
                if keyActive(0, 2):
                    toPrint = keyboard_symbol[colIndex][rowIndex]
                else:
                    toPrint = keyboard[colIndex][rowIndex]

                # keys 1,6 and 2,3 are Shift keys, so we want to upper case

                if keyActive(1, 6) or keyActive(2, 3):
                    toPrint = chr(ord(toPrint - 32))

                print(toPrint)

                comdata = toPrint
                comdata_flag = True

'''