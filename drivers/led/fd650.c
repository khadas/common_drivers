// SPDX-License-Identifier: (GPL-2.0+ OR MIT)

#include <linux/module.h>
#include <linux/kernel.h>
#include "fd650.h"
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/amlogic/aml_mbox.h>


#define		PIN_DATA			0
#define		PIN_CLK				1
#define DEFAULT_UDELAY			1

#define MESON_FD650_SCPI_CNT		50

struct mbox_chan *fd650_mbox_chan;

enum FD650State {
	FD650_STATE_DEFAULT = 0,
	FD650_STATE_SHOW,
	FD650_STATE_TIME,
	FD650_STATE_INVALID,
};

struct fd650_bus {
	/*
	 * udelay - delay [us] between GPIO toggle operations,
	 * which is 1/4 of I2C speed clock period.
	 */
	int udelay;
	 /* sda, scl */
	unsigned int gpios[2];
	struct led_classdev cdev;
	int (*get_sda)(struct fd650_bus *bus);
	void (*set_sda)(struct fd650_bus *bus, int bit);
	void (*set_scl)(struct fd650_bus *bus, int bit);
	bool use_bl30_ctrl;
};

#define LED_MAP_NUM 22

struct led_bitmap {
	u8 character;
	u8 bitmap;
};

const struct led_bitmap bcd_decode_tab[LED_MAP_NUM] = {
	{'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F},
	{'4', 0x66}, {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07},
	{'8', 0x7F}, {'9', 0x6F}, {'a', 0x77}, {'A', 0x77},
	{'b', 0x7C}, {'B', 0x7C}, {'c', 0x58}, {'C', 0x39},
	{'d', 0x5E}, {'D', 0x5E}, {'e', 0x79}, {'E', 0x79},
	{'f', 0x71}, {'F', 0x71}
};

u8 led_get_code(char ch)
{
	u8 i, bitmap = 0x00;

	for (i = 0; i < LED_MAP_NUM; i++) {
		if (bcd_decode_tab[i].character == ch) {
			bitmap = bcd_decode_tab[i].bitmap;
			break;
		}
	}

	return bitmap;
}

void fd650_start(struct fd650_bus *bus)
{
	bus->set_sda(bus, 1);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_sda(bus, 0);
	udelay(bus->udelay);
	bus->set_scl(bus, 0);
}

void fd650_stop(struct fd650_bus *bus)
{
	bus->set_sda(bus, 0);
	udelay(bus->udelay);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_sda(bus, 1);
	udelay(bus->udelay);
}

void fd650_wr_byte(struct fd650_bus *bus, u8 dat)
{
	u8 i;

	for (i = 0; i != 8; i++) {
		if (dat & 0x80)
			bus->set_sda(bus, 1);
		else
			bus->set_sda(bus, 0);

		udelay(bus->udelay);
		bus->set_scl(bus, 1);
		dat <<= 1;
		udelay(bus->udelay);
		bus->set_scl(bus, 0);
	}

	bus->set_sda(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 0);
}

u8 fd650_rd_byte(struct fd650_bus *bus)
{
	u8 dat, i;

	bus->set_sda(bus, 1);

	dat = 0;
	for (i = 0; i != 8; i++) {
		udelay(bus->udelay);
		bus->set_scl(bus, 1);
		udelay(bus->udelay);
		dat <<= 1;
		if (bus->get_sda(bus))
			dat++;

		bus->set_scl(bus, 0);
	}

	bus->set_sda(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 0);

	return dat;
}

void fd650_write(struct fd650_bus *bus, u16 cmd)
{
	fd650_start(bus);
	fd650_wr_byte(bus, ((u8)(cmd >> 7) & 0x3E) | 0x40);
	fd650_wr_byte(bus, (u8)cmd);
	fd650_stop(bus);
}

u8 fd650_read(struct fd650_bus *bus)
{
	u8 keycode = 0;

	fd650_start(bus);
	fd650_wr_byte(bus, ((u8)(FD650_GET_KEY >> 7) & 0x3E) | (0x01 | 0x40));
	keycode = fd650_rd_byte(bus);
	fd650_stop(bus);
	if ((keycode & 0x00000040) == 0)
		keycode = 0;

	return keycode;
}

void led_show_650(struct fd650_bus *bus, char *str, unsigned char sec_flag, unsigned char lock)
{
	int i, length;
	int data[4] = {0x00, 0x00, 0x00, 0x00};

	if (strcmp(str, "") == 0)
		return;

	length = strlen(str);
	if (length > 4)
		length = 4;

	for (i = 0; i < length; i++)
		data[i] = led_get_code(str[i]);

	fd650_write(bus, FD650_SYS_ON_8);
	fd650_write(bus, FD650_DIG0 | (u8)data[0] | FD650_DOT);
	if (sec_flag)
		fd650_write(bus, FD650_DIG1 | (u8)data[1] | FD650_DOT);
	else
		fd650_write(bus, FD650_DIG1 | (u8)data[1]);

	if (lock)
		fd650_write(bus, FD650_DIG2 | (u8)data[2] | FD650_DOT);
	else
		fd650_write(bus, FD650_DIG2 | (u8)data[2]);

	fd650_write(bus, FD650_DIG3 | (u8)data[3] | FD650_DOT);
}

static int fd650_sda_get(struct fd650_bus *bus)
{
	int value;

	gpio_direction_input(bus->gpios[PIN_DATA]);
	value = gpio_get_value(bus->gpios[PIN_DATA]);
	gpio_direction_output(bus->gpios[PIN_DATA], 1);

	return value;
}

static void fd650_sda_set(struct fd650_bus *bus, int bit)
{
	gpio_set_value(bus->gpios[PIN_DATA], bit);
}

static void fd650_scl_set(struct fd650_bus *bus, int bit)
{
	gpio_set_value(bus->gpios[PIN_CLK], bit);
}

static ssize_t fd650_display_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* TODO: */
	return 0;
}

static ssize_t fd650_display_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct fd650_bus *bus = container_of(led_cdev,
			struct fd650_bus, cdev);
	char display_str[10];
	int colon_on;
	int ret;
	int count;
	char data[6];

	ret = sscanf(buf, "%d %s", &colon_on, display_str);
	if (ret != 2) {
		dev_err(dev, "Can't parse! usage:[colon_on(1/0) strings]\n");
		return -EINVAL;
	}
	if (bus->use_bl30_ctrl) {
		data[0] = FD650_STATE_SHOW;
		data[1] = colon_on;
		memcpy(data + 2, display_str, 4);
		for (count = 0; count < MESON_FD650_SCPI_CNT; count++) {
			ret = aml_mbox_transfer_data(fd650_mbox_chan, MBOX_CMD_SET_FD650,
						(void *)data, sizeof(data),
						NULL, 0, MBOX_SYNC);
			if (ret >= 0)
				break;
			mdelay(5);
		}

		if (count == MESON_FD650_SCPI_CNT) {
			dev_err(dev, "%s Can't set fd650 display count=%d\n", __func__, count);
			return -EINVAL;
		}
		return size;
	}
	led_show_650(bus, display_str, colon_on, 1);

	return size;
}

static ssize_t fd650_clear_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct fd650_bus *bus = container_of(led_cdev,
			struct fd650_bus, cdev);
	led_show_650(bus, "****", 0, 1);

	return size;
}

static ssize_t fd650_sec_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct fd650_bus *bus = container_of(led_cdev,
			struct fd650_bus, cdev);
	int leds_num, sec_num, ret;

	if (bus->use_bl30_ctrl) {
		dev_err(dev, "controlled by bl30\n");
		return -EINVAL;
	}
	ret = sscanf(buf, "%d %d", &leds_num, &sec_num);
	if (ret != 2 || leds_num > 4 || sec_num > 7) {
		dev_err(dev, "Can't parse! usage:[leds_num sec_num]\n");
		return -EINVAL;
	}
/*          -3--                      -0--
 *       2 |    | 4                5 |    | 1
 *          -7--        -------->     -6--
 *       1 |    | 5                4 |    | 2
 *          -6--                      -3--
 */
	if (sec_num < 3)
		sec_num += 3;
	else if (sec_num > 6)
		sec_num = 6;
	else
		sec_num -= 3;
	switch (leds_num) {
	case 1:
		fd650_write(bus, FD650_DIG0 | (1 << sec_num));
		break;
	case 2:
		fd650_write(bus, FD650_DIG1 | (1 << sec_num));
		break;
	case 3:
		fd650_write(bus, FD650_DIG2 | (1 << sec_num));
		break;
	case 4:
		fd650_write(bus, FD650_DIG3 | (1 << sec_num));
		break;
	default:
		break;
	}

	return size;
}

static ssize_t fd650_time_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct fd650_bus *bus = container_of(led_cdev,
			struct fd650_bus, cdev);
	char data[6];
	int count, ret;

	if (!bus->use_bl30_ctrl) {
		dev_err(dev, "this function valid only when controlled by bl30\n");
		return -EINVAL;
	}
	for (count = 0; count < MESON_FD650_SCPI_CNT; count++) {
		data[0] = FD650_STATE_TIME;
		ret = aml_mbox_transfer_data(fd650_mbox_chan, MBOX_CMD_SET_FD650,
					(void *)data, sizeof(data),
					NULL, 0, MBOX_SYNC);
		if (ret >= 0)
			break;
		mdelay(5);
	}

	if (count == MESON_FD650_SCPI_CNT) {
		dev_err(dev, "%s Can't set fd650 count=%d\n",
			__func__, count);
		return -EINVAL;
	}

	return size;
}

static DEVICE_ATTR_WO(fd650_sec);
static DEVICE_ATTR_WO(fd650_clear);
static DEVICE_ATTR_RW(fd650_display);
static DEVICE_ATTR_WO(fd650_time);

static struct attribute *fd650_attributes[] = {
	&dev_attr_fd650_display.attr,
	&dev_attr_fd650_clear.attr,
	&dev_attr_fd650_sec.attr,
	&dev_attr_fd650_time.attr,
	NULL
};

static struct attribute_group fd650_attribute_group = {
	.attrs = fd650_attributes
};

void fd650_device_create(struct led_classdev *led_cdev)
{
	int rc;

	rc = sysfs_create_group(&led_cdev->dev->kobj,
				 &fd650_attribute_group);
	if (rc)
		goto err_out_led_state;

	return;

err_out_led_state:
	sysfs_remove_group(&led_cdev->dev->kobj,
				&fd650_attribute_group);
}

static int fd650_probe(struct platform_device *pdev)
{
	struct fd650_bus *bus;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	bus = devm_kzalloc(&pdev->dev, sizeof(struct fd650_bus), GFP_KERNEL);
	if (!bus)
		return -ENOMEM;

	bus->gpios[PIN_DATA] = of_get_named_gpio(np, "sda-gpios", 0);
	ret = devm_gpio_request(&pdev->dev, bus->gpios[PIN_DATA], "sda-gpios");
	if (ret)
		goto error2;

	bus->gpios[PIN_CLK] = of_get_named_gpio(np, "scl-gpios", 0);
	ret = devm_gpio_request(&pdev->dev, bus->gpios[PIN_CLK], "scl-gpios");
	if (ret)
		goto error2;

	ret = gpio_direction_output(bus->gpios[PIN_DATA], 1);
	if (ret) {
		dev_err(&pdev->dev, "fd650 gpio sda set dir failed\n");
		return ret;
	}

	ret = gpio_direction_output(bus->gpios[PIN_CLK], 1);
	if (ret) {
		dev_err(&pdev->dev, "fd650 gpio scl set dir failed\n");
		return ret;
	}
	if (fwnode_property_present(&np->fwnode, "use-bl30-ctrl"))
		bus->use_bl30_ctrl = true;
	if (bus->use_bl30_ctrl)
		/* mbox request channel */
		fd650_mbox_chan = aml_mbox_request_channel_byidx(&pdev->dev, 0);
	ret = device_property_read_u32(&pdev->dev, "fd650-gpio,delay-us",
					   &bus->udelay);
	if (!ret)
		bus->udelay = DEFAULT_UDELAY;

	bus->get_sda = fd650_sda_get;
	bus->set_sda = fd650_sda_set;
	bus->set_scl = fd650_scl_set;
	bus->cdev.name = pdev->dev.of_node->name;
	ret = led_classdev_register(&pdev->dev, &bus->cdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register for %s\n",
			bus->cdev.name);
		goto error1;
	}

	fd650_device_create(&bus->cdev);
	platform_set_drvdata(pdev, bus);

	return 0;

error1:
	led_classdev_unregister(&bus->cdev);

	return ret;
error2:
	dev_err(&pdev->dev, "Can't get %s gpios! Error: %d\n", pdev->name, ret);

	return ret;
}

static int fd650_remove(struct platform_device *pdev)
{
	struct fd650_bus *bus = platform_get_drvdata(pdev);

	sysfs_remove_group(&bus->cdev.dev->kobj,
				&fd650_attribute_group);
	led_classdev_unregister(&bus->cdev);

	return 0;
}

static const struct of_device_id of_fd650_match[] = {
	{ .compatible = "amlogic,fd650", },
	{},
};

MODULE_DEVICE_TABLE(led, of_fd650_match);

static struct platform_driver fd650_driver = {
	.probe		= fd650_probe,
	.remove		= fd650_remove,
	.driver		= {
		.name	= "fd650",
		.owner	= THIS_MODULE,
		.of_match_table = of_fd650_match,
	},
};

int __init fd650_init(void)
{
	return platform_driver_register(&fd650_driver);
}

void __exit fd650_exit(void)
{
	platform_driver_unregister(&fd650_driver);
}
