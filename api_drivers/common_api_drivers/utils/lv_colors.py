import lvgl as lv


def LV_COLOR_MAKE(r=None, g=None, b=None, rgb=None):
    if rgb is not None:
        r = (rgb >> 16) & 0xFF
        g = (rgb >> 8) & 0xFF
        b = rgb & 0xFF

    return lv.color_make(r, g, b)


class _LV_COLORS:

    def __getattr__(self, item):
        if item in self.__dict__:
            return self.__dict__[item]

        mapping = dict(
            WHITE=0xFFFFFF,
            SILVER=0xC0C0C0,
            GRAY=0x808080,
            BLACK=0x000000,
            RED=0xFF0000,
            MAROON=0x800000,
            YELLOW=0xFFFF00,
            OLIVE=0x808000,
            LIME=0x00FF00,
            GREEN=0x008000,
            CYAN=0x00FFFF,
            TEAL=0x008080,
            BLUE=0x0000FF,
            NAVY=0x000080,
            MAGENTA=0xFF00FF,
            PURPLE=0x800080,
            ORANGE=0xFFA500
        )
        mapping['AQUA'] = mapping['CYAN']

        if item in mapping:
            return LV_COLOR_MAKE(rgb=mapping[item])

        raise AttributeError(item)
