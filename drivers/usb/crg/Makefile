# SPDX-License-Identifier: GPL-2.0
ccflags-y += -I$(srctree)/drivers/usb/host/

CRG_MODULE_NAME = amlogic_usb_crg
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(CRG_MODULE_NAME).o
$(CRG_MODULE_NAME)-y += crg.o

PR_FMT_CRG = $(subst amlogic-,,$(CRG_MODULE_NAME))
PR_FMT_DEFINE_CRG="-Dpr_fmt(fmt)= \"[$(PR_FMT_CRG)]: \" fmt"
CFLAGS_crg.o += $(PR_FMT_DEFINE_CRG)

CRG_DRD_MODULE_NAME = amlogic_usb_crg_drd
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(CRG_DRD_MODULE_NAME).o
$(CRG_DRD_MODULE_NAME)-y += crg_drd.o

PR_FMT_CRG_DRD = $(subst amlogic-,,$(CRG_DRD_MODULE_NAME))
PR_FMT_DEFINE_CRG_DRD="-Dpr_fmt(fmt)= \"[$(PR_FMT_CRG_DRD)]: \" fmt"
CFLAGS_crg_drd.o += $(PR_FMT_DEFINE_CRG_DRD)

CRG_UDC_MODULE_NAME = amlogic_usb_crg_udc
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(CRG_UDC_MODULE_NAME).o
$(CRG_UDC_MODULE_NAME)-y += crg_udc.o
$(CRG_UDC_MODULE_NAME)-y += crg_usbphy.o

PR_FMT_CRG_UDC = $(subst amlogic-,,$(CRG_UDC_MODULE_NAME))
PR_FMT_DEFINE_CRG_UDC="-Dpr_fmt(fmt)= \"[$(PR_FMT_CRG_UDC)]: \" fmt"
CFLAGS_crg_udc.o += $(PR_FMT_DEFINE_CRG_UDC)
CFLAGS_crg_usbphy.o += $(PR_FMT_DEFINE_CRG_UDC)
