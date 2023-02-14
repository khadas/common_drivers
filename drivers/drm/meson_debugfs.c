// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

#include <linux/uaccess.h>
#include "meson_drv.h"
#include "meson_crtc.h"
#include "meson_plane.h"
#include "meson_debugfs.h"
#include "meson_vpu_pipeline.h"
#include <drm/amlogic/meson_connector_dev.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#define HDMITX_ATTR_LEN_MAX 16

#ifdef CONFIG_DEBUG_FS

static u8 *meson_drm_vmap(ulong addr, u32 size, bool *bflg)
{
	u8 *vaddr = NULL;
	ulong phys = addr;
	u32 offset = phys & ~PAGE_MASK;
	u32 npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = NULL;
	pgprot_t pgprot;
	int i;

	if (!PageHighMem(phys_to_page(phys)))
		return phys_to_virt(phys);

	if (offset)
		npages++;

	pages = kcalloc(npages, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;

	for (i = 0; i < npages; i++) {
		pages[i] = phys_to_page(phys);
		phys += PAGE_SIZE;
	}

	pgprot = PAGE_KERNEL;

	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	if (!vaddr) {
		pr_err("the phy(%lx) vmap fail, size: %d\n",
		       addr - offset, npages << PAGE_SHIFT);
		kfree(pages);
		return NULL;
	}

	kfree(pages);

	DRM_DEBUG("map high mem pa(%lx) to va(%p), size: %d\n",
		  addr, vaddr + offset, npages << PAGE_SHIFT);
	*bflg = true;

	return vaddr + offset;
}

static void meson_drm_unmap_phyaddr(u8 *vaddr)
{
	void *addr = (void *)(PAGE_MASK & (ulong)vaddr);

	DRM_DEBUG("unmap va(%p)\n", addr);
	vunmap(addr);
}

static ssize_t osd_fbdump_read(struct file *filp, char __user *buf,
				size_t size, loff_t *ppos)
{
	ssize_t count;
	struct drm_plane *plane = filp->private_data;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	count = simple_read_from_buffer(buf, size, ppos, amp->vir_addr,
					amp->dump_size);
	return count;
}

static int osd_fbdump_release(struct inode *inode, struct file *filp)
{
	struct drm_plane *plane = filp->private_data;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	if (amp->bflg)
		meson_drm_unmap_phyaddr(amp->vir_addr);

	return 0;
}

static int osd_fbdump_open(struct inode *inode, struct file *filp)
{
	bool bflg;
	u32 index;
	u32 fb_size;
	void *vir_addr;
	u64 phy_addr;
	struct meson_vpu_pipeline *pipeline;
	struct meson_vpu_osd_layer_info *info;
	struct meson_drm *priv;
	struct meson_vpu_pipeline_state *mvps;
	struct drm_plane *plane = inode->i_private;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	filp->private_data = inode->i_private;
	priv = plane->dev->dev_private;
	pipeline = priv->pipeline;
	mvps = priv_to_pipeline_state(pipeline->obj.state);

	index = plane->index;
	info = &mvps->plane_info[index];

	if (!info->enable) {
		DRM_INFO("osd is disabled\n");
		return 0;
	}

	phy_addr = info->phy_addr;
	fb_size = info->fb_size;
	bflg = false;
	vir_addr = meson_drm_vmap(phy_addr, fb_size, &bflg);
	amp->bflg = bflg;
	amp->vir_addr = vir_addr;
	amp->dump_size = fb_size;

	return 0;
}

static const struct file_operations meson_osd_fbdump_fops = {
	.owner = THIS_MODULE,
	.open = osd_fbdump_open,
	.read = osd_fbdump_read,
	.release = osd_fbdump_release,
};

static int meson_regdump_show(struct seq_file *sf, void *data)
{
	int i;
	struct meson_vpu_block *mvb;
	struct drm_crtc *crtc = sf->private;
	struct am_meson_crtc *amc = to_am_meson_crtc(crtc);
	struct meson_vpu_pipeline *mvp1 = amc->pipeline;

	for (i = 0; i < MESON_MAX_BLOCKS; i++) {
		mvb = mvp1->mvbs[i];
		if (!mvb)
			continue;

		seq_printf(sf, "*************%s*************\n", mvb->name);
		if (mvb->ops && mvb->ops->dump_register)
			mvb->ops->dump_register(mvb, sf);
	}
	return 0;
}

static int meson_regdump_open(struct inode *inode, struct file *file)
{
	struct drm_crtc *crtc = inode->i_private;

	return single_open(file, meson_regdump_show, crtc);
}

static const struct file_operations meson_regdump_fops = {
	.owner = THIS_MODULE,
	.open = meson_regdump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int meson_blank_show(struct seq_file *sf, void *data)
{
	struct drm_crtc *crtc = sf->private;
	struct am_meson_crtc *amc = to_am_meson_crtc(crtc);

	seq_puts(sf, "echo 1 > blank to blank the osd plane\n");
	seq_puts(sf, "echo 0 > blank to unblank the osd plane\n");
	seq_printf(sf, "blank_enable: %d\n", amc->blank_enable);
	return 0;
}

static int meson_blank_open(struct inode *inode, struct file *file)
{
	struct drm_crtc *crtc = inode->i_private;

	return single_open(file, meson_blank_show, crtc);
}

static ssize_t meson_blank_write(struct file *file, const char __user *ubuf,
				 size_t len, loff_t *offp)
{
	char buf[4];
	struct seq_file *sf = file->private_data;
	struct drm_crtc *crtc = sf->private;
	struct am_meson_crtc *amc = to_am_meson_crtc(crtc);

	if (len > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, ubuf, len))
		return -EFAULT;
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';
	buf[len] = '\0';

	if (strncmp(buf, "1", 1) == 0) {
		amc->blank_enable = 1;
		DRM_INFO("enable the osd blank\n");
	} else if (strncmp(buf, "0", 1) == 0) {
		amc->blank_enable = 0;
		DRM_INFO("disable the osd blank\n");
	}

	return len;
}

static const struct file_operations meson_blank_fops = {
	.owner = THIS_MODULE,
	.open = meson_blank_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = meson_blank_write,
};

void meson_crtc_debugfs_late_init(struct drm_crtc *crtc)
{
	debugfs_create_file("blank", 0644, crtc->debugfs_entry,
			    crtc, &meson_blank_fops);
}

static int meson_osd_reverse_show(struct seq_file *sf, void *data)
{
	struct drm_plane *plane = sf->private;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	seq_puts(sf, "echo 1/2/3 > osd_reverse :reverse the osd xy/x/y\n");
	seq_puts(sf, "echo 0 > osd_reverse to un_reverse the osd plane\n");
	seq_printf(sf, "osd_reverse: %d\n", amp->osd_reverse);
	return 0;
}

static int meson_osd_reverse_open(struct inode *inode, struct file *file)
{
	struct drm_plane *plane = inode->i_private;

	return single_open(file, meson_osd_reverse_show, plane);
}

static ssize_t meson_osd_reverse_write(struct file *file,
				       const char __user *ubuf,
				       size_t len, loff_t *offp)
{
	char buf[4];
	struct seq_file *sf = file->private_data;
	struct drm_plane *plane = sf->private;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	if (len > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, ubuf, len))
		return -EFAULT;
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';
	buf[len] = '\0';

	if (strncmp(buf, "1", 1) == 0) {
		amp->osd_reverse = DRM_MODE_REFLECT_MASK;
		DRM_INFO("enable the osd reverse\n");
	} else if (strncmp(buf, "2", 1) == 0) {
		amp->osd_reverse = DRM_MODE_REFLECT_X;
		DRM_INFO("enable the osd reverse_x\n");
	} else if (strncmp(buf, "3", 1) == 0) {
		amp->osd_reverse = DRM_MODE_REFLECT_Y;
		DRM_INFO("enable the osd reverse_y\n");
	} else if (strncmp(buf, "0", 1) == 0) {
		amp->osd_reverse = 0;
		DRM_INFO("disable the osd reverse\n");
	}

	return len;
}

static const struct file_operations meson_osd_reverse_fops = {
	.owner = THIS_MODULE,
	.open = meson_osd_reverse_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = meson_osd_reverse_write,
};

static int meson_osd_blend_bypass_show(struct seq_file *sf, void *data)
{
	struct drm_plane *plane = sf->private;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	seq_puts(sf, "echo 1/0 > osd_blend_bypass :enable/disable\n");
	seq_printf(sf, "osd_blend_bypass: %d\n", amp->osd_blend_bypass);
	return 0;
}

static int meson_osd_blend_bypass_open(struct inode *inode, struct file *file)
{
	struct drm_plane *plane = inode->i_private;

	return single_open(file, meson_osd_blend_bypass_show, plane);
}

static ssize_t meson_osd_blend_bypass_write(struct file *file,
					    const char __user *ubuf,
					    size_t len, loff_t *offp)
{
	char buf[4];
	struct seq_file *sf = file->private_data;
	struct drm_plane *plane = sf->private;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	if (len > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, ubuf, len))
		return -EFAULT;
	if (buf[len - 1] == '\n')
		buf[len - 1] = '\0';
	buf[len] = '\0';

	if (strncmp(buf, "1", 1) == 0) {
		amp->osd_blend_bypass = 1;
		DRM_INFO("enable the osd blend bypass\n");
	} else if (strncmp(buf, "0", 1) == 0) {
		amp->osd_blend_bypass = 0;
		DRM_INFO("disable the osd blend bypass\n");
	}

	return len;
}

static const struct file_operations meson_osd_blend_bypass_fops = {
	.owner = THIS_MODULE,
	.open = meson_osd_blend_bypass_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = meson_osd_blend_bypass_write,
};

static int meson_osd_read_port_show(struct seq_file *sf, void *data)
{
	struct drm_plane *plane = sf->private;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	seq_puts(sf, "echo 1 > enable read port setting\n");
	seq_puts(sf, "echo 0 > disable read port setting\n");
	seq_printf(sf, "\nstatus：%d\n", (amp->osd_read_ports == 1) ? 1 : 0);

	return 0;
}

static int meson_osd_read_port_open(struct inode *inode, struct file *file)
{
	struct drm_plane *plane = inode->i_private;

	return single_open(file, meson_osd_read_port_show, plane);
}

static ssize_t meson_osd_read_port_write(struct file *file,
				       const char __user *ubuf,
				       size_t len, loff_t *offp)
{
	char buf[16];
	long val;
	struct seq_file *sf = file->private_data;
	struct drm_plane *plane = sf->private;
	struct am_osd_plane *amp = to_am_osd_plane(plane);

	if (len > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, ubuf, len))
		return -EFAULT;

	buf[len] = '\0';

	if (kstrtoul(buf, 16, &val) < 0)
		return -EINVAL;

	val = val >= 1 ? 1 : 0;
	amp->osd_read_ports = val;

	return len;
}

static const struct file_operations meson_osd_read_port_fops = {
	.owner = THIS_MODULE,
	.open = meson_osd_read_port_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = meson_osd_read_port_write,
};

u32 overwrite_reg[256];
u32 overwrite_val[256];
int overwrite_enable;
int reg_num;

static int meson_reg_debug_show(struct seq_file *sf, void *data)
{
	int i;

	seq_puts(sf, "echo rv reg > debug to read the register\n");
	seq_puts(sf, "echo wv reg val > debug to overwrite the register\n");
	seq_puts(sf, "echo ow 1 > debug to enable overwrite register\n");
	seq_printf(sf, "\noverwrote status: %s\n", overwrite_enable ? "on" : "off");
	if (overwrite_enable) {
		for (i = 0; i < reg_num; i++)
			seq_printf(sf, "reg[0x%04x]=0x%08x\n", overwrite_reg[i],
				   overwrite_val[i]);
	}
	//seq_printf(sf, "blank_enable: %d\n", amc->blank_enable);
	return 0;
}

static int meson_reg_debug_open(struct inode *inode, struct file *file)
{
	struct drm_crtc *crtc = inode->i_private;

	return single_open(file, meson_reg_debug_show, crtc);
}

static void parse_param(char *buf_orig, char **parm)
{
	char *ps, *token;
	unsigned int n = 0;
	char delim1[3] = " ";
	char delim2[2] = "\n";

	ps = buf_orig;
	strcat(delim1, delim2);
	while (1) {
		token = strsep(&ps, delim1);
		if (!token)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}
}

static ssize_t meson_reg_debug_write(struct file *file, const char __user *ubuf,
					size_t len, loff_t *offp)
{
	char buf[64];
	long val;
	int i;
	unsigned int reg_addr, reg_val;
	char *bufp, *parm[8] = {NULL};

	if (len > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, ubuf, len))
		return -EFAULT;

	buf[len] = '\0';

	bufp = buf;
	parse_param(bufp, (char **)&parm);
	if (!strcmp(parm[0], "rv")) {
		if (kstrtoul(parm[1], 16, &val) < 0)
			return -EINVAL;

		reg_addr = val;
		DRM_INFO("reg[0x%04x]=0x%08x\n", reg_addr, meson_drm_read_reg(reg_addr));
	} else if (!strcmp(parm[0], "wv")) {
		if (kstrtoul(parm[1], 16, &val) < 0)
			return -EINVAL;
		reg_addr = val;

		if (kstrtoul(parm[2], 16, &val) < 0)
			return -EINVAL;

		reg_val = val;
		for (i = 0; i < reg_num; i++) {
			if (overwrite_reg[i] == reg_addr) {
				overwrite_val[i] = reg_val;
				return len;
			}
		}

		if (i == reg_num) {
			overwrite_reg[i] = reg_addr;
			overwrite_val[i] = reg_val;
			reg_num++;
		}
	} else if (!strcmp(parm[0], "ow")) {
		if (parm[1] && !strcmp(parm[1], "1"))
			overwrite_enable = 1;
		else if (parm[1] && !strcmp(parm[1], "0")) {
			overwrite_enable = 0;
			for (i = 0; i < reg_num; i++) {
				overwrite_val[i] = 0;
				overwrite_val[i] = 0;
			}
			reg_num = 0;
		}
	}

	return len;
}

static const struct file_operations meson_reg_debug_fops = {
	.owner = THIS_MODULE,
	.open = meson_reg_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = meson_reg_debug_write,
};

int meson_crtc_debugfs_init(struct drm_crtc *crtc, struct dentry *root)
{
	struct dentry *meson_vpu_root;
	struct dentry *entry;

	meson_vpu_root = debugfs_create_dir("vpu", root);

	entry = debugfs_create_file("reg_dump", 0400, meson_vpu_root, crtc,
				    &meson_regdump_fops);
	if (!entry) {
		DRM_ERROR("create reg_dump node error\n");
		debugfs_remove_recursive(meson_vpu_root);
	}

	entry = debugfs_create_file("blank", 0644, meson_vpu_root, crtc,
				    &meson_blank_fops);
	if (!entry) {
		DRM_ERROR("create blank node error\n");
		debugfs_remove_recursive(meson_vpu_root);
	}

	entry = debugfs_create_file("debug", 0644, meson_vpu_root, crtc,
					&meson_reg_debug_fops);
	if (!entry) {
		DRM_ERROR("create reg_debug node error\n");
		debugfs_remove_recursive(meson_vpu_root);
	}

	return 0;
}

int meson_plane_debugfs_init(struct drm_plane *plane, struct dentry *root)
{
	struct dentry *meson_vpu_root;
	struct dentry *entry;

	meson_vpu_root = debugfs_create_dir(plane->name, root);

	entry = debugfs_create_file("osd_reverse", 0644, meson_vpu_root, plane,
				    &meson_osd_reverse_fops);
	if (!entry) {
		DRM_ERROR("create osd_reverse node error\n");
		debugfs_remove_recursive(meson_vpu_root);
	}

	entry = debugfs_create_file("osd_blend_bypass", 0644,
				    meson_vpu_root, plane,
				    &meson_osd_blend_bypass_fops);
	if (!entry) {
		DRM_ERROR("create osd_blend_bypass node error\n");
		debugfs_remove_recursive(meson_vpu_root);
	}

	entry = debugfs_create_file("osd_read_port", 0644,
				    meson_vpu_root, plane,
				    &meson_osd_read_port_fops);
	if (!entry) {
		DRM_ERROR("create osd_blend_bypass node error\n");
		debugfs_remove_recursive(meson_vpu_root);
	}

	entry = debugfs_create_file("fbdump", 0644,
				    meson_vpu_root, plane,
				    &meson_osd_fbdump_fops);

	if (!entry) {
		DRM_ERROR("create osd fbdump node error\n");
		debugfs_remove_recursive(meson_vpu_root);
	}

	return 0;
}

static int mm_show(struct seq_file *sf, void *arg)
{
	return 0;
}

static struct drm_info_list meson_debugfs_list[] = {
	{"mm", mm_show, 0},
};

void meson_debugfs_init(struct drm_minor *minor)
{
	struct drm_crtc *crtc;
	struct drm_plane *plane;
	struct drm_device *dev = minor->dev;

	drm_debugfs_create_files(meson_debugfs_list,
				       ARRAY_SIZE(meson_debugfs_list),
					minor->debugfs_root, minor);

	drm_for_each_crtc(crtc, dev) {
		if (!crtc->index)
			meson_crtc_debugfs_init(crtc, minor->debugfs_root);
	}

	drm_for_each_plane(plane, dev) {
		meson_plane_debugfs_init(plane, minor->debugfs_root);
	}
}

#endif
