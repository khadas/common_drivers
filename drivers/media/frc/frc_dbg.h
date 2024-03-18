/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifndef __FRC_DBG_H__
#define __FRC_DBG_H__

extern const char * const frc_state_ary[];
extern u32 g_input_hsize;
extern u32 g_input_vsize;
extern int frc_dbg_en;
extern int frc_dbg_ctrl;

ssize_t frc_reg_show(struct class *class, struct class_attribute *attr, char *buf);
ssize_t frc_reg_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count);
ssize_t frc_tool_debug_show(struct class *class,
	struct class_attribute *attr, char *buf);
ssize_t frc_tool_debug_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count);
ssize_t frc_debug_show(struct class *class,
	struct class_attribute *attr, char *buf);
ssize_t frc_debug_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count);
ssize_t frc_buf_show(struct class *class,
	struct class_attribute *attr,
	char *buf);
ssize_t frc_buf_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count);
ssize_t frc_rdma_show(struct class *class,
	struct class_attribute *attr, char *buf);
ssize_t frc_rdma_store(struct class *class,
	struct class_attribute *attr,
	const char *buf, size_t count);
ssize_t frc_param_show(struct class *class,
	struct class_attribute *attr, char *buf);
ssize_t frc_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf, size_t count);
ssize_t frc_other_show(struct class *class,
	struct class_attribute *attr, char *buf);
ssize_t frc_other_store(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count);
void frc_power_domain_ctrl(struct frc_dev_s *devp, u32 onoff);
void frc_debug_if(struct frc_dev_s *frc_devp, const char *buf, size_t count);
ssize_t frc_debug_if_help(struct frc_dev_s *devp, char *buf);
void frc_reg_io(const char *buf);
void frc_tool_dbg_store(struct frc_dev_s *devp, const char *buf);
void frc_dump_buf_data(struct frc_dev_s *devp, u32 cma_addr, u32 size);

ssize_t frc_debug_buf_if_help(struct frc_dev_s *devp, char *buf);
void frc_debug_buf_if(struct frc_dev_s *devp, const char *buf, size_t count);
ssize_t frc_debug_rdma_if_help(struct frc_dev_s *devp, char *buf);
void frc_debug_rdma_if(struct frc_dev_s *devp, const char *buf, size_t count);
ssize_t frc_debug_param_if_help(struct frc_dev_s *devp, char *buf);
void frc_debug_param_if(struct frc_dev_s *devp, const char *buf, size_t count);
ssize_t frc_debug_other_if_help(struct frc_dev_s *devp, char *buf);
void frc_debug_other_if(struct frc_dev_s *devp, const char *buf, size_t count);
void frc_timer_proc(struct frc_dev_s *devp);
void frc_dbg_frame_show(struct frc_dev_s *devp);
void set_frc_config(const char *module, const char *debug, int len);

#endif
