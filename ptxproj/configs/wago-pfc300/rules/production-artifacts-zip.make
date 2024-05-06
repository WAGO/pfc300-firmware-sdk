# -*-makefile-*-
# todo: header
#
#
# We provide this package
#
IMAGE_PACKAGES-$(PTXCONF_IMAGE_PRODUCTION_ARTIFACTS_ZIP) += image-production-artifacts-zip
# note: name has to start with "image-" otherwise PTXdist will ignore it

#
# Paths and names
#
IMAGE_PRODUCTION_ARTIFACTS_MODEM_FW_MD5  := 293e9d0aa27fccd875d7d5b119805702
IMAGE_PRODUCTION_ARTIFACTS_ZIP		 := image-production-artifacts-zip
IMAGE_PRODUCTION_ARTIFACTS_ZIP_PATH	 := $(IMAGEDIR)/$(IMAGE_PRODUCTION_ARTIFACTS_ZIP)
IMAGE_PRODUCTION_ARTIFACTS_DOWNLOAD_PATH := wago_intern/artifactory_sources

ifndef PTXCONF_WAGO_TOOLS_BUILD_VERSION_BINARIES

IMAGE_PRODUCTION_ARTIFACTS_HEX_RAW := firmware.tar
SVNREV = $(call assert,$(shell ptxd_get_svn_revision | sed 's/svn/r/g'),"Error: cannot determine SVN revision. Aborting!")
FWVER  = $(call assert,$(shell cat "$(PTXDIST_WORKSPACE)/projectroot/etc/REVISIONS" | sed 's/FIRMWARE=/V/g' | sed 's/\.//g' | sed 's/\x28/_/g' | sed 's/\x29//g'), "Error: cannot determine FW version. Aborting!")

NAME_PTXCONF := PFC300

ARTIFACTORY_BASE_URL			:= $(call remove_quotes, ${PTXCONF_ARTIFACTORY_BASE_URL})

IMAGE_PRODUCTION_ARTIFACTS_LINUXIMAGE_FIT_RAW	:= linuximage-production.fit
IMAGE_PRODUCTION_ARTIFACTS_LINUXIMAGE_FIT	:= linuximage-production_$(FWVER)_$(SVNREV)_development.fit

IMAGE_PRODUCTION_ARTIFACTS_ZIP_ARCHIVE	= $(IMAGEDIR)/production-artifacts_$(NAME_PTXCONF)_$(FWVER)_$(SVNREV).zip
IMAGE_PRODUCTION_ARTIFACTS_HEX_NAME	= firmware_$(FWVER)_$(SVNREV)_development.hex
IMAGE_PRODUCTION_ARTIFACTS_ZIP_IMAGE	= production-artifacts_$(NAME_PTXCONF)_*.zip

$(IMAGE_PRODUCTION_ARTIFACTS_ZIP_IMAGE):
	@$(call targetinfo)
	# prepare: copy all necessary production artifacts to images/production-artifacts-zip

	@mkdir -p $(IMAGEDIR)/$(IMAGE_PRODUCTION_ARTIFACTS_ZIP)

	@cp $(IMAGEDIR)/$(IMAGE_PRODUCTION_ARTIFACTS_HEX_RAW) $(IMAGE_PRODUCTION_ARTIFACTS_ZIP_PATH)/$(IMAGE_PRODUCTION_ARTIFACTS_HEX_NAME)

	@cp $(IMAGEDIR)/$(IMAGE_PRODUCTION_ARTIFACTS_LINUXIMAGE_FIT_RAW) $(IMAGE_PRODUCTION_ARTIFACTS_ZIP_PATH)/$(IMAGE_PRODUCTION_ARTIFACTS_LINUXIMAGE_FIT)

	# zip artifacts
	cd $(IMAGE_PRODUCTION_ARTIFACTS_ZIP_PATH) && zip -r $(IMAGE_PRODUCTION_ARTIFACTS_ZIP_ARCHIVE) *
	@rm -rf $(IMAGEDIR)/$(IMAGE_PRODUCTION_ARTIFACTS_ZIP)

	@$(call finish)

# ----------------------------------------------------------------------------
# Clean
# ----------------------------------------------------------------------------

$(STATEDIR)/image-production-artifacts-zip.clean:
	@$(call targetinfo)
	@$(call clean_pkg, IMAGE_PRODUCTION_ARTIFACTS_ZIP)
	@rm -rf $(IMAGEDIR)/$(IMAGE_PRODUCTION_ARTIFACTS_ZIP)
	@rm -rf $(IMAGEDIR)/production-artifacts_*.zip

endif # PTXCONF_WAGO_TOOLS_BUILD_VERSION_BINARIES

# vim: syntax=make
