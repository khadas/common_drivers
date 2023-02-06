#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`

ARCH=arm
DEFCONFIG=amlogic_gx32_defconfig

KCONFIG_CONFIG=${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/${DEFCONFIG}
export KCONFIG_CONFIG

${ROOT_DIR}/common/scripts/kconfig/merge_config.sh -m -r \
	${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/gki_defconfig  \
	${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/amlogic_gki.fragment  \
	${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/amlogic_gki.10  \
	${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/amlogic_gki.debug \
	${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs/amlogic_gcc32_defconfig\

export -n KCONFIG_CONFIG
CROSS_COMPILE_TOOL=${ROOT_DIR}/prebuilts/gcc/linux-x86/host/x86_64-arm-10.3-2021.07/bin/arm-none-linux-gnueabihf-

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh $@
rm ${KCONFIG_CONFIG}*
