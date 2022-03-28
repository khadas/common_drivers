#!/bin/bash

function pre_defconfig_cmds() {
	if [[ ${AMLOGIC_BREAK_GKI} -eq "1" ]]; then
		sed -i "1i CONFIG_AMLOGIC_BREAK_GKI=y" ${ROOT_DIR}/${FRAGMENT_CONFIG}
	else
		sed -i "1i CONFIG_AMLOGIC_BREAK_GKI=n" ${ROOT_DIR}/${FRAGMENT_CONFIG}
	fi

	if [[ ${IN_KERNEL_MODULES} -eq "1" ]]; then
		sed -i "1i CONFIG_AMLOGIC_IN_KERNEL_MODULES=y" ${ROOT_DIR}/${FRAGMENT_CONFIG}
		SKIP_EXT_MODULES=1
		export SKIP_EXT_MODULES
		EXT_MODULES=
		export EXT_MODULES
	else
		sed -i "1i CONFIG_AMLOGIC_IN_KERNEL_MODULES=n" ${ROOT_DIR}/${FRAGMENT_CONFIG}
	fi
	KCONFIG_CONFIG=${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/gki_defconfig ${ROOT_DIR}/${FRAGMENT_CONFIG}
}
export -f pre_defconfig_cmds

function post_defconfig_cmds() {
	# checkout config
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG}
	pushd ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}
		# sed -i '5,${/CONFIG_AMLOGIC_BREAK_GKI/d}' ${ROOT_DIR}/${FRAGMENT_CONFIG}
		# sed -i '5,${/CONFIG_AMLOGIC_IN_KERNEL_MODULES/d}' ${ROOT_DIR}/${FRAGMENT_CONFIG}
		sed -i '/# SPDX-License-Identifier/,$!d' ${ROOT_DIR}/${FRAGMENT_CONFIG}
		# git checkout ${ROOT_DIR}/${FRAGMENT_CONFIG}
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

	for y_config in `cat $1 | grep "^CONFIG_.*=y" | sed 's/=y//'`; do
		PRE_DEFINE="$PRE_DEFINE"" -D"${y_config}
	done

	for m_config in `cat $1 | grep "^CONFIG_.*=m" | sed 's/=m//'`; do
		PRE_DEFINE="$PRE_DEFINE"" -D"${m_config}_MODULE
	done

	export GKI_EXT_MODULE_PREDEFINE=$PRE_DEFINE
	echo "GKI_EXT_MODULE_PREDEFINE=${GKI_EXT_MODULE_PREDEFINE}"
}

function prepare_module_build() {
	if [[ -z ${IN_KERNEL_MODULES} ]]; then
		read_ext_module_config $FRAGMENT_CONFIG && read_ext_module_predefine $FRAGMENT_CONFIG
	fi
}

export -f prepare_module_build

function mod_probe() {
	local ko=$1
	local loop
	for loop in `grep "$ko:" modules.dep | sed 's/.*://'`; do
		mod_probe $loop
		echo insmod $loop >> __install.sh
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

	delete_module=()
	for module in ${MODULES_LOAD_BLACK_LIST[@]}; do
		modules=`ls ${module}*`
		delete_module=(${delete_module[@]} ${modules[@]})
	done
	if [[ ${#delete_module[@]} == 0 ]]; then
		echo "No delete module, MODULES_LOAD_BLACK_LIST=${MODULES_LOAD_BLACK_LIST[*]}"
	else
		echo delete_module=${delete_module[*]}
		for module in ${delete_module[@]}; do
			echo Delete module: ${module}
			sed -n "/${module}:/p" modules.dep.temp
			sed -i "/${module}:/d" modules.dep.temp
		done
	fi

	cat modules.dep.temp | cut -d ':' -f 2 > modules.dep.temp1
	delete_modules=(${delete_soc_module[@]} ${delete_module[@]})
	for module in ${delete_modules[@]}; do
		match=`sed -n "/${module}/=" modules.dep.temp1`
		for match in ${match[@]}; do
			match_count=(${match_count[@]} $match)
		done
		if [[ ${#match_count[@]} != 0 ]]; then
			echo "Error ${#match_count[@]} modules depend on ${module}, please modify:"
			echo ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/modules_sequence_list:MODULES_LOAD_BLACK_LIST
			exit
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
}

create_ramdis_vendor() {
	install_temp=$1
	source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/scripts/amlogic/modules_sequence_list
	last_ramdisk_module=${RAMDISK_MODULES_LOAD_LIST[${#RAMDISK_MODULES_LOAD_LIST[@]}-1]}
	last_ramdisk_module_line=`sed -n "/${last_ramdisk_module}/=" ${install_temp}`
	for line in ${last_ramdisk_module_line}; do
		ramdisk_last_line=${line}
	done
	head -n ${ramdisk_last_line} ${install_temp} > ramdisk_install.sh
	mkdir ramdisk
	cat ramdisk_install.sh | cut -d ' ' -f 2 | xargs mv -t ramdisk/

	sed -i '1s/^/#!\/bin\/sh\n\nset -ex\n/' ramdisk_install.sh
	echo "echo Install ramdisk modules success!" >> ramdisk_install.sh
	chmod 755 ramdisk_install.sh
	mv ramdisk_install.sh ramdisk/

	file_last_line=`sed -n "$=" ${install_temp}`
	let line=${file_last_line}-${ramdisk_last_line}
	tail -n ${line} ${install_temp} > vendor_install.sh
	mkdir vendor
	cat vendor_install.sh | cut -d ' ' -f 2 | xargs mv -t vendor/

	sed -i '1s/^/#!\/bin\/sh\n\nset -ex\n/' vendor_install.sh
	echo "echo Install vendor modules success!" >> vendor_install.sh
	chmod 755 vendor_install.sh
	mv vendor_install.sh vendor/
}

function modules_install() {
	arg1=$1

	pushd ${DIST_DIR}
	rm modules -rf
	mkdir modules
	local modules_list=$(find ${MODULES_STAGING_DIR}/lib/modules -type f -name "*.ko")
	cp ${modules_list} modules

	local stagin_module=$(echo ${MODULES_STAGING_DIR}/lib/modules/*)
	echo stagin_module=${stagin_module}
	cp ${stagin_module}/modules.dep modules

	cd modules
	sed -i 's#[^ ]*/##g' modules.dep

	adjust_sequence_modules_loading "${arg1[*]}"

	touch __install.sh
	for loop in `cat modules.dep | sed 's/:.*//'`; do
	        mod_probe $loop
	        echo insmod $loop >> __install.sh
	done

	cat __install.sh  | awk ' {
		if (!cnt[$2]) {
			print $0;
			cnt[$2]++;
		}
	}' > __install.sh.tmp

	create_ramdis_vendor __install.sh.tmp

	cp __install.sh.tmp __install.sh

	#sed -i '1s/^/#!\/bin\/sh\n\nset -ex\n/' __install.sh
	#echo "echo Install modules success!" >> __install.sh
	#chmod 755 __install.sh

	echo "#!/bin/sh" > install.sh
	# echo "./__install.sh || reboot" >> install.sh
	# echo "./__install.sh" >> install.sh
	echo "cd ramdisk" >> install.sh
	echo "./ramdisk_install.sh" >> install.sh
	echo "cd ../vendor" >> install.sh
	echo "./vendor_install.sh" >> install.sh
	echo "cd ../" >> install.sh
	chmod 755 install.sh

	echo "/modules/: all `wc -l modules.dep | awk '{print $1}'` modules."
	rm __install.sh __install.sh.tmp modules.dep

	cd ../
	popd
}
export -f modules_install

function rebuild_rootfs() {
	pushd ${DIST_DIR}

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

	#modules_modules=`ls modules/*.ko`
	ramdisk_modules=`ls modules/ramdisk/*.ko`
	vendor_modules=`ls modules/vendor/*.ko`
	strip_modules=(${modules_modules[@]} ${ramdisk_modules[@]} ${vendor_modules[@]})
	if [[ -n ${LLVM} ]]; then
		for module in ${strip_modules[@]}; do
			 ${ROOT_DIR}/${CLANG_PREBUILT_BIN}/llvm-objcopy --strip-debug ${module}
		done
	elif [[ -n ${CROSS_COMPILE} ]]; then
		for module in ${strip_modules[@]}; do
			 ${CROSS_COMPILE}objcopy --strip-debug ${module}
		done
	else
		echo "can't find compile tool"
	fi

	find . | cpio -o -H newc | gzip > ../rootfs_new.cpio.gz
	cd ../
	mkimage -A ${ARCH} -O linux -T ramdisk -C none -d rootfs_new.cpio.gz rootfs_new.cpio.gz.uboot
	mv rootfs_new.cpio.gz.uboot ../
	cd ../

	popd
}
export -f rebuild_rootfs

function check_undefined_symbol() {
	pushd ${DIST_DIR}/modules
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
	${compile_tool}nm ../vmlinux | grep -E " T | D | B | R | W "> vmlinux_T.txt
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
