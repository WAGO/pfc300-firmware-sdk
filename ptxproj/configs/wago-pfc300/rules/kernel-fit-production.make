# -*-makefile-*-
#
# Copyright (C) 2019 by Sascha Hauer <s.hauer@pengutronix.de>
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
PACKAGES-$(PTXCONF_KERNEL_FIT_PRODUCTION) += kernel-fit-production

#
# Paths and names
#
KERNEL_FIT_PRODUCTION_VERSION		:= $(KERNEL_VERSION)
KERNEL_FIT_PRODUCTION_IMAGE		:= $(IMAGEDIR)/linuximage-production.fit
ifdef PTXCONF_KERNEL_FIT_PRODUCTION_SIGNED
KERNEL_FIT_PRODUCTION_SIGN_ROLE		:= image-kernel-fit-production
KERNEL_FIT_PRODUCTION_KEY_NAME_HINT	:= image-kernel-fit-production
endif
KERNEL_FIT_PRODUCTION_KERNEL		 = $(KERNEL_IMAGE_PATH_y)
KERNEL_FIT_PRODUCTION_INITRAMFS		:= $(IMAGEDIR)/root-setupfw.cpio.gz
KERNEL_FIT_PRODUCTION_DTB		 = $(IMAGEDIR)/$(KERNEL_DTB_FILES)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/kernel-fit-production.targetinstall: $(STATEDIR)/host-wago-cm-production.install
	@$(call targetinfo)
	@$(call world/image-fit-production, KERNEL_FIT_PRODUCTION)
	@$(call touch)
