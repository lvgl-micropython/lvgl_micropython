
# this class is used as a template for writing the mechanism that is
# used to store the touch screen calibration data. All properties and functions
# seen in this class need to exist in the class that is written to store the
# touch calibration data.

# For more information on how to do this see the touch calibration for the ESP32

class TouchCalData(object):

    def __init__(self, name):
        self.name = name
        self._left = None
        self._right = None
        self._top = None
        self._bottom = None
        self._is_dirty = False

    def save(self):
        self._is_dirty = False

    @property
    def left(self):
        return self._left

    @left.setter
    def left(self, value):
        self._left = value
        self._is_dirty = True

    @property
    def right(self):
        return self._right

    @right.setter
    def right(self, value):
        self._right = value
        self._is_dirty = True

    @property
    def top(self):
        return self._top

    @top.setter
    def top(self, value):
        self._top = value
        self._is_dirty = True

    @property
    def bottom(self):
        return self._bottom

    @bottom.setter
    def bottom(self, value):
        self._bottom = value
        self._is_dirty = True

    def reset(self):
        self.save()
