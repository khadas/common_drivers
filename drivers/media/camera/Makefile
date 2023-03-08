#
# Makefile for the amlogic platform camera interface device drivers.
#

# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_CAMERA_NAME = amlogic-camera

PR_FMT = $(subst amlogic-,,$(MODULE_CAMERA_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-${CONFIG_AMLOGIC_VIDEO_CAPTURE} = $(MODULE_CAMERA_NAME).o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_MEDIA_CAMERA_UTILS) += camera/common/flashlight.o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_MEDIA_CAMERA_UTILS) += camera/common/plat_ctrl.o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_MEDIA_CAMERA_UTILS) += camera/common/vm.o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_MEDIA_CAMERA_UTILS) += camera/common/cam_prober.o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_MEDIA_CAMERA_UTILS) += camera/common/config_parser.o
$(MODULE_CAMERA_NAME)-y = common/flashlight.o
$(MODULE_CAMERA_NAME)-y += common/plat_ctrl.o
$(MODULE_CAMERA_NAME)-y += common/vm.o
$(MODULE_CAMERA_NAME)-y += common/cam_prober.o
$(MODULE_CAMERA_NAME)-y += ov5640.o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_VIDEO_CAPTURE_GC2145) += camera/gc2145.o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_VIDEO_CAPTURE_GC2145_MIPI) += camera/gc2145_mipi.o
# $(MODULE_CAMERA_NAME)-$(CONFIG_AMLOGIC_VIDEO_CAPTURE_OV5640) += ov5640.o