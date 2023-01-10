// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#ifdef CONFIG_MTD_SPI_NOR
#include <linux/mtd/spi-nor.h>
#endif
#include <linux/atomic.h>

#define CONFIG_SPIFC_HWCAPS_DUAL_QUAD

#define SPIFC_AHB_BUF_CACHE_SIZE	(4096 + 256)
#define CONFIG_ENABLE_AHB_MODE		1
//#define CONFIG_SPIFC_DEBUG

/* register map */

#define REG_MAX			0x400

//AHB domain register.
#define SPIFC_AHB_CTRL		(0x0000 << 2)
	#define AHB_BUS_EN				BIT(31)
	#define DECERR_EN				BIT(30)
	#define FORCE_INCR				BIT(29)
	#define CWF_EN					BIT(19)
	#define RDBUF_SIZE				GENMASK(18, 17)
	#define AHB_MASTER_EN				BIT(16)
	#define CLR_HRDATA2				BIT(14)
	#define CLR_HRDATA1				BIT(13)
	#define CLR_HRDATA0				BIT(12)
	#define AHB_CLEAN_HRDATA_BUF_SHIFT		12

#define SPIFC_CLK_CTRL		(0x0001 << 2)
	#define ASYNC_BUFFER_AHB_CLK_DISABLE		BIT(14) // 1-disable
	#define AHB2SPI_AHB_CLK_DISABLE			BIT(13) // 1-disable
	#define AHB_ARB_AHB_CLK_DISABLE			BIT(12) // 1-disable
	#define ASYNC_BUFFER_AHB_CLK_AUTO_GATING_ENABLE	BIT(10) // 1-enable
	#define AHB2SPI_AHB_CLK_AUTO_GATING_ENABLE	BIT(9)  // 1-enable
	#define AHB_ARB_AHB_CLK_AUTO_GATING_ENABLE	BIT(8)  // 1-enable
	#define ASYNC_BUFFER_AHB_CLK_DOMAIN_RESET	BIT(2)  // 1-reset
	#define AHB2SPI_AHB_CLK_DOMAIN_RESET		BIT(1)  // 1-reset

#define SPIFC_SEC_CTRL		(0x0002 << 2)
	#define ADDRESS_SEC_RANGE_ENABLE		BIT(31)
	#define RANGE6_ENABLE				BIT(14)
	#define RANGE5_ENABLE				BIT(13)
	#define RANGE4_ENABLE				BIT(12)
	#define RANGE3_ENABLE				BIT(11)
	#define RANGE2_ENABLE				BIT(10)
	#define RANGE1_ENABLE				BIT(9)
	#define RANGE0_ENABLE				BIT(8)
	#define SPI_CLK_REG_SEC_CTRL			GENMASK(7, 4)
	#define AHB_CLK_DOMAIN_REG_SEC_CTRL		GENMASK(3, 0)

#define SPIFC_RANGE0_STA	(0x0010 << 2)
#define SPIFC_RANGE0_EDA	(0x0011 << 2)
#define SPIFC_RANGE0_CTRL	(0x0012 << 2)
#define SPIFC_RANGE1_STA	(0x0013 << 2)
#define SPIFC_RANGE1_EDA	(0x0014 << 2)
#define SPIFC_RANGE1_CTRL	(0x0015 << 2)
#define SPIFC_RANGE2_STA	(0x0016 << 2)
#define SPIFC_RANGE2_EDA	(0x0017 << 2)
#define SPIFC_RANGE2_CTRL	(0x0018 << 2)
#define SPIFC_RANGE3_STA	(0x0019 << 2)
#define SPIFC_RANGE3_EDA	(0x001a << 2)
#define SPIFC_RANGE3_CTRL	(0x001b << 2)
#define SPIFC_RANGE4_STA	(0x001c << 2)
#define SPIFC_RANGE4_EDA	(0x001d << 2)
#define SPIFC_RANGE4_CTRL	(0x001e << 2)
#define SPIFC_RANGE5_STA	(0x001f << 2)
#define SPIFC_RANGE5_EDA	(0x0021 << 2)
#define SPIFC_RANGE5_CTRL	(0x0020 << 2)
#define SPIFC_RANGE6_STA	(0x0022 << 2)
#define SPIFC_RANGE6_EDA	(0x0023 << 2)
#define SPIFC_RANGE6_CTRL	(0x0024 << 2)
#define SPIFC_RANGE7_CTRL	(0x0025 << 2)

#define SPIFC_AHB_WTCH_CTRL	(0x0026 << 2)
	#define AHB_BUS_WDT				GENMASK(15, 0)

#define SPIFC_SEC_VIO0		(0x0027 << 2)
#define SPIFC_SEC_VIO1		(0x0028 << 2)
	#define AHB_VIO_STATUS				BIT(31)
	#define AHB_VIO_HMASTER				GENMASK(6, 5)
	#define AHB_VIO_HPROT				GENMASK(4, 2)
	#define AHB_VIO_HNONSEC				BIT(1)
	#define AHB_VIO_HWRITE				BIT(0)

#define SPIFC_AHB_STS		(0x0029 << 2)
	#define AHB_STATUS				BIT(31)
	#define AHB_DATA_CYCLE_STATUS			BIT(30)
	#define AHB_BUS_SPI_STATUS			BIT(29)

//SPI cts_spi_clk domain register.
#define SPIFC_USER_CTRL0	(0x0080 << 2)
	#define USER_REQUEST_ENABLE			BIT(31) //W1R0
	#define USER_REQUEST_FINISH			BIT(30) //W0R1
	#define USER_DATA_UPDATED			BIT(0) //W0R1

#define SPIFC_USER_CTRL1	(0x0081 << 2)
	#define USER_CMD_ENABLE				BIT(30)
	#define USER_CMD_MODE				GENMASK(29, 28)
	#define USER_CMD_CODE				GENMASK(27, 20)
	#define USER_ADDR_ENABLE			BIT(19)
	#define USER_ADDR_MODE				GENMASK(18, 17)
	#define USER_ADDR_BYTES				GENMASK(16, 15)
	#define USER_DOUT_ENABLE			BIT(14)
	#define USER_DOUT_AES_ENABLE			BIT(13)
	#define USER_DOUT_SRC				BIT(12)
	#define USER_DOUT_MODE				GENMASK(11, 10)
	#define USER_DOUT_BYTES				GENMASK(9, 0)
	#define USER_CMD_MODE_SHIFT			28
	#define USER_CMD_CODE_SHIFT			20
	#define USER_ADDR_MODE_SHIFT			17
	#define USER_ADDR_BYTES_SHIFT			15
	#define USER_DOUT_MODE_SHIFT			10
	#define USER_DOUT_BYTES_SHIFT			0

#define SPIFC_USER_CTRL2	(0x0082 << 2)
	#define USER_DUMMY_ENABLE			BIT(31)
	#define USER_DUMMY_MODE				GENMASK(30, 29)
	#define USER_DUMMY_CLK_CYCLES			GENMASK(28, 23)
	#define USER_DUMMY_MODE_SHIFT			29
	#define USER_DUMMY_CLK_CYCLES_SHIFT		23

#define SPIFC_USER_CTRL3	(0x0083 << 2)
	#define USER_DIN_ENABLE				BIT(31)
	#define USER_DIN_DEST				BIT(30)
	#define USER_DIN_AES_ENABLE			BIT(29)
	#define USER_DIN_MODE				GENMASK(28, 27)
	#define USER_DIN_BYTES				GENMASK(25, 16)
	#define USER_DIN_MODE_SHIFT			27
	#define USER_DIN_BYTES_SHIFT			16

#define SPIFC_USER_ADDR		(0x0084 << 2)

#define SPIFC_AHB_REQ_CTRL	(0x0085 << 2)
	#define AHB_REQ_ENABLE				BIT(31)
	#define AHB_CMD_ENABLE				BIT(30)
	#define AHB_CMD_MODE				GENMASK(29, 28)
	#define AHB_CMD_CODE				GENMASK(27, 20)
	#define AHB_ADDR_ENABLE				BIT(19)
	#define AHB_ADDR_MODE				GENMASK(18, 17)
	#define AHB_ADDR_WIDTH				GENMASK(16, 15)
	#define AHB_DIN_MODE				GENMASK(9, 8)
	#define AHB_DIN_AES_ENABLE			BIT(7)
	#define AHB_DIN_BYTES				GENMASK(1, 0)
	#define AHB_CMD_MODE_SHIFT			28
	#define AHB_CMD_CODE_SHIFT			20
	#define AHB_ADDR_MODE_SHIFT			17
	#define AHB_ADDR_BYTES_SHIFT			15
	#define AHB_DIN_MODE_SHIFT			8

#define SPIFC_AHB_REQ_CTRL1	(0x0086 << 2)
	#define AHB_DUMMY_ENABLE			BIT(31)
	#define AHB_DUMMY_MODE				GENMASK(30, 29)
	#define AHB_DUMMY_CLK_CYCLES			GENMASK(28, 23)
	#define AHB_DUMMY_OUTPUT_DATA			GENMASK(15, 0)
	#define AHB_DUMMY_CYCLE_SHIFT			23

#define SPIFC_AHB_REQ_CTRL2	(0x0087 << 2)

#define SPIFC_ACTIMING0		(0x0088 << 2)
	#define TSLCH					GENMASK(31, 30)
	#define TCLSH					GENMASK(29, 28)
	#define TSHWL					GENMASK(20, 16)
	#define TSHSL2					GENMASK(15, 12)
	#define TSHSL1					GENMASK(11, 8)
	#define TWHSL					GENMASK(7, 0)

#define SPIFC_ACTIMING1		(0x0089 << 2)
	#define D2_WPN_ENABLE				BIT(31)
	#define D3_HOLDN_ENABLE				BIT(29)
	#define DTR_MODE				BIT(8)
	#define CLOCK_TURN_AROUND_DELAY			GENMASK(6, 4)

#define SPIFC_ACTIMING2		(0x008a << 2)
	#define SP_CLK_INPUT_PIN_ENABLE			BIT(31)
	#define SP_CLK_INPUT_PIN_DELAY			GENMASK(3, 0)

#define SPIFC_DBUF_CTRL		(0x0090 << 2)
	#define DBUF_DIR				BIT(31)
	#define DBUF_AUTO_UPDATE_ADDR			BIT(30)
	#define DBUF_ADDR				GENMASK(7, 0)

#define SPIFC_DBUF_DATA		(0x0091 << 2)

#define SPIFC_USER_DBUF_ADDR	(0x0092 << 2)

#define SPIFC_FLASH_STATUS	(0x00a0 << 2)

#define SPIFC_STATUS		(0x00a1 << 2)
	#define AES_KEY_VALID				BIT(1)
	#define SPI_CTRL_STATE				BIT(0)

#define SPIFC_CTRL		(0x00a2 << 2)
	#define DBUF_MEMPD				GENMASK(15, 14)
	#define ASYNC_BUF_CLK_DISABLE			BIT(13)
	#define SPI_CLK_DISABLE				BIT(12)
	#define ASYNC_BUF_CLK_AUTO_GATE_ENABLE		BIT(9)
	#define SPI_CLK_AUTO_GATE_ENABLE		BIT(8)
	#define SPI_INTERFACE_RESET			BIT(2)
	#define ASYNC_BUF_RESET				BIT(1)
	#define SPI_RESET				BIT(0)

#define SPIFC_BUFFER_SIZE	512
#define convert_nbits(n)	(fls(n) - 1)

/**
 * Standard SPI NAND flash commands
 */
#define SPINAND_CMD_PROG_LOAD			0x02
#define SPINAND_CMD_PROG_LOAD_RDM_DATA		0x84
#define SPINAND_CMD_PROG_LOAD_X4		0x32
#define SPINAND_CMD_PROG_LOAD_RDM_DATA_X4	0x34

/* flash std/dual/quad read command */
#define FCMD_READ                                       0x03
#define FCMD_READ_FAST                                  0x0b
#define FCMD_READ_DUAL_OUT                              0x3b
#define FCMD_READ_QUAD_OUT                              0x6b
#define FCMD_READ_DUAL_IO                               0xbb
#define FCMD_READ_QUAD_IO                               0xeb

void __iomem *spifc_ahb_map_addr;

/**
 * struct meson_spifc
 * @master:	the SPI master
 * @regmap:	regmap for device registers
 * @clk:	input clock of the built-in baud rate generator
 * @device:	the device structure
 */
struct meson_spifc {
#ifdef CONFIG_MTD_SPI_NOR
	struct spi_nor *nor;
	/* used by nor core */
	struct mutex lock;
	u32 clk_rate;
#endif
	struct spi_master *master;
	struct regmap *regmap;
	struct clk *clk;
	struct device *dev;
	u8 cmd;
	u16 cmd_nbits	:2;
	u32 addr;
	u16 addr_nbits	:2;
	u8 addr_len;		// in bytes
	u16 dummy_nbits	:2;
	u8 dummy_clk_cycles;	// in bits
	u16 dout_nbits	:2;
	u16 din_nbits	:2;
	u32 speed;
	void __iomem *base;
};

static const struct regmap_config spifc_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = REG_MAX,
};

#ifdef CONFIG_SPIFC_DEBUG
#define spifc_dbg(fmt, args...) {pr_info("spifc: " fmt, ## args); }

static void meson_spifc_dump(struct meson_spifc *spifc)
{
	u32 buf[7];

	regmap_read(spifc->regmap, SPIFC_USER_CTRL0, &buf[0]);
	regmap_read(spifc->regmap, SPIFC_USER_CTRL1, &buf[1]);
	regmap_read(spifc->regmap, SPIFC_USER_CTRL2, &buf[2]);
	regmap_read(spifc->regmap, SPIFC_USER_CTRL3, &buf[3]);
	regmap_read(spifc->regmap, SPIFC_USER_ADDR, &buf[4]);
	regmap_read(spifc->regmap, SPIFC_DBUF_CTRL, &buf[5]);
	regmap_read(spifc->regmap, SPIFC_USER_DBUF_ADDR, &buf[6]);
	pr_info("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
}

static void meson_spifc_ahb_dump(struct meson_spifc *spifc)
{
	u32 buf[4];

	regmap_read(spifc->regmap, SPIFC_AHB_REQ_CTRL, &buf[0]);
	regmap_read(spifc->regmap, SPIFC_AHB_REQ_CTRL1, &buf[1]);
	regmap_read(spifc->regmap, SPIFC_AHB_REQ_CTRL2, &buf[2]);
	regmap_read(spifc->regmap, SPIFC_AHB_CTRL, &buf[3]);
	pr_info("0x%x, 0x%x, 0x%x, 0x%x\n",
		buf[0], buf[1], buf[2], buf[3]);
}
#else
#define spifc_dbg(fmt, args...)
#define meson_spifc_dump(spifc)
#endif

/**
 * meson_spifc_wait_ready() - wait for the current operation to terminate
 * @spifc:	the Meson SPI device
 * Return:	0 on success, a negative value on error
 */
static int meson_spifc_start_then_wait_ready(struct meson_spifc *spifc,
					     bool read)
{
	unsigned long deadline = jiffies + msecs_to_jiffies(200);
	u32 data;

	meson_spifc_dump(spifc);

	/* clear transition done bit, and start transfer */
	regmap_write(spifc->regmap, SPIFC_USER_CTRL0, USER_REQUEST_ENABLE);

	do {
		regmap_read(spifc->regmap, SPIFC_USER_CTRL0, &data);
		if ((data & USER_REQUEST_FINISH) &&
		    (!read || (data & USER_DATA_UPDATED))) {
			return 0;
		}
		cond_resched();
	} while (!time_after(jiffies, deadline));

	return -ETIMEDOUT;
}

/**
 * meson_spifc_drain_buffer() - copy data from device buffer to memory
 * @spifc:	the Meson SPI device
 * @buf:	the destination buffer
 * @len:	number of bytes to copy
 */
static void meson_spifc_drain_buffer(struct meson_spifc *spifc, u8 *buf,
				     int len)
{
	void __iomem *dbuf = spifc->base + SPIFC_DBUF_DATA;
	u32 data, *p = (u32 *)buf;
	int i = len >> 2;

	writel_relaxed(DBUF_AUTO_UPDATE_ADDR,
		       spifc->base + SPIFC_DBUF_CTRL);

	while (i--)
		*p++ = readl_relaxed(dbuf);

	len %= 4;
	if (len) {
		data = readl_relaxed(dbuf);
		memcpy((void *)p, (void *)&data, len);
	}
}

/**
 * meson_spifc_fill_buffer() - copy data from memory to device buffer
 * @spifc:	the Meson SPI device
 * @buf:	the source buffer
 * @len:	number of bytes to copy
 */
static void meson_spifc_fill_buffer(struct meson_spifc *spifc, const u8 *buf,
				    int len)
{
	void __iomem *dbuf = spifc->base + SPIFC_DBUF_DATA;
	u32 data, *p = (u32 *)buf;
	int i = len >> 2;

	writel_relaxed(DBUF_DIR | DBUF_AUTO_UPDATE_ADDR,
		       spifc->base + SPIFC_DBUF_CTRL);

	while (i--)
		writel_relaxed(*p++, dbuf);

	len %= 4;
	if (len) {
		memcpy((void *)&data, (void *)p, len);
		writel_relaxed(data, dbuf);
	}
}

/**
 * meson_spifc_setup_speed() - program the clock divider
 * @spifc:	the Meson SPI device
 * @speed:	desired speed in Hz
 */
static int meson_spifc_setup_speed(struct meson_spifc *spifc, u32 speed)
{
	int ret = 0;

	if (spifc->speed != speed) {
		ret = clk_set_rate(spifc->clk, speed);
		if (ret)
			return ret;
		spifc->speed = speed;
	}

	return ret;
}

static int meson_spifc_ahb_enable(struct meson_spifc *spifc)
{
	unsigned int val0, val1;
	unsigned char cmd;
	u16 bits;
	u32 regv;
	u32 data;
	unsigned long deadline = jiffies + msecs_to_jiffies(200);

	cmd = spifc->cmd;
	bits = spifc->addr_len ? (spifc->addr_len - 1) : 0;

	val0 = AHB_CMD_ENABLE | AHB_ADDR_ENABLE |
			(cmd << AHB_CMD_CODE_SHIFT) | (bits << AHB_ADDR_BYTES_SHIFT);
	val1 = AHB_DUMMY_ENABLE | (8 << AHB_DUMMY_CYCLE_SHIFT);

	if (cmd == FCMD_READ_DUAL_OUT)
		val0 |= ((1 << AHB_DIN_MODE_SHIFT));
	else if (cmd == FCMD_READ_QUAD_OUT)
		val0 |= ((2 << AHB_DIN_MODE_SHIFT));

	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL, val0);
	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL1, val1);
	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL2, 0);
	regmap_write(spifc->regmap, SPIFC_USER_DBUF_ADDR, 0);
	regmap_write(spifc->regmap, SPIFC_AHB_CTRL, AHB_BUS_EN);
	/* clean the HRDATA buffer */
	regmap_read(spifc->regmap, SPIFC_AHB_CTRL, &regv);
	regv |= (7 << AHB_CLEAN_HRDATA_BUF_SHIFT);
	regmap_write(spifc->regmap, SPIFC_AHB_CTRL, regv);

	do {
		regmap_read(spifc->regmap, SPIFC_AHB_CTRL, &data);
		if (!(data & CLR_HRDATA0) && !(data & CLR_HRDATA1) && !(data & CLR_HRDATA2))
			return 0;
		cond_resched();
	} while (!time_after(jiffies, deadline));

	return -ETIMEDOUT;
}

static void meson_spifc_ahb_disable(struct meson_spifc *spifc)
{
	u32 regv;

	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL, 0);
	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL1, 0);
	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL2, 0);
	regmap_read(spifc->regmap, SPIFC_AHB_CTRL, &regv);
	regv &= ~(AHB_BUS_EN);
	regmap_write(spifc->regmap, SPIFC_AHB_CTRL, regv);
}

static void spifc_user_ahb_din(struct meson_spifc *spifc, u8 *buf, int len)
{
	void __iomem *dbuf = spifc_ahb_map_addr + spifc->addr;
	u32 regv;
	u32 data, *p = (u32 *)buf;
	int i = len >> 2;
	u32 *tmp = (u32 *)dbuf;

	regmap_read(spifc->regmap, SPIFC_AHB_REQ_CTRL, &regv);
	regv |= AHB_REQ_ENABLE;
	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL, regv);

	while (i--)
		*p++ = readl_relaxed(tmp++);

	len %= 4;
	if (len) {
		data = readl_relaxed(tmp++);
		memcpy((void *)p, (void *)&data, len);
	}
}

static void meson_spifc_user_init(struct meson_spifc *spifc)
{
	spifc->cmd_nbits = 0;
	spifc->addr_len = 0;
	spifc->addr_nbits = 0;
	spifc->dummy_clk_cycles = 0;
	spifc->dummy_nbits = 0;
	spifc->din_nbits = 0;
	spifc->dout_nbits = 0;
	regmap_write(spifc->regmap, SPIFC_USER_CTRL0, 0);
	regmap_write(spifc->regmap, SPIFC_USER_CTRL1, 0);
	regmap_write(spifc->regmap, SPIFC_USER_CTRL2, 0);
	regmap_write(spifc->regmap, SPIFC_USER_CTRL3, 0);
}

static void meson_spifc_set_cmd(struct meson_spifc *spifc)
{
	u32 regv;

	regmap_read(spifc->regmap, SPIFC_USER_CTRL1, &regv);
	regv &= ~(USER_CMD_MODE | USER_CMD_CODE);
	regv |= (u32)(spifc->cmd_nbits << USER_CMD_MODE_SHIFT)
		| (u32)(spifc->cmd << USER_CMD_CODE_SHIFT)
		| USER_CMD_ENABLE;
	regmap_write(spifc->regmap, SPIFC_USER_CTRL1, regv);
}

static void meson_spifc_set_addr(struct meson_spifc *spifc)
{
	u32 regv;

	/* set address */
	regmap_write(spifc->regmap, SPIFC_USER_ADDR, spifc->addr);

	/* set nbits and address len, enable address-stage */
	regmap_read(spifc->regmap, SPIFC_USER_CTRL1, &regv);
	regv &= ~(USER_ADDR_MODE | USER_ADDR_BYTES);
	regv |= (u32)(spifc->addr_nbits << USER_ADDR_MODE_SHIFT) & USER_ADDR_MODE;
	regv |= (u32)((spifc->addr_len - 1) << USER_ADDR_BYTES_SHIFT)
		& USER_ADDR_BYTES;
	regv |=	USER_ADDR_ENABLE;
	regmap_write(spifc->regmap, SPIFC_USER_CTRL1, regv);
}

static void meson_spifc_set_dummy(struct meson_spifc *spifc)
{
	u32 regv;

	regmap_read(spifc->regmap, SPIFC_USER_CTRL2, &regv);
	regv &= ~(USER_DUMMY_MODE | USER_DUMMY_CLK_CYCLES);
	regv |= (u32)(spifc->dummy_nbits << USER_DUMMY_MODE_SHIFT)
		| (u32)(spifc->dummy_clk_cycles << USER_DUMMY_CLK_CYCLES_SHIFT)
		| USER_DUMMY_ENABLE;

	regmap_write(spifc->regmap, SPIFC_USER_CTRL2, regv);
}

/**
 * meson_spifc_tx() - transfer a chunk of data
 * @spifc:	the Meson SPI device
 * @xfer:	the current SPI transfer
 * @offset:	offset of the data to transfer
 * @len:	length of the data to transfer
 * Return:	0 on success, a negative value on error
 */
static int meson_spifc_dout(struct meson_spifc *spifc,
			    u8 *buf, int offset, int len)
{
	u32 regv;

	/* write data to DBUF */
	meson_spifc_fill_buffer(spifc, buf + offset, len);

	/* set DOUT address 0*/
	regmap_write(spifc->regmap, SPIFC_USER_DBUF_ADDR, 0);

	/* set DOUT len and mode */
	regmap_read(spifc->regmap, SPIFC_USER_CTRL1, &regv);
	regv &= ~(USER_DOUT_MODE | USER_DOUT_BYTES);
	regv |= (len ? USER_DOUT_ENABLE : 0)
		| (u32)(spifc->dout_nbits << USER_DOUT_MODE_SHIFT)
		| (u32)(len << USER_DOUT_BYTES_SHIFT);
	regmap_write(spifc->regmap, SPIFC_USER_CTRL1, regv);

	/* start DOUT */
	return meson_spifc_start_then_wait_ready(spifc, 0);
}

/**
 * meson_spifc_txrx() - transfer a chunk of data
 * @spifc:	the Meson SPI device
 * @xfer:	the current SPI transfer
 * @offset:	offset of the data to transfer
 * @len:	length of the data to transfer
 * Return:	0 on success, a negative value on error
 */
static int meson_spifc_din(struct meson_spifc *spifc,
			   u8 *buf, int offset, int len)
{
	int ret;

	/* set DIN address 0*/
	regmap_write(spifc->regmap, SPIFC_USER_DBUF_ADDR, 0);

	/* set DIN len and mode */
	regmap_write(spifc->regmap, SPIFC_USER_CTRL3,
		     (u32)(len ? USER_DIN_ENABLE : 0)
		     | (u32)(spifc->din_nbits << USER_DIN_MODE_SHIFT)
		     | (u32)(len << USER_DIN_BYTES_SHIFT));

	/* start DIN */
	ret = meson_spifc_start_then_wait_ready(spifc, 1);
	if (!ret)
		/* read data from DBUF */
		meson_spifc_drain_buffer(spifc, buf + offset, len);

	return ret;
}

/**
 * meson_spifc_transfer_one() - perform a single transfer
 * @master:	the SPI master
 * @spi:	the SPI device
 * @xfer:	the current SPI transfer
 * Return:	0 on success, a negative value on error
 */
static int meson_spifc_transfer_one(struct spi_master *master,
				    struct spi_device *spi,
				    struct spi_transfer *xfer)
{
	struct meson_spifc *spifc = spi_master_get_devdata(master);
	int i, len, done = 0, ret = 0;
	u8 *p = (u8 *)xfer->tx_buf;
	bool last_xfer = spi_transfer_is_last(master, xfer);
	u8 stage;

	ret = meson_spifc_setup_speed(spifc, xfer->speed_hz);
	if (ret)
		return ret;

	stage = (xfer->tx_buf && !xfer->rx_buf) ? xfer->rx_nbits : 0;
	if (stage == 1) {
		meson_spifc_user_init(spifc);
		spifc->cmd = *p;
		spifc->cmd_nbits = convert_nbits(xfer->tx_nbits);
		spifc_dbg("cmd=0x%x\n", spifc->cmd);
		if (!CONFIG_ENABLE_AHB_MODE) {
			meson_spifc_set_cmd(spifc);
		} else {
			if ((spifc->cmd != FCMD_READ &&
				spifc->cmd != FCMD_READ_FAST &&
				spifc->cmd != FCMD_READ_DUAL_OUT &&
				spifc->cmd != FCMD_READ_QUAD_OUT &&
				spifc->cmd != FCMD_READ_DUAL_IO &&
				spifc->cmd != FCMD_READ_QUAD_IO))
				meson_spifc_set_cmd(spifc);
		}
		if (last_xfer)
			ret = meson_spifc_start_then_wait_ready(spifc, 0);
	} else if (stage == 2) {
		spifc->addr = 0;
		for (i = 0; i < xfer->len; i++) {
			spifc->addr <<= 8;
			spifc->addr |= p[i];
		}
		spifc->addr_nbits = convert_nbits(xfer->tx_nbits);
		spifc->addr_len = xfer->len;
		spifc_dbg("addr=0x%x, len=%d\n", spifc->addr, spifc->addr_len);
		if (!CONFIG_ENABLE_AHB_MODE) {
			meson_spifc_set_addr(spifc);
		} else {
			if ((spifc->cmd != FCMD_READ &&
				spifc->cmd != FCMD_READ_FAST &&
				spifc->cmd != FCMD_READ_DUAL_OUT &&
				spifc->cmd != FCMD_READ_QUAD_OUT &&
				spifc->cmd != FCMD_READ_DUAL_IO &&
				spifc->cmd != FCMD_READ_QUAD_IO))
				meson_spifc_set_addr(spifc);
		}
		if (last_xfer)
			ret = meson_spifc_start_then_wait_ready(spifc, 0);
	} else if (stage == 3) {
		spifc->dummy_nbits = convert_nbits(xfer->tx_nbits);
		spifc->dummy_clk_cycles = xfer->len << 3;
		spifc_dbg("dummy: clk_cycles=%d\n", spifc->dummy_clk_cycles);
		if (!CONFIG_ENABLE_AHB_MODE) {
			meson_spifc_set_dummy(spifc);
		} else {
			if ((spifc->cmd != FCMD_READ &&
				spifc->cmd != FCMD_READ_FAST &&
				spifc->cmd != FCMD_READ_DUAL_OUT &&
				spifc->cmd != FCMD_READ_QUAD_OUT &&
				spifc->cmd != FCMD_READ_DUAL_IO &&
				spifc->cmd != FCMD_READ_QUAD_IO))
				meson_spifc_set_dummy(spifc);
		}
		if (last_xfer)
			ret = meson_spifc_start_then_wait_ready(spifc, 0);
	} else if (xfer->tx_buf) {
		spifc->dout_nbits = convert_nbits(xfer->tx_nbits);
		spifc_dbg("dout: len=%d\n", xfer->len);
		while (done < xfer->len && !ret) {
			len = min_t(int, xfer->len - done, SPIFC_BUFFER_SIZE);
			meson_spifc_set_cmd(spifc);
			if (spifc->addr_len)
				meson_spifc_set_addr(spifc);
			if (spifc->dummy_clk_cycles)
				meson_spifc_set_dummy(spifc);
			ret = meson_spifc_dout(spifc, (u8 *)xfer->tx_buf,
					       done, len);
			spifc_dbg("  cmd=0x%x, len=%d, addr=0x%x\n",
				  spifc->cmd, len, spifc->addr);
			done += len;
			spifc->addr += len;
			if (spifc->cmd == SPINAND_CMD_PROG_LOAD)
				spifc->cmd = SPINAND_CMD_PROG_LOAD_RDM_DATA;
			else if (spifc->cmd == SPINAND_CMD_PROG_LOAD_X4)
				spifc->cmd = SPINAND_CMD_PROG_LOAD_RDM_DATA_X4;
		}
	} else if (xfer->rx_buf) {
		spifc->din_nbits = convert_nbits(xfer->rx_nbits);
		spifc_dbg("din: len=%d\n", xfer->len);
		if ((spifc->cmd == FCMD_READ ||
			spifc->cmd == FCMD_READ_FAST ||
			spifc->cmd == FCMD_READ_DUAL_OUT ||
			spifc->cmd == FCMD_READ_QUAD_OUT ||
			spifc->cmd == FCMD_READ_DUAL_IO ||
			spifc->cmd == FCMD_READ_QUAD_IO) && CONFIG_ENABLE_AHB_MODE) {
			ret = meson_spifc_ahb_enable(spifc);
			if (ret)
				pr_info("enable spifc ahb failed!\n");
			spifc_user_ahb_din(spifc, (u8 *)xfer->rx_buf, xfer->len);
			meson_spifc_ahb_disable(spifc);
		} else {
			while (done < xfer->len && !ret) {
				len = min_t(int, xfer->len - done, SPIFC_BUFFER_SIZE);
				meson_spifc_set_cmd(spifc);
				if (spifc->addr_len)
					meson_spifc_set_addr(spifc);
				if (spifc->dummy_clk_cycles)
					meson_spifc_set_dummy(spifc);
				ret = meson_spifc_din(spifc, (u8 *)xfer->rx_buf,
							done, len);
				spifc_dbg("  cmd=0x%x, len=%d, addr=0x%x\n",
					spifc->cmd, len, spifc->addr);
				done += len;
				spifc->addr += len;
			}
		}
	}
	return ret;
}

#ifdef CONFIG_MTD_SPI_NOR
static int meson_snor_prep(struct spi_nor *nor, enum spi_nor_ops ops)
{
	struct meson_spifc *spifc = nor->priv;

	mutex_lock(&spifc->lock);

	return 0;
}

static void meson_snor_unprep(struct spi_nor *nor, enum spi_nor_ops ops)
{
	struct meson_spifc *spifc = nor->priv;

	mutex_unlock(&spifc->lock);
}

static int meson_snor_read_reg(struct spi_nor *nor, u8 opcode,
			       u8 *buf, int len)
{
	struct meson_spifc *spifc = nor->priv;

	spifc_dbg("read_reg: cmd=0x%x, len=%d\n", opcode, len);

	if (len > SPIFC_BUFFER_SIZE)
		return -ENOBUFS;

	meson_spifc_user_init(spifc);
	spifc->cmd = opcode;
	meson_spifc_set_cmd(spifc);
	return meson_spifc_din(spifc, buf, 0, len);
}

ssize_t meson_snor_read(struct spi_nor *nor, loff_t from,
			size_t len, u_char *read_buf)
{
	struct meson_spifc *spifc = nor->priv;
	int current_len, done = 0, ret = 0;

	spifc_dbg("read: cmd=0x%x, len=%d, from=0x%x\n",
		  nor->read_opcode, (u32)len, (u32)from);

	meson_spifc_user_init(spifc);
	spifc->cmd = nor->read_opcode;
	spifc->addr = from;
	spifc->addr_len = nor->addr_width; /* 3 */
	spifc->dummy_clk_cycles = nor->read_dummy;
	spifc->din_nbits = convert_nbits(SNOR_PROTO_DATA(nor->read_proto));
	while (done < len && !ret) {
		current_len = min_t(int, len - done, SPIFC_BUFFER_SIZE);
		meson_spifc_set_cmd(spifc);
		meson_spifc_set_addr(spifc);
		if (spifc->dummy_clk_cycles)
			meson_spifc_set_dummy(spifc);
		ret = meson_spifc_din(spifc, read_buf, done, current_len);
		if (ret)
			break;
		done += current_len;
		spifc->addr += current_len;
	}

	return ret ? 0 : len;
}

static int meson_snor_write_reg(struct spi_nor *nor, u8 opcode,
				u8 *buf, int len)
{
	struct meson_spifc *spifc = nor->priv;

	spifc_dbg("write_reg: cmd=0x%x, len=%d\n", opcode, len);

	if (len > SPIFC_BUFFER_SIZE)
		return -ENOBUFS;

	meson_spifc_user_init(spifc);
	spifc->cmd = opcode;
	meson_spifc_set_cmd(spifc);
	return meson_spifc_dout(spifc, buf, 0, len);
}

static ssize_t meson_snor_write(struct spi_nor *nor, loff_t to,
				size_t len, const u_char *write_buf)
{
	struct meson_spifc *spifc = nor->priv;
	int current_len, done = 0, ret = 0;

	spifc_dbg("write: cmd=0x%x, len=%d, to=0x%x\n",
		  nor->program_opcode, (u32)len, (u32)to);

	meson_spifc_user_init(spifc);
	spifc->cmd = nor->program_opcode;
	spifc->addr = to;
	spifc->addr_len = nor->addr_width; /* 3 */
	spifc->dout_nbits = convert_nbits(SNOR_PROTO_DATA(nor->write_proto));
	while (done < len && !ret) {
		current_len = min_t(int, len - done, SPIFC_BUFFER_SIZE);
		meson_spifc_set_cmd(spifc);
		meson_spifc_set_addr(spifc);
		ret = meson_spifc_dout(spifc, (u8 *)write_buf, done, current_len);
		if (ret)
			break;
		done += current_len;
		spifc->addr += current_len;
	}

	return ret ? 0 : len;
}

static struct spi_nor *meson_snor_init(struct meson_spifc *spifc,
				       struct device_node *np)
{
	struct device *dev = spifc->dev;
	struct spi_nor *nor;
	struct mtd_info *mtd;

	const struct spi_nor_hwcaps hwcaps = {
#ifdef CONFIG_SPIFC_HWCAPS_DUAL_QUAD
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_READ_FAST |
			SNOR_HWCAPS_READ_1_1_2 |
			SNOR_HWCAPS_READ_1_1_4 |
			SNOR_HWCAPS_PP |
			SNOR_HWCAPS_PP_1_1_4,
#else
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_PP,
#endif
	};

	nor = devm_kzalloc(dev, sizeof(*nor), GFP_KERNEL);
	if (!nor)
		return 0;

	spifc->clk_rate = 24000000;
	of_property_read_u32(np, "spi-max-frequency", &spifc->clk_rate);
	clk_set_rate(spifc->clk, spifc->clk_rate);
	dev_info(dev, "clk_rate = %d\n", spifc->clk_rate);

	nor->dev = dev;
	spi_nor_set_flash_node(nor, np);
	nor->priv = spifc;
	nor->prepare = meson_snor_prep;
	nor->unprepare = meson_snor_unprep;
	nor->read_reg = meson_snor_read_reg;
	nor->write_reg = meson_snor_write_reg;
	nor->read = meson_snor_read;
	nor->write = meson_snor_write;
	nor->erase = NULL;
	if (!spi_nor_scan(nor, NULL, &hwcaps)) {
		dev_info(dev, "read_proto = 0x%x\n", nor->read_proto);
		dev_info(dev, "write_proto = 0x%x\n", nor->write_proto);
		dev_info(dev, "addr_width = %d\n", nor->addr_width);
		dev_info(dev, "read_dummy = %d\n", nor->read_dummy);
		mtd = &nor->mtd;
		mtd->name = (np->name) ? np->name : "meson_snor";
		if (!mtd_device_register(mtd, NULL, 0))
			return nor;
	}

	devm_kfree(dev, nor);
	return 0;
}
#endif /* end CONFIG_MTD_SPI_NOR */

/**
 * meson_spifc_hw_init() - reset and initialize the SPI controller
 * @spifc:	the Meson SPI device
 */
static void meson_spifc_hw_init(struct meson_spifc *spifc)
{
	u32 regv;

	regmap_read(spifc->regmap, SPIFC_AHB_REQ_CTRL, &regv);
	regv &= ~(1 << 31);
	regmap_write(spifc->regmap, SPIFC_AHB_REQ_CTRL, regv);

	regmap_read(spifc->regmap, SPIFC_AHB_CTRL, &regv);
	regv &= ~(1 << 31);
	regmap_write(spifc->regmap, SPIFC_AHB_CTRL, regv);

	regmap_write(spifc->regmap, SPIFC_ACTIMING0,
		     (1 << 30) | (1 << 28) | (4 << 16) |
		     (4 << 12) | (4 << 8) | (1));

	regmap_write(spifc->regmap, SPIFC_USER_DBUF_ADDR, 0);
}

static int meson_spifc_probe(struct platform_device *pdev)
{
#ifdef CONFIG_MTD_SPI_NOR
	struct device_node *np;
#endif
	struct spi_master *master = 0;
	struct meson_spifc *spifc;
	struct resource *res;
	void __iomem *base;
	int ret = 0;
	u32 ahb_addr = 0;

	spifc = devm_kzalloc(&pdev->dev, sizeof(*spifc), GFP_KERNEL);
	if (!spifc)
		return -ENOMEM;

	spifc->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(spifc->dev, res);
	if (IS_ERR(base)) {
		ret = PTR_ERR(base);
		goto out_err;
	}
	spifc->base = base;

	spifc->regmap = devm_regmap_init_mmio(spifc->dev, base,
					      &spifc_regmap_config);
	if (IS_ERR(spifc->regmap)) {
		ret = PTR_ERR(spifc->regmap);
		goto out_err;
	}

#ifdef CONFIG_ENABLE_AHB_MODE
	ret = of_property_read_u32_index(spifc->dev->of_node, "ahb-addr", 0, &ahb_addr);
	if (ret)
		pr_info("spifc ahb address parse failed!\n");

	spifc_ahb_map_addr = ioremap(ahb_addr, SPIFC_AHB_BUF_CACHE_SIZE);
	if (!spifc_ahb_map_addr)
		pr_info("spifc ahb address map failed!\n");
#endif

	spifc->clk = devm_clk_get(spifc->dev, NULL);
	if (IS_ERR(spifc->clk)) {
		dev_err(spifc->dev, "missing clock\n");
		ret = PTR_ERR(spifc->clk);
		goto out_err;
	}

	ret = clk_prepare_enable(spifc->clk);
	if (ret) {
		dev_err(spifc->dev, "can't prepare clock\n");
		goto out_err;
	}

	platform_set_drvdata(pdev, spifc);
	meson_spifc_hw_init(spifc);

#ifdef CONFIG_MTD_SPI_NOR
	np = of_get_next_available_child(spifc->dev->of_node, NULL);
	if (np && of_device_is_compatible(np, "jedec,spi-nor")) {
		mutex_init(&spifc->lock);
		spifc->nor = meson_snor_init(spifc, np);
	}

	if (spifc->nor)
		return 0;

	mutex_destroy(&spifc->lock);
	dev_warn(spifc->dev, "no snor on spifc bus\n");
#endif

	pm_runtime_set_autosuspend_delay(spifc->dev, 500);
	pm_runtime_use_autosuspend(spifc->dev);
	pm_runtime_enable(spifc->dev);

	master = spi_alloc_master(&pdev->dev, 0);
	if (!master) {
		ret = -ENOMEM;
		goto out_clk;
	}

	spifc->master = master;
	spi_master_set_devdata(master, spifc);
	master->num_chipselect = 1;
	master->dev.of_node = pdev->dev.of_node;
	master->mode_bits = SPI_TX_DUAL | SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD;
	master->bits_per_word_mask = SPI_BPW_MASK(8);
	master->auto_runtime_pm = true;
	master->transfer_one = meson_spifc_transfer_one;
	master->min_speed_hz = 1000000;
	master->max_speed_hz = 200000000;
	ret = devm_spi_register_master(spifc->dev, master);
	if (ret) {
		dev_err(spifc->dev, "failed to register spi master\n");
		goto out_clk;
	}

	return 0;
out_clk:
	clk_disable_unprepare(spifc->clk);
out_err:
	if (master)
		spi_master_put(master);
	devm_kfree(&pdev->dev, spifc);
	return ret;
}

static int meson_spifc_remove(struct platform_device *pdev)
{
	struct meson_spifc *spifc = platform_get_drvdata(pdev);

	pm_runtime_get_sync(&pdev->dev);
	clk_disable_unprepare(spifc->clk);
#ifdef CONFIG_MTD_SPI_NOR
	mutex_destroy(&spifc->lock);
#endif
	pm_runtime_disable(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int meson_spifc_suspend(struct device *dev)
{
	struct meson_spifc *spifc = dev_get_drvdata(dev);
	int ret = 0;

	if (spifc->master) {
		ret = spi_master_suspend(spifc->master);
		if (ret)
			return ret;
	}

	if (!pm_runtime_suspended(dev))
		clk_disable_unprepare(spifc->clk);

	return 0;
}

static int meson_spifc_resume(struct device *dev)
{
	struct meson_spifc *spifc = dev_get_drvdata(dev);
	int ret = 0;

	if (!pm_runtime_suspended(dev)) {
		ret = clk_prepare_enable(spifc->clk);
		if (ret)
			return ret;
	}

	meson_spifc_hw_init(spifc);

	if (spifc->master) {
		ret = spi_master_resume(spifc->master);
		if (ret)
			clk_disable_unprepare(spifc->clk);
	}

	return ret;
}
#endif /* CONFIG_PM_SLEEP */

#ifdef CONFIG_PM
static int meson_spifc_runtime_suspend(struct device *dev)
{
	struct meson_spifc *spifc = dev_get_drvdata(dev);

	clk_disable_unprepare(spifc->clk);

	return 0;
}

static int meson_spifc_runtime_resume(struct device *dev)
{
	struct meson_spifc *spifc = dev_get_drvdata(dev);
	int ret;

	ret = clk_prepare_enable(spifc->clk);
	meson_spifc_hw_init(spifc);

	return ret;
}
#endif /* CONFIG_PM */

static const struct dev_pm_ops meson_spifc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(meson_spifc_suspend, meson_spifc_resume)
	SET_RUNTIME_PM_OPS(meson_spifc_runtime_suspend,
			   meson_spifc_runtime_resume,
			   NULL)
};

static const struct of_device_id meson_spifc_dt_match[] = {
	{ .compatible = "amlogic,meson-spifc-v2", },
	{ },
};
MODULE_DEVICE_TABLE(of, meson_spifc_dt_match);

struct platform_driver meson_spifc_v2_driver = {
	.probe	= meson_spifc_probe,
	.remove	= meson_spifc_remove,
	.driver	= {
		.name		= "meson-spifc-v2",
		.of_match_table	= of_match_ptr(meson_spifc_dt_match),
		.pm		= &meson_spifc_pm_ops,
	},
};

//module_platform_driver(meson_spifc_v2_driver);

MODULE_AUTHOR("Amlogic R&D");
MODULE_DESCRIPTION("Amlogic Meson SPIFC V2 driver");
MODULE_LICENSE("GPL v2");
