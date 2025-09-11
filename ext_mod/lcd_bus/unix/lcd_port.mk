# Copyright (c) 2024 - 2025 Kevin G. Schlosser

CFLAGS_USERMOD += -I$(BUILD)/SDL/include/SDL2
CFLAGS_USERMOD += -I$(BUILD)/SDL/include-config-release/SDL2
CFLAGS_USERMOD += -Wno-discarded-qualifiers
LDFLAGS_USERMOD += -L$(BUILD)/SDL
LDFLAGS_USERMOD += -lSDL2