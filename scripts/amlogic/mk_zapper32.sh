#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`

ARCH=arm
if [ "$1" = "--all" ]; then
DEFCONFIG=meson64_a32_zapper_all_defconfig
else
DEFCONFIG=meson64_a32_zapper_defconfig
fi

KCONFIG_CONFIG=${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/${DEFCONFIG}

if [ "$1" = "--all" ]; then
export KCONFIG_CONFIG

${ROOT_DIR}/common/scripts/kconfig/merge_config.sh -m -r \
	${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/meson64_a32_zapper_defconfig  \
	${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/meson64_a32_zapper_ext_defconfig  \

fi

export -n KCONFIG_CONFIG
CROSS_COMPILE_TOOL=${ROOT_DIR}/prebuilts/gcc/linux-x86/host/x86_64-arm-10.3-2021.07/bin/arm-none-linux-gnueabihf-

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh $@

if [ "$1" = "--all" ]; then
	rm ${KCONFIG_CONFIG}*
fi

