/*******************************************************************************
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_submit.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2021/06/13	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_submit.h"

#include "adlak_api.h"
#include "adlak_common.h"
#include "adlak_context.h"
#include "adlak_device.h"
#include "adlak_dpm.h"
#include "adlak_hw.h"
#include "adlak_mm.h"
#include "adlak_queue.h"
/************************** Constant Definitions *****************************/
#ifndef ADLAK_DEBUG_CMQ_PATTTCHING_EN
#define ADLAK_DEBUG_CMQ_PATTTCHING_EN (0)
#endif
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
/*per-invoke:[fence-all,ps-reset],*/
#define ADLAK_PS_CMD_EXTERN_LEN \
    (8) /*per-layer:[swid,dep,output,active,timestamp-flag,timestamp,fence]*/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

static int adlak_cmq_public_patch_and_exec(struct adlak_task *ptask);
static int adlak_cmq_private_patch_and_exec(struct adlak_task *ptask);
static int adlak_net_pre_progress(struct adlak_context *context);

static int adlak_cmq_dump(struct adlak_task *ptask) {
#ifdef CONFIG_ADLAK_DEBUG_CMQ_DUMP
    struct adlak_device *padlak = ptask->context->padlak;

    uint32_t                 start, end;
    uint32_t *               pcmq_buf     = NULL;
    struct adlak_cmq_buffer *cmq_buf_info = ptask->context->pmodel_attr->cmq_buffer;  // TODO
    adlak_os_printf("Dump cmq buffer:");
    pcmq_buf = adlak_mm_vmap(cmq_buf_info->cmq_mm_info);
    if (ADLAK_CMQ_BUFFER_TYPE_PRIVATE == ptask->context->pmodel_attr->cmq_buffer_type) {
        start = 0;
        end   = cmq_buf_info->size / sizeof(uint32_t);
    } else {
        start = ptask->cmd_offset_start / sizeof(uint32_t);
        end   = ptask->cmd_offset_end / sizeof(uint32_t);
    }

    adlak_os_printf("cmd_offset_start: 0x%08X\t;cmd_offset_end: 0x%08X \n",
                    start * sizeof(uint32_t), end * sizeof(uint32_t));
    if (start < end) {
        while (start < end) {
            adlak_os_printf("offset:0x%08X\t0x%08X 0x%08X 0x%08X 0x%08X \n",
                            (uint32_t)(start * sizeof(uint32_t)), pcmq_buf[start + 0],
                            pcmq_buf[start + 1], pcmq_buf[start + 2], pcmq_buf[start + 3]);
            start += 4;
        }

    } else {
        // start >= end;
        while (start < (padlak->cmq_buffer_public.size / sizeof(uint32_t))) {
            adlak_os_printf("offset:0x%08X\t0x%08X 0x%08X 0x%08X 0x%08X \n",
                            (uint32_t)(start * sizeof(uint32_t)), pcmq_buf[start + 0],
                            pcmq_buf[start + 1], pcmq_buf[start + 2], pcmq_buf[start + 3]);
            start += 4;
        }
        start = 0;
        while (start < end) {
            adlak_os_printf("offset:0x%08X\t0x%08X 0x%08X 0x%08X 0x%08X \n",
                            (uint32_t)(start * sizeof(uint32_t)), pcmq_buf[start + 0],
                            pcmq_buf[start + 1], pcmq_buf[start + 2], pcmq_buf[start + 3]);
            start += 4;
        }
    }

    adlak_os_printf("\n");
#endif
    return 0;
}

static void adlak_debug_buf_dump(void *cpu_addr, int size) {
#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
    uint32_t  start, i, cmq_size_dw;
    uint32_t *pcmq_buf = NULL;
    AML_LOG_DEBUG("Dump  buffer:");
    pcmq_buf    = cpu_addr;
    cmq_size_dw = size / sizeof(uint32_t);
    start       = 0;
    i           = 0;
    for (i = 0; i < cmq_size_dw;) {
        AML_LOG_DEFAULT("offset:0x%08X\t0x%08X 0x%08X 0x%08X 0x%08X \n",
                        (uint32_t)((start + i) * sizeof(uint32_t)), pcmq_buf[start + i],
                        pcmq_buf[start + i + 1], pcmq_buf[start + i + 2], pcmq_buf[start + i + 3]);
        i = i + 4;
    }
    AML_LOG_DEFAULT("\n");
#endif
}

static inline uint32_t adlak_cmq_cal_tail(adlak_circular_buffer *buffer, uint32_t tail,
                                          uint32_t inc_size) {
    return (tail + inc_size) % buffer->size;  // Move tail circularly
}

static uint32_t adlak_cmq_get_cur_tail(adlak_circular_buffer *buffer) { return buffer->tail; }

static void adlak_cmq_write_data(adlak_circular_buffer *buffer, uint32_t value) {
    AML_LOG_DEBUG("wr data start tail=%u", buffer->tail);

    buffer->data[buffer->tail] = value;
    buffer->tail               = (buffer->tail + 1) % buffer->size;  // Move tail circularly

    if (buffer->tail == buffer->head) {
        buffer->head = (buffer->head + 1) % buffer->size;  // Move head circularly
    }

    AML_LOG_DEBUG("wr data end tail=%u", buffer->tail);
}

static void adlak_cmq_write_buffer(adlak_circular_buffer *buffer, const uint32_t *source,
                                   uint32_t length) {
    uint32_t spaceToEnd =
        buffer->size - buffer->tail;  // Calculate available space until the end of the buffer

    AML_LOG_DEBUG("wr buffer start tail=%u", buffer->tail);

    if (length <= spaceToEnd) {
        memcpy(buffer->data + buffer->tail, source, length * sizeof(uint32_t));
        buffer->tail = (buffer->tail + length) % buffer->size;  // Move tail circularly
    } else {
        memcpy(buffer->data + buffer->tail, source, spaceToEnd * sizeof(uint32_t));
        memcpy(buffer->data, source + spaceToEnd, (length - spaceToEnd) * sizeof(uint32_t));
        buffer->tail = (buffer->tail + length) % buffer->size;  // Move tail circularly
        buffer->head = buffer->tail;  // Move head to tail position (overwrite old data)
    }

    AML_LOG_DEBUG("wr buffer end tail=%u", buffer->tail);
}

static void adlak_cmq_merge_modify_data(const struct adlak_submit_task *submit_task,
                                        adlak_circular_buffer *buffer, uint32_t cmq_cfg_index,
                                        const uint32_t *modify_buf, uint32_t modify_size) {
    adlak_circular_buffer buffer_inner;
    uint32_t              cmd_bypass_cnt, cmd_modify_cnt;

    if (0 == modify_size) {
        return;
    }
    buffer_inner.data = buffer->data;
    buffer_inner.size = buffer->size;
    buffer_inner.tail = cmq_cfg_index;
    buffer_inner.head = buffer->head;

    while (modify_size) {
        cmd_bypass_cnt = ((*modify_buf) & 0xFFFF);
        cmd_modify_cnt = (((*modify_buf) >> 16) & 0xFF);
        modify_buf += 1;
        buffer_inner.tail = adlak_cmq_cal_tail(&buffer_inner, buffer_inner.tail, cmd_bypass_cnt);
        if (cmd_modify_cnt) {
            adlak_cmq_write_buffer(&buffer_inner, (const uint32_t *)modify_buf, cmd_modify_cnt);
        }
        modify_buf += cmd_modify_cnt;
        modify_size -= ((cmd_modify_cnt + 1) * 4);
    }
}

#if CONFIG_ADLAK_EMU_EN
uint32_t g_adlak_emu_dev_wpt;
uint32_t g_adlak_emu_dev_rpt;
uint32_t g_adlak_emu_dev_cmq_total_size;

uint32_t adlak_emu_update_rpt(void) {
    g_adlak_emu_dev_rpt = g_adlak_emu_dev_wpt;
    return g_adlak_emu_dev_rpt;
}
uint32_t adlak_emu_hal_get_ps_rpt(void *data) {
    uint32_t wpt_u32, rpt;
    if (g_adlak_emu_dev_wpt > 16) {
        wpt_u32 = (g_adlak_emu_dev_wpt - 16) / sizeof(uint32_t);
    } else {
        wpt_u32 = 0;
    }
    rpt = wpt_u32 % g_adlak_emu_dev_cmq_total_size;
    rpt = rpt * sizeof(uint32_t);
    return rpt;
}

#endif

static bool adlak_has_dependency(int64_t dlid, int64_t flid, int32_t max_outstanding) {
    // true if:
    //  (1) dependent on output of previous tasks, and
    //  (2) flid is not too far ahead of dlid (within the range of [0, max_outstanding -1])
    //
    // note: condition (2) is necessary due to the limited bit count used in HW to store IDs

    ASSERT(flid >= dlid);

    if ((dlid >= 0) && ((flid - dlid) < max_outstanding)) {
        return true;
    }

    return false;
};
static int32_t adlak_get_max_outstanding_outputs(enum ADLAK_PLATFORM_MODULE module) {
    switch (module) {
        case ADLAK_PLATFORM_MODULE_PWE:
            return 4;

        case ADLAK_PLATFORM_MODULE_PWX:
            return 4;

        case ADLAK_PLATFORM_MODULE_RS:
            return 4;

        default:
            // should not reach here
            ASSERT(0);
            while (1)
                ;
            return 0;
    }
}

static uint32_t adlak_gen_dep_cmd(int dependency_mode, const struct adlak_submit_task *task,
                                  struct adlak_submit_dep_fixup *pdep_fixups_base,
                                  int32_t pwe_flid_offset, int32_t pwx_flid_offset,
                                  int32_t rs_flid_offset, int32_t start_id_pwe,
                                  int32_t start_id_pwx, int32_t start_id_rs) {
    uint32_t                       cmd = PS_CMD_SET_DEPENDENCY;  // default no dependency
    struct adlak_submit_dep_fixup *dep = NULL;

    {
        bool     has_pwe_dependency = false, has_pwx_dependency = false, has_rs_dependency = false;
        int64_t  pwe_dlid = -1, pwx_dlid = -1, rs_dlid = -1;
        int      i;
        uint32_t dependency;

        if (dependency_mode != ADLAK_DEPENDENCY_MODE_PARSER) {
            return cmd;
        }
#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
        AML_LOG_DEBUG("%s", __func__);
        AML_LOG_DEBUG("pwe_flid_offset=%d,pwx_flid_offset=%d,rs_flid_offset=%d", pwe_flid_offset,
                      pwx_flid_offset, rs_flid_offset);
        AML_LOG_DEBUG("global_id_pwe=%d,global_id_pwx=%d,global_id_rs=%d", start_id_pwe,
                      start_id_pwx, start_id_rs);
        AML_LOG_DEBUG("dep_info_offset=%d,dep_info_count=%d", task->dep_info_offset,
                      task->dep_info_count);
        AML_LOG_DEBUG("reg_fixup_offset=%d,reg_fixup_count=%d", task->reg_fixup_offset,
                      task->reg_fixup_count);
#endif
        for (i = 0; i < task->dep_info_count; ++i) {
            dep = &pdep_fixups_base[task->dep_info_offset + i];
            switch (dep->module) {
                case ADLAK_DEPENDENCY_MODULE_PWE:
                    pwe_dlid = max(pwe_dlid, (int64_t)dep->dep_id);
                    break;

                case ADLAK_DEPENDENCY_MODULE_PWX:
                    pwx_dlid = max(pwx_dlid, (int64_t)dep->dep_id);
                    break;

                case ADLAK_DEPENDENCY_MODULE_RS:
                    rs_dlid = max(rs_dlid, (int64_t)dep->dep_id);
                    break;

                default:
                    // should not reach here
                    AML_LOG_ERR("dep->module=%u", dep->module);
                    ASSERT(0);
                    break;
            }
        }
#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
        AML_LOG_DEBUG("pwe_dlid =%lld, pwx_dlid =%lld,rs_dlid=%lld", pwe_dlid, pwx_dlid, rs_dlid);
#endif
        if (pwe_dlid >= 0) {
            if (pwe_dlid >= pwe_flid_offset) {
#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
                AML_LOG_DEBUG("pwe_dlid =%lld, pwe_flid_offset =%d,start_pwe_flid=%d", pwe_dlid,
                              pwe_flid_offset, task->start_pwe_flid);
#endif
                has_pwe_dependency = adlak_has_dependency(
                    pwe_dlid, task->start_pwe_flid,
                    adlak_get_max_outstanding_outputs(ADLAK_PLATFORM_MODULE_PWE));
                pwe_dlid = start_id_pwe + (pwe_dlid - pwe_flid_offset) + 1;
            }
        }
        if (pwx_dlid >= 0) {
            if (pwx_dlid >= pwx_flid_offset) {
#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
                AML_LOG_DEBUG("pwx_dlid =%lld, pwx_flid_offset =%d,start_pwx_flid=%d", pwx_dlid,
                              pwx_flid_offset, task->start_pwx_flid);
#endif
                has_pwx_dependency = adlak_has_dependency(
                    pwx_dlid, task->start_pwx_flid,
                    adlak_get_max_outstanding_outputs(ADLAK_PLATFORM_MODULE_PWX));
                pwx_dlid = start_id_pwx + (pwx_dlid - pwx_flid_offset) + 1;
            }
        }

        if (rs_dlid >= 0) {
            if (rs_dlid >= rs_flid_offset) {
#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
                AML_LOG_DEBUG("rs_dlid =%lld, rs_flid_offset =%d,start_rs_flid=%d\n", rs_dlid,
                              rs_flid_offset, task->start_rs_flid);
#endif
                has_rs_dependency = adlak_has_dependency(
                    rs_dlid, task->start_rs_flid,
                    adlak_get_max_outstanding_outputs(ADLAK_PLATFORM_MODULE_RS));
                rs_dlid = start_id_rs + (rs_dlid - rs_flid_offset) + 1;
            }
        }

        dependency =
            (has_rs_dependency ? PS_CMD_DEPENDENCY_RS_VALID_MASK : 0) |
            (has_pwe_dependency ? PS_CMD_DEPENDENCY_PWE_VALID_MASK : 0) |
            (has_pwx_dependency ? PS_CMD_DEPENDENCY_PWX_VALID_MASK : 0) |
            ((rs_dlid << PS_CMD_DEPENDENCY_RS_ID_SHIFT) & PS_CMD_DEPENDENCY_RS_ID_MASK) |
            ((pwe_dlid << PS_CMD_DEPENDENCY_PWE_ID_SHIFT) & PS_CMD_DEPENDENCY_PWE_ID_MASK) |
            ((pwx_dlid << PS_CMD_DEPENDENCY_PWX_ID_SHIFT) & PS_CMD_DEPENDENCY_PWX_ID_MASK);

        cmd |= dependency;
    }

    return cmd;
}

static int adlak_cmq_remain_space_check(struct adlak_device *    padlak,
                                        struct adlak_cmq_buffer *cmq_buf_info, uint32_t wpt_u8,
                                        int size_required) {
    int      size_remain;
    uint32_t rpt_u8;
    uint32_t retry = 0;
    do {
        AML_LOG_DEBUG("%s", __func__);
        rpt_u8 = cmq_buf_info->cmq_rd_offset;
        // |....rpt..wpt...|
        if (rpt_u8 <= wpt_u8) {
            size_remain = cmq_buf_info->size - wpt_u8 + rpt_u8;
        }
        // |...wpt...rpt...|
        else if (rpt_u8 > wpt_u8) {
            size_remain = rpt_u8 - wpt_u8;
        }

        AML_LOG_DEBUG(
            "need %d bytes space.remain %d bytes;\nrd_offset=%d,wr_offset=%d,cmq_total_size=%d.",
            size_required, size_remain, rpt_u8, wpt_u8, cmq_buf_info->size);
        if (size_required < size_remain) {
            return 0;
        }
        retry++;
        // if there is not enough cmq space ,we need wait..
        // wait_for_completion
        AML_LOG_INFO("there is not enough cmq space,we need wait!");

        if (retry > 3) {
            adlak_os_udelay(1);  // You can't sleep here,and you can't wait too long here, otherwise
                                 // it will affect performance
        }
        if (retry > 50000) {
            AML_LOG_ERR("wait cmq free timeout!");
            break;
        }
#if CONFIG_ADLAK_EMU_EN
        cmq_buf_info->cmq_rd_offset = adlak_emu_hal_get_ps_rpt(padlak);
#else
        cmq_buf_info->cmq_rd_offset = adlak_hal_get_ps_rpt(padlak);
#endif
    } while (1);
    return -1;
}

static void adlak_update_addr_fixups(const struct adlak_submit_task *task,
                                     struct adlak_submit_addr_fixup *paddr_fixups_base,
                                     adlak_circular_buffer *buffer, uint32_t cmq_offset_cfg) {
    struct adlak_submit_addr_fixup *addr_fixup = NULL;
    int32_t                         i;
    uint32_t                        cmq_offset;
    AML_LOG_DEBUG("%s", __func__);
    for (i = 0; i < task->addr_fixup_count; ++i) {
        addr_fixup = &paddr_fixups_base[task->addr_fixup_offset + i];
        cmq_offset =
            adlak_cmq_cal_tail(buffer, cmq_offset_cfg, (addr_fixup->loc / sizeof(uint32_t)));
        buffer->data[cmq_offset] =
            (buffer->data[cmq_offset] & (~addr_fixup->mask)) |
            ((((uintptr_t)addr_fixup->addr / addr_fixup->unit) << addr_fixup->shift) &
             addr_fixup->mask);
        // TODO(shiwei.sun) flush cache of update addr
    }
}

static void adlak_update_reg_fixups(int dependency_mode, const struct adlak_submit_task *task,
                                    struct adlak_submit_reg_fixup *preg_fixups_base,
                                    adlak_circular_buffer *buffer, uint32_t cmq_offset_cfg) {
    struct adlak_submit_reg_fixup *reg_fixup = NULL;
    int32_t                        i;
    uint32_t                       mode, cmq_offset;
    AML_LOG_DEBUG("%s", __func__);
    for (i = 0; i < task->reg_fixup_count; ++i) {
        reg_fixup = &preg_fixups_base[task->reg_fixup_offset + i];
        if (ADLAK_REG_FIXUP_TYPE_PW_COMP_FLUSH_MODE == reg_fixup->type) {
            mode = reg_fixup->modes[dependency_mode];
            cmq_offset =
                adlak_cmq_cal_tail(buffer, cmq_offset_cfg, (reg_fixup->loc / sizeof(uint32_t)));

            buffer->data[cmq_offset] = (buffer->data[cmq_offset] & (~reg_fixup->mask)) |
                                       ((mode << reg_fixup->shift) & reg_fixup->mask);
        } else {
            // should not reach here
            AML_LOG_ERR("reg_fixup->type=%u", reg_fixup->type);
            ASSERT(0);
        }
    }
}

static void adlak_update_module_dependency(int                             dependency_mode,
                                           const struct adlak_submit_task *task,
                                           struct adlak_submit_dep_fixup * pdep_fixups_base,
                                           int32_t pwe_flid_offset, int32_t pwx_flid_offset,
                                           int32_t rs_flid_offset, int32_t start_id_pwe,
                                           int32_t start_id_pwx, int32_t start_id_rs,
                                           adlak_circular_buffer *buffer, uint32_t cmq_offset_cfg) {
    struct adlak_submit_dep_fixup *dep = NULL;
    int32_t                        invoke_start_flid, dlid;
    int32_t                        i, task_start_flid, flid_offset, max_outstanding_outputs;
    uint32_t                       mode, cmq_offset;

    if ((dependency_mode != ADLAK_DEPENDENCY_MODE_MODULE_LAYER) &&
        (dependency_mode != ADLAK_DEPENDENCY_MODE_MODULE_H_COUNT)) {
        return;
    }
    AML_LOG_DEBUG("%s", __func__);
    for (i = 0; i < task->dep_info_count; ++i) {
        invoke_start_flid       = -1;
        task_start_flid         = -1;
        flid_offset             = 0;
        max_outstanding_outputs = 0;
        dep                     = &pdep_fixups_base[task->dep_info_offset + i];
        switch (dep->module) {
            case ADLAK_DEPENDENCY_MODULE_PWE:
                invoke_start_flid = start_id_pwe;
                task_start_flid   = task->start_pwe_flid;
                flid_offset       = pwe_flid_offset;
                max_outstanding_outputs =
                    adlak_get_max_outstanding_outputs(ADLAK_PLATFORM_MODULE_PWE);
                break;

            case ADLAK_DEPENDENCY_MODULE_PWX:
                invoke_start_flid = start_id_pwx;
                task_start_flid   = task->start_pwx_flid;
                flid_offset       = pwx_flid_offset;
                max_outstanding_outputs =
                    adlak_get_max_outstanding_outputs(ADLAK_PLATFORM_MODULE_PWX);
                break;

            case ADLAK_DEPENDENCY_MODULE_RS:
                invoke_start_flid = start_id_rs;
                task_start_flid   = task->start_rs_flid;
                flid_offset       = rs_flid_offset;
                max_outstanding_outputs =
                    adlak_get_max_outstanding_outputs(ADLAK_PLATFORM_MODULE_RS);
                break;

            default:
                // should not reach here
                AML_LOG_ERR("dep->module=%u", dep->module);
                ASSERT(0);
                break;
        }
        if (dep->dep_id >= flid_offset) {
            if (adlak_has_dependency(dep->dep_id, task_start_flid, max_outstanding_outputs)) {
                dlid = invoke_start_flid + (dep->dep_id - flid_offset) + 1;
                mode = dep->dep_modes[dependency_mode];

                cmq_offset =
                    adlak_cmq_cal_tail(buffer, cmq_offset_cfg, (dep->id_loc / sizeof(uint32_t)));
                buffer->data[cmq_offset] = (buffer->data[cmq_offset] & (~dep->id_mask)) |
                                           ((dlid << dep->id_shift) & dep->id_mask);
                cmq_offset =
                    adlak_cmq_cal_tail(buffer, cmq_offset_cfg, (dep->mode_loc / sizeof(uint32_t)));
                buffer->data[cmq_offset] = (buffer->data[cmq_offset] & (~dep->mode_mask)) |
                                           ((mode << dep->mode_shift) & dep->mode_mask);
            }
        }
    }
}

int adlak_queue_schedule_update(struct adlak_device *padlak, struct adlak_task **ptask_sch_cur,
                                int32_t net_id_pre) {
    struct adlak_workqueue *pwq   = &padlak->queue;
    struct adlak_task *     ptask = NULL, *ptask_tmp = NULL;
    int                     ret = 0;
    AML_LOG_INFO("%s", __func__);

    adlak_os_mutex_lock(&pwq->wq_mutex);
    if (list_empty(&pwq->pending_list)) {
        AML_LOG_WARN("pending_list is empty!,pwq->pending_num=%d", pwq->pending_num);
        *ptask_sch_cur = NULL;
        ret            = -1;
        pwq->pending_num--;
        goto end;
    }
    if (net_id_pre < 0) {
        ptask = list_first_entry(&pwq->pending_list, typeof(struct adlak_task), head);
        if (ptask) {
            ret = 0;
        } else {
            ret = -1;
        }

    } else {
        ret = -1;
        list_for_each_entry_safe(ptask, ptask_tmp, &pwq->pending_list, head) {
            if (net_id_pre == ptask->context->net_id) {
                ret = 0;
                break;
            }
        }
    }
    if (!ret) {
        list_move_tail(&ptask->head, &pwq->scheduled_list);

        pwq->pending_num--;
        pwq->sched_num++;
        *ptask_sch_cur = ptask;
    }

end:
    adlak_os_mutex_unlock(&pwq->wq_mutex);
    return ret;
}

/**
 * adlak_queue_schedule() - Schedule a queue inference.
 * @core:	adlak core.
 *
 * Pop the inference queue until either the queue is empty or an inference has
 * been successfully scheduled.
 */

static struct adlak_model_attr *adlak_model_create(struct adlak_context *     context,
                                                   struct adlak_network_desc *psubmit_desc) {
    struct adlak_model_attr *pmodel_attr;
    struct context_buf *     target_buf = NULL;
    int                      ret;

    AML_LOG_INFO("%s", __func__);
    pmodel_attr = adlak_os_zalloc(sizeof(struct adlak_model_attr), ADLAK_GFP_KERNEL);
    if (!pmodel_attr) {
        return ADLAK_ERR_PTR(ERR(ENOMEM));
    }

    pmodel_attr->context = context;

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_reset(context);
    adlak_dbg_inner_update(context, "task_create");
#endif
    psubmit_desc->net_register_idx = context->net_id;
#ifndef CONFIG_ADLA_FREERTOS
    pmodel_attr->submit_tasks = adlak_os_vmalloc(
        sizeof(struct adlak_submit_task) * psubmit_desc->tasks_num, ADLAK_GFP_KERNEL);
    if (!pmodel_attr->submit_tasks) {
        AML_LOG_ERR("alloc buffer for save submit data failed!");
        ret = ERR(ENOMEM);
        goto err_alloc_submit_task;
    }
    if (psubmit_desc->config_total_size) {
        pmodel_attr->config =
            adlak_os_vmalloc(sizeof(uint8_t) * psubmit_desc->config_total_size, ADLAK_GFP_KERNEL);
        if (!pmodel_attr->config) {
            AML_LOG_ERR("alloc buffer for save submit data failed!");
            ret = ERR(ENOMEM);
            goto err_alloc_config;
        }
    }
    if (psubmit_desc->dep_fixups_num) {
        pmodel_attr->submit_dep_fixups = adlak_os_vmalloc(
            sizeof(struct adlak_submit_dep_fixup) * psubmit_desc->dep_fixups_num, ADLAK_GFP_KERNEL);
        if (!pmodel_attr->submit_dep_fixups) {
            AML_LOG_ERR("alloc buffer for save submit data failed!");
            ret = ERR(ENOMEM);
            goto err_alloc_dep_fixups;
        }
    } else {
        pmodel_attr->submit_dep_fixups = NULL;
    }
    if (psubmit_desc->reg_fixups_num) {
        pmodel_attr->submit_reg_fixups = adlak_os_vmalloc(
            sizeof(struct adlak_submit_reg_fixup) * psubmit_desc->reg_fixups_num, ADLAK_GFP_KERNEL);
        if (!pmodel_attr->submit_reg_fixups) {
            AML_LOG_ERR("alloc buffer for save submit data failed!");
            ret = ERR(ENOMEM);
            goto err_alloc_reg_fixups;
        }
    } else {
        pmodel_attr->submit_reg_fixups = NULL;
    }

    if (psubmit_desc->cmd_buf_attr.support) {
        /*check cmq_buffer*/
        target_buf = find_buffer_by_desc(
            context, (void *)(uintptr_t)(psubmit_desc->cmd_buf_attr.mem_handle));
        if (!target_buf) {
            AML_LOG_ERR("no corresponding buffer found in this context!");
            ret = ERR(ENOMEM);
            goto err_alloc_reg_fixups;
        } else {
            pmodel_attr->cmd_buf_attr.mm_info = target_buf->mm_info;
        }
        /*save cmq buffer attribute*/
        pmodel_attr->cmd_buf_attr.support = psubmit_desc->cmd_buf_attr.support;
        pmodel_attr->cmd_buf_attr.reserve_count_common_head =
            psubmit_desc->cmd_buf_attr.reserve_count_common_head;
        pmodel_attr->cmd_buf_attr.reserve_count_common_tail =
            psubmit_desc->cmd_buf_attr.reserve_count_common_tail;
        pmodel_attr->cmd_buf_attr.reserve_count_modify_head =
            psubmit_desc->cmd_buf_attr.reserve_count_modify_head;
        pmodel_attr->cmd_buf_attr.reserve_count_modify_tail =
            psubmit_desc->cmd_buf_attr.reserve_count_modify_tail;
    }

    /*****copy data from user*****/
    ret = copy_from_user((void *)pmodel_attr->config,
                         (void __user *)(uintptr_t)psubmit_desc->config_va,
                         sizeof(uint8_t) * psubmit_desc->config_total_size);
    if (ret) {
        AML_LOG_ERR("copy from user failed!");
        ret = ERR(EFAULT);
        goto err_copy_from_user;
    }
    ret = copy_from_user((void *)pmodel_attr->submit_tasks,
                         (void __user *)(uintptr_t)psubmit_desc->tasks_va,
                         sizeof(struct adlak_submit_task) * psubmit_desc->tasks_num);
    if (ret) {
        AML_LOG_ERR("copy from user failed!");
        ret = ERR(EFAULT);
        goto err_copy_from_user;
    }
    if (pmodel_attr->submit_dep_fixups) {
        ret = copy_from_user((void *)pmodel_attr->submit_dep_fixups,
                             (void __user *)(uintptr_t)psubmit_desc->dep_fixups_va,
                             sizeof(struct adlak_submit_dep_fixup) * psubmit_desc->dep_fixups_num);
        if (ret) {
            AML_LOG_ERR("copy from user failed!");
            ret = ERR(EFAULT);
            goto err_copy_from_user;
        }
    }
    if (pmodel_attr->submit_reg_fixups) {
        ret = copy_from_user((void *)pmodel_attr->submit_reg_fixups,
                             (void __user *)(uintptr_t)psubmit_desc->reg_fixups_va,
                             sizeof(struct adlak_submit_reg_fixup) * psubmit_desc->reg_fixups_num);
        if (ret) {
            AML_LOG_ERR("copy from user failed!");
            ret = ERR(EFAULT);
            goto err_copy_from_user;
        }
    }

#else
    pmodel_attr->config = (void *)(uintptr_t)psubmit_desc->config_va;
    pmodel_attr->submit_tasks = (void *)(uintptr_t)psubmit_desc->tasks_va;
    pmodel_attr->submit_dep_fixups = (void *)(uintptr_t)psubmit_desc->dep_fixups_va;
    if (psubmit_desc->reg_fixups_num) {
        pmodel_attr->submit_reg_fixups = (void *)(uintptr_t)psubmit_desc->reg_fixups_va;
    } else {
        pmodel_attr->submit_reg_fixups = NULL;
    }
#endif

    AML_LOG_DEBUG("submit_dep_fixups dump...");
    adlak_debug_buf_dump((void *)pmodel_attr->submit_dep_fixups,
                         sizeof(struct adlak_submit_dep_fixup) * psubmit_desc->dep_fixups_num);
    AML_LOG_DEBUG("submit_reg_fixups dump...");
    adlak_debug_buf_dump((void *)pmodel_attr->submit_reg_fixups,
                         sizeof(struct adlak_submit_reg_fixup) * psubmit_desc->reg_fixups_num);

    pmodel_attr->config_total_size       = psubmit_desc->config_total_size;
    pmodel_attr->submit_tasks_num        = psubmit_desc->tasks_num;
    pmodel_attr->dep_fixups_num          = psubmit_desc->dep_fixups_num;
    pmodel_attr->reg_fixups_num          = psubmit_desc->reg_fixups_num;
    pmodel_attr->pm_cfg.profile_en       = psubmit_desc->profile_en;
    pmodel_attr->pm_cfg.profile_iova     = psubmit_desc->profile_iova;
    pmodel_attr->pm_cfg.profile_buf_size = psubmit_desc->profile_buf_size;
    pmodel_attr->pm_stat.pm_rpt          = 0;
    pmodel_attr->pm_stat.pm_wpt          = 0;
    pmodel_attr->cmq_buffer              = &context->padlak->cmq_buffer_public;
    pmodel_attr->cmq_buffer_type         = ADLAK_CMQ_BUFFER_TYPE_PUBLIC;
    pmodel_attr->hw_layer_first          = -1;
    pmodel_attr->hw_layer_last           = -1;

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "task_create done");
#endif
    return pmodel_attr;

err_copy_from_user:
    if (pmodel_attr->submit_reg_fixups) {
        adlak_os_vfree(pmodel_attr->submit_reg_fixups);
    }
err_alloc_reg_fixups:
    if (pmodel_attr->submit_dep_fixups) {
        adlak_os_vfree(pmodel_attr->submit_dep_fixups);
    }
err_alloc_dep_fixups:
    adlak_os_vfree(pmodel_attr->config);

err_alloc_config:

    adlak_os_vfree(pmodel_attr->submit_tasks);
err_alloc_submit_task:
    adlak_os_free(pmodel_attr);
    return ADLAK_ERR_PTR(ret);
}
void adlak_model_destroy(struct adlak_model_attr *pmodel_attr) {
    struct adlak_device *padlak = pmodel_attr->context->padlak;
    AML_LOG_DEBUG("%s", __func__);

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(pmodel_attr->context, "task destroy");
#endif

#ifndef CONFIG_ADLA_FREERTOS
    if (pmodel_attr->submit_reg_fixups) {
        adlak_os_vfree(pmodel_attr->submit_reg_fixups);
    }
    if (pmodel_attr->submit_dep_fixups) {
        adlak_os_vfree(pmodel_attr->submit_dep_fixups);
    }
    if (pmodel_attr->config) {
        adlak_os_vfree(pmodel_attr->config);
    }
    adlak_os_vfree(pmodel_attr->submit_tasks);
    if (pmodel_attr->submit_addr_fixups) {
        adlak_os_free(pmodel_attr->submit_addr_fixups);
    }
#endif

    pmodel_attr->submit_addr_fixups = NULL;
    if (pmodel_attr->cmq_priv) {
        adlak_os_free(pmodel_attr->cmq_priv);
        pmodel_attr->cmq_priv = NULL;
    }

    if (pmodel_attr->invoke_attr_rsv) {
        adlak_os_free(pmodel_attr->invoke_attr_rsv);
        pmodel_attr->invoke_attr_rsv = NULL;
    }
    adlak_os_free(pmodel_attr);
    padlak->all_task_num--;
}

static void adlak_mark_the_last_hw_layer(struct adlak_model_attr *pmodel_attr) {
    uint32_t                  task_idx;
    struct adlak_submit_task *psubmitask_base = NULL, *psubmitask = NULL;

    AML_LOG_DEBUG("%s", __func__);
    psubmitask_base = pmodel_attr->submit_tasks;

    for (task_idx = pmodel_attr->submit_tasks_num - 1; task_idx >= 0; task_idx--) {
        psubmitask = psubmitask_base + task_idx;
        if (0 >= (psubmitask->config_size + psubmitask->config_v2.common_size)) {
            // software operations
            continue;
        }
        pmodel_attr->hw_layer_last = task_idx;
        break;
    }
}

static int adlak_invoke_pre_check(struct adlak_model_attr *pmodel_attr) {
    int                  ret    = 0;
    struct adlak_device *padlak = pmodel_attr->context->padlak;

    /* 1. whether the cmq's size exceeds the maximum limit?
    If the maximum size limit is exceeded,return error.*/

    uint32_t                  task_idx;
    uint32_t                  size_max, size_per_task;
    uint32_t                  cmq_size_expected = 0;
    struct adlak_submit_task *psubmitask_base = NULL, *psubmitask = NULL;

    AML_LOG_DEBUG("%s", __func__);
    psubmitask_base = pmodel_attr->submit_tasks;
    size_max        = 0;

    for (task_idx = 0; task_idx < pmodel_attr->submit_tasks_num; task_idx++) {
        psubmitask = psubmitask_base + task_idx;
        ASSERT(!((psubmitask->config_size) > 0 && (psubmitask->config_v2.common_size > 0)));
        if (0 >= (psubmitask->config_size + psubmitask->config_v2.common_size)) {
            // software operations
            continue;
        }
        if (-1 == pmodel_attr->hw_layer_first) {
            pmodel_attr->hw_layer_first = task_idx;
        }
        size_per_task = ((uint32_t)ADLAK_PS_CMD_EXTERN_LEN) * sizeof(uint32_t);
        size_per_task += (psubmitask->config_size + psubmitask->config_v2.common_size);
        size_per_task = ADLAK_ALIGN(size_per_task, 16);
        cmq_size_expected += size_per_task;
        if (size_max < size_per_task) {
            size_max = size_per_task;
        }
    }

    AML_LOG_INFO("size max(%d);", size_max);
    if (size_max >= padlak->cmq_buffer_public.size) {
        /*size_max cannot be equal to cmq_total_size, otherwise 'wpt = rpt' will be happen*/
        ret = -1;
        AML_LOG_ERR(
            "the maximum size limit is exceeded which the cmq need (%d),but buffer size max(%d);"
            "\nPlease increase the size of command queue when insmod.",
            size_max, padlak->cmq_buffer_public.size);
        goto err;
    }

    pmodel_attr->cmq_size_expected = cmq_size_expected;
    pmodel_attr->size_max_in_layer = size_max;
    return 0;
err:
    return ret;
}

static struct adlak_task *adlak_invoke_create(struct adlak_context *            context,
                                              struct adlak_network_invoke_desc *pinvoke_desc) {
    struct adlak_model_attr *pmodel_attr;
    struct adlak_task *      pinvoke_attr;
    int                      ret;
    AML_LOG_DEBUG("%s", __func__);
    pmodel_attr = context->pmodel_attr;
    if (!pmodel_attr) {
        AML_LOG_ERR("not found network!");
        return ADLAK_ERR_PTR(ERR(ENXIO));
    }
    ASSERT(context->net_id == pinvoke_desc->net_register_idx);
    if (pinvoke_desc->start_idx < 0 || pinvoke_desc->end_idx >= pmodel_attr->submit_tasks_num ||
        pinvoke_desc->start_idx > pinvoke_desc->end_idx) {
        AML_LOG_ERR("invoke id is invalid!");
        return ADLAK_ERR_PTR(ERR(EINVAL));
    }
    if (!pmodel_attr->invoke_attr_rsv) {
        pinvoke_attr = adlak_os_zalloc(sizeof(struct adlak_task), ADLAK_GFP_KERNEL);
        if (!pinvoke_attr) {
            AML_LOG_ERR("adlak_os_zalloc fail!");
            return ADLAK_ERR_PTR(ERR(ENOMEM));
        }
        pmodel_attr->invoke_attr_rsv = pinvoke_attr;
    } else {
        pinvoke_attr = pmodel_attr->invoke_attr_rsv;
    }
    if (pinvoke_desc->addr_fixups_num) {
#ifndef CONFIG_ADLA_FREERTOS
        if (pmodel_attr->submit_addr_fixups &&
            (pmodel_attr->addr_fixups_num != pinvoke_desc->addr_fixups_num)) {
            adlak_os_free(pmodel_attr->submit_addr_fixups);
            pmodel_attr->submit_addr_fixups = NULL;
        }

        pmodel_attr->addr_fixups_num = pinvoke_desc->addr_fixups_num;

        if (NULL == pmodel_attr->submit_addr_fixups) {
            pmodel_attr->submit_addr_fixups = adlak_os_malloc(
                sizeof(struct adlak_submit_addr_fixup) * pmodel_attr->addr_fixups_num,
                ADLAK_GFP_KERNEL);
            if (!pmodel_attr->submit_addr_fixups) {
                AML_LOG_ERR("alloc buffer for save submit_addr_fixups failed!");
                return ADLAK_ERR_PTR(ERR(EINVAL));
            }
        }
        /*****copy data from user*****/
        ret = copy_from_user((void *)pmodel_attr->submit_addr_fixups,
                             (void __user *)(uintptr_t)pinvoke_desc->addr_fixups_va,
                             sizeof(struct adlak_submit_addr_fixup) * pmodel_attr->addr_fixups_num);
        if (ret) {
            AML_LOG_ERR("copy from user failed!");
            pmodel_attr = ADLAK_ERR_PTR(ERR(EFAULT));
            goto err_copy_from_user;
        }

#else

        pmodel_attr->addr_fixups_num = pinvoke_desc->addr_fixups_num;
        pmodel_attr->submit_addr_fixups = (void *)(uintptr_t)pinvoke_desc->addr_fixups_va;

#endif
    }

    pinvoke_attr->context = context;

    ++pmodel_attr->invoke_count;
    if (pmodel_attr->invoke_count < 0) {
        pmodel_attr->invoke_count = 0;
    }
    pinvoke_attr->invoke_idx          = pmodel_attr->invoke_count;
    pinvoke_desc->invoke_register_idx = pinvoke_attr->invoke_idx;  // return invoke index
    pinvoke_attr->invoke_start_idx    = pinvoke_desc->start_idx;
    pinvoke_attr->invoke_end_idx      = pinvoke_desc->end_idx;

    INIT_LIST_HEAD(&pinvoke_attr->head);

    return pinvoke_attr;

err_copy_from_user:
#ifndef CONFIG_ADLA_FREERTOS
    if (pmodel_attr->submit_addr_fixups) {
        adlak_os_free(pmodel_attr->submit_addr_fixups);
    }
#endif
    pmodel_attr->submit_addr_fixups = NULL;
    return NULL;
}

void adlak_invoke_destroy(struct adlak_task *ptask) {
    AML_LOG_DEBUG("%s", __func__);
    if (ptask) {
        ptask->context->invoke_cnt--;
        // adlak_os_free(ptask); //free the task memory will be done in model destroy.
        ptask = NULL;
    }
}

// return deleted count
static int adlak_invoke_del_from_nosch_list(struct list_head *hd, int32_t net_id,
                                            int32_t invoke_id) {
    int                ret   = 0;
    struct adlak_task *ptask = NULL, *ptask_tmp = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (!list_empty(hd)) {
        list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
            if (ptask) {
                if ((net_id != -1) && (net_id != ptask->context->net_id)) {
                    continue;
                }
                if ((invoke_id != -1) && (invoke_id != ptask->invoke_idx)) {
                    continue;
                }
                ret++;
                list_del(&ptask->head);
                adlak_invoke_destroy(ptask);
            }
        }
    }
    return ret;
}

// return deleted count
static int adlak_invoke_del_from_sch_list(struct list_head *hd, int32_t net_id, int32_t invoke_id) {
    int                ret   = 0;
    struct adlak_task *ptask = NULL, *ptask_tmp = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (!list_empty(hd)) {
        list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
            if (ptask) {
                if ((net_id != -1) && (net_id != ptask->context->net_id)) {
                    continue;
                }
                if ((invoke_id != -1) && (invoke_id != ptask->invoke_idx)) {
                    continue;
                }
                ret++;
                ptask->flag = ptask->flag | ADLAK_TASK_CANCELED;
            }
        }
    }
    return ret;
}
static int adlak_invoke_del_with_invokeid(struct adlak_device *padlak, int32_t net_id,
                                          int32_t invoke_id) {
    /*
   - if in pendding list,
     - mutex lock;
     - remove from it,and release buffer
     - mutex unlock
   - if in schedule list,set invalid flag.
   - if in done list ? del or **TODO**
     */

    int                     ret         = 0;
    struct adlak_workqueue *pwq         = &padlak->queue;
    bool                    net_is_used = false;
    adlak_os_mutex_lock(&pwq->wq_mutex);
    AML_LOG_DEBUG("%s", __func__);
    AML_LOG_INFO("invoke del: net_id=%d,invoke_id=%d.", net_id, invoke_id);

    ret = adlak_invoke_del_from_nosch_list(&pwq->finished_list, net_id, invoke_id);
    if (ret > 0) {
        ret = 0;
        if (invoke_id >= 0) {
            goto end;
        }
    }

    ret = adlak_invoke_del_from_nosch_list(&pwq->pending_list, net_id, invoke_id);
    if (0 < ret) {
        ret              = 0;
        pwq->pending_num = pwq->pending_num - ret;
        if (invoke_id >= 0) {
            goto end;
        }
    }

    ret = adlak_invoke_del_from_sch_list(&pwq->scheduled_list, net_id, invoke_id);
    if (0 < ret) {
        ret         = -1;
        net_is_used = true;
    }
end:
    adlak_os_mutex_unlock(&pwq->wq_mutex);
    return ret;
}

int adlak_invoke_del_all(struct adlak_device *padlak, int32_t net_id) {
    AML_LOG_DEBUG("%s", __func__);
    return adlak_invoke_del_with_invokeid(padlak, net_id, -1);
}

static int adlak_net_attach(struct adlak_context *     context,
                            struct adlak_network_desc *psubmit_desc) {
    int                      ret         = 0;
    struct adlak_device *    padlak      = context->padlak;
    struct adlak_model_attr *pmodel_attr = NULL;

    struct adlak_cmq_buffer *cmq_buf_info = NULL;

    AML_LOG_DEBUG("%s", __func__);
    context->macc_count = psubmit_desc->macc_count;

    /*create task*/
    pmodel_attr = adlak_model_create(context, psubmit_desc);
    if (!pmodel_attr) {
        AML_LOG_ERR("adlak task create fail!");
        ret = -1;
        goto err;
    }
    context->pmodel_attr = pmodel_attr;
    ret                  = adlak_invoke_pre_check(pmodel_attr);
    if (ret) {
        adlak_model_destroy(pmodel_attr);

        context->pmodel_attr = NULL;
        goto err;
    }
    adlak_mark_the_last_hw_layer(pmodel_attr);

    /*3.prrogress the command buffer*/
    ret = adlak_net_pre_progress(context);
    if (ret) {
        goto err;
    }

    /*prepare the private command queue*/
    adlak_prepare_command_queue_private(pmodel_attr, psubmit_desc);
    if (ADLAK_CMQ_BUFFER_TYPE_PUBLIC == pmodel_attr->cmq_buffer_type) {
        AML_LOG_INFO("cmq_buffer_type is public");
    } else {
        AML_LOG_INFO("cmq_buffer_type is private");
    }

    if (ADLAK_CMQ_BUFFER_TYPE_PUBLIC == pmodel_attr->cmq_buffer_type) {
        cmq_buf_info = (struct adlak_cmq_buffer *)pmodel_attr->cmq_buffer;
        if (pmodel_attr->size_max_in_layer > cmq_buf_info->size) {
            ret = -1;
            AML_LOG_ERR(
                "the maximum size limit is exceeded which the cmq need (%d),but buffer size "
                "max(%d);"
                "\nPlease increase the size of command queue when insmod.",
                pmodel_attr->size_max_in_layer, cmq_buf_info->size);
            adlak_destroy_command_queue_private(pmodel_attr);
            adlak_model_destroy(pmodel_attr);
            goto err;
        }
    }

    /*add list to workqueue*/
    ret = adlak_os_mutex_lock(&context->context_mutex);
    if (ret) {
        AML_LOG_ERR("mutex lock fail!");
        ret = -1;
        goto err;
    }
    padlak->all_task_num++;
    adlak_os_mutex_unlock(&context->context_mutex);

    return 0;
err:
    return ret;
}

static int adlak_invoke_add_queue(struct adlak_context *            context,
                                  struct adlak_network_invoke_desc *pinvoke_desc) {
    int                     ret    = 0;
    struct adlak_device *   padlak = context->padlak;
    struct adlak_workqueue *pwq    = &padlak->queue;
    struct adlak_task *     ptask  = NULL;
    // struct adlak_network_desc *psubmit_desc = NULL;
    AML_LOG_INFO("%s net_id[%d]", __func__, context->net_id);

    /*create task*/

    ptask = adlak_invoke_create(context, pinvoke_desc);
    if (ADLAK_IS_ERR_OR_NULL(ptask)) {
        AML_LOG_ERR("adlak task create fail!");
        ret = -1;
        goto err;
    }

    /*add list to workqueue*/

    if (ret) {
        AML_LOG_ERR("mutex lock fail!");
        ret = -1;
        goto err;
    }
    ptask->state = ADLAK_SUBMIT_STATE_PENDING;

    context->state = CONTEXT_STATE_USED;
    context->invoke_cnt++;

    ptask->hw_stat.hw_info = padlak->hw_info;

    adlak_os_mutex_lock(&pwq->wq_mutex);
    list_add_tail(&ptask->head, &pwq->pending_list);
    pwq->pending_num++;

    AML_LOG_INFO("pend++,pwq->pending_num = %d\n", pwq->pending_num);
    adlak_os_mutex_unlock(&pwq->wq_mutex);

    return 0;
err:
    return ret;
}

static int adlak_net_register_pre_check(struct adlak_context *     context,
                                        struct adlak_network_desc *psubmit_desc) {
    int                  ret    = 0;
    struct adlak_device *padlak = context->padlak;
    AML_LOG_DEBUG("%s", __func__);
    /*1.if Suppose the maximum count has been reached,then **return err***/
    if (padlak->all_task_num >= padlak->all_task_num_max) {
        ret = -1;
        AML_LOG_WARN("Suppose the maximum count has been reached,Please try again later.");
        goto err;
    }

    return 0;
err:
    return ret;
}

static void adlak_net_pre_progress_part1(struct adlak_context *context) {
    struct adlak_submit_task *psubmitask_base = NULL, *psubmitask = NULL;
    struct adlak_model_attr * pmodel_attr = NULL;
    uint32_t                  task_idx, module_index, active_modules, output_modules, fence_modules;
    const uint32_t            parser_active_modules[ADLAK_PLATFORM_MODULE_COUNT] = {
        PS_CMD_CONFIG_RS_MASK,      // ADLAK_PLATFORM_MODULE_RS
        PS_CMD_CONFIG_RS_CAT_MASK,  // ADLAK_PLATFORM_MODULE_RS_CAT
        PS_CMD_CONFIG_MC_MASK,      // ADLAK_PLATFORM_MODULE_MC
        PS_CMD_CONFIG_DMCF_MASK,    // ADLAK_PLATFORM_MODULE_DMCF
        PS_CMD_CONFIG_DMCW_MASK,    // ADLAK_PLATFORM_MODULE_DMCW
        PS_CMD_CONFIG_PE_MASK,      // ADLAK_PLATFORM_MODULE_PE
        PS_CMD_CONFIG_PE_LUT_MASK,  // ADLAK_PLATFORM_MODULE_PE_LUT
        PS_CMD_CONFIG_DW_MASK,      // ADLAK_PLATFORM_MODULE_DW
        PS_CMD_CONFIG_DMDF_MASK,    // ADLAK_PLATFORM_MODULE_DMDF
        PS_CMD_CONFIG_DMDW_MASK,    // ADLAK_PLATFORM_MODULE_DMDW
        PS_CMD_CONFIG_PX_MASK,      // ADLAK_PLATFORM_MODULE_PX
        PS_CMD_CONFIG_PX_LUT_MASK,  // ADLAK_PLATFORM_MODULE_PX_LUT
        PS_CMD_CONFIG_PWE_MASK,     // ADLAK_PLATFORM_MODULE_PWE
        PS_CMD_CONFIG_PWX_MASK      // ADLAK_PLATFORM_MODULE_PWX
    };

    pmodel_attr     = context->pmodel_attr;
    psubmitask_base = pmodel_attr->submit_tasks;

    for (task_idx = 0; task_idx < pmodel_attr->submit_tasks_num; task_idx++) {
        psubmitask = psubmitask_base + task_idx;
        if (0 >= (psubmitask->config_size + psubmitask->config_v2.common_size)) {
            // software operations
            continue;
        }
        active_modules = 0;
        for (module_index = 0; module_index < ADLAK_PLATFORM_MODULE_COUNT; ++module_index) {
            if (psubmitask->active_modules & (1 << module_index)) {
                active_modules |= parser_active_modules[module_index];
            }
        }
        output_modules = 0;
        if (psubmitask->output_modules & (1 << ADLAK_PLATFORM_MODULE_PWE)) {
            output_modules |= PS_CMD_EXECUTE_OUTPUT_PWE_MASK;
        }

        if (psubmitask->output_modules & (1 << ADLAK_PLATFORM_MODULE_PWX)) {
            output_modules |= PS_CMD_EXECUTE_OUTPUT_PWX_MASK;
        }

        if (psubmitask->output_modules & (1 << ADLAK_PLATFORM_MODULE_RS)) {
            output_modules |= PS_CMD_EXECUTE_OUTPUT_RS_MASK;
        }

        // fence modules
        fence_modules = 0;

        if (psubmitask->fence_modules & (1 << ADLAK_PLATFORM_MODULE_PWE)) {
            fence_modules |= PS_CMD_FENCE_PWE_MASK;
        }

        if (psubmitask->fence_modules & (1 << ADLAK_PLATFORM_MODULE_PWX)) {
            fence_modules |= PS_CMD_FENCE_PWX_MASK;
        }

        if (psubmitask->fence_modules & (1 << ADLAK_PLATFORM_MODULE_RS)) {
            fence_modules |= PS_CMD_FENCE_RS_MASK;
        }

        // update the active_modules && output_modules in tasks;
        psubmitask->active_modules = active_modules;
        psubmitask->output_modules = output_modules;
        psubmitask->fence_modules  = fence_modules;
    }
}

static int adlak_net_pre_progress(struct adlak_context *context) {
    struct adlak_device *     padlak          = context->padlak;
    struct adlak_submit_task *psubmitask_base = NULL, *psubmitask = NULL;
    struct adlak_model_attr * pmodel_attr = NULL;
    uint32_t                  task_idx;
    // uint32_t *                modify_buf, modify_size, cmd_count, cmd_count_max;
    uint32_t *modify_buf, modify_size;
    uint8_t * config_base = NULL;
    /*If the hardware supports parser_v2 modify part of the content, otherwise skip directly*/
    struct adlak_hw_info *phw_info = (struct adlak_hw_info *)padlak->hw_info;
    pmodel_attr                    = context->pmodel_attr;

    AML_LOG_DEBUG("%s", __func__);

    adlak_net_pre_progress_part1(context);

#define ADLAK_VER_PARSR_V2 (0x0400)
    if (phw_info->rev.all < ADLAK_VER_PARSR_V2) {
        // TODO: Accurate judgment conditions need to be determined according to the actual
        // situation the hardware not support parser_v2
        pmodel_attr->hw_parser_v2_support = 0;
        goto end;
    }
    pmodel_attr->hw_parser_v2_support = 1;
    config_base = (uint8_t *)((uintptr_t)adlak_mm_vmap(pmodel_attr->cmd_buf_attr.mm_info));

    psubmitask_base = pmodel_attr->submit_tasks;
    for (task_idx = 0; task_idx < pmodel_attr->submit_tasks_num; task_idx++) {
        psubmitask = psubmitask_base + task_idx;
        if (0 >= psubmitask->config_v2.common_size) {
            // software operations
            continue;
        }
        modify_buf  = (uint32_t *)(config_base + psubmitask->config_v2.modify_offset);
        modify_size = psubmitask->config_v2.modify_size;
        ASSERT(modify_size);
#if 0
        // TODO Populate command to the space of modify_data
        // add exec_cmd

        cmd_count_max = pmodel_attr->cmd_buf_attr.reserve_count_modify_head;
        ASSERT(cmd_count_max >= 4);
        for (cmd_count = 0; cmd_count < cmd_count_max; cmd_count++) {
            if (0 == cmd_count) {
                modify_buf[cmd_count] =
                    (((cmd_count_max - 1) & 0xFF) << 16);  // bypass 0 and modify cmd_count_max
            } else if (cmd_count_max - 1 == cmd_count) {
                modify_buf[cmd_count] = (PS_CMD_CONFIG | active_modules);  // cmd_config
            } else if (cmd_count_max - 2 == cmd_count) {
                modify_buf[cmd_count] = (PS_CMD_EXECUTE | output_modules);  // cmd_execute
            }

            else if (cmd_count_max - 3 == cmd_count) {
                modify_buf[cmd_count] = (PS_CMD_SET_DEPENDENCY | 0);  // TODO set parser dependency
            } else {
                modify_buf[cmd_count] = (PS_CMD_NOP);  // nop
            }
        }

        // add timestamp

        cmd_count_max = pmodel_attr->cmd_buf_attr.reserve_count_modify_tail;

        modify_size = psubmitask->config_v2.modify_size;
        modify_buf  = (uint32_t *)(config_base + psubmitask->config_v2.modify_offset +
                                  (modify_size - cmd_count_max));
        ASSERT(cmd_count_max >= 3);
        for (cmd_count = 0; cmd_count < cmd_count_max; cmd_count++) {
            if (0 == cmd_count) {
                modify_buf[cmd_count] =
                    (((cmd_count_max - 1) & 0xFF) << 16);  // bypass 0 and modify cmd_count_max
            } else if (cmd_count_max - 2 == cmd_count) {
                modify_buf[cmd_count] = (PS_CMD_SET_TIME_STAMP);  // set timestamp
            } else if (cmd_count_max - 1 == cmd_count) {
                modify_buf[cmd_count] = task_idx;  // set timestamp as layer_id
            } else {
                modify_buf[cmd_count] = (PS_CMD_NOP);  // nop
            }
        }
#endif
    }
end:
    return 0;
}

int adlak_net_register_request(struct adlak_context *     context,
                               struct adlak_network_desc *psubmit_desc) {
    int ret = 0;
    AML_LOG_INFO("%s", __func__);
#ifndef CONFIG_ADLA_FREERTOS
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
    if (!adlak_access_ok((void __user *)(uintptr_t)psubmit_desc->tasks_va,
                         sizeof(struct adlak_submit_task) * psubmit_desc->tasks_num))
        return ERR(EFAULT);
    if (!adlak_access_ok((void __user *)(uintptr_t)psubmit_desc->dep_fixups_va,
                         sizeof(struct adlak_submit_dep_fixup) * psubmit_desc->dep_fixups_num))
        return ERR(EFAULT);
    if (!adlak_access_ok((void __user *)(uintptr_t)psubmit_desc->config_va,
                         sizeof(uint8_t) * psubmit_desc->config_total_size))
        return ERR(EFAULT);

#else
    if (!adlak_access_ok(VERIFY_READ, (void __user *)((uintptr_t)psubmit_desc->tasks_va),
                         sizeof(struct adlak_submit_task) * psubmit_desc->tasks_num))
        return ERR(EFAULT);
    if (!adlak_access_ok(VERIFY_READ, (void __user *)(uintptr_t)psubmit_desc->dep_fixups_va,
                         sizeof(struct adlak_submit_dep_fixup) * psubmit_desc->dep_fixups_num))
        return ERR(EFAULT);
    if (!adlak_access_ok(VERIFY_READ, (void __user *)(uintptr_t)psubmit_desc->config_va,
                         sizeof(uint8_t) * psubmit_desc->config_total_size))
        return ERR(EFAULT);
#endif

#endif

    ret = adlak_net_register_pre_check(context, psubmit_desc);
    if (ret) {
        goto err;
    }
    /*2.add to workqueue*/
    ret = adlak_net_attach(context, psubmit_desc);
    if (ret) {
        goto err;
    }

    /*flush context memory DMA_TO_DEVICE*/
    // adlak_context_flush_cache(context); //no need to execute

    return 0;
err:
    return ret;
}
int adlak_net_unregister_request(struct adlak_context *         context,
                                 struct adlak_network_del_desc *submit_del) {
    int                  ret    = 0;
    struct adlak_device *padlak = context->padlak;

    AML_LOG_DEBUG("%s", __func__);

    ret = adlak_invoke_del_with_invokeid(padlak, submit_del->net_register_idx, -1);
    if (0 == ret) {
        adlak_net_dettach_by_id(context, submit_del->net_register_idx);
    }
    return 0;
}

int adlak_invoke_request(struct adlak_context *            context,
                         struct adlak_network_invoke_desc *pinvoke_desc) {
    int                     ret    = 0;
    struct adlak_device *   padlak = NULL;
    struct adlak_workqueue *pwq    = NULL;
    // struct adlak_device *padlak = context->padlak;
    AML_LOG_INFO("%s", __func__);
    AML_LOG_DEBUG("net_id=%d", pinvoke_desc->net_register_idx);
    AML_LOG_DEBUG("invoke_id=%d", pinvoke_desc->invoke_register_idx);
    AML_LOG_DEBUG("invoke start_idx=%d", pinvoke_desc->start_idx);
    AML_LOG_DEBUG("invoke end_idx=%d", pinvoke_desc->end_idx);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "invoke_request");
#endif
#ifndef CONFIG_ADLA_FREERTOS
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
    if (!adlak_access_ok((void __user *)(uintptr_t)pinvoke_desc->addr_fixups_va,
                         sizeof(struct adlak_submit_addr_fixup) * pinvoke_desc->addr_fixups_num))
        return ERR(EFAULT);

#else
    if (!adlak_access_ok(VERIFY_READ, (void __user *)((uintptr_t)pinvoke_desc->addr_fixups_va),
                         sizeof(struct adlak_submit_addr_fixup) * pinvoke_desc->addr_fixups_num))
        return ERR(EFAULT);
#endif

#endif

    adlak_os_mutex_lock(&context->context_mutex);
    ret = adlak_invoke_add_queue(context, pinvoke_desc);

    adlak_os_mutex_unlock(&context->context_mutex);

    padlak = context->padlak;
    pwq    = &padlak->queue;

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "invoke_request done");
#endif
    adlak_os_sema_give(pwq->wk_update);
    adlak_os_thread_yield();

    if (ret) {
        goto err;
    }
    return 0;
err:
    return ret;
}
int adlak_uninvoke_request(struct adlak_context *                context,
                           struct adlak_network_invoke_del_desc *pinvoke_del) {
    int                  ret    = 0;
    struct adlak_device *padlak = context->padlak;
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "uninvoke_request");
#endif

    ret = adlak_invoke_del_with_invokeid(padlak, pinvoke_del->net_register_idx,
                                         pinvoke_del->invoke_register_idx);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "uninvoke_request done");
#endif
    return 0;
}
int adlak_get_status_request(struct adlak_context *context, struct adlak_get_stat_desc *stat_desc) {
    struct adlak_device *   padlak = context->padlak;
    struct adlak_workqueue *pwq    = &padlak->queue;
    struct list_head *      hd;
    struct adlak_task *     ptask = NULL, *ptask_tmp = NULL;
    AML_LOG_DEBUG("%s", __func__);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "status_request");
#endif
    hd                   = &pwq->finished_list;
    stat_desc->ret_state = 1;  // invoke busy

    if (!list_empty(hd)) {
        list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
            if (ptask) {
                if (stat_desc->net_register_idx != ptask->context->net_id) {
                    continue;
                }
                if (stat_desc->invoke_register_idx != ptask->invoke_idx) {
                    continue;
                }
                stat_desc->profile_en       = ptask->profilling.profile_en;
                stat_desc->invoke_time_us   = ptask->profilling.time_elapsed_us;
                stat_desc->start_idx        = ptask->invoke_start_idx;
                stat_desc->end_idx          = ptask->invoke_end_idx;
                stat_desc->profile_rpt      = 0;
                stat_desc->axi_freq_cur     = padlak->clk_axi_freq_real;
                stat_desc->core_freq_cur    = padlak->clk_core_freq_real;
                stat_desc->mem_alloced_umd  = context->mem_alloced;
                stat_desc->mem_alloced_base = padlak->mm->usage.mem_alloced_kmd;
                stat_desc->mem_pool_size    = padlak->mm->usage.mem_pools_size;
                stat_desc->mem_pool_used    = padlak->mm->usage.mem_alloced_kmd +
                                           padlak->mm->usage.mem_alloced_umd +
                                           padlak->mm->share_buf.share_buf_size;
                stat_desc->efficiency = adlak_dmp_get_efficiency(padlak);
                if (ADLAK_SUBMIT_STATE_FINISHED == ptask->state) {
                    stat_desc->ret_state = 0;
                } else {
                    stat_desc->ret_state = -3;  // TODO
                }
                break;
            }
        }
    }
    return 0;
}

int adlak_profile_config(struct adlak_context *         context,
                         struct adlak_profile_cfg_desc *profile_cfg) {
    int                      ret = 0;
    struct adlak_model_attr *pmodel_attr;
    // struct adlak_device *padlak = context->padlak;
    AML_LOG_DEBUG("%s", __func__);
    AML_LOG_DEBUG("net_idx=%d", profile_cfg->net_register_idx);
    AML_LOG_INFO("profile_en=%d", profile_cfg->profile_en);
    AML_LOG_INFO("profile_iova=0x%lX", (uintptr_t)profile_cfg->profile_iova);
    AML_LOG_INFO("profile_buf_size=%lu KByte", (uintptr_t)(profile_cfg->profile_buf_size / 1024));

    adlak_os_mutex_lock(&context->context_mutex);
    pmodel_attr = (context->pmodel_attr);
    if (!pmodel_attr) {
        AML_LOG_ERR("not found network!");
        profile_cfg->errcode = 1;
        ret                  = -1;
        goto err;
    }
    pmodel_attr->pm_cfg.profile_en       = profile_cfg->profile_en;
    pmodel_attr->pm_cfg.profile_iova     = profile_cfg->profile_iova;
    pmodel_attr->pm_cfg.profile_buf_size = profile_cfg->profile_buf_size;
    adlak_os_mutex_unlock(&context->context_mutex);

    profile_cfg->errcode = 0;
    return 0;
err:
    return ret;
}

int adlak_queue_update_task_state(struct adlak_device *padlak, struct adlak_task *ptask) {
    int                     ret      = 0;
    struct adlak_workqueue *pwq      = &padlak->queue;
    struct adlak_hw_stat *  phw_stat = &ptask->hw_stat;
    struct adlak_context *  context  = NULL;
    AML_LOG_DEBUG("%s", __func__);

    ASSERT(ptask);
    if (ADLAK_SUBMIT_STATE_FINISHED != ptask->state && ADLAK_SUBMIT_STATE_FAIL != ptask->state) {
        return -1;
    }

    adlak_status_dump(phw_stat);
    if ((phw_stat->hw_info->irq_cfg.mask_err | ADLAK_IRQ_MASK_SW_TIMEOUT) &
        phw_stat->irq_status.irq_masked) {
        adlak_cmq_dump(ptask);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
        adlak_dbg_dump_module_dump_data(ptask->context);
#endif
    }
    {
        // adlak_context_invalid_cache(ptask->context);

        mb();
        pwq->sched_num--;
        context = ptask->context;
        if (ptask->flag & ADLAK_TASK_CANCELED) {
            AML_LOG_DEBUG("delete the task had canceled which the owner's net_id is %d.",
                          ptask->context->net_id);
            list_del(&ptask->head);
            adlak_invoke_destroy(ptask);

        } else {
            /*move to finished queue*/
            list_move_tail(&ptask->head, &pwq->finished_list);

            pwq->finished_num++;
#ifdef CONFIG_ADLAK_DEBUG_INNNER
            adlak_dbg_inner_update(context, "submit_done");
#endif
            adlak_os_sema_give(context->invoke_state);

            ret = 1;
        }

        if (CONTEXT_STATE_CLOSED == context->state) {
            adlak_os_sema_give(context->ctx_idle);
        }
    }
    return ret;
}

int adlak_parser_preempt(struct adlak_device *padlak, struct adlak_task *ptask) {
    int                      ret;
    struct adlak_model_attr *pmodel_attr  = ptask->context->pmodel_attr;
    struct adlak_cmq_buffer *cmq_buf_info = (struct adlak_cmq_buffer *)pmodel_attr->cmq_buffer;
    AML_LOG_DEBUG("%s", __func__);

    if (ADLAK_SUBMIT_STATE_FINISHED == ptask->state) {
        // adlak_hal_set_preempt(padlak);
        ret = adlak_hal_check_preempt_is_done(padlak);
        if (ERR(NONE) != ret) {
            AML_LOG_WARN("adlak_hal_check_preempt_is_done fail");
            //   goto err;
        }
        ret = adlak_hal_save_parser_info(padlak, &cmq_buf_info->parser_storage_info);
        if (ERR(NONE) != ret) {
            goto err;
        }
    } else {
        /*If submit error, clear the storage info of parser*/
        cmq_buf_info->parser_storage_info.size = 0;
    }
    return 0;
err:
    return ERR(EIO);
}

static int adlak_parser_resume(struct adlak_device *padlak, struct adlak_task *ptask,
                               struct adlak_cmq_buffer *cmq_buf_info) {
    int                   ret;
    uint32_t *            rpt_per_layer;
    int                   resume  = 0;
    struct adlak_context *context = NULL;

    struct adlak_model_attr *pmodel_attr;
    context     = (struct adlak_context *)ptask->context;
    pmodel_attr = context->pmodel_attr;

    if (0 == cmq_buf_info->parser_storage_info.size) {
        // parser_storage_info is not initial
        cmq_buf_info->parser_storage_info.base_addr       = cmq_buf_info->cmq_mm_info->iova_addr;
        cmq_buf_info->parser_storage_info.size            = cmq_buf_info->size;
        cmq_buf_info->parser_storage_info.rpt             = 0;
        cmq_buf_info->parser_storage_info.wpt             = 0;
        cmq_buf_info->parser_storage_info.ppt             = 0;
        cmq_buf_info->parser_storage_info.is_save_from_hw = 0;
    }
    if (ADLAK_CMQ_BUFFER_TYPE_PRIVATE == pmodel_attr->cmq_buffer_type) {
        // the cmq is private

        if (ptask->invoke_start_idx == pmodel_attr->hw_layer_first) {
            cmq_buf_info->parser_storage_info.rpt             = 0;
            cmq_buf_info->parser_storage_info.wpt             = 0;
            cmq_buf_info->parser_storage_info.ppt             = 0;
            cmq_buf_info->parser_storage_info.is_save_from_hw = 0;

        } else {
            rpt_per_layer =
                pmodel_attr->cmq_priv + (pmodel_attr->submit_tasks_num * sizeof(uint32_t)) * 1;
            AML_LOG_DEBUG("rpt_per_layer[%d] = 0x%08X", ptask->invoke_start_idx,
                          rpt_per_layer[ptask->invoke_start_idx]);
            if (cmq_buf_info->parser_storage_info.rpt != rpt_per_layer[ptask->invoke_start_idx]) {
                cmq_buf_info->parser_storage_info.rpt = rpt_per_layer[ptask->invoke_start_idx];
                cmq_buf_info->parser_storage_info.wpt = cmq_buf_info->parser_storage_info.rpt;
                cmq_buf_info->parser_storage_info.ppt = cmq_buf_info->parser_storage_info.rpt;
            }
        }

    } else {
    }
    if (padlak->queue.cmq_buffer_pre != pmodel_attr->cmq_buffer) {
        // cmq changed
        resume                       = 1;
        padlak->queue.cmq_buffer_pre = pmodel_attr->cmq_buffer;
    } else if (ADLAK_CMQ_BUFFER_TYPE_PRIVATE == pmodel_attr->cmq_buffer_type) {
        // the cmq is private
        if (ptask->invoke_start_idx == pmodel_attr->hw_layer_first) {
            resume = 1;
        }
    }
    if (resume != 0) {
        AML_LOG_INFO("parser_resume");
        ret = adlak_hal_parser_resume(padlak, &cmq_buf_info->parser_storage_info);
        if (ERR(NONE) != ret) {
            goto err;
        }
        pmodel_attr->cmq_buffer->cmq_rd_offset = cmq_buf_info->parser_storage_info.rpt;
        pmodel_attr->cmq_buffer->cmq_wr_offset = cmq_buf_info->parser_storage_info.wpt;
    }

    return ERR(NONE);
err:
    return ERR(EIO);
}

int adlak_wait_until_finished(struct adlak_context *      context,
                              struct adlak_get_stat_desc *stat_desc) {
    struct adlak_device *   padlak = context->padlak;
    struct adlak_workqueue *pwq    = &padlak->queue;
    struct adlak_task *     ptask = NULL, *ptask_tmp = NULL;
    int32_t                 finished   = 0;
    int32_t                 find_netid = -1;
    AML_LOG_DEBUG("%s", __func__);
    while (1) {
        if (ERR(NONE) == adlak_os_sema_take_timeout(context->invoke_state, stat_desc->timeout_ms)) {
            find_netid = 0;
            adlak_os_mutex_lock(&pwq->wq_mutex);
            list_for_each_entry_safe(ptask, ptask_tmp, &pwq->finished_list, head) {
                if (ptask && ptask->context->net_id == stat_desc->net_register_idx) {
                    find_netid = 1;
                    if (ptask->invoke_idx == stat_desc->invoke_register_idx) {
                        {
                            finished = 1;
#ifdef CONFIG_ADLAK_DEBUG_INNNER
                            adlak_dbg_inner_update(context, "poll to umd");
#endif
                            break;
                        }
                    }
                }
            }
            adlak_os_mutex_unlock(&pwq->wq_mutex);
        } else {
            finished = -1;
            AML_LOG_WARN("wait timeout");
#ifdef CONFIG_ADLAK_DEBUG_INNNER
            adlak_dbg_inner_update(context, "poll error to umd");
#endif
        }
        if (find_netid == 0) {
            AML_LOG_ERR("Not find valid net_id in finished queue!");
            ASSERT(0);
        }

        if (finished) {
            break;
        }
    }

    if (1 == finished) {
        ASSERT(ptask);
        adlak_os_mutex_lock(&padlak->dev_mutex);
        stat_desc->profile_en       = ptask->context->pmodel_attr->pm_cfg.profile_en;
        stat_desc->invoke_time_us   = ptask->profilling.time_elapsed_us;
        stat_desc->start_idx        = ptask->invoke_start_idx;
        stat_desc->end_idx          = ptask->invoke_end_idx;
        stat_desc->profile_rpt      = 0;
        stat_desc->axi_freq_cur     = padlak->clk_axi_freq_real;
        stat_desc->core_freq_cur    = padlak->clk_core_freq_real;
        stat_desc->mem_alloced_umd  = context->mem_alloced;
        stat_desc->mem_alloced_base = padlak->mm->usage.mem_alloced_kmd;
        stat_desc->mem_pool_size    = padlak->mm->usage.mem_pools_size;
        stat_desc->mem_pool_used    = padlak->mm->usage.mem_alloced_kmd +
                                   padlak->mm->usage.mem_alloced_umd +
                                   padlak->mm->share_buf.share_buf_size;
        stat_desc->efficiency = adlak_dmp_get_efficiency(padlak);
        if (ADLAK_SUBMIT_STATE_FINISHED == ptask->state) {
            stat_desc->ret_state = 0;
        } else {
            stat_desc->ret_state = -3;  // TODO
        }
        adlak_os_mutex_unlock(&padlak->dev_mutex);

    } else if (-1 == finished) {
        stat_desc->ret_state = -1;  // timeout
    } else {
        // not go here
        ASSERT(0);
    }
    return ERR(NONE);
}

int adlak_submit_patch_and_exec(struct adlak_task *ptask) {
    struct adlak_device *    padlak = ptask->context->padlak;
    uint32_t                 invoke_num;
    struct adlak_cmq_buffer *cmq_buf_info = NULL;
    struct adlak_model_attr *pmodel_attr;
    int                      ret;
    pmodel_attr  = ptask->context->pmodel_attr;
    cmq_buf_info = pmodel_attr->cmq_buffer;
    AML_LOG_INFO("%s", __func__);
    if (ptask->state != ADLAK_SUBMIT_STATE_PENDING) {
        ASSERT(0);
        return -1;
    }

    if (padlak->mm->use_smmu) {
        if (ptask->context->smmu_tlb_updated) {
            /*flush tlbs*/
            padlak->mm->smmu_ops->smmu_tlb_flush_cache(padlak->mm);
            /*invalid tlbs*/
            padlak->mm->smmu_ops->smmu_tlb_invalidate(padlak->mm);

            adlak_os_mutex_lock(&ptask->context->context_mutex);
            ptask->context->smmu_tlb_updated = 0;
            adlak_os_mutex_unlock(&ptask->context->context_mutex);
        }
    }

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(ptask->context, "parser_resume");
#endif
    if (adlak_parser_resume(padlak, ptask, cmq_buf_info)) {
        ASSERT(0);
    }
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(ptask->context, "submit_start");
#endif

    adlak_profile_start(padlak, ptask->context,&pmodel_attr->pm_cfg, &pmodel_attr->pm_stat,
    (ptask->invoke_start_idx <= ptask->context->pmodel_attr->hw_layer_first) ? 1 : 0);

    ptask->state = ADLAK_SUBMIT_STATE_RUNNING;

    if (ADLAK_CMQ_BUFFER_TYPE_PRIVATE == pmodel_attr->cmq_buffer_type) {
        ret = adlak_cmq_private_patch_and_exec(ptask);
    } else {
        ret = adlak_cmq_public_patch_and_exec(ptask);
    }
    if (ret) {
        return ret;
    }

    ptask->hw_stat.irq_status.timeout = false;
    invoke_num                        = ptask->invoke_end_idx + 1 - ptask->invoke_start_idx;

    if (padlak->hw_timeout_ms) {
        pmodel_attr->hw_timeout_ms = padlak->hw_timeout_ms * invoke_num;
    } else {
        pmodel_attr->hw_timeout_ms = 10 * invoke_num;
    }
    AML_LOG_DEBUG("%s End", __func__);
    return ERR(NONE);
}

int adlak_submit_patch_and_exec_v2(struct adlak_task *ptask) { return 0; }

static int adlak_cmq_public_patch_and_exec(struct adlak_task *ptask) {
    struct adlak_device *           padlak = ptask->context->padlak;
    uint32_t                        cmq_offset_cfg;
    uint32_t                        cmq_size_u32;
    struct adlak_submit_task *      psubmitask_base = NULL, *psubmitask = NULL;
    uint32_t                        task_idx;
    int                             dependency_mode;
    struct adlak_submit_dep_fixup * psubmit_dep_fixup_base   = NULL;
    struct adlak_submit_reg_fixup * psubmit_reg_fixup_base   = NULL;
    struct adlak_submit_addr_fixup *psubmit_addr_fixups_base = NULL;
    uint8_t *                       config_base              = NULL;
    uint32_t *                      pcmq_buf                 = NULL;
    int32_t                         pwe_flid_offset, pwx_flid_offset, rs_flid_offset;
    int32_t                         start_id_pwe, start_id_pwx, start_id_rs;
    uint32_t                        nop_size;
    uint32_t                        wait_cnt;
    struct adlak_cmq_buffer *       cmq_buf_info;
    struct adlak_model_attr *       pmodel_attr;
    adlak_circular_buffer           circbuffer_cmq;
    adlak_circular_buffer *         pcircbuffer;
    int32_t                         config_offset, config_size;

    struct adlak_sync_cache_ext_info sync_cache_extern;
    AML_LOG_INFO("%s", __func__);

    pmodel_attr  = ptask->context->pmodel_attr;
    cmq_buf_info = pmodel_attr->cmq_buffer;
    pcircbuffer  = &circbuffer_cmq;

    ////////////////////////pattching start////////////////////////////////////////
    // pattching to cmq buffer
    pcmq_buf = adlak_mm_vmap(cmq_buf_info->cmq_mm_info);

    psubmitask_base          = pmodel_attr->submit_tasks;
    psubmit_dep_fixup_base   = pmodel_attr->submit_dep_fixups;
    psubmit_reg_fixup_base   = pmodel_attr->submit_reg_fixups;
    psubmit_addr_fixups_base = pmodel_attr->submit_addr_fixups;
    if (0 == pmodel_attr->cmd_buf_attr.support) {
        config_base = (uint8_t *)pmodel_attr->config;
    } else {
        config_base = (uint8_t *)((uintptr_t)adlak_mm_vmap(pmodel_attr->cmd_buf_attr.mm_info));
    }

    AML_LOG_DEFAULT("tasks_num=%u, ", pmodel_attr->submit_tasks_num);
    AML_LOG_DEFAULT("invoke id[%d-%d], ", ptask->invoke_start_idx, ptask->invoke_end_idx);
    AML_LOG_DEFAULT("dep_fixups_num=%u, ", pmodel_attr->dep_fixups_num);
    AML_LOG_DEFAULT("reg_fixups_num=%u, ", pmodel_attr->reg_fixups_num);
    AML_LOG_DEFAULT("config_total_size=%u, \n", pmodel_attr->config_total_size);

    pwe_flid_offset = (psubmitask_base + ptask->invoke_start_idx)->start_pwe_flid + 1;
    pwx_flid_offset = (psubmitask_base + ptask->invoke_start_idx)->start_pwx_flid + 1;
    rs_flid_offset  = (psubmitask_base + ptask->invoke_start_idx)->start_rs_flid + 1;

    ptask->cmd_offset_start = cmq_buf_info->cmq_wr_offset;

    pcircbuffer->data = adlak_mm_vmap(cmq_buf_info->cmq_mm_info);
    pcircbuffer->size = cmq_buf_info->size / sizeof(uint32_t);
    pcircbuffer->head = cmq_buf_info->cmq_rd_offset / sizeof(uint32_t);
    pcircbuffer->tail = cmq_buf_info->cmq_wr_offset / sizeof(uint32_t);

    // reset parser
    adlak_cmq_write_data(pcircbuffer, PS_CMD_SET_FENCE | 0x700000);  // fence
    adlak_cmq_write_data(pcircbuffer, PS_CMD_RESET_ID);              // reset id
    nop_size = (16 / sizeof(uint32_t)) - 2;
    while (nop_size) {
        nop_size--;
        adlak_cmq_write_data(pcircbuffer, PS_CMD_NOP);  // cmd_nop
    }
    cmq_buf_info->cmq_wr_offset = adlak_cmq_get_cur_tail(pcircbuffer) * sizeof(uint32_t);
    start_id_pwe                = -1;
    start_id_pwx                = -1;
    start_id_rs                 = -1;

    ptask->time_stamp = 0;
    ptask->state      = ADLAK_SUBMIT_STATE_RUNNING;

    for (task_idx = ptask->invoke_start_idx; task_idx <= ptask->invoke_end_idx; task_idx++) {
        psubmitask = psubmitask_base + task_idx;

        if (0 == pmodel_attr->cmd_buf_attr.support) {
            config_offset = psubmitask->config_offset;
            config_size   = psubmitask->config_size;
        } else {
            config_offset = psubmitask->config_v2.common_offset;
            config_size   = psubmitask->config_v2.common_size;
        }
        pcircbuffer->head = cmq_buf_info->cmq_rd_offset / sizeof(uint32_t);
        pcircbuffer->tail = cmq_buf_info->cmq_wr_offset / sizeof(uint32_t);

#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
        AML_LOG_INFO("task_idx=%u", task_idx);
        AML_LOG_DEFAULT("config_offset=%d, ", config_offset);
        AML_LOG_DEFAULT("config_size=%d \n", config_size);
#endif

        // dependency mode
        dependency_mode = ((psubmitask->dependency_mode >= 0) &&
                           (psubmitask->dependency_mode < ADLAK_DEPENDENCY_MODE_COUNT))
                              ? psubmitask->dependency_mode
                              : padlak->dependency_mode;

        // adlak_os_printf("task_idx[%d], dependency_mode: 0x%08X,fence_module: 0x%08X ",
        // task_idx,dependency_mode,fence_module);

        // generate commands
        if (config_size % sizeof(uint32_t)) {
            AML_LOG_INFO("The config_size[%d] is not divisible by 4!", config_size);
            ASSERT(0);
        }

        if (adlak_cmq_remain_space_check(padlak, cmq_buf_info, sizeof(uint32_t) * pcircbuffer->tail,
                                         ADLAK_ALIGN(sizeof(uint32_t) * 7 + config_size, 16))) {
            ptask->hw_stat.irq_status.timeout = false;
            pmodel_attr->hw_timeout_ms        = 100;
            return -1;
        }
        // the readpoint maybe updated
        pcircbuffer->head = cmq_buf_info->cmq_rd_offset / sizeof(uint32_t);

        adlak_cmq_write_data(pcircbuffer,
                             PS_CMD_SET_SW_ID | (task_idx & PS_CMD_SW_ID_MASK));  // cmd_sw_id

        adlak_cmq_write_data(
            pcircbuffer,
            adlak_gen_dep_cmd(dependency_mode, psubmitask, psubmit_dep_fixup_base, pwe_flid_offset,
                              pwx_flid_offset, rs_flid_offset, start_id_pwe, start_id_pwx,
                              start_id_rs));  // cmd_dependcy

        adlak_cmq_write_data(pcircbuffer,
                             PS_CMD_EXECUTE | psubmitask->output_modules);  // cmd_execute

        adlak_cmq_write_data(pcircbuffer,
                             PS_CMD_CONFIG | psubmitask->active_modules);  // cmd_config

        cmq_offset_cfg = adlak_cmq_get_cur_tail(pcircbuffer);
        adlak_cmq_write_buffer(pcircbuffer, (const uint32_t *)(config_base + config_offset),
                               config_size / sizeof(uint32_t));
        if (0 != pmodel_attr->cmd_buf_attr.support) {
            adlak_cmq_merge_modify_data(
                psubmitask, pcircbuffer, cmq_offset_cfg,
                (const uint32_t *)(config_base + psubmitask->config_v2.modify_offset),
                psubmitask->config_v2.modify_size);
        }
        adlak_update_addr_fixups(psubmitask, psubmit_addr_fixups_base, pcircbuffer, cmq_offset_cfg);
        adlak_update_module_dependency(
            dependency_mode, psubmitask, psubmit_dep_fixup_base, pwe_flid_offset, pwx_flid_offset,
            rs_flid_offset, start_id_pwe, start_id_pwx, start_id_rs, pcircbuffer, cmq_offset_cfg);
        adlak_update_reg_fixups(dependency_mode, psubmitask, psubmit_reg_fixup_base, pcircbuffer,
                                cmq_offset_cfg);
        if (task_idx == ptask->invoke_end_idx) {
            adlak_cmq_write_data(
                pcircbuffer, PS_CMD_SET_TIME_STAMP | PS_CMD_TIME_STAMP_IRQ_MASK);  // cmd_time_stamp
        } else {
            adlak_cmq_write_data(pcircbuffer,
                                 PS_CMD_SET_TIME_STAMP);  // cmd_time_stamp
        }
        adlak_cmq_write_data(pcircbuffer, task_idx);  // time_stamp

        adlak_cmq_write_data(pcircbuffer, (PS_CMD_SET_FENCE | psubmitask->fence_modules));
        cmq_size_u32 = 7 + config_size / sizeof(uint32_t);

        if (task_idx == ptask->invoke_end_idx) {
            ptask->time_stamp = task_idx;
        }

        AML_LOG_DEBUG("cmq_size_u32=%u", cmq_size_u32);

        nop_size = ADLAK_ALIGN(cmq_size_u32, (16 / sizeof(uint32_t))) - cmq_size_u32;

        cmq_size_u32 = cmq_size_u32 + nop_size;

        AML_LOG_DEBUG("nop_size=%u", nop_size);

        AML_LOG_DEBUG("cmq_size_u32=%u", cmq_size_u32);
        while (nop_size) {
            adlak_cmq_write_data(pcircbuffer, PS_CMD_NOP);  // cmd_nop
            nop_size--;
        }

        /*flush cmq*/
        sync_cache_extern.is_partial = 0;
        adlak_flush_cache(padlak, cmq_buf_info->cmq_mm_info, &sync_cache_extern);

        cmq_buf_info->cmq_wr_offset = adlak_cmq_get_cur_tail(pcircbuffer) * sizeof(uint32_t);

        mb();

        cmq_buf_info->cmq_rd_offset = adlak_hal_get_ps_rpt(padlak);
        wait_cnt                    = 0;
        while (cmq_buf_info->cmq_wr_offset == cmq_buf_info->cmq_rd_offset) {
            adlak_os_udelay(10);
            cmq_buf_info->cmq_rd_offset = adlak_hal_get_ps_rpt(padlak);
            wait_cnt++;
            if (wait_cnt > 10000) {
                AML_LOG_WARN("cmq_wr_offset=%u , cmq_rd_offset=%u", cmq_buf_info->cmq_wr_offset,
                             cmq_buf_info->cmq_rd_offset);
                break;
            }
        }
        adlak_hal_submit((void *)padlak, cmq_buf_info->cmq_wr_offset);
        ptask->cmd_offset_end = cmq_buf_info->cmq_wr_offset;

        AML_LOG_DEBUG("cmq_wr_offset=%u , cmq_rd_offset=%u", cmq_buf_info->cmq_wr_offset,
                      cmq_buf_info->cmq_rd_offset);

#if CONFIG_ADLAK_EMU_EN
        g_adlak_emu_dev_wpt = cmq_buf_info->cmq_wr_offset;
#endif
    }
    AML_LOG_INFO("cmd_offset_start=0x%08X cmd_offset_end=0x%08X size=0x%08X",
                 (uint32_t)ptask->cmd_offset_start, (uint32_t)ptask->cmd_offset_end,
                 (uint32_t)(ptask->cmd_offset_end + cmq_buf_info->size - ptask->cmd_offset_start) %
                     cmq_buf_info->size);  // Tips: the size does not account for coverage issues

    adlak_cmq_dump(ptask);

    ptask->hw_stat.irq_status.timeout = false;
    return 0;
}

static int adlak_cmq_private_patch_and_exec(struct adlak_task *ptask) {
    struct adlak_device *     padlak = ptask->context->padlak;
    uint32_t                  cmq_offset_cfg;
    struct adlak_submit_task *psubmitask_base = NULL, *psubmitask = NULL;
    uint32_t                  task_idx;
    uint8_t *                 config_base = NULL;
    uint32_t *                pcmq_buf    = NULL;

    uint32_t *               wpt_per_layer, *rpt_per_layer, *cfg_offset;
    struct adlak_cmq_buffer *cmq_buf_info;
    struct adlak_model_attr *pmodel_attr;
    adlak_circular_buffer    circbuffer_cmq;
    adlak_circular_buffer *  pcircbuffer;
    int32_t                  wpt_u32;

    struct adlak_sync_cache_ext_info sync_cache_extern;
    AML_LOG_INFO("%s", __func__);

    pmodel_attr  = ptask->context->pmodel_attr;
    cmq_buf_info = pmodel_attr->cmq_buffer;
    pcircbuffer  = &circbuffer_cmq;

    pcircbuffer->data = adlak_mm_vmap(cmq_buf_info->cmq_mm_info);
    pcircbuffer->size = cmq_buf_info->size / sizeof(uint32_t);
    pcircbuffer->head = pcircbuffer->size - 1;
    pcircbuffer->tail = 0;

    ////////////////////////pattching start////////////////////////////////////////
    // pattching to cmq buffer
    pcmq_buf = adlak_mm_vmap(cmq_buf_info->cmq_mm_info);

    psubmitask_base = pmodel_attr->submit_tasks;
    if (0 == pmodel_attr->cmd_buf_attr.support) {
        config_base = (uint8_t *)pmodel_attr->config;
    } else {
        config_base = (uint8_t *)((uintptr_t)adlak_mm_vmap(pmodel_attr->cmd_buf_attr.mm_info));
    }

    wpt_per_layer = pmodel_attr->cmq_priv;
    rpt_per_layer = pmodel_attr->cmq_priv + (pmodel_attr->submit_tasks_num * sizeof(uint32_t)) * 1;
    cfg_offset    = pmodel_attr->cmq_priv + (pmodel_attr->submit_tasks_num * sizeof(uint32_t)) * 2;

    ptask->state = ADLAK_SUBMIT_STATE_RUNNING;

    for (task_idx = ptask->invoke_start_idx; task_idx <= ptask->invoke_end_idx; task_idx++) {
        psubmitask        = psubmitask_base + task_idx;
        pcircbuffer->tail = cfg_offset[task_idx];
        cmq_offset_cfg    = adlak_cmq_get_cur_tail(pcircbuffer);

        adlak_update_addr_fixups(psubmitask, pmodel_attr->submit_addr_fixups, pcircbuffer,
                                 cmq_offset_cfg);

        if (task_idx == ptask->invoke_end_idx) {
            // update time_stamp
            wpt_u32 = wpt_per_layer[ptask->invoke_end_idx] / sizeof(uint32_t);
            while (PS_CMD_SET_TIME_STAMP != (pcmq_buf[wpt_u32] & 0xFF000000)) {
                wpt_u32--;
            }
            pcmq_buf[wpt_u32] =
                PS_CMD_SET_TIME_STAMP | PS_CMD_TIME_STAMP_IRQ_MASK;  // cmd_time_stamp
            ptask->time_stamp = pcmq_buf[wpt_u32 + 1];

            AML_LOG_INFO("set time_stamp %u", ptask->time_stamp);
        }
    }

    /*flush cmq*/
    sync_cache_extern.is_partial = 0;
    adlak_flush_cache(padlak, cmq_buf_info->cmq_mm_info, &sync_cache_extern);
    cmq_buf_info->cmq_wr_offset = wpt_per_layer[ptask->invoke_end_idx];

    mb();
    AML_LOG_INFO("cmq_wr_offset = 0x%08X ", (uint32_t)cmq_buf_info->cmq_wr_offset);

    adlak_cmq_dump(ptask);
    adlak_hal_submit((void *)padlak, cmq_buf_info->cmq_wr_offset);

#if CONFIG_ADLAK_EMU_EN
    g_adlak_emu_dev_wpt = cmq_buf_info->cmq_wr_offset;
#endif
    ptask->hw_stat.irq_status.timeout = false;

    return 0;
}

static int adlak_cmq_private_fill(struct adlak_model_attr *pmodel_attr) {
    struct adlak_device *           padlak = pmodel_attr->context->padlak;
    uint32_t                        cmq_offset_cfg;
    uint32_t                        cmq_size_u32;
    struct adlak_submit_task *      psubmitask_base = NULL, *psubmitask = NULL;
    uint32_t                        task_idx;
    int                             dependency_mode;
    struct adlak_submit_dep_fixup * psubmit_dep_fixup_base   = NULL;
    struct adlak_submit_reg_fixup * psubmit_reg_fixup_base   = NULL;
    struct adlak_submit_addr_fixup *psubmit_addr_fixups_base = NULL;
    uint8_t *                       config_base              = NULL;
    uint32_t *                      pcmq_buf                 = NULL;
    int32_t                         pwe_flid_offset, pwx_flid_offset, rs_flid_offset;
    int32_t                         start_id_pwe, start_id_pwx, start_id_rs;
    uint32_t                        nop_size;
    struct adlak_cmq_buffer *       cmq_buf_info;
    adlak_circular_buffer           circbuffer_cmq;
    adlak_circular_buffer *         pcircbuffer;
    int32_t                         config_offset, config_size;

    uint32_t *wpt_per_layer, *rpt_per_layer, *cfg_offset;
    int32_t   rpt;
    AML_LOG_INFO("%s", __func__);

    cmq_buf_info = pmodel_attr->cmq_buffer;
    pcircbuffer  = &circbuffer_cmq;

    ////////////////////////pattching start////////////////////////////////////////
    // pattching to cmq buffer
    pcmq_buf = adlak_mm_vmap(cmq_buf_info->cmq_mm_info);

    psubmitask_base          = pmodel_attr->submit_tasks;
    psubmit_dep_fixup_base   = pmodel_attr->submit_dep_fixups;
    psubmit_reg_fixup_base   = pmodel_attr->submit_reg_fixups;
    psubmit_addr_fixups_base = pmodel_attr->submit_addr_fixups;
    if (0 == pmodel_attr->cmd_buf_attr.support) {
        config_base = (uint8_t *)pmodel_attr->config;
    } else {
        config_base = (uint8_t *)((uintptr_t)adlak_mm_vmap(pmodel_attr->cmd_buf_attr.mm_info));
    }

    AML_LOG_DEFAULT("tasks_num=%u, ", pmodel_attr->submit_tasks_num);
    AML_LOG_DEFAULT("dep_fixups_num=%u, ", pmodel_attr->dep_fixups_num);
    AML_LOG_DEFAULT("reg_fixups_num=%u, ", pmodel_attr->reg_fixups_num);
    AML_LOG_DEFAULT("config_total_size=%u, \n", pmodel_attr->config_total_size);

    pwe_flid_offset = (psubmitask_base + pmodel_attr->hw_layer_first)->start_pwe_flid + 1;
    pwx_flid_offset = (psubmitask_base + pmodel_attr->hw_layer_first)->start_pwx_flid + 1;
    rs_flid_offset  = (psubmitask_base + pmodel_attr->hw_layer_first)->start_rs_flid + 1;

    pcircbuffer->data = adlak_mm_vmap(cmq_buf_info->cmq_mm_info);
    pcircbuffer->size = cmq_buf_info->size / sizeof(uint32_t);
    pcircbuffer->head = 0;
    pcircbuffer->tail = 0;

#if 1
    start_id_pwe = -1;
    start_id_pwx = -1;
    start_id_rs  = -1;

    pmodel_attr->cmq_priv =
        adlak_os_zalloc(((sizeof(uint32_t) * pmodel_attr->submit_tasks_num) * 3), ADLAK_GFP_KERNEL);
    wpt_per_layer = pmodel_attr->cmq_priv;
    rpt_per_layer = pmodel_attr->cmq_priv + (pmodel_attr->submit_tasks_num * sizeof(uint32_t)) * 1;
    cfg_offset    = pmodel_attr->cmq_priv + (pmodel_attr->submit_tasks_num * sizeof(uint32_t)) * 2;

#endif
    rpt = 0;
    for (task_idx = 0; task_idx <= pmodel_attr->hw_layer_last; task_idx++) {
        rpt_per_layer[task_idx] = rpt;
        AML_LOG_INFO("set rpt_per_layer[%d] = 0x%08X", task_idx, rpt_per_layer[task_idx]);

        psubmitask = psubmitask_base + task_idx;

        if (0 == pmodel_attr->cmd_buf_attr.support) {
            config_offset = psubmitask->config_offset;
            config_size   = psubmitask->config_size;
        } else {
            config_offset = psubmitask->config_v2.common_offset;
            config_size   = psubmitask->config_v2.common_size;
        }
        if (0 >= config_size) {
            // software operations
            continue;
        }

#if ADLAK_DEBUG_CMQ_PATTTCHING_EN
        AML_LOG_INFO("task_idx=%u", task_idx);
        AML_LOG_DEFAULT("config_offset=%d, ", config_offset);
        AML_LOG_DEFAULT("config_size=%d \n", config_size);
#endif

        // dependency mode
        dependency_mode = ((psubmitask->dependency_mode >= 0) &&
                           (psubmitask->dependency_mode < ADLAK_DEPENDENCY_MODE_COUNT))
                              ? psubmitask->dependency_mode
                              : padlak->dependency_mode;

        // generate commands
        if (config_size % sizeof(uint32_t)) {
            AML_LOG_INFO("The config_size[%d] is not divisible by 4!", config_size);
            ASSERT(0);
        }

        adlak_cmq_write_data(pcircbuffer,
                             PS_CMD_SET_SW_ID | (task_idx & PS_CMD_SW_ID_MASK));  // cmd_sw_id

        adlak_cmq_write_data(
            pcircbuffer,
            adlak_gen_dep_cmd(dependency_mode, psubmitask, psubmit_dep_fixup_base, pwe_flid_offset,
                              pwx_flid_offset, rs_flid_offset, start_id_pwe, start_id_pwx,
                              start_id_rs));  // cmd_dependcy

        adlak_cmq_write_data(pcircbuffer,
                             PS_CMD_EXECUTE | psubmitask->output_modules);  // cmd_execute

        adlak_cmq_write_data(pcircbuffer,
                             PS_CMD_CONFIG | psubmitask->active_modules);  // cmd_config

        cmq_offset_cfg       = adlak_cmq_get_cur_tail(pcircbuffer);
        cfg_offset[task_idx] = cmq_offset_cfg;
        adlak_cmq_write_buffer(pcircbuffer, (const uint32_t *)(config_base + config_offset),
                               config_size / sizeof(uint32_t));
        if (0 != pmodel_attr->cmd_buf_attr.support) {
            adlak_cmq_merge_modify_data(
                psubmitask, pcircbuffer, cmq_offset_cfg,
                (const uint32_t *)(config_base + psubmitask->config_v2.modify_offset),
                psubmitask->config_v2.modify_size);
        }

        adlak_update_module_dependency(
            dependency_mode, psubmitask, psubmit_dep_fixup_base, pwe_flid_offset, pwx_flid_offset,
            rs_flid_offset, start_id_pwe, start_id_pwx, start_id_rs, pcircbuffer, cmq_offset_cfg);
        adlak_update_reg_fixups(dependency_mode, psubmitask, psubmit_reg_fixup_base, pcircbuffer,
                                cmq_offset_cfg);

        adlak_cmq_write_data(pcircbuffer,
                             PS_CMD_SET_TIME_STAMP);  // cmd_time_stamp

        adlak_cmq_write_data(pcircbuffer, task_idx);  // time_stamp

        adlak_cmq_write_data(pcircbuffer, (PS_CMD_SET_FENCE | psubmitask->fence_modules));

        cmq_size_u32 = 7 + config_size / sizeof(uint32_t);

        AML_LOG_DEBUG("cmq_size_u32=%u", cmq_size_u32);

        nop_size = ADLAK_ALIGN(cmq_size_u32, (16 / sizeof(uint32_t))) - cmq_size_u32;

        cmq_size_u32 = cmq_size_u32 + nop_size;

        AML_LOG_DEBUG("nop_size=%u", nop_size);

        while (nop_size) {
            adlak_cmq_write_data(pcircbuffer, PS_CMD_NOP);  // cmd_nop
            nop_size--;
        }

        AML_LOG_DEBUG("cmq_size_u32=%u", cmq_size_u32);

        wpt_per_layer[task_idx] = adlak_cmq_get_cur_tail(pcircbuffer) * sizeof(uint32_t);
        AML_LOG_INFO("set wpt_per_layer[%d] = 0x%08X", task_idx, wpt_per_layer[task_idx]);
        rpt = wpt_per_layer[task_idx];
    }

    return 0;
}

void adlak_destroy_command_queue_private(struct adlak_model_attr *pmodel_attr) {
    struct adlak_cmq_buffer *cmq_buf_info = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (ADLAK_CMQ_BUFFER_TYPE_PUBLIC == pmodel_attr->cmq_buffer_type) {
        pmodel_attr->cmq_buffer = NULL;
        return;
    }
    cmq_buf_info = pmodel_attr->cmq_buffer;
    adlak_cmq_buf_free(((struct adlak_device *)(pmodel_attr->context->padlak))->mm,
                       cmq_buf_info->cmq_mm_info);
    adlak_os_free(cmq_buf_info);
    pmodel_attr->cmq_buffer = NULL;
}

void adlak_prepare_command_queue_private(struct adlak_model_attr *  pmodel_attr,
                                         struct adlak_network_desc *psubmit_desc) {
    uint32_t                 alloc_size;
    struct adlak_cmq_buffer *cmq_buf_info = NULL;
    struct adlak_device *    padlak       = pmodel_attr->context->padlak;
    AML_LOG_DEBUG("%s", __func__);
#ifdef CONFIG_ADLAK_DEBUG_CMQ_TYPE
    if (0 == (psubmit_desc->net_register_idx % 2)) {
        psubmit_desc->cmq_buffer_type = ADLAK_CMQ_BUFFER_TYPE_PUBLIC;
    } else {
        psubmit_desc->cmq_buffer_type = ADLAK_CMQ_BUFFER_TYPE_PRIVATE;
    }
#endif
    pmodel_attr->cmq_buffer_type = ADLAK_CMQ_BUFFER_TYPE_PUBLIC;
    if (ADLAK_CMQ_BUFFER_TYPE_PUBLIC == psubmit_desc->cmq_buffer_type) {
        goto err;
    }
    cmq_buf_info = adlak_os_zalloc(sizeof(struct adlak_cmq_buffer), ADLAK_GFP_KERNEL);
    if (!cmq_buf_info) {
        goto err;
    }

    pmodel_attr->cmq_buffer = cmq_buf_info;

    alloc_size = pmodel_attr->cmq_size_expected;
    alloc_size = ADLAK_ALIGN(alloc_size, 4096);
    AML_LOG_INFO("alloc buffer for cmq(private),size_expected=0x%08x,size_alloc=0x%08x",
                 pmodel_attr->cmq_size_expected, alloc_size);

    cmq_buf_info->cmq_mm_info =
        adlak_cmq_buf_alloc(((struct adlak_device *)(padlak))->mm, alloc_size);
    if (NULL == cmq_buf_info->cmq_mm_info) {
        AML_LOG_ERR("alloc buffer for cmq(private) failed!");
        goto err;
    }
    pmodel_attr->cmq_buffer_type = ADLAK_CMQ_BUFFER_TYPE_PRIVATE;
    cmq_buf_info->size           = alloc_size;

    cmq_buf_info->cmq_wr_offset = 0;
    cmq_buf_info->cmq_rd_offset = 0;

    /* Fill the command queue ahead of time*/
    adlak_cmq_private_fill(pmodel_attr);

    return;
err:
    if (cmq_buf_info) {
        adlak_os_free(cmq_buf_info);
    }

    pmodel_attr->cmq_buffer = &padlak->cmq_buffer_public;
    return;
}
