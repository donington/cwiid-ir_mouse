#Copyright (C) 2007 L. Donnie Smith

include ../../../defs.mak

PLUGIN_NAME = ir_mouse
SOURCES = ir_mouse.c
CFLAGS += -I../../../wminput -I../../../libcwiid
#LDLIBS += -lm
INST_DIR = $(CWIID_PLUGINS_DIR)

include $(COMMON)/include/plugin.mak

distclean: clean
	rm Makefile

.PHONY: distclean
