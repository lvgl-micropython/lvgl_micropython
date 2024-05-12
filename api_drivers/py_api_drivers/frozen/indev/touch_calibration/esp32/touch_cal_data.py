import esp32  # NOQA


class TouchCalData(object):

    def __init__(self, name):
        self._config = esp32.NVS(name)

        try:
            self._left = self._config.get_i32('left')
        except OSError:
            self._left = -0x80000000
        try:
            self._right = self._config.get_i32('right')
        except OSError:
            self._right = -0x80000000

        try:
            self._top = self._config.get_i32('top')
        except OSError:
            self._top = -0x80000000
        try:
            self._bottom = self._config.get_i32('bottom')
        except OSError:
            self._bottom = -0x80000000

        self._is_dirty = False

    def save(self):
        if self._is_dirty:
            self._config.commit()

    @property
    def left(self):
        if self._left == -0x80000000:
            return None

        return self._left

    @left.setter
    def left(self, value):
        if value is None:
            value = -0x80000000

        self._left = value
        self._config.set_i32('left', value)
        self._is_dirty = True

    @property
    def right(self):
        if self._right == -0x80000000:
            return None

        return self._right

    @right.setter
    def right(self, value):
        if value is None:
            value = -0x80000000

        self._right = value
        self._config.set_i32('right', value)
        self._is_dirty = True

    @property
    def top(self):
        if self._top == -0x80000000:
            return None

        return self._top

    @top.setter
    def top(self, value):
        if value is None:
            value = -0x80000000

        self._top = value
        self._config.set_i32('top', value)
        self._is_dirty = True

    @property
    def bottom(self):
        if self._bottom == -0x80000000:
            return None

        return self._bottom

    @bottom.setter
    def bottom(self, value):
        if value is None:
            value = 0x80000000

        self._bottom = value
        self._config.set_i32('bottom', value)
        self._is_dirty = True

    def reset(self):
        self.top = None
        self.bottom = None
        self.left = None
        self.right = None
        self.save()

