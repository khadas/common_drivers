// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/gpio/consumer.h>
#include <asm-generic/io.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/dma-mapping.h>
#include <linux/leds.h>

// #define M_DEBUG

#ifdef M_DEBUG
#define DEBUG
#define LEDCON_DBG(fmt, ...) pr_info("%s " fmt, "[DCON LED]", ##__VA_ARGS__)
#else
#define LEDCON_DBG(fmt, ...)
#endif

#define LED_MAX_NUM			32
#define	COLOR_CHANNEL_NUM 3
/* Meson DCON_LED register map */
#define LED_CTRL_DATA_BASE		0x00
#define LED_CTRL_DATA_NUM		32
#define LED_CONTROL_REG		(0x0020  << 2)
#define LED_CYCLE_RATIO_RES		(0x0021  << 2)

/* Control DCON_LED fields */
#define CTRL_INTERRUPT_CLEAR		BIT(31)
#define CTRL_OUTPUT_ENABLE	BIT(30)
#define CTRL_START		BIT(29)
#define CTRL_INTERRUPT_DISABLE		BIT(28)
#define CTRL_LSB_FIRST		BIT(27)
#define CTRL_PUT_RESET_CODE_SHIFT		11
#define CTRL_PUT_RESET_CODE_MSK		GENMASK(26, 11)
#define CTRL_LED_NUMS_SHIFT		6
#define CTRL_LED_NUMS_MSK	GENMASK(10, 6)
#define CTRL_LED_FRAME_NUMS_SHIFT		2
#define CTRL_LED_FRAME_NUMS_MSK		GENMASK(5, 2)
#define CTRL_LED_DATA_FORMAT_SHIFT		0
#define CTRL_LED_DATA_FORMAT_MSK	GENMASK(1, 0)

enum {
	FORMAT_8BIT = 0,
	FORMAT_16BIT,
	FORMAT_24BIT,
	FORMAT_32BIT,
};

/* Control DCON_LED RATIO fields */
#define RATIO_INTERRUPT_STATUS		BIT(29)
#define RATIO_BUSY_STATUS		BIT(28)
#define RATIO_PRE_RESETCODE		BIT(27)
#define RATIO_RESET_DURATION_SHIFT		17
#define RATIO_RESET_DURATION_MSK		GENMASK(26, 17)
#define RATIO_HIGH_CODE_SET_SHIFT		11
#define RATIO_HIGH_CODE_SET_MSK		GENMASK(16, 11)
#define RATIO_LOW_CODE_SET_SHIFT		6
#define RATIO_LOW_CODE_SET_MSK		GENMASK(10, 6)
#define RATIO_CYCLE_SHIFT		0
#define RATIO_CYCLE_MSK		GENMASK(5, 0)

struct meson_unipolar_ctrl {
	struct device		*dev;
	struct led_classdev cdev;
	void __iomem		*regs;
	spinlock_t		lock;//protection for led controller critical section
	int irq;
	uint led_num;
	u8 *color_data;
};

#define MESON_UNIPOLAR_CTRL_CDEV_NAME		"unipolar_led"

static void meson_unipolar_ctrl_set_mask(struct meson_unipolar_ctrl *dcon_led, int reg, u32 mask,
			       u32 val)
{
	u32 data;

	data = readl(dcon_led->regs + reg);
	data &= ~mask;
	data |= val & mask;
	writel(data, dcon_led->regs + reg);
}

static void meson_unipolar_ctrl_xfer(struct meson_unipolar_ctrl *dcon_led)
{
	meson_unipolar_ctrl_set_mask(dcon_led, LED_CONTROL_REG, CTRL_START, CTRL_START);
}

static void meson_unipolar_ctrl_init(struct meson_unipolar_ctrl *dcon_led)
{
	/*disable interrupt*/
	meson_unipolar_ctrl_set_mask(dcon_led,
		LED_CONTROL_REG, CTRL_INTERRUPT_CLEAR, CTRL_INTERRUPT_CLEAR);
	meson_unipolar_ctrl_set_mask(dcon_led,
		LED_CONTROL_REG, CTRL_INTERRUPT_DISABLE, CTRL_INTERRUPT_DISABLE);
	/*set led num*/
	meson_unipolar_ctrl_set_mask(dcon_led, LED_CONTROL_REG,
		CTRL_LED_NUMS_MSK, (dcon_led->led_num - 1) << CTRL_LED_NUMS_SHIFT);
	/*set one frame data flow*/
	// meson_unipolar_ctrl_set_mask(dcon_led,
	// LED_CONTROL_REG,CTRL_LED_FRAME_NUMS_MSK, 0 << CTRL_LED_FRAME_NUMS_SHIFT);
	/*set format RGB 24BIT MSB first*/
	meson_unipolar_ctrl_set_mask(dcon_led, LED_CONTROL_REG,
		CTRL_LED_DATA_FORMAT_MSK, FORMAT_24BIT << CTRL_LED_DATA_FORMAT_SHIFT);

	/* pwm ratio set
	 * use default reg value
	 * output 1---->h:0.833 l:0.416
	 * output 0---->h:0.416 l:0.833
	 */
	/*set reset duration 0x50*1.25 = 100 us*/
	meson_unipolar_ctrl_set_mask(dcon_led, LED_CYCLE_RATIO_RES,
		RATIO_RESET_DURATION_MSK, 0x50 << RATIO_RESET_DURATION_SHIFT);

	/*send data with out reset*/
	// meson_unipolar_ctrl_set_mask(dcon_led,LED_CYCLE_RATIO_RES,
	// RATIO_RESET_DURATION_MSK, RATIO_PRE_RESETCODE);
}

static void meson_unipolar_ctrl_put_data(struct meson_unipolar_ctrl *dcon_led)
{
	u32 wdata = 0;
	int i, j;
	char *buf = dcon_led->color_data;

	LEDCON_DBG("%s flush controller data reg\n", __func__);
	spin_lock(&dcon_led->lock);
	for (j = 0; j < dcon_led->led_num; j++) {
		for (i = 0; i < COLOR_CHANNEL_NUM; i++) {
			wdata |= *buf++ << ((2 - i) * 8);
			LEDCON_DBG("%s: buffer ptr is 0x%x\n", __func__, buf);
		}
		LEDCON_DBG("flushing data 0x%x to 0x%x\n", wdata,
						dcon_led->regs + LED_CTRL_DATA_BASE + j * 4);
		writel(wdata, dcon_led->regs + LED_CTRL_DATA_BASE + j * 4);
		wdata = 0;
	}
	spin_unlock(&dcon_led->lock);
}

static int meson_unipolar_ctrl_put_data_to_buffer(struct meson_unipolar_ctrl *dcon_led,
			u8 buffer_id, u8 r_data, u8 g_data, u8 b_data)
{
	spin_lock(&dcon_led->lock);
	/*R G B*/
	// dcon_led->color_data[buffer_id] = r_data;
	// dcon_led->color_data[buffer_id + 1] = g_data;
	// dcon_led->color_data[buffer_id + 2] = b_data;
	/*G R B*/
	dcon_led->color_data[buffer_id] = g_data;
	dcon_led->color_data[buffer_id + 1] = r_data;
	dcon_led->color_data[buffer_id + 2] = b_data;
	spin_unlock(&dcon_led->lock);

	return 0;
}

static int meson_unipolar_ctrl_set_singlecolors(u32 ledid,
			struct meson_unipolar_ctrl *dcon_led, u32 color)
{
	u8 r_data, g_data, b_data;

	if (ledid > dcon_led->led_num - 1) {
		dev_err(dcon_led->dev, "valid led id\n");
		return -1;
	}
	LEDCON_DBG("%s set led id %d, color 0x%x\n", __func__, ledid, color);
	r_data = (color & GENMASK(24, 16)) >> 16;
	g_data = (color & GENMASK(15, 8)) >> 8;
	b_data = color & GENMASK(7, 0);
	LEDCON_DBG("%s set led id %d, color r:0x%x g:0x%x b:0x%x\n",
		__func__, ledid, r_data, g_data, b_data);
	meson_unipolar_ctrl_put_data_to_buffer(dcon_led,
					ledid * COLOR_CHANNEL_NUM, r_data, g_data, b_data);
	meson_unipolar_ctrl_put_data(dcon_led);
	meson_unipolar_ctrl_xfer(dcon_led);

	return 0;
}

static int meson_unipolar_ctrl_clear_all_colors(struct meson_unipolar_ctrl *dcon_led)
{
	LEDCON_DBG("%s\n", __func__);
	memset(dcon_led->color_data, 0, dcon_led->led_num * COLOR_CHANNEL_NUM);
	meson_unipolar_ctrl_put_data(dcon_led);
	meson_unipolar_ctrl_xfer(dcon_led);

	return 0;
}

static ssize_t single_color_show(struct device *child,
			  struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(child);
	struct meson_unipolar_ctrl *led_con = container_of(led_cdev,
			struct meson_unipolar_ctrl, cdev);
	u32 color;
	int j;
	ssize_t len = 0;

	for (j = 0; j < led_con->led_num; j++) {
		color = (led_con->color_data[j * COLOR_CHANNEL_NUM] << 16) +
				(led_con->color_data[j * COLOR_CHANNEL_NUM + 1] << 8) +
				led_con->color_data[j * COLOR_CHANNEL_NUM + 2];
		len += snprintf(buf + len, PAGE_SIZE - len, "color%d=0x%x\n", j, color);
	}

	return len;
}

static ssize_t single_color_store(struct device *child,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(child);
	struct meson_unipolar_ctrl *dcon_led = container_of(led_cdev,
			struct meson_unipolar_ctrl, cdev);
	int ret;
	u32 id, color;

	ret = sscanf(buf, "%d %x", &id, &color);
	LEDCON_DBG("%s id:%d color 0x%x\n", __func__, id, color);
	if (ret != 2) {
		dev_err(dcon_led->dev, "Can't parse! usage:[id color]\n");
		return -EINVAL;
	}
	meson_unipolar_ctrl_set_singlecolors(id, dcon_led, color);

	return size;
}

static DEVICE_ATTR_RW(single_color);

static ssize_t colors_clear_all_store(struct device *child,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(child);
	struct meson_unipolar_ctrl *dcon_led = container_of(led_cdev,
			struct meson_unipolar_ctrl, cdev);
	int ret;
	u32 clear;

	ret = kstrtou32(buf, 10, &clear);
	LEDCON_DBG("%s id:%d clear 0x%x\n", __func__, clear);
	if (ret) {
		dev_err(dcon_led->dev, "Can't parse! usage:[echo 1 > colors_clear_all]\n");
		return -EINVAL;
	}
	meson_unipolar_ctrl_clear_all_colors(dcon_led);

	return size;
}

static DEVICE_ATTR_WO(colors_clear_all);

static ssize_t colors_store(struct device *child,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(child);
	struct meson_unipolar_ctrl *dcon_led = container_of(led_cdev,
			struct meson_unipolar_ctrl, cdev);
	int ret, i, split_n;
	u32 id_start, id_end, color;
	char *str_buff;
	void *str_buff_;
	u8 r_data, g_data, b_data;

	str_buff_ = kzalloc(size, GFP_KERNEL);
	memcpy(str_buff_, buf, size);
	str_buff = (char *)str_buff_;
	ret = sscanf(str_buff, "%d %d %n", &id_start, &id_end, &split_n);
	str_buff += split_n;
	LEDCON_DBG("%s start_id:%d end_id:%d\n", __func__, id_start, id_end);
	if (ret != 2 || id_start > id_end || id_end > LED_MAX_NUM - 1) {
		dev_err(dcon_led->dev, "Can't parse! usage:[id_start id_end color1 color2...]\n");
		kfree(str_buff_);
		return -EINVAL;
	}
	for (i = 0; i < id_end - id_start + 1; i++) {
		ret = sscanf(str_buff, "%x %n", &color,  &split_n);
		if (ret != 1) {
			dev_err(dcon_led->dev, "Can't parse! usage:[id_start id_end color1 color2...]\n");
			kfree(str_buff_);
			return -EINVAL;
		}
		str_buff += split_n;
		r_data = (color & GENMASK(24, 16)) >> 16;
		g_data = (color & GENMASK(15, 8)) >> 8;
		b_data = color & GENMASK(7, 0);
		meson_unipolar_ctrl_put_data_to_buffer(dcon_led,
					(id_start + i) * COLOR_CHANNEL_NUM, r_data, g_data, b_data);
	}
	meson_unipolar_ctrl_put_data(dcon_led);
	meson_unipolar_ctrl_xfer(dcon_led);
	kfree(str_buff_);

	return size;
}

static DEVICE_ATTR_WO(colors);

static struct attribute *meson_unipolar_ctrl_attributes[] = {
	&dev_attr_single_color.attr,
	&dev_attr_colors_clear_all.attr,
	&dev_attr_colors.attr,
	NULL
};

static struct attribute_group meson_unipolar_ctrl_attribute_group = {
	.attrs = meson_unipolar_ctrl_attributes
};

static irqreturn_t meson_unipolar_led_irq(int irqno, void *dev_id)
{
	struct meson_unipolar_ctrl *dcon_led = dev_id;

	LEDCON_DBG("interrupt\n");
	meson_unipolar_ctrl_set_mask(dcon_led, LED_CONTROL_REG,
				CTRL_INTERRUPT_CLEAR, CTRL_INTERRUPT_CLEAR);

	return IRQ_HANDLED;
}

void meson_unipolar_set_brightness(struct led_classdev *led_cdev,
					enum led_brightness brightness)
{
	struct meson_unipolar_ctrl *dcon_led = container_of(led_cdev,
			struct meson_unipolar_ctrl, cdev);
	memset(dcon_led->color_data, brightness, dcon_led->led_num * COLOR_CHANNEL_NUM);
	meson_unipolar_ctrl_put_data(dcon_led);
	meson_unipolar_ctrl_xfer(dcon_led);
}

static int unipolar_ctrl_probe(struct platform_device *pdev)
{
	struct meson_unipolar_ctrl *dcon_led;
	struct resource *mem;
	int irq, ret = 0;

	LEDCON_DBG("unipolar led ctrl probe start\n");
	dcon_led = devm_kzalloc(&pdev->dev, sizeof(struct meson_unipolar_ctrl), GFP_KERNEL);
	if (!dcon_led)
		return -ENOMEM;

	dcon_led->dev = &pdev->dev;
	platform_set_drvdata(pdev, dcon_led);
	spin_lock_init(&dcon_led->lock);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	LEDCON_DBG("request mem start:0x%x, end:0x%x\n",
		mem->start, mem->end);
	dcon_led->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dcon_led->regs)) {
		dev_err(dcon_led->dev, "fail to get mem resources\n");
		return PTR_ERR(dcon_led->regs);
	}
	LEDCON_DBG("get mem start:0x%x, end:0x%x\n",
		mem->start, mem->end);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "can't find IRQ\n");
		return irq;
	}
	ret = devm_request_irq(&pdev->dev, irq, meson_unipolar_led_irq,
							IRQF_TRIGGER_RISING, NULL, dcon_led);
	if (ret < 0) {
		dev_err(&pdev->dev, "can't request IRQ\n");
		return ret;
	}

	dcon_led->irq = irq;

	ret = device_property_read_u32(&pdev->dev, "led_number", &dcon_led->led_num);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failure to get led num = %d\n", ret);
		return ret;
	}
	LEDCON_DBG("get led num:%u\n", dcon_led->led_num);
	if (dcon_led->led_num > LED_MAX_NUM) {
		dev_err(&pdev->dev, "erro, LED num over than LED_MAX_NUM:%d\n", LED_MAX_NUM);
		return -EINVAL;
	}
	dcon_led->color_data = devm_kzalloc(&pdev->dev,
		dcon_led->led_num * COLOR_CHANNEL_NUM, GFP_KERNEL);
	if (!dcon_led->color_data)
		return -ENOMEM;
	/* Disable the interrupt so that the system can enter low-power mode */
	// disable_irq(dcon_led->irq);
	meson_unipolar_ctrl_init(dcon_led);
	dcon_led->cdev.name = MESON_UNIPOLAR_CTRL_CDEV_NAME;
	dcon_led->cdev.brightness = 0;
	dcon_led->cdev.max_brightness = 255;
	dcon_led->cdev.brightness_set = meson_unipolar_set_brightness;
	ret = led_classdev_register(dcon_led->dev, &dcon_led->cdev);
	if (ret) {
		dev_err(dcon_led->dev,
			"unable to register led ret=%d\n", ret);
		return ret;
	}
	ret = sysfs_create_group(&dcon_led->cdev.dev->kobj,
				 &meson_unipolar_ctrl_attribute_group);
	if (ret) {
		dev_err(dcon_led->dev, "unable to create unipolar led sysfs! ret = %d\n",
			ret);
		led_classdev_unregister(&dcon_led->cdev);
		return ret;
	}

	return 0;
}

static int unipolar_ctrl_remove(struct platform_device *pdev)
{
	struct meson_unipolar_ctrl *dcon_led = platform_get_drvdata(pdev);

	pr_info("%s enter\n", __func__);
	sysfs_remove_group(&dcon_led->cdev.dev->kobj,
				&meson_unipolar_ctrl_attribute_group);
	led_classdev_unregister(&dcon_led->cdev);

	return 0;
}

static const struct of_device_id unipolar_ctrl_table[] = {
	{
		.compatible = "amlogic,led_unipolar_ctrl",
	},
	{},
};

MODULE_DEVICE_TABLE(of, unipolar_ctrl_table);

static struct platform_driver meson_led_unipolar_ctrl = {
	.probe = unipolar_ctrl_probe,
	.remove = unipolar_ctrl_remove,
	.driver = {
		.name = "meson_led_unipolar_ctrl",
		.of_match_table = unipolar_ctrl_table,
	},
};

int __init led_unipolar_ctrl_init(void)
{
	int ret;

	ret = platform_driver_register(&meson_led_unipolar_ctrl);
	return ret;
}

void __exit led_unipolar_ctrl_exit(void)
{
	platform_driver_unregister(&meson_led_unipolar_ctrl);
}
