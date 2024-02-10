
################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)

CFLAGS_USERMOD += -I$(MOD_DIR)
CFLAGS_USERMOD += -I$(MOD_DIR)/include

SRC_USERMOD_C += $(MOD_DIR)/modsdl.c
SRC_USERMOD_C += $(MOD_DIR)/src/keyboard.c
SRC_USERMOD_C += $(MOD_DIR)/src/mouse.c
SRC_USERMOD_C += $(MOD_DIR)/src/mousewheel.c
SRC_USERMOD_C += $(MOD_DIR)/src/display.c
