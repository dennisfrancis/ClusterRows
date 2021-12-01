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

PY_SRC_DIR=./src/py
COMP_IMPL_NAME=$(COMP_NAME).py

IDL_FILES = $(addprefix idl/,XGMMCluster.idl)

#XCUFILES = $(addprefix xcu/,Addons.xcu Jobs.xcu GMMCluster.xcu)
XCUFILES = $(addprefix xcu/,GMMCluster.xcu)

#XDLFILES = $(addprefix xdl/,ClusterRows.xdl)

DESCRIPTION_LANG_FILES = description-en-US.txt
DESCRIPTION_XML_TMPL = tmpl/description.xml.tmpl
DESCRIPTION_XML = $(META_DIR)/description.xml

IMG_DIR = img

IMG_FILES = img/icon.png img/icon_hc.png

URD_FILES = $(patsubst %.idl,$(URD_DIR)/%.urd,$(notdir $(IDL_FILES)))

TYPESLIST = -Tcom.github.dennisfrancis.XGMMCluster

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

# rule for component package manifest
$(MANIFEST_FILE): $(MANIFEST_TEMPLATE_FILE)
	@mkdir -p $(MANIFEST_DIR)
	COMP_IMPL_NAME=$(COMP_IMPL_NAME) COMP_RDB_NAME=$(COMP_RDB_NAME) envsubst < $< > $@

$(DESCRIPTION_XML): $(DESCRIPTION_XML_TMPL)
	@mkdir -p $(META_DIR)
	CLUSTERROWS_VERSION=$(CLUSTERROWS_VERSION) envsubst < $< > $@

# rule for component package file
$(EXT_FILE): $(PY_SRC_DIR)/$(COMP_IMPL_NAME) $(COMP_RDB) $(XCUFILES) $(MANIFEST_FILE) $(DESCRIPTION_XML) $(DESCRIPTION_LANG_FILES) $(IMG_FILES)
	@mkdir -p $(EXT_DIR) $(PACKAGE_DIR)
	@mkdir -p $(PACKAGE_DIR)/$(IMG_DIR)
	@cp $(COMP_RDB) $(PACKAGE_DIR)/
	@cp $(PY_SRC_DIR)/$(COMP_IMPL_NAME) $(PACKAGE_DIR)/
	@cp $(XCUFILES) $(PACKAGE_DIR)/
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
