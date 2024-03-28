/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_IOTRACE_H
#define __AML_IOTRACE_H

extern int ramoops_io_skip;
extern int ramoops_ftrace_en;
extern int ramoops_trace_mask;
DECLARE_PER_CPU(bool, vsync_iotrace_cut);
DECLARE_PER_CPU(bool, frc_iotrace_cut);
DECLARE_PER_CPU(bool, amvecm_iotrace_cut);
DECLARE_PER_CPU(bool, usb_iotrace_cut);

#define TRACE_MASK_IO		0x1
#define TRACE_MASK_SCHED	0x2
#define TRACE_MASK_IRQ		0x4
#define TRACE_MASK_SMC		0x8
#define TRACE_MASK_MISC		0x10

void notrace __nocfi pstore_io_save(unsigned long reg, unsigned long val, unsigned int flag,
							unsigned long *irq_flags);

#ifdef CONFIG_SHADOW_CALL_STACK
unsigned long get_prev_lr_val(unsigned long lr, unsigned long offset);
#endif

enum iotrace_record_type_id {
	RECORD_TYPE_IO_R		= 0x0,
	RECORD_TYPE_IO_W		= 0x1,
	RECORD_TYPE_IO_R_END		= 0x2,
	RECORD_TYPE_IO_W_END		= 0x3,
	RECORD_TYPE_SCHED_SWITCH	= 0x4,
	RECORD_TYPE_ISR_IN		= 0x5,
	RECORD_TYPE_ISR_OUT		= 0x6,
	RECORD_TYPE_SMC_IN		= 0x7,
	RECORD_TYPE_SMC_OUT		= 0x8,
	RECORD_TYPE_SMC_NORET_IN	= 0x9,
	RECORD_TYPE_USB_IN		= 0xa,
	RECORD_TYPE_USB_OUT		= 0xb,
	RECORD_TYPE_FRC_INPUT_IN	= 0xc,
	RECORD_TYPE_FRC_INPUT_OUT	= 0xd,
	RECORD_TYPE_FRC_OUTPUT_IN	= 0xe,
	RECORD_TYPE_FRC_OUTPUT_OUT	= 0xf,
	RECORD_TYPE_VSYNC_IN		= 0x10,
	RECORD_TYPE_VSYNC_OUT		= 0x11,
	RECORD_TYPE_AMVECM_IN		= 0x12,
	RECORD_TYPE_AMVECM_OUT		= 0x13,
	RECORD_TYPE_MASK		= 0xff
};

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
#define pstore_ftrace_io_wr(reg, val, irqflg)	\
pstore_io_save(reg, val, RECORD_TYPE_IO_W, &(irqflg))

#define pstore_ftrace_io_wr_end(reg, val, irqflg)	\
pstore_io_save(reg, val, RECORD_TYPE_IO_W_END, &(irqflg))

#define pstore_ftrace_io_rd(reg, irqflg)		\
pstore_io_save(reg, 0, RECORD_TYPE_IO_R, &(irqflg))

#define pstore_ftrace_io_rd_end(reg, val, irqflg)	\
pstore_io_save(reg, val, RECORD_TYPE_IO_R_END, &(irqflg))

#else
#define pstore_ftrace_io_wr(reg, val, irqflg)                   do {    } while (0)
#define pstore_ftrace_io_rd(reg, irqflg)                       do {    } while (0)
#define pstore_ftrace_io_wr_end(reg, val, irqflg)               do {    } while (0)
#define pstore_ftrace_io_rd_end(reg, val, irqflg)               do {    } while (0)

#endif /* CONFIG_AMLOGIC_DEBUG_IOTRACE */

enum aml_pstore_type_id {
	AML_PSTORE_TYPE_IO      = 0,
	AML_PSTORE_TYPE_SCHED   = 1,
	AML_PSTORE_TYPE_IRQ     = 2,
	AML_PSTORE_TYPE_SMC     = 3,
	AML_PSTORE_TYPE_MISC    = 4,
	AML_PSTORE_TYPE_MAX
};

struct iotrace_record {
	u16 magic;		/* 0xabcd */
	u16 pid;
	u8 cpu;
	struct {
		u8 in_irq:1;
		u8 in_softirq:1;
		u8 irq_disabled:1;
		u8 reserved:5;
	} flag;
	u16 type;
	u64 time;
	union {
		struct {
			unsigned int reg;
			unsigned int reg_val;
			unsigned long ip;
			unsigned long parent_ip;
		};	//io
		struct {
			char curr_comm[10];
			char next_comm[10];
			u16 curr_pid;
			u16 next_pid;
		};	//sched
		struct {
			unsigned int irq;
		};	//irq
		struct {
			unsigned long smcid;
			unsigned long val;
		};	//smc
		struct {
			unsigned long misc_data_1;
			unsigned long misc_data_2;
			unsigned long misc_data_3;
		};	//misc
	};
};

void aml_pstore_write(enum aml_pstore_type_id type, struct iotrace_record *rec,
			unsigned int dis_irq, unsigned int io_flag);

void iotrace_misc_record_write(enum iotrace_record_type_id type, unsigned long misc_data_1,
			unsigned long misc_data_2, unsigned long misc_data_3);

int ftrace_ramoops_init(void);
#endif

