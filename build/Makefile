#--------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
#--------------------------------------------------------------------------------
# Bootstrap Makefile for SCXCore
# 2007-08-23
#--------------------------------------------------------------------------------

# Set up architecture and Build Root Directory
# PF (i.e. PlatForm) is either linux, solaris
SHELL=/bin/bash
PWD:=$(shell pwd)
SCX_BRD=$(shell dirname $(PWD))
PF_POSIX=true

include $(SCX_BRD)/build/config.mak

BUILD_CONFIG_CACHE=$(SCX_BRD)/build/Makefile.config_cache

all : scxcore-status scxcorelib scxsystemlib util scxassertabort scxassertcppunit

scxcore-status:
	$(ECHO) "========================= Performing make SCXCore"

distclean: clean
	-$(RMDIR) $(SCX_BRD)/intermediate $(SCX_BRD)/target $(SCX_BRD)/build/config.mak

clean: clean-status doxygen_clean
	-$(RMDIR) $(SCX_BRD)/installer/intermediate
	-$(RMDIR) $(INTERMEDIATE_DIR)
	-$(RMDIR) $(TARGET_DIR)
	-find $(SCX_BRD) -name \*~ -exec rm {} \;

clean-status:
	$(ECHO) "========================= Performing make clean"

# Include the rest of the stuff we need

include $(SCX_BRD)/build/Makefile.pal
-include $(BUILD_CONFIG_CACHE)
include $(SCX_BRD)/build/Makefile.macros
include $(SCX_BRD)/build/Makefile.pf.$(PF)
include $(SCX_BRD)/build/Makefile.rules

include $(SCX_BRD)/build/Makefile.components

include $(SCX_BRD)/build/Makefile.tests
include $(SCX_BRD)/build/Makefile.stub

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifeq (,$(findstring cache,$(MAKECMDGOALS)))
-include $(DEPFILES) $(DEPFILES_TEST)
endif
endif


#-------------------------------- End of File -----------------------------------
