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

/************************** Variable Definitions *****************************/

struct adlak_task *            g_adlak_ptask_sch_cur = NULL;
static struct adlak_workqueue *g_adlak_pwq           = NULL;

/************************** Function Prototypes ******************************/

static void adlak_irq_bottom_half(struct adlak_device *padlak, uint32_t *device_state,
                                  struct adlak_task *ptask);

int adlak_submit_wait(struct adlak_dev_inference *pinference, struct adlak_task *ptask) {
    int ret;
    int repeat = 0;
    AML_LOG_INFO("%s", __func__);
#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
    /* Suppose ptask->sw_timeout_ms_ms is greater than 5 */
    adlak_os_timer_add(&pinference->emu_timer, 5);
#endif

    AML_LOG_INFO("set submit hw_timeout_ms=%u ms", ptask->hw_timeout_ms);
    do {
        ret = adlak_os_sema_take_timeout(pinference->sem_irq, ptask->hw_timeout_ms);
        if (ERR(NONE) == ret) {
            AML_LOG_INFO("%s\n", "sema_take sucsess");
            ptask->hw_stat.irq_status.timeout = false;
            break;
        } else if (ERR(EINTR) == ret) {
            AML_LOG_WARN("%s\n", "sema_take_timeout");
            ptask->hw_stat.irq_status.timeout = true;
#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
            adlak_os_timer_del(&pinference->emu_timer);
            break;
#endif
        }
        repeat++;
    } while (repeat < 2);
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

static void adlak_dpm_timer_cb(void *t) {
    struct adlak_workqueue *pwq = g_adlak_pwq;
    int                     all_task_num;

    AML_LOG_DEBUG("%s\n", __func__);
    all_task_num = pwq->sched_num + pwq->pending_num;

    if (all_task_num < 1) {
        pwq->dev_inference.dmp_timeout += 1;
        adlak_os_sema_give(pwq->wk_update);
    }
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
                if (net_id == ptask->net_id) {
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

void *adlak_dev_inference_cb(void *args) {
    int                     ret;
    struct adlak_device *   padlak = args;
    struct adlak_workqueue *pwq    = &padlak->queue;
    adlak_os_thread_t *     pthrd  = &pwq->dev_inference.thrd_inference;
    int                     all_task_num;
    uint32_t                device_state  = ADLAK_DEVICE_INIT;
    struct adlak_task *     ptask_sch_cur = NULL, *ptask_sch_pre = NULL;
    uint32_t                wq_idel_cnt    = 0;
    uint32_t                dpm_period_set = padlak->dpm_period_set;
    int32_t                 net_id_invoke_previous = -1;

    ret = adlak_os_timer_init(&pwq->dev_inference.dpm_timer, (void (*)(void *))adlak_dpm_timer_cb,
                              NULL);
    if (ret) {
        AML_LOG_ERR("dpm_timer init fail!\n");
        return NULL;
    }
    pwq->dev_inference.dmp_timeout = 0;

    if (ADLAK_DEVICE_INIT == device_state) {
        device_state = ADLAK_DEVICE_IDLE;
    }
    while (!pthrd->thrd_should_stop) {
        // adlak_might_sleep();
        AML_LOG_DEBUG("%s\n", __func__);

        if (ADLAK_DEVICE_IDLE == device_state) {
            adlak_os_mutex_lock(&pwq->wq_mutex);
            pwq->valid_num = adlak_get_valid_num(pwq, &net_id_invoke_previous);
            all_task_num   = pwq->sched_num + pwq->valid_num;
            adlak_os_mutex_unlock(&pwq->wq_mutex);

            if (all_task_num < 1) {
                AML_LOG_DEBUG("nothing need to do!\n");
                adlak_os_timer_add(&pwq->dev_inference.dpm_timer, dpm_period_set);
                adlak_os_thread_yield();
                if (ERR(NONE) == adlak_os_sema_take(pwq->wk_update)) {
                    if (pwq->dev_inference.dmp_timeout) {
                        pwq->dev_inference.dmp_timeout = 0;
                        wq_idel_cnt++;
                        if (wq_idel_cnt > 10) {
                            // device dpm schedule
                            wq_idel_cnt = 0;
                            adlak_dpm_stage_adjust(padlak, ADLAK_DPM_STRATEGY_MIN);
                        }
                    } else {
                        // pwq is not empty
                        wq_idel_cnt = 0;
                        adlak_os_timer_del(&pwq->dev_inference.dpm_timer);
                        adlak_dpm_stage_adjust(padlak, ADLAK_DPM_STRATEGY_MAX);  // wake up
                    }
                }
            }
        }

        if (padlak->need_reset_queue) {
            padlak->need_reset_queue = false;
#if CONFIG_ADLAK_DPM_EN
            AML_LOG_INFO("Need to reset work queue.\n");
            adlak_os_mutex_lock(&pwq->wq_mutex);
            adlak_queue_reset(padlak);
            // reset wq
            ptask_sch_pre         = NULL;
            ptask_sch_cur         = NULL;
            pwq->ptask_sch_cur    = ptask_sch_cur;
            g_adlak_ptask_sch_cur = ptask_sch_cur;
            device_state          = ADLAK_DEVICE_IDLE;
            adlak_os_mutex_unlock(&pwq->wq_mutex);
#endif
        }

        if (ADLAK_DEVICE_BUSY == device_state) {
            if (ERR(NONE) != adlak_submit_wait(&pwq->dev_inference, ptask_sch_cur)) {
                AML_LOG_ERR("%s\n", "submit timeout");
                ptask_sch_cur->hw_stat.irq_status.timeout = true;
            } else {
                AML_LOG_INFO("%s\n", "submit success");
            }
            adlak_irq_bottom_half(padlak, &device_state, ptask_sch_cur);
            adlak_debug_print_device_state(device_state);

            adlak_debug_invoke_list_dump(padlak, 1);
        }

        /*if hw_status is ready,schedule_next task*/
        if (ADLAK_DEVICE_IDLE == device_state) {
            adlak_os_mutex_lock(&pwq->wq_mutex);
            pwq->valid_num = adlak_get_valid_num(pwq, &net_id_invoke_previous);
            adlak_os_mutex_unlock(&pwq->wq_mutex);
            if (pwq->valid_num > 0) {
                if (NULL != ptask_sch_cur) {
                    if (NULL != ptask_sch_pre) {
                        ASSERT(0);
                    }
                    ptask_sch_pre = ptask_sch_cur;
                    ptask_sch_cur = NULL;
                }
                // submit
                adlak_queue_schedule_update(padlak, &ptask_sch_cur, net_id_invoke_previous);
                if (NULL == ptask_sch_cur) {
                    // TODO(shiwei.sun) just for debug,Program should not execute here
                    AML_LOG_WARN("Do not need sch!");
                } else {
                    pwq->ptask_sch_cur    = ptask_sch_cur;
                    g_adlak_ptask_sch_cur = ptask_sch_cur;
                    device_state          = ADLAK_DEVICE_BUSY;
                    (void)adlak_submit_patch_and_exec(ptask_sch_cur);
                    net_id_invoke_previous = -1;
                    if (padlak->share_swap_en) {
                        if (ptask_sch_cur->invoke_end_idx < ptask_sch_cur->hw_layer_last) {
                            net_id_invoke_previous = ptask_sch_cur->net_id;
                        }
                    }
                }
                adlak_debug_print_device_state(device_state);
            }
        }

        if (NULL != ptask_sch_pre) {
            adlak_os_mutex_lock(&pwq->wq_mutex);
            AML_LOG_INFO("adlak_queue_update_task_state pre %d !\n", __LINE__);
            ret = adlak_queue_update_task_state(padlak, (struct adlak_task *)ptask_sch_pre);
            adlak_os_mutex_unlock(&pwq->wq_mutex);
            if (1 == ret) {
                AML_LOG_DEBUG("%s wakeup wq", __func__);
                adlak_to_umd_sinal_give(ptask_sch_cur->context->wait); /*give signal to umd*/
            }
            if (ret >= 0) {
                ptask_sch_pre = NULL;
            }
        }
        if (ADLAK_DEVICE_ERR == device_state || ADLAK_DEVICE_IDLE == device_state) {
            if (NULL != ptask_sch_cur) {
                adlak_os_mutex_lock(&pwq->wq_mutex);
                AML_LOG_INFO("adlak_queue_update_task_state cur %d !\n", __LINE__);
                ret = adlak_queue_update_task_state(padlak, (struct adlak_task *)ptask_sch_cur);
                adlak_os_mutex_unlock(&pwq->wq_mutex);
                if (1 == ret) {
                    AML_LOG_DEBUG("%s wakeup wq", __func__);
                    adlak_to_umd_sinal_give(ptask_sch_cur->context->wait); /*give signal to umd*/
                }
                if (ret >= 0) {
                    ptask_sch_cur         = NULL;
                    pwq->ptask_sch_cur    = ptask_sch_cur;
                    g_adlak_ptask_sch_cur = ptask_sch_cur;

                    AML_LOG_INFO("adlak_queue_update_task_state finished cur %d !\n", __LINE__);
                }
            }
            adlak_debug_invoke_list_dump(padlak, 1);
        }
        if (ADLAK_DEVICE_ERR == device_state) {
            AML_LOG_INFO("adlak_hal_reset_and_start !\n");
            // reset_device
            adlak_hal_reset_and_start((void *)padlak);

            adlak_os_mutex_lock(&pwq->wq_mutex);
            adlak_queue_reset(padlak);
            // reset wq
            ptask_sch_pre         = NULL;
            ptask_sch_cur         = NULL;
            pwq->ptask_sch_cur    = ptask_sch_cur;
            g_adlak_ptask_sch_cur = ptask_sch_cur;
            device_state          = ADLAK_DEVICE_IDLE;
            adlak_os_mutex_unlock(&pwq->wq_mutex);
        }
    }
    pthrd->thrd_should_stop = 0;
    if (pwq->dev_inference.dpm_timer) {
        adlak_os_timer_destroy(&pwq->dev_inference.dpm_timer);
    }
    return NULL;
}

#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)

static void adlak_emu_irq_cb(void *t) {
    struct adlak_hw_stat *      phw_stat   = NULL;
    struct adlak_dev_inference *pinference = NULL;
    struct adlak_task *         ptask      = NULL;

    AML_LOG_DEBUG("%s\n", __func__);
    adlak_cant_sleep();
    pinference                      = &g_adlak_pwq->dev_inference;
    ptask                           = g_adlak_ptask_sch_cur;
    phw_stat                        = &ptask->hw_stat;
    phw_stat->irq_status.irq_masked = ADLAK_IRQ_MASK_TIM_STAMP;
    phw_stat->irq_status.irq_masked |= ADLAK_IRQ_MASK_LAYER_END;
    phw_stat->irq_status.time_stamp = ptask->time_stamp;
    phw_stat->ps_rbf_rpt            = adlak_emu_update_rpt();

    ((struct adlak_device *)ptask->padlak)->cmq_buf_info.cmq_rd_offset = phw_stat->ps_rbf_rpt;
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
    ret = adlak_os_timer_init(&pinference->emu_timer, (void (*)(void *))adlak_emu_irq_cb, NULL);
    if (ret) {
        AML_LOG_ERR("emu_timer init fail!\n");
    }
#endif
    ret = adlak_os_thread_create(&pinference->thrd_inference,
                                 (void *(*)(void *))adlak_dev_inference_cb, (void *)padlak);
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

int adlak_dev_inference_deinit(struct adlak_device *padlak) {
    int                         ret;
    struct adlak_workqueue *    pwq        = &padlak->queue;
    struct adlak_dev_inference *pinference = &pwq->dev_inference;
    AML_LOG_DEBUG("%s\n", __func__);

    ret = adlak_os_thread_detach(&pinference->thrd_inference);
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

static void adlak_irq_bottom_half(struct adlak_device *padlak, uint32_t *device_state,
                                  struct adlak_task *ptask) {
    struct adlak_hw_info *phw_info = NULL;
    struct adlak_hw_stat *phw_stat = NULL;
    AML_LOG_INFO("%s", __func__);

    adlak_os_mutex_lock(&padlak->dev_mutex);

    phw_stat = &ptask->hw_stat;
    adlak_profile_stop(padlak, &ptask->profilling);

    phw_info = phw_stat->hw_info;

    ptask->state = ADLAK_SUBMIT_STATE_FAIL;
    if (phw_info->irq_cfg.mask_normal & phw_stat->irq_status.irq_masked) {
        *device_state = ADLAK_DEVICE_IDLE;
        if ((ptask->time_stamp <= phw_stat->irq_status.time_stamp) ||
            ((int)ptask->time_stamp <= (int)phw_stat->irq_status.time_stamp)) {
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
