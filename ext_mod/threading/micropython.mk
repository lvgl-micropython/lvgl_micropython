
################################################################################
# threading build rules

ifneq (,$(findstring unix, $(LV_PORT)))
    MOD_DIR := $(USERMOD_DIR)

    CFLAGS_USERMOD += -I$(MOD_DIR)/unix
    CFLAGS_USERMOD += -I$(MOD_DIR)/common/inc

    SRC_USERMOD_C += $(MOD_DIR)/common/src/multiprocessing.c
    SRC_USERMOD_C += $(MOD_DIR)/common/src/thread_event.c
    SRC_USERMOD_C += $(MOD_DIR)/common/src/thread_lock.c
    SRC_USERMOD_C += $(MOD_DIR)/common/src/thread_rlock.c
    SRC_USERMOD_C += $(MOD_DIR)/common/src/thread_semaphore.c
    SRC_USERMOD_C += $(MOD_DIR)/common/src/thread_thread.c
    SRC_USERMOD_C += $(MOD_DIR)/common/src/threading.c
    SRC_USERMOD_C += $(MOD_DIR)/unix/thread_port.c

endif
