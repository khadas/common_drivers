#!/bin/bash

function pre_defconfig_cmds() {
	if [[ -n ${AMLOGIC_NONGKI} ]]; then
		echo "CONFIG_AMLOGIC_NONGKI=y" >> ${ROOT_DIR}/${FRAGMENT_CONFIG}
		SKIP_EXT_MODULES=1
		export SKIP_EXT_MODULES
		EXT_MODULES=
		export EXT_MODULES
	fi
	KCONFIG_CONFIG=${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/gki_defconfig ${ROOT_DIR}/${FRAGMENT_CONFIG}
}
export -f pre_defconfig_cmds

function post_defconfig_cmds() {
	check_defconfig
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG}
	pushd ${ROOT_DIR}/common_drivers
		git checkout ${ROOT_DIR}/${FRAGMENT_CONFIG}
	popd
}
export -f post_defconfig_cmds

function read_ext_module_config() {
	ALL_LINE=""
	while read LINE
	do
		if [[ $LINE != \#*  &&  $LINE != "" ]]; then
			ALL_LINE="$ALL_LINE"" ""$LINE"
		fi
	done < $1
	export GKI_EXT_MODULE_CONFIG=$ALL_LINE
	echo "GKI_EXT_MODULE_CONFIG=${GKI_EXT_MODULE_CONFIG}"
}

function read_ext_module_predefine() {
	PRE_DEFINE=""

	for y_config in `cat $1 | grep "^CONFIG_.*=y" | sed 's/=y//'`;
	do
		PRE_DEFINE="$PRE_DEFINE"" -D"${y_config}
	done

	for m_config in `cat $1 | grep "^CONFIG_.*=m" | sed 's/=m//'`;
	do
		PRE_DEFINE="$PRE_DEFINE"" -D"${m_config}_MODULE
	done

	export GKI_EXT_MODULE_PREDEFINE=$PRE_DEFINE
	echo "GKI_EXT_MODULE_PREDEFINE=${GKI_EXT_MODULE_PREDEFINE}"
}

function prepare_module_build() {
	if [[ -z ${AMLOGIC_NONGKI} ]]; then
		read_ext_module_config $FRAGMENT_CONFIG && read_ext_module_predefine $FRAGMENT_CONFIG
	fi
}

export -f prepare_module_build
