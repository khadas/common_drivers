#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`

ARCH=arm
DEFCONFIG=meson64_a32_C3_mini_merge_defconfig

if [[ $ARCH == arm64 ]]; then
  OUTDIR=${ROOT_DIR}/out/kernel-5.15-64
else
  OUTDIR=${ROOT_DIR}/out/kernel-5.15-32
fi

KCONFIG_CONFIG=${ROOT_DIR}/common/common_drivers/arch/arm/configs/${DEFCONFIG}
export KCONFIG_CONFIG

if [ -n "$1" ]&&[ "--c3_debug" = "$1" ]; then
echo "make meson64_a32_C3_mini_defconfig & C3_debug_defconfig"
${ROOT_DIR}/common/scripts/kconfig/merge_config.sh -m -r \
	${ROOT_DIR}/common/common_drivers/arch/arm/configs/meson64_a32_C3_mini_defconfig \
	${ROOT_DIR}/common/common_drivers/arch/arm/configs/C3_debug_defconfig
else
echo "make meson64_a32_C3_mini_defconfig"
${ROOT_DIR}/common/scripts/kconfig/merge_config.sh -m -r \
	${ROOT_DIR}/common/common_drivers/arch/arm/configs/meson64_a32_C3_mini_defconfig
fi

export -n KCONFIG_CONFIG
CROSS_COMPILE_TOOL=${ROOT_DIR}/prebuilts/gcc/linux-x86/host/x86_64-arm-10.3-2021.07/bin/arm-none-linux-gnueabihf-

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh $@

rm ${KCONFIG_CONFIG}*
