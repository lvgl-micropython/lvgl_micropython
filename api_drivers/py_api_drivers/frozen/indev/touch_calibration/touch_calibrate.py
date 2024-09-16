import lvgl as lv  # NOQA
import math

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


def build_crosshair(scrn, img, scale, color_step):
    canvas = lv.canvas(scrn)
    canvas.add_style(style, 0)
    canvas.set_size(89, 89)
    draw_buf = lv.draw_buf_create(89, 89, lv.COLOR_FORMAT.ARGB8888, 0)
    canvas.set_draw_buf(draw_buf)
    canvas.fill_bg(lv.color_hex(0x000000), 0)

    layer = lv.layer_t()
    canvas.init_layer(layer)

    desc = lv.draw_arc_dsc_t()
    desc.init()

    desc.color = lv.color_hex(0x838383)
    desc.width = 4
    desc.start_angle = 0
    desc.end_angle = 3600
    desc.center.x = 44
    desc.center.y = 44
    desc.radius = 37
    desc.opa = 88
    desc.rounded = 0

    lv.draw_arc(layer, desc)

    for i, (radius, na_color, a_color, opa) in enumerate((
        (39, 0xFF0000, 0x00FF00, 255),
        (34, 0xFF0000, 0x00FF00, 255),
        (19, 0xFF0000, 0x00FF00, 255),
        (9, 0x6D0000, 0x006D00, 207),
        (8, 0xDD1919, 0x19DD19, 190),
        (4, 0xB20000, 0x00B200, 192)
    )):
        desc = lv.draw_arc_dsc_t()
        desc.init()


        if i <= color_step:
            color = a_color
        else:
            color = na_color

        desc.color = lv.color_hex(color)
        desc.width = 1
        desc.start_angle = 0
        desc.end_angle = 3600
        desc.center.x = 44
        desc.center.y = 44
        desc.radius = radius
        desc.opa = opa
        desc.rounded = 0

        lv.draw_arc(layer, desc)

    desc = lv.draw_line_dsc_t()
    desc.init()
    desc.p1.x = 43
    desc.p1.y = 44
    desc.p2.x = 45
    desc.p2.y = 44
    desc.color = lv.color_hex(0xFD0202)
    desc.width = 3
    desc.opa = 255
    desc.blend_mode = lv.BLEND_MODE.NORMAL
    desc.round_start = 0
    desc.round_end = 0

    lv.draw_line(layer, desc)

    for x, y in (
        (44, 42),
        (44, 46),
        (42, 44),
        (46, 44)
    ):
        canvas.set_px(x, y, lv.color_hex(0xDD1818), 255)

    for x, y in (
        (42, 43),
        (46, 43),
        (42, 45),
        (46, 45),
        (43, 42),
        (45, 42),
        (43, 46),
        (45, 46)
    ):
        canvas.set_px(x, y, lv.color_hex(0xD82424), 186)

    # tested
    for (x1, y1), (x2, y2) in (
        ((34, 34), (37, 37)),
        ((54, 34), (51, 37)),
        ((34, 54), (37, 51)),
        ((54, 54), (51, 51))
    ):

        desc = lv.draw_line_dsc_t()

        desc.init()
        desc.p1.x = x1
        desc.p1.y = y1
        desc.p2.x = x2
        desc.p2.y = y2
        desc.color = lv.color_hex(0xFD0202)
        desc.width = 1
        desc.opa = 255
        desc.blend_mode = lv.BLEND_MODE.NORMAL
        desc.round_start = 0
        desc.round_end = 0

        lv.draw_line(layer, desc)

    for item in (
        (35, 31),
        (32, 26),
        (27, 23),
        (24, 18),
        (64, 70),
        (61, 65),
        (56, 62),
        (53, 57),
        (53, 31, 56, 34),
        (56, 26, 61, 31),
        (61, 23, 64, 26),
        (64, 18, 69, 23),
    ):

        desc = lv.draw_line_dsc_t()
        desc.init()
        desc.color = lv.color_hex(0xFFFFFF)
        desc.width = 1
        desc.opa = 255
        desc.blend_mode = lv.BLEND_MODE.NORMAL
        desc.round_start = 0
        desc.round_end = 0

        if len(item) == 2:
            a, b = item
            desc.p1.x = a
            desc.p1.y = b
            desc.p2.x = b
            desc.p2.y = a
        else:
            a, b, c, d = item
            desc.p1.x = a
            desc.p1.y = b
            desc.p2.x = c
            desc.p2.y = d

            desc2 = lv.draw_line_dsc_t()
            desc2.init()
            desc2.color = lv.color_hex(0xFFFFFF)
            desc2.width = 1
            desc2.opa = 255
            desc2.blend_mode = lv.BLEND_MODE.NORMAL
            desc2.round_start = 0
            desc2.round_end = 0
            desc2.p1.x = b
            desc2.p1.y = a
            desc2.p2.x = d
            desc2.p2.y = c
            lv.draw_line(layer, desc2)

        lv.draw_line(layer, desc)

    for (x1, y1), (x2, y2) in (
        ((24, 70), (18, 64)),
        ((27, 65), (23, 61)),
        ((32, 62), (26, 56)),
        ((35, 57), (31, 53)),
        ((21, 21), (33, 33)),
        ((67, 21), (55, 33)),
        ((21, 67), (33, 55)),
        ((67, 67), (55, 55))
    ):
        desc = lv.draw_line_dsc_t()

        desc.init()
        desc.p1.x = x1
        desc.p1.y = y1
        desc.p2.x = x2
        desc.p2.y = y2
        desc.color = lv.color_hex(0xFFFFFF)
        desc.width = 1
        desc.opa = 255
        desc.blend_mode = lv.BLEND_MODE.NORMAL
        desc.round_start = 0
        desc.round_end = 0

        lv.draw_line(layer, desc)

    canvas.finish_layer(layer)

    img.set_src(canvas.get_image())
    img.set_scale(scale)

    canvas.delete()


class Tpcal_point(object):

    def __init__(self, x, y):
        self.target_point = lv.point_t(dict(x=x, y=y))
        self.touched_point = lv.point_t()

    @property
    def x(self):
        return self.target_point.x

    @x.setter
    def x(self, value):
        self.touched_point.x = value

    @property
    def y(self):
        return self.target_point.y

    @y.setter
    def y(self, value):
        self.touched_point.y = value

    def __str__(self):
        return (
            "Tpcal_point("
            "touch_x={0}, "
            "touch_y={1}, "
            "screen_x={2}, "
            "screen_y={3})"
        ).format(
            self.touched_point.x,
            self.touched_point.y,
            self.target_point.x,
            self.target_point.y
        )


class TPCal(object):

    def __init__(self, indev, callback):
        self.callback = callback
        disp = indev.get_disp()
        disp.set_default()

        width = self.width = disp._physical_width
        height = self.height = disp._physical_height

        self.curr_scrn = disp.get_screen_active()

        self.scr = lv.obj(None)
        self.scr.add_style(style, 0)
        self.scr.set_size(width, height)

        lv.screen_load(self.scr)

        self.indev = indev
        self.disp = disp
        
        self.run_again_timer = None

        self.crosshair = lv.image(self.scr)
        self.crosshair.add_style(style, 0)
        self.crosshair.set_style_bg_opa(0, 0)
        self.crosshair.set_size(89, 89)

        self.crosshair_size = int(math.sqrt((width * height) >> 5))
        self.crosshair_scale = int(self.crosshair_size / 89.0 * 256.0)
        self.crosshair.set_size(self.crosshair_size, self.crosshair_size)
        self.cross_offset = int(self.crosshair_size / 2)

        build_crosshair(self.scr, self.crosshair, self.crosshair_scale, -1)

        self.crosshair.set_pos(0, 0)

        self.points = [
            Tpcal_point(x=self.cross_offset, y=self.cross_offset),
            Tpcal_point(x=self.cross_offset, y=int(height - self.cross_offset)),
            Tpcal_point(x=int(width - self.cross_offset), y=self.cross_offset)
        ]

        self.curr_point = self.points[0]

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
        
        self.alphaX = None
        self.betaX = None
        self.deltaX = None
        self.alphaY = None
        self.betaY = None
        self.deltaY = None

        self.run_count = 30

        self.scr.add_event_cb(self.on_press, lv.EVENT.PRESSED, None)
        indev._indev_drv.set_read_cb(self.on_touch)  # NOQA

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

        if coords is None:
            state = self.indev.RELEASED
            x, y = self.indev._last_x, self.indev._last_y
        else:
            state, x, y = coords

        if None not in (x, y):
            self.indev._last_x, self.indev._last_y = x, y

        data.point.x, data.point.y = (
            self.indev._last_x, self.indev._last_y
        )
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
            point.touched_point.x = 0
            point.touched_point.y = 0

        self.curr_point = self.points[0]
        self.crosshair.set_pos(0, 0)

        self.scr.add_event_cb(self.on_press, lv.EVENT.PRESSED, None)
        self.scr.remove_event_cb(self.on_scr_click)
    
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
        self.indev.get_point(self.curr_point.touched_point)
        index = self.points.index(self.curr_point) + 1

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
            
            tp1, tp2, tp3 = [point.touched_point for point in self.points]
            sp1, sp2, sp3 = self.points

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
            self.scr.add_event_cb(self.on_scr_click, lv.EVENT.PRESSED, None)

        else:
            self.curr_point = self.points[index]
            self.crosshair.set_pos(
                self.curr_point.x - self.cross_offset,
                self.curr_point.y - self.cross_offset
            )

    def on_scr_click(self, _):
        point = lv.point_t()
        self.indev.get_point(point)

        self.crosshair.set_pos(
            point.x - self.cross_offset,
            point.y - self.cross_offset
        )
