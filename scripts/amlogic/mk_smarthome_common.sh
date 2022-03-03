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
	echo "  --menuconfig            for MENUCONFIG, [default]|1, not require parameter value"
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
	--menuconfig)
		MENUCONFIG=1
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
export CROSS_COMPILE=${CROSS_COMPILE_TOOL}

if [[ $ARCH == arm64 ]]; then
	OUTDIR=${ROOT_DIR}/out/kernel-5.15-64
else
	OUTDIR=${ROOT_DIR}/out/kernel-5.15-32
fi

mkdir -p ${OUTDIR}/common
if [ "${SKIP_RM_OUTDIR}" != "1" ] ; then
	rm -rf ${OUTDIR}
fi

source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh

echo "========================================================"
echo ""
export DIST_DIR=$(readlink -m ${OUTDIR}/dist)
export MODULES_STAGING_DIR=$(readlink -m ${OUTDIR}/staging)
echo OUTDIR=$OUTDIR DIST_DIR=$DIST_DIR MODULES_STAGING_DIR=$MODULES_STAGING_DIR KERNEL_DIR=$KERNEL_DIR

mkdir -p ${DIST_DIR} ${MODULES_STAGING_DIR}

cp ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/arch/${ARCH}/configs/
if [[ -n ${SAVEDEFCONFIG} ]]; then
	set -x
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common savedefconfig
	rm ${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
	cp -f ${OUTDIR}/common/defconfig  ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
	set +x
	exit
fi
if [[ -n ${MENUCONFIG} ]]; then
	set -x
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common menuconfig
	set +x
	exit
fi
set -x
if [[ $ARCH == arm64 ]]; then
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common headers_install
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common Image -j12
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common modules -j12
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common INSTALL_MOD_PATH=${MODULES_STAGING_DIR} modules_install -j12
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common dtbs -j12
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG}
else
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common headers_install
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common uImage -j12 LOADADDR=0x208000
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common modules -j12 LOADADDR=0x208000
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common INSTALL_MOD_PATH=${MODULES_STAGING_DIR} modules_install -j12
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common dtbs -j12 LOADADDR=0x208000
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm/configs/${DEFCONFIG}
fi
set +x

IN_KERNEL_MODULES=1
BUILD_INITRAMFS=1
MODULES_LOAD_FIRSTLIST=(
	amlogic-clk
)

INITRAMFS_STAGING_DIR=${MODULES_STAGING_DIR}/initramfs_staging
rm -rf ${INITRAMFS_STAGING_DIR}

source ${ROOT_DIR}/build/kernel/build_utils.sh

MODULES=$(find ${MODULES_STAGING_DIR} -type f -name "*.ko")
if [ -n "${MODULES}" ]; then
  if [ -n "${IN_KERNEL_MODULES}" -o -n "${EXT_MODULES}" -o -n "${EXT_MODULES_MAKEFILE}" ]; then
    echo "========================================================"
    echo " Copying modules files"
    cp -p ${MODULES} ${DIST_DIR}
    if [ "${COMPRESS_MODULES}" = "1" ]; then
      echo " Archiving modules to ${MODULES_ARCHIVE}"
      tar --transform="s,.*/,," -czf ${DIST_DIR}/${MODULES_ARCHIVE} ${MODULES[@]}
    fi
  fi
  if [ "${BUILD_INITRAMFS}" = "1" ]; then
    echo "========================================================"
    echo " Creating initramfs"
    create_modules_staging "${MODULES_LIST}" ${MODULES_STAGING_DIR} \
      ${INITRAMFS_STAGING_DIR} "${MODULES_BLOCKLIST}" "-e"

    MODULES_ROOT_DIR=$(echo ${INITRAMFS_STAGING_DIR}/lib/modules/*)
    cp ${MODULES_ROOT_DIR}/modules.load ${DIST_DIR}/modules.load
    cp ${MODULES_ROOT_DIR}/modules.load ${DIST_DIR}/vendor_boot.modules.load
    echo "${MODULES_OPTIONS}" > ${MODULES_ROOT_DIR}/modules.options
  fi
fi

echo "========================================================"
echo "prepare modules"

modules_install
if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
	echo "Rebuild rootfs in order to install modules!"
	rebuild_rootfs ${ARCH}
else
	echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
fi

echo "Build success!"
