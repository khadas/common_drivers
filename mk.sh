#!/bin/bash

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo
	echo "  --nongki                for AMLOGIC_NONGKI, build in modules, not build out modules, 1[default]|0, require parameter value"
	echo "  --abi                   for ABI, call build_abi.sh not build.sh, 1|0[default], not require parameter value"
	echo "  --build_config          for BUILD_CONFIG, common_drivers/build.config.amlogic[default]|common/build.config.gki.aarch64, require parameter value"
	echo "  --userdebug             for AMLOGIC_USERDEBUG, 1[default]|0, require parameter value"
	echo "  --symbol_strict         for KMI_SYMBOL_LIST_STRICT_MODE, 1[default]|0, require parameter value"
	echo "  --lto                   for LTO, full|thin[default]|none, require parameter value"
	echo "  --menuconfig            for only menuconfig, not require parameter value"
	echo "  --image                 for only build kernel, not require parameter value"
	echo "  --modules               for only build modules, not require parameter value"
	echo "  --dtbs                  for only build dtbs, not require parameter value"
}
				# amlogic parameters default value
if [[ -z "${AMLOGIC_NONGKI}" ]]; then
	AMLOGIC_NONGKI=1
fi
if [[ -z "${ABI}" ]]; then
	ABI=0
fi
if [[ -z "${BUILD_CONFIG}" ]]; then
	BUILD_CONFIG=common_drivers/build.config.amlogic
fi
if [[ -z "${AMLOGIC_USERDEBUG}" ]]; then
	AMLOGIC_USERDEBUG=1
fi
if [[ -z "${LTO}" ]]; then
	LTO=thin
fi

VA=
ARGS=()
for i in "$@"
do
	case $i in
	--nongki)
		AMLOGIC_NONGKI=$2
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
	--userdebug)
		AMLOGIC_USERDEBUG=$2
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
set -e
export AMLOGIC_NONGKI ABI BUILD_CONFIG AMLOGIC_USERDEBUG LTO KMI_SYMBOL_LIST_STRICT_MODE
echo AMLOGIC_NONGKI=${AMLOGIC_NONGKI} ABI=${ABI} BUILD_CONFIG=${BUILD_CONFIG} AMLOGIC_USERDEBUG=${AMLOGIC_USERDEBUG} LTO=${LTO} KMI_SYMBOL_LIST_STRICT_MODE=${KMI_SYMBOL_LIST_STRICT_MODE}

export KERNEL_DIR=common
export ROOT_DIR=$(readlink -f $(dirname $0))
source ${ROOT_DIR}/common_drivers/build.config.amlogic

if [ "${ABI}" -eq "1" ]; then
	export OUT_DIR_SUFFIX="_abi"
else
	OUT_DIR_SUFFIX=
fi

echo MENUCONFIG=${MENUCONFIG} IMAGE=${IMAGE} MODULES=${MODULES} DTB_BUILD=${DTB_BUILD}
if [[ -n ${MENUCONFIG} ]] || [[ -n ${IMAGE} ]] || [[ -n ${MODULES} ]] || [[ -n ${DTB_BUILD} ]]; then

	source "${ROOT_DIR}/build/_setup_env.sh"
	source "${ROOT_DIR}/build/build_utils.sh"

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
		if [[ ${AMLOGIC_NONGKI} -eq "1" ]];
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
	${ROOT_DIR}/build/build_abi.sh "$@"
else
	${ROOT_DIR}/build/build.sh "$@"
fi

echo "========================================================"
echo ""
export COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}})
export OUT_DIR=$(readlink -m ${COMMON_OUT_DIR}/${KERNEL_DIR})
export DIST_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})
export MODULES_STAGING_DIR=$(readlink -m ${COMMON_OUT_DIR}/staging)
echo COMMON_OUT_DIR=$COMMON_OUT_DIR OUT_DIR=$OUT_DIR DIST_DIR=$DIST_DIR MODULES_STAGING_DIR=$MODULES_STAGING_DIR KERNEL_DIR=$KERNEL_DIR
# source ${ROOT_DIR}/common_drivers/amlogic_utils.sh

echo "========================================================"
echo "prepare modules"
modules_install
if [ -f ${ROOT_DIR}/common_drivers/rootfs_base.cpio.gz.uboot ]; then
	echo "Rebuild rootfs in order to install modules!"
	rebuild_rootfs
else
	echo "There's no file ${ROOT_DIR}/common_drivers/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
fi
set +e
check_undefined_symbol
