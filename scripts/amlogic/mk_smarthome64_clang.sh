#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

ROOT_DIR=`pwd`

ARCH=arm64
DEFCONFIG=meson64_a64_smarthome_defconfig
CROSS_COMPILE_TOOL=/opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

CC_CLANG=1
if [ $CC_CLANG -eq 1 ];then
	. ${ROOT_DIR}/common/build.config.constants
	export PATH=${ROOT_DIR}/prebuilts/clang/host/linux-x86/clang-${CLANG_VERSION}/bin:$PATH
	MAKE='make CC=clang HOSTCC=clang LD=ld.lld NM=llvm-nm OBJCOPY=llvm-objcopy -j8'
else
	MAKE='make'
fi

source ${ROOT_DIR}/common/common_drivers/scripts/amlogic/mk_smarthome_common.sh $@
