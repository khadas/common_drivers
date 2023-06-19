/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/

#include <linux/atomic.h>
#include <linux/dma-buf.h>

#include "aml_buf_core.h"
#include "aml_vcodec_util.h"

static bool bc_sanity_check(struct buf_core_mgr_s *bc)
{
	return (bc->state == BM_STATE_ACTIVE) ? true : false;
}

/* In mmap mode, key is phy_addr; And in dma mode, key is dma buf handle */
static bool is_dma_mode(ulong key, ulong phy_addr)
{
	return (key != phy_addr) ? true : false;
}

static void buf_core_destroy(struct kref *kref);

static void direction_buf_get(struct buf_core_mgr_s *bc,
			  struct buf_core_entry *entry,
			  enum buf_core_user user)
{
	int type;
	switch (user) {
		case BUF_USER_DEC:
			entry->holder = BUF_HOLDER_DEC;
			entry->ref_bit_map += DEC_BIT;

			break;
		case BUF_USER_VPP:
			entry->holder = BUF_HOLDER_VPP;
			entry->ref_bit_map += VPP_BIT;

			type = bc->get_pre_user(bc, entry, user);
			if (type == BUF_USER_DEC)
				entry->ref_bit_map -= DEC_BIT;
			if (type == BUF_USER_GE2D)
				entry->ref_bit_map -= GE2D_BIT;

			break;
		case BUF_USER_GE2D:
			entry->holder = BUF_HOLDER_GE2D;
			entry->ref_bit_map += GE2D_BIT;
			entry->ref_bit_map -= DEC_BIT;

			break;
		case BUF_USER_VSINK:
			entry->holder = BUF_HOLDER_VSINK;
			entry->ref_bit_map += VSINK_BIT;

			type = bc->get_pre_user(bc, entry, user);
			if (type == BUF_USER_VPP)
				entry->ref_bit_map -= VPP_BIT;
			if (type == BUF_USER_DEC)
				entry->ref_bit_map -= DEC_BIT;
			if (type == BUF_USER_GE2D)
				entry->ref_bit_map -= GE2D_BIT;

			break;
		case BUF_USER_DI:
			entry->ref_bit_map += DI_BIT;

			break;
		default:
			break;
	}
}

static void direction_buf_put(struct buf_core_mgr_s *bc,
			  struct buf_core_entry *entry,
			  enum buf_core_user user)
{
	switch (user) {
		case BUF_USER_DEC:
			if (entry->ref_bit_map & DEC_MASK)
				entry->ref_bit_map -= DEC_BIT;
			if (entry->ref_bit_map & GE2D_MASK)
				entry->holder = BUF_HOLDER_GE2D;
			if (entry->ref_bit_map & VPP_MASK)
				entry->holder = BUF_HOLDER_VPP;
			if (entry->ref_bit_map & VSINK_MASK)
				entry->holder = BUF_HOLDER_VSINK;

			break;
		case BUF_USER_VPP:
			entry->ref_bit_map -= VPP_BIT;
			if (entry->ref_bit_map & VPP_MASK)
				entry->holder = BUF_HOLDER_VPP;
			else {
				if (entry->ref_bit_map & GE2D_MASK)
					entry->holder = BUF_HOLDER_GE2D;
				if (entry->ref_bit_map & DEC_MASK)
					entry->holder = BUF_HOLDER_DEC;
			}

			break;
		case BUF_USER_GE2D:
			entry->ref_bit_map -= GE2D_BIT;
			if (entry->ref_bit_map & GE2D_MASK)
				entry->holder = BUF_HOLDER_GE2D;
			else
				entry->holder = BUF_HOLDER_DEC;

			break;
		case BUF_USER_VSINK:
			entry->ref_bit_map -= VSINK_BIT;
			if (entry->ref_bit_map & DEC_MASK)
				entry->holder = BUF_HOLDER_DEC;
			if (entry->ref_bit_map & VSINK_MASK)
				entry->holder = BUF_HOLDER_VSINK;
			else if (entry->ref_bit_map & DI_MASK)
				entry->holder = BUF_HOLDER_DI;

			break;
		case BUF_USER_DI:
			entry->ref_bit_map -= DI_BIT;

			break;
		default:
			break;
	}
}

static void buf_core_update_holder(struct buf_core_mgr_s *bc,
			  struct buf_core_entry *entry,
			  enum buf_core_user user,
			  enum buf_direction direction)
{
	struct buf_core_entry *master = entry->pair != BUF_MASTER ? entry->master_entry : entry;

	if (direction == BUF_GET)
		direction_buf_get(bc, master, user);
	else
		direction_buf_put(bc, master, user);

	if (master->ref_bit_map & 0x8888)
		v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"error! ref_bit_map(0x%x)\n", master->ref_bit_map);

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, holder:%d, key:%lx, ref_bit_map(0x%x)\n",
		__func__, user, master->holder, master->key, master->ref_bit_map);

	return;
}

static void buf_core_free_que(struct buf_core_mgr_s *bc,
			     struct buf_core_entry *entry)
{
	entry->state = BUF_STATE_FREE;
	entry->holder = BUF_HOLDER_FREE;
	entry->ref_bit_map = 0;
	list_add_tail(&entry->node, &bc->free_que);
	bc->free_num++;
	entry->inited = true;
	entry->queued_mask = 0;

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->phy_addr,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);
}

static void buf_core_get(struct buf_core_mgr_s *bc,
			enum buf_core_user user,
			struct buf_core_entry **out_entry,
			bool more_ref)
{
	struct buf_core_entry *entry = NULL, *sub_entry;
	bool user_change = false;

	mutex_lock(&bc->mutex);

	if (!bc_sanity_check(bc)) {
		goto out;
	}

	if (list_empty(&bc->free_que)) {
		goto out;
	}

	entry = list_first_entry(&bc->free_que, struct buf_core_entry, node);
	list_del(&entry->node);
	bc->free_num--;

	user_change	= entry->user != user ? 1 : 0;
	entry->user	= user;
	entry->state	= BUF_STATE_USE;
	atomic_inc(&entry->ref);

	if (entry->sub_entry[0]) {
		sub_entry = (struct buf_core_entry *)entry->sub_entry[0];
		sub_entry->user = user;
		sub_entry->state = BUF_STATE_USE;
	}

	if (entry->sub_entry[1]) {
		sub_entry = (struct buf_core_entry *)entry->sub_entry[1];
		sub_entry->user = user;
		sub_entry->state = BUF_STATE_USE;
	}

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__, user,
		entry->key,
		entry->phy_addr,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	if (bc->prepare /*&&
		user_change*/) {
		bc->prepare(bc, entry);
	}
	buf_core_update_holder(bc, entry, user, BUF_GET);
out:
	*out_entry = entry;

	mutex_unlock(&bc->mutex);
}

static void buf_core_put(struct buf_core_mgr_s *bc,
			struct buf_core_entry *entry)
{
	mutex_lock(&bc->mutex);

	if (!bc_sanity_check(bc)) {
		mutex_unlock(&bc->mutex);
		return;
	}

	atomic_dec_return(&entry->ref);

	mutex_unlock(&bc->mutex);
}

static void buf_core_get_ref(struct buf_core_mgr_s *bc,
			    struct buf_core_entry *entry)
{
	mutex_lock(&bc->mutex);

	if (!bc_sanity_check(bc)) {
		mutex_unlock(&bc->mutex);
		return;
	}

	entry->state = BUF_STATE_REF;
	atomic_inc(&entry->ref);
	//kref_get(&bc->core_ref);
	buf_core_update_holder(bc, entry, BUF_USER_DEC, BUF_GET);

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->phy_addr,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	mutex_unlock(&bc->mutex);
}

static void buf_core_put_ref(struct buf_core_mgr_s *bc,
			    struct buf_core_entry *entry)
{
	mutex_lock(&bc->mutex);

	if (!atomic_read(&entry->ref) && !bc_sanity_check(bc)) {
		v4l_dbg_ext(bc->id, 0,
		"%s, user:%d, key:%lx, phy:%lx, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->phy_addr,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);
		mutex_unlock(&bc->mutex);
		return;
	}

	if (!atomic_dec_return(&entry->ref)) {
		buf_core_free_que(bc, entry);
	} else {
		entry->state = BUF_STATE_REF;
		buf_core_update_holder(bc, entry, BUF_USER_DEC, BUF_PUT);
	}

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->phy_addr,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	//kref_put(&bc->core_ref, buf_core_destroy);

	mutex_unlock(&bc->mutex);
}

static int buf_core_done(struct buf_core_mgr_s *bc,
			   struct buf_core_entry *entry,
			   enum buf_core_user user)
{
	int ret = 0;
	struct buf_core_entry *master;
	mutex_lock(&bc->mutex);

	master = entry;
	if (entry->pair != BUF_MASTER)
		master = entry->master_entry;

	if (!bc_sanity_check(bc)) {
		goto out;
	}

	if (WARN_ON((entry->state != BUF_STATE_USE) &&
		(entry->state != BUF_STATE_REF) &&
		(entry->state != BUF_STATE_DONE))) {
		ret = -1;
		goto out;
	}

	entry->state = BUF_STATE_DONE;

	ret = bc->output(bc, entry, user);

	if (bc->vpp_dque && /* Submit to GE2D doesn't call vpp_dque! */
		bc->get_next_user(bc, entry, user) == BUF_USER_VSINK &&
		!bc->vpp_dque(bc, entry)) {
		atomic_inc(&master->ref);
		buf_core_update_holder(bc, entry, BUF_USER_DI, BUF_GET);
	}

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__, user,
		entry->key,
		entry->phy_addr,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&master->ref),
		kref_read(&bc->core_ref),
		bc->free_num);
out:
	mutex_unlock(&bc->mutex);

	return ret;
}

static void buf_core_fill(struct buf_core_mgr_s *bc,
			    struct buf_core_entry *entry,
			    enum buf_core_user user)
{
	if (bc->vpp_que && user == BUF_USER_VSINK &&
		!bc->vpp_que(bc, entry)) {
		/*
		 * For DI post scenario, if seek or change resolution is doing,
		 * the reset callback will be executed in this process, and
		 * the state of all entries will be cleaned, but in fact
		 * the memory associated with entry may still be used
		 * as reference in DI mgr. If vpp_que returns 0, this entry
		 * is referenced by DI mgr, wait for DI mgr to be used, and
		 * then call callback to retrieve the buffer.
		 */
		mutex_lock(&bc->mutex);
		if (!entry->inited || entry->state == BUF_STATE_FREE)
			goto out;
		mutex_unlock(&bc->mutex);
	}

	mutex_lock(&bc->mutex);

	if (!bc_sanity_check(bc)) {
		goto out;
	}

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__, user,
		entry->key,
		entry->phy_addr,
		entry->index,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	if (WARN_ON((entry->state != BUF_STATE_INIT) &&
		(entry->state != BUF_STATE_DONE) &&
		(entry->state != BUF_STATE_REF))) {
		goto out;
	}

	if (bc->external_process)
		bc->external_process(bc, entry);

	if (!atomic_dec_return(&entry->ref)) {
		buf_core_free_que(bc, entry);
	} else {
		entry->state = BUF_STATE_REF;
		buf_core_update_holder(bc, entry, user, BUF_PUT);
	}

out:
	mutex_unlock(&bc->mutex);
}

static void buf_core_vpp_cb(struct buf_core_mgr_s *bc, struct buf_core_entry *entry)
{
	mutex_lock(&bc->mutex);
	if (entry->pair != BUF_MASTER)
		entry = (struct buf_core_entry *)entry->master_entry;
	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, st:(%d, %d), ref:(%d, %d), free:%d\n",
		__func__, entry->user,
		entry->key,
		entry->phy_addr,
		entry->state,
		bc->state,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);

	if (!atomic_dec_return(&entry->ref)) {
		buf_core_free_que(bc, entry);
	} else {
		entry->state = BUF_STATE_REF;
		buf_core_update_holder(bc, entry, BUF_USER_DI, BUF_PUT);
	}
	mutex_unlock(&bc->mutex);
}

static int buf_core_ready_num(struct buf_core_mgr_s *bc)
{
	if (!bc_sanity_check(bc)) {
		return 0;
	}

	return bc->free_num;
}

static bool buf_core_empty(struct buf_core_mgr_s *bc)
{
	if (!bc_sanity_check(bc)) {
		return true;
	}

	return list_empty(&bc->free_que);
}

static void buf_core_reset(struct buf_core_mgr_s *bc)
{
	struct buf_core_entry *entry, *tmp;
	struct hlist_node *h_tmp;
	ulong bucket;

	if (bc->vpp_reset)
		bc->vpp_reset(bc);

	mutex_lock(&bc->mutex);

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, core st:%d, core ref:%d, free:%d\n",
		__func__,
		bc->state,
		kref_read(&bc->core_ref),
		bc->free_num);

	list_for_each_entry_safe(entry, tmp, &bc->free_que, node) {
		list_del(&entry->node);
	}

	hash_for_each_safe(bc->buf_table, bucket, h_tmp, entry, h_node) {
		entry->user = BUF_USER_MAX;
		entry->state = BUF_STATE_INIT;
		entry->queued_mask = 0;
		entry->inited = false;

		if (entry->pair == BUF_MASTER) {
			atomic_set(&entry->ref, 1);
			if (entry->sub_entry[0])
				atomic_inc(&entry->ref);
			if (entry->sub_entry[1])
				atomic_inc(&entry->ref);
		}
	}

	bc->free_num = 0;

	mutex_unlock(&bc->mutex);
}

static void buf_core_destroy(struct kref *kref)
{
	struct buf_core_mgr_s *bc =
		container_of(kref, struct buf_core_mgr_s, core_ref);
	struct buf_core_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &bc->free_que, node) {
		list_del(&entry->node);
	}

	bc->free_num	= 0;
	bc->buf_num	= 0;
	bc->state	= BM_STATE_EXIT;

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR, "%s\n", __func__);
}

static int buf_core_attach(struct buf_core_mgr_s *bc, ulong key,
					ulong phy_addr, void *priv)
{
	int ret = 0;
	struct buf_core_entry *entry;
	struct hlist_node *tmp;

	mutex_lock(&bc->mutex);

	hash_for_each_possible_safe(bc->buf_table, entry, tmp, h_node, key) {
		if (key == entry->key) {
			if (is_dma_mode(key, phy_addr) && !entry->dma_ref) {
				get_dma_buf((struct dma_buf *)key);
				entry->dma_ref++;
			}
			v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
				"reuse buffer, user:%d, key:%lx, phy:%lx idx:%d, "
				"st:(%d, %d), ref:(%d, %d, %d), free:%d\n",
				entry->user,
				entry->key,
				entry->phy_addr,
				entry->index,
				entry->state,
				bc->state,
				entry->dma_ref,
				atomic_read(&entry->ref),
				kref_read(&bc->core_ref),
				bc->free_num);

			entry->user	= BUF_USER_MAX;
			entry->state	= BUF_STATE_INIT;
			entry->vb2 	= priv;

			bc->prepare(bc, entry);

			goto out;
		}
	}

	ret = bc->mem_ops.alloc(bc, &entry, priv);
	if (ret) {
		goto out;
	}

	entry->key	= key;
	entry->phy_addr = phy_addr;
	entry->priv	= bc;
	entry->vb2	= priv;
	entry->user	= BUF_USER_MAX;
	entry->state	= BUF_STATE_INIT;
	atomic_set(&entry->ref, 1);

	hash_add(bc->buf_table, &entry->h_node, key);

	bc->state	= BM_STATE_ACTIVE;
	bc->buf_num++;
	kref_get(&bc->core_ref);
	if (is_dma_mode(key, phy_addr)) {
		get_dma_buf((struct dma_buf *)key);
		entry->dma_ref++;
	}

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
		"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d, %d), free:%d\n",
		__func__,
		entry->user,
		entry->key,
		entry->phy_addr,
		entry->index,
		entry->state,
		bc->state,
		entry->dma_ref,
		atomic_read(&entry->ref),
		kref_read(&bc->core_ref),
		bc->free_num);
out:
	mutex_unlock(&bc->mutex);

	return ret;
}

static void buf_core_detach(struct buf_core_mgr_s *bc, ulong key)
{
	struct buf_core_entry *entry;
	struct hlist_node *h_tmp;

	mutex_lock(&bc->mutex);

	hash_for_each_possible_safe(bc->buf_table, entry, h_tmp, h_node, key) {
		if (key == entry->key) {
			v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
				"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d), free:%d\n",
				__func__,
				entry->user,
				entry->key,
				entry->phy_addr,
				entry->index,
				entry->state,
				bc->state,
				atomic_read(&entry->ref),
				kref_read(&bc->core_ref),
				bc->free_num);

			entry->state = BUF_STATE_ERR;
			hash_del(&entry->h_node);
			bc->mem_ops.free(bc, entry);

			kref_put(&bc->core_ref, buf_core_destroy);
			break;
		}
	}

	mutex_unlock(&bc->mutex);
}

static void buf_core_put_dma(struct buf_core_mgr_s *bc)
{
	struct buf_core_entry *entry;
	struct hlist_node *h_tmp;
	ulong bucket;

	mutex_lock(&bc->mutex);

	hash_for_each_safe(bc->buf_table, bucket, h_tmp, entry, h_node) {
		if (is_dma_mode(entry->key, entry->phy_addr) &&
			entry->dma_ref) {
			entry->dma_ref--;
			v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
				"%s, user:%d, key:%lx, phy:%lx, idx:%d, st:(%d, %d), ref:(%d, %d, %d), free:%d\n",
				__func__,
				entry->user,
				entry->key,
				entry->phy_addr,
				entry->index,
				entry->state,
				bc->state,
				entry->dma_ref,
				atomic_read(&entry->ref),
				kref_read(&bc->core_ref),
				bc->free_num);
			dma_buf_put((struct dma_buf *)entry->key);
		}
	}

	mutex_unlock(&bc->mutex);
}


static void buf_core_update(struct buf_core_mgr_s *bc, struct buf_core_entry *entry,
					ulong phy_addr, enum buf_pair pair)
{
	mutex_lock(&bc->mutex);

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR,
			"%s, user:%d, key:%lx, phy:(%lx->%lx), "
			"idx:%d, pair:%d st:(%d, %d), ref:(%d, %d), free:%d\n",
			__func__,
			entry->user,
			entry->key,
			entry->phy_addr,
			phy_addr,
			entry->index,
			pair,
			entry->state,
			bc->state,
			atomic_read(&entry->ref),
			kref_read(&bc->core_ref),
			bc->free_num);

	entry->phy_addr = phy_addr;
	entry->pair = pair;

	bc->prepare(bc, entry);

	mutex_unlock(&bc->mutex);
}

void buf_core_replace(struct buf_core_mgr_s *bc,
				struct buf_core_entry *entry, void *priv)
{
	mutex_lock(&bc->mutex);

	entry->vb2	= priv;
	entry->user	= BUF_USER_MAX;
	entry->state	= BUF_STATE_INIT;

	bc->prepare(bc, entry);

	mutex_unlock(&bc->mutex);
}

ssize_t buf_core_walk(struct buf_core_mgr_s *bc, char *buf)
{
	struct buf_core_entry *entry, *tmp;
	struct hlist_node *h_tmp;
	ulong bucket;
	int dec_holders = 0;
	int ge2d_holders = 0;
	int vpp_holders = 0;
	int vsink_holders = 0;
	int di_holders = 0;
	char *pbuf = buf;

	mutex_lock(&bc->mutex);

	pbuf += sprintf(pbuf, "\nFree queue elements:\n");
	list_for_each_entry_safe(entry, tmp, &bc->free_que, node) {
		pbuf += sprintf(pbuf,
			"--> key:%lx, phy:%lx, idx:%d, user:%d, holder:%d, "
			"st:(%d, %d), ref:(%d, %d), free:%d\n",
			entry->key,
			entry->phy_addr,
			entry->index,
			entry->user,
			entry->holder,
			entry->state,
			bc->state,
			atomic_read(&entry->ref),
			kref_read(&bc->core_ref),
			bc->free_num);
	}

	pbuf += sprintf(pbuf, "\nHash table elements:\n");
	hash_for_each_safe(bc->buf_table, bucket, h_tmp, entry, h_node) {
		if (entry->pair == BUF_MASTER) {
			if (entry->holder == BUF_HOLDER_DEC)
				dec_holders++;
			if (entry->holder == BUF_HOLDER_GE2D)
				ge2d_holders++;
			if (entry->holder == BUF_HOLDER_VPP)
				vpp_holders++;
			if (entry->holder == BUF_HOLDER_VSINK)
				vsink_holders++;
			if (entry->ref_bit_map & DI_MASK)
				di_holders++;

			pbuf += sprintf(pbuf,
				"--> key:%lx, phy:%lx, idx:%d, user:%d, holder:%d, st:(%d, %d), ref:(%d, %d), free:%d, ref_map:0x%x\n",
				entry->key,
				entry->phy_addr,
				entry->index,
				entry->user,
				entry->holder,
				entry->state,
				bc->state,
				atomic_read(&entry->ref),
				kref_read(&bc->core_ref),
				bc->free_num,
				entry->ref_bit_map);
		}
	}
	pbuf += sprintf(pbuf, "holders: dec(%d), ge2d(%d), vpp(%d), vsink(%d) di(%d)\n",
		dec_holders, ge2d_holders, vpp_holders, vsink_holders, di_holders);

	mutex_unlock(&bc->mutex);

	return pbuf - buf;
}
EXPORT_SYMBOL(buf_core_walk);

int buf_core_mgr_init(struct buf_core_mgr_s *bc)
{
	/* Sanity check of mandatory interfaces. */
	if (WARN_ON(!bc->config) ||
		WARN_ON(!bc->input) ||
		WARN_ON(!bc->output) ||
		WARN_ON(!bc->mem_ops.alloc) ||
		WARN_ON(!bc->mem_ops.free)) {
		return -1;
	}

	hash_init(bc->buf_table);
	INIT_LIST_HEAD(&bc->free_que);
	mutex_init(&bc->mutex);
	kref_init(&bc->core_ref);

	bc->free_num		= 0;
	bc->buf_num		= 0;
	bc->state		= BM_STATE_INIT;

	/* The external interfaces of BC context. */
	bc->attach		= buf_core_attach;
	bc->detach		= buf_core_detach;
	bc->reset		= buf_core_reset;
	bc->update		= buf_core_update;
	bc->replace		= buf_core_replace;
	bc->put_dma		= buf_core_put_dma;

	/* The interface set of the buffer core operation. */
	bc->buf_ops.get		= buf_core_get;
	bc->buf_ops.put		= buf_core_put;
	bc->buf_ops.get_ref	= buf_core_get_ref;
	bc->buf_ops.put_ref	= buf_core_put_ref;
	bc->buf_ops.done	= buf_core_done;
	bc->buf_ops.fill	= buf_core_fill;
	bc->buf_ops.ready_num	= buf_core_ready_num;
	bc->buf_ops.empty	= buf_core_empty;
	bc->buf_ops.vpp_cb	= buf_core_vpp_cb;
	bc->buf_ops.update_holder = buf_core_update_holder;

	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR, "%s\n", __func__);

	return 0;
}
EXPORT_SYMBOL(buf_core_mgr_init);

void buf_core_mgr_release(struct buf_core_mgr_s *bc)
{
	v4l_dbg_ext(bc->id, V4L_DEBUG_CODEC_BUFMGR, "%s\n", __func__);

	kref_put(&bc->core_ref, buf_core_destroy);
}
EXPORT_SYMBOL(buf_core_mgr_release);

