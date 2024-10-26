# Copyright (C) 2024  Kevin G Schlosser
# Code that is written by the above named is done under the GPL license
# and that license is able to be viewed in the LICENSE file in the root
# of this project.

################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)

ifneq (,$(findstring -Wno-missing-field-initializers, $(CFLAGS_USERMOD)))
    CFLAGS_USERMOD += -Wno-missing-field-initializers
endif

SRC_USERMOD_C += $(MOD_DIR)/src/lcd_utils.c
SRC_USERMOD_C += $(MOD_DIR)/src/remap.c
SRC_USERMOD_C += $(MOD_DIR)/src/binary_float.c
