/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_SPI_MEM_H_
#define __AML_SPI_MEM_H_

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
