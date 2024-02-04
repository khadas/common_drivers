#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#
function handle_input_parameters_for_smarthome () {
	VA=
	ARGS=()
	for i in "$@"
	do
		case $i in
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
}

export -f handle_input_parameters_for_smarthome

function set_default_parameters_for_smarthome () {
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
				PATH=${ROOT_DIR}/${prebuilt_bin}:${PATH} # add the clang tool to env PATH
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
		tool_args+=("LOADADDR=0x108000")
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


	source ${ROOT_DIR}/build/kernel/build_utils.sh

	DTS_EXT_DIR=${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/boot/dts/amlogic
	DTS_EXT_DIR=$(real_path ${ROOT_DIR}/${DTS_EXT_DIR} ${KERNEL_DIR})
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
}
export -f set_default_parameters_for_smarthome

function savedefconfig_cmd_for_smarthome () {
	if [[ -n ${SAVEDEFCONFIG} ]]; then
		set -x
		make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${DEFCONFIG}
		make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} savedefconfig
		rm ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
		cp -f ${OUT_DIR}/defconfig  ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
		set +x
		exit
	fi
}
export -f savedefconfig_cmd_for_smarthome

function only_build_dtb_for_smarthome () {
	if [[ -n ${DTB} ]]; then
		set -x
		make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} ${DEFCONFIG}
		make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} dtbs || exit
		set +x
		exit
	fi
}
export -f only_build_dtb_for_smarthome

function make_menuconfig_cmd_for_smarthome () {
	if [[ -n ${MENUCONFIG} ]]; then
		set -x
		make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} ${DEFCONFIG}
		make ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUT_DIR} ${TOOL_ARGS} menuconfig
		set +x
		exit
	fi
}
export -f make_menuconfig_cmd_for_smarthome

function copy_modules_and_rebuild_rootfs_for_smarthome () {

	copy_modules_files_to_dist_dir

	if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
		echo "========================================================"
		echo "Rebuild rootfs in order to install modules!"
		rebuild_rootfs ${ARCH}
		echo "Build success!"
	else
		echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
	fi
}
export -f copy_modules_and_rebuild_rootfs_for_smarthome
