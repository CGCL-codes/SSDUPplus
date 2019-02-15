

ifneq (,$(BUILD_TAU))

DIR := src/common/events
files := pvfs_tau_api.c fmt_fsm.c

src := $(patsubst %,$(DIR)/%,$(files))

LIBSRC += $(src)
SERVERSRC += $(src)
LIBBMISRC += $(src)

ifneq (,)
MODCFLAGS_$(DIR)/fmt_fsm.c := -x c++ 
MODCFLAGS_$(DIR)/pvfs_tau_api.c := -x c++ 
endif

endif # BUILD_TAU
