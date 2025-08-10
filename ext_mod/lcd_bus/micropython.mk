# Copyright (c) 2024 - 2025 Kevin G. Schlosser

################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)

PORT_DIR = $(MOD_DIR)/$(LV_PORT)

ifneq (,$(findstring macOS, $(LV_PORT)))
    # -F/Library/Frameworks -framework SDL2
    CFLAGS_USERMOD += -DMP_PORT_UNIX=1
    CFLAGS_USERMOD += -Wno-incompatible-pointer-types-discards-qualifiers
    CFLAGS_USERMOD += -I/Library/Frameworks/SDL2.framework/Headers/SDL2
    CFLAGS_USERMOD += $(shell sdl2-config --cflags)
    LDFLAGS_USERMOD += $(shell sdl2-config --libs)
    PORT_DIR = $(MOD_DIR)/unix
endif

ifneq (,$(findstring unix, $(LV_PORT)))
    CFLAGS_USERMOD += -DMP_PORT_UNIX=1
    CFLAGS_USERMOD += -I$(BUILD)/SDL/include/SDL2
    CFLAGS_USERMOD += -I$(BUILD)/SDL/include-config-release/SDL2
    CFLAGS_USERMOD += -Wno-discarded-qualifiers
    LDFLAGS_USERMOD += -L$(BUILD)/SDL
    LDFLAGS_USERMOD += -lSDL2
endif


ifneq (,$(findstring -Wno-missing-field-initializers, $(CFLAGS_USERMOD)))
    CFLAGS_USERMOD += -Wno-missing-field-initializers
endif


CFLAGS_USERMOD += -I$(MOD_DIR)
CFLAGS_USERMOD += -I$(PORT_DIR)/include

SRC_USERMOD_C += $(MOD_DIR)/modlcd_bus.c
SRC_USERMOD_C += $(MOD_DIR)/lcd_types.c
SRC_USERMOD_C += $(PORT_DIR)/src/i2c_bus.c
SRC_USERMOD_C += $(PORT_DIR)/src/i80_bus.c
SRC_USERMOD_C += $(PORT_DIR)/src/spi_bus.c
SRC_USERMOD_C += $(PORT_DIR)/src/rgb_bus.c
SRC_USERMOD_C += $(PORT_DIR)/src/sdl_bus.c
SRC_USERMOD_C += $(PORT_DIR)/src/dsi_bus.c
