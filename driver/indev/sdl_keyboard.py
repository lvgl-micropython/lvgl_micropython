self.keyboard = lv.sdl_keyboard_create()

self.group = lv.group_create()
self.group.set_default()
self.keyboard.set_group(self.group)

self.set_default()