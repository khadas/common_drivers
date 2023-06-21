// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */
#ifndef __I2C_API__
#define __I2C_API__
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>

static inline int i2c_read_a16d8(struct i2c_client *client, u8 slave, u16 addr, u8 *value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[4];
	struct i2c_msg msgs[2];

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;

	msgs[0].addr  = slave;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	msgs[1].addr  = slave;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(client->adapter, msgs, msg_count);
		if (ret == msg_count)
			break;
	}

	if (ret != msg_count)
		dev_info(&client->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = buf[0] & 0xff;

	return 0;
}

static inline int i2c_write_a16d8(struct i2c_client *client, u8 slave, u16 addr, u8 value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[3];
	struct i2c_msg msgs[1];

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;
	buf[2] = value & 0xff;

	msgs[0].addr = slave;
	msgs[0].flags = 0;
	msgs[0].len = 3;
	msgs[0].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(client->adapter, msgs, msg_count);
		if (ret == msg_count) {
			break;
		}
	}

	if (ret != msg_count)
		dev_info(&client->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ((ret != msg_count) ? -1 : 0);
}

static inline int i2c_read_a16d16(struct i2c_client *client, u8 slave, u16 addr, u16 *value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[4];
	struct i2c_msg msgs[2];

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;

	msgs[0].addr  = slave;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	msgs[1].addr  = slave;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(client->adapter, msgs, msg_count);
		if (ret == msg_count)
			break;
	}

	if (ret != msg_count)
		dev_info(&client->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = ((buf[0] << 8) & 0xff00) | (buf[1] & 0xff);

	return 0;
}

static inline int i2c_write_a16d16(struct i2c_client *client, u8 slave, u16 addr, u16 value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[3];
	struct i2c_msg msgs[1];

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;
	buf[2] = (value >> 8) & 0xff;
	buf[3] = value & 0xff;

	msgs[0].addr = slave;
	msgs[0].flags = 0;
	msgs[0].len = 4;
	msgs[0].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(client->adapter, msgs, msg_count);
		if (ret == msg_count) {
			break;
		}
	}

	if (ret != msg_count)
		dev_info(&client->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ((ret != msg_count) ? -1 : 0);
}

static inline int i2c_read_a8d8(struct i2c_client *client, u8 slave, u8 addr, u8 *value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[4];
	struct i2c_msg msgs[2];

	buf[0] = addr & 0xff;

	msgs[0].addr  = slave;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = buf;

	msgs[1].addr  = slave;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(client->adapter, msgs, msg_count);
		if (ret == msg_count)
			break;
	}

	if (ret != msg_count)
		dev_info(&client->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = buf[0] & 0xff;

	return 0;
}

static inline int i2c_write_a8d8(struct i2c_client *client, u8 slave, u8 addr, u8 value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[2];
	struct i2c_msg msgs[1];

	buf[0] = addr & 0xff;
	buf[1] = value & 0xff;

	msgs[0].addr = slave;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(client->adapter, msgs, msg_count);
		if (ret == msg_count) {
			break;
		}
	}

	if (ret != msg_count)
		dev_info(&client->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ((ret != msg_count) ? -1 : 0);
}

#endif
