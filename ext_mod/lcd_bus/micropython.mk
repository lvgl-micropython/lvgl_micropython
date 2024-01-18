
################################################################################
# lcd_bus build rules

MOD_DIR := $(USERMOD_DIR)

CFLAGS_USERMOD += -I$(MOD_DIR)
CFLAGS_USERMOD += -I$(MOD_DIR)/common_include

SRC_USERMOD_C += $(MOD_DIR)/modlcd_bus.c
SRC_USERMOD_C += $(MOD_DIR)/lcd_types.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/i2c_bus.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/i80_bus.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/spi_bus.c
SRC_USERMOD_C += $(MOD_DIR)/common_src/rgb_bus.c
