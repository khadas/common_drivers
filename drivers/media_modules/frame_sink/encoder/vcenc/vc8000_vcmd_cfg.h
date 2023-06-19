/* SPDX-License-Identifier: GPL-2.0
 ****************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    Copyright 2020 Verisilicon(Beijing) Co.,Ltd. All Rights Reserved.
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a
 *    copy of this software and associated documentation files (the "Software"),
 *    to deal in the Software without restriction, including without limitation
 *    the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *    and/or sell copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    Copyright 2020 Verisilicon(Beijing) Co.,Ltd. All Rights Reserved.
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************
 *
 *    Note: This software is released under dual MIT and GPL licenses. A
 *    recipient may use this file under the terms of either the MIT license or
 *    GPL License. If you wish to use only one license not the other, you can
 *    indicate your decision by deleting one of the above license notices in your
 *    version of this file.
 *
 *****************************************************************************
 */

#ifndef __VC8000_VCMD_CFG_H__
#define __VC8000_VCMD_CFG_H__

/* Configure information with CMD, fill according to System Memory Map*/

/* Defines for Sub-System 0 */
#define VCMD_ENC_IO_ADDR_0 (0x0)
#define VCMD_ENC_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4)
#define VCMD_ENC_INT_PIN_0 (-1)
#define VCMD_ENC_MODULE_TYPE_0 (0)
#define VCMD_ENC_MODULE_MAIN_ADDR_0 (0x1000)
#define VCMD_ENC_MODULE_DEC400_ADDR_0 (0XFFFF) /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_0 (0XFFFF)
#define VCMD_ENC_MODULE_MMU0_ADDR_0 (0XFFFF)
#define VCMD_ENC_MODULE_MMU1_ADDR_0 (0XFFFF)
#define VCMD_ENC_MODULE_AXIFE0_ADDR_0 (0XFFFF)
#define VCMD_ENC_MODULE_AXIFE1_ADDR_0 (0XFFFF)

/* Defines for Sub-System 1 */
#define VCMD_IM_IO_ADDR_1 (0x94000)
#define VCMD_IM_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4)
#define VCMD_IM_INT_PIN_1 (-1)
#define VCMD_IM_MODULE_TYPE_1 (1)
#define VCMD_IM_MODULE_MAIN_ADDR_1 (0x1000)
#define VCMD_IM_MODULE_DEC400_ADDR_1 (0XFFFF)
#define VCMD_IM_MODULE_L2CACHE_ADDR_1 (0XFFFF)
#define VCMD_IM_MODULE_MMU0_ADDR_1 (0XFFFF)
#define VCMD_IM_MODULE_MMU1_ADDR_1 (0XFFFF)
#define VCMD_IM_MODULE_AXIFE0_ADDR_1 (0XFFFF)
#define VCMD_IM_MODULE_AXIFE1_ADDR_1 (0XFFFF)

/*for all vcmds, the core info should be listed here for subsequent use*/
struct vcmd_config {
    unsigned long vcmd_base_addr;
    u32 vcmd_iosize;
    int vcmd_irq;
    u32 sub_module_type;       /*input vc8000e=0,IM=1,vc8000d=2，jpege=3, jpegd=4*/
    u16 submodule_main_addr;   // in byte
    u16 submodule_dec400_addr; //if submodule addr == 0xffff, this submodule does not exist.// in byte
    u16 submodule_L2Cache_addr;  // in byte
    u16 submodule_MMU_addr[2];   // in byte
    u16 submodule_axife_addr[2]; // in byte
};

static struct vcmd_config vcmd_core_array[] = {
    /* Sub-System 0 */
    {VCMD_ENC_IO_ADDR_0,
     VCMD_ENC_IO_SIZE_0,
     VCMD_ENC_INT_PIN_0,
     VCMD_ENC_MODULE_TYPE_0,
     VCMD_ENC_MODULE_MAIN_ADDR_0,
     VCMD_ENC_MODULE_DEC400_ADDR_0,
     VCMD_ENC_MODULE_L2CACHE_ADDR_0,
     {VCMD_ENC_MODULE_MMU0_ADDR_0, VCMD_ENC_MODULE_MMU1_ADDR_0},
     {VCMD_ENC_MODULE_AXIFE0_ADDR_0, VCMD_ENC_MODULE_AXIFE1_ADDR_0}},
#if 0
    /* Sub-System 1 */
	{ VCMD_IM_IO_ADDR_1,
	  VCMD_IM_IO_SIZE_1,
	  VCMD_IM_INT_PIN_1,
	  VCMD_IM_MODULE_TYPE_1,
	  VCMD_IM_MODULE_MAIN_ADDR_1,
	  VCMD_IM_MODULE_DEC400_ADDR_1,
	  VCMD_IM_MODULE_L2CACHE_ADDR_1,
	  { VCMD_IM_MODULE_MMU0_ADDR_1,
	  VCMD_IM_MODULE_MMU1_ADDR_1 },
	  { VCMD_IM_MODULE_AXIFE0_ADDR_1,
	  VCMD_IM_MODULE_AXIFE1_ADDR_1 }
	},
#endif
};

#endif /*__VC8000_VCMD_CFG_H__ */

// BELOW PORT FROM vc8000_vcmd_driver.c
// KEEP AS REFERENCE
#if 0
/*------------------------------------------------------------------------
 *********************VCMD CONFIGURATION BY CUSTOMER***********************
 *-------------------------------------------------------------------------
 */
//video encoder vcmd configuration
#ifdef EMU
#define VCMD_ENC_IO_ADDR_0 0x6020000 /*customer specify according to own platform*/
#else
#define VCMD_ENC_IO_ADDR_0 0x90000 /*customer specify according to own platform*/
#endif
#define VCMD_ENC_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_0 -1
#define VCMD_ENC_MODULE_TYPE_0 0
#define VCMD_ENC_MODULE_MAIN_ADDR_0 0x1000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_0 0XFFFF //0X6000    /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_ENC_MODULE_MMU0_ADDR_0 0XFFFF   //0X2000
#define VCMD_ENC_MODULE_MMU1_ADDR_0 0XFFFF   //0X4000
#define VCMD_ENC_MODULE_AXIFE0_ADDR_0 0XFFFF //0X3000
#define VCMD_ENC_MODULE_AXIFE1_ADDR_0 0XFFFF //0X5000
#ifdef EMU
#define VCMD_ENC_IO_ADDR_1 0x6000000 /*customer specify according to own platform*/
#else
#define VCMD_ENC_IO_ADDR_1 0x91000 /*customer specify according to own platform*/
#endif
#define VCMD_ENC_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_1 -1
#define VCMD_ENC_MODULE_TYPE_1 0
#define VCMD_ENC_MODULE_MAIN_ADDR_1 0x0000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_ENC_MODULE_MMU_ADDR_1 0XFFFF

#define VCMD_ENC_IO_ADDR_2 0x92000 /*customer specify according to own platform*/
#define VCMD_ENC_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_2 -1
#define VCMD_ENC_MODULE_TYPE_2 0
#define VCMD_ENC_MODULE_MAIN_ADDR_2 0x0000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_ENC_MODULE_MMU_ADDR_2 0XFFFF

#define VCMD_ENC_IO_ADDR_3 0x93000 /*customer specify according to own platform*/
#define VCMD_ENC_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_3 -1
#define VCMD_ENC_MODULE_TYPE_3 0
#define VCMD_ENC_MODULE_MAIN_ADDR_3 0x0000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_ENC_MODULE_MMU_ADDR_3 0XFFFF

//video encoder cutree/IM  vcmd configuration

#define VCMD_IM_IO_ADDR_0 0x94000 //0xA0000    /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_0 -1
#define VCMD_IM_MODULE_TYPE_0 1
#define VCMD_IM_MODULE_MAIN_ADDR_0 0x1000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_0 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_IM_MODULE_MMU0_ADDR_0 0XFFFF //0X2000
#define VCMD_IM_MODULE_MMU1_ADDR_0 0XFFFF
#define VCMD_IM_MODULE_AXIFE0_ADDR_0 0XFFFF //0X3000
#define VCMD_IM_MODULE_AXIFE1_ADDR_0 0XFFFF // 0XFFFF

#define VCMD_IM_IO_ADDR_1 0xa1000 /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_1 -1
#define VCMD_IM_MODULE_TYPE_1 1
#define VCMD_IM_MODULE_MAIN_ADDR_1 0x0000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_IM_MODULE_MMU_ADDR_1 0XFFFF

#define VCMD_IM_IO_ADDR_2 0xa2000 /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_2 -1
#define VCMD_IM_MODULE_TYPE_2 1
#define VCMD_IM_MODULE_MAIN_ADDR_2 0x0000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_IM_MODULE_MMU_ADDR_2 0XFFFF

#define VCMD_IM_IO_ADDR_3 0xa3000 /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_3 -1
#define VCMD_IM_MODULE_TYPE_3 1
#define VCMD_IM_MODULE_MAIN_ADDR_3 0x0000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_IM_MODULE_MMU_ADDR_3 0XFFFF

//video decoder vcmd configuration

#define VCMD_DEC_IO_ADDR_0 0xb0000 /*customer specify according to own platform*/
#define VCMD_DEC_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_DEC_INT_PIN_0 -1
#define VCMD_DEC_MODULE_TYPE_0 2
#define VCMD_DEC_MODULE_MAIN_ADDR_0 0x0000   /*customer specify according to own platform*/
#define VCMD_DEC_MODULE_DEC400_ADDR_0 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_DEC_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_DEC_MODULE_MMU_ADDR_0 0XFFFF

#define VCMD_DEC_IO_ADDR_1 0xb1000 /*customer specify according to own platform*/
#define VCMD_DEC_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_DEC_INT_PIN_1 -1
#define VCMD_DEC_MODULE_TYPE_1 2
#define VCMD_DEC_MODULE_MAIN_ADDR_1 0x0000   /*customer specify according to own platform*/
#define VCMD_DEC_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_DEC_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_DEC_MODULE_MMU_ADDR_1 0XFFFF

#define VCMD_DEC_IO_ADDR_2 0xb2000 /*customer specify according to own platform*/
#define VCMD_DEC_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_DEC_INT_PIN_2 -1
#define VCMD_DEC_MODULE_TYPE_2 2
#define VCMD_DEC_MODULE_MAIN_ADDR_2 0x0000   /*customer specify according to own platform*/
#define VCMD_DEC_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_DEC_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_DEC_MODULE_MMU_ADDR_2 0XFFFF

#define VCMD_DEC_IO_ADDR_3 0xb3000 /*customer specify according to own platform*/
#define VCMD_DEC_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_DEC_INT_PIN_3 -1
#define VCMD_DEC_MODULE_TYPE_3 2
#define VCMD_DEC_MODULE_MAIN_ADDR_3 0x0000   /*customer specify according to own platform*/
#define VCMD_DEC_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_DEC_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_DEC_MODULE_MMU_ADDR_3 0XFFFF

//JPEG encoder vcmd configuration

#define VCMD_JPEGE_IO_ADDR_0 0x90000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_0 -1
#define VCMD_JPEGE_MODULE_TYPE_0 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_0 0x1000 /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_0                                                            \
    0XFFFF //0X4000    /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_JPEGE_MODULE_MMU0_ADDR_0 0XFFFF //0X2000
#define VCMD_JPEGE_MODULE_MMU1_ADDR_0 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE0_ADDR_0 0XFFFF //0X3000
#define VCMD_JPEGE_MODULE_AXIFE1_ADDR_0 0XFFFF

#define VCMD_JPEGE_IO_ADDR_1 0xC1000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_1 -1
#define VCMD_JPEGE_MODULE_TYPE_1 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_1 0x0000   /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_JPEGE_MODULE_MMU_ADDR_1 0XFFFF

#define VCMD_JPEGE_IO_ADDR_2 0xC2000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_2 -1
#define VCMD_JPEGE_MODULE_TYPE_2 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_2 0x0000   /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_JPEGE_MODULE_MMU_ADDR_2 0XFFFF

#define VCMD_JPEGE_IO_ADDR_3 0xC3000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_3 -1
#define VCMD_JPEGE_MODULE_TYPE_3 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_3 0x0000   /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_JPEGE_MODULE_MMU_ADDR_3 0XFFFF


//JPEG decoder vcmd configuration

#define VCMD_JPEGD_IO_ADDR_0 0xD0000 /*customer specify according to own platform*/
#define VCMD_JPEGD_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGD_INT_PIN_0 -1
#define VCMD_JPEGD_MODULE_TYPE_0 4
#define VCMD_JPEGD_MODULE_MAIN_ADDR_0 0x0000   /*customer specify according to own platform*/
#define VCMD_JPEGD_MODULE_DEC400_ADDR_0 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGD_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_JPEGD_MODULE_MMU_ADDR_0 0XFFFF

#define VCMD_JPEGD_IO_ADDR_1 0xD1000 /*customer specify according to own platform*/
#define VCMD_JPEGD_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGD_INT_PIN_1 -1
#define VCMD_JPEGD_MODULE_TYPE_1 4
#define VCMD_JPEGD_MODULE_MAIN_ADDR_1 0x0000   /*customer specify according to own platform*/
#define VCMD_JPEGD_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGD_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_JPEGD_MODULE_MMU_ADDR_1 0XFFFF

#define VCMD_JPEGD_IO_ADDR_2 0xD2000 /*customer specify according to own platform*/
#define VCMD_JPEGD_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGD_INT_PIN_2 -1
#define VCMD_JPEGD_MODULE_TYPE_2 4
#define VCMD_JPEGD_MODULE_MAIN_ADDR_2 0x0000   /*customer specify according to own platform*/
#define VCMD_JPEGD_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGD_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_JPEGD_MODULE_MMU_ADDR_2 0XFFFF

#define VCMD_JPEGD_IO_ADDR_3 0xD3000 /*customer specify according to own platform*/
#define VCMD_JPEGD_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGD_INT_PIN_3 -1
#define VCMD_JPEGD_MODULE_TYPE_3 4
#define VCMD_JPEGD_MODULE_MAIN_ADDR_3 0x0000   /*customer specify according to own platform*/
#define VCMD_JPEGD_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGD_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_JPEGD_MODULE_MMU_ADDR_3 0XFFFF

struct vcmd_config {
    unsigned long vcmd_base_addr;
    u32 vcmd_iosize;
    int vcmd_irq;
    u32 sub_module_type;  /*input vc8000e=0,IM=1,vc8000d=2,jpege=3, jpegd=4*/
    u16 submodule_main_addr; // in byte
    u16 submodule_dec400_addr;//if submodule addr == 0xffff, this submodule does not exist.// in byte
    u16 submodule_L2Cache_addr; // in byte
    u16 submodule_MMU_addr[2]; // in byte
    u16 submodule_axife_addr[2]; // in byte
};

#define NETINT
//#define MAGVII
//#define OYB_VCEJ
//#define OYB_VCE

/*for all vcmds, the core info should be listed here for subsequent use*/
static struct vcmd_config vcmd_core_array[] = {
#if defined(NETINT) || defined(OYB_VCE)
    //encoder configuration
    {VCMD_ENC_IO_ADDR_0,
    VCMD_ENC_IO_SIZE_0,
    VCMD_ENC_INT_PIN_0,
    VCMD_ENC_MODULE_TYPE_0,
    VCMD_ENC_MODULE_MAIN_ADDR_0,
    VCMD_ENC_MODULE_DEC400_ADDR_0,
    VCMD_ENC_MODULE_L2CACHE_ADDR_0,
    {VCMD_ENC_MODULE_MMU0_ADDR_0,
    VCMD_ENC_MODULE_MMU1_ADDR_0},
    {VCMD_ENC_MODULE_AXIFE0_ADDR_0,
    VCMD_ENC_MODULE_AXIFE1_ADDR_0} },
#endif
#if 0
    {VCMD_ENC_IO_ADDR_1,
    VCMD_ENC_IO_SIZE_1,
    VCMD_ENC_INT_PIN_1,
    VCMD_ENC_MODULE_TYPE_1,
    VCMD_ENC_MODULE_MAIN_ADDR_1,
    VCMD_ENC_MODULE_DEC400_ADDR_1,
    VCMD_ENC_MODULE_L2CACHE_ADDR_1,
    VCMD_ENC_MODULE_MMU_ADDR_1},

    {VCMD_ENC_IO_ADDR_2,
    VCMD_ENC_IO_SIZE_2,
    VCMD_ENC_INT_PIN_2,
    VCMD_ENC_MODULE_TYPE_2,
    VCMD_ENC_MODULE_MAIN_ADDR_2,
    VCMD_ENC_MODULE_DEC400_ADDR_2,
    VCMD_ENC_MODULE_L2CACHE_ADDR_2,
    VCMD_ENC_MODULE_MMU_ADDR_2},

    {VCMD_ENC_IO_ADDR_3,
    VCMD_ENC_IO_SIZE_3,
    VCMD_ENC_INT_PIN_3,
    VCMD_ENC_MODULE_TYPE_3,
    VCMD_ENC_MODULE_MAIN_ADDR_3,
    VCMD_ENC_MODULE_DEC400_ADDR_3,
    VCMD_ENC_MODULE_L2CACHE_ADDR_3,
    VCMD_ENC_MODULE_MMU_ADDR_3},
#endif
    //cutree/IM configuration
#if defined(NETINT) || defined(OYB_VCE)
    {VCMD_IM_IO_ADDR_0,
    VCMD_IM_IO_SIZE_0,
    VCMD_IM_INT_PIN_0,
    VCMD_IM_MODULE_TYPE_0,
    VCMD_IM_MODULE_MAIN_ADDR_0,
    VCMD_IM_MODULE_DEC400_ADDR_0,
    VCMD_IM_MODULE_L2CACHE_ADDR_0,
    {VCMD_IM_MODULE_MMU0_ADDR_0,
    VCMD_IM_MODULE_MMU1_ADDR_0},
    {VCMD_IM_MODULE_AXIFE0_ADDR_0,
    VCMD_IM_MODULE_AXIFE1_ADDR_0} },
#endif
#if 0
    {VCMD_IM_IO_ADDR_1,
    VCMD_IM_IO_SIZE_1,
    VCMD_IM_INT_PIN_1,
    VCMD_IM_MODULE_TYPE_1,
    VCMD_IM_MODULE_MAIN_ADDR_1,
    VCMD_IM_MODULE_DEC400_ADDR_1,
    VCMD_IM_MODULE_L2CACHE_ADDR_1,
    VCMD_IM_MODULE_MMU_ADDR_1},

    {VCMD_IM_IO_ADDR_2,
    VCMD_IM_IO_SIZE_2,
    VCMD_IM_INT_PIN_2,
    VCMD_IM_MODULE_TYPE_2,
    VCMD_IM_MODULE_MAIN_ADDR_2,
    VCMD_IM_MODULE_DEC400_ADDR_2,
    VCMD_IM_MODULE_L2CACHE_ADDR_2,
    VCMD_IM_MODULE_MMU_ADDR_2},

    {VCMD_IM_IO_ADDR_3,
    VCMD_IM_IO_SIZE_3,
    VCMD_IM_INT_PIN_3,
    VCMD_IM_MODULE_TYPE_3,
    VCMD_IM_MODULE_MAIN_ADDR_3,
    VCMD_IM_MODULE_DEC400_ADDR_3,
    VCMD_IM_MODULE_L2CACHE_ADDR_3,
    VCMD_IM_MODULE_MMU_ADDR_3},

    //decoder configuration
    {VCMD_DEC_IO_ADDR_0,
    VCMD_DEC_IO_SIZE_0,
    VCMD_DEC_INT_PIN_0,
    VCMD_DEC_MODULE_TYPE_0,
    VCMD_DEC_MODULE_MAIN_ADDR_0,
    VCMD_DEC_MODULE_DEC400_ADDR_0,
    VCMD_DEC_MODULE_L2CACHE_ADDR_0,
    VCMD_DEC_MODULE_MMU_ADDR_0},

    {VCMD_DEC_IO_ADDR_1,
    VCMD_DEC_IO_SIZE_1,
    VCMD_DEC_INT_PIN_1,
    VCMD_DEC_MODULE_TYPE_1,
    VCMD_DEC_MODULE_MAIN_ADDR_1,
    VCMD_DEC_MODULE_DEC400_ADDR_1,
    VCMD_DEC_MODULE_L2CACHE_ADDR_1,
    VCMD_DEC_MODULE_MMU_ADDR_1},

    {VCMD_DEC_IO_ADDR_2,
    VCMD_DEC_IO_SIZE_2,
    VCMD_DEC_INT_PIN_2,
    VCMD_DEC_MODULE_TYPE_2,
    VCMD_DEC_MODULE_MAIN_ADDR_2,
    VCMD_DEC_MODULE_DEC400_ADDR_2,
    VCMD_DEC_MODULE_L2CACHE_ADDR_2,
    VCMD_DEC_MODULE_MMU_ADDR_2},

    {VCMD_DEC_IO_ADDR_3,
    VCMD_DEC_IO_SIZE_3,
    VCMD_DEC_INT_PIN_3,
    VCMD_DEC_MODULE_TYPE_3,
    VCMD_DEC_MODULE_MAIN_ADDR_3,
    VCMD_DEC_MODULE_DEC400_ADDR_3,
    VCMD_DEC_MODULE_L2CACHE_ADDR_3,
    VCMD_DEC_MODULE_MMU_ADDR_3},
#endif
#if defined(MAGVII) || defined(OYB_VCEJ)
    //JPEG encoder configuration
    {VCMD_JPEGE_IO_ADDR_0,
    VCMD_JPEGE_IO_SIZE_0,
    VCMD_JPEGE_INT_PIN_0,
    VCMD_JPEGE_MODULE_TYPE_0,
    VCMD_JPEGE_MODULE_MAIN_ADDR_0,
    VCMD_JPEGE_MODULE_DEC400_ADDR_0,
    VCMD_JPEGE_MODULE_L2CACHE_ADDR_0,
    {VCMD_JPEGE_MODULE_MMU0_ADDR_0,
    VCMD_JPEGE_MODULE_MMU1_ADDR_0},
    {VCMD_JPEGE_MODULE_AXIFE0_ADDR_0,
    VCMD_JPEGE_MODULE_AXIFE1_ADDR_0} },
#endif
#if 0
    {VCMD_JPEGE_IO_ADDR_1,
    VCMD_JPEGE_IO_SIZE_1,
    VCMD_JPEGE_INT_PIN_1,
    VCMD_JPEGE_MODULE_TYPE_1,
    VCMD_JPEGE_MODULE_MAIN_ADDR_1,
    VCMD_JPEGE_MODULE_DEC400_ADDR_1,
    VCMD_JPEGE_MODULE_L2CACHE_ADDR_1,
    VCMD_JPEGE_MODULE_MMU_ADDR_1},

    {VCMD_JPEGE_IO_ADDR_2,
    VCMD_JPEGE_IO_SIZE_2,
    VCMD_JPEGE_INT_PIN_2,
    VCMD_JPEGE_MODULE_TYPE_2,
    VCMD_JPEGE_MODULE_MAIN_ADDR_2,
    VCMD_JPEGE_MODULE_DEC400_ADDR_2,
    VCMD_JPEGE_MODULE_L2CACHE_ADDR_2,
    VCMD_JPEGE_MODULE_MMU_ADDR_2},

    {VCMD_JPEGE_IO_ADDR_3,
    VCMD_JPEGE_IO_SIZE_3,
    VCMD_JPEGE_INT_PIN_3,
    VCMD_JPEGE_MODULE_TYPE_3,
    VCMD_JPEGE_MODULE_MAIN_ADDR_3,
    VCMD_JPEGE_MODULE_DEC400_ADDR_3,
    VCMD_JPEGE_MODULE_L2CACHE_ADDR_3,
    VCMD_JPEGE_MODULE_MMU_ADDR_3},
    //JPEG decoder configuration
    {VCMD_JPEGD_IO_ADDR_0,
    VCMD_JPEGD_IO_SIZE_0,
    VCMD_JPEGD_INT_PIN_0,
    VCMD_JPEGD_MODULE_TYPE_0,
    VCMD_JPEGD_MODULE_MAIN_ADDR_0,
    VCMD_JPEGD_MODULE_DEC400_ADDR_0,
    VCMD_JPEGD_MODULE_L2CACHE_ADDR_0,
    VCMD_JPEGD_MODULE_MMU_ADDR_0},

    {VCMD_JPEGD_IO_ADDR_1,
    VCMD_JPEGD_IO_SIZE_1,
    VCMD_JPEGD_INT_PIN_1,
    VCMD_JPEGD_MODULE_TYPE_1,
    VCMD_JPEGD_MODULE_MAIN_ADDR_1,
    VCMD_JPEGD_MODULE_DEC400_ADDR_1,
    VCMD_JPEGD_MODULE_L2CACHE_ADDR_1,
    VCMD_JPEGD_MODULE_MMU_ADDR_1},

    {VCMD_JPEGD_IO_ADDR_2,
    VCMD_JPEGD_IO_SIZE_2,
    VCMD_JPEGD_INT_PIN_2,
    VCMD_JPEGD_MODULE_TYPE_2,
    VCMD_JPEGD_MODULE_MAIN_ADDR_2,
    VCMD_JPEGD_MODULE_DEC400_ADDR_2,
    VCMD_JPEGD_MODULE_L2CACHE_ADDR_2,
    VCMD_JPEGD_MODULE_MMU_ADDR_2},

    {VCMD_JPEGD_IO_ADDR_3,
    VCMD_JPEGD_IO_SIZE_3,
    VCMD_JPEGD_INT_PIN_3,
    VCMD_JPEGD_MODULE_TYPE_3,
    VCMD_JPEGD_MODULE_MAIN_ADDR_3,
    VCMD_JPEGD_MODULE_DEC400_ADDR_3,
    VCMD_JPEGD_MODULE_L2CACHE_ADDR_3,
    VCMD_JPEGD_MODULE_MMU_ADDR_3},
#endif
};

#endif //END KEEP AS REFERENCE
