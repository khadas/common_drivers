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

source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh
source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/amlogic_smarthome_utils.sh

copy_pre_commit

handle_input_parameters_for_smarthome "$@"

export KERNEL_DIR COMMON_DRIVERS_DIR MANUAL_INSMOD_MODULE

set_default_parameters_for_smarthome

auto_patch_to_common_dir

mkdir -p ${DIST_DIR} ${MODULES_STAGING_DIR}

savedefconfig_cmd_for_smarthome

only_build_dtb_for_smarthome

make_menuconfig_cmd_for_smarthome

build_kernel_for_different_cpu_architecture

eval ${POST_KERNEL_BUILD_CMDS}
build_ext_modules
eval ${EXTRA_CMDS}

copy_modules_and_rebuild_rootfs_for_smarthome


