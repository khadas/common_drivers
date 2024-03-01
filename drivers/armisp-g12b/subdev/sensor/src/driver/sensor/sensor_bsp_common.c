#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include "acamera_command_api.h"
#include "acamera_sensor_api.h"
#include "system_am_mipi.h"
#include "sensor_bsp_common.h"

int sensor_bp_init(sensor_bringup_t* sbp, struct device* dev)
{
    sbp->dev = dev;
    sbp->np = dev->of_node;
    sbp->vana = 0;
    sbp->vdig = 0;
    sbp->power = 0;
    sbp->reset = 0;

    pr_info("sensor bsp init\n");

    return 0;
}

int pwr_am_enable(sensor_bringup_t* sensor_bp, const char* propname, int val)
{
    struct device_node *np = NULL;
    int ret = -1;

    np = sensor_bp->np;
    sensor_bp->vana = of_get_named_gpio(np, propname, 0);
    ret = sensor_bp->vana;

    if (ret >= 0) {
        devm_gpio_request(sensor_bp->dev, sensor_bp->vana, "POWER");
        if (gpio_is_valid(sensor_bp->vana)) {
            gpio_direction_output(sensor_bp->vana, val);
        } else {
            pr_err("pwr_enable: gpio %s is not valid\n", propname);
            return -1;
        }
    } else {
        pr_err("pwr_enable: get_named_gpio %s fail\n", propname);
    }

    return ret;
}

int pwr_am_disable(sensor_bringup_t *sensor_bp)
{
    if (gpio_is_valid(sensor_bp->vana)) {
        gpio_direction_output(sensor_bp->vana, 0);
        devm_gpio_free(sensor_bp->dev, sensor_bp->vana);
    } else {
        pr_err("Error invalid pwr gpio\n");
    }

    return 0;
}

int pwr_ir_cut_enable(sensor_bringup_t* sensor_bp, int propname, int val)
{
    int ret = -1;
    ret = propname;

    if (ret >= 0) {
        devm_gpio_request(sensor_bp->dev, propname, "POWER");
        if (gpio_is_valid(propname)) {
            gpio_direction_output(propname, val);
        } else {
            pr_err("pwr_enable: gpio %d is not valid\n", propname);
            return -1;
        }
    } else {
        pr_err("pwr_enable: get_named_gpio %d fail\n", propname);
    }
    return ret;
}

int reset_am_enable(sensor_bringup_t* sensor_bp, const char* propname, int val)
{
    struct device_node *np = NULL;
    int ret = -1;

    np = sensor_bp->np;
    sensor_bp->reset = of_get_named_gpio(np, propname, 0);
    ret = sensor_bp->reset;

    if (ret >= 0) {
        devm_gpio_request(sensor_bp->dev, sensor_bp->reset, "RESET");
        if (gpio_is_valid(sensor_bp->reset)) {
            gpio_direction_output(sensor_bp->reset, val);
        } else {
            pr_err("reset_enable: gpio %s is not valid\n", propname);
            return -1;
        }
    } else {
        pr_err("reset_enable: get_named_gpio %s fail\n", propname);
    }

    return ret;
}

int reset_am_disable(sensor_bringup_t* sensor_bp)
{
    if (gpio_is_valid(sensor_bp->reset)) {
        gpio_direction_output(sensor_bp->reset, 0);
        devm_gpio_free(sensor_bp->dev, sensor_bp->reset);
    } else {
        pr_err("Error invalid reset gpio\n");
    }

    return 0;
}

int pwren_am_enable(sensor_bringup_t* sensor_bp, const char* propname, int val)
{
    struct device_node *np = NULL;
    int ret = -1;

    np = sensor_bp->np;
    sensor_bp->pwren = of_get_named_gpio(np, propname, 0);
    ret = sensor_bp->pwren;

    if (ret >= 0) {
        devm_gpio_request(sensor_bp->dev, sensor_bp->pwren, "PWREN");
        if (gpio_is_valid(sensor_bp->pwren)) {
            gpio_direction_output(sensor_bp->pwren, val);
        } else {
            pr_err("power_enable: gpio %s is not valid\n", propname);
            return -1;
        }
    } else {
        pr_err("power_enable: get_named_gpio %s fail\n", propname);
    }

    return ret;
}

int pwren_am_disable(sensor_bringup_t* sensor_bp)
{
    if (gpio_is_valid(sensor_bp->pwren)) {
        gpio_direction_output(sensor_bp->pwren, 0);
        devm_gpio_free(sensor_bp->dev, sensor_bp->pwren);
    } else {
        pr_err("Error invalid pwren gpio\n");
    }

    return 0;
}
int clk_am_enable(sensor_bringup_t* sensor_bp, const char* propname)
{
    struct clk *clk;
    int clk_val;
    clk = devm_clk_get(sensor_bp->dev, propname);
    if (IS_ERR(clk)) {
        pr_err("DEBUG: cannot get %s clk\n", propname);
        clk = NULL;
        return -1;
    }

	clk_set_rate(clk, 24000000);
    clk_prepare_enable(clk);
    clk_val = clk_get_rate(clk);
    pr_err("DEBUG: init mclock is %d MHZ\n",clk_val/1000000);

    sensor_bp->mclk = clk;
    return 0;
}

int gp_pl_am_enable(sensor_bringup_t* sensor_bp, const char* propname, uint32_t rate)
{
    int ret;
    struct clk *clk;
    int clk_val;
    clk = devm_clk_get(sensor_bp->dev, propname);
    if (IS_ERR(clk)) {
        pr_err("cannot get %s clk\n", propname);
        clk = NULL;
        return -1;
    }
    pr_err("get %s clk\n", propname);
    ret = clk_set_rate(clk, rate);
    pr_err("set clk rate \n");
    if (ret < 0)
        pr_err("clk_set_rate failed\n");
    udelay(30);
    ret = clk_prepare_enable(clk);
    pr_err("clk prepare enable \n");
    if (ret < 0)
        pr_err(" clk_prepare_enable failed\n");
    clk_val = clk_get_rate(clk);
    pr_err("init mclock is %d MHZ\n",clk_val/1000000);

    sensor_bp->mclk = clk;
    return 0;
}

int clk_am_disable(sensor_bringup_t *sensor_bp)
{
    struct clk *mclk = NULL;

    if (sensor_bp == NULL || sensor_bp->mclk == NULL) {
        pr_err("Error input param\n");
        return -EINVAL;
    }

    mclk = sensor_bp->mclk;

    clk_disable_unprepare(mclk);

    devm_clk_put(sensor_bp->dev, mclk);

    pr_info("Success disable mclk\n");

    return 0;
}

void sensor_set_iface(sensor_mode_t *mode, exp_offset_t offset, sensor_context_t *p_ctx)
{
    am_mipi_info_t mipi_info;
    struct am_adap_info info;

    if (mode == NULL) {
        pr_info( "Error input param\n");
        return;
    }

    memset(&mipi_info, 0, sizeof(mipi_info));
    memset(&info, 0, sizeof(struct am_adap_info));
    mipi_info.fte1_flag = 0; //get_fte1_flag();
    mipi_info.lanes = mode->lanes;
    mipi_info.ui_val = 1000 / mode->bps;

    if ((1000 % mode->bps) != 0)
        mipi_info.ui_val += 1;

    am_mipi_init(&mipi_info);

    switch (mode->bits) {
    case 8:
        info.fmt = AM_RAW8;
        break;
    case 10:
        info.fmt = AM_RAW10;
        break;
    case 12:
        info.fmt = AM_RAW12;
        break;
    default :
        info.fmt = AM_RAW10;
        break;
    }

    info.img.width = mode->resolution.width;
    info.img.height = mode->resolution.height;
    info.path = p_ctx->cam_isp_path;
    info.offset.offset_x = offset.offset_x;
    info.offset.offset_y = offset.offset_y;
    //pr_err("%s x offset %d, y offset %d \n", __func__, offset.offset_x, offset.offset_y);
    pr_err("%s sensor lanes %d \n", __func__, mipi_info.lanes);
    pr_err("%s dcam %d wdr mode %d cam_isp_path %d \n", __func__, p_ctx->dcam_mode, mode->wdr_mode, p_ctx->cam_isp_path);
    if (mode->wdr_mode == WDR_MODE_FS_LIN) {
        info.mode = DOL_MODE;
        info.type = mode->dol_type;
        if (info.type == DOL_LINEINFO) {
           info.offset.long_offset = offset.long_offset;
           info.offset.short_offset = offset.short_offset;
        }
    } else {
        if (p_ctx->dcam_mode)
            info.mode = DCAM_MODE;
        else
            info.mode = DIR_MODE;
    }

    uint32_t isp_clk_rate = 0;
    camera_notify(NOTIFY_GET_ISP_CLKRATE, &isp_clk_rate);
    isp_clk_rate = (isp_clk_rate / 10) * 9;
    info.align_width = isp_clk_rate / ((mode->resolution.height + 60) * (mode->fps / 256) * 2) - mode->resolution.width;

    am_adap_set_info(&info);
    am_adap_init(p_ctx->cam_isp_path);
    am_adap_start(p_ctx->cam_isp_path, p_ctx->dcam_mode);
}

void sensor_set_iface1(sensor_mode_t *mode, exp_offset_t offset, sensor_context_t *p_ctx)
{
    am_mipi_info_t mipi_info;
    struct am_adap_info info;
    pr_err("%s start \n", __func__);
    if (mode == NULL) {
        pr_info( "Error input param\n");
        return;
    }

    pr_err("%s start++ \n", __func__);
    memset(&mipi_info, 0, sizeof(mipi_info));
    memset(&info, 0, sizeof(struct am_adap_info));
    mipi_info.fte1_flag = 0; //get_fte1_flag();
    //pr_err("%s fte1_flag %d \n", __func__, mipi_info.fte1_flag);
    mipi_info.lanes = mode->lanes;
    mipi_info.ui_val = 1000 / mode->bps;
    pr_err("%s ++ \n", __func__);
    if ((1000 % mode->bps) != 0)
        mipi_info.ui_val += 1;
    pr_err("%s +++ \n", __func__);
    am_mipi1_init(&mipi_info);
    pr_err("%s ++++ \n", __func__);

    switch (mode->bits) {
    case 8:
        info.fmt = AM_RAW8;
        break;
    case 10:
        info.fmt = AM_RAW10;
        break;
    case 12:
        info.fmt = AM_RAW12;
        break;
    default :
        info.fmt = AM_RAW10;
        break;
    }

    info.img.width = mode->resolution.width;
    info.img.height = mode->resolution.height;
    info.path = p_ctx->cam_isp_path;
    info.offset.offset_x = offset.offset_x;
    info.offset.offset_y = offset.offset_y;
    pr_err("%s x offset %d, y offset %d \n", __func__, offset.offset_x, offset.offset_y);
    pr_err("%s dcam %d wdr mode %d cam_isp_path %d \n", __func__, p_ctx->dcam_mode, mode->wdr_mode, p_ctx->cam_isp_path);
    if (mode->wdr_mode == WDR_MODE_FS_LIN) {
        info.mode = DOL_MODE;
        info.type = mode->dol_type;
        if (info.type == DOL_LINEINFO) {
           info.offset.long_offset = offset.long_offset;
           info.offset.short_offset = offset.short_offset;
        }
    } else {
        if (p_ctx->dcam_mode)
            info.mode = DCAM_MODE;
        else
            info.mode = DIR_MODE;
    }

    uint32_t isp_clk_rate = 0;
    camera_notify(NOTIFY_GET_ISP_CLKRATE, &isp_clk_rate);
    isp_clk_rate = (isp_clk_rate / 10) * 9;
    info.align_width = isp_clk_rate / ((mode->resolution.height + 60) * (mode->fps / 256) * 2) - mode->resolution.width;

    am_adap_set_info(&info);
    am_adap_init(p_ctx->cam_isp_path);
    am_adap_start(p_ctx->cam_isp_path, p_ctx->dcam_mode);
}


void sensor_iface_disable(sensor_context_t *p_ctx)
{
    am_adap_deinit(p_ctx->cam_isp_path);
    am_mipi_deinit();
}

void sensor_iface1_disable(sensor_context_t *p_ctx)
{
    am_adap_deinit(p_ctx->cam_isp_path);
    am_mipi1_deinit();
}

