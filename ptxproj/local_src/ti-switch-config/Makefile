.SUFFIXES:  # Delete make's default suffix rules

.PHONY: default
default: all

#######################################################################################################################
PROJECT_ROOT ?= .
PTXDIST_PACKAGE ?= TI_SWITCH_CONFIG
PTXDIST_PACKAGE_MK_FILE ?= $(PTXDIST_WORKSPACE)/rules/ti-switch-config.make

SRC_DIR = $(PROJECT_ROOT)/

-include $(SCRIPT_DIR)/platform_1.mk

LINT_RULE_FILES = \
	$(LINT_CONFIG_DIR)/lint-rules/pfc.lnt

#######################################################################################################################
# Build target configuration

MAIN_BUILDTARGETS += \
	ti-switch-config.elf \

TEST_BUILDTARGETS += 

BUILDTARGETS += \
	$(MAIN_BUILDTARGETS) \
	$(TEST_BUILDTARGETS)

INSTALL_TARGETS += \
	$(DESTDIR)/usr/bin/ti-switch-config

#/# The variables in this section are used only within this make file.
SHARED_CCXXFLAGS += -fPIC
#/# Hide all symbols of the referenced object files from the shared object's API, except for those marked for export.
#/# https://gcc.gnu.org/wiki/Visibility and https://www.akkadia.org/drepper/dsohowto.pdf
SHARED_CCXXFLAGS += -fvisibility=hidden
SHARED_CFLAGS    += $(call option_std,gnu11)
SHARED_CXXFLAGS  += $(call option_std,gnu++17)
SHARED_CXXFLAGS  += -fvisibility-inlines-hidden


#/# Find the names of those artefacts in a dependency list which are generated by this makefile.
# $(call local_prerequisites,name)
local_prerequisites = $(call lib_buildtarget_raw,$($(1)_LIBS) $($(1)_PKG_CONFIG_LIBS),$($(1)_STATICALLYLINKED))


ti-switch-config.elf_LIBS             +=  
ti-switch-config.elf_STATICALLYLINKED += 
ti-switch-config.elf_PKG_CONFIGS      += 
ti-switch-config.elf_DISABLEDWARNINGS += 
ti-switch-config.elf_PREREQUISITES    += $(call local_prerequisites,ti-switch-config.elf)
ti-switch-config.elf_SOURCES          += $(call glob_r,$(addprefix $(PROJECT_ROOT)/*.,$(SOURCE_FILE_EXTENSIONS)))
ti-switch-config.elf_CPPFLAGS         += -I$(SRC_DIR)
ti-switch-config.elf_CPPFLAGS         += $(call pkg_config_cppflags,$(ti-switch-config.elf_PKG_CONFIGS))
ti-switch-config.elf_CCXXFLAGS        += $(SHARED_CCXXFLAGS)
ti-switch-config.elf_CCXXFLAGS        += $(call option_disable_warning,$(ti-switch-config.elf_DISABLEDWARNINGS))
ti-switch-config.elf_CFLAGS           += $(SHARED_CFLAGS) -DUSE_LOCAL_INC -DVERSION=\"am65-v1.0\"
ti-switch-config.elf_CFLAGS           += $(ti-switch-config.elf_CCXXFLAGS)
ti-switch-config.elf_CFLAGS           += $(call pkg_config_cflags,$(ti-switch-config.elf_PKG_CONFIGS))
ti-switch-config.elf_CXXFLAGS         += $(SHARED_CXXFLAGS)
ti-switch-config.elf_CXXFLAGS         += $(ti-switch-config.elf_CCXXFLAGS)
ti-switch-config.elf_CXXFLAGS         += $(call pkg_config_cxxflags,$(ti-switch-config.elf_PKG_CONFIGS))
ti-switch-config.elf_LDFLAGS          += $(call option_lib,$(ti-switch-config.elf_LIBS),ti-switch-config.elf)
ti-switch-config.elf_LDFLAGS          += $(call pkg_config_ldflags,$(ti-switch-config.elf_PKG_CONFIGS))


#######################################################################################################################
# Build infrastructure

include $(SCRIPT_DIR)/buildclang_1.mk
-include $(PTXDIST_WORKSPACE)/../wago_intern/debugging/scripts/valgrind-target.mk


#######################################################################################################################
# Custom rules

HDR_DIR=$(DESTDIR)/usr/include/ti-switch-config

#/# This package provides a pkg-config file for use by other packages.
$(DESTDIR)/usr/lib/pkgconfig/%.pc: $(PROJECT_ROOT)/%.pc | $(DESTDIR)/usr/lib/pkgconfig
	cp $< $@

#/# Install binaries.
$(DESTDIR)/usr/bin/%: $(BIN_DIR)/%.elf | $(DESTDIR)/usr/bin
	cp $< $@

#/# Install API headers.
$(HDR_DIR)/%.h: $(PROJECT_ROOT)/src/%.h | $(HDR_DIR)
	cp $< $@

#/# Create folder if it is missing.
$(HDR_DIR):
	mkdir -p $@

#######################################################################################################################
# Bootstrapping

#/# This rule complains if the folder pointed to by SCRIPT_DIR does not contain any .mk files.
$(SCRIPT_DIR)/%.mk:
	$(error build scripts unavailable ($@) - set script dir (SCRIPT_DIR=$(SCRIPT_DIR)))

