#!/bin/bash

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo
	echo "  --arch                  for ARCH, build 64 or 32 bit kernel, arm|arm64[default], require parameter value"
	echo "  --abi                   for ABI, call build_abi.sh not build.sh, 1|0[default], not require parameter value"
	echo "  --build_config          for BUILD_CONFIG, common_drivers/build.config.amlogic[default]|common/build.config.gki.aarch64, require parameter value"
	echo "  --symbol_strict         for KMI_SYMBOL_LIST_STRICT_MODE, 1[default]|0, require parameter value"
	echo "  --lto                   for LTO, full|thin[default]|none, require parameter value"
	echo "  --menuconfig            for only menuconfig, not require parameter value"
	echo "  --basicconfig           for basicconfig, m(menuconfig)[default]|n"
	echo "  --image                 for only build kernel, not require parameter value"
	echo "  --modules               for only build modules, not require parameter value"
	echo "  --dtbs                  for only build dtbs, not require parameter value"
	echo "  --kernel_dir            for KERNEL_DIR, common[default]|other dir, require parameter value"
	echo "  --common_drivers_dir    for COMMON_DRIVERS_DIR, common[default]|other dir, require parameter value"
	echo "  --build_dir             for BUILD_DIR, build[default]|other dir, require parameter value"
	echo "  --check_defconfig       for check defconfig"
	echo "  --modules_depend        for check modules depend"
	echo "  --android_project       for android project build"
	echo "  --gki_20                for build gki 2.0 kernel:   gki_defconfig + amlogic_gki.fragment"
	echo "  --gki_10                for build gki 1.0 kernel:   gki_defconfig + amlogic_gki.fragment + amlogic_gki.10"
	echo "  --gki_debug             for build gki debug kernel: gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug"
	echo "                          for note: current can't use --gki_10, amlogic_gki.10 for optimize, amlogic_gki.debug for debug, and follow GKI1.0"
	echo "                                    so build GKI1.0 Image need with --gki_debug, default parameter --gki_debug"
	echo "  --fast_build            for fast build"
	echo "  --upgrade               for android upgrade builtin module optimize vendor_boot size"
	echo "  --android_version       for android version"
	echo "  --manual_insmod_module  for insmod ko manually when kernel is booting.It's usually used in debug test"
	echo "  --patch                 for only am patches"
	echo "  --check_gki_20          for gki 2.0 check kernel build"
	echo "  --dev_config            for use the config specified by oem instead of amlogic like ./mk.sh --dev_config a_config+b_config+c_config"
	echo "  --bazel                 for choose bazel tool to build"
}

VA=
ARGS=()
for i in "$@"
do
	case $i in
	--arch)
		ARCH=$2
		VA=1
		shift
		;;
	--abi)
		ABI=1
		shift
		;;
	--build_config)
		BUILD_CONFIG=$2
		VA=1
		shift
		;;
	--lto)
		LTO=$2
		VA=1
                shift
		;;
	--symbol_strict)
		KMI_SYMBOL_LIST_STRICT_MODE=$2
		VA=1
                shift
		;;
	--menuconfig)
		MENUCONFIG=1
		shift
		;;
	--basicconfig)
		if [ "$2" = "m" ] || [ "$2" = "n" ]; then
			BASICCONFIG=$2
		else
			BASICCONFIG="m"
		fi
		VA=1
		shift
		;;
	--image)
		IMAGE=1
		shift
		;;
	--modules)
		MODULES=1
		shift
		break
		;;
	--dtbs)
		DTB_BUILD=1
		shift
		;;
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
	--build_dir)
		BUILD_DIR=$2
		VA=1
                shift
		;;
	--check_defconfig)
		CHECK_DEFCONFIG=1
		shift
		;;
	--modules_depend)
		MODULES_DEPEND=1
		shift
		;;
	--android_project)
		ANDROID_PROJECT=$2
		VA=1
		shift
		;;
	--gki_20)
		GKI_CONFIG=gki_20
		shift
		;;
	--gki_10)
		GKI_CONFIG=gki_10
		shift
		;;
	--gki_debug)
		GKI_CONFIG=gki_debug
		shift
		;;
	--fast_build)
		FAST_BUILD=1
		shift
		;;
	--upgrade)
		UPGRADE_PROJECT=1
		ANDROID_VERSION=$2
		VA=1
		shift
		;;
	--manual_insmod_module)
		MANUAL_INSMOD_MODULE=1
		shift
		;;
	--patch)
		ONLY_PATCH=1
		PATCH_PARM=$2
		if [[ "${PATCH_PARM}" == "lunch" ]]; then
			VA=1
		fi
		shift
		;;
	--check_gki_20)
		CHECK_GKI_20=1
		GKI_CONFIG=gki_20
		LTO=none
		shift
		;;
	--dev_config)
		DEV_CONFIGS=$2
		VA=1
		shift
		;;
	--bazel)
		BAZEL=1
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

if [ "${ARCH}" = "arm" ]; then
	ARGS+=("LOADADDR=0x108000")
else
	ARCH=arm64
fi

set -- "${ARGS[@]}"		# other parameters are used as script parameters of build_abi.sh or build.sh
				# amlogic parameters default value
if [[ ${ONLY_PATCH} -eq "1" ]]; then
	CURRENT_DIR=`pwd`
	cd $(dirname $0)
fi

if [[ -z "${ABI}" ]]; then
	ABI=0
fi
if [[ -z "${LTO}" ]]; then
	LTO=thin
fi
if [[ -n ${CHECK_GKI_20} && -z ${ANDROID_PROJECT} ]]; then
	ANDROID_PROJECT=ohm
fi
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
if [[ -z "${BUILD_CONFIG}" ]]; then
	if [ "${ARCH}" = "arm64" ]; then
			BUILD_CONFIG=${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/build.config.amlogic
	elif [ "${ARCH}" = "arm" ]; then
			BUILD_CONFIG=${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/build.config.amlogic32
	fi
fi
if [[ -z "${BUILD_DIR}" ]]; then
	BUILD_DIR=build
fi

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

#first auto patch when param parse end
if [[ -f ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/auto_patch/auto_patch.sh ]]; then
	export PATCH_PARM
	${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/auto_patch/auto_patch.sh ${FULL_KERNEL_VERSION}
fi
if [[ ${ONLY_PATCH} -eq "1" ]]; then
	cd ${CURRENT_DIR}
	exit
fi

if [[ ! -f ${BUILD_DIR}/build_abi.sh ]]; then
	echo "The directory of build does not exist";
	exit
fi

ROOT_DIR=$(readlink -f $(dirname $0))
if [[ ! -f ${ROOT_DIR}/${KERNEL_DIR}/init/main.c ]]; then
	ROOT_DIR=`pwd`
	if [[ ! -f ${ROOT_DIR}/${KERNEL_DIR}/init/main.c ]]; then
		echo "the file path of $0 is incorrect"
		exit
	fi
fi
export ROOT_DIR

CHECK_DEFCONFIG=${CHECK_DEFCONFIG:-0}
MODULES_DEPEND=${MODULES_DEPEND:-0}
if [[ ! -f ${KERNEL_BUILD_VAR_FILE} ]]; then
	export KERNEL_BUILD_VAR_FILE=`mktemp /tmp/kernel.XXXXXXXXXXXX`
	RM_KERNEL_BUILD_VAR_FILE=1
fi

GKI_CONFIG=${GKI_CONFIG:-gki_debug}

export CROSS_COMPILE=

set -e

source "${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh"

export_env_variable

adjust_config_action

build_part_of_kernel

if [[ "${FULL_KERNEL_VERSION}" != "common13-5.15" && "${ARCH}" = "arm64" && ${BAZEL} == 1 ]]; then
	args="$@ --lto=${LTO}"
	PROJECT_DIR=${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/project
	[[ -d ${PROJECT_DIR} ]] || mkdir -p ${PROJECT_DIR}

	pushd ${ROOT_DIR}/${KERNEL_DIR}
	git checkout android/abi_gki_aarch64_amlogic
	cat ${COMMON_DRIVERS_DIR}/android/${FULL_KERNEL_VERSION}_abi_gki_aarch64_amlogic >> android/abi_gki_aarch64_amlogic
	cat ${COMMON_DRIVERS_DIR}/android/${FULL_KERNEL_VERSION}_abi_gki_aarch64_amlogic.10 >> android/abi_gki_aarch64_amlogic
	cat ${COMMON_DRIVERS_DIR}/android/${FULL_KERNEL_VERSION}_abi_gki_aarch64_amlogic.debug >> android/abi_gki_aarch64_amlogic
	cat ${COMMON_DRIVERS_DIR}/android/${FULL_KERNEL_VERSION}_abi_gki_aarch64_amlogic.illegal >> android/abi_gki_aarch64_amlogic
	popd

	if [[ ! -f ${PROJECT_DIR}/build.config.project ]]; then
		touch ${PROJECT_DIR}/build.config.project
		echo "# SPDX-License-Identifier: GPL-2.0" 	>  ${PROJECT_DIR}/build.config.project
		echo 						>> ${PROJECT_DIR}/build.config.project
	fi

	[[ -f ${PROJECT_DIR}/build.config.gki10 ]] || touch ${PROJECT_DIR}/build.config.gki10
	echo "# SPDX-License-Identifier: GPL-2.0" 	>  ${PROJECT_DIR}/build.config.gki10
	echo 						>> ${PROJECT_DIR}/build.config.gki10
	echo "GKI_CONFIG=${GKI_CONFIG}"			>> ${PROJECT_DIR}/build.config.gki10
	echo "ANDROID_PROJECT=${ANDROID_PROJECT}"	>> ${PROJECT_DIR}/build.config.gki10
	echo "GKI_BUILD_CONFIG_FRAGMENT=${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/build.config.amlogic.fragment.bazel" >> ${PROJECT_DIR}/build.config.gki10
	echo "COMMON_DRIVERS_DIR=${COMMON_DRIVERS_DIR}" >> ${PROJECT_DIR}/build.config.gki10

	if [[ ${GKI_CONFIG} == gki_20 ]]; then
		[[ -n ${ANDROID_PROJECT} ]] && sed -i "/GKI_BUILD_CONFIG_FRAGMENT/d" ${PROJECT_DIR}/build.config.gki10
	else
		args="${args} --allow_undeclared_modules"
	fi

	if [[ ! -f ${PROJECT_DIR}/project.bzl ]]; then
		touch ${PROJECT_DIR}/project.bzl
		echo "# SPDX-License-Identifier: GPL-2.0" 	>  ${PROJECT_DIR}/project.bzl
		echo 						>> ${PROJECT_DIR}/project.bzl

		echo "AMLOGIC_MODULES_ANDROID = [" 		>> ${PROJECT_DIR}/project.bzl
		echo "    \"common_drivers/drivers/tty/serial/amlogic-uart.ko\","	>> ${PROJECT_DIR}/project.bzl
		echo "]" 					>> ${PROJECT_DIR}/project.bzl

		echo 						>> ${PROJECT_DIR}/project.bzl
		echo "EXT_MODULES_ANDROID = [" 			>> ${PROJECT_DIR}/project.bzl
		echo "]" 					>> ${PROJECT_DIR}/project.bzl
	fi

	if [[ -z ${ANDROID_PROJECT} ]]; then
		sed -i "/amlogic-uart.ko/d" ${PROJECT_DIR}/project.bzl
	else
		echo 						>> ${PROJECT_DIR}/project.bzl
		echo "ANDROID_PROJECT = \"${ANDROID_PROJECT}\"" >> ${PROJECT_DIR}/project.bzl
	fi
	sed -i "/GKI_CONFIG/d" ${PROJECT_DIR}/project.bzl
	echo "GKI_CONFIG = \"${GKI_CONFIG}\""			>> ${PROJECT_DIR}/project.bzl

	[[ -f ${PROJECT_DIR}/dtb.bzl ]] || touch ${PROJECT_DIR}/dtb.bzl
	echo "# SPDX-License-Identifier: GPL-2.0" 	>  ${PROJECT_DIR}/dtb.bzl
	echo 						>> ${PROJECT_DIR}/dtb.bzl

	echo "AMLOGIC_DTBS = ["				>> ${PROJECT_DIR}/dtb.bzl
	cat  ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/boot/dts/amlogic/Makefile | grep -n "dtb" | cut -d "=" -f 2 | sed 's/[[:space:]][[:space:]]*/ /g' | sed 's/^[ ]*//' | sed 's/[ ]*$//' | sed '/^#/d;/^$/d' | sed 's/^/    "/' | sed 's/$/",/' >> ${PROJECT_DIR}/dtb.bzl
	echo "]"					>> ${PROJECT_DIR}/dtb.bzl

	echo args=${args}
	if [ "${ABI}" -eq "1" ]; then
		tools/bazel run //common:amlogic_abi_update_symbol_list --sandbox_debug --verbose_failures ${args}
		tools/bazel run //common:kernel_aarch64_abi_dist --sandbox_debug --verbose_failures ${args}
		exit
	else
		tools/bazel run //common:amlogic_dist --sandbox_debug --verbose_failures ${args}
	fi

	sed -i "/GKI_BUILD_CONFIG_FRAGMENT/d" ${PROJECT_DIR}/build.config.gki10

	echo "========================================================"
	echo "after compiling with bazel and organizing the document"
	source ${KERNEL_DIR}/build.config.constants
	export COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}})
	export DIST_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})
	source ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh

	bazel_extra_cmds
else
	if [ "${ABI}" -eq "1" ]; then
		export OUT_DIR_SUFFIX="_abi"
	else
		OUT_DIR_SUFFIX=
	fi

	if [ "${ABI}" -eq "1" ]; then
		${ROOT_DIR}/${BUILD_DIR}/build_abi.sh "$@"
	else
		if [[ "${FULL_KERNEL_VERSION}" != "common13-5.15" && "${ARCH}" = "arm64" ]]; then
			if [[ -z ${EXT_MODULES} ]]; then
				echo
				echo
				echo "========================================================"
				echo " Build GKI boot image and GKI modules"
				echo
				source ${KERNEL_DIR}/build.config.constants
				export OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}_gki})
				COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}})
				export DIST_GKI_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})

				if [[ "${GKI_CONFIG}" == "gki_20" ]]; then
					BUILD_CONFIG=${KERNEL_DIR}/build.config.gki.aarch64 ${ROOT_DIR}/${BUILD_DIR}/build.sh
				else
					export IN_BUILD_GKI_10=1
					${ROOT_DIR}/${BUILD_DIR}/build.sh
					unset IN_BUILD_GKI_10
				fi
				unset OUT_DIR
			fi

			echo
			echo
			echo "========================================================"
			echo " Build Vendor modules"
			echo
			${ROOT_DIR}/${BUILD_DIR}/build.sh "$@"
		else
			${ROOT_DIR}/${BUILD_DIR}/build.sh "$@"
		fi
	fi
fi

source ${ROOT_DIR}/${BUILD_CONFIG}

source ${KERNEL_BUILD_VAR_FILE}

if [[ -n ${RM_KERNEL_BUILD_VAR_FILE} ]]; then
	rm -f ${KERNEL_BUILD_VAR_FILE}
fi

rename_external_module_name

rebuild_rootfs ${ARCH}

set +e

check_undefined_symbol

abi_symbol_list_detect
