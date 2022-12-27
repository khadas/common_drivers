/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DM_BOW_H__
#define __DM_BOW_H__

#ifdef CONFIG_AMLOGIC_DM_BOW
int bow_sysfs_create(struct dm_table *table,
			  struct dm_ioctl *param, size_t param_size);
#else

int bow_sysfs_create(struct dm_table *table,
			  struct dm_ioctl *param, size_t param_size)
{
	return 0;
}
#endif
#endif /* __DM_BOW_H__ */
