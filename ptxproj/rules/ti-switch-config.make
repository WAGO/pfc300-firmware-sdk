# -*-makefile-*-
#
# Copyright (C) 2024 by WAGO GmbH \& Co. KG
#
# See CREDITS for details about who has contributed to this project.
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
PACKAGES-$(PTXCONF_TI_SWITCH_CONFIG) += ti-switch-config

#
# Paths and names
#
TI_SWITCH_CONFIG_VERSION        := am65-v1.0
TI_SWITCH_CONFIG_MD5            :=
TI_SWITCH_CONFIG                := ti-switch-config
TI_SWITCH_CONFIG_BUILDCONFIG    := Release
TI_SWITCH_CONFIG_SRC_DIR        := $(call ptx/in-path, PTXDIST_PATH, local_src/$(TI_SWITCH_CONFIG))
TI_SWITCH_CONFIG_BUILDROOT_DIR  := $(BUILDDIR)/$(TI_SWITCH_CONFIG)
TI_SWITCH_CONFIG_DIR            := $(TI_SWITCH_CONFIG_BUILDROOT_DIR)/src
TI_SWITCH_CONFIG_BUILD_DIR      := $(TI_SWITCH_CONFIG_BUILDROOT_DIR)/bin/$(TI_SWITCH_CONFIG_BUILDCONFIG)
TI_SWITCH_CONFIG_LICENSE        := GPLv2
TI_SWITCH_CONFIG_CONF_TOOL      := NO
TI_SWITCH_CONFIG_MAKE_ENV       := $(CROSS_ENV) \
BUILDCONFIG=$(TI_SWITCH_CONFIG_BUILDCONFIG) \
BIN_DIR=$(TI_SWITCH_CONFIG_BUILD_DIR) \
SCRIPT_DIR=$(PTXDIST_SYSROOT_HOST)/lib/ct-build \
PTXDIST_PACKAGE_MK_FILE=$(call ptx/in-path, PTXDIST_PATH, rules/ti-switch-config.make)

TI_SWITCH_CONFIG_LICENSE_FILES  := \
	file://LICENSE;md5=c35c4565535ec9ce05e3ed6d31f2a925 \
	file://switch-config.c;md5=659ff9658cbaba3110b81804af60de75 \
	file://sockios_loc.h;md5=7c5ee4112d3c30e117940bb6b747c15d \
	file://net_switch_config.h;md5=7c5ee4112d3c30e117940bb6b747c15d 

# ----------------------------------------------------------------------------
# Extract
# ----------------------------------------------------------------------------

# During BSP creation local_src is deleted and the source code directories are
# copied on demand. To handle this condition an order-only dependency on
# the source code directory is created. When it is missing, the target below
# is executed and an error message is generated.
$(TI_SWITCH_CONFIG_SRC_DIR):
	@echo "Error: $@: directory not found!" >&2; exit 2

$(STATEDIR)/ti-switch-config.extract:  | $(TI_SWITCH_CONFIG_SRC_DIR)
	@$(call targetinfo)
	@mkdir -p $(TI_SWITCH_CONFIG_BUILDROOT_DIR)
	@if [ ! -L $(TI_SWITCH_CONFIG_DIR) ]; then \
	  ln -s $(TI_SWITCH_CONFIG_SRC_DIR) $(TI_SWITCH_CONFIG_DIR); \
	fi
	@$(call touch)

# ----------------------------------------------------------------------------
# Prepare
# ----------------------------------------------------------------------------

$(STATEDIR)/ti-switch-config.prepare:
	@$(call targetinfo)
	@$(call world/prepare, TI_SWITCH_CONFIG)
	@$(call touch)

# ----------------------------------------------------------------------------
# Compile
# ----------------------------------------------------------------------------

$(STATEDIR)/ti-switch-config.compile:
	@$(call targetinfo)
	@$(call world/compile, TI_SWITCH_CONFIG)
	@$(call touch)

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------

$(STATEDIR)/ti-switch-config.install:
	@$(call targetinfo)
	@$(call world/install, TI_SWITCH_CONFIG)
	@$(call touch)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/ti-switch-config.targetinstall:
	@$(call targetinfo)

	@$(call install_init, ti-switch-config)
	@$(call install_fixup, ti-switch-config,PRIORITY,optional)
	@$(call install_fixup, ti-switch-config,SECTION,base)
	@$(call install_fixup, ti-switch-config,AUTHOR,"WAGO GmbH \& Co. KG")
	@$(call install_fixup, ti-switch-config,DESCRIPTION,missing)

	@$(call install_copy, ti-switch-config, 0, 0, 0755, -, /usr/bin/ti-switch-config)

	@$(call install_finish, ti-switch-config)

	@$(call touch)

# ----------------------------------------------------------------------------
# Clean
# ----------------------------------------------------------------------------

$(STATEDIR)/ti-switch-config.clean:
	@$(call targetinfo)
	@if [ -d $(TI_SWITCH_CONFIG_DIR) ]; then \
		$(TI_SWITCH_CONFIG_MAKE_ENV) $(TI_SWITCH_CONFIG_PATH) $(MAKE) $(MFLAGS) -C $(TI_SWITCH_CONFIG_DIR) clean; \
	fi
	@$(call clean_pkg, TI_SWITCH_CONFIG)
	@rm -rf $(TI_SWITCH_CONFIG_BUILDROOT_DIR)

# vim: syntax=make
