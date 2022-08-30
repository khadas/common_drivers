/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * amlogic wake up engine(AWE) API.
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _AMLOGIC_WAKE_API_H_
#define _AMLOGIC_WAKE_API_H_

/* #include <stdint.h> */
#include <linux/types.h>
#include "rpc_client_shm.h"
#define AWE_MAX_IN_CHANS 4
#define AWE_MAX_OUT_CHANS 2

/* typedef struct _AWE AWE; */
struct AWE;

enum AWE_INPUT_MODE {
	/*dsp feed input*/
	AWE_DSP_INPUT_MODE,
	/*user application feed input*/
	AWE_USER_INPUT_MODE
};

union AWE_PARA {
	s32 in_samp_rate;
	s32 in_samp_bits;
	struct {
		s32 num_samp_rates;
		s32 support_samp_rates[4];
	};
	struct {
		s32 num_samp_bits;
		s32 support_samp_bits[4];
	};
	s32 mic_channels;
	s32 ref_channels;
	s32 out_channels;
	u32 free_run_mode;
	enum AWE_INPUT_MODE input_mode;
	u32 is_bypass_input;
} __packed;

/**
 *  wake event payload structure
 */
struct AWE_WAKE {
	const char *word; /* Text of wake/hot word */
	const char *data; /* Samples of wake/hot word */
	size_t size;      /* Total bytes of samples */
	float angle;      /* DOA result */
	float score;      /* Wake confidence */
};

/*
 */
enum AWE_PARA_ID {
	/* static params */
	/* static params can only be set before AWE_Open */
	AWE_PARA_INPUT_MODE,
	AWE_PARA_IN_SAMPLE_RATE,
	AWE_PARA_IN_SAMPLE_BITS,
	AWE_PARA_MIC_IN_CHANNELS,
	AWE_PARA_REF_IN_CHANNELS,
	AWE_PARA_OUT_CHANNELS,
	/* dynamic params */
	AWE_PARA_FREE_RUN_MODE,
	/* getting only params */
	AWE_PARA_WAKE_UP_SCORE,
	AWE_PARA_SUPPORT_SAMPLE_RATES,
	AWE_PARA_SUPPORT_SAMPLE_BITS,
	AWE_PARA_BYPASS_INPUT,
};

enum AWE_RET {
	AWE_RET_OK = 0,
	AWE_RET_ERR_NO_MEM = -1,
	AWE_RET_ERR_NOT_SUPPORT = -2,
	AWE_RET_ERR_NULL_POINTER = -3,
};

enum AWE_DATA_TYPE {
	AWE_DATA_TYPE_INVALID,
	/* AWE optimized for ASR */
	AWE_DATA_TYPE_ASR,
	/* AWE optimized for VAD */
	AWE_DATA_TYPE_VAD,
	/* AWE optimized for VOIP */
	AWE_DATA_TYPE_VOIP,
	AWE_DATA_TYPE_MAX
};

enum AWE_EVENT_TYPE {
	AWE_EVENT_TYPE_INVALID,
	/* AWE event for local VAD */
	AWE_EVENT_TYPE_VAD,
	/* AWE event for wake detected, payload is `AWE_WAKE` */
	AWE_EVENT_TYPE_WAKE,
	/* AWE event for hot word detected, payload is `AWE_WAKE` */
	AWE_EVENT_TYPE_HOTWORD,
	AWE_EVENT_TYPE_MAX
};

/**
 * Create and initialize AWE instance
 *
 * @param[out] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_create(struct AWE **awe);

/**
 * Destroy AWE instance and all resources allocated for AWE instance.
 *
 * @param[in] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_destroy(struct AWE *awe);

/**
 * Enable AWE instance
 *
 * All static params should be configured through AML_AWE_SetParam
 * before this call. AWE is moved to working status after this call
 *
 * @param[in] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_open(struct AWE *awe);

/**
 * Disable AWE instance
 *
 * The API AML_AWE_Process can not working after this call.
 *
 * @param[in] AWE instance handler
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_close(struct AWE *awe);

/**
 * Configure AWE parameter
 *
 * Static params is configured before AML_AWE_Open.
 * Dynamic params can be configured after AML_AWE_Open.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] identification of a parameter see AWE_PARA_ID
 *
 * @param[in] value of a parameter, associated with para_id,
 * see AWE_PARA
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_setparam(struct AWE *awe, enum AWE_PARA_ID para_id, union AWE_PARA *para);

/**
 * Obtain AWE parameter
 *
 * @param[in] AWE instance handler
 *
 * @param[in] identification of a parameter see AWE_PARA_ID
 *
 * @param[out] value of a parameter, associated with para_id,
 * see AWE_PARA
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_getparam(struct AWE *awe, enum AWE_PARA_ID para_id, union AWE_PARA *para);

/**
 * Synchronous AWE data processing entry.
 * Application feed pcm here, application get output
 * and wake up result here.
 * Note: This API is invalid when AWE input mode is configured
 * as AWE_DSP_INPUT_MODE
 *
 * @param[in] AWE instance handler
 *
 * @param[in] Array of buffers. A member of
 * this array represent a input pcm stream. The buffer
 * is filled with mic or reference pcm by application.
 * The pcm is in none interleave format, mic0|mic1|ref0|ref1
 * Max supported input streams see AWE_MAX_IN_CHANS.
 *
 * @param[in/out] Length in bytes of a input stream buffer. Return
 * remained pcm in bytes after the call.
 *
 * @param[out] Array of buffers. A member of
 * this array represent a output pcm stream. The buffer
 * is filled with processed pcm by AWE. The pcm is in none interleave
 * format, out0|out1.
 * Max supported output stream numbers see AWE_MAX_OUT_CHANS
 *
 * @param[in/out] Space in bytes of a output stream buffer. Return processed
 * pcm length in bytes.
 *
 * @param[out] A flag indicates whether wake up words is spotted
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_process(struct AWE *awe, void *in[], s32 *in_len_in_byte, void *out[],
			     s32 *out_len_in_byte, u32 *is_waked);

/**
 * Asynchronous data processing entry
 *
 * Application feed pcm here.
 * Application obtain processed pcm through AML_AWE_DataHandler
 * Application process wake up word detect in AML_AWE_EventHandler
 * Note: This API is invalid when AWE input mode is configured as
 * AWE_DSP_INPUT_MODE.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] Pcm buffer, no interleaved format mic0|mic1|ref0|ref1
 *
 * @param[in] size in total bytes
 *
 * @return AWE_RET_OK if successful, otherwise see AWE_RET
 */
enum AWE_RET aml_awe_pushbuf(struct AWE *awe, const char *data, size_t size);

/**
 * AWE processed data handler
 *
 * AEC processed pcm can be obtained here. The implementation should copy
 * the data and return ASAP (< 1ms), otherwise the worker thread would be blocked.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE data processing type, see AWE_DATA_TYPE
 *
 * @param[in] Pcm processed data, mono channel
 *
 * @param[in] size in bytes per channel
 *
 * @param[in] user_data User-defined data
 */
typedef void (*aml_awe_datahandler)(struct AWE *awe, const enum AWE_DATA_TYPE type,
				    char *out, size_t size, void *user_data);

/**
 * AWE event handler
 *
 * Notify application a certain event appear, for example, wakeup words detect.
 * The implementation should handle event and return ASAP (< 1ms),
 * otherwise the worker thread would be blocked.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE event type, see AWE_EVENT_TYPE
 *
 * @param[in] Event code, does not define so far
 *
 * @param[in] Event payload
 *
 * @param[in] user_data User-defined data
 */
typedef void (*aml_awe_eventhandler)(struct AWE *awe, const enum AWE_EVENT_TYPE type, s32 code,
				     const void *payload, void *user_data);
/**
 * Add a data handler associated with data type
 *
 * Multiple data handlers could be added.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE data processing type, see AWE_DATA_TYPE
 *
 * @param[in] callback function to handle data
 *
 * @param[in] user_data User-defined data
 */
enum AWE_RET aml_awe_adddatahandler(struct AWE *awe, const enum AWE_DATA_TYPE type,
				    aml_awe_datahandler handler,
				    void *user_data);
/**
 * Add a event handler associated with event type
 *
 * Multiple v handlers could be added.
 *
 * @param[in] AWE instance handler
 *
 * @param[in] AWE event type, see AWE_EVENT_TYPE
 *
 * @param[in] callback function to handle event
 *
 * @param[in] user_data User-defined data
 */
enum AWE_RET aml_awe_addeventhandler(struct AWE *awe, const enum AWE_EVENT_TYPE type,
				     aml_awe_eventhandler handler,
				     void *user_data);
#endif /* _AMLOGIC_WAKE_ENGINE_H_ */
