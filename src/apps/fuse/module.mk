ifdef BUILD_FUSE

DIR := src/apps/fuse

FUSESRC += \
    $(DIR)/pvfs2fuse.c

FUSE := $(DIR)/pvfs2fuse

MODCFLAGS_$(DIR) := 
MODLDFLAGS_$(DIR) := 

endif  # BUILD_FUSE
