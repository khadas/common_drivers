#!/bin/bash

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo
	echo "  --arch                  for ARCH, build 64 or 32 bit kernel, arm|arm64[default], require parameter value"
	echo "  --abi                   for ABI, call build_abi.sh not build.sh, 1|0[default], not require parameter value"
	echo "  --build_config          for BUILD_CONFIG, common_drivers/build.config.amlogic[default]|common/build.config.gki.aarch64, require parameter value"
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
	echo "  --gki_10                for build gki 1.0 kernel:   gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug"
	echo "  --fast_build            for fast build"
	echo "  --upgrade               for android upgrade builtin module optimize vendor_boot size" following with android project name
	echo "  --manual_insmod_module  for insmod ko manually when kernel is booting.It's usually used in debug test"
	echo "  --patch                 for only am patches"
	echo "  --check_gki_20          for gki 2.0 check kernel build"
	echo "  --dev_config            for use the config specified by oem instead of amlogic like ./mk.sh --dev_config a_config+b_config+c_config"
	echo "  --use_prebuilt_gki      for use prebuilt gki, require parameter value, https://ci.android.com/builds/submitted/10412065/kernel_aarch64/latest, --use_prebuilt_gki 10412065"
	echo "  --kasan                 for build kernel with config kasan"
}

# handle the dir parameters for amlogic_utils.sh
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
	--patch)
		ONLY_PATCH=1
		PATCH_PARM=$2
		if [[ "${PATCH_PARM}" == "lunch" ]]; then
			VA=1
		fi
		CURRENT_DIR=`pwd`
		cd $(dirname $0)
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

set -- "${ARGS[@]}" # other parameters are used as script parameters to handle_input_parameters

source "${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh"

handle_input_parameters "$@"

set_default_parameters # set amlogic parameters default value

set -- "${ARGS[@]}" # other parameters are used as script parameters of build_abi.sh or build.sh

set -e

export_env_variable

copy_pre_commit

adjust_config_action

build_part_of_kernel

if [[ "${FULL_KERNEL_VERSION}" != "common13-5.15" && "${ARCH}" = "arm64" && ${BAZEL} == 1 ]]; then
	args=$@
	[[ -z ${PREBUILT_GKI} ]] && args="${args} --lto=${LTO}"
	[[ -z ${GKI_CONFIG} ]] && args="${args} --notrim --nokmi_symbol_list_strict_mode"

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
	echo "COMMON_DRIVERS_DIR=${COMMON_DRIVERS_DIR}" >> ${PROJECT_DIR}/build.config.gki10
	echo "UPGRADE_PROJECT=${UPGRADE_PROJECT}"	>> ${PROJECT_DIR}/build.config.gki10
	echo "DEV_CONFIGS=\"${DEV_CONFIGS}\""		>> ${PROJECT_DIR}/build.config.gki10
	echo "KASAN=${KASAN}"				>> ${PROJECT_DIR}/build.config.gki10
	echo "CHECK_GKI_20=${CHECK_GKI_20}"		>> ${PROJECT_DIR}/build.config.gki10

	if [[ -z ${ANDROID_PROJECT} ]]; then
		[[ -f ${PROJECT_DIR}/Kconfig.ext_modules ]] && rm -rf ${PROJECT_DIR}/Kconfig.ext_modules
		touch ${PROJECT_DIR}/Kconfig.ext_modules
		echo "# SPDX-License-Identifier: GPL-2.0" 	>  ${PROJECT_DIR}/Kconfig.ext_modules
		echo 						>> ${PROJECT_DIR}/Kconfig.ext_modules

		[[ -f ${PROJECT_DIR}/project.bzl ]] && rm -f ${PROJECT_DIR}/project.bzl
		touch ${PROJECT_DIR}/project.bzl
		echo "# SPDX-License-Identifier: GPL-2.0" 	>  ${PROJECT_DIR}/project.bzl
		echo 						>> ${PROJECT_DIR}/project.bzl

		echo 						>> ${PROJECT_DIR}/project.bzl
		echo "EXT_MODULES_ANDROID = [" 			>> ${PROJECT_DIR}/project.bzl
		echo "]" 					>> ${PROJECT_DIR}/project.bzl

		echo 						>> ${PROJECT_DIR}/project.bzl
		echo "MODULES_OUT_REMOVE = [" 		>> ${PROJECT_DIR}/project.bzl
		echo "]" 					>> ${PROJECT_DIR}/project.bzl

		echo 						>> ${PROJECT_DIR}/project.bzl
		echo "MODULES_OUT_ADD = [" 			>> ${PROJECT_DIR}/project.bzl
		echo "]" 					>> ${PROJECT_DIR}/project.bzl

		echo 						>> ${PROJECT_DIR}/project.bzl
		echo "KCONFIG_EXT_SRCS = [" 			>> ${PROJECT_DIR}/project.bzl
		echo "]" 					>> ${PROJECT_DIR}/project.bzl
	fi

	echo 						>> ${PROJECT_DIR}/project.bzl
	sed -i "/ANDROID_PROJECT/d" ${PROJECT_DIR}/project.bzl
	echo "ANDROID_PROJECT = \"${ANDROID_PROJECT}\"" >> ${PROJECT_DIR}/project.bzl

	sed -i "/GKI_CONFIG/d" ${PROJECT_DIR}/project.bzl
	echo "GKI_CONFIG = \"${GKI_CONFIG}\""		>> ${PROJECT_DIR}/project.bzl

	sed -i "/UPGRADE_PROJECT/d" ${PROJECT_DIR}/project.bzl
	echo "UPGRADE_PROJECT = \"${UPGRADE_PROJECT}\"" >> ${PROJECT_DIR}/project.bzl

	[[ -f ${PROJECT_DIR}/dtb.bzl ]] || touch ${PROJECT_DIR}/dtb.bzl
	echo "# SPDX-License-Identifier: GPL-2.0" 	>  ${PROJECT_DIR}/dtb.bzl
	echo 						>> ${PROJECT_DIR}/dtb.bzl

	echo "AMLOGIC_DTBS = ["				>> ${PROJECT_DIR}/dtb.bzl
	cat  ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/boot/dts/amlogic/Makefile | grep -n "dtb" | cut -d "=" -f 2 | sed 's/[[:space:]][[:space:]]*/ /g' | sed 's/^[ ]*//' | sed 's/[ ]*$//' | sed '/^#/d;/^$/d' | sed 's/^/    "/' | sed 's/$/",/' >> ${PROJECT_DIR}/dtb.bzl
	echo "]"					>> ${PROJECT_DIR}/dtb.bzl

	if [[ "${GKI_CONFIG}" != "gki_20" || -n ${KASAN} || -z ${ANDROID_PROJECT} ]]; then
		args="${args} --gki_build_config_fragment=//common:common_drivers/build.config.amlogic.fragment.bazel"
	fi

	if [[ ${GKI_CONFIG} != gki_20 || -n ${KASAN} || -n ${CHECK_GKI_20} ]]; then
		args="${args} --allow_undeclared_modules"
	fi

	[[ -n ${KASAN} ]] && args="${args} --kasan"

	if [[ -n ${CHECK_GKI_20} ]]; then
		args="${args} --lto=none --notrim --nokmi_symbol_list_strict_mode"
	fi

	echo args=${args}
	set -x
	if [[ -n ${GOOGLE_BAZEL_BUILD_COMMAND_LINE} ]]; then
		if [[${GOOGLE_BAZEL_BUILD_COMMAND_LINE} =~ "--kasan" ]]; then
			GOOGLE_BAZEL_BUILD_COMMAND_LINE="${GOOGLE_BAZEL_BUILD_COMMAND_LINE} \
								--gki_build_config_fragment=//common:common_drivers/build.config.amlogic.fragment.bazel \
								--allow_undeclared_modules"
		fi
		${GOOGLE_BAZEL_BUILD_COMMAND_LINE}
	elif [[ "${ABI}" -eq "1" ]]; then
		tools/bazel run //common:amlogic_abi_update_symbol_list --sandbox_debug --verbose_failures ${args}
		tools/bazel run //common:kernel_aarch64_abi_dist --sandbox_debug --verbose_failures ${args}
		exit
	elif [[ -n ${PREBUILT_GKI} ]]; then
		tools/bazel run --use_prebuilt_gki=${PREBUILT_GKI} //common:amlogic_dist --sandbox_debug --verbose_failures ${args}
	else
		tools/bazel run //common:amlogic_dist --sandbox_debug --verbose_failures ${args}
	fi
	set +x

	echo "========================================================"
	echo "after compiling with bazel and organizing the document"
	source ${KERNEL_DIR}/build.config.constants
	export COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}})
	export DIST_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})
	source ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh

	bazel_extra_cmds
	if [[ -n ${COPY_DEV_CONFIGS} ]]; then
		for config_name in ${COPY_DEV_CONFIGS}; do
			if [[ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${config_name} ]]; then
				rm -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${config_name}
			else
				echo "ERROR: config file ${config_name} is not in the right path!!"
				exit
			fi
		done
	fi
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
			if [[ "${FULL_KERNEL_VERSION}" != "common13-5.15" && "${ARCH}" = "arm" ]]; then
				build_android_32bit $@
			else
				clear_files_compressed_with_lzma_in_last_build
				${ROOT_DIR}/${BUILD_DIR}/build.sh "$@"
			fi
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

if [[ ${ARCH} = "arm64" ]]; then
	generate_lzma_format_image
fi
