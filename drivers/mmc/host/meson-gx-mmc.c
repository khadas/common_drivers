// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/iopoll.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/bitfield.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/aml_sd.h>
#include <linux/delay.h>
#include <core.h>
#include <mmc_ops.h>
#include <linux/time.h>
#include <linux/random.h>
#include <linux/gpio/consumer.h>
#include <linux/sched/clock.h>
#include <linux/debugfs.h>
#include "mmc_key.h"
#include "mmc_dtb.h"
#include <trace/hooks/mmc.h>
#include <linux/moduleparam.h>
#include <linux/amlogic/gki_module.h>

#include "meson-cqhci.h"

struct mmc_gpio {
	struct gpio_desc *ro_gpio;
	struct gpio_desc *cd_gpio;
	irqreturn_t (*cd_gpio_isr)(int irq, void *dev_id);
	char *ro_label;
	char *cd_label;
	u32 cd_debounce_delay_ms;
};

struct wifi_clk_table wifi_clk[WIFI_CLOCK_TABLE_MAX] = {
	{"8822BS", 0, 0xb822, 167000000},
	{"8822CS", 0, 0xc822, 167000000},
	{"qca6174", 0, 0x50a, 167000000}
};

struct mmc_host *sdio_host;
static char *caps2_quirks = "none";

//#if CONFIG_AMLOGIC_KERNEL_VERSION == 13515
//void mmc_sd_update_cmdline_timing(void *data, struct mmc_card *card, int *err)
//{
//	/* nothing */
//	*err = 0;
//}
//
//void mmc_sd_update_dataline_timing(void *data, struct mmc_card *card, int *err)
//{
//	/* nothing */
//	*err = 0;
//}
//
//#define SD_CMD_TIMING mmc_sd_update_cmdline_timing
//#define SD_DATA_TIMING mmc_sd_update_dataline_timing
//#endif

static inline u32 aml_mv_dly1_nommc(u32 x)
{
	return (x) | ((x) << 6) | ((x) << 12) | ((x) << 18);
}

static inline u32 aml_mv_dly1(u32 x)
{
	return (x) | ((x) << 6) | ((x) << 12) | ((x) << 18) | ((x) << 24);
}

static inline u32 aml_mv_dly2(u32 x)
{
	return (x) | ((x) << 6) | ((x) << 12) | ((x) << 24);
}

static inline u32 aml_mv_dly2_nocmd(u32 x)
{
	return (x) | ((x) << 6) | ((x) << 12);
}

int amlogic_of_parse(struct mmc_host *host)
{
	struct device *dev = host->parent;
	struct meson_host *mmc = mmc_priv(host);

	if (device_property_read_u32(dev, "init_core_phase",
			&mmc->sd_mmc.init.core_phase) < 0)
		mmc->sd_mmc.init.core_phase = 2;
	if (device_property_read_u32(dev, "init_tx_phase",
			&mmc->sd_mmc.init.tx_phase) < 0)
		mmc->sd_mmc.init.tx_delay = 0;
	if (device_property_read_u32(dev, "hs2_core_phase",
			&mmc->sd_mmc.hs2.core_phase) < 0)
		mmc->sd_mmc.hs2.core_phase = 2;
	if (device_property_read_u32(dev, "hs2_tx_phase",
			&mmc->sd_mmc.hs2.tx_phase) < 0)
		mmc->sd_mmc.hs2.tx_delay = 0;
	if (device_property_read_u32(dev, "hs4_core_phase",
			&mmc->sd_mmc.hs4.core_phase) < 0)
		mmc->sd_mmc.hs4.core_phase = 0;
	if (device_property_read_u32(dev, "hs4_tx_phase",
			&mmc->sd_mmc.hs4.tx_phase) < 0)
		mmc->sd_mmc.hs4.tx_phase = 0;
	if (device_property_read_u32(dev, "src_clk_rate",
			&mmc->src_clk_rate) < 0)
		mmc->src_clk_rate = 1000000000;
	if (device_property_read_u32(dev, "sdr_tx_delay",
			&mmc->sd_mmc.sdr.tx_delay) < 0)
		mmc->sd_mmc.sdr.tx_delay = 0;
	if (device_property_read_u32(dev, "sdr_core_phase",
			&mmc->sd_mmc.sdr.core_phase) < 0)
		mmc->sd_mmc.sdr.core_phase = 2;
	if (device_property_read_u32(dev, "sdr_tx_phase",
			&mmc->sd_mmc.sdr.tx_phase) < 0)
		mmc->sd_mmc.sdr.tx_phase = 0;

	if (device_property_read_bool(dev, "ignore_desc_busy"))
		mmc->ignore_desc_busy = true;
	else
		mmc->ignore_desc_busy = false;

	if (device_property_read_bool(dev, "use_intf3_tuning"))
		mmc->use_intf3_tuning = true;
	else
		mmc->use_intf3_tuning = false;

	if (device_property_read_u32(dev, "card_type",
			&mmc->card_type) < 0) {
		dev_err(host->parent,
			"No config cart type value\n");
		return -1;
	}

	if (device_property_read_u32(dev, "nwr_cnt", &mmc->nwr_cnt) < 0)
		mmc->nwr_cnt = 0;

	if (device_property_read_u32(dev, "tx_delay",
				&mmc->sd_mmc.hs4.tx_delay) < 0)
		mmc->sd_mmc.hs4.tx_delay = 16;

	device_property_read_u32(dev, "save_para", &mmc->save_para);
	device_property_read_u32(dev, "compute_cmd_delay",
				&mmc->compute_cmd_delay);
	device_property_read_u32(dev, "compute_coef", &mmc->compute_coef);

	if (device_property_read_bool(dev, "mmc_debug_flag"))
		mmc->debug_flag = 0;
	else
		mmc->debug_flag = 1;

	if (device_property_read_bool(dev, "mmc-run-pxp"))
		mmc->run_pxp_flag = 1;
	else
		mmc->run_pxp_flag = 0;

	if (device_property_read_bool(dev, "supports-cqe"))
		mmc->enable_hwcq = true;
	else
		mmc->enable_hwcq = false;

	if (device_property_read_bool(dev, "use-64bit-dma"))
		mmc->flags |= AML_USE_64BIT_DMA;

	if (device_property_read_bool(dev, "auto-clock-sdio"))
		mmc->auto_clk = true;
	else
		mmc->auto_clk = false;

	if (device_property_read_bool(dev, "sd-clock-sample"))
		mmc->sd_clk_sample = true;
	else
		mmc->sd_clk_sample = false;

	return 0;
}

/*
 * Checks that a normal transfer didn't have any errors
 */
static int mmc_check_result(struct mmc_request *mrq)
{
	int ret;

	WARN_ON(!mrq || !mrq->cmd || !mrq->data);

	ret = 0;

	if (!ret && mrq->cmd->error)
		ret = mrq->cmd->error;
	if (!ret && mrq->data->error)
		ret = mrq->data->error;
	if (!ret && mrq->stop && mrq->stop->error)
		ret = mrq->stop->error;
	if (!ret && mrq->data->bytes_xfered !=
			mrq->data->blocks * mrq->data->blksz)
		ret = RESULT_FAIL;

	if (ret == -EINVAL)
		ret = RESULT_UNSUP_HOST;

	return ret;
}

static void mmc_prepare_mrq(struct mmc_card *card,
			    struct mmc_request *mrq, struct scatterlist *sg,
			    unsigned int sg_len, unsigned int dev_addr,
			    unsigned int blocks,
			    unsigned int blksz, int write)
{
	WARN_ON(!mrq || !mrq->cmd || !mrq->data || !mrq->stop);

	if (blocks > 1) {
		mrq->cmd->opcode = write ?
			MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK;
	} else {
		mrq->cmd->opcode = write ?
			MMC_WRITE_BLOCK : MMC_READ_SINGLE_BLOCK;
	}

	mrq->cmd->arg = dev_addr;
	if (!mmc_card_is_blockaddr(card))
		mrq->cmd->arg <<= 9;

	mrq->cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	if (blocks == 1) {
		mrq->stop = NULL;
	} else {
		mrq->stop->opcode = MMC_STOP_TRANSMISSION;
		mrq->stop->arg = 0;
		mrq->stop->flags = MMC_RSP_R1B | MMC_CMD_AC;
	}

	mrq->data->blksz = blksz;
	mrq->data->blocks = blocks;
	mrq->data->flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
	mrq->data->sg = sg;
	mrq->data->sg_len = sg_len;

	mmc_set_data_timeout(mrq->data, card);
}

unsigned int mmc_capacity(struct mmc_card *card)
{
	if (!mmc_card_sd(card) && mmc_card_is_blockaddr(card))
		return card->ext_csd.sectors;
	else
		return card->csd.capacity << (card->csd.read_blkbits - 9);
}

static int mmc_transfer(struct mmc_card *card, unsigned int dev_addr,
			unsigned int blocks, void *buf, int write)
{
	u8 original_part_config;
	u8 user_part_number = 0;
	u8 cur_part_number;
	bool switch_partition = false;
	unsigned int size;
	struct scatterlist sg;
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_command stop = {0};
	struct mmc_data data = {0};
	int ret;

	cur_part_number = card->ext_csd.part_config
		& EXT_CSD_PART_CONFIG_ACC_MASK;
	if (cur_part_number != user_part_number) {
		switch_partition = true;
		original_part_config = card->ext_csd.part_config;
		cur_part_number = original_part_config
			& (~EXT_CSD_PART_CONFIG_ACC_MASK);
		ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONFIG, cur_part_number,
				 card->ext_csd.part_time);
		if (ret)
			return ret;

		card->ext_csd.part_config = cur_part_number;
	}
	if ((dev_addr + blocks) >= mmc_capacity(card)) {
		pr_info("[%s] %s range exceeds device capacity!\n",
			__func__, write ? "write" : "read");
		ret = -1;
		return ret;
	}

	size = blocks << card->csd.read_blkbits;
	sg_init_one(&sg, buf, size);

	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = &stop;

	mmc_prepare_mrq(card, &mrq, &sg, 1, dev_addr,
			blocks, 1 << card->csd.read_blkbits, write);

	mmc_wait_for_req(card->host, &mrq);

	ret = mmc_check_result(&mrq);
	if (switch_partition) {
		ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONFIG, original_part_config,
				 card->ext_csd.part_time);
		if (ret)
			return ret;
		card->ext_csd.part_config = original_part_config;
	}

	return ret;
}

int aml_disable_mmc_cqe(struct mmc_card *card)
{
	int ret = 0;

	if (card->reenable_cmdq && card->ext_csd.cmdq_en) {
		pr_debug("[%s] [%d]\n", __func__, __LINE__);
		ret = mmc_cmdq_disable(card);
		if (ret)
			pr_err("[%s] disable cqe mode failed\n", __func__);
	}
	return ret;
}

int aml_enable_mmc_cqe(struct mmc_card *card)
{
	int ret = 0;

	if (card->reenable_cmdq && !card->ext_csd.cmdq_en) {
		pr_debug("[%s] [%d]\n", __func__, __LINE__);
		ret = mmc_cmdq_enable(card);
		if (ret)
			pr_err("[%s] reenable cqe mode failed\n", __func__);
	}
	return ret;
}

int mmc_read_internal(struct mmc_card *card, unsigned int dev_addr,
			unsigned int blocks, void *buf)
{
	return mmc_transfer(card, dev_addr, blocks, buf, 0);
}

int mmc_write_internal(struct mmc_card *card, unsigned int dev_addr,
			unsigned int blocks, void *buf)
{
	return mmc_transfer(card, dev_addr, blocks, buf, 1);
}

static unsigned int meson_mmc_get_timeout_msecs(struct mmc_data *data)
{
	unsigned int timeout = data->timeout_ns / NSEC_PER_MSEC;

	if (!timeout)
		return SD_EMMC_CMD_TIMEOUT_DATA;

	timeout = roundup_pow_of_two(timeout);

	return min(timeout, 32768U); /* max. 2^15 ms */
}

static unsigned int meson_mmc_get_cmd_timeout_msecs(struct mmc_command *cmd)
{
	unsigned int timeout = cmd->busy_timeout;

	if (!timeout)
		return SD_EMMC_CMD_TIMEOUT;

	timeout = roundup_pow_of_two(timeout);

	return min(timeout, 32768U); /* max. 2^15 ms */
}

static void meson_mmc_get_transfer_mode(struct mmc_host *mmc,
					struct mmc_request *mrq)
{
	struct meson_host *host = mmc_priv(mmc);
	struct mmc_data *data = mrq->data;
	struct scatterlist *sg;
	int i;
	bool use_desc_chain_mode = true;

	/*
	 * When Controller DMA cannot directly access DDR memory, disable
	 * support for Chain Mode to directly use the internal SRAM using
	 * the bounce buffer mode.
	 */
	if (host->dram_access_quirk)
		return;

	/*
	 * Broken SDIO with AP6255-based WiFi on Khadas VIM Pro has been
	 * reported. For some strange reason this occurs in descriptor
	 * chain mode only. So let's fall back to bounce buffer mode
	 * for command SD_IO_RW_EXTENDED.
	 */
	/*if (mrq->cmd->opcode == SD_IO_RW_EXTENDED)
	 *	return;
	 */

	for_each_sg(data->sg, sg, data->sg_len, i)
		/* check for 8 byte alignment */
		if (sg->offset & 7) {
			use_desc_chain_mode = false;
			break;
		}

	if (use_desc_chain_mode)
		data->host_cookie |= SD_EMMC_DESC_CHAIN_MODE;
}

static inline bool meson_mmc_desc_chain_mode(const struct mmc_data *data)
{
	return data->host_cookie & SD_EMMC_DESC_CHAIN_MODE;
}

static inline bool meson_mmc_bounce_buf_read(const struct mmc_data *data)
{
	return data && data->flags & MMC_DATA_READ &&
	       !meson_mmc_desc_chain_mode(data);
}

static void meson_mmc_pre_req(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;

	if (!data)
		return;

	meson_mmc_get_transfer_mode(mmc, mrq);
	data->host_cookie |= SD_EMMC_PRE_REQ_DONE;

	if (!meson_mmc_desc_chain_mode(data))
		return;

	data->sg_count = dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len,
				    mmc_get_dma_dir(data));
	if (!data->sg_count)
		dev_err(mmc_dev(mmc), "dma_map_sg failed");
}

static void meson_mmc_post_req(struct mmc_host *mmc, struct mmc_request *mrq,
			       int err)
{
	struct mmc_data *data = mrq->data;

	if (data && meson_mmc_desc_chain_mode(data) && data->sg_count)
		dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len,
			     mmc_get_dma_dir(data));
}

/*
 * Gating the clock on this controller is tricky.  It seems the mmc clock
 * is also used by the controller.  It may crash during some operation if the
 * clock is stopped.  The safest thing to do, whenever possible, is to keep
 * clock running at stop it at the pad using the pinmux.
 */
static void meson_mmc_clk_gate(struct meson_host *host)
{
	u32 cfg;

	if (host->pins_clk_gate) {
		pinctrl_select_state(host->pinctrl, host->pins_clk_gate);
	} else {
		/*
		 * If the pinmux is not provided - default to the classic and
		 * unsafe method
		 */
		cfg = readl(host->regs + SD_EMMC_CFG);
		cfg |= CFG_STOP_CLOCK;
		writel(cfg, host->regs + SD_EMMC_CFG);
	}
}

static void meson_mmc_clk_ungate(struct meson_host *host)
{
	u32 cfg;

	if (host->pins_clk_gate)
		pinctrl_select_state(host->pinctrl, host->pins_default);

	/* Make sure the clock is not stopped in the controller */
	cfg = readl(host->regs + SD_EMMC_CFG);
	cfg &= ~CFG_STOP_CLOCK;
	writel(cfg, host->regs + SD_EMMC_CFG);
}

static void meson_mmc_set_phase_delay(struct meson_host *host, u32 mask,
				      unsigned int phase)
{
	u32 val;

	val = readl(host->regs);
	val &= ~mask;
	val |= phase << __ffs(mask);
	writel(val, host->regs);
}

static void pxp_clk_set(struct meson_host *host, unsigned long rate)
{
	u32 clk_src, val;
	unsigned long src_rate;
	u32 clk_div;

	if (rate <= 400000) {
		src_rate = 24000000;
		clk_src = 0;
	} else {
		src_rate = 1000000000;
		clk_src = (1 << 6);
	}
	dev_err(host->dev, "clock reg:0x%x\n", readl(host->regs + SD_EMMC_CLOCK));
	clk_div = DIV_ROUND_UP(src_rate, rate);
	pr_info("clk_div:0x%x\n", clk_div);
	val = readl(host->regs + SD_EMMC_CLOCK);
	pr_info("val:0x%x\n", val);
	val &= ~CLK_DIV_MASK;
	val |= clk_div;
	val &= ~CLK_SRC_MASK;
	val |= clk_src;
	writel(val, host->regs + SD_EMMC_CLOCK);
	pr_info("clock reg:0x%x, rate:%lu\n", readl(host->regs + SD_EMMC_CLOCK), rate);
}

static int no_pxp_clk_set(struct meson_host *host, struct mmc_ios *ios,
						unsigned long rate)
{
	int ret = 0;
	struct clk *src_clk = NULL;
	struct mmc_host *mmc = host->mmc;
	u32 cfg = readl(host->regs + SD_EMMC_CFG);

	dev_dbg(host->dev, "[%s]set rate:%lu\n", __func__, rate);
	if (host->src_clk)
		clk_disable_unprepare(host->src_clk);

	switch (ios->timing) {
	case MMC_TIMING_MMC_HS400:
		if (host->clk[2])
			src_clk = host->clk[2];
		else
			src_clk = host->clk[1];
		dev_dbg(host->dev, "HS400 set src rate to:%u\n",
			host->src_clk_rate);
		ret = clk_set_rate(src_clk, host->src_clk_rate);
		if (ret) {
			dev_err(host->dev, "set src err\n");
				return ret;
		}
		cfg |= CFG_AUTO_CLK;
		break;
	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_MMC_DDR52:
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_HS200:
	case MMC_TIMING_UHS_SDR12:
	case MMC_TIMING_UHS_SDR25:
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_UHS_SDR104:
		dev_dbg(host->dev, "[%s]Other mode set src rate to:%u\n",
				__func__, host->src_clk_rate);
		ret = clk_set_rate(host->clk[1], host->src_clk_rate);
		if (ret) {
			dev_err(host->dev, "set src err\n");
				return ret;
		}
		src_clk = host->clk[1];
		cfg |= CFG_AUTO_CLK;
	/* sdio set clk always on default */
		if (aml_card_type_sdio(host) && !host->auto_clk)
			cfg &= ~CFG_AUTO_CLK;
		break;
	case MMC_TIMING_LEGACY:
		dev_dbg(host->dev, "[%s]Legacy set rate to:%lu\n",
				__func__, rate);
		src_clk = host->clk[0];
	/* enable always on clock for 400KHZ */
		cfg &= ~CFG_AUTO_CLK;

	/* switch source clock as Total before clk =0, then disable source clk */
		if (rate == 0) {
			if (host->mux_div && (!strcmp(__clk_get_name(src_clk), "xtal")))
				ret = clk_set_parent(host->mux[2], src_clk);
			else
				ret = clk_set_parent(host->mux[0], src_clk);
			host->src_clk = NULL;
			cfg |= CFG_AUTO_CLK;
			writel(cfg, host->regs + SD_EMMC_CFG);
			return ret;
		}
		break;
	default:
		dev_dbg(host->dev, "Check mmc/sd/sdio timing mode\n");
		WARN_ON(1);
		break;
	}

	clk_prepare_enable(src_clk);
	writel(cfg, host->regs + SD_EMMC_CFG);
	host->src_clk = src_clk;
	if (host->mux_div) { // C1/C2
		if (!strcmp(__clk_get_name(src_clk), "xtal")) {
			ret = clk_set_parent(host->mux[2], src_clk);
		} else {
			ret = clk_set_parent(host->mux[0], src_clk);
			if (!ret) {
				clk_set_rate(host->mux_div, clk_get_rate((src_clk)));
				ret = clk_set_parent(host->mux[2], host->mux_div);
			}
		}
	} else { // other soc
		ret = clk_set_parent(host->mux[0], src_clk);
	}

	if (ret) {
		dev_err(host->dev, "set parent error\n");
		return ret;
	}

	ret = clk_set_rate(host->mmc_clk, rate);
	if (ret) {
		dev_err(host->dev, "Unable to set cfg_div_clk to %lu. ret=%d\n",
			rate, ret);
		return ret;
	}
	host->req_rate = rate;
	mmc->actual_clock = clk_get_rate(host->mmc_clk);

	dev_dbg(host->dev, "clk rate: %u Hz\n", mmc->actual_clock);

	return ret;
}

/*
 * The SD/eMMC IP block has an internal mux and divider used for
 * generating the MMC clock.  Use the clock framework to create and
 * manage these clocks.
 */
static int meson_mmc_clk_init(struct meson_host *host)
{
	struct clk_init_data init;
	struct clk_divider *div;
	char clk_name[32], name[16];
	int i, ret = 0;
	const char *clk_parent[1];
	u32 clk_reg;

	/* init SD_EMMC_CLOCK to sane defaults w/min clock rate */
	clk_reg = CLK_ALWAYS_ON(host);
	clk_reg |= CLK_DIV_MASK;
	clk_reg |= FIELD_PREP(CLK_CORE_PHASE_MASK, CLK_PHASE_180);
	clk_reg |= FIELD_PREP(CLK_TX_PHASE_MASK, CLK_PHASE_0);
	clk_reg |= FIELD_PREP(CLK_RX_PHASE_MASK, CLK_PHASE_0);
	writel(clk_reg, host->regs + SD_EMMC_CLOCK);

	for (i = 0; i < 2; i++) {
		snprintf(name, sizeof(name), "mux%d", i);
		host->mux[i] = devm_clk_get(host->dev, name);
		if (IS_ERR(host->mux[i])) {
			if (host->mux[i] != ERR_PTR(-EPROBE_DEFER))
				dev_err(host->dev, "Missing clock %s\n", name);
			return PTR_ERR(host->mux[i]);
		}
	}
	host->mux_div = devm_clk_get(host->dev, "mux_div");
	if (IS_ERR(host->mux_div)) {
		host->mux_div = NULL;
		dev_dbg(host->dev,
			"Missing clock %s(only c1/c2 have mux_div)\n",
			"mux_div");
	} else {
		snprintf(name, sizeof(name), "mux%d", 2);
		host->mux[2] = devm_clk_get(host->dev, name);
		if (IS_ERR(host->mux[2]))
			dev_err(host->dev, "Missing clock %s\n", "mux2");
	}

	for (i = 0; i < 3; i++) {
		snprintf(name, sizeof(name), "clkin%d", i);
		host->clk[i] = devm_clk_get(host->dev, name);
		if (IS_ERR(host->clk[i])) {
			dev_dbg(host->dev,
				"Missing clock%s, i = %d\n", name, i);
			host->clk[i] = NULL;
		}
	}

	if (host->mux_div) {
		ret = clk_set_parent(host->mux[2], host->mux_div);
		if (ret) {
			dev_err(host->dev, "Set div parent error\n");
			return ret;
		}
	}

	/* create the divider */
	div = devm_kzalloc(host->dev, sizeof(*div), GFP_KERNEL);
	if (!div)
		return -ENOMEM;

	snprintf(clk_name, sizeof(clk_name), "%s#div", dev_name(host->dev));
	init.name = clk_name;
	init.ops = &clk_divider_ops;
	clk_parent[0] = __clk_get_name(host->mux[1]);
	init.parent_names = clk_parent;
	init.num_parents = 1;
	init.flags = CLK_SET_RATE_PARENT;

	div->reg = host->regs + SD_EMMC_CLOCK;
	div->shift = __ffs(CLK_DIV_MASK);
	div->width = __builtin_popcountl(CLK_DIV_MASK);
	div->hw.init = &init;
	div->flags = CLK_DIVIDER_ONE_BASED;

	host->mmc_clk = devm_clk_register(host->dev, &div->hw);
	if (WARN_ON(IS_ERR(host->mmc_clk)))
		return PTR_ERR(host->mmc_clk);
	/* create the mmc core clock */
	if (host->mux_div) {
		ret = clk_set_parent(host->mux[2], host->clk[0]);
	} else {
		ret = clk_set_parent(host->mux[0], host->clk[0]);
	}
	if (ret) {
		dev_err(host->dev, "Set 24m parent error\n");
		return ret;
	}
	/* init SD_EMMC_CLOCK to sane defaults w/min clock rate */
	host->mmc->f_min = clk_round_rate(host->mmc_clk, 400000);
	ret = clk_set_rate(host->mmc_clk, host->mmc->f_min);
	if (ret)
		return ret;

	return clk_prepare_enable(host->mmc_clk);
}

static int meson_mmc_pxp_clk_init(struct meson_host *host)
{
	u32 reg_val, val;
	struct mmc_phase *mmc_phase_init = &host->sd_mmc.init;

	writel(0, host->regs + SD_EMMC_V3_ADJUST);
	writel(0, host->regs + SD_EMMC_DELAY1);
	writel(0, host->regs + SD_EMMC_DELAY2);
	writel(0, host->regs + SD_EMMC_CLOCK);

	reg_val = 0;
	reg_val |= CLK_ALWAYS_ON(host);
	reg_val |= CLK_DIV_MASK;
	writel(reg_val, host->regs + SD_EMMC_CLOCK);

	val = readl(host->clk_tree_base);
	pr_debug("clk tree base:0x%x\n", val);
	if (aml_card_type_non_sdio(host))
		writel(0x800000, host->clk_tree_base);

	else
		writel(0x80, host->clk_tree_base);

	reg_val = readl(host->regs);
	reg_val &= ~CLK_DIV_MASK;
	reg_val |= 60;
	writel(reg_val, host->regs);

	meson_mmc_set_phase_delay(host, CLK_CORE_PHASE_MASK,
									mmc_phase_init->core_phase);
	meson_mmc_set_phase_delay(host, CLK_TX_PHASE_MASK,
									mmc_phase_init->tx_phase);
	reg_val = readl(host->regs + SD_EMMC_CFG);
	reg_val |= CFG_AUTO_CLK;
	reg_val = readl(host->regs + SD_EMMC_CFG);
	return 0;
}

static unsigned int aml_sd_emmc_clktest(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 intf3 = readl(host->regs + SD_EMMC_INTF3);
	u32 clktest = 0, delay_cell = 0, clktest_log = 0, count = 0;
	u32 vcfg = readl(host->regs + SD_EMMC_CFG);
	int i = 0;
	unsigned int cycle = 0;

	writel(0, (host->regs + SD_EMMC_V3_ADJUST));
	cycle = (1000000000 / mmc->actual_clock) * 1000;
	vcfg &= ~(1 << 23);
	writel(vcfg, host->regs + SD_EMMC_CFG);
	writel(0, host->regs + SD_EMMC_DELAY1);
	writel(0, host->regs + SD_EMMC_DELAY2);
	intf3 &= ~CLKTEST_EXP_MASK;
	intf3 |= 8 << __ffs(CLKTEST_EXP_MASK);
	intf3 |= CLKTEST_ON_M;
	writel(intf3, host->regs + SD_EMMC_INTF3);
	clktest_log = readl(host->regs + SD_EMMC_CLKTEST_LOG);
	clktest = readl(host->regs + SD_EMMC_CLKTEST_OUT);
	while (!(clktest_log & 0x80000000)) {
		mdelay(1);
		i++;
		clktest_log = readl(host->regs + SD_EMMC_CLKTEST_LOG);
		clktest = readl(host->regs + SD_EMMC_CLKTEST_OUT);
		if (i > 4) {
			pr_warn("[%s] [%d] emmc clktest error\n",
				__func__, __LINE__);
			break;
		}
	}
	if (clktest_log & 0x80000000) {
		clktest = readl(host->regs + SD_EMMC_CLKTEST_OUT);
		count = clktest / (1 << 8);
		if (vcfg & 0x4)
			delay_cell = ((cycle / 2) / count);
		else
			delay_cell = (cycle / count);
	}
	pr_info("%s [%d] clktest : %u, delay_cell: %d, count: %u\n",
		__func__, __LINE__, clktest, delay_cell, count);
	intf3 = readl(host->regs + SD_EMMC_INTF3);
	intf3 &= ~CLKTEST_ON_M;
	writel(intf3, host->regs + SD_EMMC_INTF3);
	vcfg = readl(host->regs + SD_EMMC_CFG);
	vcfg |= (1 << 23);
	writel(vcfg, host->regs + SD_EMMC_CFG);
	host->delay_cell = delay_cell;
	return count;
}

static int meson_mmc_set_adjust(struct mmc_host *mmc, u32 value)
{
	u32 val;
	struct meson_host *host = mmc_priv(mmc);

	val = readl(host->regs + SD_EMMC_V3_ADJUST);
	val &= ~CLK_ADJUST_DELAY;
	val &= ~CFG_ADJUST_ENABLE;
	val |= CFG_ADJUST_ENABLE;
	val |= value << __ffs(CLK_ADJUST_DELAY);

	writel(val, host->regs + SD_EMMC_V3_ADJUST);
	return 0;
}

int meson_mmc_tuning_transfer(struct mmc_host *mmc, u32 opcode)
{
	int tuning_err = 0;
	int n, nmatch;
	/* try ntries */
	for (n = 0, nmatch = 0; n < TUNING_NUM_PER_POINT; n++) {
		tuning_err = mmc_send_tuning(mmc, opcode, NULL);
		if (!tuning_err) {
			nmatch++;
		} else {
		/* After the cmd21 command fails,
		 * it takes a certain time for the emmc status to
		 * switch from data back to transfer. Currently,
		 * only this model has this problem.
		 * by add usleep_range(20, 30);
		 */
			usleep_range(20, 30);
			break;
		}
	}
	return nmatch;
}

static int find_best_win(struct mmc_host *mmc,
		char *buf, int num, int *b_s, int *b_sz, bool wrap_f)
{
	struct meson_host *host = mmc_priv(mmc);
	int wrap_win_start = -1, wrap_win_size = 0;
	int curr_win_start = -1, curr_win_size = 0;
	int best_win_start = -1, best_win_size = 0;
	int i = 0, len = 0;
	u8 *adj_print = NULL;

	len = 0;
	adj_print = host->adj_win;
	memset(adj_print, 0, sizeof(u8) * ADJ_WIN_PRINT_MAXLEN);
	len += sprintf(adj_print, "%s: adj_win: < ", mmc_hostname(mmc));

	for (i = 0; i < num; i++) {
		/*get a ok adjust point!*/
		if (buf[i]) {
			if (i == 0)
				wrap_win_start = i;

			if (wrap_win_start >= 0)
				wrap_win_size++;

			if (curr_win_start < 0)
				curr_win_start = i;

			curr_win_size++;
			len += sprintf(adj_print + len,
					"%d ", i);
		} else {
			if (curr_win_start >= 0) {
				if (best_win_start < 0) {
					best_win_start = curr_win_start;
					best_win_size = curr_win_size;
				} else {
					if (best_win_size < curr_win_size) {
						best_win_start = curr_win_start;
						best_win_size = curr_win_size;
					}
				}
				wrap_win_start = -1;
				curr_win_start = -1;
				curr_win_size = 0;
			}
		}
	}

	sprintf(adj_print + len, ">\n");
	if (num <= AML_FIXED_ADJ_MAX)
		pr_info("%s", host->adj_win);

	/* last point is ok! */
	if (curr_win_start >= 0) {
		if (best_win_start < 0) {
			best_win_start = curr_win_start;
			best_win_size = curr_win_size;
		} else if ((wrap_win_size > 0) && wrap_f) {
			/* Wrap around case */
			if (curr_win_size + wrap_win_size > best_win_size) {
				best_win_start = curr_win_start;
				best_win_size = curr_win_size + wrap_win_size;
			}
		} else if (best_win_size < curr_win_size) {
			best_win_start = curr_win_start;
			best_win_size = curr_win_size;
		}

		curr_win_start = -1;
		curr_win_size = 0;
	}
	*b_s = best_win_start;
	*b_sz = best_win_size;

	return 0;
}

static void pr_adj_info(char *name,
		unsigned long x, u32 fir_adj, u32 div)
{
	int i;

	pr_debug("[%s] fixed_adj_win_map:%lu\n", name, x);
	for (i = 0; i < div; i++)
		pr_debug("[%d]=%d\n", (fir_adj + i) % div,
				((x >> i) & 0x1) ? 1 : 0);
}

static unsigned long _test_fixed_adj(struct mmc_host *mmc,
		u32 opcode, u32 adj, u32 div)
{
	int i = 0;
	struct meson_host *host = mmc_priv(mmc);
	u8 *adj_print = host->adj_win;
	u32 len = 0;
	u32 nmatch = 0;
	unsigned long fixed_adj_map = 0;

	memset(adj_print, 0, sizeof(u8) * ADJ_WIN_PRINT_MAXLEN);
	len += sprintf(adj_print + len, "%s: adj_win: < ", mmc_hostname(mmc));
	bitmap_zero(&fixed_adj_map, div);
	for (i = 0; i < div; i++) {
		meson_mmc_set_adjust(mmc, adj + i);
		nmatch = meson_mmc_tuning_transfer(mmc, opcode);
		/*get a ok adjust point!*/
		if (nmatch == TUNING_NUM_PER_POINT) {
			set_bit(adj + i, &fixed_adj_map);
			len += sprintf(adj_print + len,
				"%d ", adj + i);
		}
		pr_debug("%s: rx_tuning_result[%d] = %d\n",
				mmc_hostname(mmc), adj + i, nmatch);
	}
	len += sprintf(adj_print + len, ">\n");
	pr_debug("%s", host->adj_win);

	return fixed_adj_map;
}

static u32 _find_fixed_adj_mid(unsigned long map,
		u32 adj, u32 div, u32 co)
{
	u32 left, right, mid, size = 0;

	left = find_last_bit(&map, div);
	right = find_first_bit(&map, div);
	/*
	 * The lib functions don't need to be modified.
	 */
	/* coverity[callee_ptr_arith:SUPPRESS] */
	mid = find_first_zero_bit(&map, div);
	size = left - right + 1;
	pr_debug("left:%u, right:%u, mid:%u, size:%u\n",
			left, right, mid, size);
	if (size >= 3 && (mid < right || mid > left)) {
		mid = (adj + (size - 1) / 2 + (size - 1) % 2) % div;
		if ((mid == (co - 1)) && div == 5)
			return NO_FIXED_ADJ_MID;
		pr_info("tuning-c:%u, tuning-s:%u\n",
			mid, size);
		return mid;
	}
	return NO_FIXED_ADJ_MID;
}

static unsigned long _swap_fixed_adj_win(unsigned long map,
		u32 shift, u32 div)
{
	unsigned long left, right;
	/*
	 * The lib functions don't need to be modified.
	 */
	/* coverity[callee_ptr_arith:SUPPRESS] */
	bitmap_shift_right(&right, &map,
			shift, div);
	bitmap_shift_left(&left, &map,
			div - shift, div);
	bitmap_or(&map, &right, &left, div);
	return map;
}

static void set_fixed_adj_line_delay(u32 step,
		struct meson_host *host, bool no_cmd)
{
	if (aml_card_type_mmc(host)) {
		writel(aml_mv_dly1(step), host->regs + SD_EMMC_DELAY1);
		if (no_cmd)
			writel(aml_mv_dly2_nocmd(step),
					host->regs + SD_EMMC_DELAY2);
		else
			writel(aml_mv_dly2(step),
					host->regs + SD_EMMC_DELAY2);
	} else {
		writel(aml_mv_dly1_nommc(step), host->regs + SD_EMMC_DELAY1);
		if (!no_cmd)
			writel(AML_MV_DLY2_NOMMC_CMD(step),
					host->regs + SD_EMMC_DELAY2);
	}
	pr_debug("step:%u, delay1:0x%x, delay2:0x%x\n",
			step,
			readl(host->regs + SD_EMMC_DELAY1),
			readl(host->regs + SD_EMMC_DELAY2));
}

/*	1. find first removed a fixed_adj_point
 *	2. re-range fixed adj point
 *	3. retry
 */
static u32 _find_fixed_adj_valid_win(struct mmc_host *mmc,
		u32 opcode,	unsigned long *fixed_adj_map, u32 div)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 step = 0, ret = NO_FIXED_ADJ_MID, fir_adj = 0xff;
	unsigned long cur_map[1] = {0};
	unsigned long prev_map[1] = {0};
	unsigned long tmp[1] = {0};
	unsigned long dst[1] = {0};
//	struct mmc_phase *mmc_phase_init = &host->sd_mmc.init;
	u32 cop, vclk;

	vclk = readl(host->regs + SD_EMMC_CLOCK);
	cop = (vclk & CLK_CORE_PHASE_MASK) >> __ffs(CLK_CORE_PHASE_MASK);
//	cop = para->hs2.core_phase;

	div = (div == AML_FIXED_ADJ_MIN) ?
			AML_FIXED_ADJ_MIN : AML_FIXED_ADJ_MAX;
	*prev_map = *fixed_adj_map;
	pr_adj_info("prev_map", *prev_map, 0, div);
	for (; step <= 63;) {
		pr_debug("[%s]retry test fixed adj...\n", __func__);
		step += AML_FIXADJ_STEP;
		set_fixed_adj_line_delay(step, host, false);
		*cur_map = _test_fixed_adj(mmc, opcode, 0, div);
		/*pr_adj_info("cur_map", *cur_map, 0, div);*/
		bitmap_and(tmp, prev_map, cur_map, div);
		bitmap_xor(dst, prev_map, tmp, div);
		if (*dst != 0) {
			fir_adj = find_first_bit(dst, div);
			pr_adj_info(">>>>>>>>bitmap_xor_dst", *dst, 0, div);
			pr_debug("[%s] fir_adj:%u\n", __func__, fir_adj);

			*prev_map = _swap_fixed_adj_win(*prev_map,
					fir_adj, div);
			pr_adj_info(">>>>>>>>prev_map_range",
					*prev_map, fir_adj, div);
			ret = _find_fixed_adj_mid(*prev_map, fir_adj, div, cop);
			if (ret != NO_FIXED_ADJ_MID) {
				/* pre adj=core phase-1="hole"&&200MHZ,all line delay+step */
				if (((ret - 1) == (cop - 1)) && div == 5)
					set_fixed_adj_line_delay(AML_FIXADJ_STEP, host, false);
				else
					set_fixed_adj_line_delay(0,	host, false);
				return ret;
			}

			fir_adj = (fir_adj + find_next_bit(prev_map,
				div, 1)) % div;
		}
		if (fir_adj == 0xff)
			continue;

		*prev_map = *cur_map;
		*cur_map = _swap_fixed_adj_win(*cur_map, fir_adj, div);
		pr_adj_info(">>>>>>>>cur_map_range", *cur_map, fir_adj, div);
		ret = _find_fixed_adj_mid(*cur_map, fir_adj, div, cop);
		if (ret != NO_FIXED_ADJ_MID) {
			/* pre adj=core phase-1="hole"&&200MHZ,all line delay+step */
			if (((ret - 1) == (cop - 1)) && div == 5) {
				step += AML_FIXADJ_STEP;
				set_fixed_adj_line_delay(step, host, false);
			}
			return ret;
		}
	}

	pr_debug("[%s][%d] no fixed adj\n", __func__, __LINE__);
	return ret;
}

static int meson_mmc_fixadj_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 nmatch = 0;
	int adj_delay = 0;
	u8 tuning_num = 0;
	u32 clk_div, vclk;
	u32 old_dly, d1_dly, dly;
	u32 adj_delay_find =  0xff;
	unsigned long fixed_adj_map[1];
	bool all_flag = false;
	int best_s = -1, best_sz = 0;
	char rx_adj[64] = {0};
	u8 *adj_print = NULL;
	u32 len = 0;

	old_dly = readl(host->regs + SD_EMMC_V3_ADJUST);
	d1_dly = (old_dly >> 0x6) & 0x3F;
	pr_debug("Data 1 aligned delay is %d\n", d1_dly);
	writel(0, host->regs + SD_EMMC_V3_ADJUST);

tuning:
	/* renew */
	best_s = -1;
	best_sz = 0;
	memset(rx_adj, 0, 64);

	len = 0;
	adj_print = host->adj_win;
	memset(adj_print, 0, sizeof(u8) * ADJ_WIN_PRINT_MAXLEN);
	len += sprintf(adj_print + len, "%s: adj_win: < ", mmc_hostname(mmc));
	vclk = readl(host->regs + SD_EMMC_CLOCK);
	clk_div = vclk & CLK_DIV_MASK;
	pr_debug("%s: clk %d div %d tuning start\n",
			mmc_hostname(mmc), mmc->actual_clock, clk_div);

	if (clk_div <= AML_FIXED_ADJ_MAX)
		bitmap_zero(fixed_adj_map, clk_div);
	for (adj_delay = 0; adj_delay < clk_div; adj_delay++) {
		meson_mmc_set_adjust(mmc, adj_delay);
		nmatch = meson_mmc_tuning_transfer(host->mmc, opcode);
		if (nmatch == TUNING_NUM_PER_POINT) {
			rx_adj[adj_delay]++;
			if (clk_div <= AML_FIXED_ADJ_MAX)
				set_bit(adj_delay, fixed_adj_map);
			len += sprintf(adj_print + len,
				"%d ", adj_delay);
		}
	}

	sprintf(adj_print + len, ">\n");
	pr_debug("%s", host->adj_win);

	find_best_win(mmc, rx_adj, clk_div, &best_s, &best_sz, true);

	if (best_sz <= 0) {
		if ((tuning_num++ > MAX_TUNING_RETRY) || clk_div >= 10) {
			pr_info("%s: final result of tuning failed\n",
				 mmc_hostname(mmc));
			return -1;
		}
		clk_div++;
		vclk &= ~CLK_DIV_MASK;
		vclk |= clk_div & CLK_DIV_MASK;
		writel(vclk, host->regs + SD_EMMC_CLOCK);
		pr_info("%s: tuning failed, reduce freq and retuning\n",
			mmc_hostname(mmc));
		goto tuning;
	} else if ((best_sz < clk_div) &&
			(clk_div <= AML_FIXED_ADJ_MAX) &&
			(clk_div >= AML_FIXED_ADJ_MIN) &&
			!all_flag) {
		adj_delay_find = _find_fixed_adj_valid_win(mmc,
				opcode, fixed_adj_map, clk_div);
	} else if (best_sz == clk_div) {
		all_flag = true;
		dly = readl(host->regs + SD_EMMC_DELAY1);
		d1_dly = (dly >> 0x6) & 0x3F;
		pr_debug("%s() d1_dly %d, window start %d, size %d\n",
			__func__, d1_dly, best_s, best_sz);
		if (++d1_dly > 0x3F) {
			pr_err("%s: tuning failed\n",
				mmc_hostname(mmc));
			return -1;
		}
		dly &= ~(0x3F << 6);
		dly |= d1_dly << 6;
		writel(dly, host->regs + SD_EMMC_DELAY1);
		goto tuning;
	} else {
		pr_debug("%s: best_s = %d, best_sz = %d\n",
				mmc_hostname(mmc),
				best_s, best_sz);
	}

	if (adj_delay_find == 0xff) {
		adj_delay_find = best_s + (best_sz - 1) / 2
		+ (best_sz - 1) % 2;
		writel(old_dly, host->regs + SD_EMMC_DELAY1);
		pr_info("tuning-c:%u, tuning-s:%u\n",
			adj_delay_find % clk_div, best_sz);
	}
	adj_delay_find = adj_delay_find % clk_div;

	meson_mmc_set_adjust(mmc, adj_delay_find);

	pr_info("%s: clk= 0x%x, adj = 0x%x, dly1 = %x, dly2 = %x\n",
			mmc_hostname(mmc),
			readl(host->regs + SD_EMMC_CLOCK),
			readl(host->regs + SD_EMMC_V3_ADJUST),
			readl(host->regs + SD_EMMC_DELAY1),
			readl(host->regs + SD_EMMC_DELAY2));

	return 0;
}

//void sdio_get_card(struct mmc_host *host, struct mmc_card *card)
//{
//	host->card = card;
//}

int sdio_get_device(void)
{
	unsigned int i, device = 0;

	if (sdio_host && sdio_host->card)
		device = sdio_host->card->cis.device;

	for (i = 0; i < ARRAY_SIZE(wifi_clk); i++) {
		if (wifi_clk[i].m_device_id == device) {
			wifi_clk[i].m_use_flag = 1;
			break;
		}
	}
	pr_debug("sdio device is 0x%x\n", device);
	return device;
}

static int meson_mmc_clk_set(struct meson_host *host,
			struct mmc_ios *ios, bool ddr)
{
	struct mmc_host *mmc = host->mmc;
	int ret = 0;
	u32 cfg;
	unsigned long rate = ios->clock;

	/* Same request - bail-out */
	if (host->ddr == ddr && host->req_rate == rate) {
		dev_dbg(host->dev, "[%s]bail-out,clk rate: %lu Hz\n",
				__func__, rate);
		return 0;
	}

	/* stop clock */
	meson_mmc_clk_gate(host);
	host->req_rate = 0;
	mmc->actual_clock = 0;

	/* Stop the clock during rate change to avoid glitches */
	cfg = readl(host->regs + SD_EMMC_CFG);
	cfg |= CFG_STOP_CLOCK;
	cfg &= ~CFG_AUTO_CLK;
	writel(cfg, host->regs + SD_EMMC_CFG);

	if (ddr) {
		/* DDR modes require higher module clock */
		rate <<= 1;
		cfg |= CFG_DDR;
	} else {
		cfg &= ~CFG_DDR;
	}
	writel(cfg, host->regs + SD_EMMC_CFG);
	host->ddr = ddr;

	if (host->run_pxp_flag == 0)
		ret = no_pxp_clk_set(host, ios, rate);
	else
		pxp_clk_set(host, rate);

	/* We should report the real output frequency of the controller */
	if (ddr) {
		host->req_rate >>= 1;
		mmc->actual_clock >>= 1;
	}

	dev_dbg(host->dev, "clk rate: %u Hz\n", mmc->actual_clock);
	if (rate != mmc->actual_clock)
		dev_dbg(host->dev, "requested rate was %lu\n", rate);

	/* (re)start clock */
	meson_mmc_clk_ungate(host);

	return ret;
}

static int meson_mmc_prepare_ios_clock(struct meson_host *host,
				       struct mmc_ios *ios)
{
	bool ddr = false;
	int i;

	switch (ios->timing) {
	case MMC_TIMING_MMC_DDR52:
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_HS400:
		ddr = true;
		break;
	case MMC_TIMING_UHS_SDR104:
		for (i = 0; i < ARRAY_SIZE(wifi_clk); i++) {
			if (wifi_clk[i].m_use_flag) {
				ios->clock = wifi_clk[i].m_uhs_max_dtr;
				break;
			}
		}
		break;
	default:
		ddr = false;
		break;
	}

	return meson_mmc_clk_set(host, ios, ddr);
}

static void meson_mmc_check_resampling(struct meson_host *host,
				       struct mmc_ios *ios)
{
	struct mmc_phase *mmc_phase_set;
	unsigned int val;

	if (host->timing == ios->timing) {
		dev_dbg(host->dev, "[%s]bail-out, timing\n",
				__func__);
		return;
	}

	writel(0, host->regs + SD_EMMC_DELAY1);
	writel(0, host->regs + SD_EMMC_DELAY2);
	writel(0, host->regs + SD_EMMC_INTF3);
	writel(0, host->regs + SD_EMMC_V3_ADJUST);
	val = readl(host->regs + SD_EMMC_IRQ_EN);
	val &= ~CFG_CMD_SETUP;
	writel(val, host->regs + SD_EMMC_IRQ_EN);
	switch (ios->timing) {
	case MMC_TIMING_MMC_HS400:
		val = readl(host->regs + SD_EMMC_V3_ADJUST);
		val |= DS_ENABLE;
		writel(val, host->regs + SD_EMMC_V3_ADJUST);
		val = readl(host->regs + SD_EMMC_IRQ_EN);
		val |= CFG_CMD_SETUP;
		writel(val, host->regs + SD_EMMC_IRQ_EN);
		val = readl(host->regs + SD_EMMC_INTF3);
		val |= SD_INTF3;
		if (host->mmc->caps2 & MMC_CAP2_HS400_ES) {
			ios->clock = host->mmc->f_max;
			val |= RESP_DS;
		}
		writel(val, host->regs + SD_EMMC_INTF3);
		mmc_phase_set = &host->sd_mmc.hs4;
		break;
	case MMC_TIMING_MMC_HS:
		val = readl(host->regs + host->data->adjust);
		val |= CFG_ADJUST_ENABLE;
		val &= ~CLK_ADJUST_DELAY;
		val |= CALI_HS_50M_ADJUST << __ffs(CLK_ADJUST_DELAY);
		writel(val, host->regs + host->data->adjust);
		mmc_phase_set = &host->sd_mmc.init;
		break;
	case MMC_TIMING_MMC_DDR52:
		mmc_phase_set = &host->sd_mmc.init;
		break;
	case MMC_TIMING_SD_HS:
		val = readl(host->regs + SD_EMMC_V3_ADJUST);
		val |= CFG_ADJUST_ENABLE;
		writel(val, host->regs + SD_EMMC_V3_ADJUST);
		mmc_phase_set = &host->sd_mmc.init;
		break;
	case MMC_TIMING_UHS_SDR12:
	case MMC_TIMING_UHS_SDR25:
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_UHS_SDR104:
		if (aml_card_type_sdio(host))
			sdio_get_device();
		mmc_phase_set = &host->sd_mmc.sdr;
		break;
	default:
		mmc_phase_set = &host->sd_mmc.init;
	}
	meson_mmc_set_phase_delay(host, CLK_CORE_PHASE_MASK,
				  mmc_phase_set->core_phase);
	meson_mmc_set_phase_delay(host, CLK_TX_PHASE_MASK,
				  mmc_phase_set->tx_phase);
	meson_mmc_set_phase_delay(host, CLK_TX_DELAY_MASK(host),
				  mmc_phase_set->tx_delay);

	host->timing = ios->timing;
	dev_dbg(host->dev, "[%s]set mmc timing:%u\n",
			__func__, ios->timing);
}

static void meson_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 bus_width, val;
	int err;

	/*
	 * GPIO regulator, only controls switching between 1v8 and
	 * 3v3, doesn't support MMC_POWER_OFF, MMC_POWER_ON.
	 */
	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		if (!IS_ERR(mmc->supply.vmmc))
			mmc_regulator_set_ocr(mmc, mmc->supply.vmmc, 0);

		if (!IS_ERR(mmc->supply.vqmmc) && host->vqmmc_enabled) {
			regulator_set_voltage_triplet(mmc->supply.vqmmc, 1700000, 1800000, 1950000);
			regulator_disable(mmc->supply.vqmmc);
			host->vqmmc_enabled = false;
		}

		break;

	case MMC_POWER_UP:
		if (!IS_ERR(mmc->supply.vmmc))
			mmc_regulator_set_ocr(mmc, mmc->supply.vmmc, ios->vdd);

		break;

	case MMC_POWER_ON:
		if (!IS_ERR(mmc->supply.vqmmc) && !host->vqmmc_enabled) {
			int ret = regulator_enable(mmc->supply.vqmmc);

			if (ret < 0)
				dev_err(host->dev,
					"failed to enable vqmmc regulator\n");
			else
				host->vqmmc_enabled = true;
		}

		break;
	}

	/* Bus width */
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_1:
		bus_width = CFG_BUS_WIDTH_1;
		break;
	case MMC_BUS_WIDTH_4:
		bus_width = CFG_BUS_WIDTH_4;
		break;
	case MMC_BUS_WIDTH_8:
		bus_width = CFG_BUS_WIDTH_8;
		break;
	default:
		dev_err(host->dev, "Invalid ios->bus_width: %u.  Setting to 4.\n",
			ios->bus_width);
		bus_width = CFG_BUS_WIDTH_4;
	}

	val = readl(host->regs + SD_EMMC_CFG);
	val &= ~CFG_BUS_WIDTH_MASK;
	val |= FIELD_PREP(CFG_BUS_WIDTH_MASK, bus_width);
	writel(val, host->regs + SD_EMMC_CFG);

	meson_mmc_check_resampling(host, ios);
	err = meson_mmc_prepare_ios_clock(host, ios);
	if (err)
		dev_err(host->dev, "Failed to set clock: %d\n,", err);

	dev_dbg(host->dev, "SD_EMMC_CFG:  0x%08x\n", val);
}

static void aml_sd_emmc_check_sdio_irq(struct meson_host *host)
{
	u32 vstat = readl(host->regs + SD_EMMC_STATUS);

	if (host->sdio_irqen) {
		if (((vstat & IRQ_SDIO) || (!(vstat & (1 << 17)))) &&
		    host->mmc->sdio_irq_thread &&
		    (!atomic_read(&host->mmc->sdio_irq_thread_abort))) {
			/* pr_info("signalling irq 0x%x\n", vstat); */
			mmc_signal_sdio_irq(host->mmc);
		}
	}
}

static void meson_mmc_request_done(struct mmc_host *mmc,
				   struct mmc_request *mrq)
{
	struct meson_host *host = mmc_priv(mmc);

	host->cmd = NULL;
	if (host->needs_pre_post_req)
		meson_mmc_post_req(mmc, mrq, 0);
	aml_sd_emmc_check_sdio_irq(host);
	mmc_request_done(host->mmc, mrq);
}

static void meson_mmc_set_blksz(struct mmc_host *mmc, unsigned int blksz)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 cfg, blksz_old;

	cfg = readl(host->regs + SD_EMMC_CFG);
	blksz_old = FIELD_GET(CFG_BLK_LEN_MASK, cfg);

	if (!is_power_of_2(blksz))
		dev_err(host->dev, "blksz %u is not a power of 2\n", blksz);

	blksz = ilog2(blksz);

	/* check if block-size matches, if not update */
	if (blksz == blksz_old)
		return;

	dev_dbg(host->dev, "%s: update blk_len %d -> %d\n", __func__,
		blksz_old, blksz);

	cfg &= ~CFG_BLK_LEN_MASK;
	cfg |= FIELD_PREP(CFG_BLK_LEN_MASK, blksz);
	writel(cfg, host->regs + SD_EMMC_CFG);
}

static void meson_mmc_set_response_bits(struct mmc_command *cmd, u32 *cmd_cfg)
{
	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136)
			*cmd_cfg |= CMD_CFG_RESP_128;
		*cmd_cfg |= CMD_CFG_RESP_NUM;

		if (!(cmd->flags & MMC_RSP_CRC))
			*cmd_cfg |= CMD_CFG_RESP_NOCRC;

		if (cmd->flags & MMC_RSP_BUSY)
			*cmd_cfg |= CMD_CFG_R1B;
	} else {
		*cmd_cfg |= CMD_CFG_NO_RESP;
	}
}

static void meson_mmc_desc_chain_transfer(struct mmc_host *mmc, u32 cmd_cfg,
					  struct mmc_command *cmd)
{
	struct meson_host *host = mmc_priv(mmc);
	struct sd_emmc_desc *desc = host->descs;
	struct mmc_data *data = host->cmd->data;
	struct scatterlist *sg;
	u32 start;
	int i, j = 0;

	if (data->flags & MMC_DATA_WRITE)
		cmd_cfg |= CMD_CFG_DATA_WR;

	if (data->blocks > 1) {
		cmd_cfg |= CMD_CFG_BLOCK_MODE;
		meson_mmc_set_blksz(mmc, data->blksz);
	}

	if (mmc_op_multi(cmd->opcode) && cmd->mrq->sbc) {
		desc[j].cmd_cfg = 0;
		desc[j].cmd_cfg |= FIELD_PREP(CMD_CFG_CMD_INDEX_MASK,
					      MMC_SET_BLOCK_COUNT);
		desc[j].cmd_cfg |= FIELD_PREP(CMD_CFG_TIMEOUT_MASK, 0xc);
		desc[j].cmd_cfg |= CMD_CFG_OWNER;
		desc[j].cmd_cfg |= CMD_CFG_RESP_NUM;
		desc[j].cmd_arg = cmd->mrq->sbc->arg;
		desc[j].cmd_resp = 0;
		desc[j].cmd_data = 0;
		j++;
	}

	for_each_sg(data->sg, sg, data->sg_count, i) {
		unsigned int len = sg_dma_len(sg);

		if (data->blocks > 1)
			len /= data->blksz;

		desc[i + j].cmd_cfg = cmd_cfg;
		desc[i + j].cmd_cfg |= FIELD_PREP(CMD_CFG_LENGTH_MASK, len);
		if (i > 0)
			desc[i + j].cmd_cfg |= CMD_CFG_NO_CMD;
		desc[i + j].cmd_arg = host->cmd->arg;
		desc[i + j].cmd_resp = 0;
		desc[i + j].cmd_data = sg_dma_address(sg);
	}

	if (mmc_op_multi(cmd->opcode) && !cmd->mrq->sbc) {
		desc[data->sg_count].cmd_cfg = 0;
		desc[data->sg_count].cmd_cfg |=
			FIELD_PREP(CMD_CFG_CMD_INDEX_MASK,
				   MMC_STOP_TRANSMISSION);
		desc[data->sg_count].cmd_cfg |=
			FIELD_PREP(CMD_CFG_TIMEOUT_MASK, 0xc);
		desc[data->sg_count].cmd_cfg |= CMD_CFG_OWNER;
		desc[data->sg_count].cmd_cfg |= CMD_CFG_RESP_NUM;
		desc[data->sg_count].cmd_cfg |= CMD_CFG_R1B;
		desc[data->sg_count].cmd_resp = 0;
		desc[data->sg_count].cmd_data = 0;
		j++;
	}

	desc[data->sg_count + j - 1].cmd_cfg |= CMD_CFG_END_OF_CHAIN;
	dma_wmb(); /* ensure descriptor is written before kicked */
	start = host->descs_dma_addr | START_DESC_BUSY;
	writel(start, host->regs + SD_EMMC_START);
}

static void meson_mmc_quirk_transfer(struct mmc_host *mmc, u32 cmd_cfg,
					  struct mmc_command *cmd)
{
	struct meson_host *host = mmc_priv(mmc);
	struct sd_emmc_desc *desc = host->descs;
	struct mmc_data *data = host->cmd->data;
	u32 start, data_len;

	if (data->blocks > 1) {
		cmd_cfg |= CMD_CFG_BLOCK_MODE;
		meson_mmc_set_blksz(mmc, data->blksz);
		data_len = data->blocks;
	} else {
		data_len = data->blksz;
	}

	if (mmc_op_multi(cmd->opcode) && cmd->mrq->sbc) {
		desc->cmd_cfg = 0;
		desc->cmd_cfg |= FIELD_PREP(CMD_CFG_CMD_INDEX_MASK,
					      MMC_SET_BLOCK_COUNT);
		desc->cmd_cfg |= FIELD_PREP(CMD_CFG_TIMEOUT_MASK, 0xc);
		desc->cmd_cfg |= CMD_CFG_OWNER;
		desc->cmd_cfg |= CMD_CFG_RESP_NUM;
		desc->cmd_arg = cmd->mrq->sbc->arg;
		desc->cmd_resp = 0;
		desc->cmd_data = 0;
		desc++;
	}

	desc->cmd_cfg = cmd_cfg;
	desc->cmd_cfg |= FIELD_PREP(CMD_CFG_LENGTH_MASK, data_len);
	desc->cmd_arg = host->cmd->arg;
	desc->cmd_resp = 0;
	desc->cmd_data = host->bounce_dma_addr;

	if (mmc_op_multi(cmd->opcode) && !cmd->mrq->sbc) {
		desc++;
		desc->cmd_cfg = 0;
		desc->cmd_cfg |= FIELD_PREP(CMD_CFG_CMD_INDEX_MASK,
				   MMC_STOP_TRANSMISSION);
		desc->cmd_cfg |= FIELD_PREP(CMD_CFG_TIMEOUT_MASK, 0xc);
		desc->cmd_cfg |= CMD_CFG_OWNER;
		desc->cmd_cfg |= CMD_CFG_RESP_NUM;
		desc->cmd_cfg |= CMD_CFG_R1B;
		desc->cmd_resp = 0;
		desc->cmd_data = 0;
	}

	desc->cmd_cfg |= CMD_CFG_END_OF_CHAIN;

	dma_wmb(); /* ensure descriptor is written before kicked */
	start = host->descs_dma_addr | START_DESC_BUSY | CMD_DATA_SRAM;
	writel(start, host->regs + SD_EMMC_START);
}

static void meson_mmc_start_cmd(struct mmc_host *mmc, struct mmc_command *cmd)
{
	struct meson_host *host = mmc_priv(mmc);
	struct mmc_data *data = cmd->data;
	u32 val, cmd_cfg = 0, cmd_data = 0;
	unsigned int xfer_bytes = 0;

	/* Setup descriptors */
	dma_rmb();

	host->cmd = cmd;

	cmd_cfg |= FIELD_PREP(CMD_CFG_CMD_INDEX_MASK, cmd->opcode);
	cmd_cfg |= CMD_CFG_OWNER;  /* owned by CPU */

	meson_mmc_set_response_bits(cmd, &cmd_cfg);

	if (cmd->opcode == SD_SWITCH_VOLTAGE) {
		val = readl(host->regs + SD_EMMC_CFG);
		val &= ~CFG_AUTO_CLK;
		writel(val, host->regs + SD_EMMC_CFG);
	}

	/* data? */
	if (data) {
		data->bytes_xfered = 0;
		cmd_cfg |= CMD_CFG_DATA_IO;
		cmd_cfg |= FIELD_PREP(CMD_CFG_TIMEOUT_MASK,
				      ilog2(meson_mmc_get_timeout_msecs(data)));

		if (meson_mmc_desc_chain_mode(data)) {
			meson_mmc_desc_chain_transfer(mmc, cmd_cfg, cmd);
			return;
		}

		if (data->blocks > 1) {
			cmd_cfg |= CMD_CFG_BLOCK_MODE;
			cmd_cfg |= FIELD_PREP(CMD_CFG_LENGTH_MASK,
					      data->blocks);
			meson_mmc_set_blksz(mmc, data->blksz);
		} else {
			cmd_cfg |= FIELD_PREP(CMD_CFG_LENGTH_MASK, data->blksz);
		}

		xfer_bytes = data->blksz * data->blocks;
		if (data->flags & MMC_DATA_WRITE) {
			cmd_cfg |= CMD_CFG_DATA_WR;
			WARN_ON(xfer_bytes > host->bounce_buf_size);
			sg_copy_to_buffer(data->sg, data->sg_len,
					  host->bounce_buf, xfer_bytes);
			dma_wmb();
		}

		cmd_data = host->bounce_dma_addr & CMD_DATA_MASK;

		if (host->dram_access_quirk) {
			meson_mmc_quirk_transfer(mmc, cmd_cfg, cmd);
			return;
		}
	} else {
		/* Set timeout according to the setting value in ext_csd */
		cmd_cfg |= FIELD_PREP(CMD_CFG_TIMEOUT_MASK,
				      ilog2(meson_mmc_get_cmd_timeout_msecs(cmd)));
	}

	/* Last descriptor */
	cmd_cfg |= CMD_CFG_END_OF_CHAIN;
	writel(cmd_cfg, host->regs + SD_EMMC_CMD_CFG);
	writel(cmd_data, host->regs + SD_EMMC_CMD_DAT);
	writel(0, host->regs + SD_EMMC_CMD_RSP);
	wmb(); /* ensure descriptor is written before kicked */
	writel(cmd->arg, host->regs + SD_EMMC_CMD_ARG);
}

static void meson_mmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct meson_host *host = mmc_priv(mmc);

	host->needs_pre_post_req = mrq->data &&
			!(mrq->data->host_cookie & SD_EMMC_PRE_REQ_DONE);

	if (host->needs_pre_post_req) {
		meson_mmc_get_transfer_mode(mmc, mrq);
		if (!meson_mmc_desc_chain_mode(mrq->data))
			host->needs_pre_post_req = false;
	}

	if (host->needs_pre_post_req)
		meson_mmc_pre_req(mmc, mrq);

	/* Stop execution */
	writel(0, host->regs + SD_EMMC_START);

	meson_mmc_start_cmd(mmc, mrq->cmd);
}

static int aml_sd_emmc_cali_v3(struct mmc_host *mmc,
			       u8 opcode, u8 *blk_test, u32 blksz,
			       u32 blocks, u8 *pattern)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_command stop = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;

	cmd.opcode = opcode;
	if (!strcmp(pattern, MMC_PATTERN_NAME))
		cmd.arg = MMC_PATTERN_OFFSET;
	else if (!strcmp(pattern, MMC_MAGIC_NAME))
		cmd.arg = MMC_MAGIC_OFFSET;
	else if (!strcmp(pattern, MMC_RANDOM_NAME))
		cmd.arg = MMC_RANDOM_OFFSET;
	else if (!strcmp(pattern, MMC_DTB_NAME))
		cmd.arg = MMC_DTB_OFFSET;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	stop.opcode = MMC_STOP_TRANSMISSION;
	stop.arg = 0;
	stop.flags = MMC_RSP_R1B | MMC_CMD_AC;
	data.blksz = blksz;
	data.blocks = blocks;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;
	data.timeout_ns = 2048000000;
	memset(blk_test, 0, blksz * data.blocks);
	sg_init_one(&sg, blk_test, blksz * data.blocks);
	mrq.cmd = &cmd;
	mrq.stop = &stop;
	mrq.data = &data;
	mmc_wait_for_req(mmc, &mrq);
	return data.error | cmd.error;
}

static int emmc_send_cmd(struct mmc_host *mmc, u32 opcode,
			 u32 arg, unsigned int flags)
{
	struct mmc_command cmd = {0};
	u32 err = 0;

	cmd.opcode = opcode;
	cmd.arg = arg;
	cmd.flags = flags;
	err = mmc_wait_for_cmd(mmc, &cmd, 0);
	if (err) {
		pr_debug("[%s][%d] cmd:0x%x send error\n",
			 __func__, __LINE__, cmd.opcode);
		return err;
	}
	return err;
}

static int aml_sd_emmc_cmd_v3(struct mmc_host *mmc)
{
	int i;

	emmc_send_cmd(mmc, MMC_SEND_STATUS,
		      1 << 16, MMC_RSP_R1 | MMC_CMD_AC);
	emmc_send_cmd(mmc, MMC_SELECT_CARD,
		      0, MMC_RSP_NONE | MMC_CMD_AC);
	for (i = 0; i < 2; i++)
		emmc_send_cmd(mmc, MMC_SEND_CID,
			      1 << 16, MMC_RSP_R2 | MMC_CMD_BCR);
	emmc_send_cmd(mmc, MMC_SELECT_CARD,
		      1 << 16, MMC_RSP_R1 | MMC_CMD_AC);
	return 0;
}

static int emmc_eyetest_log(struct mmc_host *mmc, u32 line_x)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 adjust = readl(host->regs + SD_EMMC_V3_ADJUST);
	u32 eyetest_log = 0;
	u32 eyetest_out0 = 0, eyetest_out1 = 0;
	u32 intf3 = readl(host->regs + SD_EMMC_INTF3);
	int retry = 3;
	u64 tmp = 0;
	u32 blksz = 512;

	pr_debug("delay1: 0x%x , delay2: 0x%x, line_x: %d\n",
		 readl(host->regs + SD_EMMC_DELAY1),
		 readl(host->regs + SD_EMMC_DELAY2), line_x);
	adjust |= CALI_ENABLE;
	adjust &= ~CALI_SEL_MASK;
	adjust |= line_x << __ffs(CALI_SEL_MASK);
	writel(adjust, host->regs + SD_EMMC_V3_ADJUST);
	if (line_x < 9) {
		intf3 &= ~EYETEST_EXP_MASK;
		intf3 |= 7 << __ffs(EYETEST_EXP_MASK);
	} else {
		intf3 &= ~EYETEST_EXP_MASK;
		intf3 |= 3 << __ffs(EYETEST_EXP_MASK);
	}
RETRY:
	intf3 |= EYETEST_ON;
	writel(intf3, host->regs + SD_EMMC_INTF3);
	udelay(5);
	if (line_x < 9)
		aml_sd_emmc_cali_v3(mmc, MMC_READ_MULTIPLE_BLOCK,
				    host->blk_test, blksz,
				    40, MMC_PATTERN_NAME);
	else
		aml_sd_emmc_cmd_v3(mmc);
	udelay(1);
	eyetest_log = readl(host->regs + SD_EMMC_EYETEST_LOG);
	if (!(eyetest_log & EYETEST_DONE)) {
		pr_debug("testing eyetest times:0x%x,out:0x%x,0x%x,line:%d\n",
			 readl(host->regs + SD_EMMC_EYETEST_LOG),
			 eyetest_out0, eyetest_out1, line_x);
		intf3 &= ~EYETEST_ON;
		writel(intf3, host->regs + SD_EMMC_INTF3);
		retry--;
		if (retry == 0) {
			pr_debug("[%s][%d] retry eyetest failed-line:%d\n",
				 __func__, __LINE__, line_x);
			return 1;
		}
		goto RETRY;
	}
	eyetest_out0 = readl(host->regs + SD_EMMC_EYETEST_OUT0);
	eyetest_out1 = readl(host->regs + SD_EMMC_EYETEST_OUT1);
	intf3 &= ~EYETEST_ON;
	writel(intf3, host->regs + SD_EMMC_INTF3);
	writel(0, host->regs + SD_EMMC_V3_ADJUST);
	host->align[line_x] = ((tmp | eyetest_out1) << 32) | eyetest_out0;
	pr_debug("d1:0x%x,d2:0x%x,u64eyet:0x%016llx,l_x:%d\n",
		 readl(host->regs + SD_EMMC_DELAY1),
		 readl(host->regs + SD_EMMC_DELAY2),
		 host->align[line_x], line_x);
	return 0;
}

static int single_read_scan(struct mmc_host *mmc, u8 opcode,
			    u8 *blk_test, u32 blksz,
			    u32 blocks, u32 offset)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;

	cmd.opcode = opcode;
	cmd.arg = offset;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
	data.blksz = blksz;
	data.blocks = blocks;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;
	data.timeout_ns = 2048000000;
	memset(blk_test, 0, blksz * data.blocks);
	sg_init_one(&sg, blk_test, blksz * data.blocks);
	mrq.cmd = &cmd;
	mrq.data = &data;
	mmc_wait_for_req(mmc, &mrq);
	return data.error | cmd.error;
}

static void emmc_show_cmd_window(char *str, int repeat_times)
{
	int pre_status = 0;
	int status = 0;
	int single = 0;
	int start = 0;
	int i;

	pr_info(">>>>>>>>>>>>>>scan command window>>>>>>>>>>>>>>>\n");
	for (i = 0; i < 64; i++) {
		if (str[i] == repeat_times)
			status = 1;
		else
			status = -1;
		if (i != 0 && pre_status != status) {
			if (pre_status == 1 && single == 1)
				pr_info(">>cmd delay [ %d ] is ok\n",
					i - 1);
			else if (pre_status == 1 && single != 1)
				pr_info(">>cmd delay [ %d -- %d ] is ok\n",
					start, i - 1);
			else if (pre_status != 1 &&	 single == 1)
				pr_info(">>cmd delay [ %d ] is nok\n",
					i - 1);
			else if (pre_status != 1 && single != 1)
				pr_info(">>cmd delay [ %d -- %d ] is nok\n",
					start, i - 1);
			start = i;
			single = 1;
		} else {
			single++;
		}
		if (i == 63) {
			if (status == 1 && pre_status == 1)
				pr_info(">>cmd delay [ %d -- %d ] is ok\n",
					start, i);
			else if (status != 1 && pre_status == -1)
				pr_info(">>cmd delay [ %d -- %d ] is nok\n",
					start, i);
			else if (status == 1 && pre_status != 1)
				pr_info(">>cmd delay [ %d ] is ok\n", i);
			else if (status != 1 && pre_status == 1)
				pr_info(">>cmd delay [ %d ] is nok\n", i);
		}
		pre_status = status;
	}
	pr_info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

static u32 emmc_search_cmd_delay(char *str, int repeat_times, u32 *p_size)
{
	int best_start = -1, best_size = -1;
	int cur_start = -1, cur_size = 0;
	u32 cmd_delay;
	int i;

	for (i = 0; i < 64; i++) {
		if (str[i] == repeat_times) {
			cur_size += 1;
			if (cur_start == -1)
				cur_start = i;
		} else {
			cur_size = 0;
			cur_start = -1;
		}
		if (cur_size > best_size) {
			best_size = cur_size;
			best_start = cur_start;
		}
	}
	cmd_delay = (best_start + best_size / 2);
	if (p_size)
		*p_size = best_size;
	pr_info("cmd-best-c:%d, cmd-best-size:%d\n",
		cmd_delay, best_size);
	return cmd_delay;
}

static u32 scan_emmc_cmd_win(struct mmc_host *mmc,
		int send_status, u32 *pcmd_size)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay2 = readl(host->regs + SD_EMMC_DELAY2);
	u32 cmd_delay = 0;
	u32 delay2_bak = delay2;
	u32 i, j, err;
	int repeat_times = 100;
	char str[64] = {0};
	long long before_time;
	long long after_time;
	u32 capacity = 2 * SZ_1M;
	u32 offset;

	delay2 &= ~(0xff << 24);
	host->is_tuning = 1;
	before_time = sched_clock();
	for (i = 0; i < 64; i++) {
		writel(delay2, host->regs + SD_EMMC_DELAY2);
		offset = (u32)(get_random_long() % capacity);
		for (j = 0; j < repeat_times; j++) {
			if (send_status) {
				err = emmc_send_cmd(mmc, MMC_SEND_STATUS,
						    1 << 16,
						    MMC_RSP_R1 | MMC_CMD_AC);
			} else {
				err = single_read_scan(mmc,
						       MMC_READ_SINGLE_BLOCK,
						       host->blk_test, 512, 1,
						       offset);
				emmc_send_cmd(mmc, MMC_STOP_TRANSMISSION,
						    0,
						    MMC_RSP_R1 | MMC_CMD_AC);
				emmc_send_cmd(mmc, MMC_SEND_STATUS,
						    1 << 16,
						    MMC_RSP_R1 | MMC_CMD_AC);
			}
			if (!err)
				str[i]++;
			else
				break;
		}
		pr_debug("delay2: 0x%x, send cmd %d times success %d times, is ok\n",
				 delay2, repeat_times, str[i]);
		delay2 += (1 << 24);
	}
	after_time = sched_clock();
	host->is_tuning = 0;
	pr_debug("scan time distance: %llu ns\n", after_time - before_time);
	writel(delay2_bak, host->regs + SD_EMMC_DELAY2);
	cmd_delay = emmc_search_cmd_delay(str, repeat_times, pcmd_size);
	if (!send_status)
		emmc_show_cmd_window(str, repeat_times);
	return cmd_delay;
}

ssize_t emmc_scan_cmd_win(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct meson_host *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->mmc;

	mmc_claim_host(mmc);
	scan_emmc_cmd_win(mmc, 1, NULL);
	mmc_release_host(mmc);
	return sprintf(buf, "%s\n", "Emmc scan command window.\n");
}

static void update_all_line_eyetest(struct mmc_host *mmc)
{
	int line_x;

	for (line_x = 0; line_x < 10; line_x++) {
		if (line_x == 8 && !(mmc->caps2 & MMC_CAP2_HS400_1_8V))
			continue;
		emmc_eyetest_log(mmc, line_x);
	}
}

int emmc_clktest(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 intf3 = readl(host->regs + SD_EMMC_INTF3);
	u32 clktest = 0, delay_cell = 0, clktest_log = 0, count = 0;
	u32 vcfg = readl(host->regs + SD_EMMC_CFG);
	int i = 0;
	unsigned int cycle = 0;

	writel(0, (host->regs + SD_EMMC_V3_ADJUST));
	cycle = (1000000000 / mmc->actual_clock) * 1000;
	vcfg &= ~(1 << 23);
	writel(vcfg, host->regs + SD_EMMC_CFG);
	intf3 &= ~CLKTEST_EXP_MASK;
	intf3 |= 8 << __ffs(CLKTEST_EXP_MASK);
	intf3 |= CLKTEST_ON_M;
	writel(intf3, host->regs + SD_EMMC_INTF3);
	clktest_log = readl(host->regs + SD_EMMC_CLKTEST_LOG);
	clktest = readl(host->regs + SD_EMMC_CLKTEST_OUT);
	while (!(clktest_log & 0x80000000)) {
		udelay(1);
		i++;
		clktest_log = readl(host->regs + SD_EMMC_CLKTEST_LOG);
		clktest = readl(host->regs + SD_EMMC_CLKTEST_OUT);
		if (i > 4000) {
			pr_warn("[%s] [%d] emmc clktest error\n",
				__func__, __LINE__);
			break;
		}
	}
	if (clktest_log & 0x80000000) {
		clktest = readl(host->regs + SD_EMMC_CLKTEST_OUT);
		count = clktest / (1 << 8);
		if (vcfg & 0x4)
			delay_cell = ((cycle / 2) / count);
		else
			delay_cell = (cycle / count);
	}
	pr_info("%s [%d] clktest : %u, delay_cell: %d, count: %u\n",
		__func__, __LINE__, clktest, delay_cell, count);
	intf3 = readl(host->regs + SD_EMMC_INTF3);
	intf3 &= ~CLKTEST_ON_M;
	writel(intf3, host->regs + SD_EMMC_INTF3);
	vcfg = readl(host->regs + SD_EMMC_CFG);
	vcfg |= (1 << 23);
	writel(vcfg, host->regs + SD_EMMC_CFG);
	return count;
}

static unsigned int tl1_emmc_line_timing(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay1 = 0, delay2 = 0, count = 12;

	delay1 = (count << 0) | (count << 6) | (count << 12) |
		(count << 18) | (count << 24);
	delay2 = (count << 0) | (count << 6) | (count << 12) |
		(host->cmd_c << 24);
	writel(delay1, host->regs + SD_EMMC_DELAY1);
	writel(delay2, host->regs + SD_EMMC_DELAY2);
	pr_debug("[%s], delay1: 0x%x, delay2: 0x%x\n",
		__func__, readl(host->regs + SD_EMMC_DELAY1),
		readl(host->regs + SD_EMMC_DELAY2));
	return 0;
}

static int single_read_cmd_for_scan(struct mmc_host *mmc,
		 u8 opcode, u8 *blk_test, u32 blksz, u32 blocks, u32 offset)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;

	cmd.opcode = opcode;
	cmd.arg = offset;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = blksz;
	data.blocks = blocks;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	memset(blk_test, 0, blksz * data.blocks);
	sg_init_one(&sg, blk_test, blksz * data.blocks);

	mrq.cmd = &cmd;
	mrq.data = &data;
	mmc_wait_for_req(mmc, &mrq);
	return data.error | cmd.error;
}

static int emmc_test_bus(struct mmc_host *mmc)
{
	int err = 0;
	u32 blksz = 512, cali_blk_cnt;
	struct meson_host *host = mmc_priv(mmc);
	struct device *dev = mmc->parent;

	err = aml_sd_emmc_cali_v3(mmc, MMC_READ_MULTIPLE_BLOCK,
				  host->blk_test, blksz, 40, MMC_PATTERN_NAME);
	if (err)
		return err;

	if (device_property_read_u32(dev, "cali_blk_cnt", &cali_blk_cnt) <= 0)
		cali_blk_cnt = CALI_BLK_CNT;
	err = aml_sd_emmc_cali_v3(mmc, MMC_READ_MULTIPLE_BLOCK,
				  host->blk_test, blksz, cali_blk_cnt, MMC_RANDOM_NAME);
	if (err)
		return err;

	err = aml_sd_emmc_cali_v3(mmc, MMC_READ_MULTIPLE_BLOCK,
				  host->blk_test, blksz, 40, MMC_MAGIC_NAME);
	if (err)
		return err;

	return err;
}

static int emmc_ds_manual_sht(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 val, intf3 = readl(host->regs + SD_EMMC_INTF3);
	int i, err = 0;
	int match[64], size = 0;
	int best_start = -1, best_size = -1;
	int cur_start = -1, cur_size = 0;

	memset(match, -1, sizeof(match));
	for (i = 0; i < 64; i++) {
		host->is_tuning = 1;
		err = emmc_test_bus(mmc);
		host->is_tuning = 0;
		pr_debug("intf3: 0x%x, err[%d]: %d\n",
			 readl(host->regs + SD_EMMC_INTF3), i, err);
		if (!err) {
			match[i] = 0;
			++size;
		} else {
			match[i] = -1;
			if (size > DELAY_CELL_COUNTS)
				break;
		}
		val = intf3 & DS_SHT_M_MASK;
		val += 1 << __ffs(DS_SHT_M_MASK);
		intf3 &= ~DS_SHT_M_MASK;
		intf3 |= val;
		writel(intf3, host->regs + SD_EMMC_INTF3);
	}
	for (i = 0; i < 64; i++) {
		if (match[i] == 0) {
			if (cur_start < 0)
				cur_start = i;
			cur_size++;
		} else {
			if (cur_start >= 0) {
				if (best_start < 0) {
					best_start = cur_start;
					best_size = cur_size;
				} else {
					if (best_size < cur_size) {
						best_start = cur_start;
						best_size = cur_size;
					}
				}
				cur_start = -1;
				cur_size = 0;
			}
		}
	}
	if (cur_start >= 0) {
		if (best_start < 0) {
			best_start = cur_start;
			best_size = cur_size;
		} else if (best_size < cur_size) {
			best_start = cur_start;
			best_size = cur_size;
		}
		cur_start = -1;
		cur_size = -1;
	}
	intf3 &= ~DS_SHT_M_MASK;
	intf3 &= ~DS_SHT_EXP_MASK;
	intf3 |= (best_start + best_size / 2) << __ffs(DS_SHT_M_MASK);
	writel(intf3, host->regs + SD_EMMC_INTF3);
	pr_info("ds_sht:%lu, window:%d, intf3:0x%x, clock:0x%x, adjust:0x%x",
		(intf3 & DS_SHT_M_MASK) >> __ffs(DS_SHT_M_MASK), best_size,
		readl(host->regs + SD_EMMC_INTF3),
		readl(host->regs + SD_EMMC_CLOCK),
		readl(host->regs + SD_EMMC_V3_ADJUST));
	return 0;
}

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static int emmc_data_alignment(struct mmc_host *mmc, int best_size)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay1 = readl(host->regs + SD_EMMC_DELAY1);
	u32 delay2 = readl(host->regs + SD_EMMC_DELAY2);
	u32 intf3 = readl(host->regs + SD_EMMC_INTF3);
	u32 delay1_bak = delay1;
	u32 delay2_bak = delay2;
	u32 intf3_bak = intf3;
	int line_x, i, err = 0, win_new, blksz = 512;
	u32 d[8];

	host->is_tuning = 1;
	intf3 &= ~DS_SHT_M_MASK;
	intf3 |= (host->win_start + 4) << __ffs(DS_SHT_M_MASK);
	writel(intf3, host->regs + SD_EMMC_INTF3);
	for (line_x = 0; line_x < 8; line_x++) {
		for (i = 0; i < 20; i++) {
			if (line_x < 5) {
				delay1 += (1 << 6 * line_x);
				writel(delay1, host->regs + SD_EMMC_DELAY1);
			} else {
				delay2 += (1 << 6 * (line_x - 5));
				writel(delay2, host->regs + SD_EMMC_DELAY2);
			}
			err = aml_sd_emmc_cali_v3(mmc, MMC_READ_MULTIPLE_BLOCK,
						  host->blk_test, blksz, 40,
						  MMC_PATTERN_NAME);
			if (err) {
				pr_info("[%s]adjust line_x[%d]:%d\n",
					__func__, line_x, i);
				d[line_x] = i;
				delay1 = delay1_bak;
				delay2 = delay2_bak;
				writel(delay1_bak, host->regs + SD_EMMC_DELAY1);
				writel(delay2_bak, host->regs + SD_EMMC_DELAY2);
				break;
			}
		}
		if (i == 20) {
			pr_info("[%s][%d] return set default value",
				__func__, __LINE__);
			writel(delay1_bak, host->regs + SD_EMMC_DELAY1);
			writel(delay2_bak, host->regs + SD_EMMC_DELAY2);
			writel(intf3_bak, host->regs + SD_EMMC_INTF3);
			host->is_tuning = 0;
			return -1;
		}
	}
	delay1 += (d[0] << 0) | (d[1] << 6) | (d[2] << 12) |
		(d[3] << 18) | (d[4] << 24);
	delay2 += (d[5] << 0) | (d[6] << 6) | (d[7] << 12);
	writel(delay1, host->regs + SD_EMMC_DELAY1);
	writel(delay2, host->regs + SD_EMMC_DELAY2);
	pr_info("delay1:0x%x, delay2:0x%x\n",
		readl(host->regs + SD_EMMC_DELAY1),
		readl(host->regs + SD_EMMC_DELAY2));
	intf3 &= ~DS_SHT_M_MASK;
	writel(intf3, host->regs + SD_EMMC_INTF3);
	win_new = emmc_ds_manual_sht(mmc);
	if (win_new < best_size) {
		pr_info("[%s][%d] win_new:%d < win_old:%d,set default!",
			__func__, __LINE__, win_new, best_size);
		writel(delay1_bak, host->regs + SD_EMMC_DELAY1);
		writel(delay2_bak, host->regs + SD_EMMC_DELAY2);
		writel(intf3_bak, host->regs + SD_EMMC_INTF3);
		pr_info("intf3:0x%x, delay1:0x%x, delay2:0x%x\n",
			readl(host->regs + SD_EMMC_INTF3),
			readl(host->regs + SD_EMMC_DELAY1),
			readl(host->regs + SD_EMMC_DELAY2));
	}
	host->is_tuning = 0;
	return 0;
}
#endif

static u32 set_emmc_cmd_delay(struct mmc_host *mmc, int send_status)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay2 = readl(host->regs + SD_EMMC_DELAY2);
	u32 cmd_delay = 0, cmd_size = 0;

	delay2 &= ~(0xff << 24);
	cmd_delay = scan_emmc_cmd_win(mmc, send_status, &cmd_size);
	delay2 |= (cmd_delay << __ffs(DELAY2_CMD_MASK));
	writel(delay2, host->regs + SD_EMMC_DELAY2);
	return cmd_size;
}

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static void __maybe_unused aml_emmc_hs400_revb(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay2 = 0;
	int win_size = 0;

	delay2 = readl(host->regs + SD_EMMC_DELAY2);
	delay2 += (host->cmd_c << 24);
	writel(delay2, host->regs + SD_EMMC_DELAY2);
	pr_info("[%s], delay1: 0x%x, delay2: 0x%x\n",
		__func__, readl(host->regs + SD_EMMC_DELAY1),
		readl(host->regs + SD_EMMC_DELAY2));
	win_size = emmc_ds_manual_sht(mmc);
	emmc_data_alignment(mmc, win_size);
	set_emmc_cmd_delay(mmc, 0);
}
#endif

static void aml_emmc_hs400_tl1(struct mmc_host *mmc)
{
	u32 cmd_size = 0;

	mmc->retune_crc_disable = true;
	tl1_emmc_line_timing(mmc);
	cmd_size = set_emmc_cmd_delay(mmc, 1);
	emmc_ds_manual_sht(mmc);
	if (cmd_size >= EMMC_CMD_WIN_MAX_SIZE)
		set_emmc_cmd_delay(mmc, 0);
}

static long long _para_checksum_calc(struct aml_tuning_para *para)
{
	int i = 0;
	int size = sizeof(struct aml_tuning_para) - 6 * sizeof(unsigned int);
	unsigned int *buffer;
	long long checksum = 0;

	if (!para)
		return 1;

	size = size >> 2;
	buffer = (unsigned int *)para;
	while (i < size)
		checksum += buffer[i++];

	return checksum;
}

/*
 * read tuning para from reserved partition
 * and copy it to pdata->para
 */
int aml_read_tuning_para(struct mmc_host *mmc)
{
	int off, blk;
	int ret;
	int para_size;
	struct meson_host *host = mmc_priv(mmc);

	if (host->save_para == 0)
		return 0;

	para_size = sizeof(struct aml_tuning_para);
	blk = (para_size - 1) / 512 + 1;
	off = MMC_TUNING_OFFSET;

	ret = single_read_cmd_for_scan(mmc,
				       MMC_READ_SINGLE_BLOCK,
				       host->blk_test, 512,
				       blk, off);
	if (ret) {
		pr_info("read tuning parameter failed\n");
		return ret;
	}

	memcpy(&host->para, host->blk_test, para_size);
	return ret;
}

/*set para on controller register*/
static void aml_set_tuning_para(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	struct aml_tuning_para *para = &host->para;
	int temp_index;
	u32 delay1, delay2, intf3;
	int cmd_delay;

	if (host->compute_cmd_delay == 1) {
		temp_index = host->first_temp_index;
		delay1 = host->para.hs4[temp_index].delay1;
		delay2 = host->para.hs4[temp_index].delay2;
		intf3 = host->para.hs4[temp_index].intf3;

		cmd_delay = ((10 + host->compute_coef) * (host->cur_temp_index
				- host->first_temp_index)) / 10;

		delay2 -= cmd_delay << 24;
		pr_info("bef %d, cur %d, delay switch from 0x%x to 0x%x\n",
				host->first_temp_index,
				host->cur_temp_index,
				host->para.hs4[temp_index].delay2,
				delay2);

		writel(delay1, host->regs + SD_EMMC_DELAY1);
		writel(delay2, host->regs + SD_EMMC_DELAY2);
		writel(intf3, host->regs + SD_EMMC_INTF3);
	} else {
		temp_index = para->temperature / 10000;
		delay1 = host->para.hs4[temp_index].delay1;
		delay2 = host->para.hs4[temp_index].delay2;
		intf3 = host->para.hs4[temp_index].intf3;

		writel(delay1, host->regs + SD_EMMC_DELAY1);
		writel(delay2, host->regs + SD_EMMC_DELAY2);
		writel(intf3, host->regs + SD_EMMC_INTF3);
	}
}

/*save parameter on mmc_host pdata*/
static void aml_save_tuning_para(struct mmc_host *mmc)
{
	long long checksum;
	int temp_index;
	struct meson_host *host = mmc_priv(mmc);
	struct aml_tuning_para *para = &host->para;

	u32 delay1 = readl(host->regs + SD_EMMC_DELAY1);
	u32 delay2 = readl(host->regs + SD_EMMC_DELAY2);
	u32 intf3 = readl(host->regs + SD_EMMC_INTF3);

	if (host->save_para == 0)
		return;

	temp_index = para->temperature / 10000;
	if (para->temperature < 0 || temp_index > 6) {
		para->update = 0;
		return;
	}

	host->para.hs4[temp_index].delay1 = delay1;
	host->para.hs4[temp_index].delay2 = delay2;
	host->para.hs4[temp_index].intf3 = intf3;
	host->para.hs4[temp_index].flag = TUNED_FLAG;
	host->para.magic = TUNED_MAGIC; /*E~K\0*/
	host->para.version = TUNED_VERSION;

	checksum = _para_checksum_calc(para);
	host->para.checksum = checksum;
}

/*
 * check if tuning parameter is exist
 * check if temperature is in the 0~69
 * check if the parameter has been tuning
 *		under the current temperature
 * check if the data had been broken by  checksum
 *
 * if all four condition above is yes, the tuning parameter
 *		could be use directly
 * otherwise returning and save parameter
 */
static int aml_para_is_exist(struct mmc_host *mmc)
{
	int temperature;
	int temp_index;
	long long checksum;
	struct meson_host *host = mmc_priv(mmc);
	struct aml_tuning_para *para = &host->para;
	int i;

	if (host->save_para == 0)
		return 0;

	para->update = 1;
	temperature = -1;

	if (temperature == -1) {
		para->update = 0;
		pr_info("get temperature failed\n");
		return 0;
	}
	pr_info("current temperature is %d\n", temperature);

	temp_index = temperature / 10000;
	para->temperature = temperature;

	checksum = _para_checksum_calc(para);
	if (checksum != para->checksum) {
		pr_info("warning: checksum is not match\n");
		return 0;
	}

	if (para->magic != TUNED_MAGIC) {
		pr_warn("[%s] magic is not match\n", __func__);
		return 0;
	}

	if (para->version != TUNED_VERSION) {
		pr_warn("[%s] VERSION is not match\n", __func__);
		return 0;
	}

	if (host->compute_cmd_delay == 1) {
		for (i = 0; i < 7; i++) {
			if (para->hs4[i].flag == TUNED_FLAG) {
				host->first_temp_index = i;
				host->cur_temp_index = temp_index;
				para->update = 0;
				return 1;
			}
		}
	}

	/* temperature range is 0 ~ 69 */
	if (temperature < 0 || temp_index > 6) {
		pr_info("temperature is out of normal range\n");
		return 0;
	}

	if (para->hs4[temp_index].flag != TUNED_FLAG) {
		pr_info("current temperature %d degree not tuning yet\n",
			temperature / 1000);
		return 0;
	}

	para->update = 0;

	return 1;
}

/* Insufficient number of NWR clocks in T7 EMMC Controller */
static void set_emmc_nwr_clks(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay1 = 0, delay2 = 0, count = host->nwr_cnt;

	delay1 = (count << 0) | (count << 6) | (count << 12) |
		(count << 18) | (count << 24);
	delay2 = (count << 0) | (count << 6) | (count << 12) |
		(count << 18);
	writel(delay1, host->regs + SD_EMMC_DELAY1);
	writel(delay2, host->regs + SD_EMMC_DELAY2);
	pr_info("[%s], delay1: 0x%x, delay2: 0x%x\n",
		__func__, readl(host->regs + SD_EMMC_DELAY1),
		readl(host->regs + SD_EMMC_DELAY2));
}

static u32 emmc_cmd_sd_clk_tuning(struct mmc_host *mmc,
		int send_status, char mode, u32 *pcmd_size)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay2 = readl(host->regs + SD_EMMC_DELAY2);
	u32 sd_clk = readl(host->regs + SD_EMMC_CLOCK);
	u32 cmd_delay = 0;
	u32 i, j, err;
	int repeat_times = 100;
	char str[64] = {0};
	long long before_time;
	long long after_time;

	delay2 &= ~DELAY2_CMD_MASK;
	sd_clk &= ~CLK_V3_RX_DELAY_MASK;
	writel(delay2, host->regs + SD_EMMC_DELAY2);
	writel(sd_clk, host->regs + SD_EMMC_CLOCK);
	host->is_tuning = 1;
	before_time = sched_clock();
	for (i = 0; i < 64; i++) {
		if (mode == EMMC_CMD_LINE_DELAY_MODE) {
			delay2 &= ~DELAY2_CMD_MASK;
			delay2 |= FIELD_PREP(DELAY2_CMD_MASK, i);
			writel(delay2, host->regs + SD_EMMC_DELAY2);
		}
		if (mode == EMMC_CMD_RX_DELAY_MODE) {
			sd_clk &= ~CLK_V3_RX_DELAY_MASK;
			sd_clk |= FIELD_PREP(CLK_V3_RX_DELAY_MASK, i);
			writel(sd_clk, host->regs + SD_EMMC_CLOCK);
		}
		for (j = 0; j < repeat_times; j++) {
			if (send_status)
				err = emmc_send_cmd(mmc, MMC_SEND_STATUS,
						    1 << 16,
						    MMC_RSP_R1 | MMC_CMD_AC);
			else
				err = emmc_test_bus(mmc);
			if (!err)
				str[i]++;
			else
				break;
		}
		pr_debug("delay2:0x%x, sd_clk:0x%x\n",
			readl(host->regs + SD_EMMC_DELAY2),
			readl(host->regs + SD_EMMC_CLOCK));
	}
	after_time = sched_clock();
	host->is_tuning = 0;
	pr_debug("scan time distance: %llu ns\n", after_time - before_time);
	cmd_delay = emmc_search_cmd_delay(str, repeat_times, pcmd_size);
	emmc_show_cmd_window(str, repeat_times);
	pr_info("[%s] delay2:0x%x, sd_clk:0x%x\n", __func__,
			readl(host->regs + SD_EMMC_DELAY2),
			readl(host->regs + SD_EMMC_CLOCK));
	return cmd_delay;
}

static int set_emmc_cmd_sd_clk_delay(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 delay2 = readl(host->regs + SD_EMMC_DELAY2);
	u32 sd_clk = readl(host->regs + SD_EMMC_CLOCK);
	u32 cmd_delay1, cmd_delay2;
	u32 cmd_size1 = 0, cmd_size2 = 0, ret = 0;

	cmd_delay1 = emmc_cmd_sd_clk_tuning(mmc, 1,
		       EMMC_CMD_LINE_DELAY_MODE, &cmd_size1);
	cmd_delay2 = emmc_cmd_sd_clk_tuning(mmc, 1,
			EMMC_CMD_RX_DELAY_MODE, &cmd_size2);
	if (cmd_size1 >= cmd_size2 && cmd_size1 != EMMC_CMD_WIN_FULL_SIZE) {
		delay2 &= ~DELAY2_CMD_MASK;
		delay2 |= FIELD_PREP(DELAY2_CMD_MASK, cmd_delay1);
		writel(delay2, host->regs + SD_EMMC_DELAY2);
		ret = cmd_size1;
	} else {
		sd_clk &= ~CLK_V3_RX_DELAY_MASK;
		sd_clk |= FIELD_PREP(CLK_V3_RX_DELAY_MASK, cmd_delay2);
		writel(sd_clk, host->regs + SD_EMMC_CLOCK);
		ret = cmd_size2;
	}
	pr_info("[%s] delay2:0x%x, sd_clk:0x%x, intf3:0x%x\n", __func__,
			readl(host->regs + SD_EMMC_DELAY2),
			readl(host->regs + SD_EMMC_CLOCK),
			readl(host->regs + SD_EMMC_INTF3));
	return ret;
}

static void set_emmc_cmd_sample(struct mmc_host *mmc, char mode)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 intf3 = readl(host->regs + SD_EMMC_INTF3);

	if (mode == EMMC_CMD_FALLING_SML)
		intf3 |= CFG_RX_PN;
	if (mode == EMMC_CMD_RISING_SML)
		intf3 &= ~CFG_RX_PN;
	if (mode == EMMC_CMD_CORE_CLK_SML)
		intf3 &= ~CFG_RX_SEL;
	if (mode == EMMC_CMD_SD_CLK_SML)
		intf3 |= CFG_RX_SEL;
	writel(intf3, host->regs + SD_EMMC_INTF3);
}

static void aml_emmc_hs400_v5(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 cmd_size = 0;

	mmc->retune_crc_disable = true;
	set_emmc_nwr_clks(mmc);
	if (!host->sd_clk_sample) {
		cmd_size = set_emmc_cmd_delay(mmc, 1);
		if (cmd_size == EMMC_CMD_WIN_FULL_SIZE) {
			set_emmc_cmd_sample(mmc, EMMC_CMD_FALLING_SML);
			cmd_size = set_emmc_cmd_delay(mmc, 1);
			pr_debug(">>>cmd_size:%u\n", cmd_size);
		}
	} else {
		set_emmc_cmd_sample(mmc, EMMC_CMD_SD_CLK_SML);
		cmd_size = set_emmc_cmd_sd_clk_delay(mmc);
		pr_debug(">>>cmd_size:%u\n", cmd_size);
	}
	emmc_ds_manual_sht(mmc);
}

static void aml_get_ctrl_ver(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);

	if (host->ignore_desc_busy)
		aml_emmc_hs400_v5(mmc);
	else
		aml_emmc_hs400_tl1(mmc);
}

static void aml_post_hs400_timming(struct mmc_host *mmc)
{
	aml_sd_emmc_clktest(mmc);

	if (aml_para_is_exist(mmc)) {
		aml_set_tuning_para(mmc);
		return;
	}
	aml_get_ctrl_ver(mmc);

	aml_save_tuning_para(mmc);
}

/* disable enhanced_strobe mode when initialization
 * enable enhanced_strobe mode and tuning intf3 when mmc_select_hs400es
 */
static void meson_mmc_enhance_strobe(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct meson_host *host = mmc_priv(mmc);

	if (!host->mmc->ios.enhanced_strobe)
		return;
	mmc->retune_crc_disable = true;
	aml_sd_emmc_clktest(mmc);
	emmc_ds_manual_sht(mmc);
	dev_notice(host->dev, "[%s] done.\n", __func__);
}

static void aml_sd_emmc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 vclkc = 0, virqc = 0;

	host->sdio_irqen = enable;

	virqc = readl(host->regs + SD_EMMC_IRQ_EN);
	virqc &= ~IRQ_SDIO;
	if (enable)
		virqc |= IRQ_SDIO;
	writel(virqc, host->regs + SD_EMMC_IRQ_EN);

	if (!host->irq_sdio_sleep) {
		vclkc = readl(host->regs + SD_EMMC_CLOCK);
		vclkc |= CFG_IRQ_SDIO_SLEEP;
		vclkc &= ~CFG_IRQ_SDIO_SLEEP_DS;
		writel(vclkc, host->regs + SD_EMMC_CLOCK);
		host->irq_sdio_sleep = 1;
	}

	/* check if irq already occurred */
	aml_sd_emmc_check_sdio_irq(host);
}

static void meson_mmc_read_resp(struct mmc_host *mmc, struct mmc_command *cmd)
{
	struct meson_host *host = mmc_priv(mmc);

	if (cmd->flags & MMC_RSP_136) {
		cmd->resp[0] = readl(host->regs + SD_EMMC_CMD_RSP3);
		cmd->resp[1] = readl(host->regs + SD_EMMC_CMD_RSP2);
		cmd->resp[2] = readl(host->regs + SD_EMMC_CMD_RSP1);
		cmd->resp[3] = readl(host->regs + SD_EMMC_CMD_RSP);
	} else if (cmd->flags & MMC_RSP_PRESENT) {
		cmd->resp[0] = readl(host->regs + SD_EMMC_CMD_RSP);
	}
}

void aml_host_bus_fsm_show(struct mmc_host *mmc, int status)
{
	int fsm_val = 0;

	fsm_val = (status & BUS_FSM_MASK) >> __ffs(BUS_FSM_MASK);
	switch (fsm_val) {
	case BUS_FSM_IDLE:
		pr_err("%s: err: idle, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_SND_CMD:
		pr_err("%s: err: send cmd, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_CMD_DONE:
		pr_err("%s: err: wait for cmd done, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_RESP_START:
		pr_err("%s: err: resp start, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_RESP_DONE:
		pr_err("%s: err: wait for resp done, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_DATA_START:
		pr_err("%s: err: data start, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_DATA_DONE:
		pr_err("%s: err: wait for data done, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_DESC_WRITE_BACK:
		pr_err("%s: err: wait for desc write back, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	case BUS_FSM_IRQ_SERVICE:
		pr_err("%s: err: wait for irq service, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	default:
		pr_err("%s: err: unknown err, bus_fsm:0x%x\n",
				mmc_hostname(mmc), fsm_val);
		break;
	}
}

static irqreturn_t meson_mmc_irq(int irq, void *dev_id)
{
	struct meson_host *host = dev_id;
	struct mmc_command *cmd;
	struct mmc_data *data;
	u32 irq_en, status, raw_status;
	irqreturn_t ret = IRQ_NONE;

	if (WARN_ON(!host))
		return IRQ_NONE;

	if (!host->cmd && (aml_card_type_mmc(host) ||
			aml_card_type_non_sdio(host))) {
		pr_debug("ignore irq.[%s]status:0x%x\n",
			__func__, readl(host->regs + SD_EMMC_STATUS));
#if IS_ENABLED(CONFIG_AMLOGIC_MMC_CQHCI)
		if (host->mmc->cqe_on) {
			pr_debug("[%s]Enter cqe irq\n", __func__);
			aml_cqhci_irq(host);
		}
		/*
		 * clear invalid irq,When emmc reports an error,
		 * error_bit and end_of_chain will not be level triggered at the same
		 * time and many invalid interrupts will be generated
		 */
		writel(0x7fff, host->regs + SD_EMMC_STATUS);
#endif
		return IRQ_HANDLED;
	}

	irq_en = readl(host->regs + SD_EMMC_IRQ_EN);
	raw_status = readl(host->regs + SD_EMMC_STATUS);
	status = raw_status & irq_en;

	if (status & IRQ_SDIO) {
		if (host->mmc->sdio_irq_thread &&
		    (!atomic_read(&host->mmc->sdio_irq_thread_abort))) {
			mmc_signal_sdio_irq(host->mmc);
			if (!(status & 0x3fff))
				return IRQ_HANDLED;
		}
	} else if (!(status & 0x3fff)) {
		return IRQ_HANDLED;
	}

	cmd = host->cmd;
	data = cmd->data;
	if (WARN_ON(!host->cmd)) {
		dev_err(host->dev, "host->cmd is NULL.\n");
		return IRQ_HANDLED;
	}

	cmd->error = 0;
	if (status & IRQ_CRC_ERR) {
		if (!host->is_tuning)
			dev_err(host->dev, "%d [0x%x], CRC[0x%04x]\n",
				cmd->opcode, cmd->arg, status);
		if (host->debug_flag && !host->is_tuning) {
			dev_notice(host->dev, "clktree : 0x%x,host_clock: 0x%x\n",
				   readl(host->clk_tree_base),
				   readl(host->regs));
			dev_notice(host->dev, "adjust: 0x%x,cfg: 0x%x,intf3: 0x%x\n",
				   readl(host->regs + SD_EMMC_V3_ADJUST),
				   readl(host->regs + SD_EMMC_CFG),
				   readl(host->regs + SD_EMMC_INTF3));
			dev_notice(host->dev, "irq_en: 0x%x\n",
				   readl(host->regs + 0x4c));
			dev_notice(host->dev, "delay1: 0x%x,delay2: 0x%x\n",
				   readl(host->regs + SD_EMMC_DELAY1),
				   readl(host->regs + SD_EMMC_DELAY2));
			dev_notice(host->dev, "pinmux: 0x%x\n",
				   readl(host->pin_mux_base));
		}
		cmd->error = -EILSEQ;
		ret = IRQ_WAKE_THREAD;
		goto out;
	}

	if (status & IRQ_TIMEOUTS) {
		if (!host->is_tuning && !(cmd->arg == 0xc00 || cmd->arg == 0x80000c08))
			dev_err(host->dev, "%d [0x%x], TIMEOUT[0x%04x]\n",
				cmd->opcode, cmd->arg, status);
		if (host->debug_flag && !host->is_tuning) {
			dev_notice(host->dev, "clktree : 0x%x,host_clock: 0x%x\n",
				   readl(host->clk_tree_base),
				   readl(host->regs));
			dev_notice(host->dev, "adjust: 0x%x,cfg: 0x%x,intf3: 0x%x\n",
				   readl(host->regs + SD_EMMC_V3_ADJUST),
				   readl(host->regs + SD_EMMC_CFG),
				   readl(host->regs + SD_EMMC_INTF3));
			dev_notice(host->dev, "delay1: 0x%x,delay2: 0x%x\n",
				   readl(host->regs + SD_EMMC_DELAY1),
				   readl(host->regs + SD_EMMC_DELAY2));
			dev_notice(host->dev, "pinmux: 0x%x\n",
				   readl(host->pin_mux_base));
			dev_notice(host->dev, "irq_en: 0x%x\n",
				   readl(host->regs + 0x4c));
		}
		cmd->error = -ETIMEDOUT;
		ret = IRQ_WAKE_THREAD;
		goto out;
	}

	if (status & (IRQ_CRC_ERR | IRQ_TIMEOUTS))
		aml_host_bus_fsm_show(host->mmc, status);

	meson_mmc_read_resp(host->mmc, cmd);

	if (status & IRQ_SDIO) {
		dev_dbg(host->dev, "IRQ: SDIO TODO.\n");
		ret = IRQ_HANDLED;
	}

	if (status & (IRQ_END_OF_CHAIN | IRQ_RESP_STATUS)) {
		if (data && !cmd->error)
			data->bytes_xfered = data->blksz * data->blocks;
		if (meson_mmc_bounce_buf_read(data))
			ret = IRQ_WAKE_THREAD;
		else
			ret = IRQ_HANDLED;
	}

out:
	/* ack all raised interrupts */
	writel(0x7fff, host->regs + SD_EMMC_STATUS);
	if (cmd->error) {
		/* Stop desc in case of errors */
		u32 start = readl(host->regs + SD_EMMC_START);

		if (!host->ignore_desc_busy && (start & START_DESC_BUSY)) {
			start &= ~START_DESC_BUSY;
			writel(start, host->regs + SD_EMMC_START);
		}
	}

	if (ret == IRQ_HANDLED) {
		meson_mmc_read_resp(host->mmc, cmd);
		if (cmd->error && !host->is_tuning)
			pr_err("cmd = %d, arg = 0x%x, dev_status = 0x%x\n",
					cmd->opcode, cmd->arg, cmd->resp[0]);
		meson_mmc_request_done(host->mmc, cmd->mrq);
	} else if (ret == IRQ_NONE) {
		dev_warn(host->dev,
				"Unexpected IRQ! status=0x%08x, irq_en=0x%08x\n",
				raw_status, irq_en);
	}

	return ret;
}

static int meson_mmc_wait_desc_stop(struct meson_host *host)
{
	u32 status;

	/*
	 * It may sometimes take a while for it to actually halt. Here, we
	 * are giving it 5ms to comply
	 *
	 * If we don't confirm the descriptor is stopped, it might raise new
	 * IRQs after we have called mmc_request_done() which is bad.
	 */

	return readl_poll_timeout(host->regs + SD_EMMC_STATUS, status,
				  !(status & (STATUS_BUSY | STATUS_DESC_BUSY)),
				  100, 10000);
}

static irqreturn_t meson_mmc_irq_thread(int irq, void *dev_id)
{
	struct meson_host *host = dev_id;
	struct mmc_command *cmd = host->cmd;
	struct mmc_data *data;
	unsigned int xfer_bytes;

	if (WARN_ON(!cmd))
		return IRQ_NONE;

	if (cmd->error) {
		meson_mmc_wait_desc_stop(host);
		meson_mmc_request_done(host->mmc, cmd->mrq);

		return IRQ_HANDLED;
	}

	data = cmd->data;
	if (meson_mmc_bounce_buf_read(data)) {
		xfer_bytes = data->blksz * data->blocks;
		WARN_ON(xfer_bytes > host->bounce_buf_size);
		sg_copy_from_buffer(data->sg, data->sg_len,
				    host->bounce_buf, xfer_bytes);
	}

	meson_mmc_read_resp(host->mmc, cmd);
	meson_mmc_request_done(host->mmc, cmd->mrq);

	return IRQ_HANDLED;
}

/*
 * NOTE: we only need this until the GPIO/pinctrl driver can handle
 * interrupts.  For now, the MMC core will use this for polling.
 */
static int meson_mmc_get_cd(struct mmc_host *mmc)
{
	int status = mmc_gpio_get_cd(mmc);

	if (status == -EINVAL)
		return 1; /* assume present */

	return !status;
}

static void meson_mmc_cfg_init(struct meson_host *host)
{
	u32 cfg = 0;

	cfg |= FIELD_PREP(CFG_RESP_TIMEOUT_MASK,
			  ilog2(SD_EMMC_CFG_RESP_TIMEOUT));
	cfg |= FIELD_PREP(CFG_RC_CC_MASK, ilog2(SD_EMMC_CFG_CMD_GAP));
	cfg |= FIELD_PREP(CFG_BLK_LEN_MASK, ilog2(SD_EMMC_CFG_BLK_SIZE));

	/* abort chain on R/W errors */

	writel(cfg, host->regs + SD_EMMC_CFG);

	/* The resp returned by cmd19 and cmd52/3 can't use the same mask */
	writel(0x0, host->regs + 0x150);

	host->timing = -1;
}

static int meson_mmc_card_busy(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 regval, val;

	regval = readl(host->regs + SD_EMMC_STATUS);
	if (!aml_card_type_mmc(host) && host->sd_sdio_switch_volat_done) {
		val = readl(host->regs + SD_EMMC_CFG);
		val |= CFG_AUTO_CLK;
		writel(val, host->regs + SD_EMMC_CFG);
		host->sd_sdio_switch_volat_done = 0;
	}

	/*
	 * eMMC: We are only interrested in line 0, so mask the other ones
	 * SD: We are only interrested in lines 0 to 3, so mask the other ones
	 */
	if (aml_card_type_mmc(host))
		return !(FIELD_GET(STATUS_DATI, regval) & 0x1);
	return !(FIELD_GET(STATUS_DATI, regval) & 0xf);
}

static int meson_mmc_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct meson_host *host = mmc_priv(mmc);
	int err;

	if (IS_ERR(mmc->supply.vqmmc) && IS_ERR(mmc->supply.vmmc))
		return 0;
	/* vqmmc regulator is available */
	if (!IS_ERR(mmc->supply.vqmmc)) {
		/*
		 * The usual amlogic setup uses a GPIO to switch from one
		 * regulator to the other. While the voltage ramp up is
		 * pretty fast, care must be taken when switching from 3.3v
		 * to 1.8v. Please make sure the regulator framework is aware
		 * of your own regulator constraints
		 */
		err = mmc_regulator_set_vqmmc(mmc, ios);

		if (!err && ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180)
			host->sd_sdio_switch_volat_done = 1;

		return err;
	}

	/* no vqmmc regulator, assume fixed regulator at 3/3.3V */
	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330)
		return 0;

	return -EINVAL;
}

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
int __maybe_unused aml_emmc_hs200_tl1(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 vclkc = readl(host->regs + SD_EMMC_CLOCK);
	struct para_e *para = &host->sd_mmc;
	u32 clk_bak = 0;
	u32 delay2 = 0, count = 0;
	int i, j, txdelay, err = 0;
	int retry_times = 0;

	aml_sd_emmc_clktest(mmc);
	clk_bak = vclkc;
	vclkc &= ~CLK_TX_PHASE_MASK;
	vclkc &= ~CLK_CORE_PHASE_MASK;
	vclkc &= ~CLK_V3_TX_DELAY_MASK;
	vclkc |= para->hs4.tx_phase << __ffs(CLK_TX_PHASE_MASK);
	vclkc |= para->hs4.core_phase << __ffs(CLK_CORE_PHASE_MASK);
	vclkc |= para->hs4.tx_delay << __ffs(CLK_V3_TX_DELAY_MASK);
	txdelay = para->hs4.tx_delay;

	writel(vclkc, host->regs + SD_EMMC_CLOCK);
	pr_info("[%s][%d] clk config:0x%x\n",
		__func__, __LINE__, readl(host->regs + SD_EMMC_CLOCK));

	for (i = 0; i < 63; i++) {
		retry_times = 0;
		delay2 += (1 << 24);
		writel(delay2, host->regs + SD_EMMC_DELAY2);
retry:
		err = emmc_eyetest_log(mmc, 9);
		if (err)
			continue;

		count = __ffs(host->align[9]);
		if ((count >= 14 && count <= 20) ||
		    (count >= 48 && count <= 54)) {
			if (retry_times != 3) {
				retry_times++;
				goto retry;
			} else {
				break;
			}
		}
	}
	if (i == 63) {
		for (j = 0; j < 6; j++) {
			txdelay++;
			vclkc &= ~CLK_V3_TX_DELAY_MASK;
			vclkc |= para->hs4.tx_delay <<
				__ffs(CLK_V3_TX_DELAY_MASK);
			pr_info("modify tx delay to %d\n", txdelay);
			writel(vclkc, host->regs + SD_EMMC_CLOCK);
			err = emmc_eyetest_log(mmc, 9);
			if (err)
				continue;
			count = __ffs(host->align[9]);
			if ((count >= 14 && count <= 20) ||
			    (count >= 48 && count <= 54))
				break;
		}
	}

	host->cmd_c = (delay2 >> 24);
	pr_info("cmd->u64eyet:0x%016llx\n", host->align[9]);
	writel(0, host->regs + SD_EMMC_DELAY2);
	writel(clk_bak, host->regs + SD_EMMC_CLOCK);

	delay2 = 0;
	for (i = 0; i < 63; i++) {
		retry_times = 0;
		delay2 += (1 << 24);
		writel(delay2, host->regs + SD_EMMC_DELAY2);
retry1:
		err = emmc_eyetest_log(mmc, 9);
		if (err)
			continue;

		count = __ffs(host->align[9]);
		if (count >= 8 && count <= 56) {
			if (retry_times != 3) {
				retry_times++;
				goto retry1;
			} else {
				break;
			}
		}
	}

	pr_info("[%s][%d] clk config:0x%x\n",
		__func__, __LINE__, readl(host->regs + SD_EMMC_CLOCK));
	return 0;
}
#endif

static int intf3_scan(struct mmc_host *mmc, u32 opcode)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 intf3;
	u32 i, j, err;
	char rx_r[64] = {0}, rx_f[64] = {0};
	u32 vclk = readl(host->regs + SD_EMMC_CLOCK);
	int best_s1 = -1, best_sz1 = 0;
	int best_s2 = -1, best_sz2 = 0;

	intf3 = readl(host->regs + SD_EMMC_INTF3);
	intf3 |= SD_INTF3;
	intf3 &= ~EYETEST_SEL;

	for (i = 0; i < 2; i++) {
		if (i)
			intf3 |= RESP_SEL;
		else
			intf3 &= ~RESP_SEL;
		writel(intf3, (host->regs + SD_EMMC_INTF3));
		for (j = 0; j < 64; j++) {
			vclk &= ~CLK_V3_RX_DELAY_MASK;
			vclk |= j << __ffs(CLK_V3_RX_DELAY_MASK);
			writel(vclk, host->regs + SD_EMMC_CLOCK);
			if (aml_card_type_mmc(host)) {
				err = emmc_test_bus(mmc);
				if (!err) {
					if (i)
						rx_f[j]++;
					else
						rx_r[j]++;
				}
			} else {
				err = meson_mmc_tuning_transfer(mmc, opcode);
				if (err == TUNING_NUM_PER_POINT) {
					if (i)
						rx_f[j]++;
					else
						rx_r[j]++;
				}
			}
		}
	}
	find_best_win(mmc, rx_r, 64, &best_s1, &best_sz1, false);
	find_best_win(mmc, rx_f, 64, &best_s2, &best_sz2, false);
	if (host->debug_flag) {
		emmc_show_cmd_window(rx_r, 1);
		emmc_show_cmd_window(rx_f, 1);
	}
	pr_info("r: b_s = %x, b_sz = %x, f: b_s = %x, b_sz = %x\n",
		best_s1, best_sz1, best_s2, best_sz2);

	if (!best_sz1 && !best_sz2)
		return -1;

	vclk &= ~CLK_V3_RX_DELAY_MASK;
	if (best_sz1 >= best_sz2) {
		intf3 &= ~RESP_SEL;
		vclk |= (best_s1 + best_sz1 / 2) << __ffs(CLK_V3_RX_DELAY_MASK);
	} else {
		intf3 |= RESP_SEL;
		vclk |= (best_s2 + best_sz2 / 2) << __ffs(CLK_V3_RX_DELAY_MASK);
	}
	pr_info("the final result: sel = %lx, rx = %lx\n",
		(intf3 & SD_INTF3) >> __ffs(SD_INTF3),
		(vclk & CLK_V3_RX_DELAY_MASK) >> __ffs(CLK_V3_RX_DELAY_MASK));
	writel(vclk, host->regs + SD_EMMC_CLOCK);
	writel(intf3, host->regs + SD_EMMC_INTF3);

	return 0;
}

static int mmc_intf3_win_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 vclk, ret = -1;

	vclk = readl(host->regs + SD_EMMC_CLOCK);

	if ((vclk & CLK_DIV_MASK) > 8) {
		pr_err("clk div is too big.\n");
		return -1;
	}

	vclk &= ~CLK_V3_RX_DELAY_MASK;
	writel(vclk, host->regs + SD_EMMC_CLOCK);
	writel(0, host->regs + SD_EMMC_DELAY1);
	writel(0, host->regs + SD_EMMC_DELAY2);
	writel(0, host->regs + SD_EMMC_V3_ADJUST);
	writel(0, host->regs + SD_EMMC_INTF3);

	ret = intf3_scan(mmc, opcode);
	if (ret)
		pr_err("scan intf3 rx window fail.\n");

	return ret;
}

static int meson_mmc_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct meson_host *host = mmc_priv(mmc);
	int err = 0;

	host->is_tuning = 1;
	if (host->use_intf3_tuning)
		err = mmc_intf3_win_tuning(mmc, opcode);
	else
		err = meson_mmc_fixadj_tuning(mmc, opcode);

	host->is_tuning = 0;

	return err;
}


static void sdio_rescan(struct mmc_host *mmc)
{
	int ret;

	mmc->rescan_entered = 0;
	//mmc->host_rescan_disable = false;
	mmc_detect_change(mmc, 0);
	// start the delayed_work
	ret = flush_work(&mmc->detect.work);
	// wait for the delayed_work to finish
	if (!ret)
		pr_info("Error: delayed_work mmc_rescan() already idle!\n");
}

static void sdio_reset_comm(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	int i = 0, err = 0;

	while (i < SDIO_MAX_FUNCS && !card->sdio_func[i])
		i++;
	if (WARN_ON(i == SDIO_MAX_FUNCS))
		return;
	sdio_claim_host(card->sdio_func[i]);
	err = mmc_sw_reset(host);
	sdio_release_host(card->sdio_func[i]);
	if (err)
		pr_info("%s Failed, error = %d\n", __func__, err);
	return;
}

void sdio_reinit(void)
{
	if (sdio_host) {
		struct mmc_ios *ios = &sdio_host->ios;

		if (sdio_host->card) {
			if (ios)
				ios->timing = 0;
			sdio_reset_comm(sdio_host->card);
		}
		else
			sdio_rescan(sdio_host);
	} else {
		pr_info("Error: sdio_host is NULL\n");
	}

	pr_debug("[%s] finish\n", __func__);
}
EXPORT_SYMBOL(sdio_reinit);

void sdio_clk_always_on(bool clk_aws_on)
{
	struct meson_host *host = NULL;
	u32 conf = 0;

	if (sdio_host) {
		host = mmc_priv(sdio_host);
		conf = readl(host->regs + SD_EMMC_CFG);
		if (clk_aws_on)
			conf &= ~CFG_AUTO_CLK;
		else
			conf |= CFG_AUTO_CLK;
		writel(conf, host->regs + SD_EMMC_CFG);
		pr_info("[%s] clk:0x%x, cfg:0x%x\n",
				__func__, readl(host->regs + SD_EMMC_CLOCK),
				readl(host->regs + SD_EMMC_CFG));
	} else {
		pr_info("Error: sdio_host is NULL\n");
	}

	pr_info("[%s] finish\n", __func__);
}
EXPORT_SYMBOL(sdio_clk_always_on);

void sdio_set_max_regs(unsigned int size)
{
	if (sdio_host) {
		sdio_host->max_req_size = size;
		sdio_host->max_seg_size = sdio_host->max_req_size;
	} else {
		pr_info("Error: sdio_host is NULL\n");
	}

	pr_info("[%s] finish\n", __func__);
}
EXPORT_SYMBOL(sdio_set_max_regs);

/*this function tells wifi is using sd(sdiob) or sdio(sdioa)*/
const char *get_wifi_inf(void)
{
	if (sdio_host)
		return mmc_hostname(sdio_host);
	else
		return "sdio";
}
EXPORT_SYMBOL(get_wifi_inf);

int sdio_get_vendor(void)
{
	int vendor = 0;

	if (sdio_host && sdio_host->card)
		vendor = sdio_host->card->cis.vendor;

	pr_info("sdio vendor is 0x%x\n", vendor);
	return vendor;
}
EXPORT_SYMBOL(sdio_get_vendor);

static struct pinctrl * __must_check aml_pinctrl_select(struct meson_host *host,
							const char *name)
{
	struct pinctrl *p = host->pinctrl;
	struct pinctrl_state *s;
	int ret = 1;

	if (!p) {
		dev_err(host->dev, "%s NULL POINT!!\n", __func__);
		return ERR_PTR(ret);
	}

	s = pinctrl_lookup_state(p, name);
	if (IS_ERR(s)) {
		pr_err("lookup %s fail\n", name);
		devm_pinctrl_put(p);
		return ERR_CAST(s);
	}

	ret = pinctrl_select_state(p, s);
	if (ret < 0) {
		pr_err("select %s fail\n", name);
		devm_pinctrl_put(p);
		return ERR_PTR(ret);
	}
	return p;
}

static int aml_uart_switch(struct meson_host *host, bool on)
{
	struct pinctrl *pc;
	char *name[2] = {
		"sd_to_ao_uart_pins",
		"ao_to_sd_uart_pins",
	};

	pc = aml_pinctrl_select(host, name[on]);
	return on;
}

static int aml_is_sduart(struct meson_host *host)
{
	int in = 0, i;
	int high_cnt = 0, low_cnt = 0;
	u32 vstat = 0;

	if (host->is_uart)
		return 0;
	//if (!host->sd_uart_init) {
	//	aml_uart_switch(host, 0);
	//} else {
	//	in = (readl(host->pin_mux_base) & DATA3_PINMUX_MASK) >>
	//		__ffs(DATA3_PINMUX_MASK);
	//	if (in == 2)
	//		return 1;
	//	else
	//		return 0;
	//}
	for (i = 0; ; i++) {
		mdelay(1);
		vstat = readl(host->regs + SD_EMMC_STATUS) & 0xffffffff;
		if (vstat & 0x80000) {
			high_cnt++;
			low_cnt = 0;
		} else {
			low_cnt++;
			high_cnt = 0;
		}
		if (high_cnt > 100 || low_cnt > 100)
			break;
	}
	if (low_cnt > 100)
		in = 1;
	return in;
}

static int aml_is_card_insert(struct mmc_gpio *ctx)
{
	int ret = 0, in_count = 0, out_count = 0, i;

	if (ctx->cd_gpio) {
		for (i = 0; i < 200; i++) {
			ret = gpiod_get_value(ctx->cd_gpio);
			if (ret)
				out_count++;
			in_count++;
			if (out_count > 100 || in_count > 100)
				break;
		}
		if (out_count > 100)
			ret = 1;
		else if (in_count > 100)
			ret = 0;
	}
//        if (ctx->override_cd_active_level)
  //              ret = !ret; /* reverse, so ---- 0: no inserted  1: inserted */

	return ret;
}

int meson_mmc_cd_detect(struct mmc_host *mmc)
{
	int gpio_val, val, ret;
	struct meson_host *host = mmc_priv(mmc);
	struct mmc_gpio *ctx = mmc->slot.handler_priv;

	gpio_val = aml_is_card_insert(ctx);
	dev_dbg(host->dev, "card %s\n", gpio_val ? "OUT" : "IN");
	mmc->trigger_card_event = true;
	if (!gpio_val) {//card insert
		if (host->card_insert)
			return 0;
		host->card_insert = 1;
		val = aml_is_sduart(host);
		dev_notice(host->dev, " %s insert\n", val ? "UART" : "SDCARD");
		if (val) {//uart insert
			host->is_uart = 1;
			aml_uart_switch(host, 1);
			mmc->caps &= ~MMC_CAP_4_BIT_DATA;
			host->pins_default = pinctrl_lookup_state(host->pinctrl,
								  "sd_1bit_pins");
			if (IS_ERR(host->pins_default)) {
				ret = PTR_ERR(host->pins_default);
				return ret;
			}
		} else {//sdcard insert
			aml_uart_switch(host, 0);
			mmc->caps |= MMC_CAP_4_BIT_DATA;
			host->pins_default = pinctrl_lookup_state(host->pinctrl,
								  "sd_default");
		}
	} else { //card out
		if (!host->card_insert)
			return 0;
		if (host->is_uart) {
			host->is_uart = 0;
			devm_free_irq(mmc->parent, host->cd_irq, mmc);
		}
		host->card_insert = 0;
		aml_uart_switch(host, 0);
	}
	if (!host->is_uart)
		mmc_detect_change(mmc, msecs_to_jiffies(200));
	return 0;
}

static void scan_emmc_tx_win(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 vclk = readl(host->regs + SD_EMMC_CLOCK);
	u32 dly1 = readl(host->regs + SD_EMMC_DELAY1);
	u32 dly2 = readl(host->regs + SD_EMMC_DELAY2);
	u32 intf3 = readl(host->regs + SD_EMMC_INTF3);
	u32 clk_bak = vclk, dly1_bak = dly1, dly2_bak = dly2;
	u32 intf3_bak = intf3;
	u32 i, j, err;
	u8 tx_delay = (vclk & CLK_V3_TX_DELAY_MASK) >>
		__ffs(CLK_V3_TX_DELAY_MASK);
	int repeat_times = 100;
	char str[64] = {0};

	aml_sd_emmc_cali_v3(mmc, MMC_READ_MULTIPLE_BLOCK,
			    host->blk_test, 512, 40, MMC_RANDOM_NAME);
	host->is_tuning = 1;
	tx_delay = 0;
	vclk &= ~CLK_V3_TX_DELAY_MASK;
	vclk |= tx_delay << __ffs(CLK_V3_TX_DELAY_MASK);
	writel(vclk, host->regs + SD_EMMC_CLOCK);
	for (i = tx_delay; i < 64; i++) {
		writel(0, host->regs + SD_EMMC_DELAY1);
		writel(0, host->regs + SD_EMMC_DELAY2);
		writel(SD_INTF3, host->regs + SD_EMMC_INTF3);
		aml_get_ctrl_ver(mmc);
		for (j = 0; j < repeat_times; j++) {
			err = mmc_write_internal(mmc->card, MMC_RANDOM_OFFSET,
						 40, host->blk_test);
			if (!err)
				str[i]++;
			else
				break;
		}
		tx_delay += 1;
		vclk &= ~CLK_V3_TX_DELAY_MASK;
		vclk |= tx_delay << __ffs(CLK_V3_TX_DELAY_MASK);
		writel(vclk, host->regs + SD_EMMC_CLOCK);
		pr_debug("tx_delay: 0x%x, send cmd %d times success %d times, is ok\n",
			 tx_delay, repeat_times, str[i]);
	}
	host->is_tuning = 0;

	writel(clk_bak, host->regs + SD_EMMC_CLOCK);
	writel(dly1_bak, host->regs + SD_EMMC_DELAY1);
	writel(dly2_bak, host->regs + SD_EMMC_DELAY2);
	writel(intf3_bak, host->regs + SD_EMMC_INTF3);
	emmc_search_cmd_delay(str, repeat_times, NULL);
	pr_info(">>>>>>>>>>>>>>>>>>>>this is tx window>>>>>>>>>>>>>>>>>>>>>>\n");
	emmc_show_cmd_window(str, repeat_times);
}

void emmc_eyetestlog(struct mmc_host *mmc)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 dly, dly1_bak, dly2_bak;
	int i = 0;

	dly1_bak = readl(host->regs + SD_EMMC_DELAY1);
	dly2_bak = readl(host->regs + SD_EMMC_DELAY2);
	for (i = 0; i < 64; i++) {
		dly = (i << 0) | (i << 6) | (i << 12) | (i << 18) | (i << 24);
		writel(dly, host->regs + SD_EMMC_DELAY1);
		writel(dly, host->regs + SD_EMMC_DELAY2);
		update_all_line_eyetest(mmc);
	}
	writel(dly1_bak, host->regs + SD_EMMC_DELAY1);
	writel(dly2_bak, host->regs + SD_EMMC_DELAY2);
}

static const struct mmc_host_ops meson_mmc_ops = {
	.request	= meson_mmc_request,
	.set_ios	= meson_mmc_set_ios,
	.enable_sdio_irq = aml_sd_emmc_enable_sdio_irq,
	.get_cd         = meson_mmc_get_cd,
	.pre_req	= meson_mmc_pre_req,
	.post_req	= meson_mmc_post_req,
	.execute_tuning = meson_mmc_execute_tuning,
	.hs400_complete = aml_post_hs400_timming,
	.card_busy	= meson_mmc_card_busy,
	.start_signal_voltage_switch = meson_mmc_voltage_switch,
	.hs400_enhanced_strobe = meson_mmc_enhance_strobe,
//	.init_card      = sdio_get_card,
};

static int mmc_clktest_show(struct seq_file *s, void *data)
{
	struct mmc_host	*host = s->private;
	int count = 0;

	mmc_claim_host(host);
	count = emmc_clktest(host);
	mmc_release_host(host);

	seq_puts(s, "mmc clktest done\n");
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mmc_clktest);

static int mmc_eyetest_show(struct seq_file *s, void *data)
{
	struct mmc_host	*host = s->private;

	mmc_claim_host(host);
	update_all_line_eyetest(host);
	mmc_release_host(host);

	seq_puts(s, "mmc eyetest done\n");
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mmc_eyetest);

static int mmc_tx_win_show(struct seq_file *s, void *data)
{
	struct mmc_host	*host = s->private;

	mmc_claim_host(host);
	scan_emmc_tx_win(host);
	mmc_release_host(host);

	seq_puts(s, "mmc tx win done\n");
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mmc_tx_win);

static int mmc_cmd_rx_win_show(struct seq_file *s, void *data)
{
	struct mmc_host	*host = s->private;

	mmc_claim_host(host);
	scan_emmc_cmd_win(host, 0, NULL);
	mmc_release_host(host);

	seq_puts(s, "mmc cmd rx win done\n");
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mmc_cmd_rx_win);

static int mmc_eyetestlog_show(struct seq_file *s, void *data)
{
	struct mmc_host	*host = s->private;

	mmc_claim_host(host);
	emmc_eyetestlog(host);
	mmc_release_host(host);

	seq_puts(s, "mmc eyetestlog done\n");
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mmc_eyetestlog);

/* To provide Total Write Size which is cumulative written data
 * size from Host to e-MMC. The unit is MByte.
 *
 * The Total Write Size counts total blocks written by Write Multiple
 * Block(CMD25). The other CMDs such as CMD24 or CMDQ are not counted.
 */
static int write_size_get(void *data, u64 *val)
{
	struct mmc_host *mmc = data;
	unsigned char *buf = NULL;
	u32 err;

	buf = kmalloc(512, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mmc_claim_host(mmc);
	err = single_read_scan(mmc, MMC_GEN_CMD, buf, 512, 1, 0x42535491);
	if (err || buf[0] != 0x91 || buf[1] != 0x50) {
		pr_err("%s: read error", __func__);
		return -EINVAL;
	}
	mmc_release_host(mmc);

	*val = buf[19] << 0
		| (u64)buf[18] << 8
		| (u64)buf[17] << 16
		| (u64)buf[16] << 24;
	kfree(buf);
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_write_size, write_size_get,
			 NULL, "%llu\n");

/* Check the average erase count including average value of erase
 * count in TLC area and average value of erase count in pseudo
 * SLC area.
 */
static int erase_count_show(struct seq_file *s, void *data)
{
	struct mmc_host	*mmc = s->private;
	unsigned char *buf = NULL;
	u32 err, tlc, slc;

	buf = kmalloc(512, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mmc_claim_host(mmc);
	err = single_read_scan(mmc, MMC_GEN_CMD, buf, 512, 1, 0x42535493);
	if (err || buf[0] != 0x93 || buf[1] != 0x50) {
		pr_err("%s: read error", __func__);
		return -EINVAL;
	}

	mmc_release_host(mmc);
	tlc = buf[27] << 0
		| (u32)buf[26] << 8
		| (u32)buf[25] << 16
		| (u32)buf[24] << 24;

	slc = buf[43] << 0
		| (u32)buf[42] << 8
		| (u32)buf[41] << 16
		| (u32)buf[40] << 24;

	seq_printf(s, "tlc: 0x%x slc: 0x%x", tlc, slc);
	seq_putc(s, '\n');
	kfree(buf);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(erase_count);

void add_dtbkey(struct work_struct *work)
{
	int ret;
	struct meson_host *host =
		container_of(work, struct meson_host, dtbkey.work);
	struct mmc_host *mmc = mmc_from_priv(host);

	if (mmc->card) {
		emmc_key_init(mmc->card, &ret);
		if (ret)
			pr_err("%s:%d,emmc_key_init fail\n", __func__, __LINE__);

		amlmmc_dtb_init(mmc->card, &ret);
		if (ret)
			pr_err("%s:%d,amlmmc_dtb_init fail\n", __func__, __LINE__);
	} else {
		schedule_delayed_work(&host->dtbkey, 50);
	}
}

static int meson_mmc_probe(struct platform_device *pdev)
{
	struct meson_host *host;
	struct mmc_host *mmc;
	int ret;
	u32 val, cali_blk_cnt;

	mmc = mmc_alloc_host(sizeof(struct meson_host), &pdev->dev);
	if (!mmc)
		return -ENOMEM;

	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, host);

	/* The G12A SDIO Controller needs an SRAM bounce buffer */
	host->dram_access_quirk = device_property_read_bool(&pdev->dev,
					"amlogic,dram-access-quirk");

	/* Get regulators and the supported OCR mask */
	host->vqmmc_enabled = false;
	ret = mmc_regulator_get_supply(mmc);
	if (ret) {
		dev_warn(&pdev->dev, "power regulator get failed!\n");
		goto free_host;
	}

	ret = mmc_of_parse(mmc);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_warn(&pdev->dev, "error parsing DT: %d\n", ret);
		goto free_host;
	}
	amlogic_of_parse(mmc);
	mmc->hold_retune = 1;

	if (aml_card_type_non_sdio(host)) {
		if (!IS_ERR(mmc->supply.vqmmc))
			regulator_set_voltage_triplet(mmc->supply.vqmmc, 1700000, 1800000, 1950000);
	}

	/* caps2 qurik for eMMC */
	if (aml_card_type_mmc(host) && caps2_quirks &&
		!strncmp(caps2_quirks,
		"mmc-hs400", strlen(caps2_quirks))) {
		dev_warn(&pdev->dev, "Force HS400\n");
		mmc->caps2 |= MMC_CAP2_HS400_1_8V | MMC_CAP2_HS200_1_8V_SDR;
	}

	host->data = (struct meson_mmc_data *)
		of_device_get_match_data(&pdev->dev);
	if (!host->data) {
		ret = -EINVAL;
		goto free_host;
	}

	ret = device_reset_optional(&pdev->dev);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "device reset failed: %d\n", ret);

		return ret;
	}

	host->res[0] = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->regs = devm_ioremap_resource(&pdev->dev, host->res[0]);
	if (IS_ERR(host->regs)) {
		ret = PTR_ERR(host->regs);
		goto free_host;
	}

	host->res[1] = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	host->clk_tree_base = ioremap(host->res[1]->start, resource_size(host->res[1]));
	if (IS_ERR(host->clk_tree_base)) {
		ret = PTR_ERR(host->clk_tree_base);
		goto free_host;
	}

	val = readl(host->clk_tree_base);
	if (aml_card_type_non_sdio(host))
		val &= EMMC_SDIO_CLOCK_FELD;
	else
		val &= ~EMMC_SDIO_CLOCK_FELD;
	writel(val, host->clk_tree_base);

	host->res[2] = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	host->pin_mux_base = ioremap(host->res[2]->start, resource_size(host->res[2]));
	if (IS_ERR(host->pin_mux_base)) {
		ret = PTR_ERR(host->pin_mux_base);
		goto free_host;
	}

	host->irq = platform_get_irq(pdev, 0);
	if (host->irq <= 0) {
		ret = -EINVAL;
		goto free_host;
	}

	host->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(host->pinctrl)) {
		ret = PTR_ERR(host->pinctrl);
		goto free_host;
	}

	if (aml_card_type_non_sdio(host))
		host->pins_default = pinctrl_lookup_state(host->pinctrl,
							  "sd_1bit_pins");
	else
		host->pins_default = pinctrl_lookup_state(host->pinctrl,
							  "default");
	if (IS_ERR(host->pins_default)) {
		ret = PTR_ERR(host->pins_default);
		goto free_host;
	}

	host->pins_clk_gate = pinctrl_lookup_state(host->pinctrl,
						   "clk-gate");
	if (IS_ERR(host->pins_clk_gate)) {
		dev_warn(&pdev->dev,
			 "can't get clk-gate pinctrl, using clk_stop bit\n");
		host->pins_clk_gate = NULL;
	}

	if (host->run_pxp_flag == 0) {
		host->core_clk = devm_clk_get(&pdev->dev, "core");
		if (IS_ERR(host->core_clk)) {
			ret = PTR_ERR(host->core_clk);
			goto free_host;
		}

		ret = clk_prepare_enable(host->core_clk);
		if (ret)
			goto free_host;

		ret = meson_mmc_clk_init(host);
		if (ret)
			goto err_core_clk;
	} else {
		meson_mmc_pxp_clk_init(host);
	}

	/* set config to sane default */
	meson_mmc_cfg_init(host);

	/* Stop execution */
	writel(0, host->regs + SD_EMMC_START);

	/* clear, ack and enable interrupts */
	writel(0, host->regs + SD_EMMC_IRQ_EN);
	writel(IRQ_CRC_ERR | IRQ_TIMEOUTS | IRQ_END_OF_CHAIN,
	       host->regs + SD_EMMC_STATUS);
	writel(IRQ_CRC_ERR | IRQ_TIMEOUTS | IRQ_END_OF_CHAIN,
	       host->regs + SD_EMMC_IRQ_EN);

	if (host->enable_hwcq) {
		irq_set_affinity_hint(host->irq, cpumask_of(1));
		ret = request_threaded_irq(host->irq, meson_mmc_irq,
				   meson_mmc_irq_thread, IRQF_ONESHOT | IRQF_TRIGGER_HIGH,
				   dev_name(&pdev->dev), host);
	} else {
		ret = request_threaded_irq(host->irq, meson_mmc_irq,
				   meson_mmc_irq_thread, IRQF_ONESHOT,
				   dev_name(&pdev->dev), host);
	}
	if (ret)
		goto err_init_clk;
	if (aml_card_type_mmc(host))
		mmc->caps |= MMC_CAP_CMD23;
	if (host->dram_access_quirk) {
		/* Limit segments to 1 due to low available sram memory */
		mmc->max_segs = 1;
		/* Limit to the available sram memory */
		mmc->max_blk_count = SD_EMMC_SRAM_DATA_BUF_LEN /
				     mmc->max_blk_size;
	} else {
		mmc->max_blk_count = CMD_CFG_LENGTH_MASK;
		mmc->max_segs = SD_EMMC_MAX_SEGS;
	}
	mmc->max_req_size = SD_EMMC_MAX_REQ_SIZE;
	mmc->max_seg_size = mmc->max_req_size;
	mmc->ocr_avail = 0x200080;
	mmc->max_current_180 = 300; /* 300 mA in 1.8V */
	mmc->max_current_330 = 300; /* 300 mA in 3.3V */

	/*
	 * At the moment, we don't know how to reliably enable HS400.
	 * From the different datasheets, it is not even clear if this mode
	 * is officially supported by any of the SoCs
	 */

	if (host->dram_access_quirk) {
		/*
		 * The MMC Controller embeds 1,5KiB of internal SRAM
		 * that can be used to be used as bounce buffer.
		 * In the case of the G12A SDIO controller, use these
		 * instead of the DDR memory
		 */
		host->bounce_buf_size = SD_EMMC_SRAM_DATA_BUF_LEN;
		host->bounce_buf = host->regs + SD_EMMC_SRAM_DATA_BUF_OFF;
		host->bounce_dma_addr = host->res[0]->start + SD_EMMC_SRAM_DATA_BUF_OFF;

		host->descs = host->regs + SD_EMMC_SRAM_DESC_BUF_OFF;
		host->descs_dma_addr = host->res[0]->start + SD_EMMC_SRAM_DESC_BUF_OFF;

	} else {
		/* data bounce buffer */
		host->bounce_buf_size = mmc->max_req_size;
		host->bounce_buf =
			dma_alloc_coherent(host->dev, host->bounce_buf_size,
					   &host->bounce_dma_addr, GFP_KERNEL);
		if (!host->bounce_buf) {
			dev_err(host->dev, "Unable to map allocate DMA bounce buffer.\n");
			ret = -ENOMEM;
			goto err_free_irq;
		}

		host->descs = dma_alloc_coherent(host->dev, SD_EMMC_DESC_BUF_LEN,
			      &host->descs_dma_addr, GFP_KERNEL);
		if (!host->descs) {
			ret = -ENOMEM;
			goto err_bounce_buf;
		}
	}

	host->adj_win = devm_kzalloc(host->dev, sizeof(u8) * ADJ_WIN_PRINT_MAXLEN, GFP_KERNEL);
	if (!host->adj_win) {
		ret = -ENOMEM;
		goto err_free_irq;
	}

	if (aml_card_type_sdio(host)) {
		/* do NOT run mmc_rescan for the first time */
		mmc->rescan_entered = 1;
	} else {
		mmc->rescan_entered = 0;
	}
	if (aml_card_type_non_sdio(host))
		host->pins_default = pinctrl_lookup_state(host->pinctrl, "sd_default");

	mmc->ops = &meson_mmc_ops;
#if IS_ENABLED(CONFIG_AMLOGIC_MMC_CQHCI)
	amlogic_add_host(host);
#else
	mmc_add_host(mmc);
#endif

	if (aml_card_type_sdio(host)) {/* if sdio_wifi */
		sdio_host = mmc;
	}

	if (aml_card_type_mmc(host)) {
		INIT_DELAYED_WORK(&host->dtbkey, add_dtbkey);
		schedule_delayed_work(&host->dtbkey, 50);
	}

#if CONFIG_AMLOGIC_KERNEL_VERSION == 13515
#ifdef CONFIG_ANDROID_VENDOR_HOOKS
	if (aml_card_type_non_sdio(host)) {
		ret =
		register_trace_android_vh_mmc_sd_update_cmdline_timing(SD_CMD_TIMING,
			NULL);
		if (ret)
			pr_err("register update_cmdline_timing failed, err:%d\n", ret);
		ret =
		register_trace_android_vh_mmc_sd_update_dataline_timing(SD_DATA_TIMING,
			NULL);
		if (ret)
			pr_err("register update_dataline timing failed, err:%d\n", ret);
	}
#endif
#endif

#if IS_ENABLED(CONFIG_DEBUG_FS)
	if (mmc->debugfs_root && aml_card_type_mmc(host)) {
		host->debugfs_root = debugfs_create_dir(dev_name(&pdev->dev), mmc->debugfs_root);
		if (IS_ERR_OR_NULL(host->debugfs_root))
			pr_err("mmc debugfs creat failed\n");

		debugfs_create_file("clktest", 0400, host->debugfs_root, mmc,
				&mmc_clktest_fops);
		debugfs_create_file("eyetest", 0400, host->debugfs_root, mmc,
				&mmc_eyetest_fops);
		debugfs_create_file("eyetestlog", 0400, host->debugfs_root, mmc,
				&mmc_eyetestlog_fops);
		debugfs_create_file("tx_win", 0400, host->debugfs_root, mmc,
				&mmc_tx_win_fops);
		debugfs_create_file("cmd_rx_win", 0400, host->debugfs_root, mmc,
				&mmc_cmd_rx_win_fops);
		debugfs_create_file("write_size", 0400, host->debugfs_root, mmc,
				&fops_write_size);
		debugfs_create_file("erase_count", 0400, host->debugfs_root,
				mmc, &erase_count_fops);
	}
#endif
	if (device_property_read_u32(host->dev, "cali_blk_cnt", &cali_blk_cnt) <= 0)
		cali_blk_cnt = CALI_BLK_CNT;
	host->blk_test = devm_kzalloc(host->dev,
				      512 * cali_blk_cnt, GFP_KERNEL);

	if (!host->blk_test) {
		ret = -ENOMEM;
		goto err_bounce_buf;
	}

	dev_dbg(host->dev, "host probe success!\n");
	return 0;

err_bounce_buf:
	if (!host->dram_access_quirk)
		dma_free_coherent(host->dev, host->bounce_buf_size,
				  host->bounce_buf, host->bounce_dma_addr);
err_free_irq:
	devm_kfree(host->dev, host->adj_win);
	free_irq(host->irq, host);
err_init_clk:
	clk_disable_unprepare(host->mmc_clk);
err_core_clk:
	clk_disable_unprepare(host->core_clk);
free_host:
	dev_notice(host->dev, "host probe failed!\n");
	mmc_free_host(mmc);
	return ret;
}

static int meson_mmc_remove(struct platform_device *pdev)
{
	struct meson_host *host = dev_get_drvdata(&pdev->dev);

	mmc_remove_host(host->mmc);

	/* disable interrupts */
	writel(0, host->regs + SD_EMMC_IRQ_EN);
	free_irq(host->irq, host);

	dma_free_coherent(host->dev, SD_EMMC_DESC_BUF_LEN,
			  host->descs, host->descs_dma_addr);

	if (!host->dram_access_quirk)
		dma_free_coherent(host->dev, host->bounce_buf_size,
				  host->bounce_buf, host->bounce_dma_addr);

	clk_disable_unprepare(host->mmc_clk);
	clk_disable_unprepare(host->core_clk);

	devm_kfree(host->dev, host->adj_win);
	mmc_free_host(host->mmc);
	return 0;
}

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static const struct meson_mmc_data meson_gx_data = {
	.tx_delay_mask	= CLK_V2_TX_DELAY_MASK,
	.rx_delay_mask	= CLK_V2_RX_DELAY_MASK,
	.always_on	= CLK_V2_ALWAYS_ON,
	.adjust		= SD_EMMC_ADJUST,
};
#endif

static const struct meson_mmc_data meson_axg_data = {
	.tx_delay_mask	= CLK_V3_TX_DELAY_MASK,
	.rx_delay_mask	= CLK_V3_RX_DELAY_MASK,
	.always_on	= CLK_V3_ALWAYS_ON,
	.adjust		= SD_EMMC_V3_ADJUST,
};

static const struct of_device_id meson_mmc_of_match[] = {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	{ .compatible = "amlogic,meson-gx-mmc",		.data = &meson_gx_data },
	{ .compatible = "amlogic,meson-gxbb-mmc",	.data = &meson_gx_data },
	{ .compatible = "amlogic,meson-gxl-mmc",	.data = &meson_gx_data },
	{ .compatible = "amlogic,meson-gxm-mmc",	.data = &meson_gx_data },
#endif
	{ .compatible = "amlogic,meson-axg-mmc",	.data = &meson_axg_data },
	{}
};
MODULE_DEVICE_TABLE(of, meson_mmc_of_match);

static struct platform_driver meson_mmc_driver = {
	.probe		= meson_mmc_probe,
	.remove		= meson_mmc_remove,
	.driver		= {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(meson_mmc_of_match),
	},
};

static int caps2_setup(char *p)
{
	caps2_quirks = p;
	return 0;
}

__setup("meson-gx-mmc.caps2_quirks=", caps2_setup);

int __init meson_mmc_init(void)
{
	return platform_driver_register(&meson_mmc_driver);
}

void __exit meson_mmc_exit(void)
{
	platform_driver_unregister(&meson_mmc_driver);
}

//module_param(caps2_quirks, charp, 0444);
//MODULE_PARM_DESC(caps2_quirks, "Force certain caps2.");

//MODULE_DESCRIPTION("Amlogic S905*/GX*/AXG SD/eMMC driver");
//MODULE_AUTHOR("Kevin Hilman <khilman@baylibre.com>");
//MODULE_LICENSE("GPL v2");
