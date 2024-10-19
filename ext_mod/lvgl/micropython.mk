
################################################################################
# LVGL build rules


MOD_DIR := $(USERMOD_DIR)
LVGL_BINDING_DIR = $(subst /ext_mod/lvgl,,$(MOD_DIR))

LIB_DIR = $(LVGL_BINDING_DIR)/lib
LVGL_DIR = $(LVGL_BINDING_DIR)/lib/lvgl
LVGL_HEADER = $(LVGL_BINDING_DIR)/build/lvgl_header.h

FAKE_LIBC_PATH = $(LVGL_BINDING_DIR)/gen/fake_libc

CURRENT_DIR = $(LVGL_BINDING_DIR)/ext_mod/lvgl
CFLAGS_USERMOD += -I$(LVGL_DIR)
CFLAGS_USERMOD += -I$(LIB_DIR)

LV_PP_CFLAGS = $(CFLAGS_USERMOD)
LV_PP_CFLAGS += -std=c11 -E -I$(FAKE_LIBC_PATH) -DPYCPARSER


ifdef $(LV_CFLAGS)
    CFLAGS_USERMOD += $(LV_CFLAGS)
    LV_PP_CFLAGS += $(LV_CFLAGS)
endif

ifneq (,$(findstring -Wno-missing-field-initializers, $(CFLAGS_USERMOD)))
    CFLAGS_USERMOD += -Wno-missing-field-initializers
endif


ALL_LVGL_SRC = $(shell find $(LVGL_DIR) -type f -name '*.h') $(LVGL_BINDING_DIR)/lib/lv_conf.h

LVGL_MPY = $(BUILD)/lv_mpy.c
LVGL_MPY_PP = $(BUILD)/lv_mpy.pp
LVGL_MPY_METADATA = $(BUILD)/lv_mpy.json

SRC_USERMOD_LIB_C += $(shell find $(LVGL_DIR)/src -type f -name "*.c")
SRC_USERMOD_LIB_C += $(CURRENT_DIR)/mem_core.c

SRC_USERMOD_C += $(LVGL_MPY)

$(LVGL_MPY): $(ALL_LVGL_SRC) $(LVGL_BINDING_DIR)/gen/$(GEN_SCRIPT)_api_gen_mpy.py
	$(ECHO) "LVGL-GEN $@"
	$(Q)mkdir -p $(dir $@)

	$(Q)$(CC) $(LV_PP_CFLAGS) $(LVGL_HEADER) >> $(LVGL_MPY_PP)

	$(Q)$(PYTHON) $(LVGL_BINDING_DIR)/gen/$(GEN_SCRIPT)_api_gen_mpy.py --cflags="$(LV_PP_CFLAGS)" --pp_file=$(LVGL_MPY_PP) --board=$(LV_PORT) --output=$(LVGL_MPY)  --module_name=lvgl --module_prefix=lv --metadata=$(LVGL_MPY_METADATA) --header_file=$(LVGL_HEADER)

.PHONY: LVGL_MPY
LVGL_MPY: $(LVGL_MPY)

