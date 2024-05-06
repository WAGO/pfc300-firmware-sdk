# -*-makefile-*-
#
# Copyright (C) 2023 by Oliver Rohe
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
PACKAGES-$(PTXCONF_BLSPEC_UBOOT) += blspec-uboot

BLSPEC_UBOOT_VERSION	:= 0.0.1

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/blspec-uboot.targetinstall:
	@$(call targetinfo)

	@$(call install_init, blspec-uboot)
	@$(call install_fixup,blspec-uboot,PRIORITY,optional)
	@$(call install_fixup,blspec-uboot,SECTION,base)
	@$(call install_fixup,blspec-uboot,AUTHOR,"Oliver Rohe")
	@$(call install_fixup,blspec-uboot,DESCRIPTION,missing)

	@$(call install_alternative, blspec-uboot, 0, 0, 0644, \
		/boot/extlinux/extlinux.conf)

	@$(call install_finish,blspec-uboot)

	@$(call touch)

# vim: syntax=make
