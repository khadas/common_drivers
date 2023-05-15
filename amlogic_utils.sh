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
		if [[ ${ANDROID_VERSION} == p || ${ANDROID_VERSION} == P ]]; then
			KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${FRAGMENT_CONFIG_UPGRADE_P}
		fi
	fi

	if [[ ${IN_BUILD_GKI_10} == 1 ]]; then
		local temp_file=`mktemp /tmp/config.XXXXXXXXXXXX`
		echo "CONFIG_MODULE_SIG_ALL=y" >> ${temp_file}
		KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${temp_file}
		rm ${temp_file}
	fi

	if [[ -n ${DEV_CONFIGS} ]]; then
		config_list=$(echo ${DEV_CONFIGS}|sed 's/+/ /g')
		#verify the extra config is in the right path and merge the config
		CONFIG_DIR=arch/${ARCH}/configs
		for config_name in ${config_list[@]}
		do
			if [[ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/${CONFIG_DIR}/${config_name} ]]; then
				KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/${CONFIG_DIR}/${config_name}
			elif [[ -f ${config_name} ]]; then
				KCONFIG_CONFIG=${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KCONFIG_DEFCONFIG} ${config_name}
			else
				echo "ERROR: config file ${config_name} is not in the right path!!"
				exit
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

	head -n ${ramdisk_last_line} modules.order > vendor_boot_modules
	file_last_line=`sed -n "$=" modules.order`
	let line=${file_last_line}-${ramdisk_last_line}
	tail -n ${line} modules.order > vendor_dlkm_modules
	export MODULES_LIST=${src_dir}/vendor_boot_modules
	if [[ "${ARCH}" = "arm64" && -z ${FAST_BUILD} ]]; then
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
		echo "DIST_GKI_DIR=${DIST_GKI_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "FULL_KERNEL_VERSION=${FULL_KERNEL_VERSION}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "GKI_MODULES_LOAD_WHITE_LIST=\"${GKI_MODULES_LOAD_WHITE_LIST[*]}\"" >> ${KERNEL_BUILD_VAR_FILE}
	fi

	for module_path in ${PREBUILT_MODULES_PATH}; do
		find ${module_path} -type f -name "*.ko" -exec cp {} ${DIST_DIR} \;
	done
}

export -f extra_cmds

function bazel_extra_cmds() {
	modules_install

	if [[ -f ${KERNEL_BUILD_VAR_FILE} ]]; then
		: > ${KERNEL_BUILD_VAR_FILE}
		echo "KERNEL_DIR=${KERNEL_DIR}" >>  ${KERNEL_BUILD_VAR_FILE}
		echo "COMMON_OUT_DIR=${COMMON_OUT_DIR}" >>  ${KERNEL_BUILD_VAR_FILE}
		echo "DIST_DIR=${DIST_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "OUT_AMLOGIC_DIR=${OUT_AMLOGIC_DIR}" >> ${KERNEL_BUILD_VAR_FILE}
		echo "GKI_MODULES_LOAD_WHITE_LIST=\"${GKI_MODULES_LOAD_WHITE_LIST[*]}\"" >> ${KERNEL_BUILD_VAR_FILE}
	fi

	if [[ ${GKI_CONFIG} != gki_20 ]]; then
		[[ -f ${DIST_DIR}/system_dlkm_staging_archive_back.tar.gz ]] && rm -f ${DIST_DIR}/system_dlkm_staging_archive_back.tar.gz
		mv ${DIST_DIR}/system_dlkm_staging_archive.tar.gz ${DIST_DIR}/system_dlkm_staging_archive_back.tar.gz
		[[ -d ${DIST_DIR}/system_dlkm_gki10 ]] && rm -rf ${DIST_DIR}/system_dlkm_gki10
		mkdir ${DIST_DIR}/system_dlkm_gki10
		pushd ${DIST_DIR}/system_dlkm_gki10
		tar zxf ${DIST_DIR}/system_dlkm_staging_archive_back.tar.gz
		find -name "*.ko" | while read module; do
			module_name=${module##*/}
			if [[ ! `grep "/${module_name}" ${DIST_DIR}/system_dlkm.modules.load` ]]; then
				rm -f ${module}
			fi
		done
		tar czf ${DIST_DIR}/system_dlkm_staging_archive.tar.gz lib
		popd
		rm -rf ${DIST_DIR}/system_dlkm_gki10
	fi
}

export -f bazel_extra_cmds

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

	GKI_MODULES_LOAD_BLACK_LIST=()
	if [[ "${FULL_KERNEL_VERSION}" != "common13-5.15" ]]; then
		gki_modules_temp_file=`mktemp /tmp/config.XXXXXXXXXXXX`
		cp ${ROOT_DIR}/${KERNEL_DIR}/android/gki_system_dlkm_modules ${gki_modules_temp_file}

		for module in ${GKI_MODULES_LOAD_WHITE_LIST[@]}; do
			sed -i "/\/${module}/d" ${gki_modules_temp_file}
		done

		for module in `cat ${gki_modules_temp_file}`; do
			module=${module##*/}
			GKI_MODULES_LOAD_BLACK_LIST[${#GKI_MODULES_LOAD_BLACK_LIST[*]}]=${module}
		done
		rm -f ${gki_modules_temp_file}

		for module in ${GKI_MODULES_LOAD_BLACK_LIST[@]}; do
			echo Delete module: ${module}
			sed -n "/${module}:/p" modules.dep.temp
			sed -i "/${module}:/d" modules.dep.temp
		done
	fi

	cat modules.dep.temp | cut -d ':' -f 2 > modules.dep.temp1
	delete_modules=(${delete_soc_module[@]} ${delete_clk_soc_modules[@]} ${delete_pinctrl_soc_modules[@]} ${delete_type_modules[@]} ${black_modules[@]} ${GKI_MODULES_LOAD_BLACK_LIST[@]})
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
	if [[ -n ${last_ramdisk_module} ]]; then
		last_ramdisk_module_line=`sed -n "/${last_ramdisk_module}/=" ${install_temp}`
		for line in ${last_ramdisk_module_line}; do
			ramdisk_last_line=${line}
		done
	else
		ramdisk_last_line=1
	fi
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
		if [[ -n ${last_recovery_module} ]]; then
			last_recovery_module_line=`sed -n "/${last_recovery_module}/=" ${recovery_install_temp}`
			for line in ${last_recovery_module_line}; do
				recovery_last_line=${line}
			done
		else
			recovery_last_line=1
		fi
		sed -n "${ramdisk_last_line},${recovery_last_line}p" ${recovery_install_temp} > recovery_install.sh
		if [[ -n ${last_ramdisk_module} ]]; then
			sed -i "1d" recovery_install.sh
		fi
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

	export OUT_AMLOGIC_DIR=${OUT_AMLOGIC_DIR:-$(readlink -m ${COMMON_OUT_DIR}/amlogic)}
	echo $OUT_AMLOGIC_DIR
	rm -rf ${OUT_AMLOGIC_DIR}
	mkdir -p ${OUT_AMLOGIC_DIR}
	mkdir -p ${OUT_AMLOGIC_DIR}/modules
	mkdir -p ${OUT_AMLOGIC_DIR}/ext_modules
	mkdir -p ${OUT_AMLOGIC_DIR}/symbols

	if [[ ${BAZEL} == "1" ]]; then
		local output="out/bazel/output_user_root"
		for dir in `ls ${output}`; do
			if [[ ${dir} =~ ^[0-9A-Fa-f]*$ ]]; then
				digit_output=${output}/${dir}
				break
			fi
		done

		while read module
		do
			module_name=${module##*/}
			if [[ `echo ${module} | grep "^kernel\/"` ]]; then
				if [[ -f ${DIST_DIR}/${module_name} ]]; then
					cp ${DIST_DIR}/${module_name} ${OUT_AMLOGIC_DIR}/modules
				else
					module=`find ${digit_output}/execroot/ -name ${module_name} | grep "amlogic"`
					cp ${module} ${OUT_AMLOGIC_DIR}/modules
				fi
			elif [[ `echo ${module} | grep "^extra\/"` ]]; then
				cp ${DIST_DIR}/${module_name} ${OUT_AMLOGIC_DIR}/ext_modules
			else
				echo "warning unrecognized module: ${module}"
			fi
		done < ${DIST_DIR}/modules.load

		dep_file=`find ${digit_output}/execroot/ -name *.dep | grep "amlogic"`
		cp ${dep_file} ${OUT_AMLOGIC_DIR}/modules/full_modules.dep
		grep -E "^kernel\/" ${dep_file} > ${OUT_AMLOGIC_DIR}/modules/modules.dep
		touch ${module} ${OUT_AMLOGIC_DIR}/ext_modules/ext_modules.order
		for order_file in `find ${digit_output}/execroot/ -name "modules.order.*" | grep "amlogic"`; do
			echo "# ${order_file}" >> ${OUT_AMLOGIC_DIR}/ext_modules/ext_modules.order
			cat ${order_file} >> ${OUT_AMLOGIC_DIR}/ext_modules/ext_modules.order
			echo >> ${OUT_AMLOGIC_DIR}/ext_modules/ext_modules.order
		done
	else
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
	fi
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

	if [[ ${BAZEL} == "1" ]]; then
		cp ${DIST_DIR}/vmlinux ${OUT_AMLOGIC_DIR}/symbols

		find ${digit_output}/execroot -name *.ko | grep "unstripped" | while read module; do
		        cp ${module} ${OUT_AMLOGIC_DIR}/symbols
			chmod +w ${OUT_AMLOGIC_DIR}/symbols/$(basename ${module})
		done
		chmod -w ${OUT_AMLOGIC_DIR}/symbols/*.ko
	else
		cp ${OUT_DIR}/vmlinux ${OUT_AMLOGIC_DIR}/symbols
		find ${OUT_DIR} -type f -name "*.ko" -exec cp {} ${OUT_AMLOGIC_DIR}/symbols \;
		for ext_module in ${EXT_MODULES}; do
			find ${COMMON_OUT_DIR}/${ext_module} -type f -name "*.ko" -exec cp {} ${OUT_AMLOGIC_DIR}/symbols \;
		done
	fi
}
export -f modules_install

function rename_external_module_name() {
	local external_coppied
	local vendor_coppied
	sed 's/^[\t ]*\|[\t ]*$//g' ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/modules_rename.txt | sed '/^#/d;/^$/d' | sed 's/[[:space:]][[:space:]]*/ /g' | while read module_line; do
		target_module_name=`echo ${module_line%%:*} | sed '/^#/d;/^$/d'`
		modules_name=`echo ${module_line##:*} | sed '/^#/d;/^$/d'`
		[[ -f ${OUT_AMLOGIC_DIR}/ext_modules/${target_module_name} ]] && external_coppied=1
		[[ -f ${OUT_AMLOGIC_DIR}/modules/vendor/${target_module_name} ]] && vendor_coppied=1
		echo target_module_name=$target_module_name modules_name=$modules_name external_coppied=$external_coppied vendor_coppied=$vendor_coppied
		for module in ${modules_name}; do
			echo module=$module
			if [[ -f ${OUT_AMLOGIC_DIR}/ext_modules/${module} ]]; then
				if [[ -z ${external_coppied} ]]; then
					cp ${OUT_AMLOGIC_DIR}/ext_modules/${module} ${OUT_AMLOGIC_DIR}/ext_modules/${target_module_name}
					external_coppied=1
				fi
				rm -f ${OUT_AMLOGIC_DIR}/ext_modules/${module}
			fi
			if [[ -f ${OUT_AMLOGIC_DIR}/modules/vendor/${module} ]]; then
				if [[ -z ${vendor_coppied} ]]; then
					cp ${OUT_AMLOGIC_DIR}/modules/vendor/${module} ${OUT_AMLOGIC_DIR}/modules/vendor/${target_module_name}
					vendor_coppied=1
				fi
				rm -f ${OUT_AMLOGIC_DIR}/modules/vendor/${module}
			fi
		done
		external_coppied=
		vendor_coppied=
	done
}
export -f rename_external_module_name

# function rebuild_rootfs can rebuild the rootfs if rootfs_base.cpio.gz.uboot exist
function rebuild_rootfs() {
	echo
        echo "========================================================"
	if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
		echo "Rebuild rootfs in order to install modules!"
	else
		echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
		return
	fi

	pushd ${OUT_AMLOGIC_DIR}

	local ARCH=arm64
	if [[ -n $1 ]]; then
		ARCH=$1
	fi

	rm rootfs -rf
	mkdir rootfs
	cp ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot rootfs
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

# function check_undefined_symbol can check the dependence among the modules
# parameter:
#	--modules_depend
function check_undefined_symbol() {
	if [[ ${MODULES_DEPEND} != "1" ]]; then
		return
	fi

	echo "========================================================"
	echo "print modules depend"

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

function reorganized_abi_symbol_list_file() { # delete the extra information but symbol
	cat $@ | sed '/^$/d' >> ${symbol_tmp} # remove blank lines
	sed -i '/^\[/d' ${symbol_tmp} # remove the title
	sed -i '/^\#/d' ${symbol_tmp} # remove the comment
}
export -f reorganized_abi_symbol_list_file

function abi_symbol_list_detect () { # detect symbol information that should be submitted or fix
	symbol_file1=${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/android/abi_gki_aarch64_amlogic
	symbol_file2=${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/android/abi_gki_aarch64_amlogic.10
	symbol_file3=${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/android/abi_gki_aarch64_amlogic.debug
	symbol_file4=${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/android/abi_gki_aarch64_amlogic.illegal

	file_list=("${symbol_file1} ${symbol_file2} ${symbol_file3} ${symbol_file4}")
	for file in ${file_list}
	do
		local symbol_tmp=`mktemp tmp.XXXXXXXXXXXX`
		reorganized_abi_symbol_list_file "${file}"

		if [[ -s ${symbol_tmp} ]]; then
			if [[ ${file} =~ abi_gki_aarch64_amlogic.illegal ]]; then
				echo "WARNING: The symbols in ${file} are illegal, please deal with them as soon as possible!!!"
			else
				echo "WARNING: The symbols in ${file} should be submit to google!!!"
			fi
			cat ${symbol_tmp}
			echo -e "\n------------------------------------------------------------"
		fi
		rm ${symbol_tmp}
	done
}
export -f abi_symbol_list_detect

# adjust_config_action concerns three types of cmd:
# parameters:
#	--menuconfig:      make menuconfig manually based on different gki standard
#	--basicconfig:     only config kernel with google original gki_defconfig as base
#	--check_defconfig: contrast the defconfig generated in out directory with gki_defconfig and show the difference
function adjust_config_action () {
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

		if [[ -n ${BASICCONFIG} ]]; then # config kernel with gki_defconfig or make menuconfig based on it
			set -x
			defconfig_name=`basename ${GKI_BASE_CONFIG}`
			(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${defconfig_name})
			(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
			cp ${out_config} ${orig_config}
			cp ${out_defconfig} ${orig_defconfig}
			if [ "${BASICCONFIG}" = "m" ]; then # make menuconfig based on gki_defconfig
				(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" menuconfig)
			fi
			(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig)
			${KERNEL_DIR}/scripts/diffconfig ${orig_config} ${out_config} > ${changed_config}
			${KERNEL_DIR}/scripts/diffconfig ${orig_defconfig} ${out_defconfig} > ${changed_defconfig}
			if [ "${ARCH}" = "arm" ]; then
				cp ${out_defconfig} ${GKI_BASE_CONFIG}
			fi
			set +x # show the difference between the gki_defconfig and the config after make menuconfig
			echo
			echo "========================================================"
			echo "==================== .config diff   ===================="
			cat ${changed_config}
			echo "==================== defconfig diff ===================="
			cat ${changed_defconfig}
			echo "========================================================"
			echo
		elif [[ ${CHECK_DEFCONFIG} -eq "1" ]]; then # compare the defconfig generated in out directory with gki_defconfig
			set -x
			pre_defconfig_cmds
			(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" ${DEFCONFIG})
			(cd ${KERNEL_DIR} && make ${TOOL_ARGS} O=${OUT_DIR} "${MAKE_ARGS[@]}" savedefconfig) # export the defconfig to out directory
			diff -u ${ROOT_DIR}/${GKI_BASE_CONFIG} ${OUT_DIR}/defconfig
			post_defconfig_cmds
			set +x
		else # make menuconfig based on config with different gki standard
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
}
export -f adjust_config_action

# function build_part_of_kernel can only build part of kernel such as image modules or dtbs
# parameter:
#	--image:   only build image
#	--modules: only build kernel modules
#	--dtbs:    only build dtbs
function build_part_of_kernel () {
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
}

export -f build_part_of_kernel

function export_env_variable () {
	export ABI BUILD_CONFIG LTO KMI_SYMBOL_LIST_STRICT_MODE CHECK_DEFCONFIG MANUAL_INSMOD_MODULE ARCH
	export KERNEL_DIR COMMON_DRIVERS_DIR BUILD_DIR ANDROID_PROJECT GKI_CONFIG UPGRADE_PROJECT ANDROID_VERSION FAST_BUILD CHECK_GKI_20 DEV_CONFIGS
	export FULL_KERNEL_VERSION BAZEL

	echo ROOT_DIR=$ROOT_DIR
	echo ABI=${ABI} BUILD_CONFIG=${BUILD_CONFIG} LTO=${LTO} KMI_SYMBOL_LIST_STRICT_MODE=${KMI_SYMBOL_LIST_STRICT_MODE} CHECK_DEFCONFIG=${CHECK_DEFCONFIG} MANUAL_INSMOD_MODULE=${MANUAL_INSMOD_MODULE}
	echo KERNEL_DIR=${KERNEL_DIR} COMMON_DRIVERS_DIR=${COMMON_DRIVERS_DIR} BUILD_DIR=${BUILD_DIR} ANDROID_PROJECT=${ANDROID_PROJECT} GKI_CONFIG=${GKI_CONFIG} UPGRADE_PROJECT=${UPGRADE_PROJECT} ANDROID_VERSION=${ANDROID_VERSION}  FAST_BUILD=${FAST_BUILD} CHECK_GKI_20=${CHECK_GKI_20}
	echo FULL_KERNEL_VERSION=${FULL_KERNEL_VERSION} BAZEL=${BAZEL}
	echo MENUCONFIG=${MENUCONFIG} BASICCONFIG=${BASICCONFIG} IMAGE=${IMAGE} MODULES=${MODULES} DTB_BUILD=${DTB_BUILD}
}

export -f export_env_variable
