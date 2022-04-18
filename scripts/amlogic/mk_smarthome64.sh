#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`

ARCH=arm64
DEFCONFIG=meson64_a64_smarthome_defconfig
CROSS_COMPILE_TOOL=${ROOT_DIR}/prebuilts/gcc/linux-x86/host/x86_64-aarch64-10.3-2021.07/bin/aarch64-none-linux-gnu-

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh $@
