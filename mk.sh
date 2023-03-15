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
	echo "  --manual_insmod_module  for insmod ko manually when kernel is booting.It's usually used in debug test"
	echo "  --patch                 for only am patches"
	echo "  -v                      for kernel version: common13-5.15 common14-5.15"
	echo "  --check_gki_20          for gki 2.0 check kernel build"
	echo "  --dev_config            for use the config specified by oem instead of amlogic like ./mk.sh --dev_config a_config+b_config+c_config"
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
	-v)
		if [[ "$2" == "common13-5.15" ]] ||
		   [[ "$2" == "common14-5.15" ]]; then
			FULL_KERNEL_VERSION=$2
		else
			echo "The kernel version is not available"
			exit
		fi
		VA=1
		shift
		;;
	--check_gki_20)
		CHECK_GKI_20=1
		GKI_CONFIG=gki_20
		LTO=none
		shift
		;;
	--dev_config)
		DEV_CONFIG=1
		CONFIG_GROUP=$2
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

if [ "${ARCH}" = "arm" ]; then
	ARGS+=("LOADADDR=0x108000")
else
	ARCH=arm64
fi

set -- "${ARGS[@]}"		# other parameters are used as script parameters of build_abi.sh or build.sh
				# amlogic parameters default value
if [[ ${ONLY_PATCH} -eq "1" ]]; then
	if [[ -z ${FULL_KERNEL_VERSION} ]]; then
		if [[ "$(basename $(dirname $0))" == "common-5.15" ]]; then
			FULL_KERNEL_VERSION="common13-5.15"
		elif [[ "$(basename $(dirname $0))" == "common14-5.15" ]]; then
			FULL_KERNEL_VERSION="common14-5.15"
		fi
	fi
	CURRENT_DIR=`pwd`
	cd $(dirname $0)
fi
if [[ -z ${FULL_KERNEL_VERSION} ]]; then
	FULL_KERNEL_VERSION="common13-5.15"
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

set -e
export ABI BUILD_CONFIG LTO KMI_SYMBOL_LIST_STRICT_MODE CHECK_DEFCONFIG MANUAL_INSMOD_MODULE ARCH
export KERNEL_DIR COMMON_DRIVERS_DIR BUILD_DIR ANDROID_PROJECT GKI_CONFIG UPGRADE_PROJECT FAST_BUILD CHECK_GKI_20 DEV_CONFIG CONFIG_GROUP

if [[ -f ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/auto_patch/auto_patch.sh ]]; then
	${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/auto_patch/auto_patch.sh ${FULL_KERNEL_VERSION}
fi
if [[ ${ONLY_PATCH} -eq "1" ]]; then
	cd ${CURRENT_DIR}
	exit
fi

echo ROOT_DIR=$ROOT_DIR
echo ABI=${ABI} BUILD_CONFIG=${BUILD_CONFIG} LTO=${LTO} KMI_SYMBOL_LIST_STRICT_MODE=${KMI_SYMBOL_LIST_STRICT_MODE} CHECK_DEFCONFIG=${CHECK_DEFCONFIG} MANUAL_INSMOD_MODULE=${MANUAL_INSMOD_MODULE}
echo KERNEL_DIR=${KERNEL_DIR} COMMON_DRIVERS_DIR=${COMMON_DRIVERS_DIR} BUILD_DIR=${BUILD_DIR} ANDROID_PROJECT=${ANDROID_PROJECT} GKI_CONFIG=${GKI_CONFIG} UPGRADE_PROJECT=${UPGRADE_PROJECT} FAST_BUILD=${FAST_BUILD} CHECK_GKI_20=${CHECK_GKI_20}

export CROSS_COMPILE=

if [ "${ABI}" -eq "1" ]; then
	export OUT_DIR_SUFFIX="_abi"
else
	OUT_DIR_SUFFIX=
fi

echo MENUCONFIG=${MENUCONFIG} BASICCONFIG=${BASICCONFIG} IMAGE=${IMAGE} MODULES=${MODULES} DTB_BUILD=${DTB_BUILD}
if [[ -n ${MENUCONFIG} ]] || [[ -n ${BASICCONFIG} ]] || [[ ${CHECK_DEFCONFIG} -eq "1" ]]; then
	# ${ROOT_DIR}/${BUILD_DIR}/config.sh menuconfig
	HERMETIC_TOOLCHAIN=0
	source "${ROOT_DIR}/${BUILD_DIR}/build_utils.sh"
	source "${ROOT_DIR}/${BUILD_DIR}/_setup_env.sh"

	orig_config=$(mktemp)
	orig_defconfig=$(mktemp)
	out_config="${OUT_DIR}/.config"
	out_defconfig="${OUT_DIR}/defconfig"
	changed_config=$(mktemp)
	changed_defconfig=$(mktemp)

	if [[ -n ${BASICCONFIG} ]]; then
		set -x
		defconfig_name=`basename ${GKI_BASE_CONFIG}`
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${defconfig_name})
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
		cp ${out_config} ${orig_config}
		cp ${out_defconfig} ${orig_defconfig}
		if [ "${BASICCONFIG}" = "m" ]; then
			(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" menuconfig)
		fi
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
		${KERNEL_DIR}/scripts/diffconfig ${orig_config} ${out_config} > ${changed_config}
		${KERNEL_DIR}/scripts/diffconfig ${orig_defconfig} ${out_defconfig} > ${changed_defconfig}
		if [ "${ARCH}" = "arm" ]; then
			cp ${out_defconfig} ${GKI_BASE_CONFIG}
		fi
		set +x
		echo
		echo "========================================================"
		echo "==================== .config diff   ===================="
		cat ${changed_config}
		echo "==================== defconfig diff ===================="
		cat ${changed_defconfig}
		echo "========================================================"
		echo
	elif [[ ${CHECK_DEFCONFIG} -eq "1" ]]; then
		set -x
		pre_defconfig_cmds
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${DEFCONFIG})
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
		diff -u ${ROOT_DIR}/${GKI_BASE_CONFIG} ${OUT_DIR}/defconfig
		post_defconfig_cmds
		set +x
	else
		set -x
		pre_defconfig_cmds
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${DEFCONFIG})
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
		cp ${out_config} ${orig_config}
		cp ${out_defconfig} ${orig_defconfig}
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" menuconfig)
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
		${KERNEL_DIR}/scripts/diffconfig ${orig_config} ${out_config} > ${changed_config}
		${KERNEL_DIR}/scripts/diffconfig ${orig_defconfig} ${out_defconfig} > ${changed_defconfig}
		post_defconfig_cmds
		set +x
		echo
		echo "========================================================"
		echo "if the config follows GKI2.0, please add it to the file amlogic_gki.fragment manually"
		echo "if the config follows GKI1.0 optimize, please add it to the file amlogic_gki.10 manually"
		echo "if the config follows GKI1.0 debug, please add it to the file amlogic_gki.debug manually"
		echo "==================== .config diff   ===================="
		cat ${changed_config}
		echo "==================== defconfig diff ===================="
		cat ${changed_defconfig}
		echo "========================================================"
		echo
	fi
	rm -f ${orig_config} ${changed_config} ${orig_defconfig} ${changed_defconfig}
	exit
fi

if [[ -n ${IMAGE} ]] || [[ -n ${MODULES} ]] || [[ -n ${DTB_BUILD} ]]; then
	old_path=${PATH}
	source "${ROOT_DIR}/${BUILD_DIR}/build_utils.sh"
	source "${ROOT_DIR}/${BUILD_DIR}/_setup_env.sh"

	if [[ ! -f ${OUT_DIR}/.config ]]; then
		pre_defconfig_cmds
		set -x
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${DEFCONFIG})
		set +x
		post_defconfig_cmds
	fi

	if [[ -n ${IMAGE} ]]; then
		set -x
		if [ "${ARCH}" = "arm64" ]; then
			(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}" -j$(nproc) Image)
		elif [ "${ARCH}" = "arm" ]; then
			(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}" -j$(nproc) LOADADDR=0x108000 uImage)
		fi
		set +x
	fi
	mkdir -p ${DIST_DIR}
	if [[ -n ${DTB_BUILD} ]]; then
		set -x
		(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}" -j$(nproc) dtbs)
		set +x
	fi
	if [[ -n ${MODULES} ]]; then
		export MODULES_STAGING_DIR=$(readlink -m ${COMMON_OUT_DIR}/staging)
		rm -rf ${MODULES_STAGING_DIR}
		mkdir -p ${MODULES_STAGING_DIR}
		if [ "${DO_NOT_STRIP_MODULES}" != "1" ]; then
			MODULE_STRIP_FLAG="INSTALL_MOD_STRIP=1"
		fi
		if [[ `grep "CONFIG_AMLOGIC_IN_KERNEL_MODULES=y" ${ROOT_DIR}/${FRAGMENT_CONFIG}` ]]; then
			set -x
			(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}" -j$(nproc) modules)
			(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} ${MODULE_STRIP_FLAG} INSTALL_MOD_PATH=${MODULES_STAGING_DIR} "${MAKE_ARGS[@]}" modules_install)
			set +x
		fi
		echo EXT_MODULES=$EXT_MODULES
		prepare_module_build
		if [[ -z "${SKIP_EXT_MODULES}" ]] && [[ -n "${EXT_MODULES}" ]]; then
			echo "========================================================"
			echo " Building external modules and installing them into staging directory"
			KERNEL_UAPI_HEADERS_DIR=$(readlink -m ${COMMON_OUT_DIR}/kernel_uapi_headers)
			for EXT_MOD in ${EXT_MODULES}; do
				EXT_MOD_REL=$(rel_path ${ROOT_DIR}/${EXT_MOD} ${KERNEL_DIR})
				mkdir -p ${OUT_DIR}/${EXT_MOD_REL}
				set -x
				make -C ${EXT_MOD} M=${EXT_MOD_REL} KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}  \
					O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}"
				make -C ${EXT_MOD} M=${EXT_MOD_REL} KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}  \
					O=${OUT_DIR} ${TOOL_ARGS} ${MODULE_STRIP_FLAG}         \
					INSTALL_MOD_PATH=${MODULES_STAGING_DIR}                \
					INSTALL_MOD_DIR="extra/${EXT_MOD}"                     \
					INSTALL_HDR_PATH="${KERNEL_UAPI_HEADERS_DIR}/usr"      \
					"${MAKE_ARGS[@]}" modules_install
				set +x
			done
		fi
		export OUT_AMLOGIC_DIR=$(readlink -m ${COMMON_OUT_DIR}/amlogic)
		set -x
		extra_cmds
		set +x
		MODULES=$(find ${MODULES_STAGING_DIR} -type f -name "*.ko")
		cp -p ${MODULES} ${DIST_DIR}

		new_path=${PATH}
		PATH=${old_path}
		echo "========================================================"
		if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
			echo "Rebuild rootfs in order to install modules!"
			rebuild_rootfs ${ARCH}
		else
			echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
		fi
		PATH=${new_path}
	fi
	if [ -n "${DTS_EXT_DIR}" ]; then
		if [ -d "${ROOT_DIR}/${DTS_EXT_DIR}" ]; then
			DTS_EXT_DIR=$(rel_path ${ROOT_DIR}/${DTS_EXT_DIR} ${KERNEL_DIR})
			if [ -d ${OUT_DIR}/${DTS_EXT_DIR} ]; then
				FILES="$FILES `ls ${OUT_DIR}/${DTS_EXT_DIR}`"
			fi
		fi
	fi
	for FILE in ${FILES}; do
		if [ -f ${OUT_DIR}/${FILE} ]; then
			echo "  $FILE"
			cp -p ${OUT_DIR}/${FILE} ${DIST_DIR}/
		elif [[ "${FILE}" =~ \.dtb|\.dtbo ]]  && \
			[ -n "${DTS_EXT_DIR}" ] && [ -f "${OUT_DIR}/${DTS_EXT_DIR}/${FILE}" ] ; then
			# DTS_EXT_DIR is recalculated before to be relative to KERNEL_DIR
			echo "  $FILE"
			cp -p "${OUT_DIR}/${DTS_EXT_DIR}/${FILE}" "${DIST_DIR}/"
		else
			echo "  $FILE is not a file, skipping"
		fi
	done
	exit
fi

if [ "${ABI}" -eq "1" ]; then
	${ROOT_DIR}/${BUILD_DIR}/build_abi.sh "$@"
else
	${ROOT_DIR}/${BUILD_DIR}/build.sh "$@"
fi

source ${ROOT_DIR}/${BUILD_CONFIG}

source ${KERNEL_BUILD_VAR_FILE}
if [[ -n ${RM_KERNEL_BUILD_VAR_FILE} ]]; then
	rm -f ${KERNEL_BUILD_VAR_FILE}
fi

echo "========================================================"
if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
	echo "Rebuild rootfs in order to install modules!"
	rebuild_rootfs ${ARCH}
else
	echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
fi
set +e

if [[ ${MODULES_DEPEND} -eq "1" ]]; then
	echo "========================================================"
	echo "print modules depend"
	check_undefined_symbol
fi
