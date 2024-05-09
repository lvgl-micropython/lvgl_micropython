
################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)
LVGL_BINDING_DIR = $(subst /ext_mod/lcd_bus,,$(MOD_DIR))

CFLAGS_USERMOD += -I$(MOD_DIR)
CFLAGS_USERMOD += -I$(MOD_DIR)/common_include
CFLAGS_USERMOD += -I$(MOD_DIR)/sdl_bus

ifneq (,$(findstring -Wno-missing-field-initializers, $(CFLAGS_USERMOD)))
    CFLAGS_USERMOD += -Wno-missing-field-initializers
endif

SRC_USERMOD_C += $(MOD_DIR)/modlcd_bus.c
SRC_USERMOD_C += $(MOD_DIR)/lcd_types.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/i2c_bus.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/i80_bus.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/spi_bus.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/rgb_bus.c
SRC_USERMOD_C += $(MOD_DIR)/sdl_bus/sdl_bus.c

ifneq (,$(findstring unix, $(LV_PORT)))
    CFLAGS_USERMOD += -DMP_PORT_UNIX=1
    CFLAGS_USERMOD += -I$(BUILD)/SDL/include/SDL2
    CFLAGS_USERMOD += -I$(BUILD)/SDL/include-config-release/SDL2
    LDFLAGS_USERMOD += -L$(BUILD)/SDL

    ifeq ($(UNAME_S),Darwin)
        LDFLAGS_USERMOD += -lSDL2-2.0
    else
        LDFLAGS_USERMOD += -lSDL2
    endif
endif
