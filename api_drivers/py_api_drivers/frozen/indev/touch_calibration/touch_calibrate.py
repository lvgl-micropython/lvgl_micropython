# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import time

import task_handler
import lcd_bus


style = lv.style_t()
style.init()  # NOQA

style.set_bg_color(lv.color_hex(0x000000))  # NOQA
style.set_bg_opa(255)  # NOQA
style.set_border_opa(0)  # NOQA
style.set_border_width(0)  # NOQA
style.set_margin_bottom(0)  # NOQA
style.set_margin_left(0)  # NOQA
style.set_margin_right(0)  # NOQA
style.set_margin_top(0)  # NOQA
# style.set_opa(0)
style.set_outline_opa(0)  # NOQA
style.set_outline_pad(0)  # NOQA
style.set_outline_width(0)  # NOQA
style.set_pad_left(0)  # NOQA
style.set_pad_right(0)  # NOQA
style.set_pad_top(0)  # NOQA
style.set_pad_bottom(0)  # NOQA
style.set_radius(0)  # NOQA
style.set_shadow_offset_x(0)  # NOQA
style.set_shadow_offset_y(0)  # NOQA
style.set_shadow_opa(0)  # NOQA
style.set_shadow_spread(0)  # NOQA
style.set_shadow_width(0)  # NOQA


def calibrate(indev, cal_data):

    if not task_handler.TaskHandler.is_running():
        th_running = False
        task_handler.TaskHandler()
    else:
        th_running = True

    width = indev._orig_width  # NOQA
    height = indev._orig_height  # NOQA

    coords = [
        [20, 20],  # upper left
        [width - 40, 20],  # upper right
        [20, height - 40]  # lower left
    ]

    target_points = [
        dict(x=30, y=30),  # upper left
        dict(x=width - 30, y=30),  # upper right
        dict(x=30, y=height - 30),  # lower left
    ]

    captured_points = [
        dict(x=[], y=[]),
        dict(x=[], y=[]),
        dict(x=[], y=[])
    ]

    old_scrn = lv.screen_active()  # NOQA

    disp = old_scrn.get_display()
    rotation = disp.get_rotation()

    if rotation != lv.DISPLAY_ROTATION._0:  # NOQA
        disp.set_rotation(lv.DISPLAY_ROTATION._0)  # NOQA

    indev.enable(False)

    new_scrn = lv.obj()  # NOQA
    new_scrn.add_style(style, 0)

    label = lv.label(new_scrn)  # NOQA
    label.set_text('Touch Calibration')
    label.center()

    lv.screen_load_anim(new_scrn, lv.SCR_LOAD_ANIM.FADE_IN, 500, 0, False)  # NOQA

    count = 0
    while count < 200:
        count += 1
        lcd_bus._pump_main_thread()  # NOQA
        time.sleep_ms(5)  # NOQA

    label.delete()
    count = 0
    while count < 100:
        count += 1
        lcd_bus._pump_main_thread()  # NOQA
        time.sleep_ms(5)  # NOQA

    target = lv.obj(new_scrn)  # NOQA
    target.add_style(style, 0)
    target.set_size(20, 20)
    target.set_style_bg_color(lv.color_hex(0xFF0000), 0)

    new_scrn.remove_flag(lv.obj.FLAG.CLICKABLE)  # NOQA
    new_scrn.remove_flag(lv.obj.FLAG.CLICK_FOCUSABLE)  # NOQA
    new_scrn.remove_flag(lv.obj.FLAG.CHECKABLE)  # NOQA
    new_scrn.remove_flag(lv.obj.FLAG.SCROLLABLE)  # NOQA

    target.remove_flag(lv.obj.FLAG.CLICKABLE)  # NOQA
    target.remove_flag(lv.obj.FLAG.CLICK_FOCUSABLE)  # NOQA
    target.remove_flag(lv.obj.FLAG.CHECKABLE)  # NOQA
    target.remove_flag(lv.obj.FLAG.SCROLLABLE)  # NOQA

    for i in range(3):
        print('point', i + 1, 'of 3')

        target.set_pos(*coords[i])
        lcd_bus._pump_main_thread()  # NOQA

        time.sleep_ms(1000)  # NOQA

        for j in range(8):
            lcd_bus._pump_main_thread()  # NOQA
            touch = indev._get_coords()  # NOQA
            if touch is not None:
                state = touch[0]
            else:
                state = 0

            while touch is None or not state:
                time.sleep_ms(10)  # NOQA
                lcd_bus._pump_main_thread()  # NOQA
                touch = indev._get_coords()  # NOQA

                if touch is not None:
                    state = touch[0]
                else:
                    state = 0

            x, y = touch[1:]
            captured_points[i]['x'].append(x)
            captured_points[i]['y'].append(y)

            print('  ', j + 1, 'of 8:', (x, y))

    print()
    print('averaged trimmed points')
    for i, points in enumerate(captured_points):
        points['x'].remove(max(points['x']))
        points['x'].remove(min(points['x']))
        points['y'].remove(max(points['y']))
        points['y'].remove(min(points['y']))

        points['x'] = int(sum(points['x']) / 6)
        points['y'] = int(sum(points['y']) / 6)

        print('  point', i + 1, 'of 3:', (points['x'], points['y']))

    if captured_points[0]['x'] > captured_points[1]['x']:
        mirror_x = True
    else:
        mirror_x = False

    if captured_points[0]['y'] > captured_points[2]['y']:
        mirror_y = True
    else:
        mirror_y = False

    print('mirroring')
    print('  mirrored x:', mirror_x)
    print('  mirrored y:', mirror_y)

    tp1, tp2, tp3 = captured_points
    sp1, sp2, sp3 = target_points
    print()
    print('calibration values')
    try:
        divisor = float(
            tp1['x'] * (tp3['y'] - tp2['y']) - tp2['x'] * tp3['y'] +
            tp2['y'] * tp3['x'] + tp1['y'] * (tp2['x'] - tp3['x'])
        )
        alphaX = float(
            sp1['x'] * (tp3['y'] - tp2['y']) - sp2['x'] * tp3['y'] +
            sp3['x'] * tp2['y'] + (sp2['x'] - sp3['x']) * tp1['y']
        ) / divisor
        print('  alphaX:', alphaX)
        betaX = -float(
            sp1['x'] * (tp3['x'] - tp2['x']) - sp2['x'] * tp3['x'] +
            sp3['x'] * tp2['x'] + (sp2['x'] - sp3['x']) * tp1['x']
        ) / divisor
        print('  betaX:', betaX)
        deltaX = float(
            sp1['x'] * (tp2['y'] * tp3['x'] - tp2['x'] * tp3['y']) +
            tp1['x'] * (sp2['x'] * tp3['y'] - sp3['x'] * tp2['y']) +
            tp1['y'] * (sp3['x'] * tp2['x'] - sp2['x'] * tp3['x'])
        ) / divisor
        print('  deltaX:', deltaX)
        alphaY = float(
            sp1['y'] * (tp3['y'] - tp2['y']) - sp2['y'] * tp3['y'] +
            sp3['y'] * tp2['y'] + (sp2['y'] - sp3['y']) * tp1['y']
        ) / divisor
        print('  alphaY:', alphaY)
        betaY = -float(
            sp1['y'] * (tp3['x'] - tp2['x']) - sp2['y'] * tp3['x'] +
            sp3['y'] * tp2['x'] + (sp2['y'] - sp3['y']) * tp1['x']
        ) / divisor
        print('  betaY:', betaY)
        deltaY = float(
            sp1['y'] * (tp2['y'] * tp3['x'] - tp2['x'] * tp3['y']) +
            tp1['x'] * (sp2['y'] * tp3['y'] - sp3['y'] * tp2['y']) +
            tp1['y'] * (sp3['y'] * tp2['x'] - sp2['y'] * tp3['x'])
        ) / divisor
        print('  deltaY:', deltaY)

        cal_data.alphaX = alphaX
        cal_data.betaX = betaX
        cal_data.deltaX = deltaX
        cal_data.alphaY = alphaY
        cal_data.betaY = betaY
        cal_data.deltaY = deltaY

        cal_data.mirrorX = mirror_x
        cal_data.mirrorY = mirror_y

    except ZeroDivisionError:
        print('Error in calculation please try again.')
        res = False
    else:
        res = True

    lv.screen_load(old_scrn)  # NOQA
    new_scrn.delete()

    lcd_bus._pump_main_thread()  # NOQA

    if not th_running:
        task_handler.TaskHandler._current_instance.deinit()  # NOQA

    indev.enable(True)

    if rotation != lv.DISPLAY_ROTATION._0:  # NOQA
        disp.set_rotation(rotation)

    return res
