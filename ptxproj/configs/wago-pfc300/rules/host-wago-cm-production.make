# -*-makefile-*-
#
# Copyright (C) 2015 by <HFS>
#
# See CREDITS for details about who has contributed to this project.
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
HOST_PACKAGES-$(PTXCONF_HOST_WAGO_CM_PRODUCTION) += host-wago-cm-production

#
# Paths and names
#
HOST_WAGO_CM_PRODUCTION			:= wago-cm-production
HOST_WAGO_CM_PRODUCTION_ARCH		:= $(call remove_quotes, $(PTXCONF_HOST_WAGO_CM_PRODUCTION_ARCH))
HOST_WAGO_CM_PRODUCTION_ARCHIVE		:= $(HOST_WAGO_CM_PRODUCTION_ARCH)
HOST_WAGO_CM_PRODUCTION_SUFFIX		:= tar
HOST_WAGO_CM_PRODUCTION_URL		:= $(call remove_quotes, $(PTXCONF_SETUP_PTXMIRROR))/$(HOST_WAGO_CM_PRODUCTION_ARCHIVE).$(HOST_WAGO_CM_PRODUCTION_SUFFIX)
HOST_WAGO_CM_PRODUCTION_SOURCE		:= $(SRCDIR)/$(HOST_WAGO_CM_PRODUCTION_ARCHIVE).$(HOST_WAGO_CM_PRODUCTION_SUFFIX)
HOST_WAGO_CM_PRODUCTION_DIR		:= $(HOST_BUILDDIR)/$(HOST_WAGO_CM_PRODUCTION)
HOST_WAGO_CM_PRODUCTION_MD5		:= $(call remove_quotes, $(PTXCONF_HOST_WAGO_CM_PRODUCTION_ARCH_MD5))

# ----------------------------------------------------------------------------
# Extract
# ----------------------------------------------------------------------------

$(STATEDIR)/host-wago-cm-production.extract:
	@$(call targetinfo)

	@mkdir -p $(HOST_WAGO_CM_PRODUCTION_DIR)
	@tar xf $(HOST_WAGO_CM_PRODUCTION_SOURCE) -C $(HOST_WAGO_CM_PRODUCTION_DIR)

	@$(call touch)

# ----------------------------------------------------------------------------
# Compile
# ----------------------------------------------------------------------------

$(STATEDIR)/host-wago-cm-production.compile:
	@$(call targetinfo)
	@$(call touch)

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------

$(STATEDIR)/host-wago-cm-production.install:
	@$(call targetinfo)

	@cp $(HOST_WAGO_CM_PRODUCTION_DIR)/root-setupfw.cpio.gz $(IMAGEDIR)

	@$(call touch)

# ----------------------------------------------------------------------------
# Clean
# ----------------------------------------------------------------------------

$(STATEDIR)/host-wago-cm-production.clean:
	@$(call targetinfo)

	@rm -rf $(IMAGEDIR)/root-setupfw.cpio.gz

	@$(call clean_pkg, HOST_WAGO_CM_PRODUCTION)

# vim: syntax=make
