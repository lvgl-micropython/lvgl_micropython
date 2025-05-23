
# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# this driver uses a special i2c bus implimentation I have written.
# This implimentation takes into consideration the ESP32 and it having
# threading available. It also has some convience methods built into it
# that figure out what is wanting to be done automatically.
# read more about it's use in the stub files.

from micropython import const  # NOQA
import pointer_framework
import machine  # NOQA
import time

I2C_ADDR = 0x53
BITS = 8


class _tp_status_high_t:

    def __init__(self):
        self.none0 = 0
        self.none1 = 0
        self.none2 = 0
        self.cpu_run = 0
        self.tint_low = 0
        self.tic_in_cpu = 0
        self.tic_in_bios = 0
        self.tic_busy = 0


class _tp_status_low_t:
    def __init__(self):
        self.pt_exist = 0
        self.gesture = 0
        self.key = 0
        self.aux = 0
        self.keep = 0
        self.raw_or_pt = 0
        self.none6 = 0
        self.none7 = 0


class _tp_status_t:
    def __init__(self):
        self.status_low = _tp_status_low_t()
        self.status_high = _tp_status_high_t()
        self.read_len = 0


class _tp_report_t:
    def __init__(self):
        self.id = 0
        self.x = 0
        self.y = 0
        self.weight = 0


class _tp_touch_t:
    def __init__(self):
        self.rpt = [_tp_report_t() for _ in range(10)]
        self.touch_num = 0
        self.pack_code = 0
        self.down = 0
        self.up = 0
        self.gesture = 0
        self.down_x = 0
        self.down_y = 0
        self.up_x = 0
        self.up_y = 0


class _tp_hdp_status_t:
    def __init__(self):
        self.status = 0
        self.next_packet_len = 0


class SPD2010T(pointer_framework.PointerDriver):

    def __init__(
        self,
        device,
        reset_pin=None,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        self._tx_buf = bytearray(4)
        self._tx_mv = memoryview(self._tx_buf)
        self._rx_buf = bytearray(64)
        self._rx_mv = memoryview(self._rx_buf)

        self._device = device

        self.__x = 0
        self.__y = 0
        self.__last_state = self.RELEASED

        if isinstance(reset_pin, int):
            reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)

        self._reset_pin = reset_pin

        self.hw_reset()

        super().__init__(
            touch_cal=touch_cal, startup_rotation=startup_rotation, debug=debug
        )

    def hw_reset(self):
        if self._reset_pin:
            self._reset_pin(1)
            time.sleep_ms(2)  # NOQA
            self._reset_pin(0)
            time.sleep_ms(22)  # NOQA

    def __write_tp_point_mode_cmd(self):
        self._tx_buf[:4] = bytearray([0x50, 0x00, 0x00, 0x00])

        self._device.write(self._tx_mv[:4])
        time.sleep_us(200)  # NOQA

    def __write_tp_start_cmd(self):
        self._tx_buf[:4] = bytearray([0x46, 0x00, 0x00, 0x00])

        self._device.write(self._tx_mv[:4])
        time.sleep_us(200)  # NOQA

    def __write_tp_cpu_start_cmd(self):
        self._tx_buf[:4] = bytearray([0x04, 0x00, 0x01, 0x00])

        self._device.write(self._tx_mv[:4])
        time.sleep_us(200)  # NOQA

    def __write_tp_clear_int_cmd(self):
        self._tx_buf[:4] = bytearray([0x02, 0x00, 0x01, 0x00])

        self._device.write(self._tx_mv[:4])
        time.sleep_us(200)  # NOQA

    def __read_tp_status_length(self):
        tp_status = _tp_status_t()

        self._tx_buf[:2] = bytearray([0x20, 0x00])

        self._device.write(self._tx_mv[:2])
        time.sleep_us(200)  # NOQA

        self._device.read_into(self._rx_mv[:4])
        time.sleep_us(200)  # NOQA

        rx_buf = self._rx_buf

        tp_status.status_low.pt_exist = rx_buf[0] & 0x01
        tp_status.status_low.gesture = rx_buf[0] & 0x02
        tp_status.status_high.tic_busy = (rx_buf[1] & 0x80) >> 7
        tp_status.status_high.tic_in_bios = (rx_buf[1] & 0x40) >> 6
        tp_status.status_high.tic_in_cpu = (rx_buf[1] & 0x20) >> 5
        tp_status.status_high.tint_low = (rx_buf[1] & 0x10) >> 4
        tp_status.status_high.cpu_run = (rx_buf[1] & 0x08) >> 3
        tp_status.status_low.aux = rx_buf[0] & 0x08  # aux, cytang
        tp_status.read_len = (rx_buf[3] << 8) | rx_buf[2]

        return tp_status

    def __read_tp_hdp(self, tp_status, touch):
        self._tx_buf[:2] = bytearray([0x00, 0x03])
        self._device.write(self._tx_mv[:2])
        time.sleep_us(200)  # NOQA

        self._device.read_into(self._rx_mv[:tp_status.read_len])
        time.sleep_us(200)  # NOQA

        rx_buf = self._rx_buf

        check_id = rx_buf[4]

        if check_id <= 0x0A and tp_status.status_low.pt_exist:
            touch.touch_num = int((tp_status.read_len - 4) / 6)
            touch.gesture = 0x00

            for i in range(touch.touch_num):
                offset = i * 6
                touch.rpt[i].id = rx_buf[4 + offset]
                touch.rpt[i].x = ((rx_buf[7 + offset] & 0xF0) << 4) | rx_buf[5 + offset]
                touch.rpt[i].y = ((rx_buf[7 + offset] & 0x0F) << 8) | rx_buf[6 + offset]
                touch.rpt[i].weight = rx_buf[8 + offset]

            # For slide gesture recognize */
            if touch.rpt[0].weight != 0 and touch.down != 1:
                touch.down = 1
                touch.up = 0
                touch.down_x = touch.rpt[0].x
                touch.down_y = touch.rpt[0].y
            elif touch.rpt[0].weight == 0 and touch.down == 1:
                touch.up = 1
                touch.down = 0
                touch.up_x = touch.rpt[0].x
                touch.up_y = touch.rpt[0].y

        elif check_id == 0xF6 and tp_status.status_low.gesture:
            touch.touch_num = 0x00
            touch.up = 0
            touch.down = 0
            touch.gesture = rx_buf[6] & 0x07
        else:
            touch.touch_num = 0x00
            touch.gesture = 0x00

    def __read_tp_hdp_status(self):
        tp_hdp_status = _tp_hdp_status_t()

        self._tx_buf[:2] = bytearray([0xFC, 0x02])
        self._device.write(self._tx_mv[:2])
        time.sleep_us(200)  # NOQA

        self._device.read_into(self._rx_mv[:8])
        time.sleep_us(200)  # NOQA

        rx_buf = self._rx_buf
        tp_hdp_status.status = rx_buf[5]
        tp_hdp_status.next_packet_len = rx_buf[2] | (rx_buf[3] << 8)

        return tp_hdp_status

    def __Read_HDP_REMAIN_DATA(self, tp_hdp_status):
        self._tx_buf[:2] = bytearray([0x00, 0x03])
        self._device.write(self._tx_mv[:2])
        time.sleep_us(200)  # NOQA

        self._device.read_into(self._rx_mv[:tp_hdp_status.next_packet_len])
        time.sleep_us(200)  # NOQA

    def __read_fw_version(self):
        self._tx_buf[:2] = bytearray([0x26, 0x00])
        self._device.write(self._tx_mv[:2])
        time.sleep_us(200)  # NOQA

        self._device.read_into(self._rx_mv[:18])
        time.sleep_us(200)  # NOQA

        rx_buf = self._rx_buf

        Dummy = (rx_buf[0] << 24) | (rx_buf[1] << 16) | (rx_buf[3] << 8) | rx_buf[0]
        DVer = (rx_buf[5] << 8) | rx_buf[4]
        PID = (rx_buf[9] << 24) | (rx_buf[8] << 16) | (rx_buf[7] << 8) | rx_buf[6]
        ICName_L = (rx_buf[13] << 24) | (rx_buf[12] << 16) | (rx_buf[11] << 8) | rx_buf[10]  # "2010"
        ICName_H = (rx_buf[17] << 24) | (rx_buf[16] << 16) | (rx_buf[15] << 8) | rx_buf[14]  # "SPD"

        print(Dummy, DVer, PID, ICName_H, ICName_L)

    def __tp_read_data(self):
        touch = _tp_touch_t()

        tp_status = self.__read_tp_status_length()

        if tp_status.status_high.tic_in_bios:
            # Write Clear TINT Command
            self.__write_tp_clear_int_cmd()

            # Write CPU Start Command
            self.__write_tp_cpu_start_cmd()

        elif tp_status.status_high.tic_in_cpu:
            # Write Touch Change Command
            self.__write_tp_point_mode_cmd()

            # Write Touch Start Command
            self.__write_tp_start_cmd()

            # Write Clear TINT Command
            self.__write_tp_clear_int_cmd()

        elif tp_status.status_high.cpu_run and tp_status.read_len == 0:
            self.__write_tp_clear_int_cmd()
        elif tp_status.status_low.pt_exist or tp_status.status_low.gesture:
            # Read HDP
            self.__read_tp_hdp(tp_status, touch)

            def hdp_done_check():
                # Read HDP Status
                tp_hdp_status = self.__read_tp_hdp_status()

                if tp_hdp_status.status == 0x82:
                    # Clear INT
                    self.__write_tp_clear_int_cmd()
                elif tp_hdp_status.status == 0x00:
                    # Read HDP Remain Data
                    self.__Read_HDP_REMAIN_DATA(tp_hdp_status)
                    hdp_done_check()

            hdp_done_check()

        elif tp_status.status_high.cpu_run and tp_status.status_low.aux:
            self.__write_tp_clear_int_cmd()

        return touch

    def _get_coords(self):
        touch = self.__tp_read_data()

        # Expect Number of touched points */
        touch_cnt = touch.touch_num

        if touch_cnt:
            coords = touch.rpt[0]
            self.__x = coords.x
            self.__y = coords.y
            if coords.weight:
                self.__last_state = self.PRESSED
            else:
                self.__last_state = self.RELEASED

        else:
            self.__last_state = self.RELEASED

        return self.__last_state, self.__x, self.__y
