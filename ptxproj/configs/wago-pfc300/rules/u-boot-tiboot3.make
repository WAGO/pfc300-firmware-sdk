# -*-makefile-*-
#
# Copyright (C) 2020 by WAGO GmbH \& Co. KG
#
# See CREDITS for details about who has contributed to this project.
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
PACKAGES-$(PTXCONF_U_BOOT_TIBOOT3) += u-boot-tiboot3

#
# Paths and names
#
U_BOOT_TIBOOT3_MD5		:= $(call remove_quotes,$(PTXCONF_U_BOOT_TIBOOT3_MD5))
U_BOOT_VERSION			:= $(call remove_quotes, $(PTXCONF_U_BOOT_VERSION)-$(PTXCONF_U_BOOT_LOCALVERSION))
U_BOOT_TIBOOT3_VERSION		:= $(U_BOOT_VERSION)
U_BOOT_TIBOOT3			:= tiboot3-$(U_BOOT_VERSION).bin
U_BOOT_TIBOOT3_SOURCE		:= $(SRCDIR)/$(U_BOOT_TIBOOT3)
U_BOOT_TIBOOT3_MD5_FILE		:= $(U_BOOT_TIBOOT3_SOURCE).md5
U_BOOT_TIBOOT3_URL		:= $(call jfrog_template_to_url, U_BOOT_TIBOOT3)


# ----------------------------------------------------------------------------
# Get
# ----------------------------------------------------------------------------
#
$(U_BOOT_TIBOOT3_SOURCE):
	@$(call targetinfo)
	$(call ptx/in-path, PTXDIST_PATH, scripts/wago/artifactory.sh) fetch \
		'$(U_BOOT_TIBOOT3_URL)' '$(U_BOOT_TIBOOT3_SOURCE)' '$(U_BOOT_TIBOOT3_MD5_FILE)'
	@$(call touch)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/u-boot-tiboot3.targetinstall:
	@$(call targetinfo)
	@install -v -D -m644 $(U_BOOT_TIBOOT3_SOURCE) $(IMAGEDIR)/tiboot3.bin

# ----------------------------------------------------------------------------
# Clean
# ----------------------------------------------------------------------------

$(STATEDIR)/u-boot-tiboot3.clean:
	@$(call targetinfo)
	@$(call clean_pkg, U_BOOT_TIBOOT3)
	@rm -vf $(IMAGEDIR)/tiboot3.bin



