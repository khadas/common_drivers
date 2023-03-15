// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/list.h>
#include <asm/div64.h>
#include <linux/regmap.h>

#include <linux/amlogic/pmic/meson_pmic6b.h>

#define SAR_IRQ_TIMEOUT		5000
int bat_present;

struct pmic6b_bat {
	struct regmap_irq_chip_data *regmap_irq;
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock; /*protect operation propval*/

	struct power_supply *battery;
	struct power_supply *charger;
	struct work_struct char_work;
	struct delayed_work work;
	int status;
	int soc;
	atomic_t sar_irq;

	int internal_resist;
	int total_cap;
	int init_cap;
	int alarm_cap;
	int init_clbcnt;
	int max_volt;
	int min_volt;
	int table_len;
	struct power_supply_battery_ocv_table *cap_table;
};

static char *pmic6b_bat_irqs[] = {
	"CHG_ERROR",
	"CHG_BAT_OCOV",
	"CHG_EOC",
	"CHG_NOBAT",
	"CHG_CHGING",
	"CHG_BAT_TEMP",
	"CHG_TIMEOUT",
	"CHG_PLUGOUT",
	"CHG_PLUGIN",
};

static int pmic6b_bat_irq_bits[] = {
	PMIC6B_IRQ_CHG_ERROR,
	PMIC6B_IRQ_CHG_BAT_OCOV,
	PMIC6B_IRQ_CHG_EOC,
	PMIC6B_IRQ_CHG_NOBAT,
	PMIC6B_IRQ_CHG_CHGING,
	PMIC6B_IRQ_CHG_BAT_TEMP,
	PMIC6B_IRQ_CHG_TIMEOUT,
	PMIC6B_IRQ_CHG_PLUGOUT,
	PMIC6B_IRQ_CHG_PLUGIN,
};

static irqreturn_t pmic6b_chg_error_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear error irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(1), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(1), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(1), 1);
	printk_once("Info: battery charge error.\n");

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_bat_ocov_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear ocov irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(2), 0);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(2), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(2), 1);
	printk_once("Warning: battery is over current or over voltage..\n");

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_eoc_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear eoc irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(3), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(3), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(3), 1);
	printk_once("Info: battery charge is finished. .\n");

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_no_bat_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear no bat irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(4), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(4), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(4), 1);
	printk_once("Warning: there is no battery.\n");

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_chging_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear chging irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(5), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(5), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(5), 1);
	printk_once("Info: started charging.\n");

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_bat_temp_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear bat temp irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(6), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(6), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(6), 1);
	printk_once("Warning: the battery is over temp.\n");

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_timeout_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear timeout irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(7), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(7), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(7), 1);
	printk_once("Warning: battery charging timeout.\n");

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_plugout_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear plugout irq*/
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(9), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(9), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(9), 1);
	printk_once("Info: DCIN has plug out.\n");

	bat_present = 0;

	return IRQ_HANDLED;
}

static irqreturn_t pmic6b_chg_plugin_irq_handle(int irq, void *data)
{
	struct pmic6b_bat *bat = data;
	int ret;

	/*clear plugin irq*/

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(10), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_STATUS_CLR0,
				 BIT(10), 1);
	if (ret)
		dev_err(bat->dev,
			"Failed to read status: %d\n", __LINE__);

	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_IRQ_MASK0,
				 BIT(10), 1);
	printk_once("Info: DCIN has plug in.\n");
	bat_present = 1;
	return IRQ_HANDLED;
}

static irq_handler_t pmic6b_bat_irq_handle[] = {
	pmic6b_chg_error_irq_handle,
	pmic6b_chg_bat_ocov_irq_handle,
	pmic6b_chg_eoc_irq_handle,
	pmic6b_chg_no_bat_irq_handle,
	pmic6b_chg_chging_irq_handle,
	pmic6b_chg_bat_temp_irq_handle,
	pmic6b_chg_timeout_irq_handle,
	pmic6b_chg_plugout_irq_handle,
	pmic6b_chg_plugin_irq_handle,
};

static int pmic6b_bat_present(struct pmic6b_bat *bat,
			     union power_supply_propval *val)
{
	val->intval = bat_present;
	return bat_present;
}

static int pmic6b_bat_voltage_now(struct pmic6b_bat *bat, int *val)
{
	u32 value;
	int ret, result = 0;

	ret = regmap_read(bat->regmap, PMIC6B_AR_SW_EN, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);

	value &= ~(0x000f);
	value |= 0x0002;
	ret = regmap_write(bat->regmap, PMIC6B_AR_SW_EN, value);
	if (ret < 0) {
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
		return ret;
	}
	mdelay(1);
	ret = regmap_read(bat->regmap, PMIC6B_AR_SW_EN, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	mdelay(2);

	ret = regmap_read(bat->regmap, PMIC6B_SAR_RD_VBAT_OFF, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);

	// Vbat = val*(4.8V/4096) = val*(4800mV/4096)
	result = (value * 4800) / 4096;
	*val = result;

	return 0;
}

/* todo CHG_DA_DB_REG0 CHG_DA_DB_REG1 */
static int pmic6b_bat_health(struct pmic6b_bat *bat,
			    union power_supply_propval *val)
{
	int vol;
	int ret;

	ret = pmic6b_bat_voltage_now(bat, &vol);
	if (ret)
		return ret;

	if (vol > bat->max_volt)
		val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	else
		val->intval = POWER_SUPPLY_HEALTH_GOOD;

	return 0;
}

/* return charge(+) or discharge(-) value mA */
static int pmic6b_bat_current_now(struct pmic6b_bat *bat, int *val)
{
	u32 value;
	int ret;
	int sign_bit;
	u64 temp;

	/* set the measure once bit to 1 */
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_AR_SW_EN,
				 BIT(0), 0);
	ret = regmap_update_bits(bat->regmap,
				 PMIC6B_AR_SW_EN,
				 BIT(0), 1);
	ret = regmap_read(bat->regmap, PMIC6B_AR_SW_EN, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	mdelay(1);
	ret = regmap_read(bat->regmap, PMIC6B_SAR_RD_IBAT_LAST, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);

	temp = value & 0x1fff;

	sign_bit = temp & 0x1000;

	/* complement code */
	if (temp & 0x1000)
		temp = (temp ^ 0x1fff) + 1;

	ret = (temp * 5333) / 4096 * (sign_bit ? 1 : -1);
	if (ret < 0)
		ret = 0 - ret;
	*val = ret;

	return 0;
}

static inline int pmic6b_get_charge_status(struct pmic6b_bat *bat)
{
	u32 value1, value2;
	int ret = 0;

	ret = regmap_read(bat->regmap, PMIC6B_CHG_DA_DB_REG0,
			  &value1);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	ret = regmap_read(bat->regmap, PMIC6B_CHG_DA_OUT_REG,
			  &value2);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
/*
 *#ifdef PRINTK
 *	printk("DA_QI_PPATH_FASTON_DB: %0x\n", value1);
 *	printk("DA_QI_CHGING_EN: %0x\n", value2);
 *#endif
 */
	/* 1-charging 2-discharging 3-stand by update sp_dcdc_status*/
	if (value2 & 0x8)
		return 1;
	else if (value1 & 0x1)
		return 2;
	else
		return 3;
}

static int avg_voltage, avg_current;

static int pmic6b_bat_voltage_ocv(struct pmic6b_bat *bat, int *val)
{
	int ret;
	int vol, cur, charge_status, ocv;

	ret = pmic6b_bat_voltage_now(bat, &vol);
	if (ret)
		return ret;

	ret = pmic6b_bat_current_now(bat, &cur);
	if (ret)
		return ret;

	charge_status = pmic6b_get_charge_status(bat);
	/* 1-charging 2-discharging 3-stand by ;charging vbat > ocv */
	if (charge_status == 1)
		ocv = vol - (cur * bat->internal_resist) / 1000;
	else if (charge_status == 2)
		ocv = vol + (cur * bat->internal_resist) / 1000;
	else
		ocv = vol;

	avg_voltage += vol;
	avg_current += cur;

	*val = ocv;
	return 0;
}

//----------------------------------------------------------------------
//---------------------------------chip_id------------------------------
//
//0x0010   |----------------|-----------------|---------------|-------|
//         | 15  14  13  12 | 11  10  9  8  7 | 6  5  4  3  2 | 1  0  |
//         |      Month     |         Day     |     Hour      | Unused|
//
//0x003B   |----------------|--------------------|--------------------|
//         | 15  14  13  12 | 11  10  9  8  7  6 | 5  4  3  2   1  0  |
//         | Year(2021->0..)|        Min         |         Sec        |
//
//0x000A   |----------------|-----------|---------|---------|---------|
//         | 15  14  13  12 | 11  10  9 | 8  7  6 | 5  4  3 | 2 1  0  |
//         | efuse version  | pkg info  |Wafer Rev|   LB    | site num|
//
int  pmu__version = -1;
int pmic6b_get_pmu_version(struct pmic6b_bat *bat)
{
	u32 value = 0;
	int ret;

	ret = regmap_read(bat->regmap, PMIC6B_SAR_RD_IBAT_LAST, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	if (pmu__version == -1)
		pmu__version = (value & 0xf000) >> 11;

	return 0;
}

static int pmic6b_bat_capacity(struct pmic6b_bat *bat,
			      union power_supply_propval *val)
{
	struct power_supply_battery_ocv_table *table = bat->cap_table;
	int vbat_ocv;
	int ocv_lower = 0;
	int ocv_upper = 0;
	int capacity_lower = 0;
	int capacity_upper = 0;
	int ret, i, tmp, flag;

	ret = pmic6b_bat_voltage_ocv(bat, &vbat_ocv);
	if (ret)
		return ret;

	if ((vbat_ocv * 1000) >= table[0].ocv) {
		val->intval = 100;
		return 0;
	}

	if ((vbat_ocv * 1000) <= table[bat->table_len - 1].ocv) {
		val->intval = 0;
		return 0;
	}

	flag = 0;
	for (i = 0; i < bat->table_len - 1; i++) {
		if (((vbat_ocv * 1000) <= table[i].ocv) &&
		    ((vbat_ocv * 1000) >= table[i + 1].ocv)) {
			ocv_lower = table[i + 1].ocv;
			ocv_upper = table[i].ocv;
			capacity_lower = table[i + 1].capacity;
			capacity_upper = table[i].capacity;
			flag = 1;
			break;
		}
	}

	if (!flag)
		return -EAGAIN;

	tmp = capacity_upper - capacity_lower;
	tmp = DIV_ROUND_CLOSEST(((vbat_ocv * 1000 - ocv_lower) * tmp),
				(ocv_upper - ocv_lower));
	val->intval = capacity_lower + tmp;
	return 0;
}

static enum power_supply_property pmic6b_bat_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_ONLINE,
};

static int pmic6b_bat_get_prop(struct power_supply *psy,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	struct pmic6b_bat *bat = dev_get_drvdata(psy->dev.parent);
	int value = 0;
	int ret = -EINVAL;

	mutex_lock(&bat->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = pmic6b_bat_capacity(bat, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = pmic6b_bat_present(bat, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = pmic6b_bat_voltage_now(bat, &value);
		if (ret)
			return ret;
		val->intval = value * 1000; //uV
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = pmic6b_bat_health(bat, val);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		ret = pmic6b_bat_current_now(bat, &value);
		if (ret)
			return ret;
		val->intval = value * 1000; //uA
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		ret = pmic6b_bat_voltage_ocv(bat, &value);
		if (ret)
			return ret;
		val->intval = value * 1000; //uV
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		if (pmic6b_get_charge_status(bat) == 1)
			val->intval = 1;
		else
			val->intval = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&bat->lock);
	return 0;
}

static inline int pmic6b_get_charger_voltage(struct pmic6b_bat *bat,
					    union power_supply_propval *val)
{
	u32 value;
	int ret, result = 0;

	ret = regmap_read(bat->regmap, PMIC6B_AR_SW_EN, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	value &= ~(0x000f);
	value |= 0x0004;
	ret = regmap_write(bat->regmap, PMIC6B_AR_SW_EN, value);
	if (ret < 0) {
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
		return ret;
	}

	mdelay(2);

	ret = regmap_read(bat->regmap, PMIC6B_SAR_RD_VBAT_ACTIVE, &value);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);

	// Vbat = val*(4.8V/4096) = val*(4800mV/4096)
	result = (value * 4800) / 4096;
	val->intval = result;

	return 0;
}

static inline int pmic6b_get_charger_current(struct pmic6b_bat *charger,
					    union power_supply_propval *val)
{
	u32 value;

	pmic6b_bat_current_now(charger, &value);
	val->intval = value;
	return 0;
}

static inline int pmic6b_get_charger_status(struct pmic6b_bat *charger,
					   union power_supply_propval *val)
{
	u32 value1, value2;
	int ret = 0;

	ret = regmap_read(charger->regmap, PMIC6B_CHG_DA_DB_REG0,
			  &value1);
	if (ret < 0)
		dev_dbg(charger->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	ret = regmap_read(charger->regmap, PMIC6B_CHG_DA_OUT_REG,
			  &value2);
	if (ret < 0)
		dev_dbg(charger->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
/*
 *#ifdef PRINTK
 *	printk("DA_QI_PPATH_FASTON_DB: %0x\n", value1);
 *	printk("DA_QI_CHGING_EN: %0x\n", value2);
 *#endif
 */
	if (value2 & BIT(3))
		val->intval = POWER_SUPPLY_STATUS_CHARGING;
	else if (!(value1 & BIT(0)))
		val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
	else
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
	return ret;
}

static inline
int pmic6b_get_charger_current_limit(struct pmic6b_bat *charger,
				    union power_supply_propval *val)
{
	//nothing need to do

	return 0;
}

static int pmic6b_charger_get_prop(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	struct pmic6b_bat *charger = dev_get_drvdata(psy->dev.parent);
	int ret;
	u32 value1;

	ret = regmap_read(charger->regmap, PMIC6B_CHG_DA_DB_REG0,
			  &value1);
	if (ret < 0)
		dev_dbg(charger->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	mutex_lock(&charger->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = pmic6b_get_charger_current(charger, val);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = pmic6b_get_charger_status(charger, val);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		ret = pmic6b_get_charger_voltage(charger, val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&charger->lock);
	return ret;
}

static inline int pmic6b_set_charger_current(struct pmic6b_bat *charger,
					    int val)
{
	int ret;

	ret = regmap_write(charger->regmap, PMIC6B_OTP_REG_0x27, val);
	if (ret < 0) {
		dev_dbg(charger->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
		return ret;
	}

	return 0;
}

static int pmic6b_charger_set_prop(struct power_supply *psy,
				  enum power_supply_property psp,
				  const union power_supply_propval *val)
{
	struct pmic6b_bat *charger = dev_get_drvdata(psy->dev.parent);
	int ret;

	mutex_lock(&charger->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = pmic6b_set_charger_current(charger, val->intval);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&charger->lock);
	return ret;
}

static int pmic6b_charger_writable_property(struct power_supply *psy,
					   enum power_supply_property psp)
{
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 1;
		break;

	default:
		ret = 0;
	}

	return ret;
}

static enum power_supply_property pmic6b_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
};

static const struct power_supply_desc pmic6b_charger_desc = {
	.name		= "pmic6b-charger",
	.type		= POWER_SUPPLY_TYPE_MAINS,
	.properties	= pmic6b_charger_props,
	.num_properties	= ARRAY_SIZE(pmic6b_charger_props),
	.get_property	= pmic6b_charger_get_prop,
	.set_property	= pmic6b_charger_set_prop,
	.property_is_writeable = pmic6b_charger_writable_property,
};

static const struct power_supply_desc pmic6b_bat_desc = {
	.name		= "pmic6b-bat",
	.type		= POWER_SUPPLY_TYPE_BATTERY,
	.properties	= pmic6b_bat_props,
	.num_properties	= ARRAY_SIZE(pmic6b_bat_props),
	.get_property	= pmic6b_bat_get_prop,
};

int pmic6b_bat_hw_init(struct pmic6b_bat *bat)
{
	int ret = 0;
	struct power_supply_battery_info info = { };
	struct power_supply_battery_ocv_table *table;

	ret = power_supply_get_battery_info(bat->battery, &info);
	if (ret) {
		dev_err(bat->dev, "failed to get battery information\n");
		return ret;
	}
	bat->total_cap = info.charge_full_design_uah / 1000;
	bat->max_volt = info.constant_charge_voltage_max_uv / 1000;
	bat->internal_resist = info.factory_internal_resistance_uohm / 1000;
	bat->min_volt = info.voltage_min_design_uv;

	table = power_supply_find_ocv2cap_table(&info, 20, &bat->table_len);
	if (!table)
		return -EINVAL;

	bat->cap_table = devm_kmemdup(bat->dev, table,
				      bat->table_len * sizeof(*table),
				      GFP_KERNEL);
	if (!bat->cap_table) {
		power_supply_put_battery_info(bat->battery, &info);
		return -ENOMEM;
	}

	bat->alarm_cap = power_supply_ocv2cap_simple(bat->cap_table,
						     bat->table_len,
						     bat->min_volt);

	power_supply_put_battery_info(bat->battery, &info);

	return ret;
}

static int pmic6b_map_irq(struct pmic6b_bat *bat, int irq)
{
	return regmap_irq_get_virq(bat->regmap_irq, irq);
}

int pmic6b_request_irq(struct pmic6b_bat *bat, int irq, char *name,
		      irq_handler_t handler, void *data)
{
	int virq;

	virq = pmic6b_map_irq(bat, irq);
	if (virq < 0)
		return virq;

	return devm_request_threaded_irq(bat->dev, virq, NULL, handler,
					 IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					 name, data);
}

static int pmic6b_bat_probe(struct platform_device *pdev)
{
	struct meson_pmic6b *pmic6b = dev_get_drvdata(pdev->dev.parent);
	struct device_node *np = pdev->dev.of_node;
	struct pmic6b_bat *bat;
	int ret = 0;
	int i;
	int val = 0;

	ret = of_device_is_available(np);
	if (!ret)
		return -ENODEV;

	bat = devm_kzalloc(&pdev->dev, sizeof(struct pmic6b_bat), GFP_KERNEL);
	if (!bat)
		return -ENOMEM;

	platform_set_drvdata(pdev, bat);
	bat->regmap_irq = pmic6b->regmap_irq;
	bat->regmap = pmic6b->regmap;
	bat->dev = &pdev->dev;
	atomic_set(&bat->sar_irq, 0);
	mutex_init(&bat->lock);

	/* set sar clk 1MHz */
	ret = regmap_read(bat->regmap, PMIC6B_SAR_CNTL_REG0, &val);
	if (ret < 0) {
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
		return ret;
	}
	val &= ~(0x0f00);
	val |= 0x300;
	ret = regmap_write(bat->regmap, PMIC6B_SAR_CNTL_REG0, val);
	if (ret < 0) {
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
		return ret;
	}

	ret = regmap_read(bat->regmap, PMIC6B_CHG_DA_DB_REG0,
			  &val);
	if (ret < 0)
		dev_dbg(bat->dev,
			"failed in func: %s line: %d\n", __func__, __LINE__);
	if (val & BIT(0))
		bat_present = 0;
	else
		bat_present = 1;

	for (i = 0; i < ARRAY_SIZE(pmic6b_bat_irqs); i++) {
		ret = pmic6b_request_irq(bat, pmic6b_bat_irq_bits[i],
					pmic6b_bat_irqs[i],
					pmic6b_bat_irq_handle[i],
					bat);
		if (ret != 0) {
			dev_err(bat->dev,
				"PMIC6B failed to request %s IRQ: %d\n",
				pmic6b_bat_irqs[i], ret);
			return ret;
		}
	}
	bat->charger = devm_power_supply_register(&pdev->dev,
						  &pmic6b_charger_desc,
						  NULL);
	if (IS_ERR(bat->charger)) {
		ret = PTR_ERR(bat->charger);
		dev_err(&pdev->dev, "failed to register charger: %d\n", ret);
		return ret;
	}

	bat->battery = devm_power_supply_register(&pdev->dev,
						  &pmic6b_bat_desc, NULL);
	if (IS_ERR(bat->battery)) {
		ret = PTR_ERR(bat->battery);
		dev_err(&pdev->dev, "failed to register battery: %d\n", ret);
		return ret;
	}

	bat->battery->of_node = np;
	bat->charger->of_node = np;
	ret = pmic6b_bat_hw_init(bat);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"Failed to pmic6b_dev_of_bat_init\n");
		return ret;
	}

	return 0;
}

static int pmic6b_bat_remove(struct platform_device *pdev)
{
	struct pmic6b_bat *bat = platform_get_drvdata(pdev);

	cancel_delayed_work(&bat->work);

	return 0;
}

static const struct of_device_id pmic6b_bat_match_table[] = {
	{ .compatible = "amlogic,pmic6b-battery" },
	{ },
};
MODULE_DEVICE_TABLE(of, pmic6b_bat_match_table);

static struct platform_driver pmic6b_bat_driver = {
	.driver = {
		.name = "pmic6b-battery",
		.of_match_table = pmic6b_bat_match_table,
	},
	.probe = pmic6b_bat_probe,
	.remove = pmic6b_bat_remove,
};

int __init meson_pmic6b_bat_init(void)
{
	return platform_driver_register(&pmic6b_bat_driver);
}

void __exit meson_pmic6b_bat_exit(void)
{
	platform_driver_unregister(&pmic6b_bat_driver);
}

MODULE_DESCRIPTION("Battery Driver for PMIC6B");
MODULE_AUTHOR("Amlogic");
MODULE_LICENSE("GPL");
