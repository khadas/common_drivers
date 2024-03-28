/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#define VPU_READ0   0
#define VPU_READ1   1
#define VPU_READ2   2
#define VPU_WRITE0  3
#define VPU_WRITE1  4
#define NO_PORT     5

extern unsigned int vpu_print_level;

#define MODULE_ARB_MODE    BIT(0)
#define MODULE_ARB_URGENT  BIT(1)

#define vpu_pr_debug(module, fmt, ...) \
	do { \
		if ((module) & vpu_print_level) \
			pr_info(fmt, ##__VA_ARGS__);\
	} while (0)

struct vpu_arb_table_s {
	enum vpu_arb_mod_e vmod;
	unsigned int reg;
	//unsigned int val;
	unsigned int bit;
	unsigned int len;
	unsigned int bind_port;
	char *name;
	unsigned int reqen_slv_reg;
	unsigned int reqen_slv_bit;
	unsigned int reqen_slv_len;
};

struct vpu_urgent_table_s {
	enum vpu_arb_mod_e vmod;
	unsigned int reg;
	unsigned int port;
	unsigned int val;
	unsigned int start_bit;
	unsigned int len;
	char *name;
};

struct vpu_arb_info {
	struct mutex vpu_arb_lock;/*vpu arb mutex*/
	unsigned char registered;
	void (*arb_cb)(enum vpu_arb_mod_e module,
		u32 pre_urgent_value, u32 adjust_urgent_value);
};

int vpu_rdarb0_2_bind_l1(enum vpu_arb_mod_e level1_module, enum vpu_arb_mod_e level2_module);
int vpu_rdarb0_2_bind_l2(enum vpu_arb_mod_e level2_module, u32 vpu_read_port);
int vpu_urgent_set(enum vpu_arb_mod_e vmod, u32 urgent_value);
void print_bind1_change_info(enum vpu_arb_mod_e level1_module,
					enum vpu_arb_mod_e level2_module);
void print_bind2_change_info(enum vpu_arb_mod_e level1_module,
					enum vpu_arb_mod_e level2_module);
int vpu_urgent_get(enum vpu_arb_mod_e vmod);
void dump_vpu_rdarb_table(void);
void dump_vpu_wrarb_table(void);
void dump_vpu_clk1_rdarb_table(void);
void dump_vpu_clk2_rdarb_table(void);
void dump_vpu_clk1_wrarb_table(void);
void dump_vpu_clk2_wrarb_table(void);
void dump_vpu_urgent_table(u32 vpu_port);
int init_arb_urgent_table(void);
void dump_vpu_rdarb_table(void);
void get_rdarb0_2_module_info(void);
void get_rdarb1_module_info(void);
void get_wrarb0_module_info(void);
void get_wrarb1_module_info(void);
