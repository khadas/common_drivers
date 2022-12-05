#!/bin/bash

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo
	echo "  --arch                  for ARCH, build 64 or 32 bit kernel, arm|arm64[default], require parameter value"
	echo "  --abi                   for ABI, call build_abi.sh not build.sh, 1|0[default], not require parameter value"
	echo "  --build_config          for BUILD_CONFIG, common_drivers/build.config.amlogic[default]|common/build.config.gki.aarch64, require parameter value"
	echo "  --symbol_strict         for KMI_SYMBOL_LIST_STRICT_MODE, 1[default]|0, require parameter value"
	echo "  --lto                   for LTO, full|thin[default]|none, require parameter value"
	echo "  --savedefconfig         for only savedefconfig, not require parameter value"
	echo "  --image                 for only build kernel, not require parameter value"
	echo "  --modules               for only build modules, not require parameter value"
	echo "  --dtbs                  for only build dtbs, not require parameter value"
	echo "  --kernel_dir            for KERNEL_DIR, common[default]|other dir, require parameter value"
	echo "  --common_drivers_dir    for COMMON_DRIVERS_DIR, common[default]|other dir, require parameter value"
	echo "  --build_dir             for BUILD_DIR, build[default]|other dir, require parameter value"
	echo "  --check_defconfig       for check defconfig"
	echo "  --modules_depend        for check modules depend"
	echo "  --android_project       for android project build"
	echo "  --gki_20"		for build gki 2.0 kernel:	gki_defconfig + amlogic_gki.fragment
	echo "  --gki_10"		for build gki 1.0 kernel:	gki_defconfig + amlogic_gki.fragment + amlogic_gki.10
	echo "  --gki_debug"        	for build gki debug kernel:	gki_defconfig + amlogic_gki.fragment + amlogic_gki.10 + amlogic_gki.debug
	echo "  --upgrade		for android upgrade builtin module optimize vendor_boot size"
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
	--savedefconfig)
		SAVEDEFCONFIG=1
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
	--upgrade)
		UPGRADE_PROJECT=1
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
if [[ -z "${ABI}" ]]; then
	ABI=0
fi
if [[ -z "${LTO}" ]]; then
	LTO=thin
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
export ABI BUILD_CONFIG LTO KMI_SYMBOL_LIST_STRICT_MODE CHECK_DEFCONFIG
echo ABI=${ABI} BUILD_CONFIG=${BUILD_CONFIG} LTO=${LTO} KMI_SYMBOL_LIST_STRICT_MODE=${KMI_SYMBOL_LIST_STRICT_MODE} CHECK_DEFCONFIG=${CHECK_DEFCONFIG}
export KERNEL_DIR COMMON_DRIVERS_DIR BUILD_DIR ANDROID_PROJECT GKI_CONFIG UPGRADE_PROJECT
echo KERNEL_DIR=${KERNEL_DIR} COMMON_DRIVERS_DIR=${COMMON_DRIVERS_DIR} BUILD_DIR=${BUILD_DIR} GKI_CONFIG=${GKI_CONFIG} UPGRADE_PROJECT=${UPGRADE_PROJECT}

export CROSS_COMPILE=

source ${ROOT_DIR}/${BUILD_CONFIG}

if [ "${ABI}" -eq "1" ]; then
	export OUT_DIR_SUFFIX="_abi"
else
	OUT_DIR_SUFFIX=
fi

echo SAVEDEFCONFIG=${SAVEDEFCONFIG} IMAGE=${IMAGE} MODULES=${MODULES} DTB_BUILD=${DTB_BUILD}
if [[ -n ${SAVEDEFCONFIG} ]] || [[ -n ${IMAGE} ]] || [[ -n ${MODULES} ]] || [[ -n ${DTB_BUILD} ]]; then

	source "${ROOT_DIR}/${BUILD_DIR}/_setup_env.sh"
	source "${ROOT_DIR}/${BUILD_DIR}/build_utils.sh"

	if [[ -n ${SAVEDEFCONFIG} ]]; then
		set -x
		pre_defconfig_cmds
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${DEFCONFIG})
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
		post_defconfig_cmds
		set +x
	fi
	if [[ -n ${IMAGE} ]]; then
		set -x
		(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}" -j$(nproc) Image)
		set +x
	fi
	if [[ -n ${MODULES} ]]; then
		if [[ ${IN_KERNEL_MODULES} -eq "1" ]];
		then
			set -x
			(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}" -j$(nproc) modules)
			set +x
		else
			echo EXT_MODULES=$EXT_MODULES
			set -x
			prepare_module_build
			if [[ -z "${SKIP_EXT_MODULES}" ]] && [[ -n "${EXT_MODULES}" ]]; then
				echo "========================================================"
				echo " Building external modules and installing them into staging directory"

				MODULES_STAGING_DIR=$(readlink -m ${COMMON_OUT_DIR}/staging)
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
						INSTALL_HDR_PATH="${KERNEL_UAPI_HEADERS_DIR}/usr"      \
						"${MAKE_ARGS[@]}" modules_install
					set +x
				done
			fi
			set +x
		fi
	fi
	if [[ -n ${DTB_BUILD} ]]; then
		set -x
		(cd ${OUT_DIR} && make O=${OUT_DIR} ${TOOL_ARGS} "${MAKE_ARGS[@]}" -j$(nproc) dtbs)
		set +x
	fi
	exit
fi

if [ "${ABI}" -eq "1" ]; then
	${ROOT_DIR}/${BUILD_DIR}/build_abi.sh "$@"
else
	${ROOT_DIR}/${BUILD_DIR}/build.sh "$@"
fi

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
