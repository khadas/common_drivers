// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/seq_file.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_modeset_lock.h>
#include <linux/kernel.h>

#include "meson_sysfs.h"
#include "meson_crtc.h"
#include "meson_plane.h"
#include "meson_vpu_pipeline.h"

static const char vpu_group_name[] = "vpu";
static const char osd0_group_name[] = "osd0";
static const char osd1_group_name[] = "osd1";
static const char osd2_group_name[] = "osd2";
static const char osd3_group_name[] = "osd3";
int osd_index[MESON_MAX_OSDS] = {0, 1, 2, 3};
u32 pages;
//EXPORT_SYMBOL_GPL(vpu_group_name);

static u8 *am_meson_drm_vmap(ulong addr, u32 size, bool *bflg)
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

static void am_meson_drm_unmap_phyaddr(u8 *vaddr)
{
	void *addr = (void *)(PAGE_MASK & (ulong)vaddr);

	DRM_DEBUG("unmap va(%p)\n", addr);
	vunmap(addr);
}

static ssize_t vpu_blank_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t len = 0;
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct drm_crtc *crtc;
	struct am_meson_crtc *amc;

	if (!minor || !minor->dev)
		return -EINVAL;

	crtc = drm_crtc_from_index(minor->dev, 0);
	if (!crtc)
		return -EINVAL;

	amc = to_am_meson_crtc(crtc);

	len += scnprintf(&buf[len], PAGE_SIZE - len, "%s\n",
			"echo 1 > vpu_blank to blank the osd plane");
	len += scnprintf(&buf[len], PAGE_SIZE - len, "%s\n",
			"echo 0 > vpu_blank to unblank the osd plane");
	len += scnprintf(&buf[len], PAGE_SIZE - len,
			"blank_enable: %d\n", amc->blank_enable);

	return len;
}

static ssize_t vpu_blank_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct drm_crtc *crtc;
	struct am_meson_crtc *amc;

	if (!minor || !minor->dev)
		return -EINVAL;

	crtc = drm_crtc_from_index(minor->dev, 0);
	if (!crtc)
		return -EINVAL;

	amc = to_am_meson_crtc(crtc);

	if (sysfs_streq(buf, "1")) {
		amc->blank_enable = 1;
		DRM_INFO("enable the osd blank\n");
	} else if (sysfs_streq(buf, "0")) {
		amc->blank_enable = 0;
		DRM_INFO("disable the osd blank\n");
	} else {
		return -EINVAL;
	}

	return n;
}

static ssize_t debug_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int i, pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo rv reg > debug to read the register\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo wv reg val > debug to overwrite the register\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo ow 1 > debug to enable overwrite register\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"\noverwrote status: %s\n", overwrite_enable ? "on" : "off");

	if (overwrite_enable) {
		for (i = 0; i < reg_num; i++)
			pos += snprintf(buf + pos, PAGE_SIZE - pos,
			"reg[0x%04x]=0x%08x\n", overwrite_reg[i], overwrite_val[i]);
	}

	return pos;
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

static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	char dst_buf[64];
	long val;
	int i;
	unsigned int reg_addr, reg_val;
	char *bufp, *parm[8] = {NULL};
	int len = strlen(buf);

	if (len > sizeof(dst_buf) - 1)
		return -EINVAL;

	memcpy(dst_buf, buf, len);

	dst_buf[len] = '\0';
	bufp = dst_buf;
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
		if (parm[1] && !strcmp(parm[1], "1")) {
			overwrite_enable = 1;
		} else if (parm[1] && !strcmp(parm[1], "0")) {
			overwrite_enable = 0;
			for (i = 0; i < reg_num; i++) {
				overwrite_val[i] = 0;
				overwrite_val[i] = 0;
			}
			reg_num = 0;
		}
	}

	return n;
}

static DEVICE_ATTR_RW(vpu_blank);
static DEVICE_ATTR_RW(debug);

static struct attribute *vpu_attrs[] = {
	&dev_attr_vpu_blank.attr,
	&dev_attr_debug.attr,
	NULL,
};

static const struct attribute_group vpu_attr_group = {
	.name	= vpu_group_name,
	.attrs	= vpu_attrs,
};

static ssize_t osd_reverse_show(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct meson_drm *priv;
	struct am_osd_plane *amp;
	int pos = 0;

	if (!minor || !minor->dev)
		return -EINVAL;
	if (off > 0)
		return 0;

	priv = minor->dev->dev_private;
	amp = priv->osd_planes[*(int *)attr->private];

	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo 1/2/3 > osd_reverse :reverse the osd xy/x/y\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo 0 > osd_reverse to un_reverse the osd plane\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"osd_reverse: %d\n", amp->osd_reverse);

	return pos;
}

static ssize_t osd_reverse_store(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	int idx = *(int *)attr->private;
	struct meson_drm *priv;
	struct am_osd_plane *amp;

	if (!minor || !minor->dev)
		return -EINVAL;
	priv = minor->dev->dev_private;
	amp = priv->osd_planes[idx];

	if (sysfs_streq(buf, "0")) {
		amp->osd_reverse = 0;
		DRM_INFO("disable the osd reverse\n");
	} else if (sysfs_streq(buf, "1")) {
		amp->osd_reverse = DRM_MODE_REFLECT_MASK;
		DRM_INFO("enable the osd reverse\n");
	} else if (sysfs_streq(buf, "2")) {
		amp->osd_reverse = DRM_MODE_REFLECT_X;
		DRM_INFO("enable the osd reverse_x\n");
	} else if (sysfs_streq(buf, "3")) {
		amp->osd_reverse = DRM_MODE_REFLECT_Y;
		DRM_INFO("enable the osd reverse_y\n");
	} else {
		return -EINVAL;
	}

	return count;
}

static ssize_t osd_blend_bypass_show(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct meson_drm *priv;
	struct am_osd_plane *amp;
	int pos = 0;

	if (!minor || !minor->dev)
		return -EINVAL;
	if (off > 0)
		return 0;

	priv = minor->dev->dev_private;
	amp = priv->osd_planes[*(int *)attr->private];

	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo 1/0 > osd_blend_bypass :enable/disable\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"osd_blend_bypass: %d\n", amp->osd_blend_bypass);

	return pos;
}

static ssize_t osd_blend_bypass_store(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct meson_drm *priv;
	struct am_osd_plane *amp;

	if (!minor || !minor->dev)
		return -EINVAL;

	priv = minor->dev->dev_private;
	amp = priv->osd_planes[*(int *)attr->private];

	if (sysfs_streq(buf, "1")) {
		amp->osd_blend_bypass = 1;
		DRM_INFO("enable the osd blend bypass\n");
	} else if (sysfs_streq(buf, "0")) {
		amp->osd_blend_bypass = 0;
		DRM_INFO("disable the osd blend bypass\n");
	}

	return count;
}

static ssize_t osd_read_port_show(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct meson_drm *priv;
	struct am_osd_plane *amp;
	int pos = 0;

	if (!minor || !minor->dev)
		return -EINVAL;
	if (off > 0)
		return 0;

	priv = minor->dev->dev_private;
	amp = priv->osd_planes[*(int *)attr->private];

	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo 1 > enable read port setting\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"echo 0 > disable read port setting\n");
	pos += snprintf(buf + pos, PAGE_SIZE - pos,
		"\nstatus：%d\n", (amp->osd_read_ports == 1) ? 1 : 0);

	return pos;
}

static ssize_t osd_read_port_store(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct meson_drm *priv;
	struct am_osd_plane *amp;
	long val;

	if (!minor || !minor->dev)
		return -EINVAL;

	priv = minor->dev->dev_private;
	amp = priv->osd_planes[*(int *)attr->private];

	if (kstrtoul(buf, 16, &val) < 0)
		return -EINVAL;

	val = val >= 1 ? 1 : 0;
	amp->osd_read_ports = val;

	return count;
}

static ssize_t osd_fbdump_show(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct meson_drm *priv;
	struct am_osd_plane *amp;
	bool bflg;
	u32 fb_size;
	void *vir_addr;
	u64 phy_addr;
	struct meson_vpu_pipeline *pipeline;
	struct meson_vpu_osd_layer_info *info;
	struct meson_vpu_pipeline_state *mvps;
	u32 num_pages;

	if (!minor || !minor->dev)
		return -EINVAL;

	priv = minor->dev->dev_private;
	amp = priv->osd_planes[*(int *)attr->private];
	pipeline = priv->pipeline;
	mvps = priv_to_pipeline_state(pipeline->obj.state);
	info = &mvps->plane_info[*(int *)attr->private];

	if (!info->enable) {
		DRM_INFO("osd is disabled\n");
		return 0;
	}

	phy_addr = info->phy_addr;
	fb_size = info->fb_size;
	bflg = false;
	if (pages == 0 && off < fb_size) {
		vir_addr = am_meson_drm_vmap(phy_addr, fb_size, &bflg);
		amp->bflg = bflg;
		amp->vir_addr = vir_addr;
		amp->dump_size = fb_size;
	}
	if (!amp->vir_addr) {
		DRM_INFO("vmap failed, vir_addr is null\n");
		return -EINVAL;
	}
	num_pages = PAGE_ALIGN(amp->dump_size) / PAGE_SIZE;
	pages++;

	if (pages <= num_pages && off < amp->dump_size) {
		memcpy(buf, amp->vir_addr + off, count);
		if (pages == num_pages && amp->bflg)
			am_meson_drm_unmap_phyaddr(amp->vir_addr);
		return count;
	}

	if (off >= amp->dump_size)
		pages = 0;

	return 0;
}

static ssize_t osd_fbdump_store(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	return count;
}

static ssize_t state_show(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct drm_print_iterator iter;
	struct drm_printer p;
	ssize_t ret;

	iter.data = buf;
	iter.start = off;
	iter.remain = count;

	p = drm_coredump_printer(&iter);
	drm_state_dump(minor->dev, &p);
	ret = count - iter.remain;
	return ret;
}

static ssize_t reg_dump_show(struct file *filp, struct kobject *kobj,
			 struct bin_attribute *attr, char *buf, loff_t off,
			 size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct drm_minor *minor = dev_get_drvdata(dev);
	struct drm_print_iterator iter;
	struct drm_printer p;
	struct meson_drm *priv;
	struct am_meson_crtc *amc;
	struct meson_vpu_pipeline *mvp1;
	struct meson_vpu_block *mvb;
	int i;
	ssize_t ret;

	iter.data = buf;
	iter.start = off;
	iter.remain = count;

	p = drm_coredump_printer(&iter);

	if (!minor || !minor->dev)
		return -EINVAL;

	priv = minor->dev->dev_private;
	amc = priv->crtcs[0];
	mvp1 = amc->pipeline;

	for (i = 0; i < MESON_MAX_BLOCKS; i++) {
		mvb = mvp1->mvbs[i];
		if (!mvb)
			continue;

		drm_printf(&p, "*************%s*************\n", mvb->name);
		if (mvb->ops && mvb->ops->dump_register)
			mvb->ops->dump_register(&p, mvb);
	}
	ret = count - iter.remain;

	return ret;
}

static struct bin_attribute osd0_attr[] = {
	{
		.attr.name = "osd_reverse",
		.attr.mode = 0664,
		.private = &osd_index[0],
		.read = osd_reverse_show,
		.write = osd_reverse_store,
	},
	{
		.attr.name = "osd_blend_bypass",
		.attr.mode = 0664,
		.private = &osd_index[0],
		.read = osd_blend_bypass_show,
		.write = osd_blend_bypass_store,
	},
	{
		.attr.name = "osd_read_port",
		.attr.mode = 0664,
		.private = &osd_index[0],
		.read = osd_read_port_show,
		.write = osd_read_port_store,
	},
	{
		.attr.name = "fbdump",
		.attr.mode = 0664,
		.private = &osd_index[0],
		.read = osd_fbdump_show,
		.write = osd_fbdump_store,
		.size = 36864000,
	},
};

static struct bin_attribute *osd0_bin_attrs[] = {
	&osd0_attr[0],
	&osd0_attr[1],
	&osd0_attr[2],
	&osd0_attr[3],
	NULL,
};

static struct bin_attribute osd1_attr[] = {
	{
		.attr.name = "osd_reverse",
		.attr.mode = 0664,
		.private = &osd_index[1],
		.read = osd_reverse_show,
		.write = osd_reverse_store,
	},
	{
		.attr.name = "osd_blend_bypass",
		.attr.mode = 0664,
		.private = &osd_index[1],
		.read = osd_blend_bypass_show,
		.write = osd_blend_bypass_store,
	},
	{
		.attr.name = "osd_read_port",
		.attr.mode = 0664,
		.private = &osd_index[1],
		.read = osd_read_port_show,
		.write = osd_read_port_store,
	},
	{
		.attr.name = "fbdump",
		.attr.mode = 0664,
		.private = &osd_index[1],
		.read = osd_fbdump_show,
		.write = osd_fbdump_store,
		.size = 36864000,
	},
};

static struct bin_attribute *osd1_bin_attrs[] = {
	&osd1_attr[0],
	&osd1_attr[1],
	&osd1_attr[2],
	&osd1_attr[3],
	NULL,
};

static struct bin_attribute osd2_attr[] = {
	{
		.attr.name = "osd_reverse",
		.attr.mode = 0664,
		.private = &osd_index[2],
		.read = osd_reverse_show,
		.write = osd_reverse_store,
	},
	{
		.attr.name = "osd_blend_bypass",
		.attr.mode = 0664,
		.private = &osd_index[2],
		.read = osd_blend_bypass_show,
		.write = osd_blend_bypass_store,
	},
	{
		.attr.name = "osd_read_port",
		.attr.mode = 0664,
		.private = &osd_index[2],
		.read = osd_read_port_show,
		.write = osd_read_port_store,
	},
	{
		.attr.name = "fbdump",
		.attr.mode = 0664,
		.private = &osd_index[2],
		.read = osd_fbdump_show,
		.write = osd_fbdump_store,
		.size = 36864000,
	},
};

static struct bin_attribute *osd2_bin_attrs[] = {
	&osd2_attr[0],
	&osd2_attr[1],
	&osd2_attr[2],
	&osd2_attr[3],
	NULL,
};

static struct bin_attribute osd3_attr[] = {
	{
		.attr.name = "osd_reverse",
		.attr.mode = 0664,
		.private = &osd_index[3],
		.read = osd_reverse_show,
		.write = osd_reverse_store,
	},
	{
		.attr.name = "osd_blend_bypass",
		.attr.mode = 0664,
		.private = &osd_index[3],
		.read = osd_blend_bypass_show,
		.write = osd_blend_bypass_store,
	},
	{
		.attr.name = "osd_read_port",
		.attr.mode = 0664,
		.private = &osd_index[3],
		.read = osd_read_port_show,
		.write = osd_read_port_store,
	},
	{
		.attr.name = "fbdump",
		.attr.mode = 0664,
		.private = &osd_index[3],
		.read = osd_fbdump_show,
		.write = osd_fbdump_store,
		.size = 36864000,
	},
};

static struct bin_attribute *osd3_bin_attrs[] = {
	&osd3_attr[0],
	&osd3_attr[1],
	&osd3_attr[2],
	&osd3_attr[3],
	NULL,
};

static const struct attribute_group osd_attr_group[MESON_MAX_OSDS] = {
	{
		.name = osd0_group_name,
		.bin_attrs = osd0_bin_attrs,
	},
	{
		.name = osd1_group_name,
		.bin_attrs = osd1_bin_attrs,
	},
	{
		.name = osd2_group_name,
		.bin_attrs = osd2_bin_attrs,
	},
	{
		.name = osd3_group_name,
		.bin_attrs = osd3_bin_attrs,
	},
};

static struct bin_attribute state_attr = {
	.attr.name = "state",
	.attr.mode = 0664,
	.read = state_show,
};

static struct bin_attribute reg_dump_attr = {
	.attr.name = "reg_dump",
	.attr.mode = 0664,
	.read = reg_dump_show,
};

int meson_drm_sysfs_register(struct drm_device *drm_dev)
{
	int rc, i;
	struct meson_drm *priv = drm_dev->dev_private;
	struct device *dev = drm_dev->primary->kdev;

	rc = sysfs_create_group(&dev->kobj, &vpu_attr_group);

	rc = sysfs_create_bin_file(&dev->kobj, &state_attr);
	rc = sysfs_create_bin_file(&dev->kobj, &reg_dump_attr);

	for (i = 0; i < priv->pipeline->num_osds; i++)
		rc = sysfs_create_group(&dev->kobj, &osd_attr_group[i]);

	return rc;
}

void meson_drm_sysfs_unregister(struct drm_device *drm_dev)
{
	int rc, i;
	struct meson_drm *priv = drm_dev->dev_private;
	struct device *dev = drm_dev->primary->kdev;

	sysfs_remove_group(&dev->kobj, &vpu_attr_group);
	sysfs_remove_bin_file(&dev->kobj, &state_attr);
	sysfs_remove_bin_file(&dev->kobj, &reg_dump_attr);

	for (i = 0; i < priv->pipeline->num_osds; i++)
		rc = sysfs_create_group(&dev->kobj, &osd_attr_group[i]);

}

