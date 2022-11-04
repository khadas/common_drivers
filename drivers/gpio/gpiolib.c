// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/gpiolib.h>
#include <gpiolib.h>

/*
 * This descriptor validation needs to be inserted verbatim into each
 * function taking a descriptor, so we need to use a preprocessor
 * macro to avoid endless duplication. If the desc is NULL it is an
 * optional GPIO and calls should just bail out.
 */
static int validate_desc(const struct gpio_desc *desc, const char *func)
{
	if (!desc)
		return 0;
	if (IS_ERR(desc)) {
		pr_warn("%s: invalid GPIO (errorpointer)\n", func);
		return PTR_ERR(desc);
	}
	if (!desc->gdev) {
		pr_warn("%s: invalid GPIO (no device)\n", func);
		return -EINVAL;
	}
	if (!desc->gdev->chip) {
		dev_warn(&desc->gdev->dev,
			 "%s: backing chip is gone\n", func);
		return 0;
	}
	return 1;
}

#define VALIDATE_DESC(desc) do { \
	int __valid = validate_desc(desc, __func__); \
	if (__valid <= 0) \
		return __valid; \
	} while (0)

/**
 * gpiod_set_pull - enable pull-down/up for the gpio, or disable.
 * @desc: descriptor of the GPIO for which to set pull
 * @value: value to set
 *
 * Returns:
 * 0 on success, %-ENOTSUPP if the controller doesn't support setting the
 * pull.
 */
int gpiod_set_pull(struct gpio_desc *desc, unsigned int value)
{
	struct gpio_chip	*chip;
	unsigned long		config;
	enum pin_config_param	mode;

	VALIDATE_DESC(desc);
	chip = desc->gdev->chip;
	if (!chip || !chip->set_config) {
		gpiod_dbg(desc,
			  "%s: missing set() or set_config() operations\n",
			  __func__);
		return -ENOTSUPP;
	}

	switch (value) {
	case GPIOD_PULL_DIS:
		mode = PIN_CONFIG_BIAS_DISABLE;
		break;
	case GPIOD_PULL_DOWN:
		mode = PIN_CONFIG_BIAS_PULL_DOWN;
		break;
	case GPIOD_PULL_UP:
		mode = PIN_CONFIG_BIAS_PULL_UP;
		break;
	default:
		return -ENOTSUPP;
	}
	config = PIN_CONF_PACKED(mode, 0);

	return chip->set_config(chip, gpio_chip_hwgpio(desc), config);
}
EXPORT_SYMBOL_GPL(gpiod_set_pull);

int __init gpiolib_module_init(void)
{
	return 0;
}

void __exit gpiolib_module_exit(void)
{
}
