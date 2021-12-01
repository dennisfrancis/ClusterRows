#*************************************************************************
#
#  The Contents of this file are made available subject to the terms of
#  the BSD license.
#
#  Copyright 2000, 2010 Oracle and/or its affiliates.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of Sun Microsystems, Inc. nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
#  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
#  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
#  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#**************************************************************************

ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
WORKING_DIR := $(shell pwd)

ifneq (${ROOT_DIR}, ${WORKING_DIR})
$(error Run make from the directory of the Makefile.)
endif

PRJ=$(OO_SDK_HOME)
SETTINGS=$(PRJ)/settings

include $(SETTINGS)/settings.mk
include $(SETTINGS)/std.mk

CLUSTERROWS_VERSION = 1.0.3

# Define non-platform/compiler specific settings
COMP_NAME=ClusterRows
COMP_IDENTIFIER=com.github.dennisfrancis.ClusterRowsImpl

BUILD_DIR=./build
INCLUDES_DIR=$(BUILD_DIR)/include
LOCAL_INCLUDES_DIR=./inc
OBJECTS_DIR=$(BUILD_DIR)/objects
FLAGS_DIR=$(BUILD_DIR)/flags
URD_DIR=$(BUILD_DIR)/urd
RDB_DIR=$(BUILD_DIR)/rdb

META_DIR=$(BUILD_DIR)/meta
MANIFEST_DIR=$(META_DIR)/META-INF
MANIFEST_FILE=$(MANIFEST_DIR)/manifest.xml
MANIFEST_TEMPLATE_FILE=tmpl/manifest.xml.tmpl

COMPONENTS_FILE=$(META_DIR)/$(COMP_NAME).components
COMPONENTS_TEMPLATE_FILE=tmpl/ClusterRows.components.tmpl

COMP_RDB_NAME = GMMCluster.uno.rdb
COMP_RDB = $(RDB_DIR)/$(COMP_RDB_NAME)

EXT_DIR=$(BUILD_DIR)/extension
EXT_FILE=$(EXT_DIR)/$(COMP_NAME).$(UNOOXT_EXT)

PACKAGE_DIR=$(BUILD_DIR)/pkg

TYPES_DONE=$(FLAGS_DIR)/types.done

COMP_IMPL_NAME=$(COMP_NAME).uno.$(SHAREDLIB_EXT)

CXXFILES = $(addprefix src/,component.cxx cluster.cxx DialogHelper.cxx logging.cxx helper.cxx range.cxx perf.cxx datatypes.cxx preprocess.cxx em.cxx GMMCluster.cxx colorgen.cxx)
HXXFILES = $(addprefix inc/,GMMCluster.hxx cluster.hxx perf.hxx range.hxx datatypes.hxx em.hxx preprocess.hxx colorgen.hxx DialogHelper.hxx helper.hxx)

IDL_FILES = $(addprefix idl/,XGMMCluster.idl)

XCUFILES = $(addprefix xcu/,Addons.xcu Jobs.xcu GMMCluster.xcu)

XDLFILES = $(addprefix xdl/,ClusterRows.xdl)

DESCRIPTION_LANG_FILES = description-en-US.txt
DESCRIPTION_XML_TMPL = tmpl/description.xml.tmpl
DESCRIPTION_XML = $(META_DIR)/description.xml

IMG_DIR = img

IMG_FILES = img/icon.png img/icon_hc.png

OBJECT_FILES = $(patsubst %.cxx,$(OBJECTS_DIR)/%.$(OBJ_EXT),$(notdir $(CXXFILES)))

URD_FILES = $(patsubst %.idl,$(URD_DIR)/%.urd,$(notdir $(IDL_FILES)))

TYPESLIST = -Tcom.github.dennisfrancis.XGMMCluster -Tcom.github.dennisfrancis.GMMCluster

COMPILE_FLAGS = compile_flags.txt

CC_INCLUDES = -I$(PRJ)/include

ifeq "$(ENABLE_LOGGING)" "1"
CLUSTER_DEFINES = -DLOGGING_ENABLED
endif

# Targets
.PHONY: ALL
ALL : $(EXT_FILE)

include $(SETTINGS)/stdtarget.mk

$(URD_DIR)/%.urd : $(IDL_FILES)
	@mkdir -p $(URD_DIR)
	$(IDLC) -I. -I$(IDL_DIR) -O$(URD_DIR) $<

$(RDB_DIR)/%.rdb : $(URD_FILES)
	@rm -rf $(RDB_DIR)
	@mkdir -p $(RDB_DIR)
	$(REGMERGE) $@ /UCR $(URD_FILES)

$(TYPES_DONE): $(COMP_RDB)
	@mkdir -p $(INCLUDES_DIR)
	@mkdir -p $(FLAGS_DIR)
	$(CPPUMAKER) -Gc -O$(INCLUDES_DIR)/ $(URE_TYPES) $(OFFICE_TYPES)
	$(CPPUMAKER) -Gc -O$(INCLUDES_DIR)/ $(TYPESLIST) $(COMP_RDB) -X$(URE_TYPES) -X$(OFFICE_TYPES)
	touch $@

compileflags: $(TYPES_DONE)
	@echo "-c -fpic -fvisibility=hidden -O2 -std=c++17 $(CC_INCLUDES) -I$(INCLUDES_DIR) -I$(LOCAL_INCLUDES_DIR) $(CC_DEFINES) $(CLUSTER_DEFINES)" | tr ' ' '\n' > $(COMPILE_FLAGS)

$(OBJECTS_DIR)/%.$(OBJ_EXT): src/%.cxx $(TYPES_DONE) $(HXXFILES)
	@mkdir -p $(OBJECTS_DIR)
	$(CC) -c -fpic -fvisibility=hidden -O2 -std=c++17 $(CC_INCLUDES) -I$(INCLUDES_DIR) -I$(LOCAL_INCLUDES_DIR) $(CC_DEFINES) $(CLUSTER_DEFINES) -o $@ $<

$(OBJECTS_DIR)/$(COMP_IMPL_NAME): $(OBJECT_FILES)
	$(LINK) $(COMP_LINK_FLAGS) $(LINK_LIBS) -o $@ $(OBJECT_FILES) \
	$(CPPUHELPERLIB) $(CPPULIB) $(SALLIB) $(STC++LIB)

# rule for component package manifest
$(MANIFEST_FILE): $(MANIFEST_TEMPLATE_FILE)
	@mkdir -p $(MANIFEST_DIR)
	COMP_NAME=$(COMP_NAME) UNOPKG_PLATFORM=$(UNOPKG_PLATFORM) COMP_RDB_NAME=$(COMP_RDB_NAME) envsubst < $< > $@

$(COMPONENTS_FILE): $(COMPONENTS_TEMPLATE_FILE)
	@mkdir -p $(META_DIR)
	UNOPKG_PLATFORM=$(UNOPKG_PLATFORM) COMP_IMPL_NAME=$(COMP_IMPL_NAME) envsubst < $< > $@

$(DESCRIPTION_XML): $(DESCRIPTION_XML_TMPL)
	@mkdir -p $(META_DIR)
	CLUSTERROWS_VERSION=$(CLUSTERROWS_VERSION) envsubst < $< > $@

# rule for component package file
$(EXT_FILE): $(OBJECTS_DIR)/$(COMP_IMPL_NAME) $(COMP_RDB) $(XCUFILES) $(MANIFEST_FILE) $(COMPONENTS_FILE) $(DESCRIPTION_XML) $(DESCRIPTION_LANG_FILES) $(XDLFILES) $(IMG_FILES)
	@mkdir -p $(EXT_DIR) $(PACKAGE_DIR)
	@mkdir -p $(PACKAGE_DIR)/$(UNOPKG_PLATFORM)
	@mkdir -p $(PACKAGE_DIR)/$(IMG_DIR)
	@cp $(COMPONENTS_FILE) $(PACKAGE_DIR)/
	@cp $(COMP_RDB) $(PACKAGE_DIR)/
	@cp $(OBJECTS_DIR)/$(COMP_IMPL_NAME) $(PACKAGE_DIR)/$(UNOPKG_PLATFORM)/
	@cp $(XCUFILES) $(PACKAGE_DIR)/
	@cp $(XDLFILES) $(PACKAGE_DIR)/
	@cp $(DESCRIPTION_LANG_FILES) $(PACKAGE_DIR)/
	@cp $(DESCRIPTION_XML) $(PACKAGE_DIR)/
	@cp $(IMG_FILES) $(PACKAGE_DIR)/$(IMG_DIR)/
	@cp -r $(MANIFEST_DIR) $(PACKAGE_DIR)/
	@cd $(PACKAGE_DIR) && zip -r ../../$@ *

install: $(EXT_FILE)
	-unopkg remove $(COMP_IDENTIFIER)
	unopkg add $(EXT_FILE)

.PHONY: clean
clean :
	rm -rf $(BUILD_DIR)
