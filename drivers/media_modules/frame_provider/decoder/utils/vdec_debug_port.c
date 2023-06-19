#include <linux/init.h>
#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/nmi.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/uaccess.h>

#include <linux/amlogic/media/codec_mm/codec_mm.h>

#include "vdec_debug_port.h"
#include "vdec.h"
#include "frame_check.h"
#include <linux/crc32.h>


static struct dentry *dec_debug_entry;
static struct amvdec_debug_port_t *dec_debug_port;


#define pr_dbg_port(port, mask, ...) do {				\
			if ((port && (port->debug_flag & mask)) ||	\
				(mask == PORT_VDBG_FLAG_ERR))			\
				printk("[VDBG] "__VA_ARGS__);			\
		} while(0)


static int debug_port_set_yuv_dump(struct amvdec_debug_port_t *port, int vdec_id, int pic_start, int pic_num)
{
	struct vdec_s *vdec;

	vdec = vdec_get_vdec_by_id(vdec_id);
	if (vdec) {
		struct pic_dump_t *dmp = &vdec->vfc.pic_dump;

		/* config created vdec dump yuv */
		dmp->start    = pic_start;
		dmp->num      = pic_num;
		dmp->end      = pic_start + pic_num;
		dmp->dump_cnt = 0;
		dmp->yuv_fp   = NULL;   //no write file in driver
		dmp->buf_addr = port->buf_vaddr;
		dmp->buf_size = port->buf_size;

		vdec->vfc.id  = vdec_id;
		vdec->vfc.enable |= 0x1;
	} else {
		char cmd_buf[64] = {0};

		snprintf(cmd_buf, sizeof(cmd_buf), "%d %d %d\n", vdec_id, pic_start, pic_num);
		dump_yuv_store(NULL, NULL, cmd_buf, sizeof(cmd_buf));
	}

	pr_info("%s, config dump yuv end. %d, %d, %d\n", __func__, vdec_id, pic_start, pic_num);

	return 0;
}

static int debug_port_set_crc_dump(struct amvdec_debug_port_t *port, int vdec_id, bool on_off)
{
	char cmd_buf[64] = {0};

	snprintf(cmd_buf, sizeof(cmd_buf), "%d %d\n", vdec_id, on_off);
	frame_check_store(NULL, NULL, cmd_buf, sizeof(cmd_buf));

	return 0;
}

static int debug_port_set_es_dump(struct amvdec_debug_port_t *port, int vdec_id, int mode)
{

	return 0;
}

int debug_port_debug_config(struct amvdec_debug_port_t *port, ulong arg)
{
	int ret = 0;
	struct debug_config_param param;

	if (copy_from_user((void *)&param, (void *)arg,
		sizeof(struct debug_config_param)))
		return -EFAULT;

	switch (param.type) {
	case TYPE_YUV:
		debug_port_set_yuv_dump(port, param.id, param.pic_start, param.pic_num);
		break;
	case TYPE_CRC:
		debug_port_set_crc_dump(port, param.id, 1);
		break;
	case TYPE_ES:
		debug_port_set_es_dump(port, param.id, param.mode);
		break;
	default:
		pr_info("%s, can not find debug port config type\n", __func__);
		break;
	}

	return ret;
}

static u32 copy_phys_to_buf(char *to, ulong from, u32 size)
{
	u8 *virt = NULL;
	u32 map_size = SZ_1M;
	u32 stride = SZ_1M;
	u32 res = size;
	u32 phy = from;
	u32 total = 0;
	int retry = 0;

	do {
		if (res > map_size)
			stride = map_size;
		else
			stride = res;

		do {
			virt = codec_mm_vmap(phy, stride);
			if (!virt) {
				if (++retry > 10) {
					pr_err("%s map failed\n", __func__);
					return 0;
				}
				map_size >>= 1;
				stride = map_size;
			}
		} while(!virt);

		codec_mm_dma_flush(virt, stride, DMA_FROM_DEVICE);
		memcpy(to, virt, stride);
		phy += stride;
		to += stride;
		total += stride;
		res -= stride;
		codec_mm_unmap_phyaddr(virt);
		virt = NULL;
	} while (res);

	return total;
}

enum {
	BUF_TYPE_VIRT,
	BUF_TYPE_PHYS,
	BUF_TYPE_USER,
	BUF_TYPE_MAX
};

static int common_copy(void *to, const void *from, ulong n, int type)
{
	int ret = n;

	if (likely(type == BUF_TYPE_VIRT)) {
		memcpy(to, from, n);
	} else if (type == BUF_TYPE_PHYS) {
		ret = copy_phys_to_buf(to, (ulong)from, n);
	} else if (type == BUF_TYPE_USER) {
		ret = n - copy_from_user(to, from, n);
	} else
		ret = 0;

	return ret;
}

static int port_packet_data_write(struct amvdec_debug_port_t *port,
	struct port_data_packet *pkt, const void *data_buf, int buf_type)
{
	u32 ret;
	u32 plen = sizeof(struct port_data_packet);
	u32 dlen = pkt->data_size;
	u32 pad_size = PADING_SIZE - plen;
	char *wp = port->wp;

#if 0
	if (port->rp > port->wp) {
		if (port->rp - port->wp < total_size)
			return -1;
	} else {
		if (port->buf_vaddr + port->buf_size - port->wp < plen) {
			if (port->rp - port->buf_vaddr < total_size)
				return -1;
		} else if (port->rp + port->buf_size - port->wp < total_size)
			return -1;
	}
#endif
	if (port->rp <= wp) {
		u32 left = port->buf_vaddr + port->buf_size - wp;

		if (left < (dlen + PADING_SIZE)) {
			if (left <= plen) {
				memset(wp, 0, left);
				wp = port->buf_vaddr;
			} else {
				memcpy(wp, pkt, plen);
				wp += plen;
				left -= plen;

				if (left >= dlen) {
					ret = common_copy(wp, data_buf, dlen, buf_type);
					ERRP(ret < dlen, return 0, 1, "%s failed\n", __func__);
					left -= dlen;
					wp += dlen;
					if (left)
						memset(wp, 0, left);

					pad_size -= left;
					wp = port->buf_vaddr + pad_size;
				} else {
					ret = common_copy(wp, data_buf, left, buf_type);
					ERRP(ret < left, return 0, 1, "%s failed\n", __func__);
					wp = port->buf_vaddr;   //update wp to buf start

					ret = common_copy(wp, data_buf + left, dlen - left, buf_type);
					ERRP(ret < dlen - left, return 0, 1, "%s failed\n", __func__);
					wp += (ret + pad_size);
				}
				goto success_wr;
			}
		}
	}

	memcpy(wp, pkt, plen);
	ret = common_copy(wp + plen, data_buf, dlen, buf_type);
	ERRP(ret < dlen, return 0, 1, "%s failed\n", __func__);
	wp += (dlen + PADING_SIZE);
success_wr:
	port->wp = wp;
	if (pad_size)
		memset(wp - pad_size, 0, pad_size);

	port->packet_cnt++;
	return 0;
}

static bool is_vdec_releasing(int id)
{
	struct vdec_s *vdec;

	vdec = vdec_get_vdec_by_id(id);
	if (vdec) {
		if (vdec->next_status == VDEC_STATUS_DISCONNECTED)
			return true;
	}

	return false;
}

/*
@data_buf : data source;
@data_size: data size to write
@id       : vdec id
@type     : [bit 00 ~ 15] data type;
            [bit 16 ~ 31] dta buf type, 0 vbuf, 1 phybuf, 2 userbuf;
*/
static int debug_port_write_data(const void *data_buf, int data_size,
	int id, int type)
{
	struct amvdec_debug_port_t *port = dec_debug_port;
	struct port_data_packet pkt;
	u32 port_dat_size;
	u32 port_buf_size;
	int ret, timeout_cnt = 0;
	int enable_mask = (1 << (type & 0xffff));

	if ((!data_buf) || (!data_size) || (id > MAX_INSTANCE_NUM - 1))
		return 0;

	if ((port->enable[id] & enable_mask) == 0)
		return 0;

	port_dat_size = data_size + PADING_SIZE;
	if (port_dat_size > port->buf_size) {
		pr_err("%s, data size %x > buf_size %x\n",
			__func__, port_dat_size, port->buf_size);
		return -ENOMEM;
	}

	do {
		mutex_lock(&port->mlock);
		if (port->rp > port->wp)
			port_buf_size = port->rp - port->wp;
		else {
			if ((port->buf_vaddr + port->buf_size - port->wp) <= sizeof(pkt))
				port_buf_size = port->rp - port->buf_vaddr;  //discard less bytes
			else
				port_buf_size = port->rp + port->buf_size - port->wp;
		}
		mutex_unlock(&port->mlock);

		if (port->packet_cnt)
			wake_up_interruptible(&port->poll_wait);

		if (is_vdec_releasing(id)) {
			pr_info("debug_port: vdec.%d releasing, exit\n", id);
			return 0;
		}

		ret = wait_event_interruptible_timeout(port->wait_data_done,
			(port_buf_size >= port_dat_size), msecs_to_jiffies(25));
		if (!ret) {
			if (++timeout_cnt > 10) {
				//port->fatal_error |= DEBUG_PORT_FATAL_ERROR_NOMEM;
				pr_err("%s, no buf to wr sz 0x%x, buf_sz 0x%x, packcnt %d\n",
					__func__, port_dat_size, port_buf_size, port->packet_cnt);
				timeout_cnt = 0;
			}
		}

	} while (port_buf_size < port_dat_size);

	mutex_lock(&port->mlock);
	pkt.header    = PACKET_HEADER;
	pkt.id        = id;
	pkt.type      = type & 0xffff;
	pkt.data_size = data_size;
	pkt.crc       = 0;

	port_packet_data_write(port, &pkt, data_buf, ((type >> 16) & 0xf));
	mutex_unlock(&port->mlock);

	if (port->wp > port->buf_vaddr + port->buf_size) {
		pr_err("%s, wp error\n", __func__);
		port->debug_flag |= PORT_VDBG_FLAG_DBG;
	}

	if (port->debug_flag & PORT_VDBG_FLAG_DBG) {
		pr_info("write: id %d, type %x, data_size 0x%x, packcnt %d\n",
			id, type, data_size, port->packet_cnt);
		pr_info("write: rp %px, wp %px\n", port->rp, port->wp);
	}

	wake_up_interruptible(&port->poll_wait);

	return 0;
}

static int debug_port_update_vinfo(int id, int format, struct vframe_s *vf)
{
	struct port_vdec_info info;

	info.id = id;
	info.format = format;
	info.double_write = 0;
	info.stream_w = vf->canvas0_config[0].width;
	info.stream_h = vf->canvas0_config[0].height;
	info.dw_w     = vf->width;
	info.dw_h     = vf->height;
	info.plane_num = vf->plane_num;
	info.bitdepth = vf->bitdepth;
	info.is_interlace = (vf->type & VIDTYPE_INTERLACE_BOTTOM) ? 1 : 0;

	debug_port_write_data((const char *)&info,
		sizeof(info), info.id, TYPE_INFO);

	return 0;
}

static void debug_port_buf_status_reset(struct amvdec_debug_port_t *port)
{
	mutex_lock(&port->mlock);
	port->rp = port->buf_vaddr;
	port->wp = port->buf_vaddr;
	port->packet_cnt = 0;
	port->fatal_error = 0;
	mutex_unlock(&port->mlock);

	pr_info("%s\n", __func__);
}

static int vdec_dbg_port_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
#if 0
	struct amvdec_debug_port_t *port =
		(struct amvdec_debug_port_t *)file->private_data;

	pr_info("%s in\n", __func__);

	if (port == NULL || port->buf_start) {
		pr_info("%s port or buf null\n", __func__);
		return -EFAULT;
	}

	if (vma->vm_pgoff > port->buf_size) {
		pr_info("%s pgoff: %lx, bufsize 0x%x\n", __func__, vma->vm_pgoff, port->buf_size);
		return -EFAULT;
	}

	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP | VM_IO;

	ret = remap_pfn_range(vma, vma->vm_start,
		(port->buf_start >> PAGE_SHIFT)  + vma->vm_pgoff,
		vma->vm_end - vma->vm_start,
		vma->vm_page_prot);

	pr_info("vdec_dbg_port_mmap ret %d\n", ret);
#endif
	return ret;
}

long vdec_dbg_port_ioctl(struct file *file, unsigned int cmd, unsigned long param)
{
	long ret = 0;
	struct amvdec_debug_port_t *port =
		(struct amvdec_debug_port_t *)file->private_data;

	switch (cmd) {
		case VDBG_IOC_PORT_CFG:
			ret = debug_port_debug_config(port, param);
			break;

		case VDBG_IOC_GET_DATA:
			break;

		case VDBG_IOC_DATA_DONE:
			wake_up_interruptible(&port->wait_data_done);
			break;

		case VDBG_IOC_BUF_RESET:
			debug_port_buf_status_reset(port);
			break;
		default:
			break;
	}

	return ret;
}

static __poll_t vdec_dbg_port_poll(struct file *file,
		poll_table *wait_table)
{
	struct amvdec_debug_port_t *port =
		(struct amvdec_debug_port_t *)file->private_data;

	if (port->packet_cnt == 0) {
		poll_wait(file, &port->poll_wait, wait_table);
	}
	pr_dbg_port(port, PORT_VDBG_FLAG_DBG,
		"poll wait done, packcnt %d\n", port->packet_cnt);
	if (port->packet_cnt) {
		return POLLIN | POLLRDNORM;
	} else
		return 0;
}

static ssize_t vdec_dbg_port_read(struct file *file,
	char __user *ubuf, size_t count, loff_t *offset)
{
	struct amvdec_debug_port_t *port =
		(struct amvdec_debug_port_t *)file->private_data;

	struct port_data_packet pkt;
	u32 total_size;

	if (port->packet_cnt == 0)
		return 0;

	if (port->rp <= port->wp)
		total_size = port->wp - port->rp;
	else {
		total_size = port->wp - port->rp + port->buf_size;
	}
	if (total_size < sizeof(pkt)) {
		pr_err("%s, no data to copy\n", __func__);
		debug_port_buf_status_reset(port);
		return 0;
	}

	mutex_lock(&port->mlock);
	if (port->rp > port->wp) {
		u32 left = port->buf_vaddr + port->buf_size - port->rp;

		if (left <= sizeof(pkt)) {   //discard size less then packet header size
			port->rp = port->buf_vaddr;
		} else {
			memcpy(&pkt, port->rp, sizeof(pkt));
			if (count < pkt.data_size + PADING_SIZE) {
				goto err_no_enough;
			}

			if (left >= pkt.data_size + PADING_SIZE) {
				if (copy_to_user(ubuf, port->rp, pkt.data_size + PADING_SIZE)) {
					pr_info("%s, copy failed 0\n", __func__);
				}
				port->rp += pkt.data_size + PADING_SIZE;
			} else {
				if (copy_to_user(ubuf, port->rp, left)) {
					pr_info("%s, copy failed 1\n", __func__);
				}

				//pr_info("%s, rp %px, left %x\n", __func__, port->rp, left);
				port->rp = port->buf_vaddr;
				if (copy_to_user(ubuf + left, port->rp, pkt.data_size + PADING_SIZE - left))
					pr_info("%s, copy failed 2\n", __func__);

				port->rp += (pkt.data_size + PADING_SIZE - left);
			}
			goto read_end;
		}
	}
	memcpy(&pkt, port->rp, sizeof(pkt));
	if (count < pkt.data_size + PADING_SIZE) {
		goto err_no_enough;
	}
	if (copy_to_user(ubuf, port->rp, pkt.data_size + PADING_SIZE)) {
		pr_info("%s, copy failed\n", __func__);
	}
	port->rp += (pkt.data_size + PADING_SIZE);

read_end:
	port->packet_cnt--;
	mutex_unlock(&port->mlock);

	port->fatal_error &= (~DEBUG_PORT_FATAL_ERROR_NOMEM);
	wake_up_interruptible(&port->wait_data_done);

	if (port->debug_flag & PORT_VDBG_FLAG_DBG) {
		pr_info("read: id %d, type %x, data_size 0x%x, packcnt %d\n",
			pkt.id, pkt.type, pkt.data_size, port->packet_cnt);

		pr_info("read: rp %px, wp %px\n", port->rp, port->wp);
	}
	return pkt.data_size + PADING_SIZE;

err_no_enough:
	pr_err("%s , count 0x%zx total data 0x%x no enough buffer\n",
		__func__, count, pkt.data_size + PADING_SIZE);
	mutex_unlock(&port->mlock);
	return 0;
}

static ssize_t vdec_dbg_port_write(struct file *file,
	const char __user *buf, size_t count, loff_t *offset)
{
	struct amvdec_debug_port_t *port =
		(struct amvdec_debug_port_t *)file->private_data;
	char *cmd;

	u32 id;
	u32 val = 0;
	ssize_t ret;
	char cbuf[32];

	cmd = (char *)vzalloc(count + 16);
	if (!cmd) {
		return 0;
	}
	if (copy_from_user(cmd, buf, count)) {
		vfree(cmd);
		return 0;
	}
	cbuf[0] = 0;
	ret = sscanf(cmd, "%s %x %x", cbuf, &id, &val);

	if (!strncmp(cbuf, "reset", strlen("reset"))) {
		debug_port_buf_status_reset(port);

	} else if (!strncmp(cbuf, "debug", strlen("debug"))) {
		if (id == 0) {
			pr_info("debug off\n");
			port->debug_flag &= (~PORT_VDBG_FLAG_DBG);
		} else {
			pr_info("debug on, buf range: %px ~ %px\n",
				port->buf_vaddr, port->buf_vaddr + port->buf_size);
			port->debug_flag |= PORT_VDBG_FLAG_DBG;
		}
	} else if (!strncmp(cbuf, "enable", strlen("enable"))) {
		if (!val) {
			vfree(cmd);
			return count;
		}
		port->enable[id] = val | 0x1;

		ret = 0;
		memset(cbuf, 0, sizeof(cbuf));
		if (val & (1 << TYPE_INFO))
			ret += snprintf(cbuf + ret, sizeof(cbuf), "INFO ");

		if (val & (1 << TYPE_YUV))
			ret += snprintf(cbuf + ret, sizeof(cbuf), "YUV ");

		if (val & (1 << TYPE_CRC))
			ret += snprintf(cbuf + ret, sizeof(cbuf), "CRC ");

		if (val & (1 << TYPE_ES))
			ret += snprintf(cbuf + ret, sizeof(cbuf), "ES ");

		pr_info("enable instance %d ( %s) dump\n", id, cbuf);

	} else if (!strncmp(cbuf, "disable", strlen("disable"))) {
		port->enable[id] &= (~val);
		pr_info("disable instance %d, dump %x\n", id, val);
	} else if (!strncmp(cbuf, "config", strlen("config"))) {
		port->enable[id] = val;
		pr_info("config instance %d, dump %x\n", id, val);
	}

	vfree(cmd);

	return count;
}

static int vdec_dbg_port_open(struct inode *inode, struct file *file)
{
	file->private_data = (void*)dec_debug_port;
	pr_info("%s\n", __func__);
	return 0;
}

static int vdec_dbg_port_close(struct inode *inode, struct file *file)
{
	pr_info("%s\n", __func__);
	return 0;
}

static const struct file_operations vdec_dbg_port_fops = {
	.open    = vdec_dbg_port_open,
	.read    = vdec_dbg_port_read,
	.write   = vdec_dbg_port_write,
	.unlocked_ioctl = vdec_dbg_port_ioctl,
	.release = vdec_dbg_port_close,
	.poll    = vdec_dbg_port_poll,
	.mmap    = vdec_dbg_port_mmap,
};

static u32 dec_debug_buf_size;
module_param(dec_debug_buf_size, uint, 0664);

static int vdec_debug_port_probe(struct amvdec_debug_port_t **p_mdbg)
{
	struct amvdec_debug_port_t *mport;
	int flags = CODEC_MM_FLAGS_DMA;
	u32 buf_size = SZ_1M * 16;

	mport = (struct amvdec_debug_port_t *)vzalloc(sizeof(struct amvdec_debug_port_t));
	if (!mport) {
		return -1;
	}

	INIT_LIST_HEAD(&mport->head);
	mutex_init(&mport->mlock);

	/* debug data buffer alloc */
	flags |= CODEC_MM_FLAGS_CMA_FIRST;
	flags |= CODEC_MM_FLAGS_FOR_VDECODER;

	if (dec_debug_buf_size)
		buf_size = dec_debug_buf_size;
	buf_size = round_up(buf_size, PAGE_SIZE);

	mport->buf_start = codec_mm_alloc_for_dma(VDEC_DEBUG_MODULE,
			buf_size/PAGE_SIZE, 4+PAGE_SHIFT, flags);
	if (!mport->buf_start) {
		pr_err("%s, alloc debug buf failed\n", __func__);
		vfree(mport);
		return -1;
	}
	mport->buf_vaddr = codec_mm_vmap(mport->buf_start, buf_size);
	if (mport->buf_vaddr) {
		pr_info("%s, vmap debug buf success %px\n", __func__, mport->buf_vaddr);
	} else {
		codec_mm_free_for_dma(VDEC_DEBUG_MODULE, mport->buf_start);
		mport->buf_start = 0;
		mport->buf_vaddr = (char *)vzalloc(buf_size);
		if (mport->buf_vaddr == NULL)
			return -ENOMEM;
	}
	mport->buf_size = buf_size;
	mport->rp = mport->buf_vaddr;
	mport->wp = mport->buf_vaddr;
	mport->fatal_error = 0;
	//mport->enable[] = 0;

	init_waitqueue_head(&mport->poll_wait);
	init_waitqueue_head(&mport->wait_data_done);

	*p_mdbg = mport;

	vdec_debug_port_register(debug_port_write_data,
		debug_port_update_vinfo);

	return 0;
}

static void vdec_debug_port_remove(struct amvdec_debug_port_t *mport)
{
	int ret;

	if (mport == NULL)
		return;

	if (mport->buf_start) {
		if (mport->buf_vaddr)
			codec_mm_unmap_phyaddr(mport->buf_vaddr);
		mport->buf_vaddr = NULL;
	} else{
		if (mport->buf_vaddr)
			vfree(mport->buf_vaddr);
		mport->buf_vaddr = NULL;
	}

	if (mport->buf_start) {
		ret = codec_mm_free_for_dma(VDEC_DEBUG_MODULE, mport->buf_start);
		if (ret < 0)
			pr_err("%s free failed\n", __func__);
	}

	vdec_debug_port_unregister();

	vfree(mport);
}

int __init vdec_debug_module_init(void)
{
	struct dentry *proot, *entry;
	int ret;

	proot = debugfs_lookup("vdec_profile", NULL);
	if (proot == NULL) {
		pr_info("debugfs_lookup vdec_profile dir failed\n");
		return -1;
	}

	entry = debugfs_create_file("debug_port", 0666, proot, NULL,
		&vdec_dbg_port_fops);
	if (!entry) {
		pr_info("%s create failed\n", __func__);
		return 0;
	}

	ret = vdec_debug_port_probe(&dec_debug_port);
	if (ret < 0) {
		debugfs_remove(entry);
		pr_info("debug utils probe failed, ret = %d\n", ret);
		return ret;
	}

	dec_debug_entry = entry;
	pr_info("vdec debugfs entry created\n");

	return 0;
}

void __exit vdec_debug_module_exit(void)
{
	vdec_debug_port_remove(dec_debug_port);

	debugfs_remove(dec_debug_entry);

	pr_info("vdec debugfs entry removed\n");
}

module_init(vdec_debug_module_init);
module_exit(vdec_debug_module_exit);

MODULE_DESCRIPTION("AMLOGIC vdec debug Driver");
MODULE_LICENSE("GPL");


