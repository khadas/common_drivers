/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _OPTEE_LOG_H_
#define _OPTEE_LOG_H_

#define LOGGER_SHM_ADDR_MAGIC           0x1001

#define DEF_LOGGER_SHM_SIZE             (256 * 1024)

int optee_log_init(void *va);

void optee_log_uninit(void);

#endif//_OPTEE_LOG_H_
