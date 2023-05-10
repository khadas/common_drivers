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
	echo "  --manual_insmod_module  for insmod ko manually when kernel is booting.It's usually used in debug test"
	echo "  --patch                 for only am patches"
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
	--dtb)
		DTB=1
		shift
		;;
	--manual_insmod_module)
		MANUAL_INSMOD_MODULE=1
		shift
		;;
	--patch)
		ONLY_PATCH=1
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

export KERNEL_DIR COMMON_DRIVERS_DIR MANUAL_INSMOD_MODULE

version_message=$(grep -rn BRANCH= ${KERNEL_DIR}/build.config.constants)
version_message="common${version_message##*android}"
if [[ -n ${FULL_KERNEL_VERSION} ]]; then
	if [[ "${FULL_KERNEL_VERSION}" != "${version_message}" ]]; then
		echo "kernel version is not match!!"
		exit
	fi
else
	FULL_KERNEL_VERSION=${version_message}
fi

if [[ -f ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/auto_patch/auto_patch.sh ]]; then
	${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/auto_patch/auto_patch.sh ${FULL_KERNEL_VERSION}
fi
if [[ ${ONLY_PATCH} -eq "1" ]]; then
	exit
fi

tool_args=()
prebuilts_paths=(
	CLANG_PREBUILT_BIN
	#BUILDTOOLS_PREBUILT_BIN
)
echo CC_CLANG=$CC_CLANG
if [[ $CC_CLANG -eq "1" ]]; then
	source ${ROOT_DIR}/${KERNEL_DIR}/build.config.common
	if [[ -n "${LLVM}" ]]; then
		tool_args+=("LLVM=1")
	fi
	#if [ -n "${DTC}" ]; then
	#	tool_args+=("DTC=${DTC}")
	#fi
	for prebuilt_bin in "${prebuilts_paths[@]}"; do
		prebuilt_bin=\${${prebuilt_bin}}
		eval prebuilt_bin="${prebuilt_bin}"
		if [ -n "${prebuilt_bin}" ]; then
			PATH=${PATH//"${ROOT_DIR}\/${prebuilt_bin}:"}
			PATH=${ROOT_DIR}/${prebuilt_bin}:${PATH}
		fi
	done
	export PATH
elif [[ -n $CROSS_COMPILE_TOOL ]]; then
	export CROSS_COMPILE=${CROSS_COMPILE_TOOL}
fi

if [[ $ARCH == arm64 ]]; then
	OUTDIR=${ROOT_DIR}/out/kernel-5.15-64
elif [[ $ARCH == arm ]]; then
	OUTDIR=${ROOT_DIR}/out/kernel-5.15-32
	tool_args+=("LOADADDR=0x208000")
elif [[ $ARCH == riscv ]]; then
	OUTDIR=${ROOT_DIR}/out/riscv-kernel-5.15-64
fi
TOOL_ARGS="${tool_args[@]}"

OUT_DIR=${OUTDIR}/common
mkdir -p ${OUT_DIR}
if [ "${SKIP_RM_OUTDIR}" != "1" ] ; then
	rm -rf ${OUTDIR}
fi

echo "========================================================"
echo ""
export DIST_DIR=$(readlink -m ${OUTDIR}/dist)
export MODULES_STAGING_DIR=$(readlink -m ${OUTDIR}/staging)
export OUT_AMLOGIC_DIR=$(readlink -m ${OUTDIR}/amlogic)
echo OUTDIR=$OUTDIR DIST_DIR=$DIST_DIR MODULES_STAGING_DIR=$MODULES_STAGING_DIR KERNEL_DIR=$KERNEL_DIR

source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh
source ${ROOT_DIR}/build/kernel/build_utils.sh

DTS_EXT_DIR=${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/boot/dts/amlogic
DTS_EXT_DIR=$(rel_path ${ROOT_DIR}/${DTS_EXT_DIR} ${KERNEL_DIR})
export dtstree=${DTS_EXT_DIR}
export DTC_INCLUDE=${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/include

EXT_MODULES="
	${EXT_MODULES}
"

EXT_MODULES_CONFIG="
	${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/ext_modules_config
"

EXT_MODULES_PATH="
	${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/ext_modules_path
"

POST_KERNEL_BUILD_CMDS="prepare_module_build"
EXTRA_CMDS="extra_cmds"

IN_KERNEL_MODULES=1

mkdir -p ${DIST_DIR} ${MODULES_STAGING_DIR}

cp ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/arch/${ARCH}/configs/
if [[ -n ${SAVEDEFCONFIG} ]]; then
	set -x
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${DEFCONFIG}
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} savedefconfig
	rm ${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
	cp -f ${OUT_DIR}/defconfig  ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
	set +x
	exit
fi

if [[ -n ${DTB} ]]; then
        set -x
        make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} ${DEFCONFIG}
        make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} dtbs || exit
        set +x
        exit
fi
if [[ -n ${MENUCONFIG} ]]; then
	set -x
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${DEFCONFIG}
	make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} menuconfig
	set +x
	exit
fi
set -x
if [[ $ARCH == arm64 ]]; then
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} ${DEFCONFIG}
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} headers_install &&
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} Image -j12 &&
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} modules -j12 &&
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} INSTALL_MOD_PATH=${MODULES_STAGING_DIR} INSTALL_MOD_STRIP=1 modules_install -j12 &&
	make ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} dtbs -j12 || exit
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG}
elif [[ $ARCH == arm ]]; then
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} ${DEFCONFIG}
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} headers_install &&
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} uImage -j12 &&
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} modules -j12 &&
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} INSTALL_MOD_PATH=${MODULES_STAGING_DIR} INSTALL_MOD_STRIP=1 modules_install -j12 &&
	make ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} dtbs -j12 || exit
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm/configs/${DEFCONFIG}
elif [[ $ARCH == riscv ]]; then
	make ARCH=riscv -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} ${DEFCONFIG}
	make ARCH=riscv -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} headers_install &&
	make ARCH=riscv -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} Image -j12 &&
	make ARCH=riscv -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} modules -j12 &&
	make ARCH=riscv -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} INSTALL_MOD_PATH=${MODULES_STAGING_DIR} modules_install -j12 &&
	make ARCH=riscv -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} dtbs -j12 || exit
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/riscv/configs/${DEFCONFIG}
fi
cp ${OUT_DIR}/arch/${ARCH}/boot/Image* ${DIST_DIR}
cp ${OUT_DIR}/arch/${ARCH}/boot/uImage* ${DIST_DIR}
cp ${OUT_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/boot/dts/amlogic/*.dtb ${DIST_DIR}
cp ${OUT_DIR}/vmlinux ${DIST_DIR}
set +x

function build_ext_modules() {
	for EXT_MOD in ${EXT_MODULES}; do
		EXT_MOD_REL=$(rel_path ${ROOT_DIR}/${EXT_MOD} ${KERNEL_DIR})
		mkdir -p ${OUT_DIR}/${EXT_MOD_REL}

		set -x
		make ARCH=${ARCH} -C ${ROOT_DIR}/${EXT_MOD} M=${EXT_MOD_REL} KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}  \
				O=${OUT_DIR} ${TOOL_ARGS} -j12 || exit
		make ARCH=${ARCH} -C ${ROOT_DIR}/${EXT_MOD} M=${EXT_MOD_REL} KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}  \
				O=${OUT_DIR} ${TOOL_ARGS} ${MODULE_STRIP_FLAG}		\
				INSTALL_MOD_PATH=${MODULES_STAGING_DIR}			\
				INSTALL_MOD_DIR="extra/${EXT_MOD}"			\
				INSTALL_MOD_STRIP=1					\
				modules_install -j12 || exit
		set +x
	done
}

eval ${POST_KERNEL_BUILD_CMDS}
build_ext_modules
eval ${EXTRA_CMDS}

MODULES=$(find ${MODULES_STAGING_DIR} -type f -name "*.ko")
if [ -n "${MODULES}" ]; then
	if [ -n "${IN_KERNEL_MODULES}" -o -n "${EXT_MODULES}" -o -n "${EXT_MODULES_MAKEFILE}" ]; then
		echo "========================================================"
		echo " Copying modules files"
		for module in ${MODULES}; do
			cp ${module} ${DIST_DIR}
		done
		if [ "${COMPRESS_MODULES}" = "1" ]; then
			echo " Archiving modules to ${MODULES_ARCHIVE}"
			tar --transform="s,.*/,," -czf ${DIST_DIR}/${MODULES_ARCHIVE} ${MODULES[@]}
		fi
	fi

	if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
		echo "Rebuild rootfs in order to install modules!"
		rebuild_rootfs ${ARCH}
		echo "Build success!"
	else
		echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
	fi
fi
