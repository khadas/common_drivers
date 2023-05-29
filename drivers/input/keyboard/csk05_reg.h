/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**************************************************************************
 *  AW9163_para.h
 *
 *  AW9163 Driver code version 1.0
 *
 *  Create Date : 2016/09/12
 *
 *  Modify Date :
 *
 *  Create by   : liweilei
 *
 **************************************************************************/
#ifndef __CSK05_REG_H__
#define __CSK05_REG_H__

/*
 * The time of anti-shake,default 3
 */
#define     DEBOUNCE         0x13
/*
 * enable 0~7 channel of scan
 */
#define     CHANNELEN0       0x14
/*
 * enable 8~15 channel of scan
 */
#define     CHANNELEN1       0x15
/*
 * enable 16~23 channel of scan
 */
#define     CHANNELEN2       0x16
/*
 * sets the global scan frequency. F=6.4MHz/(GFREQ + 1)
 */
#define     GFREQ            0x18
/*
 * sets the global scan accuracy. Win=256*(GWIN + 1)
 */
#define     GWIN             0x19
/*
 * sets the global detection current. I=1uA*(GIDAC)
 */
#define     GIDAC            0x1a
/*
 * sets the global noise threshold.
 */
#define     GNOISETH         0x1b
/*
 * Trigger threshold set 8 bits lower,default 200.
 */
#define     GFINGERTH_L      0x1c
/*
 * Trigger threshold set 8 bits higher,default 0.
 */
#define     GFINGERTH_H      0x1d
/*
 * sets global sensitivity.range:0~255,the bigger the value,the higher sensitivity.
 */
#define     GSENSITIVITY     0x1e
/*
 * enable various scan mode. [7] sleep mode,
 * [6] monitor mode ,[0] scan mode
 */
#define     SCANCR           0x1f

/*
 * Gestures code 00:invalid,01:upglide,02:downglide,03:leftglide,
 * 04:rightglide,05:click,0B:double click,0C:long press
 */
#define     MOTIONSR         0x20
/*
 * current position of slider(01~200) 00:invalid
 */
#define     SLIDESR          0x21
/*
 * status of 0~7 scan channel. 1:trigger,0:not trigger
 */
#define     ButtonSR0        0x22
/*
 * status of 8~15 scan channel. 1:trigger,0:not trigger
 */
#define     ButtonSR1        0x23
/*
 * status of 16~23 scan channel. 1:trigger,0:not trigger
 */
#define     ButtonSR2        0x24
/*
 * status register check
 */
#define     CHECKSUM         0x27

/*
 * Irq pulse width setting.width = Tick*IRQWIDTH
 */
#define     IRQWIDTH         0x60
/*
 * interrupt triggering mode:trigger or glide,single or periodic interruption.
 */
#define     IRQCR            0x6
/*
 * [1] EnPull control IIC and IRQ drive modes 0:0Ddrive 1:on the resistance to pull,about 8K
 * [0] DP_V18 interface voltage selection    0:VDD 1:1.8v
 */
#define     IOMODE           0x62
/*
 * The lower 8 bits of the active wake san cycle in monitor mode,T = MONITORF*MonitorT
 */
#define     MONITORT_L       0x70
/*
 * The higher 8 bits of the active wake san cycle in monitor mode T = MONITORF*MonitorT
 */
#define     MONITORT_H       0x71
/*
 * The scan cycle in monitor mode,Unit of ms
 */
#define     MONITORF         0x72
/*
 * When the monitor mode is enabled,
 * the X and MS will automatically enter monitor mode from dynamic mode whitout touch.
 */
#define     ATMCR            0x73
/*
 * The scan cycle int dynamic mode,Unit of ms.
 */
#define     ACTIVEF          0x74

/*
 * use 0~7 channel in monitor mode,1:use 0:nonuse
 */
#define     GSENMAKS0        0x78
/*
 * use 8~15 channel in monitor mode,1:use 0:nonuse
 */
#define     GSENMAKS1        0x79
/*
 * use 16~23 channel in monitor mode,1:use 0:nonuse
 */
#define     GSENMAKS2        0x7a
/*
 * set noise threshold in monitor mode.
 */
#define     GSNOISETH        0x7b
/*
 * set scan frequency in monitor mode.F=6.4MHz/(gFreq + 1)
 */
#define     GSFREQ           0x7c
/*
 * set scan accuracy in monitor mode.Win=256*(GWIN + 1)
 */
#define     GSWIN            0x7d
/*
 * set the detection current in monitor mode.I=1uA*(GIDAC)
 */
#define     GSIDAC           0x7e

#endif

