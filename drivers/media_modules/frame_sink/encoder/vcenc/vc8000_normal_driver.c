// SPDX-License-Identifier: GPL-2.0
/****************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    Copyright (C) 2014  VeriSilicon Microelectronics Co., Ltd.
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
 *    Copyright (C) 2014  VeriSilicon Microelectronics Co., Ltd.
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

#include <linux/kernel.h>
#include <linux/module.h>
/* needed for __init,__exit directives */
#include <linux/init.h>
/* needed for remap_page_range
 * SetPageReserved
 * ClearPageReserved
 */
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>

#include <linux/moduleparam.h>
/* request_irq(), free_irq() */
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <linux/semaphore.h>
#include <linux/spinlock.h>
/* needed for virt_to_phys() */
#include <asm/io.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>

#include <asm/irq.h>

#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

/* our own stuff */
#include "vc8000_driver.h"
#include "vc8000_normal_cfg.h"

int return_val;
#ifdef HANTROMMU_SUPPORT
extern unsigned int mmu_enable;
extern volatile u8 *mmu_hwregs[HXDEC_MAX_CORES][2];
#else
static unsigned int mmu_enable;
#endif

//#define MULTI_THR_TEST
#ifdef MULTI_THR_TEST

#define WAIT_NODE_NUM 32
struct wait_list_node {
    u32 node_id;                //index of the node
    u32 used_flag;              //1:the node is insert to the wait queue list.
    u32 sem_used;               //1:the source is released and the semphone is uped.
    struct semaphore wait_sem;  //the unique semphone for per reserve_encoder thread.
    u32 wait_cond;              //the condition for wait. Equal to the "core_info".
    struct list_head wait_list; //list node.
};

static struct list_head reserve_header;
static struct wait_list_node res_wait_node[WAIT_NODE_NUM];

static void wait_delay(unsigned int delay)
{
    if (delay > 0) {
#if KERNEL_VERSION(2, 6, 28) <= LINUX_VERSION_CODE
        ktime_t dl = ktime_set((delay / MSEC_PER_SEC), (delay % MSEC_PER_SEC) * NSEC_PER_MSEC);
        __set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_hrtimeout(&dl, HRTIMER_MODE_REL);
#else
        msleep(delay);
#endif
    }
}

static u32 request_wait_node(struct wait_list_node **node, u32 start_id)
{
    u32 i;
    struct wait_list_node *temp_node;

    while (1) {
        for (i = start_id; i < WAIT_NODE_NUM; i++) {
            temp_node = &res_wait_node[i];
            if (temp_node->used_flag == 0) {
                temp_node->used_flag = 1;
                *node = temp_node;
                return i;
            }
        }
        wait_delay(10);
    }
}

static void request_wait_sema(struct wait_list_node **node)
{
    u32 i;
    struct wait_list_node *temp_node;

    while (1) {
        for (i = 0; i < WAIT_NODE_NUM; i++) {
            temp_node = &res_wait_node[i];
            if (temp_node->used_flag == 0 && temp_node->sem_used == 0) {
                temp_node->sem_used = 1;
                *node = temp_node;
                return;
            }
        }
        wait_delay(10);
    }
}

static void init_wait_node(struct wait_list_node *node, u32 cond, u32 sem_flag)
{
    node->used_flag = 0;
    node->wait_cond = cond;
    sema_init(&node->wait_sem, sem_flag);
    INIT_LIST_HEAD(&node->wait_list);
    if (sem_flag > 0)
        node->sem_used = 1;
}

static void init_reserve_wait(u32 dev_num)
{
    u32 i;
    u32 cond = 0x80000001;
    u32 sem_flag = 0;
    struct wait_list_node *node;

    //    printk("%s,%d, dev_num %d\n",__FUNCTION__,__LINE__,dev_num);

    INIT_LIST_HEAD(&reserve_header);

    for (i = 0; i < WAIT_NODE_NUM; i++) {
        if (i < dev_num)
            sem_flag = 1;
        else
            sem_flag = 0;
        node = &res_wait_node[i];
        node->node_id = i;
        init_wait_node(node, cond, sem_flag);
    }
}

void release_reserve_wait(u32 dev_num)
{
}

#endif
/********variables declaration related with race condition**********/

static struct semaphore enc_core_sem;
static DECLARE_WAIT_QUEUE_HEAD(hw_queue);
static DEFINE_SPINLOCK(owner_lock);
static DECLARE_WAIT_QUEUE_HEAD(enc_wait_queue);

#ifdef PCIE_EN
/*******************PCIE CONFIG*************************/
#ifdef EMU
#define PCI_VENDOR_ID_HANTRO 0x1d9b     //0x10ee//0x16c3
#define PCI_DEVICE_ID_HANTRO_PCI 0xface //0x8014// 0x7011
/* Base address got control register */
#define PCI_H2_BAR 2
/* Base address DDR register */
#define PCI_DDR_BAR 4
#else
#define PCI_VENDOR_ID_HANTRO 0x10ee     //0x16c3
#define PCI_DEVICE_ID_HANTRO_PCI 0x8014 // 0x7011
/* Base address got control register */
#define PCI_H2_BAR 4
/* Base address DDR register */
#define PCI_DDR_BAR 0
#endif

struct pci_dev *gDev;     /* PCI device structure. */
unsigned long gBaseHdwr;  /* PCI base register address (Hardware address) */
unsigned long gBaseDDRHw; /* PCI base register address (memalloc) */
u32 gBaseLen;             /* Base register address Length */
unsigned int pcie = 1;    /* used in hantro_mmu.c*/
#else
static unsigned int pcie; /* used in hantro_mmu.c*/
#endif

/*------------------------------------------------------------------------
 *****************************PORTING LAYER********************************
 *--------------------------------------------------------------------------
 */
/*------------------------------END-------------------------------------*/

/***************************TYPE AND FUNCTION DECLARATION****************/

/* here's all the must remember stuff */
typedef struct {
    SUBSYS_DATA subsys_data;    //config of each core,such as base addr, iosize,etc
    u32 hw_id;                  //VC8000E/VC8000EJ hw id to indicate project
    u32 subsys_id;              //subsys id for driver and sw internal use
    u32 is_valid;               //indicate this subsys is hantro's core or not
    int pid[CORE_MAX];          //indicate which process is occupying the subsys
    u32 is_reserved[CORE_MAX];  //indicate this subsys is occupied by user or not
    u32 irq_received[CORE_MAX]; //indicate which core receives irq
    u32 irq_status[CORE_MAX];   //IRQ status of each core
    u32 job_id[CORE_MAX];
    char *buffer;
    unsigned int buffsize;

    volatile u8 *hwregs;
    struct fasync_struct *async_queue;
} hantroenc_t;

static int ReserveIO(void);
static void ReleaseIO(void);
//static void ResetAsic(hantroenc_t * dev);

#ifdef hantroenc_DEBUG
static void dump_regs(unsigned long data);
#endif

/* IRQ handler */
#if (KERNEL_VERSION(2, 6, 18) > LINUX_VERSION_CODE)
static irqreturn_t hantroenc_isr(int irq, void *dev_id, struct pt_regs *regs);
#else
static irqreturn_t hantroenc_isr(int irq, void *dev_id);
#endif

/*********************local variable declaration*****************/
static unsigned long sram_base;
static unsigned int sram_size;
/* and this is our MAJOR; use 0 for dynamic allocation (recommended)*/
static int hantroenc_major;
static int total_subsys_num;
static int total_core_num;
//static volatile unsigned int asic_status;
/* dynamic allocation*/
static hantroenc_t *hantroenc_data;
#ifdef IRQ_SIMULATION
struct timer_list timer0;
struct timer_list timer1;
#endif

/******************************************************************************/
static int CheckEncIrq(hantroenc_t *dev, u32 *core_info, u32 *irq_status, u32 *job_id)
{
    unsigned long flags;
    int rdy = 0;
    u8 core_type = 0;
    u8 subsys_idx = 0;

    core_type = (u8)(*core_info & 0x0F);
    subsys_idx = (u8)(*core_info >> 4);
    pr_info("***************************subsys_idx, total_subsys_num %d %d\n", subsys_idx,
            total_subsys_num);
    if (subsys_idx > total_subsys_num - 1) {
        *core_info = -1;
        *irq_status = 0;
        return 1;
    }

    spin_lock_irqsave(&owner_lock, flags);

    if (dev[subsys_idx].irq_received[core_type]) {
        /* reset the wait condition(s) */
        PDEBUG("check subsys[%d][%d] irq ready\n", subsys_idx, core_type);
        //dev[subsys_idx].irq_received[core_type] = 0;
        rdy = 1;
        *core_info = subsys_idx;
        *irq_status = dev[subsys_idx].irq_status[core_type];
        if (job_id)
            *job_id = dev[subsys_idx].job_id[core_type];
    }

    spin_unlock_irqrestore(&owner_lock, flags);

    return rdy;
}

static unsigned int WaitEncReady(hantroenc_t *dev, u32 *core_info, u32 *irq_status)
{
    PDEBUG("%s\n", __func__);

    if (wait_event_interruptible(enc_wait_queue, CheckEncIrq(dev, core_info, irq_status, NULL))) {
        PDEBUG("ENC wait_event_interruptible interrupted\n");
        return -ERESTARTSYS;
    }

    return 0;
}

static int CheckEncIrqbyPolling(hantroenc_t *dev, u32 *core_info, u32 *irq_status, u32 *job_id)
{
    unsigned long flags;
    int rdy = 0;
    u8 core_type = 0;
    u8 subsys_idx = 0;
    u32 irq, hwId, majorId, wClr, minorId, productId;
    unsigned long reg_offset = 0;
    u32 loop = 300;
    u32 interval = 10;
    u32 enable_status = 0;

    core_type = (u8)(*core_info & 0x0F);
    subsys_idx = (u8)(*core_info >> 4);

    if (subsys_idx > total_subsys_num - 1) {
        *core_info = -1;
        *irq_status = 0;
        return 1;
    }

    do {
        spin_lock_irqsave(&owner_lock, flags);
        if (dev[subsys_idx].is_reserved[core_type] == 0) {
            /*printk(KERN_DEBUG"subsys[%d][%d]  is not reserved\n",
			 *subsys_idx, core_type);
			 */
            goto end_1;
        } else if (dev[subsys_idx].irq_received[core_type] &&
                   (dev[subsys_idx].irq_status[core_type] &
                    (ASIC_STATUS_FUSE_ERROR | ASIC_STATUS_HW_TIMEOUT | ASIC_STATUS_BUFF_FULL |
                     ASIC_STATUS_HW_RESET | ASIC_STATUS_ERROR | ASIC_STATUS_FRAME_READY))) {
            rdy = 1;
            *core_info = subsys_idx;
            *irq_status = dev[subsys_idx].irq_status[core_type];
            *job_id = dev[subsys_idx].job_id[core_type];
            goto end_1;
        }

        reg_offset = dev[subsys_idx].subsys_data.core_info.offset[core_type];
        irq = (u32)ioread32((void __iomem *)(dev[subsys_idx].hwregs + reg_offset + 0x04));

        enable_status = (u32)ioread32((void __iomem *)(dev[subsys_idx].hwregs + reg_offset + 20));

        if (irq & ASIC_STATUS_ALL) {
            PDEBUG("check subsys[%d][%d] irq ready\n", subsys_idx, core_type);
            if (irq & 0x20)
                iowrite32(0, (void __iomem *)(dev[subsys_idx].hwregs + reg_offset + 0x14));

            /* clear all IRQ bits. (hwId >= 0x80006100) means IRQ is cleared
			 * by writing 1
			 */
            hwId = ioread32((void __iomem *)dev[subsys_idx].hwregs + reg_offset);
            productId = (hwId & 0xFFFF0000) >> 16;
            majorId = (hwId & 0x0000FF00) >> 8;
            minorId = (hwId & 0x000000FF);
            wClr = ((majorId >= 0x61) || (productId == 0x9000) || (productId == 0x9010) ||
                    ((productId == 0x8000) && (minorId >= 1)))
                       ? irq
                       : (irq & (~0x1FD));
            iowrite32(wClr, (void __iomem *)(dev[subsys_idx].hwregs + reg_offset + 0x04));

            rdy = 1;
            *core_info = subsys_idx;
            *irq_status = irq;
            dev[subsys_idx].irq_received[core_type] = 1;
            dev[subsys_idx].irq_status[core_type] = irq;
            *job_id = dev[subsys_idx].job_id[core_type];
            goto end_1;
        }

        spin_unlock_irqrestore(&owner_lock, flags);
        mdelay(interval);
    } while (loop--);
    goto end_2;

end_1:
    spin_unlock_irqrestore(&owner_lock, flags);
end_2:
    return rdy;
}

static int CheckEncAnyIrqByPolling(hantroenc_t *dev, CORE_WAIT_OUT *out)
{
    u32 i;
    int rdy = 0;
    u32 core_info, irq_status, job_id;
    u32 core_type = CORE_VC8000E;

    for (i = 0; i < total_subsys_num; i++) {
        if (!(dev[i].subsys_data.core_info.type_info & (1 << core_type)))
            continue;

        core_info = ((i << 4) | core_type);
        if ((CheckEncIrqbyPolling(dev, &core_info, &irq_status, &job_id) == 1) &&
            (core_info == i)) {
            out->job_id[out->irq_num] = job_id;
            out->irq_status[out->irq_num] = irq_status;
            //printk(KERN_DEBUG "irq_status of subsys %d job_id %d is:%x\n",i,job_id,irq_status);
            out->irq_num++;
            rdy = 1;
        }
    }

    return rdy;
}

static unsigned int WaitEncAnyReadyByPolling(hantroenc_t *dev, CORE_WAIT_OUT *out)
{
    CheckEncAnyIrqByPolling(dev, out);
    return 0;
}

static int CheckEncAnyIrq(hantroenc_t *dev, CORE_WAIT_OUT *out)
{
    u32 i;
    int rdy = 0;
    u32 core_info, irq_status, job_id;
    u32 core_type = CORE_VC8000E;

    for (i = 0; i < total_subsys_num; i++) {
        if (!(dev[i].subsys_data.core_info.type_info & (1 << core_type)))
            continue;

        core_info = ((i << 4) | core_type);
        if ((CheckEncIrq(dev, &core_info, &irq_status, &job_id) == 1) && core_info == i) {
            out->job_id[out->irq_num] = job_id;
            out->irq_status[out->irq_num] = irq_status;
            /*printk(KERN_DEBUG "irq_status of subsys %d job_id %d is:%x\n",
			 *i,job_id,irq_status);
			 */
            out->irq_num++;
            rdy = 1;
        }
    }

    return rdy;
}

static unsigned int WaitEncAnyReady(hantroenc_t *dev, CORE_WAIT_OUT *out)
{
    if (wait_event_interruptible(enc_wait_queue, CheckEncAnyIrq(dev, out))) {
        PDEBUG("ENC wait_event_interruptible interrupted\n");
        return -ERESTARTSYS;
    }

    return 0;
}

static int CheckCoreOccupation(hantroenc_t *dev, u8 core_type)
{
    int ret = 0;
    unsigned long flags;

    core_type = (core_type == CORE_VC8000EJ ? CORE_VC8000E : core_type);

    spin_lock_irqsave(&owner_lock, flags);
    if (!dev->is_reserved[core_type]) {
        dev->is_reserved[core_type] = 1;
#ifndef MULTI_THR_TEST
        dev->pid[core_type] = current->pid;
#endif
        ret = 1;
        PDEBUG("%s pid=%d\n", __func__, dev->pid[core_type]);
    }

    spin_unlock_irqrestore(&owner_lock, flags);

    return ret;
}

static int GetWorkableCore(hantroenc_t *dev, u32 *core_info, u32 *core_info_tmp)
{
    int ret = 0;
    u32 i = 0;
    u32 cores;
    u8 core_type = 0;
    u32 required_num = 0;
    static u32 reserved_job_id;
    unsigned long flags;
    /*input core_info[32 bit]: mode[1bit](1:all 0:specified)+amount[3bit]
	 *(the needing amount -1)+reserved+core_type[8bit]

     *output core_info[32 bit]: the reserved core info to user space and
	 *defined as below.
     *mode[1bit](1:all 0:specified)+amount[3bit](reserved total core num)+
	 *reserved+subsys_mapping[8bit]
	 */
    cores = *core_info;
    required_num = ((cores >> CORE_INFO_AMOUNT_OFFSET) & 0x7) + 1;
    core_type = (u8)(cores & 0xFF);

    if (*core_info_tmp == 0)
        *core_info_tmp = required_num << CORE_INFO_AMOUNT_OFFSET;
    else
        required_num = (*core_info_tmp >> CORE_INFO_AMOUNT_OFFSET);

    PDEBUG("%s:required_num=%d,core_info=%x\n", __func__, required_num, *core_info);

    if (required_num) {
        /* a valid free Core with specified core type */
        for (i = 0; i < total_subsys_num; i++) {
            if (dev[i].subsys_data.core_info.type_info & (1 << core_type)) {
                core_type = (core_type == CORE_VC8000EJ ? CORE_VC8000E : core_type);
                if (dev[i].is_valid && CheckCoreOccupation(&dev[i], core_type)) {
                    *core_info_tmp = ((((*core_info_tmp >> CORE_INFO_AMOUNT_OFFSET) - 1)
                                       << CORE_INFO_AMOUNT_OFFSET) |
                                      (*core_info_tmp & 0x0FF));
                    *core_info_tmp = (*core_info_tmp | (1 << i));
                    if ((*core_info_tmp >> CORE_INFO_AMOUNT_OFFSET) == 0) {
                        ret = 1;
                        spin_lock_irqsave(&owner_lock, flags);
                        *core_info = (reserved_job_id << 16) | (*core_info_tmp & 0xFF);
                        dev[i].job_id[core_type] = reserved_job_id;
                        /*maintain job_id in 16 bits for core_info can
						 *only save job_id in high 16 bits
						 */
                        reserved_job_id = (reserved_job_id + 1) & 0xFFFF;
                        spin_unlock_irqrestore(&owner_lock, flags);
                        *core_info_tmp = 0;
                        required_num = 0;
                        break;
                    }
                }
            }
        }
    } else
        ret = 1;

    PDEBUG("*core_info = %x\n", *core_info);
    return ret;
}

static long ReserveEncoder(hantroenc_t *dev, u32 *core_info)
{
    u32 core_info_tmp = 0;
#ifdef MULTI_THR_TEST
    struct wait_list_node *wait_node;
    u32 start_id = 0;
#endif

    /*If HW resources are shared inter cores, just make sure only one is
	 *using the HW
	 */
    if (dev[0].subsys_data.cfg.resource_shared) {
        if (down_interruptible(&enc_core_sem))
            return -ERESTARTSYS;
    }

#ifdef MULTI_THR_TEST
    while (1) {
        start_id = request_wait_node(&wait_node, start_id);
        if (wait_node->sem_used == 1) {
            if (GetWorkableCore(dev, core_info, &core_info_tmp)) {
                down_interruptible(&wait_node->wait_sem);
                wait_node->sem_used = 0;
                wait_node->used_flag = 0;
                break;
            } else {
                start_id++;
            }
        } else {
            wait_node->wait_cond = *core_info;
            list_add_tail(&wait_node->wait_list, &reserve_header);
            down_interruptible(&wait_node->wait_sem);
            *core_info = wait_node->wait_cond;
            list_del(&wait_node->wait_list);
            wait_node->sem_used = 0;
            wait_node->used_flag = 0;
            break;
        }
    }
#else

    /* lock a core that has specified core id*/
    if (wait_event_interruptible(hw_queue, GetWorkableCore(dev, core_info, &core_info_tmp) != 0))
        return -ERESTARTSYS;
#endif
    return 0;
}

static void ReleaseEncoder(hantroenc_t *dev, u32 *core_info)
{
    unsigned long flags;
    u8 core_type = 0, subsys_idx = 0, unCheckPid = 0;

    unCheckPid = (u8)((*core_info) >> 31);
#ifdef MULTI_THR_TEST
    u32 release_ok = 0;
    struct list_head *node;
    struct wait_list_node *wait_node;
    u32 core_info_tmp = 0;
#endif
    subsys_idx = (u8)((*core_info & 0xF0) >> 4);
    core_type = (u8)(*core_info & 0x0F);

    PDEBUG("%s:subsys_idx=%d,core_type=%x\n", __func__, subsys_idx, core_type);
    /* release specified subsys and core type */

    if (dev[subsys_idx].subsys_data.core_info.type_info & (1 << core_type)) {
        core_type = (core_type == CORE_VC8000EJ ? CORE_VC8000E : core_type);
        spin_lock_irqsave(&owner_lock, flags);
        PDEBUG("subsys[%d].pid[%d]=%d,current->pid=%d\n", subsys_idx, core_type,
               dev[subsys_idx].pid[core_type], current->pid);
#ifdef MULTI_THR_TEST
        if (dev[subsys_idx].is_reserved[core_type])
#else
        if (dev[subsys_idx].is_reserved[core_type] &&
            (dev[subsys_idx].pid[core_type] == current->pid || unCheckPid == 1))
#endif
        {
            dev[subsys_idx].pid[core_type] = -1;
            dev[subsys_idx].is_reserved[core_type] = 0;
            dev[subsys_idx].irq_received[core_type] = 0;
            dev[subsys_idx].irq_status[core_type] = 0;
            dev[subsys_idx].job_id[core_type] = 0;
            spin_unlock_irqrestore(&owner_lock, flags);
#ifdef MULTI_THR_TEST
            release_ok = 0;
            if (list_empty(&reserve_header)) {
                request_wait_sema(&wait_node);
                up(&wait_node->wait_sem);
            } else {
                list_for_each(node, &reserve_header)
                {
                    wait_node = container_of(node, struct wait_list_node, wait_list);
                    if ((GetWorkableCore(dev, &wait_node->wait_cond, &core_info_tmp)) &&
                        wait_node->sem_used == 0) {
                        release_ok = 1;
                        wait_node->sem_used = 1;
                        up(&wait_node->wait_sem);
                        break;
                    }
                }
                if (release_ok == 0) {
                    request_wait_sema(&wait_node);
                    up(&wait_node->wait_sem);
                }
            }
#endif

        } else {
            if (dev[subsys_idx].pid[core_type] != current->pid && unCheckPid == 0)
                pr_info("WARNING:pid(%d) is trying to release core reserved by pid(%d)\n",
                        current->pid, dev[subsys_idx].pid[core_type]);
            spin_unlock_irqrestore(&owner_lock, flags);
        }
        //wake_up_interruptible_all(&hw_queue);
    }
#ifndef MULTI_THR_TEST
    wake_up_interruptible_all(&hw_queue);
#endif
    if (dev->subsys_data.cfg.resource_shared)
        up(&enc_core_sem);
}

#ifdef IRQ_SIMULATION
static void get_random_bytes(void *buf, int nbytes);

static void hantroenc_trigger_irq_0(unsigned long value)
{
    PDEBUG("trigger core 0 irq\n");
    del_timer(&timer0);
    hantroenc_isr(0, (void *)&hantroenc_data[0]);
}

static void hantroenc_trigger_irq_1(unsigned long value)
{
    PDEBUG("trigger core 1 irq\n");
    del_timer(&timer1);
    hantroenc_isr(0, (void *)&hantroenc_data[1]);
}

#endif

static long hantroenc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    unsigned int tmp;
#ifdef HANTROMMU_SUPPORT
    u32 i = 0;
#endif

    PDEBUG("ioctl cmd %08ux\n", cmd);
    /* extract the type and number bitfields, and don't encode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != HANTRO_IOC_MAGIC
#ifdef HANTROMMU_SUPPORT
        && _IOC_TYPE(cmd) != HANTRO_IOC_MMU
#endif
    )
        return -ENOTTY;
    if ((_IOC_TYPE(cmd) == HANTRO_IOC_MAGIC && _IOC_NR(cmd) > HANTRO_IOC_MAXNR)
#ifdef HANTROMMU_SUPPORT
        || (_IOC_TYPE(cmd) == HANTRO_IOC_MMU && _IOC_NR(cmd) > HANTRO_IOC_MMU_MAXNR)
#endif
    )
        return -ENOTTY;

    /* the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
        err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
#else
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
#endif
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
        err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
#else
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
#endif
    if (err)
        return -EFAULT;

    switch (cmd) {
    case HANTRO_IOCH_GET_VCMD_ENABLE: {
        __put_user(0, (unsigned long __user *)arg);
        break;
    }

    case HANTRO_IOCH_GET_MMU_ENABLE: {
        __put_user(mmu_enable, (unsigned int __user *)arg);
        break;
    }

    case HANTRO_IOCG_HWOFFSET: {
        u32 id;

        __get_user(id, (u32 __user *)arg);

        if (id >= total_subsys_num)
            return -EFAULT;
        __put_user(hantroenc_data[id].subsys_data.cfg.base_addr, (unsigned long __user *)arg);
        break;
    }

    case HANTRO_IOCG_HWIOSIZE: {
        u32 id;
        u32 io_size;

        __get_user(id, (u32 __user *)arg);

        if (id >= total_subsys_num)
            return -EFAULT;
        io_size = hantroenc_data[id].subsys_data.cfg.iosize;
        __put_user(io_size, (u32 __user *)arg);

        return 0;
    }
    case HANTRO_IOCG_SRAMOFFSET:
        __put_user(sram_base, (unsigned long __user *)arg);
        break;
    case HANTRO_IOCG_SRAMEIOSIZE:
        __put_user(sram_size, (unsigned int __user *)arg);
        break;
    case HANTRO_IOCG_CORE_NUM:
        __put_user(total_subsys_num, (unsigned int __user *)arg);
        break;
    case HANTRO_IOCG_CORE_INFO: {
        u32 idx;
        SUBSYS_CORE_INFO in_data;

        return_val = copy_from_user(&in_data, (void __user *)arg, sizeof(SUBSYS_CORE_INFO));
        idx = in_data.type_info;
        if (idx > total_subsys_num - 1)
            return -1;

        return_val = copy_to_user((void __user *)arg, &hantroenc_data[idx].subsys_data.core_info,
                                  sizeof(SUBSYS_CORE_INFO));
        break;
    }
    case HANTRO_IOCH_ENC_RESERVE: {
        u32 core_info;
        int ret;

        PDEBUG("Reserve ENC Cores\n");
        __get_user(core_info, (u32 __user *)arg);
        ret = ReserveEncoder(hantroenc_data, &core_info);
        if (ret == 0)
            __put_user(core_info, (u32 __user *)arg);
        return ret;
    }
    case HANTRO_IOCH_ENC_RELEASE: {
        u32 core_info;

        __get_user(core_info, (u32 __user *)arg);

        PDEBUG("Release ENC Core\n");

        ReleaseEncoder(hantroenc_data, &core_info);

        break;
    }

    case HANTRO_IOCG_CORE_WAIT: {
        u32 core_info;
        u32 irq_status;

        __get_user(core_info, (u32 __user *)arg);
#ifdef IRQ_SIMULATION
        u32 random_num;

        get_random_bytes(&random_num, sizeof(u32));
        random_num = random_num % 10 + 80;
        PDEBUG("random_num=%d\n", random_num);

        /*init a timer to trigger irq*/
        if (core_info == 1) {
            init_timer(&timer0);
            timer0.function = &hantroenc_trigger_irq_0;
            //the expires time is 1s
            timer0.expires = jiffies + random_num * HZ / 10;
            add_timer(&timer0);
        }

        if (core_info == 2) {
            init_timer(&timer1);
            timer1.function = &hantroenc_trigger_irq_1;
            //the expires time is 1s
            timer1.expires = jiffies + random_num * HZ / 10;
            add_timer(&timer1);
        }
#endif

        tmp = WaitEncReady(hantroenc_data, &core_info, &irq_status);
        if (tmp == 0) {
            __put_user(irq_status, (unsigned int __user *)arg);
            return core_info; //return core_id
        } else {
            return -1;
        }

        break;
    }
    case HANTRO_IOCG_ANYCORE_WAIT_POLLING: {
        CORE_WAIT_OUT out;

        memset(&out, 0, sizeof(CORE_WAIT_OUT));
#ifdef IRQ_SIMULATION
        u32 random_num;

        get_random_bytes(&random_num, sizeof(u32));
        random_num = random_num % 10 + 80;
        PDEBUG("random_num=%d\n", random_num);

        /*init a timer to trigger irq*/
        if (core_info == 1) {
            init_timer(&timer0);
            timer0.function = &hantroenc_trigger_irq_0;
            //the expires time is 1s
            timer0.expires = jiffies + random_num * HZ / 10;
            add_timer(&timer0);
        }

        if (core_info == 2) {
            init_timer(&timer1);
            timer1.function = &hantroenc_trigger_irq_1;
            //the expires time is 1s
            timer1.expires = jiffies + random_num * HZ / 10;
            add_timer(&timer1);
        }
#endif

        tmp = WaitEncAnyReadyByPolling(hantroenc_data, &out);
        if (tmp == 0) {
            return_val = copy_to_user((void __user *)arg, &out, sizeof(CORE_WAIT_OUT));
            return 0;
        } else
            return -1;

        break;
    }
    case HANTRO_IOCG_ANYCORE_WAIT: {
        CORE_WAIT_OUT out;

        memset(&out, 0, sizeof(CORE_WAIT_OUT));

        tmp = WaitEncAnyReady(hantroenc_data, &out);
        if (tmp == 0) {
            return_val = copy_to_user((void __user *)arg, &out, sizeof(CORE_WAIT_OUT));
            return 0;
        } else
            return -1;

        break;
    }

    default: {
#ifdef HANTROMMU_SUPPORT
        if (_IOC_TYPE(cmd) == HANTRO_IOC_MMU) {
            if (mmu_enable)
                return MMUIoctl(cmd, filp, arg, mmu_hwregs);
            else
                return -1;
        }
#endif
    }
    }
    return 0;
}

static int hantroenc_open(struct inode *inode, struct file *filp)
{
    int result = 0;
    hantroenc_t *dev = hantroenc_data;

    filp->private_data = (void *)dev;

    PDEBUG("dev opened\n");
    return result;
}

static int hantroenc_release(struct inode *inode, struct file *filp)
{
    hantroenc_t *dev = (hantroenc_t *)filp->private_data;
    u32 core_id = 0, i = 0;

#ifdef hantroenc_DEBUG
    dump_regs((unsigned long)dev); /* dump the regs */
#endif
    unsigned long flags;

    PDEBUG("dev closed\n");

    for (i = 0; i < total_subsys_num; i++) {
        for (core_id = 0; core_id < CORE_MAX; core_id++) {
            spin_lock_irqsave(&owner_lock, flags);
            if (dev[i].is_reserved[core_id] == 1 && dev[i].pid[core_id] == current->pid) {
                dev[i].pid[core_id] = -1;
                dev[i].is_reserved[core_id] = 0;
                dev[i].irq_received[core_id] = 0;
                dev[i].irq_status[core_id] = 0;
                PDEBUG("release reserved core\n");
            }
            spin_unlock_irqrestore(&owner_lock, flags);
        }
    }

#ifdef HANTROMMU_SUPPORT
    for (i = 0; i < total_subsys_num; i++) {
        if (!(hantroenc_data[i].subsys_data.core_info.type_info & (1 << CORE_MMU)))
            continue;

        MMURelease(filp, hantroenc_data[i].hwregs +
                             hantroenc_data[i].subsys_data.core_info.offset[CORE_MMU]);
        break;
    }
#endif

    wake_up_interruptible_all(&hw_queue);

    if (dev->subsys_data.cfg.resource_shared)
        up(&enc_core_sem);

    return 0;
}

/* VFS methods */
static struct file_operations hantroenc_fops = {
    .owner = THIS_MODULE,
    .open = hantroenc_open,
    .release = hantroenc_release,
    .unlocked_ioctl = hantroenc_ioctl,
    .fasync = NULL,
};

#ifdef PCIE_EN
/*------------------------------------------------------------------------------
 *Function name   : PcieInit
 *Description     : Initialize PCI Hw access
 *
 *Return type     : int
 *------------------------------------------------------------------------------
 */
static int PcieInit(void)
{
    int i = 0;

    gDev = pci_get_device(PCI_VENDOR_ID_HANTRO, PCI_DEVICE_ID_HANTRO_PCI, gDev);
    if (!gDev) {
        pr_err("Init: Hardware not found.\n");
        goto out;
    }

    if (pci_enable_device(gDev) < 0) {
        pr_err("Init: Device not enabled.\n");
        goto out;
    }

    gBaseHdwr = pci_resource_start(gDev, PCI_H2_BAR);
    if (gBaseHdwr < 0) {
        pr_info("Init: Base Address not set.\n");
        goto out_pci_disable_device;
    }
    pr_info("Base hw val 0x%llx\n", (unsigned long long)gBaseHdwr);

    gBaseLen = pci_resource_len(gDev, PCI_H2_BAR);
    pr_info("Base hw len 0x%08x\n", (unsigned int)gBaseLen);

    for (i = 0; i < total_subsys_num; i++) {
        //the offset is based on which bus interface is chosen
        subsys_array[i].base_addr = gBaseHdwr + subsys_array[i].base_addr;
    }

    sram_base = gBaseHdwr + 0x000000; //axi0 interface
    sram_size = 0x80000;

    gBaseDDRHw = pci_resource_start(gDev, PCI_DDR_BAR);
    if (gBaseDDRHw == 0) {
        pr_info("%s: Base Address not set.\n", __func__);
        goto out_pci_disable_device;
    }
    pr_info("Base memory val 0x%llx\n", (unsigned long long)gBaseDDRHw);

    gBaseLen = pci_resource_len(gDev, PCI_DDR_BAR);
    pr_info("Base memory len 0x%08x\n", (unsigned int)gBaseLen);

    return 0;

out_pci_disable_device:
    pci_disable_device(gDev);
out:
    return -1;
}
#endif

int hantroenc_normal_init(void)
{
    int result = 0;
    int i, j;

#ifdef HANTROMMU_SUPPORT
    enum MMUStatus mmu_status = MMU_STATUS_FALSE;
#endif

    total_subsys_num = sizeof(subsys_array) / sizeof(SUBSYS_CONFIG);

#ifdef PCIE_EN
    result = PcieInit();
    if (result)
        goto err1;
#endif

    for (i = 0; i < total_subsys_num; i++) {
        pr_info("hantroenc: module init - subsys[%d] addr =%p\n", i,
                (void *)subsys_array[i].base_addr);
    }

    hantroenc_data = vmalloc(sizeof(hantroenc_t) * total_subsys_num);
    if (!hantroenc_data)
        goto err1;
    memset(hantroenc_data, 0, sizeof(hantroenc_t) * total_subsys_num);

    for (i = 0; i < total_subsys_num; i++) {
        hantroenc_data[i].subsys_data.cfg = subsys_array[i];
        hantroenc_data[i].async_queue = NULL;
        hantroenc_data[i].hwregs = NULL;
        hantroenc_data[i].subsys_id = i;
        for (j = 0; j < CORE_MAX; j++)
            hantroenc_data[i].subsys_data.core_info.irq[j] = -1;
    }

    total_core_num = sizeof(core_array) / sizeof(CORE_CONFIG);
    for (i = 0; i < total_core_num; i++) {
        hantroenc_data[core_array[i].subsys_idx].subsys_data.core_info.type_info |=
            (1 << (core_array[i].core_type));
        hantroenc_data[core_array[i].subsys_idx]
            .subsys_data.core_info.offset[core_array[i].core_type] = core_array[i].offset;
        hantroenc_data[core_array[i].subsys_idx]
            .subsys_data.core_info.regSize[core_array[i].core_type] = core_array[i].reg_size;
        hantroenc_data[core_array[i].subsys_idx]
            .subsys_data.core_info.irq[core_array[i].core_type] = core_array[i].irq;
    }

    result = register_chrdev(hantroenc_major, "vc8000", &hantroenc_fops);
    if (result < 0) {
        pr_info("hantroenc: unable to get major <%d>\n", hantroenc_major);
        goto err1;
    } else if (result != 0) {
        /* this is for dynamic major */
        hantroenc_major = result;
    }

    result = ReserveIO();
    if (result < 0)
        goto err;

    //ResetAsic(hantroenc_data);  /* reset hardware */

    sema_init(&enc_core_sem, 1);

#ifdef HANTROMMU_SUPPORT
    /* MMU only initial once No matter how many MMU we have */
    for (i = 0; i < total_subsys_num; i++) {
        if (!(hantroenc_data[i].subsys_data.core_info.type_info & (1 << CORE_MMU)))
            continue;

        result = MMUInit(hantroenc_data[i].hwregs +
                         hantroenc_data[i].subsys_data.core_info.offset[CORE_MMU]);
        if (result == MMU_STATUS_NOT_FOUND)
            pr_info("MMU does not exist!\n");
        else if (result != MMU_STATUS_OK) {
            ReleaseIO();
            goto err;
        }

        break;
    }

    memset(mmu_hwregs, 0, MAX_SUBSYS_NUM * 2);
    for (i = 0; i < total_subsys_num; i++) {
        if (hantroenc_data[i].subsys_data.core_info.type_info & (1 << CORE_MMU))
            mmu_hwregs[i][0] =
                hantroenc_data[i].hwregs + hantroenc_data[i].subsys_data.core_info.offset[CORE_MMU];

        if (hantroenc_data[i].subsys_data.core_info.type_info & (1 << CORE_MMU_1))
            mmu_hwregs[i][1] = hantroenc_data[i].hwregs +
                               hantroenc_data[i].subsys_data.core_info.offset[CORE_MMU_1];
    }

    mmu_status = MMUEnable(mmu_hwregs);
    PDEBUG("mmu_enable is %d\n", mmu_enable);
    if (mmu_status != MMU_STATUS_OK) {
        pr_info("MMUEnable: MMU enable failed\n");
        return -3;
    }
#endif

    /* get the IRQ line */
    for (i = 0; i < total_subsys_num; i++) {
        if (hantroenc_data[i].is_valid == 0)
            continue;

        for (j = 0; j < CORE_MAX; j++) {
            if (hantroenc_data[i].subsys_data.core_info.irq[j] != -1) {
                result = request_irq(hantroenc_data[i].subsys_data.core_info.irq[j], hantroenc_isr,
#if (KERNEL_VERSION(2, 6, 18) > LINUX_VERSION_CODE)
                                     SA_INTERRUPT | SA_SHIRQ,
#else
                                     IRQF_SHARED,
#endif
                                     "vc8000", (void *)&hantroenc_data[i]);
                if (result == -EINVAL) {
                    pr_err("hantroenc: Bad irq number or handler\n");
                    ReleaseIO();
                    goto err;
                } else if (result == -EBUSY) {
                    pr_err("hantroenc: IRQ <%d> busy, change your config\n",
                           hantroenc_data[i].subsys_data.core_info.irq[j]);
                    ReleaseIO();
                    goto err;
                }
            } else {
                pr_info("hantroenc: IRQ not in use!\n");
            }
        }
    }
#ifdef MULTI_THR_TEST
    init_reserve_wait(total_subsys_num);
#endif
    pr_info("hantroenc: module inserted. Major <%d>\n", hantroenc_major);

    return 0;

err:
    unregister_chrdev(hantroenc_major, "vc8000");
err1:
    if (hantroenc_data)
        vfree(hantroenc_data);
    pr_info("hantroenc: module not inserted\n");
    return result;
}

void hantroenc_normal_cleanup(void)
{
    int i = 0, j = 0;

    for (i = 0; i < total_subsys_num; i++) {
        if (hantroenc_data[i].is_valid == 0)
            continue;
        //writel(0, hantroenc_data[i].hwregs + 0x14); /* disable HW */
        //writel(0, hantroenc_data[i].hwregs + 0x04); /* clear enc IRQ */

        /* free the core IRQ */
        for (j = 0; j < total_core_num; j++) {
            if (hantroenc_data[i].subsys_data.core_info.irq[j] != -1) {
                free_irq(hantroenc_data[i].subsys_data.core_info.irq[j],
                         (void *)&hantroenc_data[i]);
            }
        }
    }

#ifdef HANTROMMU_SUPPORT
    MMUCleanup(mmu_hwregs);
#endif

    ReleaseIO();
    vfree(hantroenc_data);

    unregister_chrdev(hantroenc_major, "vc8000");

    pr_info("hantroenc: module removed\n");
    return;
}

static int ReserveIO(void)
{
    u32 hwid;
    int i;
    u32 found_hw = 0, hw_cfg;
    u32 VC8000E_core_idx;

    for (i = 0; i < total_subsys_num; i++) {
        if (!request_mem_region(hantroenc_data[i].subsys_data.cfg.base_addr,
                                hantroenc_data[i].subsys_data.cfg.iosize, "vc8000")) {
            pr_info("hantroenc: failed to reserve HW regs\n");
            continue;
        }
#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE)
        hantroenc_data[i].hwregs = (volatile u8 __force *)ioremap_nocache(
            hantroenc_data[i].subsys_data.cfg.base_addr, hantroenc_data[i].subsys_data.cfg.iosize);
#else
        hantroenc_data[i].hwregs = (volatile u8 *)ioremap(
            hantroenc_data[i].subsys_data.cfg.base_addr, hantroenc_data[i].subsys_data.cfg.iosize);
#endif
        if (!hantroenc_data[i].hwregs) {
            pr_info("hantroenc: failed to ioremap HW regs\n");
            ReleaseIO();
            continue;
        }

        /*read hwid and check validness and store it*/
        VC8000E_core_idx = GET_ENCODER_IDX(hantroenc_data[0].subsys_data.core_info.type_info);
        if (!(hantroenc_data[i].subsys_data.core_info.type_info & (1 << CORE_VC8000E)))
            VC8000E_core_idx = CORE_CUTREE;
        hwid = (u32)ioread32((void __iomem *)hantroenc_data[i].hwregs +
                             hantroenc_data[i].subsys_data.core_info.offset[VC8000E_core_idx]);
        pr_info("hwid=0x%08x\n", hwid);

        /* check for encoder HW ID */
        if (((((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID1 >> 16) & 0xFFFF))) &&
            ((((hwid >> 16) & 0xFFFF) != ((ENC_HW_ID2 >> 16) & 0xFFFF)))) {
            pr_info("hantroenc: HW not found at %p\n",
                    (void *)hantroenc_data[i].subsys_data.cfg.base_addr);
#ifdef hantroenc_DEBUG
            dump_regs((unsigned long)&hantroenc_data);
#endif
            hantroenc_data[i].is_valid = 0;
            ReleaseIO();
            continue;
        }
        hantroenc_data[i].hw_id = hwid;
        hantroenc_data[i].is_valid = 1;
        found_hw = 1;

        hw_cfg =
            (u32)ioread32((void __iomem *)hantroenc_data[i].hwregs +
                          hantroenc_data[i].subsys_data.core_info.offset[VC8000E_core_idx] + 320);
        hantroenc_data[i].subsys_data.core_info.type_info &= 0xFFFFFFFC;
        if (hw_cfg & 0x88000000)
            hantroenc_data[i].subsys_data.core_info.type_info |= (1 << CORE_VC8000E);
        if (hw_cfg & 0x00008000)
            hantroenc_data[i].subsys_data.core_info.type_info |= (1 << CORE_VC8000EJ);

        pr_info("hantroenc: HW at base <%p> with ID <0x%08x>\n",
                (void *)hantroenc_data[i].subsys_data.cfg.base_addr, hwid);
    }

    if (found_hw == 0) {
        pr_err("hantroenc: NO ANY HW found!!\n");
        return -1;
    }

    return 0;
}

static void ReleaseIO(void)
{
    u32 i;

    for (i = 0; i <= total_subsys_num; i++) {
        if (hantroenc_data[i].is_valid == 0)
            continue;
        if (hantroenc_data[i].hwregs)
            iounmap((void __iomem *)hantroenc_data[i].hwregs);
        release_mem_region(hantroenc_data[i].subsys_data.cfg.base_addr,
                           hantroenc_data[i].subsys_data.cfg.iosize);
    }
}

#if (KERNEL_VERSION(2, 6, 18) > LINUX_VERSION_CODE)
static irqreturn_t hantroenc_isr(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t hantroenc_isr(int irq, void *dev_id)
#endif
{
    unsigned int handled = 0;
    hantroenc_t *dev = (hantroenc_t *)dev_id;
    u32 irq_status;
    unsigned long flags;
    u32 core_type = 0, i = 0;
    unsigned long reg_offset = 0;
    u32 hwId, majorId, wClr, minorId, productId;

    /*get core id by irq from subsys config*/
    for (i = 0; i < CORE_MAX; i++) {
        if (dev->subsys_data.core_info.irq[i] == irq) {
            core_type = i;
            reg_offset = dev->subsys_data.core_info.offset[i];
            break;
        }
    }

    /*If core is not reserved by any user, but irq is received, just clean it*/
    spin_lock_irqsave(&owner_lock, flags);
    if (!dev->is_reserved[core_type]) {
        pr_debug("hantroenc_isr:received IRQ but core is not reserved!\n");
        irq_status = (u32)ioread32((void __iomem *)(dev->hwregs + reg_offset + 0x04));
        if (irq_status & 0x01) {
            /*  Disable HW when buffer over-flow happen
			 *  HW behavior changed in over-flow
			 *    in-pass, HW cleanup HWIF_ENC_E auto
			 *    new version:  ask SW cleanup HWIF_ENC_E when buffer over-flow
			 */
            if (irq_status & 0x20)
                iowrite32(0, (void __iomem *)(dev->hwregs + reg_offset + 0x14));

            /* clear all IRQ bits. (hwId >= 0x80006100) means IRQ is cleared by writing 1 */
            hwId = ioread32((void __iomem *)dev->hwregs + reg_offset);
            productId = (hwId & 0xFFFF0000) >> 16;
            majorId = (hwId & 0x0000FF00) >> 8;
            minorId = (hwId & 0x000000FF);
            wClr = ((majorId >= 0x61) || (productId == 0x9000) || (productId == 0x9010) ||
                    ((productId == 0x8000) && (minorId >= 1)))
                       ? irq_status
                       : (irq_status & (~0x1FD));
            iowrite32(wClr, (void __iomem *)(dev->hwregs + reg_offset + 0x04));
        }
        spin_unlock_irqrestore(&owner_lock, flags);
        return IRQ_HANDLED;
    }
    spin_unlock_irqrestore(&owner_lock, flags);

    pr_debug("hantroenc_isr:received IRQ!\n");
    irq_status = (u32)ioread32((void __iomem *)(dev->hwregs + reg_offset + 0x04));
    pr_debug("irq_status of subsys %d core %d is:%x\n", dev->subsys_id, core_type, irq_status);
    if (irq_status & 0x01) {
        /*  Disable HW when buffer over-flow happen
	     *  HW behavior changed in over-flow
	     *    in-pass, HW cleanup HWIF_ENC_E auto
	     *    new version:  ask SW cleanup HWIF_ENC_E when buffer over-flow
	     */
        if (irq_status & 0x20)
            iowrite32(0, (void __iomem *)(dev->hwregs + reg_offset + 0x14));

        /* clear all IRQ bits. (hwId >= 0x80006100) means IRQ is cleared by writing 1 */
        hwId = ioread32((void __iomem *)dev->hwregs + reg_offset);
        productId = (hwId & 0xFFFF0000) >> 16;
        majorId = (hwId & 0x0000FF00) >> 8;
        minorId = (hwId & 0x000000FF);
        wClr = ((majorId >= 0x61) || (productId == 0x9000) || (productId == 0x9010) ||
                ((productId == 0x8000) && (minorId >= 1)))
                   ? irq_status
                   : (irq_status & (~0x1FD));
        iowrite32(wClr, (void __iomem *)(dev->hwregs + reg_offset + 0x04));

        spin_lock_irqsave(&owner_lock, flags);
        dev->irq_received[core_type] = 1;
        dev->irq_status[core_type] = irq_status & (~0x01);
        spin_unlock_irqrestore(&owner_lock, flags);

        wake_up_interruptible_all(&enc_wait_queue);
        handled++;
    }
    if (!handled)
        PDEBUG("IRQ received, but not hantro's!\n");
    return IRQ_HANDLED;
}

#ifdef hantroenc_DEBUG
static void ResetAsic(hantroenc_t *dev)
{
    int i, n;

    for (n = 0; n < total_subsys_num; n++) {
        if (dev[n].is_valid == 0)
            continue;
        iowrite32(0, (void *)(dev[n].hwregs + 0x14));
        for (i = 4; i < dev[n].subsys_data.cfg.iosize; i += 4)
            iowrite32(0, (void *)(dev[n].hwregs + i));
    }
}

static void dump_regs(unsigned long data)
{
    hantroenc_t *dev = (hantroenc_t *)data;
    int i;

    PDEBUG("Reg Dump Start\n");
    for (i = 0; i < dev->iosize; i += 4)
        PDEBUG("\toffset %02X = %08X\n", i, ioread32(dev->hwregs + i));
    PDEBUG("Reg Dump End\n");
}
#endif
