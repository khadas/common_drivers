#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo "  --kernel_dir            for KERNEL_DIR, common[default]|other dir, require parameter value"
	echo "  --common_drivers_dir    for COMMON_DRIVERS_DIR, common[default]|other dir, require parameter value"
}

VA=
ARGS=()
for i in "$@"
do
	case $i in
	--kernel_dir)
		KERNEL_DIR=$2
		VA=1
                shift
		;;
	--common_drivers_dir)
		COMMON_DRIVERS_DIR=$2
		VA=1
                shift
		;;
	-h|--help)
		show_help
		exit 0
		;;
	*)
		if [[ -n $1 ]];
		then
			if [[ -z ${VA} ]];
			then
				ARGS+=("$1")
			fi
		fi
		VA=
		shift
		;;
	esac
done
set -- "${ARGS[@]}"		# other parameters are used as script parameters of build_abi.sh or build.sh

if [[ -z "${KERNEL_DIR}" ]]; then
	KERNEL_DIR=common
fi
if [[ ! -f ${KERNEL_DIR}/init/main.c ]]; then
	echo "The directory of kernel does not exist";
	exit
fi
if [[ -z "${COMMON_DRIVERS_DIR}" ]]; then
	if [[ -d ${KERNEL_DIR}/../common_drivers ]]; then
		COMMON_DRIVERS_DIR=../common_drivers
	elif [[ -d "${KERNEL_DIR}/common_drivers" ]]; then
		COMMON_DRIVERS_DIR=common_drivers
	fi
fi
if [[ ! -f ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh ]]; then
	echo "The directory of common_drivers does not exist";
	exit
fi

export KERNEL_DIR COMMON_DRIVERS_DIR

export CROSS_COMPILE=/opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

ROOTDIR=`pwd`

OUTDIR=${ROOTDIR}/out/kernel-5.15/
mkdir -p ${OUTDIR}/common
if [ "${SKIP_RM_OUTDIR}" != "1" ] ; then
	rm -rf ${OUTDIR}
fi

DEFCONFIG=meson64_a64_smarthome_defconfig
cp ${ROOTDIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/arm64/configs/${DEFCONFIG} ${ROOTDIR}/${KERNEL_DIR}/arch/arm64/configs/
set -x
make ARCH=arm64 -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
make ARCH=arm64 -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common headers_install
make ARCH=arm64 -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common Image -j12
make ARCH=arm64 -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common modules -j12
make ARCH=arm64 -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common dtbs -j12
rm ${ROOTDIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG}
set +x

echo "Build success!"
