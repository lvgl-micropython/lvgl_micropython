import nvs  # NOQA


class TouchCalData(object):

    def __init__(self, name):
        self._config = nvs.NVS(name)

        try:
            self._alphaX = self._config.get(nvs.TYPE_FLOAT, 'alphaX')
        except OSError:
            self._alphaX = None
        try:
            self._betaX = self._config.get(nvs.TYPE_FLOAT, 'betaX')
        except OSError:
            self._betaX = None

        try:
            self._deltaX = self._config.get(nvs.TYPE_FLOAT, 'deltaX')
        except OSError:
            self._deltaX = None
        try:
            self._alphaY = self._config.get(nvs.TYPE_FLOAT, 'alphaY')
        except OSError:
            self._alphaY = None

        try:
            self._betaY = self._config.get(nvs.TYPE_FLOAT, 'betaY')
        except OSError:
            self._betaY = None

        try:
            self._deltaY = self._config.get(nvs.TYPE_FLOAT, 'deltaY')
        except OSError:
            self._deltaY = None

        self._is_dirty = False

    def save(self):
        if self._is_dirty:
            if self._alphaX is None:
                self._config.erase('alphaX')
            if self._betaX is None:
                self._config.erase('betaX')
            if self._deltaX is None:
                self._config.erase('deltaX')
            if self._alphaY is None:
                self._config.erase('alphaY')
            if self._betaY is None:
                self._config.erase('betaY')
            if self._deltaY is None:
                self._config.erase('deltaY')

            self._config.commit()

    @property
    def alphaX(self):
        return self._alphaX

    @alphaX.setter
    def alphaX(self, value):
        self._alphaX = value
        if value is not None:
            self._config.set(nvs.TYPE_FLOAT, 'alphaX', value)
            self._is_dirty = True

    @property
    def betaX(self):
        return self._betaX

    @betaX.setter
    def betaX(self, value):
        self._betaX = value
        if value is not None:
            self._config.set(nvs.TYPE_FLOAT, 'betaX', value)
            self._is_dirty = True

    @property
    def deltaX(self):
        return self._deltaX

    @deltaX.setter
    def deltaX(self, value):
        self._deltaX = value
        if value is not None:
            self._config.set(nvs.TYPE_FLOAT, 'deltaX', value)
            self._is_dirty = True

    @property
    def alphaY(self):
        return self._alphaY

    @alphaY.setter
    def alphaY(self, value):
        self._alphaY = value
        if value is not None:
            self._config.set(nvs.TYPE_FLOAT, 'alphaY', value)
            self._is_dirty = True

    @property
    def betaY(self):
        return self._betaY

    @betaY.setter
    def betaY(self, value):
        self._betaY = value
        if value is not None:
            self._config.set(nvs.TYPE_FLOAT, 'betaY', value)
            self._is_dirty = True

    @property
    def deltaY(self):
        return self._deltaY

    @deltaY.setter
    def deltaY(self, value):
        self._deltaY = value
        if value is not None:
            self._config.set(nvs.TYPE_FLOAT, 'deltaY', value)
            self._is_dirty = True

    def reset(self):
        self.alphaX = None
        self.betaX = None
        self.deltaX = None
        self.alphaY = None
        self.betaY = None
        self.deltaY = None
        self.save()

