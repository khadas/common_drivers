/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DEBUG_FTRACE_RAMOOPS_H__
#define  __DEBUG_FTRACE_RAMOOPS_H__
#include <linux/pstore_ram.h>
#include <linux/arm-smccc.h>

extern unsigned int ramoops_ftrace_en;
extern int ramoops_io_en;
extern int ramoops_io_dump;
extern unsigned int dump_iomap;

#define PSTORE_FLAG_FUNC	0x1
#define PSTORE_FLAG_IO_R	0x2
#define PSTORE_FLAG_IO_W	0x3
#define PSTORE_FLAG_IO_R_END	0x4
#define PSTORE_FLAG_IO_W_END	0x5
#define PSTORE_FLAG_IO_TAG	0x6
#define PSTORE_FLAG_IO_SCHED_SWITCH 0x7
#define PSTORE_FLAG_IO_SMC_IN   0x8
#define PSTORE_FLAG_IO_SMC_OUT  0x9
#define PSTORE_FLAG_IO_SMC_NORET_IN  0xA
#define PSTORE_FLAG_CLK_ENABLE 0XB
#define PSTORE_FLAG_CLK_DISABLE 0XC
#define PSTORE_FLAG_PD_POWER_ON 0XD
#define PSTORE_FLAG_PD_POWER_OFF 0XE
#define PSTORE_FLAG_MASK	0xF

void notrace __nocfi pstore_io_save(unsigned long reg, unsigned long val,
			    unsigned long parant, unsigned int flag,
			    unsigned long *irq_flag);

void notrace __pstore_io_save(unsigned long reg, unsigned long val,
			    unsigned long parant, unsigned int flag,
			    unsigned long *irq_flag);

#ifdef MODULE
#define PSTORE_IO_SAVE pstore_io_save
#else
#define PSTORE_IO_SAVE __pstore_io_save
#endif

struct persistent_ram_zone;
void pstore_ftrace_dump_old(struct persistent_ram_zone *prz);

void save_iomap_info(unsigned long virt_addr, unsigned long phys_addr,
		     unsigned int size);

void delete_iomap_info(unsigned long addr);

#if (IS_ENABLED(CONFIG_AMLOGIC_BGKI_DEBUG_IOTRACE)) && (!defined SKIP_IO_TRACE)
#define pstore_ftrace_io_wr(reg, val)	\
unsigned long irqflg;					\
PSTORE_IO_SAVE(reg, val, 0, PSTORE_FLAG_IO_W, &irqflg)

#define pstore_ftrace_io_wr_end(reg, val)	\
PSTORE_IO_SAVE(reg, val, 0, PSTORE_FLAG_IO_W_END, &irqflg)

#define pstore_ftrace_io_rd(reg)		\
unsigned long irqflg;					\
PSTORE_IO_SAVE(reg, 0, 0, PSTORE_FLAG_IO_R, &irqflg)

#define pstore_ftrace_io_rd_end(reg)	\
PSTORE_IO_SAVE(reg, 0, 0, PSTORE_FLAG_IO_R_END, &irqflg)

#define pstore_ftrace_io_tag(reg, val)  \
PSTORE_IO_SAVE(reg, val, 0, PSTORE_FLAG_IO_TAG, NULL)

#define need_dump_iomap()               (ramoops_io_en | dump_iomap)

#define pstore_ftrace_sched_switch(next_pid, next_comm)	\
PSTORE_IO_SAVE(next_pid, next_comm, 0, PSTORE_FLAG_IO_SCHED_SWITCH, NULL)

#define pstore_ftrace_io_smc_in(a0, a1)	\
PSTORE_IO_SAVE(a0, a1, 0, PSTORE_FLAG_IO_SMC_IN, NULL)

#define pstore_ftrace_io_smc_out(a0, a1) \
PSTORE_IO_SAVE(a0, a1, 0, PSTORE_FLAG_IO_SMC_OUT, NULL)

#define pstore_ftrace_io_smc_noret_in(a0, a1)	\
PSTORE_IO_SAVE(a0, a1, 0, PSTORE_FLAG_IO_SMC_NORET_IN, NULL)

#define pstore_ftrace_clk_enable(name) \
PSTORE_IO_SAVE(0, name, 0, PSTORE_FLAG_CLK_ENABLE, NULL)

#define pstore_ftrace_clk_disable(name) \
PSTORE_IO_SAVE(0, name, 0, PSTORE_FLAG_CLK_DISABLE, NULL)

#define pstore_ftrace_pd_power_on(name) \
PSTORE_IO_SAVE(0, name, 0, PSTORE_FLAG_PD_POWER_ON, NULL)

#define pstore_ftrace_pd_power_off(name) \
PSTORE_IO_SAVE(0, name, 0, PSTORE_FLAG_PD_POWER_OFF, NULL)

#else
#define pstore_ftrace_io_wr(reg, val)                   do {    } while (0)
#define pstore_ftrace_io_rd(reg)                        do {    } while (0)
#define need_dump_iomap()                               0
#define pstore_ftrace_io_wr_end(reg, val)               do {    } while (0)
#define pstore_ftrace_io_rd_end(reg)                    do {    } while (0)
#define pstore_ftrace_io_tag(reg, val)                  do {    } while (0)
#define pstore_ftrace_sched_switch(next_pid, next_comm) do {	} while (0)
#define pstore_ftrace_io_smc_in(a0, a1)			do {	} while (0)
#define pstore_ftrace_io_smc_out(a0, a1)		do {	} while (0)
#define pstore_ftrace_io_smc_noret_in(a0, a1)		do {	} while (0)
#define pstore_ftrace_clk_enable(name)			do {	} while (0)
#define pstore_ftrace_clk_disable(name)			do {	} while (0)
#define pstore_ftrace_pd_power_on(name)			do {	} while (0)
#define pstore_ftrace_pd_power_off(name)		do {	} while (0)

#endif /*CONFIG_AMLOGIC_BGKI_DEBUG_IOTRACE && !SKIP_IO_TRACE */

#if IS_ENABLED(CONFIG_AMLOGIC_BGKI_DEBUG_IOTRACE)
#define pstore_ftrace_io_copy_from(reg, cnt)	\
unsigned long irqflg;                                   \
PSTORE_IO_SAVE(reg, cnt, 0, PSTORE_FLAG_IO_R, &irqflg)

#define pstore_ftrace_io_copy_from_end(reg, cnt)	\
PSTORE_IO_SAVE(reg, cnt, 0, PSTORE_FLAG_IO_R_END, &irqflg)

#define pstore_ftrace_io_copy_to(reg, cnt)	\
unsigned long irqflg;                                   \
PSTORE_IO_SAVE(reg, 0x12340000 | cnt, 0, PSTORE_FLAG_IO_W, &irqflg)

#define pstore_ftrace_io_copy_to_end(reg, cnt)		\
PSTORE_IO_SAVE(reg, 0x12340000 | cnt, 0, PSTORE_FLAG_IO_W_END, &irqflg)

#define pstore_ftrace_io_memset(reg, cnt)	\
unsigned long irqflg;					\
PSTORE_IO_SAVE(reg, 0xabcd0000 | cnt, 0, PSTORE_FLAG_IO_W, &irqflg)

#define pstore_ftrace_io_memset_end(reg, cnt)		\
PSTORE_IO_SAVE(reg, 0xabcd0000 | cnt, 0, PSTORE_FLAG_IO_W_END, &irqflg)
#else
#define pstore_ftrace_io_copy_from(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_copy_from_end(reg, cnt)	do {	} while (0)
#define pstore_ftrace_io_copy_to(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_copy_to_end(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_memset(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_memset_end(reg, cnt)		do {	} while (0)
#endif /* CONFIG_AMLOGIC_BGKI_DEBUG_IOTRACE */

#endif
