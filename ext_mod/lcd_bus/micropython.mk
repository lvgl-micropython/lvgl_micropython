# Copyright (c) 2024 - 2025 Kevin G. Schlosser

################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)
LVGL_BINDING_DIR = $(subst /ext_mod/lcd_bus,,$(MOD_DIR))


CFLAGS_USERMOD += -I$(MOD_DIR)/include

SRC_USERMOD_C += $(MOD_DIR)/src/common/lcd_framebuf.c
SRC_USERMOD_C += $(MOD_DIR)/src/common/modlcd_bus.c
SRC_USERMOD_C += $(MOD_DIR)/src/common/rgb565_dither.c
SRC_USERMOD_C += $(MOD_DIR)/src/common/sw_rotate.c

ifneq (,$(findstring -Wno-missing-field-initializers, $(CFLAGS_USERMOD)))
    CFLAGS_USERMOD += -Wno-missing-field-initializers
endif


ifneq (,$(findstring unix, $(LV_PORT)))
    CFLAGS_USERMOD += -I$(MOD_DIR)/include/posix

    CFLAGS_USERMOD += -DMP_PORT_UNIX=1
    CFLAGS_USERMOD += -I$(BUILD)/SDL/include/SDL2
    CFLAGS_USERMOD += -I$(BUILD)/SDL/include-config-release/SDL2
    CFLAGS_USERMOD += -Wno-discarded-qualifiers

    LDFLAGS_USERMOD += -L$(BUILD)/SDL
    LDFLAGS_USERMOD += -lSDL2

    SRC_USERMOD_C += $(MOD_DIR)/src/posix/i2c_bus.c
    SRC_USERMOD_C += $(MOD_DIR)/src/posix/spi_bus.c
    SRC_USERMOD_C += $(MOD_DIR)/src/posix/i80_bus.c
    SRC_USERMOD_C += $(MOD_DIR)/src/posix/rgb_bus.c
    SRC_USERMOD_C += $(MOD_DIR)/src/posix/lcd_bus_utils.c
    SRC_USERMOD_C += $(MOD_DIR)/src/posix/lcd_types.c
    SRC_USERMOD_C += $(MOD_DIR)/src/posix/sw_rotate_task.c
    SRC_USERMOD_C += $(MOD_DIR)/src/posix/sdl_bus.c
else
    ifneq (,$(findstring macOS, $(LV_PORT)))
        CFLAGS_USERMOD += -I$(MOD_DIR)/include/posix

        CFLAGS_USERMOD += -DMP_PORT_UNIX=1
        CFLAGS_USERMOD += -Wno-incompatible-pointer-types-discards-qualifiers
        CFLAGS_USERMOD += -I/Library/Frameworks/SDL2.framework/Headers/SDL2
        CFLAGS_USERMOD += $(shell sdl2-config --cflags)

    	LDFLAGS_USERMOD += $(shell sdl2-config --libs)

    	SRC_USERMOD_C += $(MOD_DIR)/src/posix/i2c_bus.c
        SRC_USERMOD_C += $(MOD_DIR)/src/posix/spi_bus.c
		SRC_USERMOD_C += $(MOD_DIR)/src/posix/i80_bus.c
		SRC_USERMOD_C += $(MOD_DIR)/src/posix/rgb_bus.c
		SRC_USERMOD_C += $(MOD_DIR)/src/posix/lcd_bus_utils.c
		SRC_USERMOD_C += $(MOD_DIR)/src/posix/lcd_types.c
		SRC_USERMOD_C += $(MOD_DIR)/src/posix/sw_rotate_task.c
		SRC_USERMOD_C += $(MOD_DIR)/src/posix/sdl_bus.c
	else
	    CFLAGS_USERMOD += -I$(MOD_DIR)/include/other_mcus

	    SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/i2c_bus.c
        SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/spi_bus.c
		SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/i80_bus.c
		SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/rgb_bus.c
		SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/lcd_bus_utils.c
		SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/lcd_types.c
		SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/sw_rotate_task.c
		SRC_USERMOD_C += $(MOD_DIR)/src/other_mcus/sdl_bus.c
    endif
endif
