#ifndef AVS3_GLOBAL_H_
#define AVS3_GLOBAL_H_

//#define DEBUG_AMRISC

#define LINUX
#define NEW_FB_CODE
#define NEW_FRONT_BACK_CODE
#define LARGE_INSTRUCTION_SPACE_SUPORT
#define BUFMGR_SANITY_CHECK

#define P010_ENABLE

#define OW_TRIPLE_WRITE

#include "com_def.h"

#ifdef SIMULATION
#ifdef SYSTEM_TEST
#define printf st_printf
#define printk st_printf
#else
#define printf io_printf
#define printk io_printf
#endif
#endif

#define WITH_OLD_CODE

#define RPM_BEGIN                                              0x080  //0x100
#define ALF_BEGIN                                              0x100  //0x180
#define RPM_END                                                0x200  //0x280
#define RPM_VALID_END                                          0x1b8


typedef union param_u {
	struct {
		unsigned short data[RPM_END - RPM_BEGIN];
	} l;
	struct {

		/*sequence head*/
		unsigned short sqh_profile_id;
		unsigned short sqh_level_id;
		unsigned short sqh_progressive_sequence;
		unsigned short sqh_field_coded_sequence;
		unsigned short sqh_library_stream_flag;
		unsigned short sqh_library_picture_enable_flag;
		unsigned short sqh_horizontal_size;
		unsigned short sqh_vertical_size;
		unsigned short sqh_sample_precision;
		unsigned short sqh_encoding_precision;
		unsigned short sqh_aspect_ratio;
		unsigned short sqh_frame_rate_code;
		unsigned short sqh_low_delay;
		unsigned short sqh_temporal_id_enable_flag;
		unsigned short sqh_max_dpb_size;
		unsigned short sqh_num_ref_default_active_minus1[2];
		unsigned short sqh_log2_max_cu_width_height;
		unsigned short sqh_adaptive_leveling_filter_enable_flag;
		unsigned short sqh_num_of_hmvp_cand;
		unsigned short sqh_output_reorder_delay;
		unsigned short sqh_cross_patch_loop_filter;
		/*picture head*/
		unsigned short pic_header_decode_order_index;
		unsigned short pic_header_picture_output_delay;
		unsigned short pic_header_progressive_frame;
		unsigned short pic_header_top_field_first;
		unsigned short pic_header_repeat_first_field;
		unsigned short pic_header_ref_pic_list_sps_flag[2];
		unsigned short pic_header_rpl_l0_idx;
		unsigned short pic_header_rpl_l1_idx;
		unsigned short pic_header_rpl_l0_ref_pic_num;
		unsigned short pic_header_rpl_l0_ref_pics_ddoi[17];
		unsigned short pic_header_rpl_l1_ref_pic_num;
		unsigned short pic_header_rpl_l1_ref_pics_ddoi[17];
		unsigned short pic_header_rpl_l0_reference_to_library_enable_flag;
		unsigned short pic_header_rpl_l0_library_index_flag[17];
		unsigned short pic_header_rpl_l1_reference_to_library_enable_flag;
		unsigned short pic_header_rpl_l1_library_index_flag[17];
		unsigned short pic_header_loop_filter_disable_flag;
		unsigned short pic_header_random_access_decodable_flag;
		unsigned short pic_header_slice_type;
		unsigned short pic_header_num_ref_idx_active_override_flag;
		unsigned short pic_header_rpl_l0_ref_pic_active_num;
		unsigned short pic_header_rpl_l1_ref_pic_active_num;
		/*patch head*/
		/**/
		unsigned short sqh_adaptive_filter_shape_enable_flag;
		unsigned short pic_header_library_picture_index;
		unsigned short pic_header_top_field_picture_flag;
		unsigned short pic_header_alpha_c_offset;
		unsigned short pic_header_beta_offset;
		unsigned short pic_header_chroma_quant_param_delta_cb;
		unsigned short pic_header_chroma_quant_param_delta_cr;
//HDR10
		uint16_t video_signal_type;
		uint16_t color_description;
		uint16_t display_primaries_x[3];
		uint16_t display_primaries_y[3];
		uint16_t white_point_x;
		uint16_t white_point_y;
		uint16_t max_display_mastering_luminance;
		uint16_t min_display_mastering_luminance;
		uint16_t max_content_light_level;
		uint16_t max_picture_average_light_level;
	} p;
	struct {
		uint16_t padding[ALF_BEGIN - RPM_BEGIN];
		uint16_t picture_alf_enable_Y;
		uint16_t picture_alf_enable_Cb;
		uint16_t picture_alf_enable_Cr;
		uint16_t alf_filters_num_m_1;
		uint16_t dir_index;
		uint16_t region_distance[16];
		uint16_t alf_cb_coeffmulti[9];
		uint16_t alf_cr_coeffmulti[9];
		uint16_t alf_y_coeffmulti[16][9];
	} alf;
}param_t;

/******************************************************************************
 * CONTEXT used for decoding process.
 *
 * All have to be stored are in this structure.
 *****************************************************************************/
typedef struct _DEC_CTX DEC_CTX;
struct _DEC_CTX
{
	COM_INFO              info;
	/* magic code */
	u32                   magic;
	/* DEC identifier */
	DEC                   id;
	/* CORE information used for fast operation */
	//DEC_CORE             *core;
	/* current decoding bitstream */
	//COM_BSR               bs;

	/* decoded picture buffer management */
	COM_PM                dpm;
	/* create descriptor */
	DEC_CDSC              cdsc;

	/* current decoded (decoding) picture buffer */
	COM_PIC              *pic;
#if EVS_UBVS_MODE
	pel                   dpb_evs[N_C][MAX_SRB_PRED_SIZE];
#endif
	/* SBAC */
	//DEC_SBAC              sbac_dec;
	u8                    init_flag;

	COM_MAP               map;
#if CHROMA_NOT_SPLIT
	u8                    tree_status;
#endif
#if MODE_CONS
	u8                    cons_pred_mode;
#endif

	/* total count of remained LCU for decoding one picture. if a picture is
	decoded properly, this value should reach to zero */
	int                   lcu_cnt;

	int                 **edge_filter[LOOPFILTER_DIR_TYPE];

	COM_PIC              *pic_sao;
	SAO_STAT_DATA        ***sao_stat_data; //[SMB][comp][types]
	SAO_BLK_PARAM         **sao_blk_params; //[SMB][comp]
	SAO_BLK_PARAM         **rec_sao_blk_params;//[SMB][comp]

#if ESAO
	COM_PIC              *pic_esao;  //rec buff after deblock filter
	ESAO_BLK_PARAM          pic_esao_params[N_C];//comp]
	ESAO_FUNC_POINTER       func_esao_block_filter;
#endif

#if CCSAO
#if CCSAO_ENHANCEMENT
	COM_PIC              *pic_ccsao[2];
#else
	COM_PIC              *pic_ccsao;
#endif
#if !CCSAO_PH_SYNTAX
	CCSAO_BLK_PARAM       pic_ccsao_params[N_C-1];
#endif
	CCSAO_FUNC_POINTER    ccsao_func_ptr;
#endif

	COM_PIC              *pic_alf_Dec;
	COM_PIC              *pic_alf_Rec;
	int                   pic_alf_on[N_C];
	int                ***coeff_all_to_write_alf;
	//DEC_ALF_VAR            *dec_alf;

	u8                    ctx_flags[NUM_CNID];

	/**************************************************************************/
	/* current slice number, which is increased whenever decoding a slice.
	when receiving a slice for new picture, this value is set to zero.
	this value can be used for distinguishing b/w slices */
	u16                   slice_num;
	/* last coded intra picture's presentation temporal reference */
	int                   last_intra_ptr;
	/* current picture's decoding temporal reference */
	int                   dtr;
	/* previous picture's decoding temporal reference low part */
	int                   dtr_prev_low;
	/* previous picture's decoding temporal reference high part */
	int                   dtr_prev_high;
	/* current picture's presentation temporal reference */
	int                   ptr;
	/* the number of currently decoded pictures */
	int                   pic_cnt;
	/* picture buffer allocator */
	PICBUF_ALLOCATOR      pa;
	/* bitstream has an error? */
	u8                    bs_err;
	/* reference picture (0: forward, 1: backward) */
	COM_REFP              refp[MAX_NUM_REF_PICS][REFP_NUM];
	/* flag for picture signature enabling */
	u8                    use_pic_sign;
	/* picture signature (MD5 digest 128bits) */
	u8                    pic_sign[16];
	/* flag to indicate picture signature existing or not */
	u8                    pic_sign_exist;
	//AmlDbg*               amldbg;

#if PATCH
	int                   patch_column_width[64];
	int                   patch_row_height[128];
	PATCH_INFO           *patch;
#endif

	u8                   *wq[2];
#if CUDQP
	COM_CU_QP_GROUP       cu_qp_group;
#endif
};

#ifdef WITH_OLD_CODE
#define I_PICTURE_START_CODE    0xB3
#define PB_PICTURE_START_CODE   0xB6
#define SLICE_START_CODE_MIN    0x00
#define SLICE_START_CODE_MAX    0x8F
#define USER_DATA_START_CODE    0xB2
#define SEQUENCE_HEADER_CODE    0xB0
#define EXTENSION_START_CODE    0xB5
#define SEQUENCE_END_CODE       0xB1
#define VIDEO_EDIT_CODE         0xB7

enum ALFComponentID {
	ALF_Y = 0,
	ALF_Cb,
	ALF_Cr,
	NUM_ALF_COMPONENT
};

#if 0
typedef struct {
	int32_t alf_flag;
	int32_t num_coeff;
	int32_t filters_per_group;
	int32_t componentID;
	int32_t filterPattern[16]; // *filterPattern;
	int32_t coeffmulti[16][9]; // **coeffmulti;
} ALFParam;
#endif

typedef ALF_PARAM ALFParam;

#define INTRA_IMG                    0   //!< I frame
#define INTER_IMG                    1   //!< P frame
#define B_IMG                        2   //!< B frame
#define I_IMG                        0   //!< I frame
#define P_IMG                        1   //!< P frame
#define F_IMG                        4  //!< F frame

#define BACKGROUND_IMG               3

typedef struct {
	//int32_t typeb;
	//int32_t type;
	//int32_t tr;                                     //<! temporal reference, 8 bit,
	int32_t width;                   //!< Number of pels
	int32_t height;                  //!< Number of lines
	int32_t num_of_references;
	int32_t            pic_alf_on[NUM_ALF_COMPONENT];
	//int32_t is_field_sequence;
	//int32_t is_top_field;
	int number;
} ImageParameters;

struct inp_par {
	u32 sample_bit_depth;
	u32 alf_enable;
};
#else
#define PIC_POOL_SIZE 32
#endif

//new dual
struct buff_s {
	u32 buf_start;
	u32 buf_size;
	u32 buf_end;
};
typedef struct buff_s buff_t;

#ifdef NEW_FRONT_BACK_CODE
#define MAX_FB_IFBUF_NUM             16
typedef struct {
	uint32_t mmu0_ptr;
	uint32_t mmu1_ptr;
	uint32_t scalelut_ptr;
	uint32_t vcpu_imem_ptr;
	uint32_t sys_imem_ptr;
	uint32_t lmem0_ptr;
	uint32_t lmem1_ptr;
	uint32_t parser_sao0_ptr;
	uint32_t parser_sao1_ptr;
	uint32_t mpred_imp0_ptr;
	uint32_t mpred_imp1_ptr;
	//
	uint32_t scalelut_ptr_pre;
	//for linux
	void *sys_imem_ptr_v;
} buff_ptr_t;
#endif

typedef struct avs3_decoder {
#ifdef WITH_OLD_CODE
	uint8_t init_hw_flag;
	struct inp_par   input;
	ImageParameters  img;
	//Video_Com_data  hc;
	//Video_Dec_data  hd;
	//union param_u param;
	//avs3_frame_t *fref[REF_MAXBUFFER];
#ifdef AML
	/*used for background
	when background_picture_output_flag is 0*/
	//avs3_frame_t *m_bg;
	/*current background picture, ether m_bg or fref[..]*/
	avs3_frame_t *f_bg;
#endif
	//outdata outprint;
	uint32_t cm_header_start;
	ALF_PARAM m_alfPictureParam[N_C];
	ALF_PARAM *p_alfPictureParam[N_C];
/*#ifdef FIX_CHROMA_FIELD_MV_BK_DIST*/
	int8_t bk_img_is_top_field;
/*#endif*/
#ifdef AML
	int32_t lcu_size;
	int32_t lcu_size_log2;
	int32_t lcu_x_num;
	int32_t lcu_y_num;
	int32_t lcu_total;
#endif
/*WITH_OLD_CODE*/
#endif
	union param_u param;
	DEC_CTX ctx;
	DEC_STAT stat;
	COM_PIC  pic_pool[MAX_PB_SIZE];
	LibVCData libvc_data;
	unsigned int dec_status;
	avs3_frame_t *cur_pic;
	avs3_frame_t *col_pic;
	u8 slice_type;
	u8 seq_change_flag;
	int decode_id;
/**/
#ifdef NEW_FRONT_BACK_CODE
	uint8_t wait_working_buf;
	uint8_t front_pause_flag; /*multi pictures in one packe*/
	/*FB mgr*/
	uint8_t fb_wr_pos;
	uint8_t fb_rd_pos;
	buff_t fb_buf_mmu0;
	buff_t fb_buf_mmu1;
	buff_t fb_buf_scalelut;
	buff_t fb_buf_vcpu_imem;
	buff_t fb_buf_sys_imem;
	buff_t fb_buf_lmem0;
	buff_t fb_buf_lmem1;
	buff_t fb_buf_parser_sao0;
	buff_t fb_buf_parser_sao1;
	buff_t fb_buf_mpred_imp0;
	buff_t fb_buf_mpred_imp1;
	uint32_t frontend_decoded_count;
	uint32_t backend_decoded_count;
	buff_ptr_t fr;
	buff_ptr_t bk;
	buff_ptr_t init_fr;
	buff_ptr_t p_fr;
	buff_ptr_t next_bk[MAX_FB_IFBUF_NUM];
	avs3_frame_t* next_be_decode_pic[MAX_FB_IFBUF_NUM];
	/**/
	/*for WRITE_BACK_RET*/
	uint32_t sys_imem_ptr;
	void *sys_imem_ptr_v;
	void *fb_buf_sys_imem_addr;
	uint32_t  instruction[256*4]; //avoid code crash, but only 256 used
	uint32_t  ins_offset;
#endif
#ifdef AML
	int max_pb_size;
	int8_t bufmgr_error_flag;
	u64 start_time;
	//int32_t ref_maxbuffer;
#endif
} avs3_decoder_t;

void avs3_bufmgr_init(struct avs3_decoder *hw);

int com_picman_out_libpic(COM_PIC * pic, int library_picture_index, COM_PM * pm);
COM_PIC * com_picman_out_pic(COM_PM * pm, int * err, int cur_pic_doi, int state);
int com_picman_deinit(COM_PM * pm);
int com_picman_init(COM_PM * pm, int max_pb_size, int max_num_ref_pics, PICBUF_ALLOCATOR * pa);
int com_picman_dpbpic_doi_minus_cycle_length( COM_PM *pm );
int com_picman_check_repeat_doi(COM_PM * pm, COM_PIC_HEADER * pic_header);
int com_construct_ref_list_doi( COM_PIC_HEADER *pic_header );
int com_picman_refpic_marking_decoder(COM_PM *pm, COM_PIC_HEADER *pic_header);
int com_cleanup_useless_pic_buffer_in_pm( COM_PM *pm );
int com_picman_refp_rpl_based_init_decoder(COM_PM *pm, COM_PIC_HEADER *pic_header, COM_REFP(*refp)[REFP_NUM]);
void com_picman_print_state(COM_PM * pm);
COM_PIC * com_picman_get_empty_pic(COM_PM * pm, int * err);
int com_picman_put_libpic(COM_PM * pm, COM_PIC * pic, int slice_type, u32 ptr, u32 dtr, u8 temporal_id, int need_for_output, COM_REFP(*refp)[REFP_NUM], COM_PIC_HEADER * pic_header);
int com_picman_put_pic(COM_PM * pm, COM_PIC * pic, int slice_type, u32 ptr, u32 dtr,
	u32 picture_output_delay, u8 temporal_id, int need_for_output, COM_REFP(*refp)[REFP_NUM]);
void com_pic_free(struct avs3_decoder *hw, PICBUF_ALLOCATOR *pa, COM_PIC *pic);
COM_PIC * com_pic_alloc(struct avs3_decoder *hw, PICBUF_ALLOCATOR * pa, int * ret);

int dec_eco_pic_header(union param_u *param, COM_PIC_HEADER * pic_header, COM_SQH * sqh, int* need_minus_256, unsigned int start_code);
int dec_eco_patch_header(union param_u *param, COM_SQH *sqh, COM_PIC_HEADER * ph, COM_SH_EXT * sh,PATCH_INFO *patch);
int dec_eco_sqh(union param_u *param, COM_SQH * sqh);
int dec_eco_pic_header(union param_u *param, COM_PIC_HEADER * pic_header, COM_SQH * sqh, int* need_minus_256, unsigned int start_code);
int dec_eco_alf_coeff(union param_u *rpm_param, ALF_PARAM *alf_param);
int dec_eco_alf_param(union param_u *rpm_param, COM_PIC_HEADER *sh
#if ALF_SHAPE
				, int num_coef
#endif
#if ALF_IMP
	, int max_filter_num
#endif
);

#define AVS3_DBG_BUFMGR                   0x01
#define AVS3_DBG_IRQ_EVENT                0x02
#define AVS3_DBG_BUFMGR_MORE              0x04
#define AVS3_DBG_BUFMGR_DETAIL            0x08
#define AVS3_DBG_PRINT_PARAM              0x10
#define AVS3_DBG_PRINT_PIC_LIST           0x20
#define AVS3_DBG_OUT_PTS                  0x40
#define AVS3_DBG_PRINT_SOURCE_LINE        0x80
#define AVS3_DBG_SEND_PARAM_WITH_REG      0x100
#define AVS3_DBG_MERGE                    0x200
#define AVS3_DBG_NOT_RECYCLE_MMU_TAIL     0x400
#define AVS3_DBG_REG                      0x800
#define AVS3_DBG_PIC_LEAK                 0x1000
#define AVS3_DBG_PIC_LEAK_WAIT            0x2000
#define AVS3_DBG_HDR_INFO                 0x4000
#define AVS3_DBG_QOS_INFO                 0x8000
#define AVS3_DBG_DIS_LOC_ERROR_PROC       0x10000
#define AVS3_DBG_DIS_SYS_ERROR_PROC   0x20000
#define AVS3_DBG_DUMP_PIC_LIST       0x40000
#define AVS3_DBG_TRIG_SLICE_SEGMENT_PROC 0x80000
#define AVS3_DBG_DISABLE_IQIT_SCALELUT_INIT  0x100000
//#define AVS3_DBG_BE_SIMULATE_IRQ       0x200000
#define AVS3_DBG_SAO_CRC                0x200000
#define AVS3_DBG_FORCE_SEND_AGAIN       0x400000
#define AVS3_DBG_DUMP_DATA              0x800000
#define AVS3_DBG_DUMP_LMEM_BUF         0x1000000
#define AVS3_DBG_DUMP_RPM_BUF          0x2000000
#define AVS3_DBG_CACHE                 0x4000000
#define IGNORE_PARAM_FROM_CONFIG         0x8000000
/*MULTI_INSTANCE_SUPPORT*/
#define PRINT_FLAG_ERROR        0
#define PRINT_FLAG_VDEC_STATUS             0x20000000
#define PRINT_FLAG_VDEC_DETAIL             0x40000000
#define PRINT_FLAG_VDEC_DATA             0x80000000

u32 avs3_get_debug_flag(void);

bool is_avs3_print_bufmgr_detail(void);

struct AVS3Decoder_s;

extern u32 debug_mask;
extern	int avs3_debug(struct AVS3Decoder_s *dec,
		int flag, const char *fmt, ...);


#define avs3_print(dec, flag, fmt, args...)					\
	do {									\
		if (dec == NULL ||    \
			(flag == 0) || \
			((debug_mask & \
			(1 << dec->index)) \
		&& (debug & flag))) { \
			avs3_debug(dec, flag, fmt, ##args);	\
			} \
	} while (0)

//int avs3_print(struct AVS3Decoder_s *dec,
//	int flag, const char *fmt, ...);
#define assert(x)
#ifdef DEBUG_AMRISC
#ifndef DEBUG_RELEASE_MODE
#define printf(...) do {\
	if (is_avs3_print_bufmgr_detail()) \
	avs3_debug(NULL, 0, __VA_ARGS__); \
} while (0)

#define PRINT_LINE() \
	do { \
	if (avs3_get_debug_flag() & AVS3_DBG_PRINT_SOURCE_LINE)\
	avs3_debug(NULL, 0, "%s line %d\n", __func__, __LINE__);\
	} while (0)
#else
#define printf(...) do {\
		; \
	} while (0)

#define PRINT_LINE() \
		do { \
		; \
		} while (0)
#endif
#else

#define printf(...) do {\
	if (is_avs3_print_bufmgr_detail()) \
	printk(__VA_ARGS__); \
} while (0)

#define PRINT_LINE() \
	do { \
	if (avs3_get_debug_flag() & AVS3_DBG_PRINT_SOURCE_LINE)\
	pr_info("%s line %d\n", __func__, __LINE__);\
	} while (0)

#endif

void create_alf_global_buffer(DEC_CTX *ctx);
//void init_pic_list(struct avs3_decoder *hw);
COM_PIC *dec_pull_frm(DEC_CTX *ctx, int state);
void print_pic_pool(struct avs3_decoder *hw, char *mark);
void init_pic_pool(struct avs3_decoder *hw);
int avs3_bufmgr_process(struct avs3_decoder *hw, int start_code);
int avs3_bufmgr_post_process(struct avs3_decoder *hw);
void avs3_cleanup_useless_pic_buffer_in_pm(struct avs3_decoder *hw);
void print_alf_param(union param_u * param);
void print_param(union param_u * param);
int avs3_get_error_policy(void);

#endif

