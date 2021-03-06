#
# Makefile stub for bmi_mx.
#
# Copyright (C) 2008 Pete Wyckoff <pw@osc.edu>
#
# See COPYING in top-level directory.
#

# only do any of this if configure decided to use MX
ifneq (,$(BUILD_MX))

#
# Local definitions.
#
DIR := src/io/bmi/bmi_mx
cfiles := mx.c

#
# Export these to the top Makefile to tell it what to build.
#
src := $(patsubst %,$(DIR)/%,$(cfiles))
LIBSRC    += $(src)
SERVERSRC += $(src)
LIBBMISRC += $(src)

#
# Extra cflags for files in this directory.
#
MODCFLAGS_$(DIR) := -I 

endif  # BUILD_MX
