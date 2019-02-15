GOSSIP_ENABLE_BACKTRACE = 1

DIR := src/common/gossip
LIBSRC += $(DIR)/gossip.c
SERVERSRC += $(DIR)/gossip.c
LIBBMISRC += $(DIR)/gossip.c
ifdef GOSSIP_ENABLE_BACKTRACE
MODCFLAGS_$(DIR)/gossip.c := -DGOSSIP_ENABLE_BACKTRACE
endif
