#!/bin/bash

if [[ $# < 1 ]]; then
	FULL_KERNEL_VERSION="common13-5.15"
else
	FULL_KERNEL_VERSION=$1
fi
KERNEL_DIR=${KERNEL_DIR:-common}
COMMON_DRIVERS_DIR=${COMMON_DRIVERS_DIR:-common_drivers}
T=$(pwd)
ROOT_DIR=${ROOT_DIR:-${T}}
PATCHES_PATH=${ROOT_DIR}/$(dirname $0)/${FULL_KERNEL_VERSION}

if [[ ! -d ${PATCHES_PATH} ]]; then
	echo "None patch to am, ${PATCHES_PATH}/${FULL_KERNEL_VERSION}/patches does not exist!!!"
	exit
fi

# echo KERNEL_DIR=$KERNEL_DIR COMMON_DRIVERS_DIR=$COMMON_DRIVERS_DIR ROOT_DIR=$ROOT_DIR PATCHES_PATH=$PATCHES_PATH

function am_patch()
{
	local patch=$1
	local dir=$2
	local change_id=`grep 'Change-Id' $patch | head -n1 | awk '{print $2}'`

	# echo $patch $dir
	if [ -d "$dir" ]; then
		cd $dir;
		git log -n 400 | grep $change_id 1>/dev/null 2>&1;
		if [ $? -ne 0 ]; then
			# echo "###patch ${patch##*/}###      "
			git am -q $patch 1>/dev/null 2>&1;
			if [ $? != 0 ]; then
				git am --abort
				cd $ROOT_DIR
				echo "Patch Error : Failed to patch [$patch], Need check it. exit!!!"
				exit -1
			fi
		fi
		cd $ROOT_DIR
	fi
}

function auto_patch()
{
	local patch_dir=$1
	echo patch_dir=$patch_dir

	for file in `ls $patch_dir/*.patch`; do
		local file_name=${file%.*};           #echo file_name $file_name
		local resFile=`basename $file_name`;  #echo resFile $resFile
		local dir_name1=${resFile//#/\/};     #echo dir_name $dir_name
		local dir_name=${dir_name1%/*};       #echo dir_name $dir_name
		local dir=$T/$dir_name;               #echo $dir

		am_patch $file $dir
	done
}

function traverse_patch_dir()
{
	# git am common and common_driver patches
	for file in `ls ${PATCHES_PATH}/common`; do
		# echo file=$file
		if [ -d ${PATCHES_PATH}/common/${file} ]; then
			for patch in `ls ${PATCHES_PATH}/common/${file}/*.patch`; do
				am_patch ${patch} ${KERNEL_DIR}
			done
		fi
	done

	if [[ -d ${PATCHES_PATH}/common_drivers ]]; then
		for patch in `ls ${PATCHES_PATH}/common_drivers/*.patch`; do
			am_patch ${patch} ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}
		done
	fi

	for file in `ls ${PATCHES_PATH}`; do
		[[ "${file}" == "common" || "${file}" == "common_drivers" ]] && continue

		if [ -d ${PATCHES_PATH}/${file} ]; then
			local dest_dir=${PATCHES_PATH}/${file}
			auto_patch ${dest_dir}
		fi
	done

	echo "Patch Finish: ${ROOT_DIR}"
}

traverse_patch_dir
