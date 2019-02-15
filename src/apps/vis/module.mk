
ifdef BUILD_VIS

DIR := src/apps/vis

VISSRC += \
    $(DIR)/simple.c \
    $(DIR)/pvfs2-vis-bw-2d.c

VISMISCSRC += \
    $(DIR)/pvfs2-vis.c

# add SDL include dirs and libs
MODCFLAGS_$(DIR) := 
MODLDFLAGS_$(DIR) :=  -lSDL_ttf

endif  # BUILD_VIS
