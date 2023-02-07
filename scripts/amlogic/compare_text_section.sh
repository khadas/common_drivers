#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

# ./common/common_drivers/scripts/amlogic/compare_text_section --vmlinux_gki <path>/vmlinux --vmlinux_amlogic <path>/vmlinux
function show_help {
	echo "USAGE:"
	echo
	echo "  --vmlinux_gki		ori vmlinux file, eg: <path>/vmlinux"
	echo "  --vmlinux_amlogic	amlogic vmlinux file, eg: <path>/vmlinux"
	echo "  --buildtool_dir		build tool bin path"
	echo "  --output		output dir, default: ./compare_text_tmp/"
}

for i in "$@"
do
	case $i in
	--vmlinux_gki)
		VM_GKI_FILE=$2
		shift
		;;
	--vmlinux_amlogic)
		VM_AMLOGIC_FILE=$2
		shift
		;;
	--buildtool_dir)
		BUILDTOOL_DIR=$2
		shift
		;;
	--output)
		OUTPUT_DIR=$2
		shift
		;;
	-h|--help)
		show_help
		exit 0
		;;
	*)
		shift
		;;
	esac
done

if [[ ! -e ${VM_GKI_FILE} ]]; then
	echo "gki vmlinux not exit, --vmlinux_gki ${VM_GKI_FILE}, Failed!"
	exit 0
fi

if [[ ! -e ${VM_AMLOGIC_FILE} ]]; then
	echo "amlogic vmlinux not exit, --vmlinux_amlogic ${VM_AMLOGIC_FILE}, Failed!"
	exit 0
fi

if [[ ! -d ${BUILDTOOL_DIR} ]]; then
	DEFAULT_BUILD_CONFIG_CONSTANTS=common/build.config.constants
	DEFAULT_BUILD_CONFIG_COMMON=common/build.config.common
	[[ -f ${DEFAULT_BUILD_CONFIG_CONSTANTS} ]] && source ${DEFAULT_BUILD_CONFIG_CONSTANTS}
	[[ -f ${DEFAULT_BUILD_CONFIG_COMMON} ]] && source ${DEFAULT_BUILD_CONFIG_COMMON}

	if [[ ! -d ${CLANG_PREBUILT_BIN} ]]; then
		echo "--buildtool_dir not exit, --buildtool_dir ${BUILDTOOL_DIR}, Failed!"
		exit 0
	else
		BUILDTOOL_DIR=${CLANG_PREBUILT_BIN}
	fi
fi

if [ -z ${OUTPUT_DIR} ]; then
	OUTPUT_DIR=compare_text_tmp
fi

if [ -e ${OUTPUT_DIR} ]
then
	rm -rf ${OUTPUT_DIR}
fi

echo "VM_GKI_FILE=${VM_GKI_FILE} VM_AMLOGIC_FILE=${VM_AMLOGIC_FILE} BUILDTOOL_DIR=${BUILDTOOL_DIR} OUTPUT_DIR=${OUTPUT_DIR}"

mkdir -p ${OUTPUT_DIR}
VM_GKI_FILE_NAME=${VM_GKI_FILE##*/}.gki
VM_AMLOGIC_FILE_NAME=${VM_AMLOGIC_FILE##*/}.amlogic
TEXT_GKI_FILE=${VM_GKI_FILE_NAME}.text
TEXT_AMLOGIC_FILE=${VM_AMLOGIC_FILE_NAME}.text

#copy vmlinux to output path
cp ${VM_GKI_FILE} ${OUTPUT_DIR}/${VM_GKI_FILE_NAME}
cp ${VM_AMLOGIC_FILE} ${OUTPUT_DIR}/${VM_AMLOGIC_FILE_NAME}

#objdump .text section
${BUILDTOOL_DIR}/llvm-objdump --section=.text -d ${OUTPUT_DIR}/${VM_GKI_FILE_NAME} > ${OUTPUT_DIR}/${TEXT_GKI_FILE}
${BUILDTOOL_DIR}/llvm-objdump --section=.text -d ${OUTPUT_DIR}/${VM_AMLOGIC_FILE_NAME} > ${OUTPUT_DIR}/${TEXT_AMLOGIC_FILE}

#only need compare until cmd column
clang_hash_format=`awk 'NR==7 {print $2}' ${OUTPUT_DIR}/${TEXT_GKI_FILE}`
echo clang_hash_format=${clang_hash_format}
if [ $((16#${clang_hash_format})) -gt $((16#ff)) ]; then
	awk '{NF=3}1' ${OUTPUT_DIR}/${TEXT_GKI_FILE} > ${OUTPUT_DIR}/${TEXT_GKI_FILE}.tmp
	awk '{NF=3}1' ${OUTPUT_DIR}/${TEXT_AMLOGIC_FILE} > ${OUTPUT_DIR}/${TEXT_AMLOGIC_FILE}.tmp
else
	awk '{NF=6}1' ${OUTPUT_DIR}/${TEXT_GKI_FILE} > ${OUTPUT_DIR}/${TEXT_GKI_FILE}.tmp
	awk '{NF=6}1' ${OUTPUT_DIR}/${TEXT_AMLOGIC_FILE} > ${OUTPUT_DIR}/${TEXT_AMLOGIC_FILE}.tmp
fi

diff -y --suppress-common-lines ${OUTPUT_DIR}/${TEXT_AMLOGIC_FILE}.tmp ${OUTPUT_DIR}/${TEXT_GKI_FILE}.tmp > ${OUTPUT_DIR}/diff.log.tmp

#save cmd or address different line
sed -i '/compare_text_tmp/d' ${OUTPUT_DIR}/diff.log.tmp

if [ $((16#${clang_hash_format})) -gt $((16#ff)) ]; then
	awk '$1 ~ /:/ && ($1 != $5 || $3 != $7)' ${OUTPUT_DIR}/diff.log.tmp > ${OUTPUT_DIR}/diff.log
else
	awk '$1 ~ /:/ && ($1 != $8 || $6 != $13)' ${OUTPUT_DIR}/diff.log.tmp > ${OUTPUT_DIR}/diff.log
fi

#output compare info
if [ -s ${OUTPUT_DIR}/diff.log ]; then
	FIRST_ADDRESS=$(awk '{print $1}' ${OUTPUT_DIR}/diff.log | sed -n '1p')
	ADDR_INFO=`${BUILDTOOL_DIR}/llvm-addr2line -e ${OUTPUT_DIR}/${VM_AMLOGIC_FILE_NAME} ${FIRST_ADDRESS%%:*}`
	echo "Failed, first different with address: ${FIRST_ADDRESS}, info: ${ADDR_INFO}"
	echo "More different see the ${OUTPUT_DIR}/diff.log file"
else
	echo "Succeed, Check not break"
fi
