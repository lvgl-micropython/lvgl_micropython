
################################################################################
# LVGL build rules

$(info    running ext_mod/lvgl/micropython.mk)

MOD_DIR := $(USERMOD_DIR)

LVGL_BINDING_DIR = $(subst /ext_mod/lvgl,,$(MOD_DIR))
LVGL_DIR = $(LVGL_BINDING_DIR)/lib/lvgl
LVGL_HEADER = $(LVGL_BINDING_DIR)/build/lvgl_header.h


CFLAGS_USERMOD += -I$(LVGL_BINDING_DIR)
CFLAGS_USERMOD += -I$(LVGL_DIR)
CFLAGS_USERMOD += -I$(LVGL_BINDING_DIR)/include

ALL_LVGL_SRC = $(shell find $(LVGL_DIR) -type f -name '*.h') $(LVGL_BINDING_DIR)/lib/lv_conf.h

LVGL_MPY = $(BUILD)/lv_mpy.c
LVGL_MPY_METADATA = $(BUILD)/lv_mpy.json

CFLAGS_USERMOD += $(LV_CFLAGS) 

$(LVGL_MPY): $(ALL_LVGL_SRC) $(LVGL_BINDING_DIR)/gen/gen_mpy.py 
	$(ECHO) "LVGL-GEN $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(PYTHON) $(LVGL_BINDING_DIR)/gen/gen_mpy.py $(LV_CFLAGS) --board=$(LV_PORT) --output=$(LVGL_MPY) --include=$(LVGL_BINDING_DIR) --include=$(LVGL_DIR) --include=$(LVGL_BINDING_DIR)/include --module_name=lvgl --module_prefix=lv --metadata=$(LVGL_MPY_METADATA) $(LVGL_HEADER)

.PHONY: LVGL_MPY
LVGL_MPY: $(LVGL_MPY)

CFLAGS_USERMOD += -Wno-unused-function
SRC_USERMOD_LIB_C += $(shell find $(LVGL_DIR)/src -type f -name "*.c")

SRC_USERMOD_C += $(LVGL_MPY)

ifneq (,$(findstring MICROPY_SDL=1, $(LV_CFLAGS)))
    LDFLAGS_USERMOD += -lSDL2
endif
