#!/bin/bash

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo
	echo "  --abi                   for ABI, call build_abi.sh not build.sh, 1|0[default], not require parameter value"
	echo "  --build_config          for BUILD_CONFIG, common_drivers/build.config.amlogic[default]|common/build.config.gki.aarch64, require parameter value"
	echo "  --break_gki             for AMLOGIC_BREAK_GKI, 1[default]|0, require parameter value"
	echo "  --symbol_strict         for KMI_SYMBOL_LIST_STRICT_MODE, 1[default]|0, require parameter value"
	echo "  --lto                   for LTO, full|thin[default]|none, require parameter value"
	echo "  --menuconfig            for only menuconfig, not require parameter value"
	echo "  --image                 for only build kernel, not require parameter value"
	echo "  --modules               for only build modules, not require parameter value"
	echo "  --dtbs                  for only build dtbs, not require parameter value"
	echo "  --kernel_dir            for KERNEL_DIR, common[default]|other dir, require parameter value"
	echo "  --common_drivers_dir    for COMMON_DRIVERS_DIR, common[default]|other dir, require parameter value"
	echo "  --build_dir             for BUILD_DIR, build[default]|other dir, require parameter value"
}

VA=
ARGS=()
for i in "$@"
do
	case $i in
	--abi)
		ABI=1
		shift
		;;
	--build_config)
		BUILD_CONFIG=$2
		VA=1
		shift
		;;
	--break_gki)
		AMLOGIC_BREAK_GKI=$2
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

export ROOT_DIR=$(readlink -f $(dirname $0))
				# amlogic parameters default value
if [[ -z "${ABI}" ]]; then
	ABI=0
fi
if [[ -z "${AMLOGIC_BREAK_GKI}" ]]; then
	AMLOGIC_BREAK_GKI=1
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
	BUILD_CONFIG=${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/build.config.amlogic
fi
if [[ -z "${BUILD_DIR}" ]]; then
	BUILD_DIR=build
fi
if [[ ! -f ${BUILD_DIR}/build_abi.sh ]]; then
	echo "The directory of build does not exist";
fi

set -e
export ABI BUILD_CONFIG AMLOGIC_BREAK_GKI LTO KMI_SYMBOL_LIST_STRICT_MODE
echo ABI=${ABI} BUILD_CONFIG=${BUILD_CONFIG} AMLOGIC_BREAK_GKI=${AMLOGIC_BREAK_GKI} LTO=${LTO} KMI_SYMBOL_LIST_STRICT_MODE=${KMI_SYMBOL_LIST_STRICT_MODE}
export KERNEL_DIR COMMON_DRIVERS_DIR BUILD_DIR
echo KERNEL_DIR=${KERNEL_DIR} COMMON_DRIVERS_DIR=${COMMON_DRIVERS_DIR} BUILD_DIR=${BUILD_DIR}

source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/build.config.amlogic

if [ "${ABI}" -eq "1" ]; then
	export OUT_DIR_SUFFIX="_abi"
else
	OUT_DIR_SUFFIX=
fi

echo MENUCONFIG=${MENUCONFIG} IMAGE=${IMAGE} MODULES=${MODULES} DTB_BUILD=${DTB_BUILD}
if [[ -n ${MENUCONFIG} ]] || [[ -n ${IMAGE} ]] || [[ -n ${MODULES} ]] || [[ -n ${DTB_BUILD} ]]; then

	source "${ROOT_DIR}/${BUILD_DIR}/_setup_env.sh"
	source "${ROOT_DIR}/${BUILD_DIR}/build_utils.sh"

	if [[ -n ${MENUCONFIG} ]]; then
		set -x
		pre_defconfig_cmds
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${DEFCONFIG})
		(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" menuconfig)
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

echo "========================================================"
echo ""
export COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}})
export OUT_DIR=$(readlink -m ${COMMON_OUT_DIR}/${KERNEL_DIR})
export DIST_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})
export MODULES_STAGING_DIR=$(readlink -m ${COMMON_OUT_DIR}/staging)
echo COMMON_OUT_DIR=$COMMON_OUT_DIR OUT_DIR=$OUT_DIR DIST_DIR=$DIST_DIR MODULES_STAGING_DIR=$MODULES_STAGING_DIR KERNEL_DIR=$KERNEL_DIR

echo "========================================================"
echo "prepare modules"
modules_install
if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
	echo "Rebuild rootfs in order to install modules!"
	rebuild_rootfs
else
	echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
fi
set +e
check_undefined_symbol
