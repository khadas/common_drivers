#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`
ARCH=arm
CROSS_COMPILE_TOOL=${ROOT_DIR}/prebuilts/gcc/linux-x86/host/x86_64-arm-10.3-2021.07/bin/arm-none-linux-gnueabihf-

OPTION_PARAM="$*"
if [[ "${OPTION_PARAM}" =~ "--all" ]]; then
	echo "make meson64_a32_zapper_defconfig and meson64_a32_zapper_ext_defconfig"
	DEBUG=1
	OPTION_PARAM=${OPTION_PARAM//--all/}
	DEFCONFIG=meson64_a32_zapper_all_defconfig
	CONFIG_DIR=${ROOT_DIR}/common/common_drivers/arch/${ARCH}/configs
	KCONFIG_CONFIG=${CONFIG_DIR}/${DEFCONFIG} ${ROOT_DIR}/common/scripts/kconfig/merge_config.sh -m -r \
		${CONFIG_DIR}/meson64_a32_zapper_defconfig \
		${CONFIG_DIR}/meson64_a32_zapper_ext_defconfig
else
	echo "make meson64_a32_zapper_defconfig"
	DEFCONFIG=meson64_a32_zapper_defconfig
fi

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh ${OPTION_PARAM}

if [[ "${DEBUG}" == "1" ]]; then
	rm ${CONFIG_DIR}/${DEFCONFIG}
fi
