# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# -F/Library/Frameworks -framework SDL2
CFLAGS_USERMOD += -Wno-incompatible-pointer-types-discards-qualifiers
CFLAGS_USERMOD += -I/Library/Frameworks/SDL2.framework/Headers/SDL2
CFLAGS_USERMOD += $(shell sdl2-config --cflags)
LDFLAGS_USERMOD += $(shell sdl2-config --libs)