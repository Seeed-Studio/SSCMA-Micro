SHELL = /bin/bash
SRCTREE := $(CURDIR)
TARGET_OUT_DIR := $(CURDIR)/out
TOPTARGETS := all clean install
TOPSUBDIRS := recamera ipcamera

## setup path ##
ifeq ($(findstring $(CHIP_ARCH), CV182X CV183X MARS PHOBOS CV180X CV181X), )
	$(error UNKNOWN chip series - $(CHIP_ARCH))
endif

TMP_STRING = $(foreach f,$(TOPSUBDIRS),$(findstring $f,$(MAKECMDGOALS)))
PROJECT = $(strip $(TMP_STRING))
export SRCTREE PROJECT TARGET_OUT_DIR

$(PROJECT):
	$(MAKE) -C solutions/$@ $(strip $(subst $@,,$(MAKECMDGOALS)))


$(TOPSUBDIRS): $(PROJECT)

$(TOPTARGETS): $(TOPSUBDIRS)

.PHONY: $(TOPTARGETS) $(TOPSUBDIRS)