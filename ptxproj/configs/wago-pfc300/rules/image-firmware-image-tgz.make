# -*-makefile-*-
#
# Copyright (C) 2014 by <AGa>
#
# See CREDITS for details about who has contributed to this project.
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
IMAGE_PACKAGES-$(PTXCONF_IMAGE_FIRMWARE_IMAGE_TAR) += image-firmware-image-tar

#
# Paths and names
#
IMAGE_FIRMWARE_IMAGE_TAR		:= image-firmware-image-tar
IMAGE_FIRMWARE_IMAGE_TAR_DIR		:= $(BUILDDIR)/$(IMAGE_FIRMWARE_IMAGE_TAR)
IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE	:= $(PTXDIST_TEMPDIR)/image-firmware-image-tar-tmp
IMAGE_FIRMWARE_IMAGE_TAR_IMAGE		:= $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)/firmware.tar
IMAGE_FIRMWARE_IMAGE_TAR_CONFIG		:= firmware.config

# ----------------------------------------------------------------------------
# Image
# ----------------------------------------------------------------------------

$(IMAGE_FIRMWARE_IMAGE_TAR_IMAGE): $(IMAGEDIR)/root.tgz \
					$(STATEDIR)/host-wago-cm-production.install
	@$(call targetinfo)

	@$(call image/genimage, IMAGE_FIRMWARE_IMAGE_TAR)

	@cp $(HOST_WAGO_CM_PRODUCTION_DIR)/postinst $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)
	@cp $(HOST_WAGO_CM_PRODUCTION_DIR)/util-linux-ng_*_arm.ipk $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)
	@cp $(HOST_WAGO_CM_PRODUCTION_DIR)/e2fsprogs_*_arm.ipk $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)
	@cp $(HOST_WAGO_CM_PRODUCTION_DIR)/format_emmc.sh $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)
	@cp $(HOST_WAGO_CM_PRODUCTION_DIR)/coreutils_*_arm.ipk $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)
	@cp $(HOST_WAGO_CM_PRODUCTION_DIR)/common-functions.sh $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)

	@echo "Creating '$(notdir $@)'..."

	@(  echo -n "tar -cf $@ "; \
	    echo -n "--exclude=$(notdir $@) "; \
	    echo -n "-C $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)/ ." \
	) | $(FAKEROOT) --

	@cp $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)/$(notdir $@) $(IMAGEDIR)/
	@rm -rf $(IMAGE_FIRMWARE_IMAGE_TAR_PATH_IMAGE)

	@echo "done."

	@$(call finish)

# vim: syntax=make
