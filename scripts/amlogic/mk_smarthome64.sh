# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2019 Amlogic, Inc. All rights reserved.
#

export CROSS_COMPILE=/opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

ROOTDIR=$(readlink -f $(dirname $0))/../../..
OUTDIR=${ROOTDIR}/out/kernel-5.15/
mkdir -p ${OUTDIR}/common
if [ "${SKIP_RM_OUTDIR}" != "1" ] ; then
	rm -rf ${OUTDIR}
fi

DEFCONFIG=meson64_a64_smarthome_defconfig
cp ${ROOTDIR}/common_drivers/arch/arm64/configs/${DEFCONFIG} ${ROOTDIR}/common/arch/arm64/configs/
set -x
make ARCH=arm64 -C ${ROOTDIR}/common O=${OUTDIR}/common ${DEFCONFIG}
make ARCH=arm64 -C ${ROOTDIR}/common O=${OUTDIR}/common headers_install
make ARCH=arm64 -C ${ROOTDIR}/common O=${OUTDIR}/common Image -j12
make ARCH=arm64 -C ${ROOTDIR}/common O=${OUTDIR}/common modules -j12
make ARCH=arm64 -C ${ROOTDIR}/common O=${OUTDIR}/common dtbs -j12
rm ${ROOTDIR}/common/arch/arm64/configs/${DEFCONFIG}
set +x

echo "Build success!"
