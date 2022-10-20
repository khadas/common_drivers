/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _USB_MAIN_H__
#define _USB_MAIN_H__

#if IS_ENABLED(CONFIG_AMLOGIC_USBPHY)
int __init amlogic_new_usb3_v2_driver_init(void);
#else
static inline  int __init amlogic_new_usb3_v2_driver_init(void)
{
	return -1;
}

#endif

#if IS_ENABLED(CONFIG_AMLOGIC_USB2PHY)
int __init amlogic_new_usb2_v2_driver_init(void);
#else
static inline  int __init amlogic_new_usb2_v2_driver_init(void)
{
	return -1;
}

#endif

#if IS_ENABLED(CONFIG_AMLOGIC_USB3PHY)
int __init amlogic_new_otg_driver_init(void);
/*int __init amlogic_new_usb3_v3_driver_init(void); */
#else
static inline  int __init amlogic_new_otg_driver_init(void)
{
	return -1;
}

/*static inline  int __init amlogic_new_usb3_v3_driver_init(void) {return 0;}*/
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_USBPHYC2)
int __init amlogic_new_c2_usb2_v2_driver_init(void);
int __init amlogic_new_c2_usb3_v2_driver_init(void);
#else
static inline  int __init amlogic_new_c2_usb2_v2_driver_init(void)
{
	return -1;
}

static inline  int __init amlogic_new_c2_usb3_v2_driver_init(void)
{
	return -1;
}

#endif

#if IS_ENABLED(CONFIG_AMLOGIC_USB_DWC_OTG_HCD)
int __init dwc_otg_init(void);
#else
static inline  int __init dwc_otg_init(void)
{
	return -1;
}

#endif

#if IS_ENABLED(CONFIG_AMLOGIC_CRG)
int __init amlogic_crg_drd_usb2_drv_init(void);
int __init amlogic_crg_init(void);
int __init amlogic_crg_drd_usb3_drv_init(void);
int __init amlogic_crg_host_driver_init(void);
int __init amlogic_usb3_m31_drv_init(void);
int __init amlogic_usb2_m31_drv_init(void);
int __init crg_otg_init(void);
int __init crg_otg_v2_init(void);
#else
static inline  int __init amlogic_crg_drd_usb2_drv_init(void)
{
	return -1;
}

static inline  int __init amlogic_crg_init(void)
{
	return -1;
}

static inline  int __init amlogic_crg_drd_usb3_drv_init(void)
{
	return -1;
}

static inline  int __init amlogic_crg_host_driver_init(void)
{
	return -1;
}

static inline int __init crg_otg_init(void)
{
	return -1;
}

static inline  int __init crg_otg_v2_init(void)
{	return -1;
}

static int __init amlogic_usb3_m31_drv_init(void)
{
	return -1;
}

static int __init amlogic_usb2_m31_drv_init(void)
{
	return -1;
}
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_CC)
int __init amlogic_cc_driver_init(void);
#else
static inline  int __init amlogic_cc_driver_init(void)
{
	return -1;
}

#endif

#if IS_ENABLED(CONFIG_AMLOGIC_BC)
int __init amlogic_bc_driver_init(void);
#else
static inline  int __init amlogic_bc_driver_init(void)
{
	return -1;
}
#endif

int get_otg_mode(void);

#endif
