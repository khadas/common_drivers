MODULE_NAME = amvdec_ports
obj-m += ${MODULE_NAME}.o
${MODULE_NAME}-objs += aml_vcodec_dec_drv.o
${MODULE_NAME}-objs += aml_vcodec_dec.o
${MODULE_NAME}-objs += aml_vcodec_util.o
${MODULE_NAME}-objs += aml_vcodec_adapt.o
${MODULE_NAME}-objs += aml_vcodec_vpp.o
${MODULE_NAME}-objs += aml_vcodec_ge2d.o
${MODULE_NAME}-objs += aml_vcodec_ts.o
${MODULE_NAME}-objs += vdec_drv_if.o
${MODULE_NAME}-objs += aml_task_chain.o
${MODULE_NAME}-objs += aml_buf_mgr.o
${MODULE_NAME}-objs += aml_buf_core.o
${MODULE_NAME}-objs += decoder/vdec_h264_if.o
${MODULE_NAME}-objs += decoder/vdec_hevc_if.o
${MODULE_NAME}-objs += decoder/vdec_vp9_if.o
${MODULE_NAME}-objs += decoder/vdec_mpeg12_if.o
${MODULE_NAME}-objs += decoder/vdec_mpeg4_if.o
${MODULE_NAME}-objs += decoder/vdec_mjpeg_if.o
${MODULE_NAME}-objs += decoder/vdec_avs_if.o
${MODULE_NAME}-objs += decoder/vdec_av1_if.o
${MODULE_NAME}-objs += decoder/vdec_avs2_if.o
${MODULE_NAME}-objs += decoder/vdec_avs3_if.o
${MODULE_NAME}-objs += decoder/vdec_vc1_if.o
ifdef CONFIG_AMLOGIC_MEDIA_V4L_SOFTWARE_PARSER
${MODULE_NAME}-objs += decoder/aml_h264_parser.o
${MODULE_NAME}-objs += decoder/aml_hevc_parser.o
${MODULE_NAME}-objs += decoder/aml_vp9_parser.o
${MODULE_NAME}-objs += decoder/aml_mpeg12_parser.o
${MODULE_NAME}-objs += decoder/aml_mpeg4_parser.o
${MODULE_NAME}-objs += decoder/aml_mjpeg_parser.o
${MODULE_NAME}-objs += utils/golomb.o
endif
${MODULE_NAME}-objs += utils/common.o
${MODULE_NAME}-objs += utils/aml_dec_trace.o

PR_FMT = "amlv4l"
PR_FMT_DEFINE="-Dpr_fmt(fmt)=\"[$(PR_FMT)]:\" fmt"
ccflags-y += $(PR_FMT_DEFINE)
