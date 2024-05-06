#!/bin/bash
#-----------------------------------------------------------------------------#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file is part of PTXdist package wago-custom-install.
#
# Copyright (c) 2017-2022 WAGO GmbH & Co. KG
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
# Script:   rauc_failsafe
#
# Brief:    uboot specific "rauc" wrapper: makes boot areas writeable during rauc calls to interact with the bootloader.
#
# Author:   MLa: WAGO GmbH & Co. KG
# Author:   SD: WAGO GmbH & Co. KG
# Author:   KK: WAGO GmbH & Co. KG
#-----------------------------------------------------------------------------#

declare readonly RAUC="/usr/bin/rauc"
declare readonly RAUC_BAREBOX_LOCK="/var/lock/rauc_barebox.lock" 

function rauc_status_fail_safe
{ 
    flock "${RAUC_BAREBOX_LOCK}" $RAUC "$@"
}

function main
{ 
    if [[ ! -e "$RAUC_BAREBOX_LOCK" ]]; then
        touch "$RAUC_BAREBOX_LOCK"
    fi
    
    local force_ro_backup_0=$(cat /sys/block/mmcblk1boot0/force_ro)    
    echo 0 > /sys/block/mmcblk1boot0/force_ro
    local force_ro_backup_1=$(cat /sys/block/mmcblk1boot1/force_ro)
    echo 0 > /sys/block/mmcblk1boot1/force_ro

    rauc_status_fail_safe $@
    local RESULT=$?

    echo "${force_ro_backup_0}" > /sys/block/mmcblk1boot0/force_ro
    echo "${force_ro_backup_1}" > /sys/block/mmcblk1boot1/force_ro
    
    return $RESULT
}

main $@
