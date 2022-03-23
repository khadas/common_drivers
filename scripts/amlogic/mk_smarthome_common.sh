#!/bin/bash
# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

function show_help {
	echo "USAGE: $0 [--nongki] [--abi]"
	echo "  --kernel_dir            for KERNEL_DIR, common[default]|other dir, require parameter value"
	echo "  --common_drivers_dir    for COMMON_DRIVERS_DIR, common[default]|other dir, require parameter value"
	echo "  --savedefconfig         for SAVEDEFCONFIG, [default]|1, not require parameter value"
	echo "  --menuconfig            for MENUCONFIG, [default]|1, not require parameter value"
}

VA=
ARGS=()
for i in "$@"
do
	case $i in
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
	--savedefconfig)
		SAVEDEFCONFIG=1
		shift
		;;
	--menuconfig)
		MENUCONFIG=1
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

export KERNEL_DIR COMMON_DRIVERS_DIR
export CROSS_COMPILE=${CROSS_COMPILE_TOOL}

if [[ $ARCH == arm64 ]]; then
	OUTDIR=${ROOT_DIR}/out/kernel-5.15-64
else
	OUTDIR=${ROOT_DIR}/out/kernel-5.15-32
fi

mkdir -p ${OUTDIR}/common
if [ "${SKIP_RM_OUTDIR}" != "1" ] ; then
	rm -rf ${OUTDIR}
fi

source ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/amlogic_utils.sh

echo "========================================================"
echo ""
export DIST_DIR=$(readlink -m ${OUTDIR}/dist)
export MODULES_STAGING_DIR=$(readlink -m ${OUTDIR}/staging)
echo OUTDIR=$OUTDIR DIST_DIR=$DIST_DIR MODULES_STAGING_DIR=$MODULES_STAGING_DIR KERNEL_DIR=$KERNEL_DIR

mkdir -p ${DIST_DIR} ${MODULES_STAGING_DIR}

cp ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/arch/${ARCH}/configs/
if [[ -n ${SAVEDEFCONFIG} ]]; then
	set -x
	$MAKE ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	$MAKE ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common savedefconfig
	rm ${KERNEL_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
	cp -f ${OUTDIR}/common/defconfig  ${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/arch/${ARCH}/configs/${DEFCONFIG}
	set +x
	exit
fi
if [[ -n ${MENUCONFIG} ]]; then
	set -x
	$MAKE ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	$MAKE ARCH=${ARCH} -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common menuconfig
	set +x
	exit
fi
set -x
if [[ $ARCH == arm64 ]]; then
	$MAKE ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	$MAKE ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common headers_install &&
	$MAKE ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common Image -j12 &&
	$MAKE ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common modules -j12 &&
	$MAKE ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common INSTALL_MOD_PATH=${MODULES_STAGING_DIR} modules_install -j12 &&
	$MAKE ARCH=arm64 -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common dtbs -j12 || exit
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm64/configs/${DEFCONFIG}
else
	$MAKE ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common ${DEFCONFIG}
	$MAKE ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common headers_install &&
	$MAKE ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common uImage -j12 LOADADDR=0x208000 &&
	$MAKE ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common modules -j12 LOADADDR=0x208000 &&
	$MAKE ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common INSTALL_MOD_PATH=${MODULES_STAGING_DIR} modules_install -j12 &&
	$MAKE ARCH=arm -C ${ROOT_DIR}/${KERNEL_DIR} O=${OUTDIR}/common dtbs -j12 LOADADDR=0x208000 || exit
	rm ${ROOT_DIR}/${KERNEL_DIR}/arch/arm/configs/${DEFCONFIG}
fi
set +x

IN_KERNEL_MODULES=1
MODULES_LOAD_FIRSTLIST=(
	amlogic-clk.ko
	amlogic-clk-soc-c2.ko
	pwm-regulator.ko
	realtek.ko
	amlogic-pinctrl.ko
	amlogic-pinctrl-soc-c2.ko
	amlogic-pinctrl-soc-c3.ko
	amlogic-irqchip.ko
	amlogic-power.ko
	amlogic-gpiolib.ko
	amlogic-input-gpiokey.ko
	amlogic-mailbox.ko
	amlogic-input-adckey.ko
	amlogic-gkitool.ko
	amlogic-input-ir.ko
	amlogic-i2c.ko
	amlogic-spi.ko
	amlogic-pwm.ko
	amlogic-cpufreq.ko
	amlogic-secmon.ko
	amlogic-cpuinfo.ko
	amlogic-media.ko
	amlogic-adc.ko
	amlogic-rng.ko
	amlogic-reset.ko
	amlogic-rtc-virtual.ko
	amlogic-thermal.ko
	amlogic-efuse-unifykey.ko
	amlogic-mmc.ko
	amlogic_usb2_phy.ko
	amlogic_usb3_v2_phy.ko
	amlogic_usb2_c2_phy.ko
	amlogic_usb_cc.ko
	amlogic_usb3_c2_phy.ko
	amlogic_usb_otg.ko
	amlogic_usb_bc.ko
	amlogic_usb_crg.ko
	xhci-plat-hcd.ko
	dwc3.ko
	dwc3-of-simple.ko
	dwc_otg.ko
	amlogic-jtag.ko
	amlogic-reg.ko
	amlogic-ddr-tool.ko
	amlogic-irblaster.ko
	amlogic-reboot.ko
	amlogic-hifidsp.ko
	amlogic-crypto-dma.ko
	amlogic-snd-soc.ko
	amlogic-snd-codec-t9015.ko
	amlogic-snd-codec-a1.ko
	amlogic-snd-codec-tlv320adc3101.ko
	amlogic-snd-codec-ad82584f.ko
	amlogic_audiodsp.ko
	amlogic_audioattrs.ko
	amlogic_audiodata.ko
	amlogic-snd-codec-dummy.ko
)

INITRAMFS_STAGING_DIR=${MODULES_STAGING_DIR}/initramfs_staging
rm -rf ${INITRAMFS_STAGING_DIR}

source ${ROOT_DIR}/build/kernel/build_utils.sh

MODULES=$(find ${MODULES_STAGING_DIR} -type f -name "*.ko")
if [ -n "${MODULES}" ]; then
  if [ -n "${IN_KERNEL_MODULES}" -o -n "${EXT_MODULES}" -o -n "${EXT_MODULES_MAKEFILE}" ]; then
    echo "========================================================"
    echo " Copying modules files"
    cp -p ${MODULES} ${DIST_DIR}
    if [ "${COMPRESS_MODULES}" = "1" ]; then
      echo " Archiving modules to ${MODULES_ARCHIVE}"
      tar --transform="s,.*/,," -czf ${DIST_DIR}/${MODULES_ARCHIVE} ${MODULES[@]}
    fi
  fi

  echo "========================================================"
  echo "prepare modules"

  modules_install

  if [ -f ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot ]; then
    echo "Rebuild rootfs in order to install modules!"
    rebuild_rootfs ${ARCH}
    echo "Build success!"
  else
    echo "There's no file ${ROOT_DIR}/${KERNEL_DIR}/${COMMON_DRIVERS_DIR}/rootfs_base.cpio.gz.uboot, so don't rebuild rootfs!"
  fi
fi
