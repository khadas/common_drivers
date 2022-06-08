# SPDX-License-Identifier: GPL-2.0

USB2PHY_MODULE_NAME = amlogic_usb2_crg_drd_phy
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(USB2PHY_MODULE_NAME).o
$(USB2PHY_MODULE_NAME)-y += phy-aml-crg-drd-usb2.o
$(USB2PHY_MODULE_NAME)-y += phy-aml-crg-drd-usb.o

PR_FMT_USB2PHY = $(subst amlogic-,,$(USB2PHY_MODULE_NAME))
PR_FMT_DEFINE_USB2PHY="-Dpr_fmt(fmt)= \"[$(PR_FMT_USB2PHY)]: \" fmt"
CFLAGS_phy-aml-crg-drd-usb2.o += $(PR_FMT_DEFINE_USB2PHY)
CFLAGS_phy-aml-crg-drd-usb.o += $(PR_FMT_DEFINE_USB2PHY)

USB3PHY_MODULE_NAME = amlogic_usb3_crg_drd_phy
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(USB3PHY_MODULE_NAME).o
$(USB3PHY_MODULE_NAME)-y += phy-aml-crg-drd-usb3.o

PR_FMT_USB3PHY = $(subst amlogic-,,$(USB3PHY_MODULE_NAME))
PR_FMT_DEFINE_USB3PHY="-Dpr_fmt(fmt)= \"[$(PR_FMT_USB3PHY)]: \" fmt"
CFLAGS_phy-aml-crg-drd-usb3.o += $(PR_FMT_DEFINE_USB3PHY)

USBOTG_MODULE_NAME = amlogic_usb_crg_drd_otg_v2
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(USBOTG_MODULE_NAME).o
$(USBOTG_MODULE_NAME)-y += phy-aml-crg-drd-otg-v2.o

PR_FMT_USBOTG = $(subst amlogic-,,$(USBOTG_MODULE_NAME))
PR_FMT_DEFINE_USBOTG="-Dpr_fmt(fmt)= \"[$(PR_FMT_USBOTG)]: \" fmt"
CFLAGS_phy-aml-crg-drd-otg-v2.o += $(PR_FMT_DEFINE_USBOTG)
