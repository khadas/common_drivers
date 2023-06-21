// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>

#include "sensor_drv.h"

struct amlsens *g_sensor[8];

static struct amlsens * sensor_get_ptr(struct i2c_client * client)
{
	int i = 0;
	for (i = 0; i < sizeof(g_sensor); i++) {
		if (g_sensor[i]->client == client)
			return g_sensor[i];
	}

	return NULL;
}

static int sensor_set_ptr(struct amlsens * sensor)
{
	int i = 0;
	for (i = 0; i < sizeof(g_sensor); i++) {
		if (g_sensor[i] == NULL) {
			g_sensor[i] = sensor;
			return 0;
		}
	}

	return -1;
}

static int sensor_id_detect(struct amlsens *sensor)
{
	int i = 0, ret = -1;
	struct sensor_subdev *subdev = NULL;

	for (i = 0; i < ARRAY_SIZE(aml_sensors); i++) {
		subdev = aml_sensors[i];
		if (subdev == NULL)
			return ret;

		subdev->sensor_power_on(sensor->dev, &sensor->gpio);

		ret = subdev->sensor_get_id(sensor->client);
		if (ret == 0) {
			sensor->sd_sdrv = subdev;
			return ret;
		}

		subdev->sensor_power_off(sensor->dev, &sensor->gpio);
	}

	return ret;
}

static int sensor_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct amlsens *sensor = sensor_get_ptr(client);

	sensor->sd_sdrv->sensor_power_suspend(dev);

	return 0;
}

static int sensor_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct amlsens *sensor = sensor_get_ptr(client);

	sensor->sd_sdrv->sensor_power_resume(dev);

	return 0;
}

static const struct dev_pm_ops sensor_pm_ops = {
	SET_RUNTIME_PM_OPS(sensor_power_suspend, sensor_power_resume, NULL)};

static int sensor_parse_power(struct amlsens *sensor)
{
	int rtn = 0;

	sensor->gpio.rst_gpio = devm_gpiod_get_optional(sensor->dev,
												"reset",
												 GPIOD_OUT_LOW);
	if (IS_ERR(sensor->gpio.rst_gpio))
	{
		rtn = PTR_ERR(sensor->gpio.rst_gpio);
		dev_err(sensor->dev, "Cannot get reset gpio: %d\n", rtn);
		goto err_return;
	}

	sensor->gpio.pwdn_gpio = devm_gpiod_get_optional(sensor->dev,
												"pwdn",
												GPIOD_OUT_LOW);
	if (IS_ERR(sensor->gpio.pwdn_gpio)) {
		rtn = PTR_ERR(sensor->gpio.pwdn_gpio);
		dev_err(sensor->dev, "Cannot get pwdn gpio: %d\n", rtn);
		return 0;
	}

	sensor->gpio.power_gpio = devm_gpiod_get_optional(sensor->dev,
												"pwr",
												GPIOD_OUT_LOW);
	if (IS_ERR(sensor->gpio.power_gpio)) {
		rtn = PTR_ERR(sensor->gpio.power_gpio);
		dev_info(sensor->dev, "Cannot get power gpio: %d\n", rtn);
		return 0;
	}

err_return:

	return rtn;
}

static int sensor_parse_endpoint(struct amlsens *sensor)
{
	int rtn = 0;
	struct fwnode_handle *endpoint = NULL;

	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(sensor->dev), NULL);
	if (!endpoint) {
		dev_err(sensor->dev, "Endpoint node not found\n");
		return -EINVAL;
	}

	rtn = v4l2_fwnode_endpoint_alloc_parse(endpoint, &sensor->ep);
	fwnode_handle_put(endpoint);
	if (rtn)
	{
		dev_err(sensor->dev, "Parsing endpoint node failed\n");
		rtn = -EINVAL;
		goto err_return;
	}

	/* Only CSI2 is supported for now */
	if (sensor->ep.bus_type != V4L2_MBUS_CSI2_DPHY)
	{
		dev_err(sensor->dev, "Unsupported bus type, should be CSI2\n");
		rtn = -EINVAL;
		goto err_free;
	}

	sensor->nlanes = sensor->ep.bus.mipi_csi2.num_data_lanes;
	if (sensor->nlanes != 2 && sensor->nlanes != 4)
	{
		dev_err(sensor->dev, "Invalid data lanes: %d\n", sensor->nlanes);
		rtn = -EINVAL;
		goto err_free;
	}
	dev_info(sensor->dev, "Using %u data lanes\n", sensor->nlanes);

	if (!sensor->ep.nr_of_link_frequencies)
	{
		dev_err(sensor->dev, "link-frequency property not found in DT\n");
		rtn = -EINVAL;
		goto err_free;
	}

	return rtn;

err_free:
	v4l2_fwnode_endpoint_free(&sensor->ep);
err_return:
	return rtn;
}

static int sensor_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct amlsens *sensor;
	int ret = -EINVAL;

	sensor = devm_kzalloc(dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->dev = dev;
	sensor->client = client;

	ret = sensor_parse_endpoint(sensor);
	if (ret) {
		dev_err(sensor->dev, "Error parse endpoint\n");
		goto return_err;
	}

	ret = sensor_parse_power(sensor);
	if (ret) {
		dev_err(sensor->dev, "Error parse power ctrls\n");
		goto free_err;
	}

	ret = sensor_id_detect(sensor);
	if (ret) {
		dev_err(sensor->dev, "None sensor detect\n");
		goto free_err;
	}

	/* Power on the device to match runtime PM state below */
	dev_info(dev, "bef get id. pwdn - 0, reset - 1\n");

	sensor->sd_sdrv->sensor_init(client, (void *)sensor);

	v4l2_fwnode_endpoint_free(&sensor->ep);

	sensor_set_ptr(sensor);

	return 0;

free_err:
	v4l2_fwnode_endpoint_free(&sensor->ep);
return_err:
	return ret;
}

static int sensor_remove(struct i2c_client *client)
{
	struct amlsens *sensor = sensor_get_ptr(client);

	if (sensor) {
		sensor->sd_sdrv->sensor_deinit(client);
		sensor = NULL;
	}

	return 0;
}

static const struct of_device_id sensor_of_match[] = {
	{.compatible = "amlogic, sensor"},
	{/* sentinel */}};
MODULE_DEVICE_TABLE(of, sensor_of_match);

static struct i2c_driver sensor_i2c_driver = {
	.probe_new = sensor_probe,
	.remove = sensor_remove,
	.driver = {
		.name = "amlsens",
		.pm = &sensor_pm_ops,
		.of_match_table = of_match_ptr(sensor_of_match),
	},
};

module_i2c_driver(sensor_i2c_driver);

MODULE_DESCRIPTION("Amlogic Image Sensor Driver");
MODULE_AUTHOR("Amlogic Inc.");
MODULE_LICENSE("GPL v2");
