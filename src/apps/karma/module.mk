ifdef BUILD_KARMA

DIR := src/apps/karma

KARMASRC += \
    $(DIR)/color.c \
    $(DIR)/comm.c \
    $(DIR)/details.c \
    $(DIR)/fsview.c \
    $(DIR)/karma.c \
    $(DIR)/menu.c \
    $(DIR)/messages.c \
    $(DIR)/prep.c \
    $(DIR)/status.c \
    $(DIR)/traffic.c \
    $(DIR)/units.c

KARMA := $(DIR)/karma

# add GTK include dirs and libs
MODCFLAGS_$(DIR) := -pthread -I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12  
MODLDFLAGS_$(DIR) := -pthread -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0  

# gtk-2.0 has many bad prototypes
ifdef GNUC
MODCFLAGS_$(DIR) += -Wno-strict-prototypes
endif

endif  # BUILD_KARMA
