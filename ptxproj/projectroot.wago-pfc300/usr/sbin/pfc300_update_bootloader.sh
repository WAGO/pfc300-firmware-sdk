#!/bin/bash
#-----------------------------------------------------------------------------#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file is part of PTXdist package wago-custom-install.
#
# Copyright (c) 2015-2022 WAGO GmbH & Co. KG
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
# Script:   pfc300_update_bootloader.sh
#
# Brief:    Update script for PFC bootloaders.
#
# Author:   AGa: WAGO GmbH & Co. KG
# Author:   PEn: WAGO GmbH & Co. KG
# Author:   KK: WAGO GmbH & Co. KG
# Auhtor:   SD: WAGO GmbH & Co. KG
#-----------------------------------------------------------------------------#

#
# PFCXXXv1/v2 has 4 copies of each the MLO and the 2nd stage Barebox bootloader.
# 
# We use the following strategy to prevent bricking the device when updating the bootolader:
# - a copy of MLO and Barebox are written to the flash but purposely left invalid.
# The invalidation method depends on the CRC algorithm used:
# MLO on PFC200 uses 1 bit Hamming - we inject a CRC error to the first page.
# Other bootloader parts use 8 bit BCH - we skip 1st 2048 bytes long block first and write
# it in a separate "validation" step if everything else went well.
# The CPU skips the invalid copies and use the old ones.
#
# This approach is repeated for each of the 4 copies.

#
# PFCXXXv3 has a central boot partition for MLO bootloader. Barebox 2nd stage bootloader
# is stored in /boot-Directory on ROOT FS.
#
PATH=/sbin:/usr/sbin:/bin:/usr/bin

. /etc/config-tools/board_specific_defines; . $(dirname $0)/${BOARD_ID}_commons

#
set -o nounset
set -o errexit

#
# Parameters: $1 - full path of the updated u-boot tiboot3 binary
#             $2 - full path of the updated u-boot tispl binary
#             $3 - full path of the updated u-boot image
#
function update_bootloader_in_rootfs
{
    local tiboot3_path="$1"
    local tispl_path="$2"
    local u_boot_path="$3"

    local loader_mount_point="/tmp/bootloader"
    local update_folder="$loader_mount_point/update"

    mkdir -p "$loader_mount_point"
    mount "${G_EMMC_BOOT_DEV}" "$loader_mount_point"
    mkdir -p "$update_folder"

    cp "$tiboot3_path"* "$update_folder/tiboot3.bin"
    cp "$tispl_path"* "$update_folder/tispl.bin"
    cp "$u_boot_path"* "$update_folder/u-boot.img"

    mv "$update_folder/"* "$loader_mount_point/"
    sync
    rmdir "$update_folder"
    umount "$loader_mount_point"
    rmdir  "$loader_mount_point"
    
    return $?
}

#
# Parameters: $1 - index of boot hardware partition (0 for mmcblk1boot0; 1 for mmcblk1boot1)
#             $2 - full path of the updated u-boot tiboot3 binary
#             $3 - full path of the updated u-boot tispl binary
#             $4 - full path of the updated u-boot image
#
function update_bootloader_in_emmc_hw_part
{
    local hw_part_index=$1
    local tiboot3_path="$2"
    local tispl_path="$3"
    local u_boot_path="$4"
    
    local hw_part=mmcblk1boot${hw_part_index}
    local result=0

    # ensure the force_ro flag is set to 0 but set it to its previous value afterwards
    local force_ro_backup=$(cat /sys/block/${hw_part}/force_ro)
    echo 0 > /sys/block/${hw_part}/force_ro
    result=$?
    if [ ! $result ]; then
        ReportError "Error: Failed to change write protection for ${hw_part}."
        return $result
    fi

    dd if="${tiboot3_path}" of=/dev/${hw_part} seek=0
    result=$?
    if [ ! $result ]; then
        ReportError "Error: Failed to update uboot tiboot3 in ${hw_part}."
        return $result
    fi

    dd if="${tispl_path}" of=/dev/${hw_part} seek=1024
    result=$?
    if [ ! $result ]; then
        ReportError "Error: Failed to update uboot tispl in ${hw_part}."
        return $result
    fi

    dd if="${u_boot_path}" of=/dev/${hw_part} seek=3584
    result=$?
    if [ ! $result ]; then
        ReportError "Error: Failed to update uboot in ${hw_part}."
        return $result
    fi

    sync

    echo "${force_ro_backup}" > /sys/block/${hw_part}/force_ro
    result=$?
    if [ ! $result ]; then
        ReportError "Error: Failed to restore write protection status for ${hw_part}."
        return $result
    fi

    return $result
}

#
# Parameters: $1 - full path of the updated u-boot tiboot3 binary
#             $2 - full path of the updated u-boot tispl binary
#             $3 - full path of the updated u-boot image
#
function update_bootloader_in_emmc
{
    local tiboot3_path="$1"
    local tispl_path="$2"
    local u_boot_path="$3"

    if ! update_bootloader_in_emmc_hw_part 0 "${tiboot3_path}" "${tispl_path}" "${u_boot_path}"; then
        ReportError "Error: Failed to update bootloader in first emmc partition."
        return $SHELL_ERROR
    fi

    if ! update_bootloader_in_emmc_hw_part 1 "${tiboot3_path}" "${tispl_path}" "${u_boot_path}"; then
        ReportError "Error: Failed to update bootloader in second emmc partition."
        return $SHELL_ERROR
    fi

    return 0
}

#
# Parameters: $1 - full path of the updated u-boot tiboot3 binary
#             $2 - full path of the updated u-boot tispl binary
#             $3 - full path of the updated u-boot image
#
function update_bootloader
{
    local tiboot3_path="$1"
    local tispl_path="$2"
    local u_boot_path="$3"

    if ! update_bootloader_in_emmc "${tiboot3_path}" "${tispl_path}" "${u_boot_path}"; then
        ReportError "Error: Failed to update bootloader in emmc."
        return $SHELL_ERROR
    fi

    if ! update_bootloader_in_rootfs "${tiboot3_path}" "${tispl_path}" "${u_boot_path}"; then
        ReportError "Error: Failed to update bootloader in rootfs."
        return $SHELL_ERROR
    fi

    return 0
}

#
# Parameters: $1 - index of system for backup destination 
#             $2 - full path of the bootloader_compatible_versions file
#             $3 - full path of the updated u-boot tiboot3 binary
#             $4 - full path of the updated u-boot tispl binary
#             $5 - full path of the updated u-boot image
#
function backup_bootloader
{
    local system_index="$1"
    local bootloader_compatible_versions="$2"
    local tiboot3_path="$3"
    local tispl_path="$4"
    local u_boot_path="$5"
    local result=0

    # Determine target ROOT FS device
    local backup_root=""

    case $system_index in
        "1")
            backup_root="$G_EMMC_ROOT1_DEV"
            ;;
        "2")
            backup_root="$G_EMMC_ROOT2_DEV"
            ;;
        *)
            result=$INTERNAL_ERROR
            ;;
    esac

    # Mount target ROOT FS and copy bootloaders
    local mount_point="/tmp/bootloader-update/backup_root"
    local loader_backup_destination="/boot/loader-backup"
    if [[ "$result" -eq "0" ]]; then
        mkdir -p "$mount_point"
        result=$(( $? != 0 ? $INTERNAL_ERROR : 0 ))
    fi

    if [[ "$result" -eq "0" ]]; then
        mount "$backup_root" "$mount_point"
        result=$(( $? != 0 ? $INTERNAL_ERROR : 0 ))
        if [[ "$result" -eq "0" ]]; then
            rm -rf "$mount_point$loader_backup_destination"
            mkdir "$mount_point$loader_backup_destination"
            result=$(( $? != 0 ? $INTERNAL_ERROR : 0 ))
            if [[ "$result" -eq "0" ]]; then
                cp "$tiboot3_path"* "$mount_point$loader_backup_destination"
                result=$(( $? != 0 ? $INTERNAL_ERROR : 0 ))
            fi
            if [[ "$result" -eq "0" ]]; then
                cp "$tispl_path"* "$mount_point$loader_backup_destination"
                result=$(( $? != 0 ? $INTERNAL_ERROR : 0 ))
            fi
            if [[ "$result" -eq "0" ]]; then
                cp "$u_boot_path"* "$mount_point$loader_backup_destination"
                result=$(( $? != 0 ? $INTERNAL_ERROR : 0 ))
            fi
            if [[ "$result" -eq "0" ]]; then
                cp "$bootloader_compatible_versions" "$mount_point$loader_backup_destination"
                result=$(( $? != 0 ? $INTERNAL_ERROR : 0 ))
            fi
            sync
            umount "$mount_point"
            rm -rf "/tmp/bootloader-update"
        fi

    fi

    return $result
}

#
# If the installed version is whitelisted, return false, else return true
# If no version is stored in typelabel, the version IS too old
#
function current_bootloader_version_too_old
{
    local too_old="yes"

    local installed_bl_version="$(/etc/config-tools/get_typelabel_value -n BLIDX)"
    installed_bl_version="${installed_bl_version##BLIDX=}" # cut 'BLIDX='

    local whitelist="${BAREBOX_COMPATIBLE_VERSIONS}"

    for version in ${whitelist}; do
        if [[ "${installed_bl_version}" == "${version}" && "${version}" != "99" ]]; then
            too_old="no"
            break
        fi
    done

    if [[ "${too_old}" == "yes" ]]; then
        true
    else
        false
    fi
}

#Reports unknown device and exits with $SHELL_ERROR
function report_unknown_device
{
    ReportError "Error: unknown device (pfc300 expected)."
    exit $SHELL_ERROR
}

#
# Parameters: $1 - platform type pfc300 
#             $2 - bootloader source path (optional)
#             $3 - system index to store bootloader backup (optional, default: active system, 9: restore backuped bootloader)
#
function __main
{
    local dev_type="${1:-unknown}"
    local base_path="${2:-}"
    local bootloader_compatible_versions="${2:-/etc}"
    local system_index="${3:-$(/etc/config-tools/get_systeminfo active-system-number)}"

    if [[ -z "$system_index" ]]; then
        ReportError "Error: undefined system index."
        exit $SHELL_ERROR
    fi

    bootloader_compatible_versions="${bootloader_compatible_versions}/barebox-compatible-versions"
    if [[ -f "$bootloader_compatible_versions" ]]; then
        source "$bootloader_compatible_versions"
    else
        ReportError "Error: \"$bootloader_compatible_versions\" not found."
        exit $SHELL_ERROR
    fi

    if [[ "${dev_type}" != "pfc300" ]]; then
        report_unknown_device
    fi

    # Build source file names
    local tiboot3_base="$(basename "${G_TIBOOT3_FILE_BASE}")"
    local tispl_base="$(basename "${G_TISPL_FILE_BASE}")"
    local u_boot_base="$(basename "${G_U_BOOT_FILE_BASE}")"
    local tiboot3_path=""
    local tispl_path=""
    local u_boot_path=""
    if [[ -n "$base_path" ]]; then
        tiboot3_path="${base_path}/${tiboot3_base}"
        tispl_path="${base_path}/${tispl_base}"
        u_boot_path="${base_path}/${u_boot_base}"
    else
        tiboot3_path="${G_TIBOOT3_FILE_BASE}"
        tispl_path="${G_TISPL_FILE_BASE}"
        u_boot_path="${G_U_BOOT_FILE_BASE}"
    fi

    # Store bootloader backup
    if [[ -z "$system_index" ]]; then
        echo "Note: Skipping bootloader backup in ROOT FS."
    elif [[ "$system_index" -eq "9" ]]; then
        echo "Restore bootloaders for type $dev_type from backup at \"$base_path\""
    else
        # Backup bootloader to ROOT FS
        backup_bootloader $system_index "$bootloader_compatible_versions" "${tiboot3_path}" "${tispl_path}" "${u_boot_path}"
        if [[ "$?" -ne "0" ]]; then
            ReportError "Error: Unable to write bootloaders backup to system $system_index"
            exit $INTERNAL_ERROR
        fi
    fi

    # Update bootloader if necessary
    if ! current_bootloader_version_too_old; then
        echo "Note: No bootloader update needed on central place."
        exit $SUCCESS
    fi

    update_bootloader "${tiboot3_path}" "${tispl_path}" "${u_boot_path}"

    # Update bootloader version stored in eeprom
    local new_version="${BAREBOX_COMPATIBLE_VERSIONS}"

    # Replace version by 2 if ${BAREBOX_COMPATIBLE_VERSIONS} is empty
    new_version=${new_version:-2}

    echo "Setting bootloader version to ${new_version}..."
    setfwnr ${new_version}

    return $?
}

__main "$@"
