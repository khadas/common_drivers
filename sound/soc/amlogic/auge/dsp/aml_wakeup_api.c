// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * amlogic wake up engine api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 *         Yang Liu <Yang.Liu@amlogic.com>
 * Version:
 * - 0.1        init
 * - 0.2        port to linux kernel
 */

/* #include <string.h> */
/* #include <stdlib.h> */
#include <linux/slab.h>
/* #include <stdio.h> */
#include <linux/printk.h>
/* #include <semaphore.h> */
#include <linux/semaphore.h>
/* #include <pthread.h> */
#include <linux/kthread.h>
#include "ipc_cmd_type.h"
#include "rpc_client_aipc.h"
#include "rpc_client_vsp.h"
#include "rpc_client_shm.h"
#include "aml_wakeup_api.h"
#include "generic_macro.h"

#define VOICE_CHUNK_MS 20
#define VOICE_CHUNK_NUM 50

enum AWE_STATUS {
	/* Set to IDLE in Create*/
	AWE_STATUS_IDLE,
	/* Set to EXECUTE in Open*/
	AWE_STATUS_EXECUTE,
	/* Set to FREE_RUN in low power*/
	AWE_STATUS_FREE_RUN,
} AWE_STATUS;

struct AWE {
	AML_VSP_HANDLE		h_vsp;
	AML_MEM_HANDLE		h_param;
	size_t			param_size;
	AML_MEM_HANDLE		h_input;
	size_t			input_size;
	AML_MEM_HANDLE		h_output;
	size_t			output_size;
	s32			in_sample_rate;
	s32			in_sample_bit;
	s32			mic_channels;
	s32			ref_channels;
	s32			out_channels;
	s32			input_mode;
	aml_awe_datahandler	awe_data_handler_func[AWE_DATA_TYPE_MAX];
	void			*awe_data_handler_userdata[AWE_DATA_TYPE_MAX];
	aml_awe_eventhandler	awe_event_handler_func[AWE_EVENT_TYPE_MAX];
	void			*awe_event_handler_userdata[AWE_EVENT_TYPE_MAX];
	s32			work_thread_exit;
	AML_MEM_HANDLE		h_in[AWE_MAX_IN_CHANS];
	s32			in_buf_len;
	s32			in_buf_samples;
	AML_MEM_HANDLE		h_out[AWE_MAX_OUT_CHANS];
	s32			out_buf_len;
	char			*user_fill;
	u32			user_fill_rd;
	u32			user_fill_wr;
	u32			user_fill_size;
	struct semaphore	*user_fill_sem;
	/* pthread_t		work_thread; */
	struct task_struct	*work_thread;
	u32			status;
	u32			kick_free_run;
};

struct aml_vsp_awe_process_param_in {
	s32	in_channels;
	s32	in_len;
	u64	chan_in[AWE_MAX_IN_CHANS];
	s32	out_channels;
	s32	out_len;
	u64	chan_out[AWE_MAX_OUT_CHANS];
} __packed;

struct aml_vsp_awe_process_param_out {
	s32	in_len;
	s32	out_len;
	u32	is_waked;
} __packed;

/*for PCM_OUT*/
static u32 aml_awe_ring_buf_space(u32 size,
				  u32 wr, u32 rd)
{
	return (wr > rd) ? (size + rd - wr - 1) :
		(rd - wr - 1);
}

static enum AWE_RET internal_aml_awe_process(struct AWE *awe, AML_MEM_HANDLE in[],
					     s32 *in_len_in_byte, AML_MEM_HANDLE out[],
					     s32 *out_len_in_byte, u32 *is_waked)
{
	enum AWE_RET ret = AWE_RET_OK;
	struct aml_vsp_awe_process_param_in *p_param_in;
	struct aml_vsp_awe_process_param_out *p_param_out;
	size_t output_size;
	int i;

	if (!awe) {
		pr_info("Invalid param %s\n", __func__);
		return AWE_RET_ERR_NULL_POINTER;
	}

	p_param_in = aml_mem_getvirtaddr(awe->h_input);
	p_param_out = aml_mem_getvirtaddr(awe->h_output);
	output_size = awe->output_size;

	if (awe->input_mode == AWE_USER_INPUT_MODE) {
		p_param_in->in_len = *in_len_in_byte;
		p_param_in->in_channels = awe->mic_channels + awe->ref_channels;
		if (p_param_in->in_channels <= AWE_MAX_IN_CHANS) {
			int i;

			for (i = 0; i < p_param_in->in_channels; i++)
				p_param_in->chan_in[i] = CAST_PTR64(aml_mem_getphyaddr(in[i]));
		} else {
			pr_info("The input channel number is out of AWE capability: mic:%d, ref:%d\n",
				awe->mic_channels, awe->ref_channels);
			return AWE_RET_ERR_NOT_SUPPORT;
		}
	} else {
		p_param_in->in_len = 0;
		p_param_in->in_channels = 0;
	}

	p_param_in->out_len = *out_len_in_byte;
	p_param_in->out_channels = awe->out_channels;
	if (p_param_in->out_channels <= AWE_MAX_OUT_CHANS) {
		for (i = 0; i < p_param_in->out_channels; i++)
			p_param_in->chan_out[i] = CAST_PTR64(aml_mem_getphyaddr(out[i]));
	} else {
		pr_info("The output channel number is out of AWE capability: out:%d\n",
			awe->out_channels);
		return AWE_RET_ERR_NOT_SUPPORT;
	}

	aml_mem_clean(awe->h_input, awe->input_size);
	ret = aml_vsp_process(awe->h_vsp, aml_mem_getphyaddr(awe->h_input), awe->input_size,
			      aml_mem_getphyaddr(awe->h_output), &output_size);
	aml_mem_invalidate(awe->h_output, output_size);
	for (i = 0; i < p_param_in->out_channels; i++) {
		/*
		 * pr_info("inv i=%d out=%llx size=%zu\n",
		 * i, p_param_in->chan_out[i], p_param_out->out_len);
		 */
		aml_mem_invalidate(p_param_in->chan_out[i], p_param_out->out_len);
	}
	*in_len_in_byte = p_param_out->in_len;
	*out_len_in_byte = p_param_out->out_len;
	*is_waked = p_param_out->is_waked;
	return ret;
}

int awe_thread_process_data(void *data)
{
	int i;
	struct AWE *awe;
	char *vir_out[AWE_MAX_OUT_CHANS];
	u32 is_waked;
	s32 in_len;
	s32 out_len;

	awe = (struct AWE *)data;
	while (!awe->work_thread_exit) {
		if (awe->input_mode != AWE_DSP_INPUT_MODE)
			pr_info("Impossible, invalid input mode:%d\n", awe->input_mode);
		if (awe->kick_free_run == 1) {
			awe->kick_free_run = 0;
			/*
			 * sem_wait(&awe->user_fill_sem);
			 * down(&awe->user_fill_sem);
			 */
		}
		is_waked = 0;
		in_len = 0;
		out_len = awe->out_buf_len;
		internal_aml_awe_process(awe, awe->h_in, &in_len,
					 awe->h_out, &out_len, &is_waked);
		for (i = 0; i < awe->out_channels; i++)
			vir_out[i] = (char *)aml_mem_getvirtaddr(awe->h_out[i]);
		/*throw out data*/
		if (!awe->awe_data_handler_func[AWE_DATA_TYPE_ASR]) {
			pr_info("Do not install ASR data handler callback\n");
			return 0;
		}
		awe->awe_data_handler_func[AWE_DATA_TYPE_ASR]
			(awe, AWE_DATA_TYPE_ASR, vir_out[0], out_len,
			 awe->awe_data_handler_userdata[AWE_DATA_TYPE_ASR]);

		/*throw out voip data*/
		if (!awe->awe_data_handler_func[AWE_DATA_TYPE_VOIP]) {
			pr_info("Do not install VOIP data handler callback\n");
			return 0;
		}
		awe->awe_data_handler_func[AWE_DATA_TYPE_VOIP]
			(awe, AWE_DATA_TYPE_VOIP, vir_out[1], out_len,
			 awe->awe_data_handler_userdata[AWE_DATA_TYPE_VOIP]);

		/*throw out event*/
		if (is_waked) {
			if (!awe->awe_event_handler_func[AWE_EVENT_TYPE_WAKE]) {
				pr_info("Do not install wake up event handler callback\n");
				return 0;
			}
			awe->awe_event_handler_func[AWE_EVENT_TYPE_WAKE]
				(awe, AWE_EVENT_TYPE_WAKE, 0, NULL,
				 awe->awe_event_handler_userdata[AWE_EVENT_TYPE_WAKE]);
		}
	}
	return 0;
}

enum AWE_RET aml_awe_create(struct AWE **awe)
{
	struct AWE *pawe = NULL;

	if (*awe) {
		pr_info("Dangerous, *awe should be initialized as NULL, *awe=%p\n", *awe);
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	pawe = kmalloc(sizeof(*pawe), GFP_KERNEL);
	if (!pawe)
		return AWE_RET_ERR_NO_MEM;
	memset(pawe, 0, sizeof(struct AWE));
	pawe->h_input = aml_mem_allocate(sizeof(struct aml_vsp_awe_process_param_in));
	pawe->input_size = sizeof(struct aml_vsp_awe_process_param_in);
	pawe->h_output = aml_mem_allocate(sizeof(struct aml_vsp_awe_process_param_out));
	pawe->output_size = sizeof(struct aml_vsp_awe_process_param_out);
	pawe->h_param = aml_mem_allocate(sizeof(union AWE_PARA));
	pawe->param_size = sizeof(union AWE_PARA);
	pawe->h_vsp = aml_vsp_init("AML.VSP.AWE", NULL, 0);

	if (pawe->h_param && pawe->h_input && pawe->h_output && pawe->h_vsp) {
		*awe = pawe;
		pr_debug("Create AWE success: h_vsp=%lx h_param=%lx h_input=%lx h_output=%lx\n",
			 (unsigned long)pawe->h_vsp, pawe->h_param, pawe->h_input, pawe->h_output);
		(*awe)->status = AWE_STATUS_IDLE;
		return AWE_RET_OK;
	}
	pr_info("Allocate AWE failed: h_vsp=%lx h_param=%lx h_input=%lx h_output=%lx\n",
		(unsigned long)pawe->h_vsp, pawe->h_param, pawe->h_input, pawe->h_output);
	if (pawe->h_param)
		aml_mem_free(pawe->h_param);
	if (pawe->h_input)
		aml_mem_free(pawe->h_input);
	if (pawe->h_output)
		aml_mem_free(pawe->h_output);
	if (pawe->h_vsp)
		aml_vsp_deinit(pawe->h_vsp);
	kfree(pawe);
	return AWE_RET_ERR_NO_MEM;
}

enum AWE_RET aml_awe_destroy(struct AWE *awe)
{
	if (awe->h_output) {
		aml_mem_free(awe->h_output);
		awe->h_output = 0;
	}
	if (awe->h_input) {
		aml_mem_free(awe->h_input);
		awe->h_input = 0;
	}
	if (awe->h_param) {
		aml_mem_free(awe->h_param);
		awe->h_param = 0;
	}
	if (awe->h_vsp) {
		aml_vsp_deinit(awe->h_vsp);
		awe->h_vsp = 0;
	}
	kfree(awe);
	return 0;
}

enum AWE_RET aml_awe_open(struct AWE *awe)
{
	int i;
	enum AWE_RET ret;
	union AWE_PARA *para;

	if (awe->status != AWE_STATUS_IDLE) {
		pr_err("%s:%d invalid status %d\n", __func__, __LINE__,
		       awe->status);
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	para = (union AWE_PARA *)aml_mem_getvirtaddr(awe->h_param);

	aml_vsp_getparam(awe->h_vsp, AWE_PARA_MIC_IN_CHANNELS,
			 aml_mem_getphyaddr(awe->h_param), awe->param_size);
	aml_mem_invalidate(awe->h_param, awe->param_size);
	awe->mic_channels = para->mic_channels;

	aml_vsp_getparam(awe->h_vsp, AWE_PARA_REF_IN_CHANNELS,
			 aml_mem_getphyaddr(awe->h_param), awe->param_size);
	aml_mem_invalidate(awe->h_param, awe->param_size);
	awe->ref_channels = para->ref_channels;

	aml_vsp_getparam(awe->h_vsp, AWE_PARA_OUT_CHANNELS,
			 aml_mem_getphyaddr(awe->h_param), awe->param_size);
	aml_mem_invalidate(awe->h_param, awe->param_size);
	awe->out_channels = para->out_channels;

	aml_vsp_getparam(awe->h_vsp, AWE_PARA_IN_SAMPLE_RATE,
			 aml_mem_getphyaddr(awe->h_param), awe->param_size);
	aml_mem_invalidate(awe->h_param, awe->param_size);
	awe->in_sample_rate = para->in_samp_rate;

	aml_vsp_getparam(awe->h_vsp, AWE_PARA_IN_SAMPLE_BITS,
			 aml_mem_getphyaddr(awe->h_param), awe->param_size);
	aml_mem_invalidate(awe->h_param, awe->param_size);
	awe->in_sample_bit = para->in_samp_bits;

	aml_vsp_getparam(awe->h_vsp, AWE_PARA_INPUT_MODE,
			 aml_mem_getphyaddr(awe->h_param), awe->param_size);
	aml_mem_invalidate(awe->h_param, awe->param_size);
	awe->input_mode = para->input_mode;
	pr_debug("Open AWE: SampleRate=%d Bitdepth=%d input_mode=%d channels mic=%d ref=%d out=%d\n",
		 awe->in_sample_rate, awe->in_sample_bit, awe->input_mode,
		 awe->mic_channels, awe->ref_channels, awe->out_channels);

	ret = aml_vsp_open(awe->h_vsp);
	if (ret != AWE_RET_OK) {
		pr_info("Open hifi awe failed:%d\n", ret);
		goto table_failure_handling;
	}

	awe->in_buf_samples = awe->in_sample_rate * VOICE_CHUNK_MS / 1000;
	awe->in_buf_len = (awe->in_sample_bit >> 3) * awe->in_buf_samples;
	for (i = 0; i < (awe->mic_channels + awe->ref_channels); i++) {
		awe->h_in[i] = aml_mem_allocate(awe->in_buf_len);
		if (!awe->h_in[i]) {
			pr_info("Failed to allocate input work buf, ch:%d", i);
			goto table_failure_handling;
		}
	}

	awe->out_buf_len = awe->in_buf_len * 4;
	for (i = 0; i < awe->out_channels; i++) {
		awe->h_out[i] = aml_mem_allocate(awe->out_buf_len);
		if (!awe->h_out[i]) {
			pr_info("Failed to allocate output work buf, ch:%d", i);
			goto table_failure_handling;
		}
	}

	awe->user_fill_size = VOICE_CHUNK_NUM *
		(awe->mic_channels + awe->ref_channels) * awe->in_buf_len;
	/*one more chunk here is for handling wrap around*/
	awe->user_fill = kmalloc(awe->user_fill_size +
				 (awe->mic_channels + awe->ref_channels) * awe->in_buf_len,
				 GFP_KERNEL);
	awe->user_fill_rd = 0;
	awe->user_fill_wr = 0;
	if (!awe->user_fill) {
		pr_info("Failed to allocate user fill ring buf\n");
		goto table_failure_handling;
	}

	/*
	 * ret = sem_init(&awe->user_fill_sem, 0, 0);
	 * sema_init(&awe->user_fill_sem, 0);
	 */
	if (ret != 0) {
		pr_info("create user filling sem error. %d\n", ret);
		goto table_failure_handling;
	}
	awe->work_thread_exit = 0;
	awe->kick_free_run = 0;
	/* ret = pthread_create(&awe->work_thread, NULL, awe_thread_process_data, (void *)awe); */
	awe->work_thread = kthread_create(awe_thread_process_data, (void *)awe,
					  "awe task");
	if (ret != 0) {
		pr_info("create working thread error. %d\n", ret);
		goto table_failure_handling;
	}
	/* pthread_setname_np(awe->work_thread, "awe_thread_process_data"); */
	wake_up_process(awe->work_thread);

	awe->status = AWE_STATUS_EXECUTE;
	goto table_end;
table_failure_handling:
	awe->work_thread_exit = 1;
	/* sem_post(&awe->user_fill_sem); */
	/* up(&awe->user_fill_sem); */
	/* pthread_join(awe->work_thread, NULL); */
	kthread_stop(awe->work_thread);
	/* sem_destroy(&awe->user_fill_sem); */
	kfree(awe->user_fill);

	for (i = 0; i < (awe->mic_channels + awe->ref_channels); i++)
		if (awe->h_in[i])
			aml_mem_free(awe->h_in[i]);

	for (i = 0; i < awe->out_channels; i++)
		if (awe->h_out[i])
			aml_mem_free(awe->h_out[i]);
table_end:
	return ret;
}

enum AWE_RET aml_awe_close(struct AWE *awe)
{
	int i;
	enum AWE_RET ret;

	ret = AWE_RET_OK;

	if (awe->status != AWE_STATUS_EXECUTE) {
		pr_err("%s:%d invalid status %d\n", __func__, __LINE__,
		       awe->status);
		return AWE_RET_ERR_NOT_SUPPORT;
	}

	awe->work_thread_exit = 1;
	/* sem_post(&awe->user_fill_sem); */
	/* up(&awe->user_fill_sem); */
	/* pthread_join(awe->work_thread, NULL); */
	kthread_stop(awe->work_thread);

	for (i = 0; i < (awe->mic_channels + awe->ref_channels); i++)
		if (awe->h_in[i])
			aml_mem_free(awe->h_in[i]);

	for (i = 0; i < awe->out_channels; i++)
		if (awe->h_out[i])
			aml_mem_free(awe->h_out[i]);

	kfree(awe->user_fill);
	if (awe->h_vsp)
		ret = aml_vsp_close(awe->h_vsp);
	/* sem_destroy(&awe->user_fill_sem); */
	return ret;
}

enum AWE_RET aml_awe_setparam(struct AWE *awe, enum AWE_PARA_ID para_id, union AWE_PARA *para)
{
	enum AWE_RET ret = AWE_RET_OK;
	char *p_param;

	p_param = (char *)aml_mem_getvirtaddr(awe->h_param);
	memcpy(p_param, para, sizeof(union AWE_PARA));
	aml_mem_clean(awe->h_param, awe->param_size);

	if (para_id == AWE_PARA_FREE_RUN_MODE) {
		if (para->free_run_mode == 1) {
			awe->kick_free_run = 1;
			while (awe->kick_free_run)
				;
			ret = aml_vsp_setparam(awe->h_vsp, (s32)para_id,
					       aml_mem_getphyaddr(awe->h_param), awe->param_size);
		} else {
			ret = aml_vsp_setparam(awe->h_vsp, (s32)para_id,
					       aml_mem_getphyaddr(awe->h_param), awe->param_size);
			/* sem_post(&awe->user_fill_sem); */
			/* up(&awe->user_fill_sem); */
		}
	} else {
		ret = aml_vsp_setparam(awe->h_vsp, (s32)para_id,
				       aml_mem_getphyaddr(awe->h_param), awe->param_size);
	}
	return ret;
}

enum AWE_RET aml_awe_getparam(struct AWE *awe, enum AWE_PARA_ID para_id, union AWE_PARA *para)
{
	enum AWE_RET ret = AWE_RET_OK;
	char *p_param;

	ret = aml_vsp_getparam(awe->h_vsp, (s32)para_id,
			       aml_mem_getphyaddr(awe->h_param), awe->param_size);
	aml_mem_invalidate(awe->h_param, awe->param_size);
	p_param = (char *)aml_mem_getvirtaddr(awe->h_param);
	memcpy(para, p_param, sizeof(union AWE_PARA));
	return ret;
}

enum AWE_RET aml_awe_process(struct AWE *awe, void *in[],
			     s32 *in_len_in_byte, void *out[],
			     s32 *out_len_in_byte, u32 *is_waked)
{
	enum AWE_RET ret = AWE_RET_OK;

	if (awe->status != AWE_STATUS_EXECUTE) {
		pr_err("%s:%d invalid status %d\n", __func__, __LINE__,
		       awe->status);
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	if (awe->input_mode == AWE_USER_INPUT_MODE) {
		unsigned int i;
		s32 in_len, out_len;

		in_len = AMX_MIN(*in_len_in_byte, awe->in_buf_len);
		out_len = AMX_MIN(*out_len_in_byte, awe->out_buf_len);

		for (i = 0; i < (awe->ref_channels + awe->mic_channels); i++) {
			memcpy(aml_mem_getvirtaddr(awe->h_in[i]), in[i], in_len);
			aml_mem_clean(awe->h_in[i], in_len);
		}
		ret = internal_aml_awe_process(awe, awe->h_in, &in_len, awe->h_out,
					       &out_len, is_waked);
		for (i = 0; i < awe->out_channels; i++) {
			aml_mem_invalidate(awe->h_out[i], out_len);
			memcpy(out[i], aml_mem_getvirtaddr(awe->h_out[i]), out_len);
		}
		*in_len_in_byte = in_len;
		*out_len_in_byte = out_len;
	} else {
		pr_info("AWE is not worked at user input mode. current input mode:%d\n",
			awe->input_mode);
		ret = AWE_RET_ERR_NOT_SUPPORT;
	}
	return ret;
}

enum AWE_RET aml_awe_pushbuf(struct AWE *awe, const char *data, size_t size)
{
	if (awe->status != AWE_STATUS_EXECUTE) {
		pr_err("%s:%d invalid status %d\n", __func__, __LINE__,
		       awe->status);
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	if (awe->input_mode == AWE_DSP_INPUT_MODE) {
		pr_info("Do not support this API when input is from dsp\n");
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	if (awe->user_fill) {
		/*
		 * pr_info("wr:%d rd:%d.size:%d\n\n",
		 * awe->user_fill_wr, awe->user_fill_rd, awe->user_fill_size);
		 */
		if (aml_awe_ring_buf_space(awe->user_fill_size,
					   awe->user_fill_wr,
					   awe->user_fill_rd) < size)
			/* pr_info("No enough space to fill pcm: rd:%d wr:%d size:%d\n",
			 *  awe->user_fill_rd, awe->user_fill_wr, awe->user_fill_size);
			 */
			return AWE_RET_ERR_NO_MEM;
		if (size <= (awe->user_fill_size - awe->user_fill_wr)) {
			memcpy(awe->user_fill + awe->user_fill_wr, data, size);
		} else {
			u32 block1;
			u32 block2;

			block1 = awe->user_fill_size - awe->user_fill_wr;
			block2 = size - block1;
			memcpy(awe->user_fill + awe->user_fill_wr, data, block1);
			memcpy(awe->user_fill, data + block1, block2);
		}
		awe->user_fill_wr = (awe->user_fill_wr + size) % awe->user_fill_size;
		/*post semaphore here, tell work thread user fills new pcm*/
		/* sem_post(&awe->user_fill_sem); */
		/* up(&awe->user_fill_sem); */
		return AWE_RET_OK;
	}
	pr_info("Does not allocate ring buffer\n");
	return AWE_RET_ERR_NO_MEM;
}

enum AWE_RET aml_awe_adddatahandler(struct AWE *awe, const enum AWE_DATA_TYPE type,
				    aml_awe_datahandler handler,
				    void *user_data)
{
	if (awe->status != AWE_STATUS_IDLE) {
		pr_err("%s:%d invalid status %d\n", __func__, __LINE__,
		       awe->status);
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	if (type >= AWE_DATA_TYPE_MAX) {
		pr_info("Invalid data type\n");
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	if (awe->awe_data_handler_func[type])
		pr_info("New data handler override old one=%lx\n",
			(unsigned long)awe->awe_data_handler_func[type]);
	awe->awe_data_handler_func[type] = handler;
	awe->awe_data_handler_userdata[type] = user_data;
	return AWE_RET_OK;
}

enum AWE_RET aml_awe_addeventhandler(struct AWE *awe, const enum AWE_EVENT_TYPE type,
				     aml_awe_eventhandler handler,
				     void *user_data)
{
	if (awe->status != AWE_STATUS_IDLE) {
		pr_err("%s:%d invalid status %d\n", __func__, __LINE__,
		       awe->status);
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	if (type >= AWE_EVENT_TYPE_MAX) {
		pr_info("Invalid event type\n");
		return AWE_RET_ERR_NOT_SUPPORT;
	}
	if (awe->awe_event_handler_func[type])
		pr_info("New event handler override old one=%lx\n",
			(unsigned long)awe->awe_event_handler_func[type]);
	awe->awe_event_handler_func[type] = handler;
	awe->awe_event_handler_userdata[type] = user_data;
	return AWE_RET_OK;
}
