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
#ifndef _AML_BUF_CORE_H_
#define _AML_BUF_CORE_H_

#include <linux/kref.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/hash.h>
#include <linux/hashtable.h>

#define NEW_FB_CODE

#define BUF_HASH_BITS	(10)

#define DEC_BIT  (UL(1) << (0))
#define GE2D_BIT  (UL(1) << (4))
#define VPP_BIT  (UL(1) << (8))
#define VSINK_BIT  (UL(1) << (12))
#define DI_BIT  (UL(1) << (16))

#define DEC_MASK  (0xf)
#define GE2D_MASK  (0xf0)
#define VPP_MASK  (0xf00)
#define VSINK_MASK  (0xf000)
#define DI_MASK  (0xf0000)

#define MASTER_DONE  (1)
#define SUB0_DONE  (2)
#define SUB1_DONE  (3)
#define PAIR_DONE  (3)

struct buf_core_mgr_s;

/*
 * enum buf_core_state - The state of the buffer to be used.
 *
 * @BUF_STATE_INIT	: The initialization state of the buffer.
 * @BUF_STATE_FREE	: The idle state of the buffer that can be used.
 * @BUF_STATE_USE	: The status of the buffer being allocated by the user.
 * @BUF_STATE_REF	: The state of the buffer referenced by the user.
 * @BUF_STATE_DONE	: The state of the buffer is filled done by user.
 * @BUF_STATE_ERR     	: The buffer has an error or is about to be released.
 */
enum buf_core_state {
	BUF_STATE_INIT,
	BUF_STATE_FREE,
	BUF_STATE_USE,
	BUF_STATE_REF,
	BUF_STATE_DONE,
	BUF_STATE_ERR
};

/*
 * enum buf_core_state - The state of the buffer core manager context.
 *
 * @BM_STATE_INIT	: The initialization state of context.
 * @BM_STATE_ACTIVE	: Status indicates that there are available buffers to manage.
 * @BM_STATE_EXIT	: The buffer core manager context release.
 */
enum buf_core_mgr_state {
	BM_STATE_INIT,
	BM_STATE_ACTIVE,
	BM_STATE_EXIT
};

/*
 * enum buf_core_user - Buffer users.
 *
 * @BUF_USER_DEC	: Indicates that the current buffer user is decoder.
 * @BUF_USER_VPP	: Indicates that the current buffer user is vpp wrapper.
 * @BUF_USER_GE2D	: Indicates that the current buffer user is ge2d wrapper.
 * @BUF_USER_VSINK	: Indicates that the current buffer user is vsink.
 * @BUF_USER_MAX	: Invalid user.
 */
enum buf_core_user {
	BUF_USER_DEC,
	BUF_USER_VPP,
	BUF_USER_GE2D,
	BUF_USER_VSINK,
	BUF_USER_DI,
	BUF_USER_MAX
};

/*
 * enum buf_core_holder - Buffer holders.
 *
 * @BUF_HOLDER_DEC	: Indicates that the current buffer holder is decoder.
 * @BUF_HOLDER_VPP	: Indicates that the current buffer holder is vpp wrapper.
 * @BUF_HOLDER_GE2D	: Indicates that the current buffer holder is ge2d wrapper.
 * @BUF_HOLDER_VSINK	: Indicates that the current buffer holder is vsink.
 * @BUF_HOLDER_MAX	: Invalid holder.
 */
enum buf_core_holder {
	BUF_HOLDER_DEC,
	BUF_HOLDER_VPP,
	BUF_HOLDER_GE2D,
	BUF_HOLDER_VSINK,
	BUF_HOLDER_DI,
	BUF_HOLDER_FREE,
};

enum buf_direction {
	BUF_GET,
	BUF_PUT
};

/*
 * enum buf_pair - For di post.
 *
 * @BUF_MASTER	: Indicates that the first field.
 * @BUF_SUB0	: Indicates that the second field.
 * @BUF_SUB1	: Indicates that the third field.
 */
enum buf_pair {
	BUF_MASTER,
	BUF_SUB0,
	BUF_SUB1
};

/*
 * struct buf_core_entry - The entry of buffer.
 *
 * @key		: Record the actual physical address associated with vb.
 * @ref		: Decode buffer's reference count status.
 * @master_ref	: Point to master buf ref.
 * @node	: The position of the decoded buffer entry.
 * @h_node	: The node of hash list used for query buffer.
 * @state	: The state of the buffer to be used.
 * @user	: Indicates the user that holds the current entry.
 * @vb2		: The handle of v4l2 video buffer2.
 * @priv	: Record associated private data.
 * @ref_bit_map	: [0, 3]: dec ref, [4, 7]: ge2d ref,
 *		  [8, 11]: vpp ref, [12, 15]: vsink ref.
 * @sub_entry[2]: sub_entry[0]: The second filed buf entry;
 *		  sub_entry[1]: The third filed buf entry;
 * @master_entry: The first filed buf entry.
 * @pair	: Buf attributes: master, sub0, sub1.
 * @pair_state	: Buffer pairing status.
 * @index	: Aml buf index.
 * @inited	: The pairing is completed and enters the free queue.
 * @queued_mask	: Field buffer return times.
 */
struct buf_core_entry {
	ulong			key;
	ulong			phy_addr;
	atomic_t		ref;
	atomic_t		*master_ref;
	u32			dma_ref;
	struct list_head	node;
	struct hlist_node	h_node;
	enum buf_core_state	state;
	enum buf_core_user	user;
	enum buf_core_holder	holder;
	void			*vb2;
	void			*priv;
	u32			ref_bit_map;
	void			*sub_entry[2];
	void			*master_entry;
	enum buf_pair		pair;
	u32			pair_state;
	u32			index;
	u32			inited;
	u32			queued_mask; /* bit0: master; bit1: sub0; bit1: sub1*/
};

/*
 * struct buf_core_ops - The interface set of the buffer core operation.
 *
 * @get		: Get a free buffer from free queue.
 * @put		: Put an unused buffer to the free queue.
 * @get_ref	: Increase a reference count to the buffer and switch state to REF.
 * @put_ref	: Decrease a reference count to the buffer.
 * @done	: The done interface is called if the user finishes fill the data.
 * @fill	: The fill interface is called if the user consumes the data.
 * @ready_num	: Query the number of buffers in the free queue.
 * @empty	: Check whether the free queue is empty.
 */
struct buf_core_ops {
	void	(*get)(struct buf_core_mgr_s *, enum buf_core_user, struct buf_core_entry **, bool);
	void	(*put)(struct buf_core_mgr_s *, struct buf_core_entry *);
	void	(*get_ref)(struct buf_core_mgr_s *, struct buf_core_entry *);
	void	(*put_ref)(struct buf_core_mgr_s *, struct buf_core_entry *);
	int	(*done)(struct buf_core_mgr_s *, struct buf_core_entry *, enum buf_core_user);
	void	(*fill)(struct buf_core_mgr_s *, struct buf_core_entry *, enum buf_core_user);
	int	(*ready_num)(struct buf_core_mgr_s *);
	bool	(*empty)(struct buf_core_mgr_s *);
	void	(*vpp_cb)(struct buf_core_mgr_s *, struct buf_core_entry *);
	void	(*update_holder)(struct buf_core_mgr_s *, struct buf_core_entry *,
		enum buf_core_user, enum buf_direction direction);
};

/*
 * struct buf_core_mem_ops - The memory operation of entry.
 *
 * @alloc	: Allocates an instance of an entry.
 * @free	: Releases an instance of an entry.
 */
struct buf_core_mem_ops {
	int	(*alloc)(struct buf_core_mgr_s *, struct buf_core_entry **, void *);
	void	(*free)(struct buf_core_mgr_s *, struct buf_core_entry *);
};

/*
 * struct buf_core_mgr_s - Decoder buffer management structure.
 *
 * @id		: Instance ID of the buffer core manager context.
 * @name	: Name of the buffer core manager context.
 * @state	: State of the buffer core manager context.
 * @mutex	: Lock is used to ensure interface serialization.
 * @core_ref	: Reference count of the buffer core manager context.
 * @free_num	: The number of free buffers available.
 * @free_que	: Queue for storing free buffers.
 * @buf_num	: Record the serial number of buffer attached to the buffer manager.
 * @buf_table	: Used to store the attached buffer.
 * @config	: Interface Settings parameters to buffer manager.
 * @attach	: The interface is used to attach buffer to buffer manager.
 * @detach	: Interface for detach buffer to buffer manager.
 * @reset	: Interface used to reset the state of the buffer manager.
 * @prepare	: The interface is used for the preprocessing of buffer data.
 * @input	: The interface uses data input and is triggered after calling the interface fill.
 * @output	: The interface uses data output and is triggered after the interface is called done.
 * vpp_que	: Interact with DI mgr to notify the buffer that has been displayed back to the driver.
 * vpp_dque	: The decoded buffer is submitted to DI mgr for post-processing.
 * vpp_reset	: Used to reset the buffer information managed by DI mgr.
 * @mem_ops	: Set of interfaces for memory-related operations.
 * @buf_ops	: Set of interfaces for buffer operations.
 */
struct buf_core_mgr_s {
	int			id;
	char			*name;
	enum buf_core_mgr_state	state;
	struct mutex		mutex;
	struct kref		core_ref;

	int			free_num;
	struct list_head	free_que;

	int			buf_num;
	DECLARE_HASHTABLE(buf_table, BUF_HASH_BITS);

	void	(*config)(struct buf_core_mgr_s *, void *);
	int	(*attach)(struct buf_core_mgr_s *, ulong, ulong, void *);
	void	(*detach)(struct buf_core_mgr_s *, ulong);
	void	(*reset)(struct buf_core_mgr_s *);
	void	(*prepare)(struct buf_core_mgr_s *, struct buf_core_entry *);
	void	(*input)(struct buf_core_mgr_s *, struct buf_core_entry *, enum buf_core_user);
	int	(*output)(struct buf_core_mgr_s *, struct buf_core_entry *, enum buf_core_user);
	int	(*vpp_que)(struct buf_core_mgr_s *, struct buf_core_entry *);
	int	(*vpp_dque)(struct buf_core_mgr_s *, struct buf_core_entry *);
	int	(*vpp_reset)(struct buf_core_mgr_s *);
	void    (*external_process)(struct buf_core_mgr_s *, struct buf_core_entry *);
	int	(*get_pre_user) (struct buf_core_mgr_s *, struct buf_core_entry *, enum buf_core_user);
	int	(*get_next_user) (struct buf_core_mgr_s *, struct buf_core_entry *, enum buf_core_user);
	void	(*update)(struct buf_core_mgr_s *, struct buf_core_entry *, ulong, enum buf_pair);
	void	(*replace)(struct buf_core_mgr_s *, struct buf_core_entry *, void *);
	void	(*put_dma)(struct buf_core_mgr_s *);

	struct buf_core_mem_ops	mem_ops;
	struct buf_core_ops	buf_ops;
};

/*
 * buf_core_walk() - Iterate over the buffer entities used for debugging.
 *
 * @bc	: pointer to &struct buf_core_mgr_s buffer core manager context.
 *
 * Iterate over information about the used buffer entities
 * for debugging, including available buffers recorded in
 * the Free queue and managed buffers attached to the hash table,
 * and their states are iterated and printed out.
 */
ssize_t buf_core_walk(struct buf_core_mgr_s *bc, char *buf);

/*
 * buf_core_mgr_init() - buffer core management initialization.
 *
 * @bc		: pointer to &struct buf_core_mgr_s buffer core manager context.
 *
 * Used to initialize the buffer core manager context.
 *
 * Return	: returns zero on success; an error code otherwise
 */
int buf_core_mgr_init(struct buf_core_mgr_s *bc);

/*
 * buf_core_mgr_release() - buffer core management release.
 *
 * @bc		: pointer to &struct buf_core_mgr_s buffer core manager context.
 *
 * Used to release buffer core manager context
 */
void buf_core_mgr_release(struct buf_core_mgr_s *bc);

#endif //_AML_BUF_CORE_H_

