import lvgl as lv

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


class Tpcal_point(object):

    def __init__(self, x, y, name):
        self.point = lv.point_t()
        self.point.x = x
        self.point.y = y
        self.state = lv.INDEV_STATE.PRESSED
        self.touch_coordinate = None
        self.name = name

    def __repr__(self):
        return "%s: (tx=%d, ty=%d, dx=%d, dy=%d)" % (
            self.name,
            self.touch_coordinate.x,
            self.touch_coordinate.y,
            self.point.x,
            self.point.y
        )


class Tpcal(object):

    def __init__(self, touch_count=500):
        indev = lv.indev_active()
        self.touch = touch = indev.get_driver_data()
        self.config = self.touch._config

        width = touch.width
        height = touch.height

        self.points = [
            Tpcal_point(
                int(width * 0.20),
                int(height * 0.20),
                "upper left-hand corner"
                ),
            Tpcal_point(
                int(width * 0.80),
                int(height * 0.20),
                "upper right-hand corner"
                ),
            Tpcal_point(
                int(width * 0.20),
                int(height * 0.80),
                "lower left-hand corner"
                ),
            Tpcal_point(
                int(width * 0.80),
                int(height * 0.80),
                "lower right-hand corner"
                ),
        ]

        self.touch_count = touch_count
        self.finished = False

        # Storage point to calculate median
        self.med = [lv.point_t() for _ in range(0, touch_count)]

        self.curr_scrn = lv.screen_active()
        self.touch.enable(False)

        self.cur_point = 0
        self.cur_touch = 0
        self.scr = lv.obj(None)
        self.scr.add_style(style, 0)
        self.scr.set_size(width, height)
        lv.screen_load(self.scr)

        self.label_main = lv.label(self.scr)
        self.label_main.set_text('Touch Screen Calibration')
        self.label_main.center()

        if width > height:
            circle_size = int(height * 0.10)
        else:
            circle_size = int(width * 0.10)

        self.circ_area = lv.obj(self.scr)
        self.circ_area.add_style(style, 0)
        self.circ_area.set_size(circle_size, circle_size)
        self.circ_area.set_style_radius(lv.RADIUS_CIRCLE, 0)
        self.circ_area.remove_flag(lv.obj.FLAG.CLICKABLE)

        self.show_circle()

    def show_text(self, txt):
        self.label_main.set_text(txt)
        self.label_main.center()

    def show_circle(self):
        point = self.points[self.cur_point]

        self.circ_area.set_pos(
            point.point.x - self.circ_area.get_width() // 2,
            point.point.y - self.circ_area.get_height() // 2
        )
        self.circ_area.set_style_bg_color(lv.color_hex(0xFF0000), 0)

        while self.touch._get_coords() is not None:  # NOQA
            pass

    def calibrate_clicked(self, x, y):
        point = self.points[self.cur_point]

        self.med[self.cur_touch].x = x
        self.med[self.cur_touch].y = y

        self.cur_touch += 1

        self.show_text(
            "Click the circle in\n" +
            point.name + "\n" +
            "%d left" % (self.touch_count - self.cur_touch)
        )

        if self.cur_touch == self.touch_count:
            med_x = int(sum(med.x for med in self.med) // len(self.med))
            med_y = int(sum(med.y for med in self.med) // len(self.med))
            point.touch_coordinate = lv.point_t()
            point.touch_coordinate.x = med_x
            point.touch_coordinate.y = med_y

            self.cur_point += 1
            self.cur_touch = 0

            if self.cur_point != len(self.points):
                self.show_circle()

        if self.cur_point == len(self.points):
            self.show_text("DONE!")
            self.circ_area.set_style_bg_color(lv.color_hex(0xFF0000), 0)

            while self.touch._get_coords() is not None:  # NOQA
                pass

            left, top, right, bottom = self.calibrate(self.points)

            self.config.left = left
            self.config.top = top
            self.config.right = right
            self.config.bottom = bottom
            self.config.save()

            self.finished = True
            self.cur_point = 0
            self.show_text(
                "Click and drag on screen\n" +
                "to check calibration"
            )

            while self.touch._get_coords() is None:  # NOQA
                pass

            self.check()

    def check(self):
        data = Tpcal_point(0, 0, '')
        self.touch.read(None, data)

        released_count = 0

        while released_count != 5000:
            # print("click position: x: %d, y: %d"%(point.x,point.y))
            self.circ_area.set_pos(
                data.point.x - self.circ_area.get_width() // 2,
                data.point.y - self.circ_area.get_height() // 2
            )
            self.touch.read(None, data)
            if data.state == lv.STATE.PRESSED:
                released_count = 0
            else:
                released_count += 1

    def calibrate(self, points):
        for item in self.points:
            print(repr(item))

        width = self.touch.width
        height = self.touch.height

        t_width_min = width
        t_width_max = 0

        t_height_min = height
        t_height_max = 0

        v_height_min = height
        v_height_max = 0

        v_width_min = width
        v_width_max = 0

        orientation = self.touch.get_rotation()

        if orientation in (lv.DISPLAY_ROTATION._270, lv.DISPLAY_ROTATION._180):
            (
                points[0].point,
                points[3].point,
                points[1].point,
                points[2].point
            ) = (
                points[3].point,
                points[0].point,
                points[2].point,
                points[1].point
            )

        if orientation in (lv.DISPLAY_ROTATION._90, lv.DISPLAY_ROTATION._270):
            width, height = height, width
            (
                points[0].point.x,
                points[0].point.y,
                points[3].point.x,
                points[3].point.y,
                points[1].point.x,
                points[1].point.y,
                points[2].point.x,
                points[2].point.y
            ) = (
                points[0].point.y,
                points[0].point.x,
                points[3].point.y,
                points[3].point.x,
                points[2].point.y,
                points[2].point.x,
                points[1].point.y,
                points[1].point.x
            )

            (
                t_width_min,
                t_height_min,
                v_height_min,
                v_width_min
            ) = (
                t_height_min,
                t_width_min,
                v_width_min,
                v_height_min
            )

        if orientation == lv.DISPLAY_ROTATION._270:
            (
                points[0].point,
                points[1].point,
                points[2].point,
                points[3].point
            ) = (
                points[1].point,
                points[3].point,
                points[0].point,
                points[2].point
            )

        for item in self.points:
            print(repr(item))

        for point in points:
            t_width_min = min(t_width_min, point.touch_coordinate.x)
            t_width_max = max(t_width_max, point.touch_coordinate.x)
            t_height_min = min(t_height_min, point.touch_coordinate.y)
            t_height_max = max(t_height_max, point.touch_coordinate.y)

            v_width_min = min(v_width_min, point.point.x)
            v_width_max = max(v_width_max, point.point.x)
            v_height_min = min(v_height_min, point.point.y)
            v_height_max = max(v_height_max, point.point.y)

        v_height = v_height_max - v_width_min
        v_width = v_width_max - v_width_min

        t_height = t_height_max - t_width_min
        t_width = t_width_max - t_width_min

        pixel_width = int(t_width / v_width)
        pixel_height = int(t_height / v_height)

        print('v_height:', v_height)
        print('v_width:', v_width)
        print('t_height:', t_height)
        print('t_width:', t_width)

        print('pixel_width:', pixel_width)
        print('pixel_height:', pixel_height)

        p_1 = None
        p_2 = None

        for point in points:
            if point.point.x == v_width_min and point.point.y == v_height_min:
                p_1 = point
            if point.point.x == v_width_max and point.point.y == v_height_max:
                p_2 = point

        print(p_1)
        print(p_2)

        x0 = p_1.touch_coordinate.x - p_1.point.x * pixel_width
        y0 = p_1.touch_coordinate.y - p_1.point.y * pixel_height

        x1 = p_2.touch_coordinate.x + (width - p_2.point.x) * pixel_width
        y1 = p_2.touch_coordinate.y + (height - p_2.point.y) * pixel_height

        x0 = int(round(x0))
        y0 = int(round(y0))
        x1 = int(round(x1))
        y1 = int(round(y1))

        print(f"Calibration result: x0={x0}, y0={y0}, x1={x1}, y1={y1}")

        return x0, y0, x1, y1


# Run calibration
def run():
    tpcal = Tpcal()
    while tpcal.finished is False:
        try:
            coords = touch._get_coords()  # NOQA
            # time.sleep_us(5)
            if coords is None:
                tpcal.circ_area.set_style_bg_color(lv.color_hex(0xFF0000), 0)
                continue

            tpcal.circ_area.set_style_bg_color(lv.color_hex(0x00FF00), 0)
            tpcal.calibrate_clicked(*coords)
        except OSError:
            tpcal.circ_area.set_style_bg_color(lv.color_hex(0xFF0000), 0)

    lv.screen_load(tpcal.curr_scrn)
    tpcal.scr.delete()
    tpcal.touch.enable(True)

