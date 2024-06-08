import lvgl as lv
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


def build_crosshair(scrn):
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

    for radius, color, opa in (
        (39, 0xFFFFFF, 255),
        (34, 0xFFFFFF, 255),
        (19, 0xFFFFFF, 255),
        (9, 0x727171, 207),
        (8, 0xDD1919, 190),
        (4, 0xBABABA, 192)
    ):
        desc = lv.draw_arc_dsc_t()
        desc.init()

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

    for x1, y1, x2, y2 in (
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

    for a, b in (
        (24, 18),
        (27, 23),
        (32, 26),
        (35, 31),
        (64, 18),
        (61, 23),
        (56, 26),
        (53, 31),
        (64, 70),
        (61, 65),
        (56, 62),
        (53, 57)
    ):

        desc = lv.draw_line_dsc_t()
        desc.init()
        desc.p1.x = a
        desc.p1.y = b
        desc.p2.x = b
        desc.p2.y = a
        desc.color = lv.color_hex(0xFFFFFF)
        desc.width = 1
        desc.opa = 255
        desc.blend_mode = lv.BLEND_MODE.NORMAL
        desc.round_start = 0
        desc.round_end = 0

        lv.draw_line(layer, desc)

    for x1, y1, x2, y2 in (
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

    image = lv.image(scrn)
    image.add_style(style, 0)
    image.set_style_bg_opa(0, 0)
    image.set_size(89, 89)
    image.set_src(canvas.get_image())
    return image


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

        horRes = self.width = disp.get_physical_horizontal_resolution()
        verRes = self.height =disp.get_physical_vertical_resolution()

        marginH = horRes * 0.15
        marginV = verRes * 0.15
        margin = marginH if marginH < marginV else marginV

        self.points = [
            Tpcal_point(x=int(margin), y=int(verRes * 0.3)),
            Tpcal_point(x=int(horRes * 0.4), y=int(verRes - margin)),
            Tpcal_point(x=int(horRes - margin), y=int(margin))
        ]

        self.curr_point = self.points[0]
        self.curr_scrn = disp.get_screen_active()

        self.scr = lv.obj(None)
        self.scr.add_style(style, 0)
        self.scr.set_size(horRes, verRes)

        lv.screen_load(self.scr)

        self.indev = indev
        self.disp = disp

        self.cross_hair = build_crosshair(self.scr)

        crosshair_size = int(math.sqrt((horRes * verRes) >> 5))
        crosshair_scale = int(crosshair_size / 89.0 * 256.0)
        self.cross_hair.set_scale(crosshair_scale)
        self.cross_hair.set_size(crosshair_size, crosshair_size)
        self.cross_offset = int(crosshair_size / 2)

        self.cross_hair.set_pos(
            self.curr_point.x - self.cross_offset,
            self.curr_point.y - self.cross_offset
        )

        self.recal_button = lv.button(self.scr)
        self.recal_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.recal_button.remove_flag(lv.obj.FLAG.CLICKABLE)

        self.cal_ok_button = lv.button(self.scr)
        self.cal_ok_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.cal_ok_button.remove_flag(lv.obj.FLAG.CLICKABLE)

        recal_label = lv.text(self.recal_button)
        recal_label.set_text("Recalibrate")
        cal_ok_label = lv.text(self.cal_ok_button)
        cal_ok_label.set_text("Calibration Good")

        recal_label.center()
        cal_ok_label.center()

        self.cal_ok_button.update_layout()
        self.recal_button.update_layout()

        but_height = self.cal_ok_button.get_height()
        recal_but_width = self.recal_button.get_width()

        center_x = int(horRes / 2)
        center_y = int(verRes / 2)

        but_y = center_y - int(but_height / 2)
        recal_but_x = center_x - 7 - recal_but_width
        ok_but_x = center_x + 7
        self.recal_button.set_pos(recal_but_x, but_y)
        self.cal_ok_button.set_pos(ok_but_x, but_y)

        self.alphaX = None
        self.betaX = None
        self.deltaX = None
        self.alphaY = None
        self.betaY = None
        self.deltaY = None

        indev._indev_drv.set_read_cb(self.on_touch)

    def on_ok(self, _):
        lv.screen_load(self.curr_scrn)
        self.cross_hair.delete()
        self.cal_ok_button.delete()
        self.recal_button.delete()
        self.scr.delete()

        self.callback(
            self.alphaX,
            self.betaX,
            self.deltaX,
            self.alphaY,
            self.betaY,
            self.deltaY
        )

    def on_touch(self, drv, data):
        coords = self.indev._get_coords()

        if coords is None:
            state = self.indev.RELEASED
            x, y = self.indev._last_x, self.indev._last_y
        else:
            state, x, y = coords

        if None not in (x, y):
            self.indev._last_x, self.indev._last_y = x, y
            if coords is None:
                data.continue_reading = False
            else:
                data.continue_reading = True
        else:
            data.continue_reading = False

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
        self.alphaX = None
        self.betaX = None
        self.deltaX = None
        self.alphaY = None
        self.betaY = None
        self.deltaY = None

        self.recal_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.recal_button.remove_flag(lv.obj.FLAG.CLICKABLE)

        self.cal_ok_button.add_flag(lv.obj.FLAG.HIDDEN)
        self.cal_ok_button.remove_flag(lv.obj.FLAG.CLICKABLE)

        for point in self.points:
            point.x = 0
            point.y = 0

        self.curr_point = self.points[0]
        self.cross_hair.set_pos(
            self.curr_point.x - self.cross_offset,
            self.curr_point.y - self.cross_offset
        )
        self.cross_hair.add_flag(lv.obj.FLAG.CLICKABLE)

        self.scr.remove_event_cb(self.on_scr_click)

    def on_click(self):
        self.indev.get_point(self.curr_point.touched_point)
        index = self.points.index(self.curr_point) + 1
        if index == len(self.points):
            self.cross_hair.remove_flag(lv.obj.FLAG.CLICKABLE)

            self.recal_button.add_flag(lv.obj.FLAG.CLICKABLE)
            self.recal_button.remove_flag(lv.obj.FLAG.HIDDEN)

            self.cal_ok_button.add_flag(lv.obj.FLAG.CLICKABLE)
            self.cal_ok_button.remove_flag(lv.obj.FLAG.HIDDEN)

            tpoint1, tpoint2, tpoint3 = [
                point.touched_point for point in self.points
            ]
            spoint1, spoint2, spoint3 = self.points

            divisor = float(
                tpoint1.x *
                (tpoint3.y - tpoint2.y) -
                tpoint2.x *
                tpoint3.y +
                tpoint2.y *
                tpoint3.x +
                tpoint1.y *
                (tpoint2.x - tpoint3.x)
            )

            self.alphaX = float(
                spoint1.x *
                (tpoint3.y - tpoint2.y) -
                spoint2.x *
                tpoint3.y +
                spoint3.x *
                tpoint2.y +
                (spoint2.x - spoint3.x) *
                tpoint1.y
            ) / divisor
            self.betaX = -float(
                spoint1.x *
                (tpoint3.x - tpoint2.x) -
                spoint2.x *
                tpoint3.x +
                spoint3.x *
                tpoint2.x +
                (spoint2.x - spoint3.x) *
                tpoint1.x
            ) / divisor
            self.deltaX = float(
                spoint1.x *
                (tpoint2.y * tpoint3.x - tpoint2.x * tpoint3.y) +
                tpoint1.x *
                (spoint2.x * tpoint3.y - spoint3.x * tpoint2.y) +
                tpoint1.y *
                (spoint3.x * tpoint2.x - spoint2.x * tpoint3.x)
            ) / divisor
            self.alphaY = float(
                spoint1.y *
                (tpoint3.y - tpoint2.y) -
                spoint2.y *
                tpoint3.y +
                spoint3.y *
                tpoint2.y +
                (spoint2.y - spoint3.y) *
                tpoint1.y
            ) / divisor
            self.betaY = -float(
                spoint1.y *
                (tpoint3.x - tpoint2.x) -
                spoint2.y *
                tpoint3.x +
                spoint3.y *
                tpoint2.x +
                (spoint2.y - spoint3.y) *
                tpoint1.x
            ) / divisor
            self.deltaY = float(
                spoint1.y *
                (tpoint2.y * tpoint3.x - tpoint2.x * tpoint3.y) +
                tpoint1.x *
                (spoint2.y * tpoint3.y - spoint3.y * tpoint2.y) +
                tpoint1.y *
                (spoint3.y * tpoint2.x - spoint2.y * tpoint3.x)
            ) / divisor

            self.scr.add_event_cb(self.on_scr_click, lv.EVENT.CLICK, None)

        else:
            self.curr_point = self.points[index]
            self.cross_hair.set_pos(
                self.curr_point.x - self.cross_offset,
                self.curr_point.y - self.cross_offset
            )

    def on_scr_click(self, _):
        point = lv.point_t()
        self.indev.get_point(point)

        self.cross_hair.set_pos(
            point.x - self.cross_offset,
            point.y - self.cross_offset
        )
