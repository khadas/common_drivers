#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo "  --kernel_dir            for KERNEL_DIR, common[default]|other dir, require parameter value"
	echo "  --common_drivers_dir    for COMMON_DRIVERS_DIR, common[default]|other dir, require parameter value"
	echo "  --savedefconfig         for SAVEDEFCONFIG, [default]|1, not require parameter value"
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
	--savedefconfig)
		SAVEDEFCONFIG=1
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

export CROSS_COMPILE=/opt/gcc-linaro-7.3.1-2018.05-x86_64_armv8l-linux-gnueabihf/bin/armv8l-linux-gnueabihf-

ROOTDIR=`pwd`

OUTDIR=${ROOTDIR}/out/kernel-5.15
mkdir -p ${OUTDIR}/common
if [ "${SKIP_RM_OUTDIR}" != "1" ] ; then
	rm -rf ${OUTDIR}
fi

DEFCONFIG=meson64_a32_smarthome_defconfig
cp ${ROOTDIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/arm/configs/${DEFCONFIG} ${ROOTDIR}/${KERNEL_DIR}/arch/arm/configs/
if [[ -n ${SAVEDEFCONFIG} ]]; then
	set -x
	make ARCH=arm -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	make ARCH=arm -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common savedefconfig
	rm ${KERNEL_DIR}/arch/arm/configs/${DEFCONFIG}
	cp -f ${OUTDIR}/common/defconfig  ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/arm/configs/${DEFCONFIG}
	set +x
	exit
fi
set -x
make ARCH=arm -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
make ARCH=arm -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common headers_install
make ARCH=arm -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common uImage -j12 LOADADDR=0x208000
make ARCH=arm -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common modules -j12 LOADADDR=0x208000
make ARCH=arm -C ${ROOTDIR}/${KERNEL_DIR} O=${OUTDIR}/common dtbs -j12 LOADADDR=0x208000
rm ${ROOTDIR}/${KERNEL_DIR}/arch/arm/configs/${DEFCONFIG}
set +x

echo "Build success!"
