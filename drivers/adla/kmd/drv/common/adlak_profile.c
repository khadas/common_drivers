/*******************************************************************************
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_profile.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2021/08/26	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_profile.h"

#include "adlak_submit.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

int adlak_profile_start(struct adlak_device *padlak, struct adlak_context *context, struct adlak_pm_cfg *pm_cfg,
                        struct adlak_pm_state *pm_stat, int32_t layer_start) {
    uint32_t wpt;
    AML_LOG_DEBUG("%s", __func__);

    ASSERT(padlak);
    ASSERT(pm_cfg);
    ASSERT(pm_stat);
    if (1 == pm_cfg->profile_en) {
        pm_stat->start = adlak_os_ktime_get();
        adlak_pm_enable(padlak, true);
        adlak_check_dev_is_idle(padlak);
        adlak_pm_reset(padlak);
        wpt = ADLAK_ALIGN(pm_stat->pm_wpt, 256);
        adlak_pm_config(padlak, pm_cfg->profile_iova, pm_cfg->profile_buf_size, wpt);
        if (layer_start) {
            context->invoke_time_elapsed_tmp = 0;
        }
    } else if (padlak->save_time_en) {
        pm_stat->start = adlak_os_ktime_get();
        if (layer_start == 1) {
            context->invoke_time_elapsed_tmp = 0;
        }
    }
    return 0;
}

int adlak_profile_stop(struct adlak_device *padlak, struct adlak_context *context, struct adlak_pm_cfg *pm_cfg,
                       struct adlak_pm_state *pm_stat, struct adlak_profile *profile_data,
                       int32_t layer_end) {
    uint32_t               time_elapsed_us = 0;
    AML_LOG_DEBUG("%s", __func__);
    ASSERT(padlak);
    ASSERT(pm_cfg);
    ASSERT(pm_stat);
    if (1 == pm_cfg->profile_en) {
        adlak_pm_fush_until_empty(padlak);
        if (1 == layer_end) {
            pm_stat->pm_wpt = 0;
            pm_stat->pm_rpt = 0;
        } else {
            pm_stat->pm_wpt = adlak_pm_get_stat(padlak);  // recoder the write point
#if CONFIG_ADLAK_EMU_EN
            pm_stat->pm_wpt = ((struct adlak_task *)padlak->queue.ptask_sch_cur)
                                  ->context->pmodel_attr->submit_tasks_num *
                              256;

#endif
        }
        adlak_pm_enable(padlak, false);
        pm_stat->finish = adlak_os_ktime_get();
        profile_data->time_elapsed_us =
            (uint32_t)adlak_os_ktime_us_delta(pm_stat->finish, pm_stat->start);
        AML_LOG_DEBUG("pm used %d ms.", profile_data->time_elapsed_us / 1000);
        context->invoke_time_elapsed_tmp += profile_data->time_elapsed_us;
        if (layer_end == 1) {
            context->invoke_time_elapsed_total = context->invoke_time_elapsed_tmp;
        }
    } else if (padlak->save_time_en) {
        pm_stat->finish = adlak_os_ktime_get();
        if (!pm_stat->start) {
            time_elapsed_us =(uint32_t)adlak_os_ktime_us_delta(pm_stat->finish, pm_stat->start);
        }
        context->invoke_time_elapsed_tmp += time_elapsed_us;
        if (layer_end == 1) {
            context->invoke_time_elapsed_total = context->invoke_time_elapsed_tmp;
        }
    }

    return 0;
}
