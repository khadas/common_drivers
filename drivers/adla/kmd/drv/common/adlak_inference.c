/*******************************************************************************
 * Copyright (C) 2022 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_inference.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2022/04/10	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_dpm.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

enum ADLAK_DEVICE_STATE {
    ADLAK_DEVICE_INIT = 1,
    ADLAK_DEVICE_ERR,
    ADLAK_DEVICE_IDLE,
    ADLAK_DEVICE_BUSY,
};

enum ADLAK_INFERENCE_STATE {
    ADLAK_INFERENCE_STATE_INIT = 0,
    ADLAK_INFERENCE_STATE_RESUME,
    ADLAK_INFERENCE_STATE_DEV_RESET,
    ADLAK_INFERENCE_STATE_WQ_RESET,
    ADLAK_INFERENCE_STATE_WQ_CHECK,
    ADLAK_INFERENCE_STATE_SLEEP_CHECK,
    ADLAK_INFERENCE_STATE_POST_TASK,
    ADLAK_INFERENCE_STATE_REPORT_TASK_PRE,
    ADLAK_INFERENCE_STATE_REPORT_TASK_PRE2,
    ADLAK_INFERENCE_STATE_SUBMIT_WAIT,
    ADLAK_INFERENCE_STATE_CHECK_TASK,
    ADLAK_INFERENCE_STATE_REPORT_TASK_CUR,
    ADLAK_INFERENCE_STATE_COUNT
};

/************************** Variable Definitions *****************************/

static struct adlak_workqueue *g_adlak_pwq = NULL;

/************************** Function Prototypes ******************************/

static void adlak_irq_bottom_half(struct adlak_device *padlak, int *device_state,
                                  struct adlak_task *ptask);

int adlak_submit_wait(struct adlak_dev_inference *pinference, struct adlak_task *ptask) {
    int                  ret;
    int                  repeat = 0;
    uint32_t             timeout;
    struct adlak_device *padlak = ptask->context->padlak;
    AML_LOG_INFO("%s", __func__);
#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
    /* Suppose ptask->sw_timeout_ms_ms is greater than 5 */
    adlak_os_timer_add(&pinference->emu_timer, 5);
#endif

    AML_LOG_INFO("set submit hw_timeout_ms=%u ms", ptask->context->pmodel_attr->hw_timeout_ms);
    do {
        ret = adlak_os_sema_take_timeout(pinference->sem_irq,
                                         ptask->context->pmodel_attr->hw_timeout_ms);
        if (ERR(NONE) == ret) {
            AML_LOG_INFO("%s\n", "sema_take success");
            timeout = false;
            break;
        } else {
            AML_LOG_WARN("%s\n", "sema_take_timeout");
            timeout = true;
#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
            adlak_os_timer_del(&pinference->emu_timer);
            break;
#endif
        }
        repeat++;
    } while (repeat < 2);
    ptask->hw_stat.irq_status.timeout = timeout;
    if (true == timeout) {
        adlak_os_spinlock_lock(&padlak->spinlock);
        adlak_irq_proc(padlak);
        adlak_os_spinlock_unlock(&padlak->spinlock);
    }
    return ret;
}

static void adlak_debug_print_device_state(uint32_t device_state) {
#if ADLAK_DEBUG
    switch (device_state) {
        case ADLAK_DEVICE_INIT:
            AML_LOG_INFO("device state = %s !\n", "ADLAK_DEVICE_INIT");
            break;
        case ADLAK_DEVICE_ERR:
            AML_LOG_INFO("device state = %s !\n", "ADLAK_DEVICE_ERR");
            break;
        case ADLAK_DEVICE_IDLE:
            AML_LOG_INFO("device state = %s !\n", "ADLAK_DEVICE_IDLE");
            break;
        case ADLAK_DEVICE_BUSY:
            AML_LOG_INFO("device state = %s !\n", "ADLAK_DEVICE_BUSY");
            break;
        default:
            AML_LOG_INFO("device state = %d !\n", device_state);
            break;
    }
#endif
}

static void adlak_dpm_timer_cb(adlak_os_timer_cb_t t) {
    struct adlak_workqueue *pwq            = g_adlak_pwq;
    static int              submit_num_pre = -1;

    adlak_os_spinlock_lock(&pwq->dev_inference.spinlock);
    if (pwq->submit_num == submit_num_pre) {
        pwq->dev_inference.wq_idel_cnt += 1;
        pwq->dev_inference.cnt_idel += 1;
        adlak_os_sema_give(pwq->wk_update);
    } else {
        submit_num_pre                 = pwq->submit_num;
        pwq->dev_inference.wq_idel_cnt = 0;
        pwq->dev_inference.cnt_busy += 1;
    }
    pwq->dev_inference.cnt_elapsed++;
    if (pwq->dev_inference.cnt_elapsed == 0) {
        pwq->dev_inference.cnt_busy = 0;
        pwq->dev_inference.cnt_idel = 0;
    }
    adlak_os_timer_modify(&pwq->dev_inference.dpm_timer, pwq->dev_inference.dpm_period_set);

    adlak_os_spinlock_unlock(&pwq->dev_inference.spinlock);
}

static int32_t adlak_get_valid_num(struct adlak_workqueue *pwq, int32_t *net_id_pre) {
    /* if the net_id of context has been destroyed,the net_id will be set to -1*/
    int32_t            valid_num = 0;
    int                find      = 0;
    struct adlak_task *ptask = NULL, *ptask_tmp = NULL;
    int32_t            net_id = *net_id_pre;
    if (net_id < 0) {
        valid_num = pwq->pending_num;
    } else {
        /*the model not invoke to the end*/
        if (pwq->pending_num > 0) {
            list_for_each_entry_safe(ptask, ptask_tmp, &pwq->pending_list, head) {
                if (net_id == ptask->context->net_id) {
                    if (CONTEXT_STATE_CLOSED == ptask->context->state) {
                        /*the net_id of context has been destroyed.*/
                        *net_id_pre = -1;
                    } else {
                        find = 1;
                    }
                    break;
                }
            }
        }
        net_id = *net_id_pre;
        if (net_id < 0) {
            valid_num = pwq->pending_num;
        } else {
            valid_num = find;
        }
    }
    return valid_num;
}

#ifndef CONFIG_ADLA_FREERTOS
static int adlak_dev_inference_cb(void *args) {
#else
static void *adlak_dev_inference_cb(void *args) {
#endif

    int                     ret;
    struct adlak_device *   padlak        = args;
    struct adlak_workqueue *pwq           = &padlak->queue;
    adlak_os_thread_t *     pthrd         = &pwq->dev_inference.thrd_inference;
    struct adlak_task *     ptask_sch_cur = NULL, *ptask_sch_pre = NULL;
    int32_t                 net_id_invoke_previous = -1;
    int                     inference_state        = ADLAK_INFERENCE_STATE_INIT;
    int                     device_state, dpm_stategy, valid_num, pending_num;

#ifdef CONFIG_PM
    int pm_suspend;
#endif
    ret = adlak_os_timer_init(&pwq->dev_inference.dpm_timer, adlak_dpm_timer_cb, NULL);
    if (ret) {
        AML_LOG_ERR("dpm_timer init fail!\n");
#ifndef CONFIG_ADLA_FREERTOS
        return 0;
#else
        return NULL;
#endif
    }

#if CONFIG_ADLAK_DPM_EN
    pwq->dev_inference.dpm_period_set = padlak->dpm_period_set;
    AML_LOG_WARN("dpm_timer_period: %d\n", pwq->dev_inference.dpm_period_set);
    adlak_os_timer_add(&pwq->dev_inference.dpm_timer, pwq->dev_inference.dpm_period_set);
#endif

    while (!pthrd->thrd_should_stop) {
        // adlak_might_sleep();
        AML_LOG_DEBUG("%s\n", __func__);

#ifdef CONFIG_PM
        adlak_os_mutex_lock(&padlak->dev_mutex);
        pm_suspend = padlak->pm_suspend;
        adlak_os_mutex_unlock(&padlak->dev_mutex);
#endif
        switch (inference_state) {
            case ADLAK_INFERENCE_STATE_INIT:
                dpm_stategy     = ADLAK_DPM_STRATEGY_MIN;
                device_state    = ADLAK_DEVICE_INIT;
                inference_state = ADLAK_INFERENCE_STATE_RESUME;
                break;
            case ADLAK_INFERENCE_STATE_RESUME:
                // do resume
                if (ADLAK_DPM_STRATEGY_MAX != dpm_stategy) {
                    dpm_stategy = ADLAK_DPM_STRATEGY_MAX;
                    adlak_dpm_stage_adjust(padlak, dpm_stategy);
                    inference_state = ADLAK_INFERENCE_STATE_DEV_RESET;
                } else {
                    AML_LOG_ERR("%s\n", "shouldn't be here");
                    ASSERT(0);
                }
                break;
            case ADLAK_INFERENCE_STATE_DEV_RESET:
                // reset device
                adlak_hal_reset_and_start((void *)padlak);
                device_state    = ADLAK_DEVICE_IDLE;
                inference_state = ADLAK_INFERENCE_STATE_WQ_RESET;

                break;
            case ADLAK_INFERENCE_STATE_WQ_RESET:
                // reset workqueue
                adlak_os_mutex_lock(&pwq->wq_mutex);
                adlak_queue_reset(padlak);
                ptask_sch_pre                = NULL;
                ptask_sch_cur                = NULL;
                pwq->ptask_sch_cur           = ptask_sch_cur;
                padlak->queue.cmq_buffer_pre = NULL;
                adlak_os_mutex_unlock(&pwq->wq_mutex);

                inference_state = ADLAK_INFERENCE_STATE_WQ_CHECK;
                break;
            case ADLAK_INFERENCE_STATE_WQ_CHECK:

                adlak_os_mutex_lock(&pwq->wq_mutex);
                valid_num   = adlak_get_valid_num(pwq, &net_id_invoke_previous);
                pending_num = pwq->pending_num;
                adlak_os_mutex_unlock(&pwq->wq_mutex);
                if (0 == valid_num) {
                    if (ptask_sch_pre) {
                        inference_state = ADLAK_INFERENCE_STATE_REPORT_TASK_PRE2;
                        break;
                    }
                }
                adlak_os_sema_take(pwq->wk_update);

                adlak_os_mutex_lock(&pwq->wq_mutex);
                valid_num   = adlak_get_valid_num(pwq, &net_id_invoke_previous);
                pending_num = pwq->pending_num;
                adlak_os_mutex_unlock(&pwq->wq_mutex);
                if ((0 == valid_num) && (0 < pending_num)) {
                    ASSERT(padlak->share_swap_en);
                    adlak_os_sema_give(pwq->wk_update);
                    adlak_os_thread_yield();
                    adlak_os_msleep(1); /* must sleep to free up CPU,if the thread's priority is
                                           particularly high.*/
                }

                // check workqueue
                if (0 < valid_num) {
                    if (ADLAK_DPM_STRATEGY_MIN == dpm_stategy) {
                        pwq->dev_inference.wq_idel_cnt = 0;
                        inference_state                = ADLAK_INFERENCE_STATE_RESUME;
                        adlak_os_sema_give(pwq->wk_update);
                    } else {
                        inference_state = ADLAK_INFERENCE_STATE_POST_TASK;
                    }
                } else {
                    inference_state = ADLAK_INFERENCE_STATE_SLEEP_CHECK;
                }
#ifdef CONFIG_PM
                if (true == pm_suspend) {
                    if (ptask_sch_pre) {
                        inference_state = ADLAK_INFERENCE_STATE_REPORT_TASK_PRE2;
                    } else {
                        inference_state = ADLAK_INFERENCE_STATE_SLEEP_CHECK;
                    }
                }
#endif
                break;
            case ADLAK_INFERENCE_STATE_SLEEP_CHECK:
                // do_sleep
                if (0 == pending_num) {  // valid_num must be 0
                    AML_LOG_INFO("nothing need to do!\n");
                    if (10 <= pwq->dev_inference.wq_idel_cnt) {
                        /*If CONFIG_ADLAK_DPM_EN==0, here can never be reached*/
                        //if (ADLAK_DPM_STRATEGY_MIN != dpm_stategy) {
                            if (ptask_sch_pre || ptask_sch_cur) {
                                ASSERT(0);
                            }
                            dpm_stategy = ADLAK_DPM_STRATEGY_MIN;
                            adlak_dpm_stage_adjust(padlak, ADLAK_DPM_STRATEGY_MIN);
                        //}
                    }
                }
#ifdef CONFIG_PM

                if (true == pm_suspend && device_state == ADLAK_DEVICE_IDLE) {
                    dpm_stategy = ADLAK_DPM_STRATEGY_MIN;
                    adlak_dpm_stage_adjust(padlak, ADLAK_DPM_STRATEGY_MIN);
                    adlak_os_mutex_lock(&padlak->dev_mutex);
                    padlak->pm_suspend = false;
                    adlak_os_mutex_unlock(&padlak->dev_mutex);
                    adlak_os_sema_take(padlak->sem_pm_wakeup);
                }
#endif
                inference_state = ADLAK_INFERENCE_STATE_WQ_CHECK;
                break;
            case ADLAK_INFERENCE_STATE_POST_TASK:
                // submit task to hardware
                ASSERT(NULL == ptask_sch_cur);
                ret = adlak_queue_schedule_update(padlak, &ptask_sch_cur, net_id_invoke_previous);
                if (ret) {
                    inference_state = ADLAK_INFERENCE_STATE_WQ_CHECK;
                    break;
                }

                pwq->ptask_sch_cur = ptask_sch_cur;
                pwq->submit_num++;
                if (0 == ptask_sch_cur->context->pmodel_attr->hw_parser_v2_support) {
                    (void)adlak_submit_patch_and_exec(ptask_sch_cur);
                } else {
                    (void)adlak_submit_patch_and_exec_v2(ptask_sch_cur);
                }
                net_id_invoke_previous = -1;
                if (padlak->share_swap_en) {
                    if (ptask_sch_cur->invoke_end_idx <
                        ptask_sch_cur->context->pmodel_attr->hw_layer_last) {
                        net_id_invoke_previous = ptask_sch_cur->context->net_id;
                    }
                }

                device_state    = ADLAK_DEVICE_BUSY;
                inference_state = ADLAK_INFERENCE_STATE_REPORT_TASK_PRE;
                break;
            case ADLAK_INFERENCE_STATE_REPORT_TASK_PRE:
            case ADLAK_INFERENCE_STATE_REPORT_TASK_PRE2:
                if (ptask_sch_pre) {
                    adlak_os_mutex_lock(&pwq->wq_mutex);
                    ret = adlak_queue_update_task_state(padlak, (struct adlak_task *)ptask_sch_pre);
                    adlak_os_mutex_unlock(&pwq->wq_mutex);
                    if (1 == ret) {
                        adlak_to_umd_sinal_give(
                            ptask_sch_pre->context->wait); /*give signal to umd*/
                    }
                    // report state
                    ptask_sch_pre = NULL;
                }
                if (ADLAK_INFERENCE_STATE_REPORT_TASK_PRE == inference_state) {
                    inference_state = ADLAK_INFERENCE_STATE_SUBMIT_WAIT;
                } else {
                    inference_state = ADLAK_INFERENCE_STATE_WQ_CHECK;
                }
                break;

            case ADLAK_INFERENCE_STATE_SUBMIT_WAIT:
                // wait until finished
                if (ERR(NONE) != adlak_submit_wait(&pwq->dev_inference, ptask_sch_cur)) {
                    AML_LOG_ERR("%s\n", "submit timeout");
                    ptask_sch_cur->hw_stat.irq_status.timeout = true;
                } else {
                    AML_LOG_INFO("%s\n", "submit success");
                }
                adlak_irq_bottom_half(padlak, &device_state, ptask_sch_cur);
                if (adlak_parser_preempt(padlak, ptask_sch_cur)) {
                    ASSERT(0);
                }
                adlak_debug_print_device_state(device_state);

                adlak_debug_invoke_list_dump(padlak, 1);
                inference_state = ADLAK_INFERENCE_STATE_CHECK_TASK;
                break;
            case ADLAK_INFERENCE_STATE_CHECK_TASK:
                if (ptask_sch_cur->state == ADLAK_SUBMIT_STATE_FINISHED) {
                    // store task
                    ptask_sch_pre      = ptask_sch_cur;
                    ptask_sch_cur      = NULL;
                    pwq->ptask_sch_cur = ptask_sch_cur;
                    inference_state    = ADLAK_INFERENCE_STATE_WQ_CHECK;
                } else if (ptask_sch_cur->state == ADLAK_SUBMIT_STATE_FAIL) {
                    inference_state = ADLAK_INFERENCE_STATE_REPORT_TASK_CUR;
                } else {
                    AML_LOG_ERR("invalid task_state %d\n", ptask_sch_cur->state);
                    ASSERT(0);
                }
                break;
            case ADLAK_INFERENCE_STATE_REPORT_TASK_CUR:
                ASSERT(NULL != ptask_sch_cur);
                adlak_os_mutex_lock(&pwq->wq_mutex);
                AML_LOG_INFO("adlak_queue_update_task_state cur %d !\n", __LINE__);
                ret = adlak_queue_update_task_state(padlak, (struct adlak_task *)ptask_sch_cur);
                adlak_os_mutex_unlock(&pwq->wq_mutex);
                if (1 == ret) {
                    adlak_to_umd_sinal_give(ptask_sch_cur->context->wait); /*give signal to umd*/
                }
                if (ret >= 0) {
                    ptask_sch_cur      = NULL;
                    pwq->ptask_sch_cur = ptask_sch_cur;
                    AML_LOG_INFO("adlak_queue_update_task_state finished cur %d !\n", __LINE__);
                }
                inference_state = ADLAK_INFERENCE_STATE_DEV_RESET;
                break;
            default:
                AML_LOG_ERR("unknown state\n");
                break;
        }
    }
    pthrd->thrd_should_stop = 0;

    if (pwq->dev_inference.dpm_timer) {
        adlak_os_timer_destroy(&pwq->dev_inference.dpm_timer);
    }
#ifndef CONFIG_ADLA_FREERTOS
    return 0;
#else
    return NULL;
#endif
}

#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)

static void adlak_emu_irq_cb(adlak_os_timer_cb_t t) {
    struct adlak_hw_stat *      phw_stat   = NULL;
    struct adlak_dev_inference *pinference = NULL;
    struct adlak_task *         ptask      = NULL;
    AML_LOG_DEBUG("%s\n", __func__);
    adlak_cant_sleep();
    pinference                      = &g_adlak_pwq->dev_inference;
    ptask                           = (struct adlak_task *)(g_adlak_pwq->ptask_sch_cur);
    phw_stat                        = &ptask->hw_stat;
    phw_stat->irq_status.irq_masked = ADLAK_IRQ_MASK_TIM_STAMP;
    phw_stat->irq_status.irq_masked |= ADLAK_IRQ_MASK_LAYER_END;
    phw_stat->irq_status.time_stamp = ptask->time_stamp;
    phw_stat->ps_rbf_rpt            = adlak_emu_update_rpt();

    ptask->context->pmodel_attr->cmq_buffer->cmq_rd_offset = phw_stat->ps_rbf_rpt;
    adlak_os_sema_give_from_isr(pinference->sem_irq);
}
#endif

/**
 * @brief inference on adlak hardware
 *
 * @return int
 */
int adlak_dev_inference_init(struct adlak_device *padlak) {
    int ret;

    struct adlak_workqueue *pwq = &padlak->queue;

    struct adlak_dev_inference *pinference = &pwq->dev_inference;
    AML_LOG_DEBUG("%s\n", __func__);
    ret = adlak_os_mutex_lock(&pwq->wq_mutex);
    if (ret) {
        goto err;
    }
    g_adlak_pwq = pwq;

    ret = adlak_os_spinlock_init(&pinference->spinlock);
    ret = adlak_os_sema_init(&pinference->sem_irq, 1, 0);

#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
    ret = adlak_os_timer_init(&pinference->emu_timer, adlak_emu_irq_cb, NULL);
    if (ret) {
        AML_LOG_ERR("emu_timer init fail!\n");
    }
#endif
    ret =
        adlak_os_thread_create(&pinference->thrd_inference, adlak_dev_inference_cb, (void *)padlak);
    if (ret) {
        AML_LOG_ERR("Create inference thread fail!\n");
    }
    AML_LOG_INFO("Create inference thread success\n");
    ret = adlak_os_mutex_unlock(&pwq->wq_mutex);
    if (ret) {
        AML_LOG_ERR("wq_mutex unlock fail!\n");
    }
    return ret;
err:
    return ERR(EINTR);
}

static void adlak_dev_inference_finalize(void *args) {
    struct adlak_device *   padlak = args;
    struct adlak_workqueue *pwq    = &padlak->queue;
    adlak_os_sema_give(pwq->wk_update);
}

int adlak_dev_inference_deinit(struct adlak_device *padlak) {
    int                         ret;
    struct adlak_workqueue *    pwq        = &padlak->queue;
    struct adlak_dev_inference *pinference = &pwq->dev_inference;
    AML_LOG_DEBUG("%s\n", __func__);
    ret = adlak_os_thread_detach(&pinference->thrd_inference, adlak_dev_inference_finalize,
                                 (void *)padlak);
    if (ret) {
        AML_LOG_ERR("Detach inference thread fail!\n");
    }
    ret = adlak_os_mutex_lock(&pwq->wq_mutex);
    if (ret) {
        goto err;
    }
    if (pinference->emu_timer) {
        adlak_os_timer_destroy(&pinference->emu_timer);
    }
    if (pinference->sem_irq) {
        ret = adlak_os_sema_destroy(&pinference->sem_irq);
    }
    if (pinference->spinlock) {
        ret = adlak_os_spinlock_destroy(&pinference->spinlock);
    }
    ret = adlak_os_mutex_unlock(&pwq->wq_mutex);
err:
    return 0;
}

static void adlak_irq_status_decode(uint32_t state) {
    if (state & ADLAK_IRQ_MASK_PARSER_STOP_CMD) {
        AML_LOG_INFO(" [0]: parser stop for command");
    }
    if (state & ADLAK_IRQ_MASK_PARSER_STOP_ERR) {
        AML_LOG_INFO(" [1]: parser stop for error");
    }
    if (state & ADLAK_IRQ_MASK_PARSER_STOP_PMT) {
        AML_LOG_INFO(" [2]: parser stop for preempt");
    }
    if (state & ADLAK_IRQ_MASK_PEND_TIMOUT) {
        AML_LOG_INFO(" [3]: pending timer timeout");
    }
    if (state & ADLAK_IRQ_MASK_LAYER_END) {
        AML_LOG_INFO(" [4]: layer end event");
    }
    if (state & ADLAK_IRQ_MASK_TIM_STAMP) {
        AML_LOG_INFO(" [5]: time_stamp irq event");
    }
    if (state & ADLAK_IRQ_MASK_APB_WAIT_TIMOUT) {
        AML_LOG_INFO(" [6]: apb wait timer timeout");
    }
    if (state & ADLAK_IRQ_MASK_PM_DRAM_OVF) {
        AML_LOG_INFO(" [7]: pm dram overflow");
    }
    if (state & ADLAK_IRQ_MASK_PM_FIFO_OVF) {
        AML_LOG_INFO(" [8]: pm fifo overflow");
    }
    if (state & ADLAK_IRQ_MASK_PM_ARBITER_OVF) {
        AML_LOG_INFO(" [9]: pm arbiter overflow");
    }
    if (state & ADLAK_IRQ_MASK_INVALID_IOVA) {
        AML_LOG_INFO(" [10]: smmu has an invalid-va");
    }
    if (state & ADLAK_IRQ_MASK_SW_TIMEOUT) {
        AML_LOG_INFO("user define : software timeout");
    }
}

static void adlak_status_report_decode(uint32_t state) {
    HAL_ADLAK_REPORT_STATUS_S d;
    d.all = state;

    if (d.bitc.hang_dw_sramf) {
        AML_LOG_INFO(" [0]: dw sramf hang");
    }
    if (d.bitc.hang_dw_sramw) {
        AML_LOG_INFO(" [1]: dw sramw hang");
    }
    if (d.bitc.hang_pe_srama) {
        AML_LOG_INFO(" [2]: pe srama hang");
    }
    if (d.bitc.hang_pe_sramm) {
        AML_LOG_INFO(" [3]: pe sramm hang");
    }
    if (d.bitc.hang_px_srama) {
        AML_LOG_INFO(" [4]: px srama hang");
    }
    if (d.bitc.hang_px_sramm) {
        AML_LOG_INFO(" [5]: px sramm hang");
    }
    if (d.bitc.rsv1) {
        AML_LOG_INFO(" [6]: reserved");
    }
    if (d.bitc.hang_vlc_decoder) {
        AML_LOG_INFO(" [7]: vlc decoder hang rpid = %d.", d.bitc.vlc_decoder_rpid);
    }
}

static void adlak_irq_bottom_half(struct adlak_device *padlak, int *device_state,
                                  struct adlak_task *ptask) {
    struct adlak_hw_info *phw_info = NULL;
    struct adlak_hw_stat *phw_stat = NULL;
    AML_LOG_INFO("%s", __func__);

    adlak_os_mutex_lock(&padlak->dev_mutex);

    phw_stat = &ptask->hw_stat;
    adlak_profile_stop(
        padlak, ptask->context, &ptask->context->pmodel_attr->pm_cfg, &ptask->context->pmodel_attr->pm_stat,
        &ptask->profilling,
        (ptask->invoke_end_idx >= ptask->context->pmodel_attr->hw_layer_last) ? 1 : 0);

    phw_info = phw_stat->hw_info;

    ptask->state = ADLAK_SUBMIT_STATE_FAIL;
    if (phw_info->irq_cfg.mask_normal & phw_stat->irq_status.irq_masked) {
        *device_state = ADLAK_DEVICE_IDLE;
        if (ptask->time_stamp == phw_stat->irq_status.time_stamp) {
            ptask->state = ADLAK_SUBMIT_STATE_FINISHED;
            AML_LOG_INFO("submit_finished,IRQ status[0x%08X].", phw_stat->irq_status.irq_masked);
        } else {
            *device_state = ADLAK_DEVICE_ERR;
            if (CONTEXT_STATE_CLOSED != ptask->context->state) {
                AML_LOG_ERR("time_stamp_expect[%d],time_stamp_get[%d]", ptask->time_stamp,
                            phw_stat->irq_status.time_stamp);
                ASSERT(0);
            }
        }

    } else if (phw_info->irq_cfg.mask_err & phw_stat->irq_status.irq_masked) {
        *device_state = ADLAK_DEVICE_ERR;
        AML_LOG_ERR("IRQ status[0x%08X].", phw_stat->irq_status.irq_masked);
        adlak_irq_status_decode(phw_stat->irq_status.irq_masked);
        adlak_status_report_decode(phw_stat->irq_status.status_report);
    } else {
        *device_state = ADLAK_DEVICE_ERR;
        if (CONTEXT_STATE_CLOSED != ptask->context->state) {
            AML_LOG_ERR("Not support status[0x%08X]!", phw_stat->irq_status.irq_masked);
            ASSERT(0);
        } else {
            AML_LOG_DEBUG("Not support status[0x%08X],and the context has been closed!",
                          phw_stat->irq_status.irq_masked);
        }
    }

    adlak_os_mutex_unlock(&padlak->dev_mutex);
}
