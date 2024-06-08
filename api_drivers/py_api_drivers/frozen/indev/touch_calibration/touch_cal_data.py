
# this class is used as a template for writing the mechanism that is
# used to store the touch screen calibration data. All properties and functions
# seen in this class need to exist in the class that is written to store the
# touch calibration data.

# For more information on how to do this see the touch calibration for the ESP32

class TouchCalData(object):

    def __init__(self, name):
        self.name = name
        self._alphaX = None
        self._betaX = None
        self._deltaX = None
        self._alphaY = None
        self._betaY = None
        self._deltaY = None

        self._is_dirty = False

    def save(self):
        self._is_dirty = False

    @property
    def alphaX(self):
        return self._alphaX

    @alphaX.setter
    def alphaX(self, value):
        self._alphaX = value
        self._is_dirty = True

    @property
    def betaX(self):
        return self._betaX

    @betaX.setter
    def betaX(self, value):
        self._betaX = value
        self._is_dirty = True

    @property
    def deltaX(self):
        return self._deltaX

    @deltaX.setter
    def deltaX(self, value):
        self._deltaX = value
        self._is_dirty = True

    @property
    def alphaY(self):
        return self._alphaY

    @alphaY.setter
    def alphaY(self, value):
        self._alphaY = value
        self._is_dirty = True

    @property
    def betaY(self):
        return self._betaY

    @betaY.setter
    def betaY(self, value):
        self._betaY = value
        self._is_dirty = True

    @property
    def deltaY(self):
        return self._deltaY

    @deltaY.setter
    def deltaY(self, value):
        self._deltaY = value
        self._is_dirty = True

    def reset(self):
        self.save()
