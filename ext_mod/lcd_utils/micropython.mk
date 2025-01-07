# Copyright (c) 2024 - 2025 Kevin G. Schlosser

################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)

ifneq (,$(findstring -Wno-missing-field-initializers, $(CFLAGS_USERMOD)))
    CFLAGS_USERMOD += -Wno-missing-field-initializers
endif

SRC_USERMOD_C += $(MOD_DIR)/src/lcd_utils.c
SRC_USERMOD_C += $(MOD_DIR)/src/remap.c
SRC_USERMOD_C += $(MOD_DIR)/src/binary_float.c
