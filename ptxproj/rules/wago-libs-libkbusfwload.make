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
PACKAGES-$(PTXCONF_LIBKBUSFWLOAD) += libkbusfwload

#
# Paths and names
#
LIBKBUSFWLOAD_VERSION        := 1.0.1
LIBKBUSFWLOAD_MD5            :=
LIBKBUSFWLOAD                := libkbusfwload
LIBKBUSFWLOAD_BUILDCONFIG    := Release
LIBKBUSFWLOAD_SRC_DIR        := $(call ptx/in-path, PTXDIST_PATH, wago_intern/$(LIBKBUSFWLOAD))
LIBKBUSFWLOAD_BUILDROOT_DIR  := $(BUILDDIR)/$(LIBKBUSFWLOAD)
LIBKBUSFWLOAD_DIR            := $(LIBKBUSFWLOAD_BUILDROOT_DIR)/src
LIBKBUSFWLOAD_BUILD_DIR      := $(LIBKBUSFWLOAD_BUILDROOT_DIR)/bin/$(LIBKBUSFWLOAD_BUILDCONFIG)
LIBKBUSFWLOAD_LICENSE        := WAGO
LIBKBUSFWLOAD_CONF_TOOL      := NO
LIBKBUSFWLOAD_MAKE_ENV       := $(CROSS_ENV) \
BUILDCONFIG=$(LIBKBUSFWLOAD_BUILDCONFIG) \
BIN_DIR=$(LIBKBUSFWLOAD_BUILD_DIR) \
SCRIPT_DIR=$(PTXDIST_SYSROOT_HOST)/lib/ct-build \
PTXDIST_PACKAGE_MK_FILE=$(call ptx/in-path, PTXDIST_PATH, rules/wago-libs-libkbusfwload.make)

LIBKBUSFWLOAD_PACKAGE_NAME             := $(LIBKBUSFWLOAD)_$(LIBKBUSFWLOAD_VERSION)_$(PTXDIST_IPKG_ARCH_STRING)
LIBKBUSFWLOAD_PLATFORMCONFIGPACKAGEDIR := $(PTXDIST_PLATFORMCONFIGDIR)/packages

# ----------------------------------------------------------------------------
# Extract
# ----------------------------------------------------------------------------

$(STATEDIR)/libkbusfwload.extract:
	@$(call targetinfo)
	@mkdir -p $(LIBKBUSFWLOAD_BUILDROOT_DIR)
ifndef PTXCONF_WAGO_TOOLS_BUILD_VERSION_BINARIES
	@if [ ! -L $(LIBKBUSFWLOAD_DIR) ]; then \
	  ln -s $(LIBKBUSFWLOAD_SRC_DIR) $(LIBKBUSFWLOAD_DIR); \
	fi
endif
	@$(call touch)

# ----------------------------------------------------------------------------
# Prepare
# ----------------------------------------------------------------------------

$(STATEDIR)/libkbusfwload.prepare:
	@$(call targetinfo)
ifndef PTXCONF_WAGO_TOOLS_BUILD_VERSION_BINARIES
	@$(call world/prepare, LIBKBUSFWLOAD)
endif
	@$(call touch)

# ----------------------------------------------------------------------------
# Compile
# ----------------------------------------------------------------------------

$(STATEDIR)/libkbusfwload.compile:
	@$(call targetinfo)
ifndef PTXCONF_WAGO_TOOLS_BUILD_VERSION_BINARIES
	@$(call world/compile, LIBKBUSFWLOAD)
endif
	@$(call touch)

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------

$(STATEDIR)/libkbusfwload.install:
	@$(call targetinfo)
ifdef PTXCONF_WAGO_TOOLS_BUILD_VERSION_BINARIES
# BSP mode: install by extracting tgz file
	@mkdir -p $(LIBKBUSFWLOAD_PKGDIR) && \
	  tar xvzf $(LIBKBUSFWLOAD_PLATFORMCONFIGPACKAGEDIR)/$(LIBKBUSFWLOAD_PACKAGE_NAME).tgz -C $(LIBKBUSFWLOAD_PKGDIR)
else
# normal mode, call "make install"

	@$(call world/install, LIBKBUSFWLOAD)

ifdef PTXCONF_WAGO_TOOLS_BUILD_VERSION_RELEASE
# save install directory to tgz for BSP mode
	@mkdir -p $(LIBKBUSFWLOAD_PLATFORMCONFIGPACKAGEDIR)
	@cd $(LIBKBUSFWLOAD_PKGDIR) && tar cvzf $(LIBKBUSFWLOAD_PLATFORMCONFIGPACKAGEDIR)/$(LIBKBUSFWLOAD_PACKAGE_NAME).tgz *
endif
endif

	@$(call touch)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/libkbusfwload.targetinstall:
	@$(call targetinfo)

	@$(call install_init, libkbusfwload)
	@$(call install_fixup, libkbusfwload,PRIORITY,optional)
	@$(call install_fixup, libkbusfwload,SECTION,base)
	@$(call install_fixup, libkbusfwload,AUTHOR,"WAGO GmbH \& Co. KG")
	@$(call install_fixup, libkbusfwload,DESCRIPTION,missing)

	@$(call install_lib, libkbusfwload, 0, 0, 0644, libkbusfwload)
	@$(call install_copy, libkbusfwload, 0, 0, 0755, -, /usr/bin/kbusfwloader)
	@$(call install_copy, libkbusfwload, 0, 0, 0750, -, /etc/init.d/check_kbusfw)
	@$(call install_link, libkbusfwload, /etc/init.d/check_kbusfw, /etc/rc.d/S26_check_kbusfw)

	@$(call install_finish, libkbusfwload)

	@$(call touch)

# ----------------------------------------------------------------------------
# Clean
# ----------------------------------------------------------------------------

$(STATEDIR)/libkbusfwload.clean:
	@$(call targetinfo)
	@if [ -d $(LIBKBUSFWLOAD_DIR) ]; then \
		$(LIBKBUSFWLOAD_MAKE_ENV) $(LIBKBUSFWLOAD_PATH) $(MAKE) $(MFLAGS) -C $(LIBKBUSFWLOAD_DIR) clean; \
	fi
	@$(call clean_pkg, LIBKBUSFWLOAD)
	@rm -rf $(LIBKBUSFWLOAD_BUILDROOT_DIR)

# vim: syntax=make
