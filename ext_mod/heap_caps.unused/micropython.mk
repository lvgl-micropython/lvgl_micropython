################################################################################
# heap_caps build rules

MOD_DIR := $(USERMOD_DIR)

CFLAGS_USERMOD += -I$(MOD_DIR)
SRC_USERMOD_C += $(MOD_DIR)/common_src/heap_caps.c
