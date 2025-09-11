# Copyright (c) 2024 - 2025 Kevin G. Schlosser

################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)
LVGL_BINDING_DIR = $(subst /ext_mod/lcd_bus,,$(MOD_DIR))


ifneq (,$(findstring -Wno-missing-field-initializers, $(CFLAGS_USERMOD)))
    CFLAGS_USERMOD += -Wno-missing-field-initializers
endif

CFLAGS_USERMOD += -I$(MOD_DIR)/common/include
CFLAGS_USERMOD += -I$(MOD_DIR)/$(LV_PORT)/include

SRC_USERMOD_C += $(MOD_DIR)/common/src/modlcd_bus.c
SRC_USERMOD_C += $(MOD_DIR)/common/src/lcd_types.c
SRC_USERMOD_C += $(wildcard $(MOD_DIR)/$(LV_PORT)/src/*.c)

include $(MOD_DIR)/$(LV_PORT)/lcd_port.mk
