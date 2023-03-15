#!/bin/bash

function pre_defconfig_cmds() {
	export OUT_AMLOGIC_DIR=$(readlink -m ${COMMON_OUT_DIR}/amlogic)
	if [ "${ARCH}" = "arm" ]; then
		export PATH=${PATH}:/usr/bin/
	fi

	if [[ -z ${ANDROID_PROJECT} ]]; then
		local temp_file=`mktemp /tmp/config.XXXXXXXXXXXX`
		echo "CONFIG_AMLOGIC_SERIAL_MESON=y" > ${temp_file}
		echo "CONFIG_DEVTMPFS=y" >> ${temp_file}
		if [[ ${GKI_CONFIG} == gki_20 ]]; then
			KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${GKI_BASE_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG} ${temp_file}
		elif [[ ${GKI_CONFIG} == gki_10 ]]; then
			KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${GKI_BASE_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG_GKI10} ${temp_file}
		elif [[ ${GKI_CONFIG} == gki_debug ]]; then
			KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${GKI_BASE_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG_GKI10} ${ROOT_DIR}/${FRAGMENT_CONFIG_DEBUG} ${temp_file}
		fi
		rm ${temp_file}
	else
		if [[ ${GKI_CONFIG} == gki_20 ]]; then
			if [[  -n ${CHECK_GKI_20} ]]; then
				local temp_file=`mktemp /tmp/config.XXXXXXXXXXXX`
				echo "CONFIG_STMMAC_ETH=n" > ${temp_file}
				KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${GKI_BASE_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG} ${temp_file}
				rm ${temp_file}
			else
				KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${GKI_BASE_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG}
			fi
		elif [[ ${GKI_CONFIG} == gki_10 ]]; then
			KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${GKI_BASE_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG_GKI10}
		elif [[ ${GKI_CONFIG} == gki_debug ]]; then
			KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${GKI_BASE_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG_GKI10} ${ROOT_DIR}/${FRAGMENT_CONFIG_DEBUG}
		fi
	fi

	if [[ -n ${UPGRADE_PROJECT} ]]; then
		KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG_UPGRADE}
	fi

	if [[ -n ${DEV_CONFIG} ]]; then
		config_list=$(echo ${CONFIG_GROUP}|sed 's/+/ /g')
		#verify the extra config is in the right path and merge the config
		CONFIG_DIR=arch/${ARCH}/configs
		for config_name in ${config_list[@]}
		do
			if [[ ! -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/${CONFIG_DIR}/${config_name} ]]; then
				echo "ERROR: config file ${config_name} is not in the right path!!"
				exit
			else
				KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/${CONFIG_DIR}/${config_name}
			fi
		done
	fi
}
export -f pre_defconfig_cmds

function post_defconfig_cmds() {
	rm ${ROOT_DIR}/${KCONFIG_DEFCONFIG}
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

	echo "${ALL_LINE}"
}

function read_ext_module_predefine() {
	PRE_DEFINE=""

	for y_config in `cat $1 | grep "^CONFIG_.*=y" | sed 's/=y//'`; do
		PRE_DEFINE="$PRE_DEFINE"" -D"${y_config}
	done

	for m_config in `cat $1 | grep "^CONFIG_.*=m" | sed 's/=m//'`; do
		PRE_DEFINE="$PRE_DEFINE"" -D"${m_config}_MODULE
	done

	echo "${PRE_DEFINE}"
}

top_ext_drivers=top_ext_drivers
function prepare_module_build() {
	local temp_file=`mktemp /tmp/kernel.XXXXXXXXXXXX`
	if [[ ! `grep "CONFIG_AMLOGIC_IN_KERNEL_MODULES=y" ${ROOT_DIR}/${FRAGMENT_CONFIG}` ]]; then
		sed 's:#.*$::g' ${ROOT_DIR}/${FRAGMENT_CONFIG} | sed '/^$/d' | sed 's/^[ ]*//' | sed 's/[ ]*$//' > ${temp_file}
		GKI_EXT_KERNEL_MODULE_CONFIG=$(read_ext_module_config ${temp_file})
		GKI_EXT_KERNEL_MODULE_PREDEFINE=$(read_ext_module_predefine ${temp_file})
		export GKI_EXT_KERNEL_MODULE_CONFIG GKI_EXT_KERNEL_MODULE_PREDEFINE
	fi

	for ext_module_config in ${EXT_MODULES_CONFIG}; do
		sed 's:#.*$::g' ${ROOT_DIR}/${ext_module_config} | sed '/^$/d' | sed 's/^[ ]*//' | sed 's/[ ]*$//' > ${temp_file}
		GKI_EXT_MODULE_CONFIG=$(read_ext_module_config ${temp_file})
		GKI_EXT_MODULE_PREDEFINE=$(read_ext_module_predefine ${temp_file})
	done
	export GKI_EXT_MODULE_CONFIG GKI_EXT_MODULE_PREDEFINE
	echo GKI_EXT_MODULE_CONFIG=${GKI_EXT_MODULE_CONFIG}
	echo GKI_EXT_MODULE_PREDEFINE=${GKI_EXT_MODULE_PREDEFINE}

	if [[ ${TOP_EXT_MODULE_COPY_BUILD} -eq "1" ]]; then
		if [[ -d ${top_ext_drivers} ]]; then
			rm -rf ${top_ext_drivers}
		fi
		mkdir -p ${top_ext_drivers}
	fi

	if [[ ${AUTO_ADD_EXT_SYMBOLS} -eq "1" ]]; then
		extra_symbols="KBUILD_EXTRA_SYMBOLS +="
	fi
	ext_modules=
	for ext_module in ${EXT_MODULES}; do
		module_abs_path=`readlink -e ${ext_module}`
		module_rel_path=$(rel_path ${module_abs_path} ${ROOT_DIR})
		if [[ ${TOP_EXT_MODULE_COPY_BUILD} -eq "1" ]]; then
			if [[ `echo ${module_rel_path} | grep "\.\.\/"` ]]; then
				cp -rf ${module_abs_path} ${top_ext_drivers}
				module_rel_path=${top_ext_drivers}/${module_abs_path##*/}
			fi
		fi
		ext_modules="${ext_modules} ${module_rel_path}"
	done

	for ext_module_path in ${EXT_MODULES_PATH}; do
		sed 's:#.*$::g' ${ROOT_DIR}/${ext_module_path} | sed '/^$/d' | sed 's/^[ ]*//' | sed 's/[ ]*$//' > ${temp_file}
		while read LINE
		do
			module_abs_path=`readlink -e ${LINE}`
			module_rel_path=$(rel_path ${module_abs_path} ${ROOT_DIR})
			if [[ ${TOP_EXT_MODULE_COPY_BUILD} -eq "1" ]]; then
				if [[ `echo ${module_rel_path} | grep "\.\.\/"` ]]; then
					cp -rf ${module_abs_path} ${top_ext_drivers}
					module_rel_path=${top_ext_drivers}/${module_abs_path##*/}
				fi
			fi
			ext_modules="${ext_modules} ${module_rel_path}"

		done < ${temp_file}
	done
	EXT_MODULES=${ext_modules}

	local flag=0
	if [[ ${AUTO_ADD_EXT_SYMBOLS} -eq "1" ]]; then
		for ext_module in ${EXT_MODULES}; do
			ext_mod_rel=$(rel_path ${ext_module} ${KERNEL_DIR})
			if [[ ${flag} -eq "1" ]]; then
				sed -i "/# auto add KBUILD_EXTRA_SYMBOLS start/,/# auto add KBUILD_EXTRA_SYMBOLS end/d" ${ext_module}/Makefile
				sed -i "2 i # auto add KBUILD_EXTRA_SYMBOLS end" ${ext_module}/Makefile
				sed -i "2 i ${extra_symbols}" ${ext_module}/Makefile
				sed -i "2 i # auto add KBUILD_EXTRA_SYMBOLS start" ${ext_module}/Makefile
				echo "${ext_module}/Makefile add: ${extra_symbols}"
			fi
			extra_symbols="${extra_symbols} ${ext_mod_rel}/Module.symvers"
			flag=1
		done
	fi

	export EXT_MODULES
	echo EXT_MODULES=${EXT_MODULES}

	rm ${temp_file}
}

export -f prepare_module_build

function extra_cmds() {
	if [[ ${AUTO_ADD_EXT_SYMBOLS} -eq "1" ]]; then
		for ext_module in ${EXT_MODULES}; do
			sed -i "/# auto add KBUILD_EXTRA_SYMBOLS start/,/# auto add KBUILD_EXTRA_SYMBOLS end/d" ${ext_module}/Makefile
		done
	fi

	if [[ ${TOP_EXT_MODULE_COPY_BUILD} -eq "1" ]]; then
		if [[ -d ${top_ext_drivers} ]]; then
			rm -rf ${top_ext_drivers}
		fi
	fi

	for FILE in ${FILES}; do
		if [[ "${FILE}" =~ \.dtbo ]]  && \
			[ -n "${DTS_EXT_DIR}" ] && [ -f "${OUT_DIR}/${DTS_EXT_DIR}/${FILE}" ] ; then
			MKDTIMG_DTBOS="${MKDTIMG_DTBOS} ${DIST_DIR}/${FILE}"
		fi
	done
	export MKDTIMG_DTBOS

	set +x
	modules_install
	set -x

	local src_dir=$(echo ${MODULES_STAGING_DIR}/lib/modules/*)
	pushd ${src_dir}
	cp modules.order modules_order.back
	: > modules.order
	set +x
	while read LINE
	do
		find -name ${LINE} >> modules.order
	done < ${OUT_AMLOGIC_DIR}/modules/modules.order
	set -x
	sed -i "s/^\.\///" modules.order
	: > ${OUT_AMLOGIC_DIR}/ext_modules/ext_modules.order
	ext_modules=
	for ext_module in ${EXT_MODULES}; do
		if [[ ${ext_module} =~ "../" ]]; then
			ext_module_old=${ext_module}
			ext_module=${ext_module//\.\.\//}
			ext_dir=$(dirname ${ext_module})
			[[ -d extra/${ext_module} ]] && rm -rf extra/${ext_dir}
			mkdir -p extra/${ext_dir}
			cp -rf extra/${ext_module_old} extra/${ext_dir}

			ext_modules_order_file=$(ls extra/${ext_module}/modules.order.*)
			ext_dir_top=${ext_module%/*}
			sed -i "s/\.\.\///g" ${ext_modules_order_file}
			cat ${ext_modules_order_file} >> modules.order
			cat ${ext_modules_order_file} | awk -F/ '{print $NF}' >> ${OUT_AMLOGIC_DIR}/ext_modules/ext_modules.order
			: > ${ext_modules_order_file}
		else
			ext_modules_order_file=$(ls extra/${ext_module}/modules.order.*)
			cat ${ext_modules_order_file} >> modules.order
			cat ${ext_modules_order_file} | awk -F/ '{print $NF}' >> ${OUT_AMLOGIC_DIR}/ext_modules/ext_modules.order
			: > ${ext_modules_order_file}
		fi
		ext_modules="${ext_modules} ${ext_module}"
	done
	EXT_MODULES=${ext_modules}
	echo EXT_MODULES=${EXT_MODULES}
	export EXT_MODULES

	head -n ${ramdisk_last_line} modules.order > system_dlkm_modules
	file_last_line=`sed -n "$=" modules.order`
	let line=${file_last_line}-${ramdisk_last_line}
	tail -n ${line} modules.order > vendor_dlkm_modules
	export MODULES_LIST=${src_dir}/system_dlkm_modules
	if [ "${ARCH}" = "arm64" -a -z ${FAST_BUILD} ]; then
		export VENDOR_DLKM_MODULES_LIST=${src_dir}/vendor_dlkm_modules
	fi

	popd

	if [[ -z ${ANDROID_PROJECT} ]] && [[ -d ${OUT_DIR}/${DTS_EXT_DIR} ]]; then
		FILES="$FILES `ls ${OUT_DIR}/${DTS_EXT_DIR}`"
	fi

	if [[ -f ${KERNEL_BUILD_VAR_FILE} ]]; then
		: > ${KERNEL_BUILD_VAR_FILE}
		echo "KERNEL_DIR=${KERNEL_DIR}" >>  ${KERNEL_BUILD_VAR_FILE}
		echo "COMMON_OUT_DIR=${COMMON_OUT_DIR}" >>  ${KERNEL_BUILD_VAR_FILE}
		echo "OUT_DIR=${OUT_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "DIST_DIR=${DIST_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "OUT_AMLOGIC_DIR=${OUT_AMLOGIC_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "MODULES_STAGING_DIR=${MODULES_STAGING_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "MODULES_PRIVATE_DIR=${MODULES_PRIVATE_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "INITRAMFS_STAGING_DIR=${INITRAMFS_STAGING_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "SYSTEM_DLKM_STAGING_DIR=${SYSTEM_DLKM_STAGING_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "VENDOR_DLKM_STAGING_DIR=${VENDOR_DLKM_STAGING_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "MKBOOTIMG_STAGING_DIR=${MKBOOTIMG_STAGING_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
	fi

	for module_path in ${PREBUILT_MODULES_PATH}; do
		find ${module_path} -type f -name "*.ko" -exec cp {} ${DIST_DIR} \;
	done
}

export -f extra_cmds

function mod_probe() {
	local ko=$1
	local loop
	for loop in `grep "^$ko:" modules.dep | sed 's/.*://'`; do
		mod_probe $loop
		echo insmod $loop >> __install.sh
	done
}

function mod_probe_recovery() {
	local ko=$1
	local loop
	for loop in `grep "^$ko:" modules_recovery.dep | sed 's/.*://'`; do
		mod_probe_recovery $loop
		echo insmod $loop >> __install_recovery.sh
	done
}

function adjust_sequence_modules_loading() {
	if [[ -n $1 ]]; then
		chips=$1
	fi

	source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/modules_sequence_list
	cp modules.dep modules.dep.temp

	soc_module=()
	for chip in ${chips[@]}; do
		chip_module=`ls amlogic-*-soc-${chip}.ko`
		soc_module=(${soc_module[@]} ${chip_module[@]})
	done
	echo soc_module=${soc_module[*]}

	delete_soc_module=()
	if [[ ${#soc_module[@]} == 0 ]]; then
		echo "Use all soc module"
	else
		for module in `ls amlogic-*-soc-*`; do
			if [[ ! "${soc_module[@]}" =~ "${module}" ]] ; then
				echo Delete soc module: ${module}
				sed -n "/${module}:/p" modules.dep.temp
				sed -i "/${module}:/d" modules.dep.temp
				delete_soc_module=(${delete_soc_module[@]} ${module})
			fi
		done
		echo delete_soc_module=${delete_soc_module[*]}
	fi

	if [[ -n ${CLK_SOC_MODULE} ]]; then
		delete_clk_soc_modules=()
		for module in `ls amlogic-clk-soc-*`; do
			if [[ "${CLK_SOC_MODULE}" != "${module}" ]] ; then
				echo Delete clk soc module: ${module}
				sed -n "/${module}:/p" modules.dep.temp
				sed -i "/${module}:/d" modules.dep.temp
				delete_clk_soc_modules=(${delete_clk_soc_modules[@]} ${module})
			fi
		done
		echo delete_clk_soc_modules=${delete_clk_soc_modules[*]}
	fi

	if [[ -n ${PINCTRL_SOC_MODULE} ]]; then
		delete_pinctrl_soc_modules=()
		for module in `ls amlogic-pinctrl-soc-*`; do
			if [[ "${PINCTRL_SOC_MODULE}" != "${module}" ]] ; then
				echo Delete pinctrl soc module: ${module}
				sed -n "/${module}:/p" modules.dep.temp
				sed -i "/${module}:/d" modules.dep.temp
				delete_pinctrl_soc_modules=(${delete_pinctrl_soc_modules[@]} ${module})
			fi
		done
		echo delete_pinctrl_soc_modules=${delete_pinctrl_soc_modules[*]}
	fi

	in_line_i=a
	delete_type_modules=()
	echo "TYPE_MODULE_SELECT_MODULE=${TYPE_MODULE_SELECT_MODULE}"
	mkdir temp_dir
	cd temp_dir
	in_temp_dir=y
	for element in ${TYPE_MODULE_SELECT_MODULE}; do
		if [[ ${in_temp_dir} = y ]]; then
			cd ../
			rm -r temp_dir
			in_temp_dir=
		fi
		if [[ ${in_line_i} = a ]]; then
			in_line_i=b
			type_module=${element}
			select_modules_i=0
			select_modules_count=
			select_modules=
		elif [[ ${in_line_i} = b ]]; then
			in_line_i=c
			select_modules_count=${element}
		else
			let select_modules_i+=1
			select_modules="${select_modules} ${element}"
			if [[ ${select_modules_i} -eq ${select_modules_count} ]]; then
				in_line_i=a
				echo type_module=$type_module select_modules=$select_modules
				for module in `ls ${type_module}`; do
					dont_delete_module=0
					for select_module in ${select_modules}; do
						if [[ "${select_module}" == "${module}" ]] ; then
							dont_delete_module=1
							break;
						fi
					done
					if [[ ${dont_delete_module} != 1 ]]; then
						echo Delete module: ${module}
						sed -n "/${module}:/p" modules.dep.temp
						sed -i "/${module}:/d" modules.dep.temp
						delete_type_modules=(${delete_type_modules[@]} ${module})
					fi
				done
				echo delete_type_modules=${delete_type_modules[*]}
			fi
		fi
	done
	if [[ -n ${in_temp_dir} ]]; then
		cd ../
		rm -r temp_dir
	fi

	black_modules=()
	mkdir service_module
	echo  MODULES_SERVICE_LOAD_LIST=${MODULES_SERVICE_LOAD_LIST[@]}
	BLACK_AND_SERVICE_LIST=(${MODULES_LOAD_BLACK_LIST[@]} ${MODULES_SERVICE_LOAD_LIST[@]})
	echo ${BLACK_AND_SERVICE_LIST[@]}
	for module in ${BLACK_AND_SERVICE_LIST[@]}; do
		modules=`ls ${module}*`
		black_modules=(${black_modules[@]} ${modules[@]})
	done
	if [[ ${#black_modules[@]} == 0 ]]; then
		echo "black_modules is null, don't delete modules, MODULES_LOAD_BLACK_LIST=${MODULES_LOAD_BLACK_LIST[*]}"
	else
		echo black_modules=${black_modules[*]}
		for module in ${black_modules[@]}; do
			echo Delete module: ${module}
			sed -n "/${module}:/p" modules.dep.temp
			sed -i "/${module}:/d" modules.dep.temp
		done
	fi

	cat modules.dep.temp | cut -d ':' -f 2 > modules.dep.temp1
	delete_modules=(${delete_soc_module[@]} ${delete_clk_soc_modules[@]} ${delete_pinctrl_soc_modules[@]} ${delete_type_modules[@]} ${black_modules[@]})
	for module in ${delete_modules[@]}; do
		if [[ ! `ls $module` ]]; then
			continue
		fi
		match=`sed -n "/${module}/=" modules.dep.temp1`
		for match in ${match[@]}; do
			match_count=(${match_count[@]} $match)
		done
		if [[ ${#match_count[@]} != 0 ]]; then
			echo "Error ${#match_count[@]} modules depend on ${module}, please modify:"
			echo ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/modules_sequence_list:MODULES_LOAD_BLACK_LIST
			exit
		fi
		if [[ -n ${ANDROID_PROJECT} ]]; then
			for service_module_temp in ${MODULES_SERVICE_LOAD_LIST[@]}; do
				if [[ ${module} = ${service_module_temp} ]]; then
					mv ${module} service_module
				fi
			done
		fi
		rm -f ${module}
	done
	rm -f modules.dep.temp1
	touch modules.dep.temp1

	for module in ${RAMDISK_MODULES_LOAD_LIST[@]}; do
		echo RAMDISK_MODULES_LOAD_LIST: $module
		sed -n "/${module}:/p" modules.dep.temp
		sed -n "/${module}:/p" modules.dep.temp >> modules.dep.temp1
		sed -i "/${module}:/d" modules.dep.temp
		sed -n "/${module}.*\.ko:/p" modules.dep.temp
		sed -n "/${module}.*\.ko:/p" modules.dep.temp >> modules.dep.temp1
		sed -i "/${module}.*\.ko:/d" modules.dep.temp
	done

	if [[ -n ${ANDROID_PROJECT} ]]; then
		cp modules.dep.temp modules_recovery.dep.temp
		cp modules.dep.temp1 modules_recovery.dep.temp1
	fi

	for module in ${VENDOR_MODULES_LOAD_FIRST_LIST[@]}; do
		echo VENDOR_MODULES_LOAD_FIRST_LIST: $module
		sed -n "/${module}:/p" modules.dep.temp
		sed -n "/${module}:/p" modules.dep.temp >> modules.dep.temp1
		sed -i "/${module}:/d" modules.dep.temp
		sed -n "/${module}.*\.ko:/p" modules.dep.temp
		sed -n "/${module}.*\.ko:/p" modules.dep.temp >> modules.dep.temp1
		sed -i "/${module}.*\.ko:/d" modules.dep.temp
	done

	if [ -f modules.dep.temp2 ]; then
		rm modules.dep.temp2
	fi
	touch modules.dep.temp2
	for module in ${VENDOR_MODULES_LOAD_LAST_LIST[@]}; do
		echo VENDOR_MODULES_LOAD_FIRST_LIST: $module
		sed -n "/${module}:/p" modules.dep.temp
		sed -n "/${module}:/p" modules.dep.temp >> modules.dep.temp2
		sed -i "/${module}:/d" modules.dep.temp
		sed -n "/${module}.*\.ko:/p" modules.dep.temp
		sed -n "/${module}.*\.ko:/p" modules.dep.temp >> modules.dep.temp2
		sed -i "/${module}.*\.ko:/d" modules.dep.temp
	done

	cat modules.dep.temp >> modules.dep.temp1
	cat modules.dep.temp2 >> modules.dep.temp1

	cp modules.dep.temp1 modules.dep
	rm modules.dep.temp
	rm modules.dep.temp1
	rm modules.dep.temp2

	if [[ -n ${ANDROID_PROJECT} ]]; then
		for module in ${RECOVERY_MODULES_LOAD_LIST[@]}; do
			echo RECOVERY_MODULES_LOAD_LIST: $module
			sed -n "/${module}:/p" modules_recovery.dep.temp
			sed -n "/${module}:/p" modules_recovery.dep.temp >> modules_recovery.dep.temp1
			sed -i "/${module}:/d" modules_recovery.dep.temp
			sed -n "/${module}.*\.ko:/p" modules_recovery.dep.temp
			sed -n "/${module}.*\.ko:/p" modules_recovery.dep.temp >> modules_recovery.dep.temp1
			sed -i "/${module}.*\.ko:/d" modules_recovery.dep.temp
		done

		cat modules_recovery.dep.temp >> modules_recovery.dep.temp1

		cp modules_recovery.dep.temp1 modules_recovery.dep
		rm modules_recovery.dep.temp
		rm modules_recovery.dep.temp1
	fi
}

create_ramdisk_vendor_recovery() {
	install_temp=$1
	if [[ -n ${ANDROID_PROJECT} ]]; then
		recovery_install_temp=$2
	fi
	source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/modules_sequence_list
	ramdisk_module_i=${#RAMDISK_MODULES_LOAD_LIST[@]}
	while [ ${ramdisk_module_i} -gt 0 ]; do
		let ramdisk_module_i--
		echo ramdisk_module_i=$ramdisk_module_i ${RAMDISK_MODULES_LOAD_LIST[${ramdisk_module_i}]}
		if [[ `grep "${RAMDISK_MODULES_LOAD_LIST[${ramdisk_module_i}]}" ${install_temp}` ]]; then
			last_ramdisk_module=${RAMDISK_MODULES_LOAD_LIST[${ramdisk_module_i}]}
			break;
		fi
	done
	# last_ramdisk_module=${RAMDISK_MODULES_LOAD_LIST[${#RAMDISK_MODULES_LOAD_LIST[@]}-1]}
	last_ramdisk_module_line=`sed -n "/${last_ramdisk_module}/=" ${install_temp}`
	for line in ${last_ramdisk_module_line}; do
		ramdisk_last_line=${line}
	done
	export ramdisk_last_line

	if [[ -n ${ANDROID_PROJECT} ]]; then
		recovery_module_i=${#RECOVERY_MODULES_LOAD_LIST[@]}
		#when RECOVERY_MODULES_LOAD_LIST is NULL
		if [ ${#RECOVERY_MODULES_LOAD_LIST[@]}  -eq  0 ]; then
			last_recovery_module=${last_ramdisk_module}
		fi
		while [ ${recovery_module_i} -gt 0 ]; do
			let recovery_module_i--
			echo recovery_module_i=$recovery_module_i ${RECOVERY_MODULES_LOAD_LIST[${recovery_module_i}]}
			if [[ `grep "${RECOVERY_MODULES_LOAD_LIST[${recovery_module_i}]}" ${recovery_install_temp}` ]]; then
				last_recovery_module=${RECOVERY_MODULES_LOAD_LIST[${recovery_module_i}]}
				break;
			fi
		done
		# last_recovery_module=${RECOVERY_MODULES_LOAD_LIST[${#RECOVERY_MODULES_LOAD_LIST[@]}-1]}
		last_recovery_module_line=`sed -n "/${last_recovery_module}/=" ${recovery_install_temp}`
		for line in ${last_recovery_module_line}; do
			recovery_last_line=${line}
		done

		sed -n "${ramdisk_last_line},${recovery_last_line}p" ${recovery_install_temp} > recovery_install.sh
		sed -i "1d" recovery_install.sh
		mkdir recovery
		cat recovery_install.sh | cut -d ' ' -f 2 > recovery/recovery_modules.order
		if [ ${#RECOVERY_MODULES_LOAD_LIST[@]} -ne 0 ]; then
			cat recovery_install.sh | cut -d ' ' -f 2 | xargs cp -t recovery/
		fi
		sed -i '1s/^/#!\/bin\/sh\n\nset -x\n/' recovery_install.sh
		echo "echo Install recovery modules success!" >> recovery_install.sh
		chmod 755 recovery_install.sh
		mv recovery_install.sh recovery/
	fi

	head -n ${ramdisk_last_line} ${install_temp} > ramdisk_install.sh
	mkdir ramdisk
	cat ramdisk_install.sh | cut -d ' ' -f 2 > ramdisk/ramdisk_modules.order
	cat ramdisk_install.sh | cut -d ' ' -f 2 | xargs mv -t ramdisk/

	sed -i '1s/^/#!\/bin\/sh\n\nset -x\n/' ramdisk_install.sh
	echo "echo Install ramdisk modules success!" >> ramdisk_install.sh
	chmod 755 ramdisk_install.sh
	mv ramdisk_install.sh ramdisk/

	file_last_line=`sed -n "$=" ${install_temp}`
	let line=${file_last_line}-${ramdisk_last_line}
	tail -n ${line} ${install_temp} > vendor_install.sh
	mkdir vendor
	cat vendor_install.sh | cut -d ' ' -f 2 > vendor/vendor_modules.order
	cat vendor_install.sh | cut -d ' ' -f 2 | xargs mv -t vendor/

	sed -i '1s/^/#!\/bin\/sh\n\nset -x\n/' vendor_install.sh
	echo "echo Install vendor modules success!" >> vendor_install.sh
	chmod 755 vendor_install.sh
	mv vendor_install.sh vendor/
}

function modules_install() {
	arg1=$1

	rm -rf ${OUT_AMLOGIC_DIR}
	mkdir -p ${OUT_AMLOGIC_DIR}
	mkdir -p ${OUT_AMLOGIC_DIR}/modules
	mkdir -p ${OUT_AMLOGIC_DIR}/ext_modules
	mkdir -p ${OUT_AMLOGIC_DIR}/symbols

	local MODULES_ROOT_DIR=$(echo ${MODULES_STAGING_DIR}/lib/modules/*)
	pushd ${MODULES_ROOT_DIR}
	local common_drivers=${COMMON_DRIVERS_DIR##*/}
	local modules_list=$(find -type f -name "*.ko")
	for module in ${modules_list}; do
		if [[ -n ${ANDROID_PROJECT} ]]; then			# copy internal build modules
			if [[ `echo ${module} | grep -E "\.\/kernel\/|\/${common_drivers}\/"` ]]; then
				cp ${module} ${OUT_AMLOGIC_DIR}/modules/
			else
				cp ${module} ${OUT_AMLOGIC_DIR}/ext_modules/
			fi
		else							# copy all modules, include external modules
			cp ${module} ${OUT_AMLOGIC_DIR}/modules/
		fi
	done

	if [[ -n ${ANDROID_PROJECT} ]]; then				# internal build modules
		grep -E "^kernel\/|^${common_drivers}\/" modules.dep > ${OUT_AMLOGIC_DIR}/modules/modules.dep
	else								# all modules, include external modules
		cp modules.dep ${OUT_AMLOGIC_DIR}/modules
	fi
	popd

	pushd ${OUT_AMLOGIC_DIR}/modules
	sed -i 's#[^ ]*/##g' modules.dep

	adjust_sequence_modules_loading "${arg1[*]}"

	touch __install.sh
	touch modules.order
	for loop in `cat modules.dep | sed 's/:.*//'`; do
	        mod_probe $loop
		echo $loop >> modules.order
	        echo insmod $loop >> __install.sh
	done

	cat __install.sh  | awk ' {
		if (!cnt[$2]) {
			print $0;
			cnt[$2]++;
		}
	}' > __install.sh.tmp

	cp modules.order modules.order.back
	cut -d ' ' -f 2 __install.sh.tmp > modules.order

	if [[ -n ${ANDROID_PROJECT} ]]; then
		touch __install_recovery.sh
		touch modules_recovery.order
		for loop in `cat modules_recovery.dep | sed 's/:.*//'`; do
		        mod_probe_recovery $loop
			echo $loop >> modules_recovery.order
		        echo insmod $loop >> __install_recovery.sh
		done

		cat __install_recovery.sh  | awk ' {
			if (!cnt[$2]) {
				print $0;
				cnt[$2]++;
			}
		}' > __install_recovery.sh.tmp

		cut -d ' ' -f 2 __install_recovery.sh.tmp > modules_recovery.order
	fi
	create_ramdisk_vendor_recovery __install.sh.tmp __install_recovery.sh.tmp

	if [[ -n ${MANUAL_INSMOD_MODULE} ]]; then
		install_file=manual_install.sh
	else
		install_file=install.sh
	fi
	echo "#!/bin/sh" > ${install_file}
	echo "cd ramdisk" >> ${install_file}
	echo "./ramdisk_install.sh" >> ${install_file}
	echo "cd ../vendor" >> ${install_file}
	echo "./vendor_install.sh" >> ${install_file}
	echo "cd ../" >> ${install_file}
	chmod 755 ${install_file}

	echo "/modules/: all `wc -l modules.dep | awk '{print $1}'` modules."
	rm __install.sh __install.sh.tmp

	if [[ -n ${ANDROID_PROJECT} ]]; then
		rm __install_recovery.sh __install_recovery.sh.tmp
	fi

	popd

	cp ${OUT_DIR}/vmlinux ${OUT_AMLOGIC_DIR}/symbols
	find ${OUT_DIR} -type f -name "*.ko" -exec cp {} ${OUT_AMLOGIC_DIR}/symbols \;
	for ext_module in ${EXT_MODULES}; do
		find ${COMMON_OUT_DIR}/${ext_module} -type f -name "*.ko" -exec cp {} ${OUT_AMLOGIC_DIR}/symbols \;
	done
}
export -f modules_install

function rebuild_rootfs() {
	pushd ${OUT_AMLOGIC_DIR}

	local ARCH=arm64
	if [[ -n $1 ]]; then
		ARCH=$1
	fi

	rm rootfs -rf
	mkdir rootfs
	cp ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot	rootfs
	cd rootfs
	dd if=rootfs_base.cpio.gz.uboot of=rootfs_base.cpio.gz bs=64 skip=1
	gunzip rootfs_base.cpio.gz
	mkdir rootfs
	cd rootfs
	cpio -i -F ../rootfs_base.cpio
	if [ -d ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/customer ]; then
		cp ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/customer . -rf
	fi
	cp -rf ../../modules .

	find . | cpio -o -H newc | gzip > ../rootfs_new.cpio.gz
	cd ../
	mkimage -A ${ARCH} -O linux -T ramdisk -C none -d rootfs_new.cpio.gz rootfs_new.cpio.gz.uboot
	mv rootfs_new.cpio.gz.uboot ../
	cd ../
	cp rootfs_new.cpio.gz.uboot ${DIST_DIR}

	popd
}
export -f rebuild_rootfs

function check_undefined_symbol() {
	pushd ${OUT_AMLOGIC_DIR}/modules
	echo
	echo "========================================================"
	echo "Functions or variables not defined in this module refer to which module."
	if [[ -n ${LLVM} ]]; then
		compile_tool=${ROOT_DIR}/${CLANG_PREBUILT_BIN}/llvm-
	elif [[ -n ${CROSS_COMPILE} ]]; then
		compile_tool=${CROSS_COMPILE}
	else
		echo "can't find compile tool"
	fi
	${compile_tool}nm ${DIST_DIR}/vmlinux | grep -E " T | D | B | R | W "> vmlinux_T.txt
	# cat __install.sh | grep "insmod" | cut -d ' ' -f 2 > module_list.txt
	cat ramdisk/ramdisk_install.sh | grep "insmod" | cut -d ' ' -f 2 > module_list.txt
	cat vendor/vendor_install.sh | grep "insmod" | cut -d ' ' -f 2 >> module_list.txt
	cp ramdisk/*.ko .
	cp vendor/*.ko .
	while read LINE
	do
		echo ${LINE}
		for U in `${compile_tool}nm ${LINE} | grep " U " | sed -e 's/^\s*//' -e 's/\s*$//' | cut -d ' ' -f 2`
		do
			#echo ${U}
			U_v=`grep -w ${U} vmlinux_T.txt`
			in_vmlinux=0
			if [ -n "${U_v}" ];
			then
				#printf "\t%-50s <== vmlinux\n" ${U}
				in_vmlinux=1
				continue
			fi
			in_module=0
			MODULE=
			while read LINE1
			do
				U_m=`${compile_tool}nm ${LINE1} | grep -E " T | D | B | R " | grep -v "\.cfi_jt" | grep "${U}"`
				if [ -n "${U_m}" ];
				then
					in_module=1
					MODULE=${LINE1}
				fi
			done < module_list.txt
			if [ ${in_module} -eq "1" ];
			then
				printf "\t%-50s <== %s\n" ${U} ${MODULE}
				continue
			else
				printf "\t%-50s <== none\n" ${U}
			fi
		done
		echo
		echo
	done  < module_list.txt
	rm vmlinux_T.txt
	rm module_list.txt
	rm *.ko
	popd
}
export -f check_undefined_symbol
