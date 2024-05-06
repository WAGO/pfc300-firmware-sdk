#!/bin/bash
#-----------------------------------------------------------------------------#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file is part of project fwupdate (PTXdist package wago-fwupdate).
#
# Copyright (c) 2018-2022 WAGO GmbH & Co. KG
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
# Script:   pfc300_switch_boot_partition.sh
#
# Brief:    Used to switch the active system.
#
# Usage:
#    pfc300_switch_boot_partition.sh         - switch to another system
#    pfc300_switch_boot_partition.sh {1|2}   - switch to system[1|2]
# 
# Return: 0 - success, unequal to 0 - failed
#
# Note: Use "rauc status" to see which system is marked as "active"
#
# Author:   MLa: WAGO GmbH & Co. KG
# Author:   KK: WAGO GmbH & Co. KG
# Auhtor:   SD: WAGO GmbH & Co. KG
#-----------------------------------------------------------------------------#
if [ ! -f "/etc/config-tools/board_specific_defines" ]; then
    echo "Missing dependency board_specific_defines"
    exit 64
fi
source "/etc/config-tools/board_specific_defines"

if [ ! -f "/usr/sbin/${BOARD_ID}_defines" ]; then
    echo "Missing dependency ${BOARD_ID}_defines"
    exit 64
fi
source "/usr/sbin/${BOARD_ID}_defines"

SYSTEM_NR=""
PARTITIONS_ONLY=0

declare readonly TOOL="$(basename "$0")"

declare readonly UBOOT_GET="/usr/sbin/fw_printenv -n"
declare readonly UBOOT_SET=/usr/sbin/fw_setenv 

declare BOOT_ORDER
declare -i BOOT_1_LEFT
declare -i BOOT_2_LEFT
declare -i LAST_CHOSEN
declare -i CONFIGURED_SYSTEM_NUMBER




# Function to activate a specified system (will boot on next restart)
#
# Param 1: name of the variable to be set
# Param 2: value of the variable
#-----------------------------------------------------------------------------#
function u_boot_set_variable
{
    local force_ro_backup_0=$(cat /sys/block/mmcblk1boot0/force_ro)
    echo 0 > /sys/block/mmcblk1boot0/force_ro
    local force_ro_backup_1=$(cat /sys/block/mmcblk1boot1/force_ro)
    echo 0 > /sys/block/mmcblk1boot1/force_ro

    $UBOOT_SET "$1" "$2"
    $UBOOT_SET "$1" "$2"

    echo "${force_ro_backup_0}" > /sys/block/mmcblk1boot0/force_ro
    echo "${force_ro_backup_1}" > /sys/block/mmcblk1boot1/force_ro

}



# Function to activate a specified system (will boot on next restart)
#
# Param 1: number of the system which shall be activated
#
# Return: 0 on success, unequal to 0 otherwise
#-----------------------------------------------------------------------------#
function activate_system
{
    local NR=$1
    if [ $NR -eq 1 ]; then
        u_boot_set_variable BOOT_ORDER "1 2"
    elif [ $NR -eq 2 ]; then
        u_boot_set_variable BOOT_ORDER "2 1"
    fi
    
}

# Function to swap priorities in barebox-state variables of both systems.
#
# Return: 0 on success, unequal to 0 otherwise
#-----------------------------------------------------------------------------#
function swap_priorities
{
    dump_uboot_variables
    #swap priorities 
    if [ $CONFIGURED_SYSTEM_NUMBER -eq 1 ]; then
        activate_system 2
    elif [ $CONFIGURED_SYSTEM_NUMBER -eq 2 ]; then
        activate_system 1
    fi
}

# Function to reset remaining attemps in barebox-state variables of both systems.
#
# Return: 0 on success, unequal to 0 otherwise
#-----------------------------------------------------------------------------#
function reset_attempts
{
    u_boot_set_variable BOOT_1_LEFT 3
    u_boot_set_variable BOOT_2_LEFT 3
}

# Function to set uboot variables
# Return: 0 on success, unequal to 0 otherwise
#-----------------------------------------------------------------------------#
function dump_uboot_variables
{
    BOOT_ORDER=$($UBOOT_GET "BOOT_ORDER")
    BOOT_1_LEFT=$($UBOOT_GET "BOOT_1_LEFT")
    BOOT_2_LEFT=$($UBOOT_GET "BOOT_2_LEFT")
    CONFIGURED_SYSTEM_NUMBER=$(echo -n $BOOT_ORDER| cut -d' ' -f1)
    LAST_CHOSEN=$CONFIGURED_SYSTEM_NUMBER
}   

# Function to update partition labels with internal flash emmc.
#
# Note: The inactive system shall be valid before calling this function. It takes
#       the partition infos from /usr/sbin/${BOARD_ID}_defines in the active or 
#       inactive system (depends on $SYSTEM_NR) and the currently running system).
#
# Return: 0 on success, unequal to 0 otherwise
#-----------------------------------------------------------------------------#
function restore_partition_labels
{

    dump_uboot_variables
    local result=0
    local target_rootfs=""
    if [[ "$(/etc/config-tools/get_filesystem_data active-partition-medium)" == "sd-card" ]]; then
        local target_partition=""
        target_rootfs="/tmp/${TOOL}_target_rootfs"
        if [[ $CONFIGURED_SYSTEM_NUMBER -eq 2 ]]; then
            target_partition=$G_EMMC_ROOT2_DEV
        else
            target_partition=$G_EMMC_ROOT1_DEV
        fi
        mkdir -p "$target_rootfs" &&
            mount "$target_partition" "$target_rootfs" -o sync
        if [[ $? -ne 0 ]]; then
             echo "Failed to mount ${target_partition} on ${target_rootfs}"
             result=$INTERNAL_ERROR
        fi
    else #running from internal flash...
        target_rootfs="/"
        if [[ $LAST_CHOSEN -ne $SYSTEM_NR ]]; then
            target_rootfs="/tmp/${TOOL}_target_rootfs"
            # Mount other root filesystem
            mkdir -p "$target_rootfs" &&
                mount -t auto /dev/rootfs_inactive "$target_rootfs" -o sync
            if [[ $? -ne 0 ]]; then
                echo "Failed to mount /dev/rootfs_inactive on ${target_rootfs}"
                result=$INTERNAL_ERROR
            fi
        fi
    fi

    local emmc_partition_devices
    local emmc_partition_labels
    local emmc_partition_fs_types
    if [[ $result -eq 0 ]]; then
        if [[ -e "$target_rootfs/usr/sbin/${BOARD_ID}_defines" ]]; then
            source "$target_rootfs/usr/sbin/${BOARD_ID}_defines"

            #set local variables from sourced ${BOARD_ID}_defines
            emmc_partition_devices=(${EMMC_PARTITION_DEVICES[*]})
            emmc_partition_labels=(${EMMC_PARTITION_LABELS[*]})
            emmc_partition_fs_types=(${EMMC_PARTITION_FS_TYPES[*]})

            if [[ -z "$emmc_partition_devices" ]] ||
               [[ -z "$emmc_partition_labels" ]] ||
               [[ -z "$emmc_partition_fs_types" ]]; then
                echo "Failed to set variables from sourced {BOARD_ID}_defines"
                result=$INTERNAL_ERROR
            fi
        else
            echo "File ${BOARD_ID}_defines doesn't exist on system ${SYSTEM_NR}"
            result=$INTERNAL_ERROR
        fi
    fi

    if [[ -n "$target_rootfs" ]] && [[ "$(realpath "$target_rootfs")" != "/" ]]; then
        # Unmount other root filesystem
        umount "$target_rootfs" &&
            rm -rf "$target_rootfs"
    fi

    if [[ $result -eq 0 ]]; then
        local index=0
        for label in "${emmc_partition_labels[@]}"; do
            local ptypefs=${emmc_partition_fs_types[${index}]}
            local device="/dev/${emmc_partition_devices[${index}]}"
            if [[ $ptypefs =~ ^ext[2-4]$ ]]; then
                /sbin/tune2fs -L "$label" "$device" &>/dev/null
                result=$((result == 0 ? result : $?))
            elif [[ $ptypefs == "vfat" ]]; then
                /sbin/fatlabel "$device" "$label" &>/dev/null
                result=$((result == 0 ? result : $?))
            fi
            index=$((index+1))
        done
        if [[ $result -ne 0 ]]; then
            echo "Error during update of partition labels"
        fi
    fi
    return $result
}


# Function to evaluate the parameters of this tool.
#
# Params: all parameters passt to this tool ($@)
# Return: 0 on success, unequal to 0 otherwise
#-----------------------------------------------------------------------------#
function evaluate_params
{
    local result=0
    if [[ $# -gt 2 ]]; then
        result=$INVALID_PARAMETER
    else
        PARTITIONS_ONLY=$(get_parameter_option "-p" "-p" "$@")
        if [[ $? -ne 0 ]] || [[ $PARTITIONS_ONLY -eq 0 ]]; then # -p not found => 0 or 1 parameters shall be passed
            if [[ $# -lt 2 ]]; then
                SYSTEM_NR=${1:-}
            else
                result=$INVALID_PARAMETER
            fi
        else # -p was found => 1 or 2 parameters shall be passed
            if [[ $# -ne 0 ]]; then
                if [[ $# -eq 1 ]]; then
                    SYSTEM_NR=""
                else
                    SYSTEM_NR=$1
                fi
            else
                result=$INVALID_PARAMETER
            fi
        fi
    fi
}

# Function to print usage of this tool.
#
# Return: 0 on success, unequal to 0 otherwise
#-----------------------------------------------------------------------------#
function print_info
{ 
    echo "Usage:"
    echo "     $TOOL [options]       - switch to another system"
    echo "     $TOOL {1|2} [options] - switch to system[1|2]"
    echo ""
    echo "Options (always at the end of the argument list):"
    echo "     -p:          - do not switch, update partitions only"
}

evaluate_params $@
if [[ $? -ne 0 ]]; then
    print_info
    exit $INVALID_PARAMETER
else
    case "$SYSTEM_NR" in
      "1" | "2")
        activate_system $SYSTEM_NR
        reset_attempts
        restore_partition_labels
        ;;

      "")
        SYSTEM_NR=$(/etc/config-tools/get_systeminfo inactive-system-number)
        if [[ $? -ne 0 ]]; then
          echo "Failed to get inactive-system-number"
          exit $INTERNAL_ERROR
        fi
        swap_priorities
        reset_attempts
        restore_partition_labels
        ;;

      *)
        print_info
        exit $INVALID_PARAMETER
        ;;
    esac
fi