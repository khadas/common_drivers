/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * flat buffer api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _AML_FLATBUF_API_
#define _AML_FLATBUF_API_

/* include <stddef.h> */
#include <linux/stddef.h>
/* #include <stdint.h> */
#include <linux/types.h>
/* #include <stdbool.h> */

/** A flag that specifies that the flat buffer support write operation.
 */
#define FLATBUF_FLAG_WR 0x00000001

/** A flag that specifies that the flat buffer support read operation.
 */
#define FLATBUF_FLAG_RD 0x10000000

enum FLATBUF_CH_TYPE {
	FLATBUF_CH_ARM2DSPA,
	FLATBUF_CH_ARM2DSPB,
	FLATBUF_CH_MAX,
};

struct flatbuffer_config {
	/*0 - HiFi4A, 1 - HiFi4B*/
	enum FLATBUF_CH_TYPE phy_ch;
	/*Size of CC buffer*/
	size_t size;
};

/* typedef void *AML_FLATBUF_HANDLE; */
#define AML_FLATBUF_HANDLE void *

/**
 * Reset flat buffer.
 *
 * @param[in] flat buffer handle
 *
 * @param[in] set to true when user want to zero out buffer
 *
 * @return a flat buffer handler if success, otherwise return 0
 */
void aml_flatbuf_reset(AML_FLATBUF_HANDLE h_fbuf, bool clear);

/**
 * Create a flat buffer.
 * The handler is associated with a buffer string id,
 * If buffer string id already exists, return the already created handler.
 *
 * @param[in] buffer string id
 *
 * @param[in] flags, see the macro defined by FLATBUF_FLAG_XXX
 *
 * @param[in] flat buffer config, this param is ignored if buf_id already exists
 *
 * @return a flat buffer handler if success, otherwise return 0
 */
AML_FLATBUF_HANDLE aml_flatbuf_create(const char *buf_id, int flags,
				      struct flatbuffer_config *config);

/**
 * Destroy a flat buffer
 *
 * @param[in] flat buffer handler
 *
 * @return
 */
void aml_flatbuf_destroy(AML_FLATBUF_HANDLE h_fbuf);

/**
 * Read data from flat buffer.
 *
 * The API blocks till all bytes are read, when ms_timeout is equal to -1.
 * Time out mechanism is enabled when ms_timeout is not equal to -1.
 * When time out is enabled, all bytes or part of bytes are read before time out.
 *
 * @param[in] flat buffer handler
 *
 * @param[out] read data into this buffer
 *
 * @param[in] size in bytes, to be read
 *
 * @param[in] time out in micro seconds
 *
 * @return real size of the read, -1 means failure of the call
 */
int aml_flatbuf_read(AML_FLATBUF_HANDLE h_fbuf, void *buf, size_t size, int ms_timeout);

/**
 * Write data to flat buffer
 *
 * The API blocks till all bytes are written when ms_timeout is equal to -1
 * Time out mechanism is enabled when ms_timeout is not equal to -1,
 * When time out is enabled, all bytes or part of bytes are written before time out
 *
 * @param[in] flat buffer handler
 *
 * @param[in] write data into this buffer
 *
 * @param[in] size in bytes, to be written
 *
 * @param[in] time out in micro seconds
 *
 * @return real size of the write, -1 means failure of the call
 */
int aml_flatbuf_write(AML_FLATBUF_HANDLE h_fbuf, const void *buf, size_t size, int ms_timeout);

/**
 * Get flat buffer fullness
 * .
 * @param[in] flat buffer handler
 *
 * @return flat buffer fullness in bytes
 */
size_t aml_flatbuf_getfullness(AML_FLATBUF_HANDLE h_fbuf);

/**
 * Get flat buffer space
 *
 * @param[in] flat buffer handler
 *
 * @return flat buffer space in bytes
 */
size_t aml_flatbuf_getspace(AML_FLATBUF_HANDLE h_fbuf);

#endif
