# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# Makefile for LVGL_Micropython Binding

PYTHON_CMD=python3

# User-friendly check for python3
ifeq ($(shell which $(PYTHON_CMD) >/dev/null 2>&1; echo $$?), 1)
    $(error The '$(PYTHON)' command was not found. Make sure you have Python 3.8.x+)
endif

variables=$(foreach v,$(.VARIABLES),$(if $(filter command line,$(origin $(v))),$(v)=$($(v))))
BUILD_CMD=$(shell echo "${variables}" | sed 's/[ ][ ]*/ /g')

arguments = $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))

target =$@

PYTHON_CMD += make.py

ifneq (,$(findstring clean ,$(arguments)))
    args=$(subst clean ,,$(arguments))
    clean_cmd =clean
else
    args=$(arguments)
    clean_cmd =
endif

BUILD_CMD+=$(foreach v,$(args),--$(v))

.ONESHELL:

esp32:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

stm32:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

unix:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

rp2:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

macOS:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

renesas-ra:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

nrf:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

mimxrt:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

samd:
	@$(PYTHON_CMD) $@ $(clean_cmd) $(BUILD_CMD)
	@exit 0

%:
	@exit 0
