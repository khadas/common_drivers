/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_SPI_MEM_H_
#define __AML_SPI_MEM_H_
#include <linux/mtd/mtd.h>
#include <linux/spi/spi-mem.h>

/* spi nfc needed */
#define SPI_XFER_OOB           BIT(4)
#define SPI_XFER_RAW           BIT(5)
#define SPI_XFER_AUTO_OOB      BIT(6)
#define SPI_XFER_OOB_ONLY      BIT(7)
#define SPI_XFER_NFC_MASK_FLAG                                          \
		(SPI_XFER_OOB | SPI_XFER_RAW |                          \
		SPI_XFER_AUTO_OOB | SPI_XFER_OOB_ONLY)

void spi_mem_set_xfer_flag(u8 flag);
u8 spi_mem_get_xfer_flag(void);
void spi_mem_umask_xfer_flags(void);
void spi_mem_set_mtd(struct mtd_info *mtd);
struct mtd_info *spi_mem_get_mtd(void);
int meson_spi_mem_exec_op(struct spi_mem *mem,
			  const struct spi_mem_op *op);
ssize_t meson_spi_mem_dirmap_write(struct spi_mem_dirmap_desc *desc,
				   u64 offs, size_t len,
				   const void *buf);
int meson_spi_mem_poll_status(struct spi_mem *mem,
			      const struct spi_mem_op *op,
			      u16 mask,
			      u16 match,
			      unsigned long initial_delay_us,
			      unsigned long polling_delay_us,
			      u16 timeout_ms);
ssize_t meson_spi_mem_dirmap_read(struct spi_mem_dirmap_desc *desc,
				  u64 offs,
				  size_t len,
				  void *buf);
#endif
