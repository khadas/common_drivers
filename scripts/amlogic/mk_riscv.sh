#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`

ARCH=riscv
DEFCONFIG=meson64_smarthome_defconfig
CROSS_COMPILE_TOOL=${ROOT_DIR}/prebuilts/gcc/linux-x86/host/nuclei_riscv_glibc_prebuilt_linux64_2022.04/bin/riscv-nuclei-linux-gnu-

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh $@
