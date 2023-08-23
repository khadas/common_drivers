/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SYSFS_H
#define __SYSFS_H

void host_create_device_files(struct device *dev);
void host_destroy_device_files(struct device *dev);
void host_create_debugfs_files(struct host_module *host);
void host_destroy_debugfs_files(struct host_module *host);

#endif /*__SYSFS_H*/

