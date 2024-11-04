import lvgl as lv  # NOQA
import math
import time

style = lv.style_t()
style.init()

style.set_bg_color(lv.color_hex(0x000000))
style.set_bg_opa(255)
style.set_border_opa(0)
style.set_border_width(0)
style.set_margin_bottom(0)
style.set_margin_left(0)
style.set_margin_right(0)
style.set_margin_top(0)
# style.set_opa(0)
style.set_outline_opa(0)
style.set_outline_pad(0)
style.set_outline_width(0)
style.set_pad_left(0)
style.set_pad_right(0)
style.set_pad_top(0)
style.set_pad_bottom(0)
style.set_radius(0)
style.set_shadow_offset_x(0)
style.set_shadow_offset_y(0)
style.set_shadow_opa(0)
style.set_shadow_spread(0)
style.set_shadow_width(0)



def draw_base_crosshair(scrn, size, center):
    canvas = lv.canvas(scrn)
    canvas.add_style(style, 0)
    canvas.set_size(size + 1, size + 1)
    draw_buf = lv.draw_buf_create(size + 1, size + 1, lv.COLOR_FORMAT.ARGB8888, 0)
    canvas.set_draw_buf(draw_buf)
    canvas.fill_bg(lv.color_hex(0x000000), 0)

    layer = lv.layer_t()
    canvas.init_layer(layer)

    vertical_line = lv.draw_line_dsc_t()
    vertical_line.init()
    vertical_line.p1.x = center
    vertical_line.p1.y = 0
    vertical_line.p2.x = center
    vertical_line.p2.y = size
    vertical_line.color = lv.color_hex(0x00FF00)
    vertical_line.width = 3
    vertical_line.opa = 255
    vertical_line.blend_mode = lv.BLEND_MODE.NORMAL
    vertical_line.round_start = 0
    vertical_line.round_end = 0

    lv.draw_line(layer, vertical_line)

    horizontal_line = lv.draw_line_dsc_t()
    horizontal_line.init()
    horizontal_line.p1.x = 0
    horizontal_line.p1.y = center
    horizontal_line.p2.x = size
    horizontal_line.p2.y = center
    horizontal_line.color = lv.color_hex(0x00FF00)
    horizontal_line.width = 3
    horizontal_line.opa = 255
    horizontal_line.blend_mode = lv.BLEND_MODE.NORMAL
    horizontal_line.round_start = 0
    horizontal_line.round_end = 0

    lv.draw_line(layer, horizontal_line)

    return canvas, layer


def build_crosshair(scrn):
    display_height = scrn.get_height()
    display_width = scrn.get_width()
    size = min(display_width, display_height)
    size = int(size * 0.2)

    if not size % 2:
        size += 1

    center = int(size / 2)
    size -= 1

    canvas, layer = draw_base_crosshair(scrn, size, center)

    large_radius = center + 1
    small_radius = int(large_radius * 0.33)

    outside_circle = lv.draw_arc_dsc_t()
    outside_circle.init()

    outside_circle.color = lv.color_hex(0xFF0000)
    outside_circle.width = 2
    outside_circle.start_angle = 0
    outside_circle.end_angle = 3600
    outside_circle.center.x = center
    outside_circle.center.y = center
    outside_circle.radius = large_radius
    outside_circle.opa = 255
    outside_circle.rounded = 0

    lv.draw_arc(layer, outside_circle)

    inside_circle = lv.draw_arc_dsc_t()
    inside_circle.init()

    inside_circle.color = lv.color_hex(0xFF0000)
    inside_circle.width = 1
    inside_circle.start_angle = 0
    inside_circle.end_angle = 3600
    inside_circle.center.x = center
    inside_circle.center.y = center
    inside_circle.radius = small_radius
    inside_circle.opa = 255
    inside_circle.rounded = 0

    lv.draw_arc(layer, inside_circle)

    canvas.finish_layer(layer)
    base_crosshair = canvas.get_image()
    canvas.delete()

    radius_range = large_radius - small_radius
    num_grads = int(radius_range * 0.33)

    grad_factor = 255 / num_grads / 255
    crosshair_anim = []

    for i in range(radius_range + num_grads):
        canvas, layer = draw_base_crosshair(scrn, size, center)
        inside_circle.color = lv.color_hex(0x9424AC)

        for j in range(num_grads, -1, -1):
            r = i + small_radius - j
            if r < small_radius or r > large_radius:
                continue

            inside_circle.opa = int((~j + num_grads + 1) * grad_factor * 255)
            inside_circle.radius = r
            lv.draw_arc(layer, inside_circle)

        clear_radius = i - num_grads - 1
        if large_radius >= clear_radius >= small_radius:
            inside_circle.color = lv.color_hex(0x000000)
            inside_circle.opa = 255
            inside_circle.radius = clear_radius
            lv.draw_arc(layer, inside_circle)

        canvas.finish_layer(layer)
        crosshair_anim.append(canvas.get_image())
        canvas.delete()

    return base_crosshair, crosshair_anim


class Tpcal_point(object):

    def __init__(self, x, y, center):
        self.center = center
        self.target_point = lv.point_t(dict(x=x, y=y))
        self.touched_points = []

    @property
    def target(self):
        return self.target_point.x, self.target_point.y

    def __setitem__(self, item, value):
        try:
            point = self.touched_points[item]
        except IndexError:
            point = lv.point_t()
            self.touched_points.append(point)

        x, y = value

        point.x = x
        point.y = y

    def __len__(self):
        return len(self.touched_points)

    @property
    def x(self):
        return self.target_point.x

    @property
    def y(self):
        return self.target_point.y

    def get_center_point(self):
        x = self.target_point.x + self.center
        y = self.target_point.y + self.center
        return lv.point_t(dict(x=x, y=y))

    def get_calculated_point(self):
        x_points = [point.x for point in self.touched_points]
        y_points = [point.y for point in self.touched_points]

        # trim off the highest and lowest values to
        # eliminate them as possible outliners
        x_points = sorted(x_points)[1:-1]
        y_points = sorted(y_points)[1:-1]

        # get the average x and y
        avg_x = int(sum(x_points) / len(x_points))
        avg_y = int(sum(y_points) / len(y_points))

        return lv.point_t(dict(x=avg_x, y=avg_y))

    def reset(self):
        del self.touched_points[:]

    def __str__(self):
        calculated = self.get_calculated_point()
        center = self.get_center_point()

        res = [
            "Tpcal_point("
            f"  ScreenPoint(x={center.x}, y={center.y}),"
        ]

        for i, point in enumerate(self.touched_points):
            res.append(f"  TouchPoint_{i + 1}(x={point.x}, y={point.y}),")

        res.extend([
            f"  CalcPoint(x={calculated.x}, y={calculated.y})",
            ")"
        ])

        return '\n'.join(res)


class TPCal(object):

    def __init__(self, touch_count, indev, callback):
        self.callback = callback
        disp = indev.get_disp()
        disp.set_default()

        width = self.width = disp._physical_width
        height = self.height = disp._physical_height

        size = min(width, height)
        size = int(size * 0.20)
        if not size % 2:
            size += 1

        self.curr_scrn = disp.get_screen_active()

        self.scr = lv.obj(None)
        self.scr.add_style(style, 0)
        self.scr.set_size(width, height)
        self.scr.remove_flag(lv.obj.FLAG.SCROLLABLE)

        lv.screen_load(self.scr)

        self.indev = indev
        self.disp = disp
        
        self.run_again_timer = None
        self.touch_count = touch_count

        self.base_crosshair, self.crosshair_anim = build_crosshair(self.scr)
        self.crosshair_size = size
        self.crosshair_center = int(size / 2)

        self.points = [
            Tpcal_point(
                x=5 + self.crosshair_center,
                y=5 + self.crosshair_center,
                center=self.crosshair_center
            ),
            Tpcal_point(
                x=5 + self.crosshair_center,
                y=int(height - self.crosshair_size - 5),
                center=self.crosshair_center
            ),
            Tpcal_point(
                x=int(width - self.crosshair_size - 5),
                y=5 + self.crosshair_center,
                center=self.crosshair_center
            )
        ]

        self.crosshair = lv.image(self.scr)
        self.crosshair.add_style(style, 0)
        self.crosshair.set_style_bg_opa(0, 0)
        self.crosshair.set_size(size, size)
        self.crosshair.set_src(self.base_crosshair)
        self.crosshair.remove_flag(lv.obj.FLAG.SCROLLABLE)
        self.crosshair.remove_flag(lv.obj.FLAG.CLICKABLE)

        self.curr_point = self.points[0]
        self.crosshair.set_pos(self.curr_point.x, self.curr_point.y)

        self.run_again_button = lv.button(self.scr)
        self.run_again_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.run_again_button.remove_flag(lv.obj.FLAG.CLICKABLE)
        
        self.run_again_button.add_event_cb(self.on_recalibrate, lv.EVENT.CLICKED, None)

        self.ok_button = lv.button(self.scr)
        self.ok_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.ok_button.remove_flag(lv.obj.FLAG.CLICKABLE)
        
        self.ok_button.add_event_cb(self.on_ok, lv.EVENT.CLICKED, None)
        
        run_again_label = self.run_again_label = lv.label(self.run_again_button)
        run_again_label.set_text("Run Again (30)")
        ok_label = lv.label(self.ok_button)
        ok_label.set_text("OK")

        self.ok_button.set_size(lv.SIZE_CONTENT, lv.SIZE_CONTENT)
        self.run_again_button.set_size(lv.SIZE_CONTENT, lv.SIZE_CONTENT)

        run_again_label.center()
        ok_label.center()
        
        self.ok_button.update_layout()
        self.run_again_button.update_layout()

        but_height = self.ok_button.get_height()
        recal_but_width = self.run_again_button.get_width()

        center_x = int(width / 2)
        center_y = int(height / 2)

        but_y = center_y - int(but_height / 2)
        recal_but_x = center_x - 7 - recal_but_width
        ok_but_x = center_x + 7
        self.run_again_button.set_pos(recal_but_x, but_y)
        self.ok_button.set_pos(ok_but_x, but_y)

        self.info_text = lv.label(self.scr)
        self.info_text.set_text('Touch Screen Calibration')
        self.info_text.update_layout()
        
        info_height = self.info_text.get_height()
        self.info_text.align(lv.ALIGN.CENTER, 0, -info_height - 10)
        self.info_text.remove_flag(lv.obj.FLAG.CLICKABLE)

        self.count_text = lv.label(self.scr)
        self.count_text.set_text('Target press count:  0')
        self.count_text.update_layout()

        count_height = self.count_text.get_height()
        self.count_text.align(lv.ALIGN.CENTER, 0, -count_height - info_height - 20)
        self.count_text.remove_flag(lv.obj.FLAG.CLICKABLE)
        
        self.alphaX = None
        self.betaX = None
        self.deltaX = None
        self.alphaY = None
        self.betaY = None
        self.deltaY = None

        self.run_count = 30

        self.scr.add_event_cb(self.on_press, lv.EVENT.PRESSED, None)
        indev._indev_drv.set_read_cb(self.on_touch)  # NOQA
        self.taking_samples = True

    def on_ok(self, _):
        self.run_again_timer.delete()
        self.run_again_timer = None
        
        lv.screen_load(self.curr_scrn)
        self.crosshair.delete()
        self.ok_button.delete()
        self.run_again_button.delete()
        self.scr.delete()

        self.callback(
            self.alphaX,
            self.betaX,
            self.deltaX,
            self.alphaY,
            self.betaY,
            self.deltaY
        )

    def on_touch(self, _, data):
        coords = self.indev._get_coords()  # NOQA

        def get_coords():
            if coords is None:
                state_ = self.indev.RELEASED
                x_, y_ = self.indev._last_x, self.indev._last_y
            else:
                state_, x_, y_ = coords

            if None in (x_, y_):
                x_, y_ = self.indev._last_x, self.indev._last_y

            return state_, x_, y_

        state, x, y = get_coords()
        count = 0

        while self.taking_samples:
            point = self.curr_point
            if state == self.indev.PRESSED:
                point[count] = (x, y)
                count += 1

                count_text = str(count)
                if len(count_text) < 2:
                    count_text = ' ' + count_text

                self.count_text.set_text(f'Target press count: {count_text}')

            anim_index = 0

            while state == self.indev.PRESSED:
                self.crosshair.set_src(self.crosshair_anim[anim_index])
                anim_index += 1

                if anim_index == len(self.crosshair_anim):
                    anim_index = 0

                time.sleep_ms(2)
                state, x, y = get_coords()

            self.crosshair.set_src(self.base_crosshair)

            if count == self.touch_count:
                self.taking_samples = False
                state = self.indev.PRESSED
            else:
                while state == self.indev.RELEASED:
                    time.sleep_ms(2)
                    state, x, y = get_coords()

        x, y = self.calibrate_touch(x, y)

        self.indev._last_x = x
        self.indev._last_y = y

        data.point.x = x
        data.point.y = y
        data.state = state

    def calibrate_touch(self, x, y):
        if None in (
            self.alphaX,
            self.betaX,
            self.deltaX,
            self.alphaY,
            self.betaY,
            self.deltaY
        ):
            return x, y

        new_x = int(round(x * self.alphaX + y * self.betaX + self.deltaX))
        new_y = int(round(x * self.alphaY + y * self.betaY + self.deltaY))
        return new_x, new_y

    def on_recalibrate(self, _):
        self.run_again_timer.delete()
        self.run_again_timer = None
        
        self.alphaX = None
        self.betaX = None
        self.deltaX = None
        self.alphaY = None
        self.betaY = None
        self.deltaY = None

        self.run_again_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.run_again_button.remove_flag(lv.obj.FLAG.CLICKABLE)

        self.ok_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.ok_button.remove_flag(lv.obj.FLAG.CLICKABLE)

        for point in self.points:
            point.reset()

        self.curr_point = self.points[0]

        self.crosshair.set_pos(self.curr_point.x, self.curr_point.y)
        self.taking_samples = True
        self.scr.add_event_cb(self.on_press, lv.EVENT.PRESSED, None)
        self.scr.remove_event_cb(self.on_scr_click)
        self.scr.remove_event_cb(self.on_scr_drag)

    def on_run_again_timer(self, _):
        self.run_count -= 1

        label = str(self.run_count)
        if len(label) != 2:
            label = '0' + label

        self.run_again_label.set_text(
            f"Run Again ({label})"
        )
        
        if self.run_count == 0:
            self.run_count = 30
            self.on_recalibrate(None)
            
    def on_press(self, _):
        index = self.points.index(self.curr_point) + 1
        self.count_text.set_text('Target press count:  0')

        if index == len(self.points):
            self.crosshair.remove_flag(lv.obj.FLAG.CLICKABLE)
            self.run_count = 30
            self.run_again_label.set_text("Run Again (30)")
            
            self.run_again_button.add_flag(lv.obj.FLAG.CLICKABLE)
            self.run_again_button.remove_flag(lv.obj.FLAG.HIDDEN)

            self.ok_button.add_flag(lv.obj.FLAG.CLICKABLE)
            self.ok_button.remove_flag(lv.obj.FLAG.HIDDEN)
            
            self.run_again_timer = lv.timer_create(
                self.on_run_again_timer, 
                1000, None)
            
            self.run_again_timer.set_repeat_count(30)
            for i, point in enumerate(self.points):
                print(f'{i + 1}: {str(point)}')
            
            tp1, tp2, tp3 = [point.get_calculated_point() for point in self.points]
            sp1, sp2, sp3 = [point.get_center_point() for point in self.points]

            divisor = float(
                tp1.x * (tp3.y - tp2.y) - tp2.x * tp3.y +
                tp2.y * tp3.x + tp1.y * (tp2.x - tp3.x)
            )

            self.alphaX = float(
                sp1.x * (tp3.y - tp2.y) - sp2.x * tp3.y +
                sp3.x * tp2.y + (sp2.x - sp3.x) * tp1.y
            ) / divisor
            self.betaX = -float(
                sp1.x * (tp3.x - tp2.x) - sp2.x * tp3.x +
                sp3.x * tp2.x + (sp2.x - sp3.x) * tp1.x
            ) / divisor
            self.deltaX = float(
                sp1.x * (tp2.y * tp3.x - tp2.x * tp3.y) +
                tp1.x * (sp2.x * tp3.y - sp3.x * tp2.y) +
                tp1.y * (sp3.x * tp2.x - sp2.x * tp3.x)
            ) / divisor
            self.alphaY = float(
                sp1.y * (tp3.y - tp2.y) - sp2.y * tp3.y +
                sp3.y * tp2.y + (sp2.y - sp3.y) * tp1.y
            ) / divisor
            self.betaY = -float(
                sp1.y * (tp3.x - tp2.x) - sp2.y * tp3.x +
                sp3.y * tp2.x + (sp2.y - sp3.y) * tp1.x
            ) / divisor
            self.deltaY = float(
                sp1.y * (tp2.y * tp3.x - tp2.x * tp3.y) +
                tp1.x * (sp2.y * tp3.y - sp3.y * tp2.y) +
                tp1.y * (sp3.y * tp2.x - sp2.y * tp3.x)
            ) / divisor

            self.scr.remove_event_cb(self.on_press)
            self.scr.add_event_cb(self.on_scr_click, lv.EVENT.CLICKED, None)
            self.scr.add_event_cb(self.on_scr_drag, lv.EVENT.PRESSING, None)
        else:
            self.curr_point = self.points[index]
            self.crosshair.set_pos(
                self.curr_point.x,
                self.curr_point.y
            )
            self.taking_samples = True

    def on_scr_drag(self):
        vect = lv.point_t()
        self.indev.get_vect(vect)
        x = self.crosshair.get_x_aligned() + vect.x
        y = self.crosshair.get_y_aligned() + vect.y
        self.crosshair.set_pos(x, y)

    def on_scr_click(self, _):
        point = lv.point_t()
        self.indev.get_point(point)
        self.crosshair.set_pos(
            point.x - self.crosshair_center,
            point.y - self.crosshair_center
        )
