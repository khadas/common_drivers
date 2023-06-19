#include "../../../include/regs/dos_registers.h"

#define LPF_SPCC_ENABLE

/* to do */
//#define DOUBLE_WRITE_VH0_TEMP    0
//#define DOUBLE_WRITE_VH1_TEMP    0
//#define DOUBLE_WRITE_VH0_HALF    0
//#define DOUBLE_WRITE_VH1_HALF    0
//#define DOS_BASE_ADR  0

//#define HEVCD_MPP_DECOMP_AXIURG_CTL 0

/**/
#ifdef FOR_S5
ulong dos_reg_compat_convert(ulong adr);
#endif
#define print_scratch_error(a)
//#define MEM_MAP_MODE    0

static void init_pic_list_hw_fb(struct AVS2Decoder_s *dec);

//typedef union param_u param_t;
static void WRITE_BACK_RET(struct avs2_decoder *avs2_dec)
{
	struct AVS2Decoder_s *dec = container_of(avs2_dec,
		struct AVS2Decoder_s, avs2_dec);

	avs2_dec->instruction[avs2_dec->ins_offset] = 0xcc00000;   //ret
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"WRITE_BACK_RET()\ninstruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = 0;           //nop
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
}

static void WRITE_BACK_8(struct avs2_decoder *avs2_dec, uint32_t spr_addr, uint8_t data)
{
	struct AVS2Decoder_s *dec = container_of(avs2_dec,
		struct AVS2Decoder_s, avs2_dec);

	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x20<<22) | ((spr_addr&0xfff)<<8) | (data&0xff);   //mtspi data, spr_addr
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
	avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (avs2_dec->ins_offset < 256 && (avs2_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs2_dec);
		avs2_dec->ins_offset = 256;
	}
#endif
}

static void WRITE_BACK_16(struct avs2_decoder *avs2_dec, uint32_t spr_addr, uint8_t rd_addr, uint16_t data)
{
	struct AVS2Decoder_s *dec = container_of(avs2_dec,
		struct AVS2Decoder_s, avs2_dec);

	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x1a<<22) | ((data&0xffff)<<6) | (rd_addr&0x3f);       // movi rd_addr, data[15:0]
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"%s(%x,%x,%x)\ninstruction[%3d] = %8x, data= %x\n",
		__func__, spr_addr, rd_addr, data,
	avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset], data&0xffff);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8) | (rd_addr&0x3f);  // mtsp rd_addr, spr_addr
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
		avs2_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (avs2_dec->ins_offset < 256 && (avs2_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs2_dec);
		avs2_dec->ins_offset = 256;
	}
#endif
}

static void WRITE_BACK_32(struct avs2_decoder *avs2_dec, uint32_t spr_addr, uint32_t data)
{
	struct AVS2Decoder_s *dec = container_of(avs2_dec,
		struct AVS2Decoder_s, avs2_dec);

	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x1a<<22) | ((data&0xffff)<<6);   // movi COMMON_REG_0, data
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
	avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	data = (data & 0xffff0000)>>16;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x1b<<22) | (data<<6);                // mvihi COMMON_REG_0, data[31:16]
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8);    // mtsp COMMON_REG_0, spr_addr
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (avs2_dec->ins_offset < 256 && (avs2_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs2_dec);
		avs2_dec->ins_offset = 256;
	}
#endif
}

static void READ_INS_WRITE(struct avs2_decoder *avs2_dec, uint32_t spr_addr0, uint32_t spr_addr1, uint8_t rd_addr, uint8_t position, uint8_t size)
{
	struct AVS2Decoder_s *dec = container_of(avs2_dec,
		struct AVS2Decoder_s, avs2_dec);

	spr_addr0 = (dos_reg_compat_convert(spr_addr0) & 0xfff);
	spr_addr1 = (dos_reg_compat_convert(spr_addr1) & 0xfff);
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x19<<22) | ((spr_addr0&0xfff)<<8) | (rd_addr&0x3f);    //mfsp rd_addr, src_addr0
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"%s(%x,%x,%x,%x,%x)\n",
		__func__, spr_addr0, spr_addr1, rd_addr, position, size);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | ((rd_addr&0x3f)<<6) | (rd_addr&0x3f);   //ins rd_addr, rd_addr, position, size
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x18<<22) | ((spr_addr1&0xfff)<<8) | (rd_addr&0x3f);    //mtsp rd_addr, src_addr1
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (avs2_dec->ins_offset < 256 && (avs2_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs2_dec);
		avs2_dec->ins_offset = 256;
	}
#endif
}

//Caution:  pc offset fixed to 4, the data of cmp_addr need ready before call this function
void READ_CMP_WRITE(struct avs2_decoder *avs2_dec, uint32_t spr_addr0, uint32_t spr_addr1, uint8_t rd_addr, uint8_t cmp_addr, uint8_t position, uint8_t size)
{
	struct AVS2Decoder_s *dec = container_of(avs2_dec,
		struct AVS2Decoder_s, avs2_dec);

	spr_addr0 = (dos_reg_compat_convert(spr_addr0) & 0xfff);
	spr_addr1 = (dos_reg_compat_convert(spr_addr1) & 0xfff);
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x19<<22) | ((spr_addr0&0xfff)<<8) | (rd_addr&0x3f);    //mfsp rd_addr, src_addr0
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"%s(%x,%x,%x,%x,%x,%x)\n",
		__func__, spr_addr0, spr_addr1, rd_addr, cmp_addr, position, size);

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | ((rd_addr&0x3f)<<6) | (rd_addr&0x3f);   //ins rd_addr, rd_addr, position, size
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x29<<22) | (4<<12) | ((rd_addr&0x3f)<<6) | cmp_addr;     //cbne current_pc+4, rd_addr, cmp_addr
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = 0;                                                       //nop
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x19<<22) | ((spr_addr0&0xfff)<<8) | (rd_addr&0x3f);    //mfsp rd_addr, src_addr0
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | ((rd_addr&0x3f)<<6) | (rd_addr&0x3f);   //ins rd_addr, rd_addr, position, size
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x18<<22) | ((spr_addr1&0xfff)<<8) | (rd_addr&0x3f);    //mtsp rd_addr, src_addr1
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (avs2_dec->ins_offset < 256 && (avs2_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs2_dec);
		avs2_dec->ins_offset = 256;
	}
#endif
}

static void READ_WRITE_DATA16(struct avs2_decoder *avs2_dec, uint32_t spr_addr, uint16_t data, uint8_t position, uint8_t size)
{
	struct AVS2Decoder_s *dec = container_of(avs2_dec,
		struct AVS2Decoder_s, avs2_dec);

	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x19<<22) | ((spr_addr&0xfff)<<8);    //mfsp COMON_REG_0, spr_addr
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"%s(%x,%x,%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data, position, size,
	avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x1a<<22) | (data<<6) | 1;        //movi COMMON_REG_1, data
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | (0<<6) | 1;  //ins COMMON_REG_0, COMMON_REG_1, position, size
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8);    //mtsp COMMON_REG_0, spr_addr
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n", avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (avs2_dec->ins_offset < 256 && (avs2_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs2_dec);
		avs2_dec->ins_offset = 256;
	}
#endif
}

static int32_t config_mc_buffer_fb(struct AVS2Decoder_s *dec)
{
	int32_t i,j;
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	struct avs2_frame_s *cur_pic = avs2_dec->hc.cur_pic;
	struct avs2_frame_s *pic;

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"Entered config_mc_buffer....\n");
	if (avs2_dec->f_bg != NULL) {
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for background (canvas_y %d, canvas_u_v %d)\n",
		avs2_dec->f_bg->mc_canvas_y, avs2_dec->f_bg->mc_canvas_u_v);
		WRITE_BACK_16(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (15 << 8) | (0<<1) | 1);   // L0:BG

		WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
			(avs2_dec->f_bg->mc_canvas_u_v<<16)|(avs2_dec->f_bg->mc_canvas_u_v<<8)|avs2_dec->f_bg->mc_canvas_y);

		WRITE_BACK_16(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (31 << 8) | (0<<1) | 1);  // L1:BG

		WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
			(avs2_dec->f_bg->mc_canvas_u_v<<16)|(avs2_dec->f_bg->mc_canvas_u_v<<8)|avs2_dec->f_bg->mc_canvas_y);

#if (defined NEW_FRONT_BACK_CODE) && (!defined FB_BUF_DEBUG_NO_PIPLINE)
		for (j = 0; j < MAXREF; j++) {
			if (avs2_dec->f_bg == cur_pic->ref_pic[j])
				break;
			if (cur_pic->ref_pic[j] == NULL) {
				cur_pic->ref_pic[j] = avs2_dec->f_bg;
				break;
			}
		}
#endif
	}

	if (avs2_dec->img.type == I_IMG)
		return 0;

	if (avs2_dec->img.type == P_IMG) {
		int valid_ref_cnt;
		valid_ref_cnt = 0;
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for P_IMG, img type %d\n", avs2_dec->img.type);

		WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);
		for (i = 0; i < avs2_dec->img.num_of_references; i++) {
			pic = avs2_dec->fref[i];
			if (pic->referred_by_others != 1)
				continue;
			valid_ref_cnt++;
			WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
				(pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			if (pic->error_mark)
				cur_pic->error_mark = 1;
			avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
				"refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i, pic->mc_canvas_u_v, pic->mc_canvas_y);
#if (defined NEW_FRONT_BACK_CODE) && (!defined FB_BUF_DEBUG_NO_PIPLINE)
			for (j = 0; j < MAXREF; j++) {
				if (pic == cur_pic->ref_pic[j])
					break;
				if (cur_pic->ref_pic[j] == NULL) {
					cur_pic->ref_pic[j] = pic;
					break;
				}
			}
#endif
		}
		if (valid_ref_cnt != avs2_dec->img.num_of_references)
			cur_pic->error_mark = 1;
	} else if (avs2_dec->img.type == F_IMG) {
		int valid_ref_cnt;
		valid_ref_cnt = 0;
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for F_IMG, img type %d\n", avs2_dec->img.type);

		WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);
		for (i = 0; i < avs2_dec->img.num_of_references; i++) {
			pic = avs2_dec->fref[i];
			if (pic->referred_by_others != 1)
				continue;
			valid_ref_cnt++;
			WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
				(pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			if (pic->error_mark)
				cur_pic->error_mark = 1;
			avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
				"refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i, pic->mc_canvas_u_v, pic->mc_canvas_y);
#if (defined NEW_FRONT_BACK_CODE) && (!defined FB_BUF_DEBUG_NO_PIPLINE)
			for (j = 0; j < MAXREF; j++) {
				if (pic == cur_pic->ref_pic[j])
					break;
				if (cur_pic->ref_pic[j] == NULL) {
					cur_pic->ref_pic[j] = pic;
					break;
				}
			}
#endif
		}
		if (valid_ref_cnt != avs2_dec->img.num_of_references)
			cur_pic->error_mark = 1;
		WRITE_BACK_16(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (16 << 8) | (0<<1) | 1);
		for (i = 0; i < avs2_dec->img.num_of_references; i++) {
			pic = avs2_dec->fref[i];
			WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
				(pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
				"refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i, pic->mc_canvas_u_v, pic->mc_canvas_y);
		}
	} else {
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for B_IMG\n");

		pic = avs2_dec->fref[1];
		WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);

		WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
			(pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
		if (pic->error_mark)
			cur_pic->error_mark = 1;
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"refid %x mc_canvas_u_v %x mc_canvas_y %x\n", 1, pic->mc_canvas_u_v, pic->mc_canvas_y);
#if (defined NEW_FRONT_BACK_CODE) && (!defined FB_BUF_DEBUG_NO_PIPLINE)
		for (j = 0; j < MAXREF; j++) {
			if (pic == cur_pic->ref_pic[j])
				break;
			if (cur_pic->ref_pic[j] == NULL) {
				cur_pic->ref_pic[j] = pic;
				break;
			}
		}
#endif
		pic = avs2_dec->fref[0];
		WRITE_BACK_16(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (16 << 8) | (0<<1) | 1);
		WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, (pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
		if (pic->error_mark)
			cur_pic->error_mark = 1;
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"refid %x mc_canvas_u_v %x mc_canvas_y %x\n", 0, pic->mc_canvas_u_v, pic->mc_canvas_y);
#if (defined NEW_FRONT_BACK_CODE) && (!defined FB_BUF_DEBUG_NO_PIPLINE)
		for (j = 0; j < MAXREF; j++) {
			if (pic == cur_pic->ref_pic[j])
				break;
			if (cur_pic->ref_pic[j] == NULL) {
				cur_pic->ref_pic[j] = pic;
				break;
			}
		}
#endif
	}
	return 0;
}

#ifdef NEW_FRONT_BACK_CODE
/*copy from simulation code*/

#define FB_LMEM_SIZE_LOG2           6
#define FB_LMEM_SIZE             (1 << FB_LMEM_SIZE_LOG2)
#define FB_VCPU_IMEM_SIZE         0x400
#define FB_SYSTEM_IMEM_SIZE       0x400
#define FB_SCALELUT_LMEM_SIZE     0x400

#if 0
#define FB_PARSER_SAO0_BLOCK_SIZE       (4*1024*4)*64
#define FB_PARSER_SAO1_BLOCK_SIZE       (4*1024*4)*64
#define FB_MPRED_IMP0_BLOCK_SIZE        (4*1024*4)*256
#define FB_MPRED_IMP1_BLOCK_SIZE        (4*1024*4)*256
#else
#define FB_PARSER_SAO0_BLOCK_SIZE       (4*1024*4)
#define FB_PARSER_SAO1_BLOCK_SIZE       (4*1024*4)
#define FB_MPRED_IMP0_BLOCK_SIZE        (4*1024*4)
#define FB_MPRED_IMP1_BLOCK_SIZE        (4*1024*4)
#endif
/**/
#define BUF_BLOCK_NUM 256

#define FB_IFBUF_SCALELUT_BLOCK_SIZE       FB_SCALELUT_LMEM_SIZE
#define FB_IFBUF_VCPU_IMEM_BLOCK_SIZE FB_VCPU_IMEM_SIZE
#define FB_IFBUF_SYS_IMEM_BLOCK_SIZE FB_SYSTEM_IMEM_SIZE
#define FB_IFBUF_LMEM0_BLOCK_SIZE   FB_LMEM_SIZE
#define FB_IFBUF_LMEM1_BLOCK_SIZE   FB_LMEM_SIZE

#define IFBUF_SCALELUT_SIZE                  (FB_IFBUF_SCALELUT_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_VCPU_IMEM_SIZE        (FB_IFBUF_VCPU_IMEM_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_SYS_IMEM_SIZE        (FB_IFBUF_SYS_IMEM_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_LMEM0_SIZE                  (FB_IFBUF_LMEM0_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_LMEM1_SIZE                  (FB_IFBUF_LMEM1_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_PARSER_SAO0_SIZE      (FB_PARSER_SAO0_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_PARSER_SAO1_SIZE      (FB_PARSER_SAO1_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_MPRED_IMP0_SIZE      (FB_MPRED_IMP0_BLOCK_SIZE * BUF_BLOCK_NUM)
#define IFBUF_MPRED_IMP1_SIZE      (FB_MPRED_IMP1_BLOCK_SIZE * BUF_BLOCK_NUM)

static void copy_loopbufs_ptr(buff_ptr_t* trg, buff_ptr_t* src)
{
	trg->mmu0_ptr = src->mmu0_ptr;
	trg->mmu1_ptr = src->mmu1_ptr;
	trg->scalelut_ptr = src->scalelut_ptr;
	trg->vcpu_imem_ptr = src->vcpu_imem_ptr;
	trg->sys_imem_ptr = src->sys_imem_ptr;
	trg->sys_imem_ptr_v = src->sys_imem_ptr_v;
	trg->lmem0_ptr = src->lmem0_ptr;
	trg->lmem1_ptr = src->lmem1_ptr;
	trg->parser_sao0_ptr = src->parser_sao0_ptr;
	trg->parser_sao1_ptr = src->parser_sao1_ptr;
	trg->mpred_imp0_ptr = src->mpred_imp0_ptr;
	trg->mpred_imp1_ptr = src->mpred_imp1_ptr;
	//
	trg->scalelut_ptr_pre = src->scalelut_ptr_pre;
	trg->scalelut_ptr_changed = src->scalelut_ptr_changed;
}

static void print_loopbufs_ptr(struct AVS2Decoder_s *dec, char* mark, buff_ptr_t* ptr)
{
	avs2_print(dec, AVS2_DBG_BUFMGR,
		"%s:mmu0_ptr 0x%x, mmu1_ptr 0x%x, scalelut_ptr 0x%x (changed %d) pre_ptr 0x%x, vcpu_imem_ptr 0x%x, sys_imem_ptr 0x%x (vir 0x%x), lmem0_ptr 0x%x, lmem1_ptr 0x%x, parser_sao0_ptr 0x%x, parser_sao1_ptr 0x%x, mpred_imp0_ptr 0x%x, mpred_imp1_ptr 0x%x\n",
		mark,
		ptr->mmu0_ptr,
		ptr->mmu1_ptr,
		ptr->scalelut_ptr,
		ptr->scalelut_ptr_changed,
		ptr->scalelut_ptr_pre,
		ptr->vcpu_imem_ptr,
		ptr->sys_imem_ptr,
		ptr->sys_imem_ptr_v,
		ptr->lmem0_ptr,
		ptr->lmem1_ptr,
		ptr->parser_sao0_ptr,
		ptr->parser_sao1_ptr,
		ptr->mpred_imp0_ptr,
		ptr->mpred_imp1_ptr);
}

static int init_mmu_fb_bufstate(struct AVS2Decoder_s *dec, int mmu_fb_4k_number)
{
	int ret;
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	dma_addr_t tmp_phy_adr;
	int mmu_map_size = ((mmu_fb_4k_number * 4) >> 6) << 6;

	avs2_print(dec, AVS2_DBG_BUFMGR,
		"%s: mmu_fb_4k_number %d\n", __func__, mmu_fb_4k_number);

	if (mmu_fb_4k_number < 0)
		return -1;

	dec->mmu_box_fb = decoder_mmu_box_alloc_box(DRIVER_NAME,
	dec->index, 2, (mmu_fb_4k_number << 12) * 2, 0);

	dec->fb_buf_mmu0_addr = dma_alloc_coherent(amports_get_dma_device(),
		mmu_map_size, &tmp_phy_adr, GFP_KERNEL);
	avs2_dec->fb_buf_mmu0.buf_start = tmp_phy_adr;
	if (dec->fb_buf_mmu0_addr == NULL) {
		avs2_print(dec, 0, "%s: failed to alloc fb_mmu0_map\n", __func__);
		return -1;
	}
	memset(dec->fb_buf_mmu0_addr, 0, mmu_map_size);
	avs2_dec->fb_buf_mmu0.buf_size = mmu_map_size;
	avs2_dec->fb_buf_mmu0.buf_end = avs2_dec->fb_buf_mmu0.buf_start + mmu_map_size;

	dec->fb_buf_mmu1_addr = dma_alloc_coherent(amports_get_dma_device(),
		mmu_map_size, &tmp_phy_adr, GFP_KERNEL);
	avs2_dec->fb_buf_mmu1.buf_start = tmp_phy_adr;
	if (dec->fb_buf_mmu1_addr == NULL) {
		avs2_print(dec, 0, "%s: failed to alloc fb_mmu1_map\n", __func__);
		return -1;
	}
	memset(dec->fb_buf_mmu1_addr, 0, mmu_map_size);
	avs2_dec->fb_buf_mmu1.buf_size = mmu_map_size;
	avs2_dec->fb_buf_mmu1.buf_end = avs2_dec->fb_buf_mmu1.buf_start + mmu_map_size;

	ret = decoder_mmu_box_alloc_idx(dec->mmu_box_fb,
		0, mmu_fb_4k_number, dec->fb_buf_mmu0_addr);
	if (ret != 0) {
		avs2_print(dec, 0, "%s: failed to alloc fb_mmu0 pages");
		return -1;
	}

	ret = decoder_mmu_box_alloc_idx(dec->mmu_box_fb,
		1, mmu_fb_4k_number, dec->fb_buf_mmu1_addr);
	if (ret != 0) {
		avs2_print(dec, 0, "%s: failed to alloc fb_mmu1 pages");
		return -1;
	}

	dec->mmu_fb_4k_number = mmu_fb_4k_number;
	avs2_dec->fr.mmu0_ptr = avs2_dec->fb_buf_mmu0.buf_start;
	avs2_dec->bk.mmu0_ptr = avs2_dec->fb_buf_mmu0.buf_start;
	avs2_dec->fr.mmu1_ptr = avs2_dec->fb_buf_mmu1.buf_start;
	avs2_dec->bk.mmu1_ptr = avs2_dec->fb_buf_mmu1.buf_start;

	return 0;
}

static void init_fb_bufstate(struct AVS2Decoder_s *dec)
{
/*simulation code: change to use linux APIs; also need write uninit_fb_bufstate()*/
	int ret;
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	dma_addr_t tmp_phy_adr;
	unsigned long tmp_adr;
	int mmu_fb_4k_number = dec->fb_ifbuf_num * avs2_mmu_page_num(dec,
		dec->init_pic_w, dec->init_pic_h, 1);

	ret = init_mmu_fb_bufstate(dec, mmu_fb_4k_number);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc mmu fb buffer\n", __func__);
		return ;
	}

	avs2_dec->fb_buf_scalelut.buf_size = IFBUF_SCALELUT_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_SCALELUT_ID, avs2_dec->fb_buf_scalelut.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_scalelut.buf_size);
	}

	avs2_dec->fb_buf_scalelut.buf_start = tmp_adr;
	avs2_dec->fb_buf_scalelut.buf_end = avs2_dec->fb_buf_scalelut.buf_start + avs2_dec->fb_buf_scalelut.buf_size;

	avs2_dec->fb_buf_vcpu_imem.buf_size = IFBUF_VCPU_IMEM_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_VCPU_IMEM_ID, avs2_dec->fb_buf_vcpu_imem.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_vcpu_imem.buf_size);
	}

	avs2_dec->fb_buf_vcpu_imem.buf_start = tmp_adr;
	avs2_dec->fb_buf_vcpu_imem.buf_end = avs2_dec->fb_buf_vcpu_imem.buf_start + avs2_dec->fb_buf_vcpu_imem.buf_size;

	avs2_dec->fb_buf_sys_imem.buf_size = IFBUF_SYS_IMEM_SIZE * dec->fb_ifbuf_num;
	avs2_dec->fb_buf_sys_imem_addr = dma_alloc_coherent(amports_get_dma_device(),
		avs2_dec->fb_buf_sys_imem.buf_size, &tmp_phy_adr, GFP_KERNEL);
	avs2_dec->fb_buf_sys_imem.buf_start = tmp_phy_adr;
	if (avs2_dec->fb_buf_sys_imem_addr == NULL) {
		pr_err("%s: failed to alloc fb_buf_sys_imem\n", __func__);
		return;
	}
	avs2_dec->fb_buf_sys_imem.buf_end = avs2_dec->fb_buf_sys_imem.buf_start + avs2_dec->fb_buf_sys_imem.buf_size;

	avs2_dec->fb_buf_lmem0.buf_size = IFBUF_LMEM0_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_LMEM0_ID, avs2_dec->fb_buf_lmem0.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_lmem0.buf_size);
	}

	avs2_dec->fb_buf_lmem0.buf_start = tmp_adr;
	avs2_dec->fb_buf_lmem0.buf_end = avs2_dec->fb_buf_lmem0.buf_start + avs2_dec->fb_buf_lmem0.buf_size;

	avs2_dec->fb_buf_lmem1.buf_size = IFBUF_LMEM1_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_LMEM1_ID, avs2_dec->fb_buf_lmem1.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_lmem1.buf_size);
	}

	avs2_dec->fb_buf_lmem1.buf_start = tmp_adr;
	avs2_dec->fb_buf_lmem1.buf_end = avs2_dec->fb_buf_lmem1.buf_start + avs2_dec->fb_buf_lmem1.buf_size;

	avs2_dec->fb_buf_parser_sao0.buf_size = IFBUF_PARSER_SAO0_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_PARSER_SAO0_ID, avs2_dec->fb_buf_parser_sao0.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_parser_sao0.buf_size);
	}

	avs2_dec->fb_buf_parser_sao0.buf_start = tmp_adr;
	avs2_dec->fb_buf_parser_sao0.buf_end = avs2_dec->fb_buf_parser_sao0.buf_start + avs2_dec->fb_buf_parser_sao0.buf_size;

	avs2_dec->fb_buf_parser_sao1.buf_size = IFBUF_PARSER_SAO1_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_PARSER_SAO1_ID, avs2_dec->fb_buf_parser_sao1.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_parser_sao1.buf_size);
	}

	avs2_dec->fb_buf_parser_sao1.buf_start = tmp_adr;
	avs2_dec->fb_buf_parser_sao1.buf_end = avs2_dec->fb_buf_parser_sao1.buf_start + avs2_dec->fb_buf_parser_sao1.buf_size;

	avs2_dec->fb_buf_mpred_imp0.buf_size = IFBUF_MPRED_IMP0_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUFF_MPRED_IMP0_ID, avs2_dec->fb_buf_mpred_imp0.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_mpred_imp0.buf_size);
	}

	avs2_dec->fb_buf_mpred_imp0.buf_start = tmp_adr;
	avs2_dec->fb_buf_mpred_imp0.buf_end = avs2_dec->fb_buf_mpred_imp0.buf_start + avs2_dec->fb_buf_mpred_imp0.buf_size;

	avs2_dec->fb_buf_mpred_imp1.buf_size = IFBUF_MPRED_IMP1_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUFF_MPRED_IMP1_ID, avs2_dec->fb_buf_mpred_imp1.buf_size,
		DRIVER_NAME, &tmp_adr);
	if (ret) {
		avs2_print(dec, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
	} else {
		if (!vdec_secure(hw_to_vdec(dec)))
			codec_mm_memset(tmp_adr, 0, avs2_dec->fb_buf_mpred_imp1.buf_size);
	}

	avs2_dec->fb_buf_mpred_imp1.buf_start = tmp_adr;
	avs2_dec->fb_buf_mpred_imp1.buf_end = avs2_dec->fb_buf_mpred_imp1.buf_start + avs2_dec->fb_buf_mpred_imp1.buf_size;

	avs2_dec->fr.scalelut_ptr = avs2_dec->fb_buf_scalelut.buf_start;
	avs2_dec->bk.scalelut_ptr = avs2_dec->fb_buf_scalelut.buf_start;
	avs2_dec->fr.vcpu_imem_ptr = avs2_dec->fb_buf_vcpu_imem.buf_start;
	avs2_dec->bk.vcpu_imem_ptr = avs2_dec->fb_buf_vcpu_imem.buf_start;
	avs2_dec->fr.sys_imem_ptr = avs2_dec->fb_buf_sys_imem.buf_start;
	avs2_dec->bk.sys_imem_ptr = avs2_dec->fb_buf_sys_imem.buf_start;
	avs2_dec->fr.lmem0_ptr = avs2_dec->fb_buf_lmem0.buf_start;
	avs2_dec->bk.lmem0_ptr = avs2_dec->fb_buf_lmem0.buf_start;
	avs2_dec->fr.lmem1_ptr = avs2_dec->fb_buf_lmem1.buf_start;
	avs2_dec->bk.lmem1_ptr = avs2_dec->fb_buf_lmem1.buf_start;
	avs2_dec->fr.parser_sao0_ptr = avs2_dec->fb_buf_parser_sao0.buf_start;
	avs2_dec->bk.parser_sao0_ptr = avs2_dec->fb_buf_parser_sao0.buf_start;
	avs2_dec->fr.parser_sao1_ptr = avs2_dec->fb_buf_parser_sao1.buf_start;
	avs2_dec->bk.parser_sao1_ptr = avs2_dec->fb_buf_parser_sao1.buf_start;
	avs2_dec->fr.mpred_imp0_ptr = avs2_dec->fb_buf_mpred_imp0.buf_start;
	avs2_dec->bk.mpred_imp0_ptr = avs2_dec->fb_buf_mpred_imp0.buf_start;
	avs2_dec->fr.mpred_imp1_ptr = avs2_dec->fb_buf_mpred_imp1.buf_start;
	avs2_dec->bk.mpred_imp1_ptr = avs2_dec->fb_buf_mpred_imp1.buf_start;
	avs2_dec->fr.scalelut_ptr_pre = 0;
	avs2_dec->bk.scalelut_ptr_pre = 0;
	avs2_dec->fr.scalelut_ptr_changed = 0;
	avs2_dec->bk.scalelut_ptr_changed = 0;

	avs2_dec->fr.sys_imem_ptr_v = avs2_dec->fb_buf_sys_imem_addr; //for linux

	print_loopbufs_ptr(dec, "init", &avs2_dec->fr);
}

static void uninit_mmu_fb_bufstate(struct AVS2Decoder_s *dec)
{
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;

	if (dec->fb_buf_mmu0_addr) {
		dma_free_coherent(amports_get_dma_device(),
			avs2_dec->fb_buf_mmu0.buf_size, dec->fb_buf_mmu0_addr,
			avs2_dec->fb_buf_mmu0.buf_start);
		dec->fb_buf_mmu0_addr = NULL;
	}

	if (dec->fb_buf_mmu1_addr) {
		dma_free_coherent(amports_get_dma_device(),
			avs2_dec->fb_buf_mmu1.buf_size, dec->fb_buf_mmu1_addr,
			avs2_dec->fb_buf_mmu1.buf_start);
		dec->fb_buf_mmu1_addr = NULL;
	}

	if (dec->mmu_box_fb) {
		decoder_mmu_box_free(dec->mmu_box_fb);
		dec->mmu_box_fb = NULL;
	}

	dec->mmu_fb_4k_number = 0;
}

static void uninit_fb_bufstate(struct AVS2Decoder_s *dec)
{
	int i;
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;

	for (i = 0; i < FB_LOOP_BUF_COUNT; i++) {
		if (i != BMMU_IFBUF_SYS_IMEM_ID)
			decoder_bmmu_box_free_idx(dec->bmmu_box, i);
	}

	if (avs2_dec->fb_buf_sys_imem_addr) {
		dma_free_coherent(amports_get_dma_device(),
			avs2_dec->fb_buf_sys_imem.buf_size, avs2_dec->fb_buf_sys_imem_addr,
			avs2_dec->fb_buf_sys_imem.buf_start);
		avs2_dec->fb_buf_sys_imem_addr = NULL;
	}

	uninit_mmu_fb_bufstate(dec);
}

static void config_bufstate_front_hw(struct avs2_decoder *avs2_dec)
{
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_START, avs2_dec->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_END, avs2_dec->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0, avs2_dec->fr.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_START, avs2_dec->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_END, avs2_dec->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1, avs2_dec->fr.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_parser_sao0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_parser_sao0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.parser_sao0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.parser_sao0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_parser_sao1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_parser_sao1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.parser_sao1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.parser_sao1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	//    config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	// config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_scalelut.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_scalelut.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.scalelut_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.scalelut_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, avs2_dec->fr.scalelut_ptr_pre);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_vcpu_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_vcpu_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.vcpu_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.vcpu_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	//config lmem buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_lmem0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_lmem0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.lmem0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.lmem0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, FB_IFBUF_LMEM0_BLOCK_SIZE);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs2_dec->fb_buf_lmem1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs2_dec->fb_buf_lmem1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs2_dec->fr.lmem1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs2_dec->bk.lmem1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, FB_IFBUF_LMEM1_BLOCK_SIZE);
}

static void config_bufstate_back_hw(struct avs2_decoder *avs2_dec)
{
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_START, avs2_dec->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_END, avs2_dec->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0, avs2_dec->bk.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_START, avs2_dec->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_END, avs2_dec->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1, avs2_dec->bk.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_parser_sao0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_parser_sao0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.parser_sao0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_parser_sao1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_parser_sao1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.parser_sao1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	//    config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.mpred_imp0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.mpred_imp1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	// config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_scalelut.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_scalelut.buf_end);

	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.scalelut_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_vcpu_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_vcpu_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.vcpu_imem_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.sys_imem_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	// config lmem buffers
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_lmem0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_lmem0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.lmem0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs2_dec->fb_buf_lmem1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs2_dec->fb_buf_lmem1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs2_dec->bk.lmem1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
}

static void read_bufstate_front(struct avs2_decoder *avs2_dec)
{
	//struct AVS2Decoder_s *dec = container_of(avs2_dec,
		//struct AVS2Decoder_s, avs2_dec);
	//uint32_t tmp;
	avs2_dec->fr.mmu0_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0);
	avs2_dec->fr.mmu1_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);

	avs2_dec->fr.scalelut_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	//avs2_dec->fr.scalelut_ptr_pre = READ_VREG(HEVC_ASSIST_RING_F_THRESHOLD);
	//avs2_print(dec, PRINT_FLAG_VDEC_DETAIL,
		//"pic_end_ptr = %x; pic_start_ptr = %x\n", avs2_dec->fr.scalelut_ptr, avs2_dec->fr.scalelut_ptr_pre);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	avs2_dec->fr.vcpu_imem_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	avs2_dec->fr.lmem0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	avs2_dec->fr.lmem1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	avs2_dec->fr.parser_sao0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	avs2_dec->fr.parser_sao1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	avs2_dec->fr.mpred_imp0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	avs2_dec->fr.mpred_imp1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);\

	avs2_dec->fr.sys_imem_ptr = avs2_dec->sys_imem_ptr;
	avs2_dec->fr.sys_imem_ptr_v = avs2_dec->sys_imem_ptr_v;
}

static int  compute_losless_comp_body_size(struct AVS2Decoder_s *dec,
	int width, int height,
	bool is_bit_depth_10);
static  int  compute_losless_comp_header_size(struct AVS2Decoder_s *dec,
	int width, int height);

static void config_work_space_hw(struct AVS2Decoder_s *dec, uint8_t front_flag, uint8_t back_flag)
{
	uint32_t data32;
	struct BuffInfo_s* buf_spec = &dec->work_space_buf_store;
	int is_bit_depth_10 = (dec->avs2_dec.input.sample_bit_depth == 8) ? 0 : 1;
	int losless_comp_header_size =
		compute_losless_comp_header_size(
			dec, dec->init_pic_w,
			dec->init_pic_h);
	int losless_comp_body_size =
		compute_losless_comp_body_size(dec,
			dec->init_pic_w,
			dec->init_pic_h, is_bit_depth_10);
#ifdef AVS2_10B_MMU_DW
	int losless_comp_body_size_dw = losless_comp_body_size;
	int losless_comp_header_size_dw = losless_comp_header_size;
#endif
	if (front_flag) {
		avs2_print(dec, AVS2_DBG_BUFMGR,
			"%s front %x %x %x %x %x %x %x %x\n", __func__,
			buf_spec->start_adr,
			buf_spec->rpm.buf_start,
			buf_spec->short_term_rps.buf_start,
			buf_spec->rcs.buf_start,
			buf_spec->sps.buf_start,
			buf_spec->pps.buf_start,
			buf_spec->swap_buf.buf_start,
			buf_spec->swap_buf2.buf_start,
			buf_spec->scalelut.buf_start);
		WRITE_VREG(HEVC_RPM_BUFFER, (u32)dec->rpm_phy_addr);
		WRITE_VREG(AVS2_ALF_SWAP_BUFFER, buf_spec->short_term_rps.buf_start);
		WRITE_VREG(HEVC_RCS_BUFFER, buf_spec->rcs.buf_start);
		WRITE_VREG(HEVC_SPS_BUFFER, buf_spec->sps.buf_start);
		WRITE_VREG(HEVC_PPS_BUFFER, buf_spec->pps.buf_start);
	}

	if (back_flag) {
		avs2_print(dec, AVS2_DBG_BUFMGR,
			"%s back %x %x %x %x %x %x %x\n", __func__,
			buf_spec->ipp.buf_start,
			buf_spec->ipp1.buf_start,
			buf_spec->sao_up.buf_start,
			buf_spec->scalelut.buf_start,
			buf_spec->dblk_para.buf_start,
			buf_spec->dblk_data.buf_start,
			buf_spec->dblk_data2.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE,buf_spec->ipp.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE_DBE1,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2_DBE1,buf_spec->ipp.buf_start);
#ifdef AVS2_10B_MMU
		//WRITE_VREG(H265_MMU_MAP_BUFFER, FRAME_MMU_MAP_ADDR);
		WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR, dec->frame_mmu_map_phy_addr);
		WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR_DBE1, dec->frame_mmu_map_phy_addr_1); //new dual
#else
		// WRITE_VREG(HEVC_STREAM_SWAP_BUFFER, buf_spec->swap_buf.buf_start);
#endif
#ifdef AVS2_10B_MMU_DW
		if (dec->dw_mmu_enable) {
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2, dec->dw_frame_mmu_map_phy_addr);
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2_DBE1, dec->dw_frame_mmu_map_phy_addr_1); //new dual
		}
#endif
		//WRITE_VREG(HEVC_SCALELUT, buf_spec->scalelut.buf_start);

		WRITE_VREG(HEVC_DBLK_CFG4, buf_spec->dblk_para.buf_start);
		WRITE_VREG(HEVC_DBLK_CFG4_DBE1, buf_spec->dblk_para.buf_start);
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			" [DBLK DBG] CFG4: 0x%x\n", buf_spec->dblk_para.buf_start);
		WRITE_VREG(HEVC_DBLK_CFG5, buf_spec->dblk_data.buf_start);
		WRITE_VREG(HEVC_DBLK_CFG5_DBE1, buf_spec->dblk_data.buf_start);
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			" [DBLK DBG] CFG5: 0x%x\n", buf_spec->dblk_data.buf_start);
		WRITE_VREG(HEVC_DBLK_CFGE, buf_spec->dblk_data2.buf_start);
		WRITE_VREG(HEVC_DBLK_CFGE_DBE1, buf_spec->dblk_data2.buf_start);
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			" [DBLK DBG] CFGE: 0x%x\n", buf_spec->dblk_data2.buf_start);

#ifdef LOSLESS_COMPRESS_MODE

		data32 = READ_VREG(HEVC_SAO_CTRL5);
#if 1
		data32 &= ~(1<<9);
#else
		if (params->p.bit_depth != 0x00)
			data32 &= ~(1<<9);
		else
			data32 |= (1<<9);
#endif
		WRITE_VREG(HEVC_SAO_CTRL5, data32);

		data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
#if 1
		data32 &= ~(1<<9);
#else
		if (params->p.bit_depth != 0x00)
			data32 &= ~(1<<9);
		else
			data32 |= (1<<9);
#endif
		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);

#ifdef AVS2_10B_MMU
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1,(0x1<< 4)); // bit[4] : paged_mem_mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,0x0);
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1,(0x1<< 4)); // bit[4] : paged_mem_mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1,0x0);
#else
		// WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0<<3)); // bit[3] smem mdoe
#if 1
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0<<3)); // bit[3] smem mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, (0<<3)); // bit[3] smem mode
#else
		if (params->p.bit_depth != 0x00) WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0<<3)); // bit[3] smem mode
		else WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (1<<3)); // bit[3] smem mdoe
#endif
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,(losless_comp_body_size >> 5));
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1,(losless_comp_body_size >> 5));
#endif
		//WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,(losless_comp_body_size >> 5));
		//WRITE_VREG(HEVCD_MPP_DECOMP_CTL3,(0xff<<20) | (0xff<<10) | 0xff); //8-bit mode
		WRITE_VREG(HEVC_CM_BODY_LENGTH,losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET,losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH,losless_comp_header_size);
		WRITE_VREG(HEVC_CM_BODY_LENGTH_DBE1,losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET_DBE1,losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH_DBE1,losless_comp_header_size);
#else // LOSLESS_COMPRESS_MODE
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1,0x1 << 31);
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1,0x1 << 31);
#endif
#ifdef AVS2_10B_MMU
#if 1
		WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR, buf_spec->mmu_vbh.buf_start);
		WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/4);
		WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR_DBE1, buf_spec->mmu_vbh.buf_start  + buf_spec->mmu_vbh.buf_size/2);
		WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR_DBE1, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/2 + buf_spec->mmu_vbh.buf_size/4);
#else
		WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR, buf_spec->mmu_vbh.buf_start);
		WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/2);
		WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR_DBE1, buf_spec->mmu_vbh.buf_start);
		WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR_DBE1, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/2);
#endif

	/* use HEVC_CM_HEADER_START_ADDR */
		data32 = READ_VREG(HEVC_SAO_CTRL5);
		data32 |= (1<<10);
		WRITE_VREG(HEVC_SAO_CTRL5, data32);
		data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
		data32 |= (1<<10);
		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);

#endif
#ifdef AVS2_10B_MMU_DW
		if (dec->dw_mmu_enable) {
			WRITE_VREG(HEVC_CM_BODY_LENGTH2,losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_OFFSET2,losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_LENGTH2,losless_comp_header_size_dw);
			WRITE_VREG(HEVC_CM_BODY_LENGTH2_DBE1,losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_OFFSET2_DBE1,losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_LENGTH2_DBE1,losless_comp_header_size_dw);

#if 1
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2, buf_spec->mmu_vbh_dw.buf_start);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/4);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2_DBE1, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/2);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2_DBE1, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/2 + buf_spec->mmu_vbh_dw.buf_size/4);
#else
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2, buf_spec->mmu_vbh_dw.buf_start);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/2);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2_DBE1, buf_spec->mmu_vbh_dw.buf_start);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2_DBE1, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/2);
#endif

#ifdef AVS2_10B_MMU_DW
#if 1
#ifndef FOR_S5
			WRITE_VREG(HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
			WRITE_VREG(HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
			WRITE_VREG(HEVC_DW_VH0_ADDDR_DBE1, DOUBLE_WRITE_VH0_HALF);
			WRITE_VREG(HEVC_DW_VH1_ADDDR_DBE1, DOUBLE_WRITE_VH1_HALF);
#endif
#else
			//WRITE_VREG(HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
			//WRITE_VREG(HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
			WRITE_VREG(HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
			WRITE_VREG(HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
#endif
#endif

			/* use HEVC_CM_HEADER_START_ADDR */
			data32 = READ_VREG(HEVC_SAO_CTRL5);
			data32 |= (1<<15);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
			data32 |= (1<<15);
			WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
		}
#endif
	} // back_flag
	if (front_flag) {
		WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, buf_spec->mpred_above.buf_start);
#ifdef CO_MV_COMPRESS
		data32 = READ_VREG(HEVC_MPRED_CTRL4);
		data32 |=  (1<<1);
		WRITE_VREG(HEVC_MPRED_CTRL4, data32);
#endif
	} // front_flag
}

static void hevc_init_decoder_hw(struct AVS2Decoder_s *dec,
	uint8_t front_flag, uint8_t back_flag)
{
	uint32_t data32;
	int32_t i;

int32_t g_WqMDefault4x4[16] = {
	64,     64,     64,     68,
	64,     64,     68,     72,
	64,     68,     76,     80,
	72,     76,     84,     96
};

int32_t g_WqMDefault8x8[64] = {
	64,     64,     64,     64,     68,     68,     72,     76,
	64,     64,     64,     68,     72,     76,     84,     92,
	64,     64,     68,     72,     76,     80,     88,     100,
	64,     68,     72,     80,     84,     92,     100,    112,
	68,     72,     80,     84,     92,     104,    112,    128,
	76,     80,     84,     92,     104,    116,    132,    152,
	96,     100,    104,    116,    124,    140,    164,    188,
	104,    108,    116,    128,    152,    172,    192,    216
};

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[test.c] Entering hevc_init_decoder_hw\n");

#if 0 //def AVS2_10B_HED_FB
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[test.c] Init AVS2_10B_HED_FB\n");
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
	data32 = data32 |
		(1 << 3) | // fb_read_avs2_enable0
		(1 << 6) | // fb_read_avs2_enable1
		(1 << 1) | // fb_avs2_enable
		(0 << 13) | // fb_read_avs3_enable0
		(0 << 14) | // fb_read_avs3_enable1
		(0 << 9) | // fb_avs3_enable
		(3 << 7)  //core0_en, core1_en,hed_fb_en
		;
	WRITE_VREG(HEVC_ASSIST_FB_CTL, data32); // new dual
#endif

#if 1 //move to ucode
	if (!efficiency_mode && front_flag) {
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"[test.c] Enable HEVC Parser Interrupt\n");
		data32 = READ_VREG(HEVC_PARSER_INT_CONTROL);
		data32 = data32 |
			(1 << 24) |  // stream_buffer_empty_int_amrisc_enable
			(1 << 22) |  // stream_fifo_empty_int_amrisc_enable
			(1 << 7) |  // dec_done_int_cpu_enable
			(1 << 4) |  // startcode_found_int_cpu_enable
			(0 << 3) |  // startcode_found_int_amrisc_enable
			(1 << 0)    // parser_int_enable
			;
		WRITE_VREG(HEVC_PARSER_INT_CONTROL, data32);

		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[test.c] Enable HEVC Parser Shift\n");

		data32 = READ_VREG(HEVC_SHIFT_STATUS);
#ifdef AVS2
		data32 = data32 |
			(0 << 1) |  // emulation_check_on // AVS2 emulation on/off will be controlled in microcode according to startcode type
			(1 << 0)    // startcode_check_on
			;
#else
		data32 = data32 |
			(1 << 1) |  // emulation_check_on
			(1 << 0)    // startcode_check_on
			;
#endif
		WRITE_VREG(HEVC_SHIFT_STATUS, data32);

		WRITE_VREG(HEVC_SHIFT_CONTROL,
			(6 << 20) |  // emu_push_bits  (6-bits for AVS2)
			(0 << 19) |  // emu_3_enable // maybe turned on in microcode
			(0 << 18) |  // emu_2_enable // maybe turned on in microcode
			(0 << 17) |  // emu_1_enable // maybe turned on in microcode
			(0 << 16) |  // emu_0_enable // maybe turned on in microcode
			(0 << 14) | // disable_start_code_protect
			(3 << 6) | // sft_valid_wr_position
			(2 << 4) | // emulate_code_length_sub_1
			(2 << 1) | // start_code_length_sub_1
			(1 << 0)   // stream_shift_enable
			);

		WRITE_VREG(HEVC_SHIFT_LENGTH_PROTECT,
			(0 << 30) |   // data_protect_fill_00_enable
			(1 << 29)     // data_protect_fill_ff_enable
			);

		WRITE_VREG(HEVC_CABAC_CONTROL,
			(1 << 0)   // cabac_enable
			);

		WRITE_VREG(HEVC_PARSER_CORE_CONTROL,
			(1 << 0)   // hevc_parser_core_clk_en
			);

		WRITE_VREG(HEVC_DEC_STATUS_REG, 0);
	}
#endif

	if (back_flag) {
#if 0 // Dual Core : back Microcode will always initial SCALELUT
		// Initial IQIT_SCALELUT memory -- just to avoid X in simulation
		printk("[test.c] Initial IQIT_SCALELUT memory -- just to avoid X in simulation...\n");
		WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0); // cfg_p_addr
		for (i=0; i<1024; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, 0);
		WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR_DBE1, 0); // cfg_p_addr
		for (i=0; i<1024; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA_DBE1, 0);
#endif
		// Zero out canvas registers in IPP -- avoid simulation X
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);
		for (i = 0; i < 32; i++) {
			WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#ifdef DUAL_CORE_64
			WRITE_VREG(HEVC2_HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#endif
		}
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR_DBE1, (0 << 8) | (0<<1) | 1);
		for (i = 0; i < 32; i++) {
			WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR_DBE1, 0);
#ifdef DUAL_CORE_64
			WRITE_VREG(HEVC2_HEVCD_MPP_ANC_CANVAS_DATA_ADDR_DBE1, 0);
#endif
		}
	}

	if (!efficiency_mode && front_flag) {
#ifdef ENABLE_SWAP_TEST
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 100);
#else
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 0);
#endif

#if 0
		// Send parser_cmd
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"[test.c] SEND Parser Command ...\n");
		WRITE_VREG(HEVC_PARSER_CMD_WRITE, (1<<16) | (0<<0));
		for (i = 0; i < PARSER_CMD_NUMBER; i++) {
			WRITE_VREG(HEVC_PARSER_CMD_WRITE, parser_cmd[i]);
		}

		WRITE_VREG(HEVC_PARSER_CMD_SKIP_0, PARSER_CMD_SKIP_CFG_0);
		WRITE_VREG(HEVC_PARSER_CMD_SKIP_1, PARSER_CMD_SKIP_CFG_1);
		WRITE_VREG(HEVC_PARSER_CMD_SKIP_2, PARSER_CMD_SKIP_CFG_2);
#endif
#if 1 //move to ucode

		WRITE_VREG(HEVC_PARSER_IF_CONTROL,
			(1 << 9) | // parser_alf_if_en
			//  (1 << 8) | // sao_sw_pred_enable
			(1 << 5) | // parser_sao_if_en
			(1 << 2) | // parser_mpred_if_en
			(1 << 0) // parser_scaler_if_en
			);
#endif
	}

	if (back_flag) {
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[test.c] Config AVS2 default seq_wq_matrix ...\n");
	// 4x4
	WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 64); // default seq_wq_matrix_4x4 begin address
	for (i = 0; i < 16; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, g_WqMDefault4x4[i]);

	// 8x8
	WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0); // default seq_wq_matrix_8x8 begin address
	for (i = 0; i < 64; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, g_WqMDefault8x8[i]);

	// 4x4
	WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR_DBE1, 64); // default seq_wq_matrix_4x4 begin address
	for (i = 0; i < 16; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA_DBE1, g_WqMDefault4x4[i]);

	// 8x8
	WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR_DBE1, 0); // default seq_wq_matrix_8x8 begin address
	for (i = 0; i < 64; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA_DBE1, g_WqMDefault8x8[i]);

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[test.c] Reset IPP\n");
		WRITE_VREG(HEVCD_IPP_TOP_CNTL,
		(0 << 1) | // enable ipp
		(4 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
		(1 << 0)   // software reset ipp and mpp
		);
	WRITE_VREG(HEVCD_IPP_TOP_CNTL,
		(1 << 1) | // enable ipp
		(4 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
		(0 << 0)   // software reset ipp and mpp
		);
	WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
		(0 << 1) | // enable ipp
		(4 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
		(1 << 0)   // software reset ipp and mpp
		);
	WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
		(1 << 1) | // enable ipp
		(4 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
		(0 << 0)   // software reset ipp and mpp
		);

	// Init dblk
#define LPF_LINEBUF_MODE_CTU_BASED
	//must be defined for DUAL_CORE

	data32 = READ_VREG(HEVC_DBLK_CFGB);
	data32 |= (2 << 0);
	WRITE_VREG(HEVC_DBLK_CFGB, data32); // [3:0] cfg_video_type -> AVS2
#ifdef LPF_LINEBUF_MODE_CTU_BASED
	WRITE_VREG(HEVC_DBLK_CFG0, (0<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#else
	WRITE_VREG(HEVC_DBLK_CFG0, (1<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#endif

	data32 = READ_VREG(HEVC_DBLK_CFGB_DBE1);
	data32 |= (2 << 0);
	WRITE_VREG(HEVC_DBLK_CFGB_DBE1, data32); // [3:0] cfg_video_type -> AVS2
#ifdef LPF_LINEBUF_MODE_CTU_BASED
	WRITE_VREG(HEVC_DBLK_CFG0_DBE1, (0<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#else
	WRITE_VREG(HEVC_DBLK_CFG0_DBE1, (1<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#endif

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[test.c] Bitstream level Init for DBLK .Done.\n");

	// Initialize mcrcc and decomp perf counters
#if 0
	mcrcc_perfcount_reset();
	decomp_perfcount_reset();
#endif
	}//back_flag end
	if (back_flag) {
#if 0
		//to do: move to config_sao_hw_fb
		data32 = READ_VREG(HEVC_SAO_CTRL1);
		data32 &= (~(3 << 14));
		data32 |= (2 << 14);	/* line align with 64*/
		data32 &= (~0x3000);
		data32 |= (MEM_MAP_MODE << 12); /* [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32 */
		data32 &= (~0xff0);
#ifdef AVS2_10B_MMU_DW
		if (dec->dw_mmu_enable == 0)
		data32 |= ((dec->endian >> 8) & 0xfff);
#else
		data32 |= ((dec->endian >> 8) & 0xfff); /* data32 |= 0x670; Big-Endian per 64-bit */
#endif
		data32 &= (~0x3); /*[1]:dw_disable [0]:cm_disable*/

		if (get_double_write_mode(dec) == 0)
			data32 |= 0x2; /*disable double write*/
		else if (get_double_write_mode(dec) & 0x10)
			data32 |= 0x1; /*disable cm*/

		/*
		*  [31:24] ar_fifo1_axi_thred
		*  [23:16] ar_fifo0_axi_thred
		*  [15:14] axi_linealign, 0-16bytes, 1-32bytes, 2-64bytes
		*  [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
		*  [11:08] axi_lendian_C
		*  [07:04] axi_lendian_Y
		*  [3]	   reserved
		*  [2]	   clk_forceon
		*  [1]	   dw_disable:disable double write output
		*  [0]	   cm_disable:disable compress output
		*/
		WRITE_VREG(HEVC_SAO_CTRL1, data32);
		WRITE_VREG(HEVC_SAO_CTRL1_DBE1, data32);

		if (get_double_write_mode(dec) & 0x10) {
			/* [23:22] dw_v1_ctrl
			[21:20] dw_v0_ctrl
			[19:18] dw_h1_ctrl
			[17:16] dw_h0_ctrl
			*/
			data32 = READ_VREG(HEVC_SAO_CTRL5);
			/*set them all 0 for H265_NV21 (no down-scale)*/
			data32 &= ~(0xff << 16);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
		} else {
		if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_T7) {
			WRITE_VREG(HEVC_SAO_CTRL26, 0);
			WRITE_VREG(HEVC_SAO_CTRL26_DBE1, 0);
		}

		data32 = READ_VREG(HEVC_SAO_CTRL5);
		data32 &= (~(0xff << 16));
		if ((get_double_write_mode(dec) & 0xf) == 8 ||
			(get_double_write_mode(dec) & 0xf) == 9) {
			data32 |= (0xff<<16);
			WRITE_VREG(HEVC_SAO_CTRL26, 0xf);
			WRITE_VREG(HEVC_SAO_CTRL26_DBE1, 0xf);
		} else if ((get_double_write_mode(dec) & 0xf) == 2 ||
			(get_double_write_mode(dec) & 0xf) == 3)
			data32 |= (0xff<<16);
		else if ((get_double_write_mode(dec) & 0xf) == 4)
			data32 |= (0x33<<16);
		WRITE_VREG(HEVC_SAO_CTRL5, data32);
		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
		}
#endif
#ifdef AVS2_10B_MMU
		data32 = READ_VREG(HEVC_SAO_CTRL9);
		data32 |= 0x1;
		WRITE_VREG(HEVC_SAO_CTRL9, data32);
		WRITE_VREG(HEVC_SAO_CTRL9_DBE1, data32);
#endif
#ifdef AVS2_10B_MMU_DW
		if (dec->dw_mmu_enable) {
			data32 = READ_VREG(HEVC_SAO_CTRL9);
			data32 |= (1 << 10);
			WRITE_VREG(HEVC_SAO_CTRL9, data32);
			WRITE_VREG(HEVC_SAO_CTRL9_DBE1, data32);
		}
#endif
	}

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[test.c] Leaving hevc_init_decoder_hw\n");
	return;
}

extern void config_cuva_buf(struct AVS2Decoder_s *dec);
static int32_t avs2_hw_init(struct AVS2Decoder_s *dec, uint8_t front_flag, uint8_t back_flag)
{
	uint32_t data32;
	uint32_t tmp = 0;

	avs2_print(dec, AVS2_DBG_BUFMGR,
	"%s front_flag %d back_flag %d\n", __func__, front_flag, back_flag);
	if (dec->front_back_mode != 1) {
		if (front_flag)
			avs2_hw_ctx_restore(dec);
		if (back_flag) {
		/* clear mailbox interrupt */
		WRITE_VREG(dec->backend_ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(dec->backend_ASSIST_MBOX0_MASK, 1);
		}
		return 0;
	}

	if (front_flag) {
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 | (3 << 7) | (1 << 1);
		tmp = (1 << 0) | (1 << 9) | (1 << 10);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32); // new dual
	}

	if (back_flag) {
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 | (3 << 7) | (1 << 3) | (1 << 6);
		tmp = (1 << 2) | (1 << 13) | (1 << 11) | (1 << 5) | (1 << 14) | (1 << 12);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32); // new dual
	}

	config_work_space_hw(dec, front_flag, back_flag);

	if (!efficiency_mode && front_flag && dec->pic_list_init_flag)
		init_pic_list_hw_fb(dec);

	hevc_init_decoder_hw(dec, front_flag, back_flag);

	//Start JT
#if 1 //move to ucode
	if (!efficiency_mode && front_flag) {
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"[test.c] Enable BitStream Fetch\n");
#if 0
		data32 = READ_VREG(HEVC_STREAM_CONTROL);
		data32 = data32 |
			(1 << 0) // stream_fetch_enable
			;
		WRITE_VREG(HEVC_STREAM_CONTROL, data32);

		data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
		if (data32 != 0x00000100) { print_scratch_error(29); return -1; }
		/*data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
		if (data32 != 0x00000300) { print_scratch_error(30); return; }*/
		WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x12345678);
		WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x9abcdef0);
		data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
		if (data32 != 0x12345678) { print_scratch_error(31); return -1; }
		data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
		if (data32 != 0x9abcdef0) { print_scratch_error(32); return -1; }
#endif
		WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x00000100);
		WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x00000000); // 0x000000 - 0x000003 emulate code for AVS2
	}
#endif
	// End JT

	if (back_flag) {
		// Set MCR fetch priorities
		data32 = 0x1 | (0x1 << 2) | (0x1 <<3) | (24 << 4) | (32 << 11) | (24 << 18) | (32 << 25);
		WRITE_VREG(HEVCD_MPP_DECOMP_AXIURG_CTL, data32);
		WRITE_VREG(HEVCD_MPP_DECOMP_AXIURG_CTL_DBE1, data32);

		// Set IPP MULTICORE CFG
		WRITE_VREG(HEVCD_IPP_MULTICORE_CFG, 1);
		WRITE_VREG(HEVCD_IPP_MULTICORE_CFG_DBE1, 1);

#ifdef DYN_CACHE
		if (dec->front_back_mode == 1) {
			WRITE_VREG(HEVCD_IPP_DYN_CACHE,0x2b);//enable new mcrcc
			WRITE_VREG(HEVCD_IPP_DYN_CACHE_DBE1,0x2b);//enable new mcrcc
			avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"HEVC DYN MCRCC\n");
		}
#endif
	}

	if (front_flag) {
		unsigned int decode_mode;

		WRITE_VREG(LMEM_DUMP_ADR, (u32)dec->lmem_phy_addr);

#if 1 // JT

#if 0 //def SIMULATION
		if (decode_pic_begin == 0)
			WRITE_VREG(HEVC_WAIT_FLAG, 1);
		else
			WRITE_VREG(HEVC_WAIT_FLAG, 0);
#else
		WRITE_VREG(HEVC_WAIT_FLAG, 1);
#endif

		/* disable PSCALE for hardware sharing */
#ifdef DOS_PROJECT
#else
#ifndef FOR_S5
		WRITE_VREG(HEVC_PSCALE_CTRL, 0);
#endif
#endif
		/* clear mailbox interrupt */
		WRITE_VREG(dec->ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(dec->ASSIST_MBOX0_MASK, 1);

		//WRITE_VREG(DEBUG_REG1, 0x0);  //no debug
		WRITE_VREG(NAL_SEARCH_CTL, 0x8); //check SEQUENCE/I_PICTURE_START in ucode
		WRITE_VREG(DECODE_STOP_POS, udebug_flag);
#ifdef MULTI_INSTANCE_SUPPORT
		if (!dec->m_ins_flag)
			decode_mode = DECODE_MODE_SINGLE;
		else if (vdec_frame_based(hw_to_vdec(dec)))
			decode_mode = DECODE_MODE_MULTI_FRAMEBASE;
		else
			decode_mode = DECODE_MODE_MULTI_STREAMBASE;
		if (dec->avs2_dec.bufmgr_error_flag &&
			(error_handle_policy & 0x1)) {
			dec->bufmgr_error_count++;
			dec->avs2_dec.bufmgr_error_flag = 0;
			if (dec->bufmgr_error_count > (re_search_seq_threshold & 0xff)
				&& dec->frame_count > ((re_search_seq_threshold >> 8) & 0xff)) {
				struct avs2_decoder *avs2_dec = &dec->avs2_dec;
				dec->start_decoding_flag = 0;
				avs2_dec->hd.vec_flag = 1;
				dec->skip_PB_before_I = 1;
				avs2_print(dec, 0,
				"!!Bufmgr error, search seq again (0x%x %d %d)\n",
				error_handle_policy,
				dec->frame_count,
				dec->bufmgr_error_count);
				dec->bufmgr_error_count = 0;
			}
		}
		decode_mode |= (dec->start_decoding_flag << 16);

		WRITE_VREG(DECODE_MODE, decode_mode);
		WRITE_VREG(HEVC_DECODE_SIZE, 0xffffffff);
		avs2_print(dec, AVS2_DBG_BUFMGR,
			"decode_mode 0x%x\n", decode_mode);
		config_cuva_buf(dec);
#endif
		avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
			"[test.c] P_HEVC_MPSR!\n");
#endif
	}
	if (back_flag) {
		/* clear mailbox interrupt */
		WRITE_VREG(dec->backend_ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(dec->backend_ASSIST_MBOX0_MASK, 1);
	}

	return 0;
}

void avs2_store_pbi_fb(struct avs2_decoder *dec, struct avs2_decoder_fb *dec_fb, u32 res_ch_flag)
{
	int i = 0;
	if (res_ch_flag) {
		dec_fb->wait_working_buf = dec->wait_working_buf;
		dec_fb->fb_wr_pos = dec->fb_wr_pos;
		dec_fb->fb_rd_pos = dec->fb_rd_pos;
		dec_fb->frontend_decoded_count = dec->frontend_decoded_count;
		dec_fb->backend_decoded_count = dec->backend_decoded_count;
		dec_fb->ins_offset = dec->ins_offset;
		for (i = 0; i < 256 * 4; i++)
			dec_fb->instruction[i] = dec->instruction[i];
		for (i = 0; i < MAX_FB_IFBUF_NUM; i++) {
			dec_fb->next_bk[i] = dec->next_bk[i];
			dec_fb->next_be_decode_pic[i] = dec->next_be_decode_pic[i];
		}
	}
	dec_fb->front_pause_flag = dec->front_pause_flag;
	dec_fb->fb_buf_mmu0 = dec->fb_buf_mmu0;
	dec_fb->fb_buf_mmu1 = dec->fb_buf_mmu1;
	dec_fb->fb_buf_mpred_imp0 = dec->fb_buf_mpred_imp0;
	dec_fb->fb_buf_mpred_imp1 = dec->fb_buf_mpred_imp1;
	dec_fb->fb_buf_scalelut = dec->fb_buf_scalelut;
	dec_fb->fb_buf_vcpu_imem = dec->fb_buf_vcpu_imem;
	dec_fb->fb_buf_sys_imem = dec->fb_buf_sys_imem;
	dec_fb->fb_buf_lmem0 = dec->fb_buf_lmem0;
	dec_fb->fb_buf_lmem1 = dec->fb_buf_lmem1;
	dec_fb->fb_buf_parser_sao0 = dec->fb_buf_parser_sao0;
	dec_fb->fb_buf_parser_sao1 = dec->fb_buf_parser_sao1;
	dec_fb->fr = dec->fr;
	dec_fb->bk = dec->bk;
	dec_fb->fb_buf_sys_imem_addr = dec->fb_buf_sys_imem_addr;
	dec_fb->sys_imem_ptr_v = dec->sys_imem_ptr_v;
}

void avs2_restore_pbi_fb(struct avs2_decoder *dec, struct avs2_decoder_fb *dec_fb, u32 res_ch_flag)
{
	int i = 0;
	if (res_ch_flag) {
		dec->wait_working_buf = dec_fb->wait_working_buf;
		dec->fb_wr_pos = dec_fb->fb_wr_pos;
		dec->fb_rd_pos = dec_fb->fb_rd_pos;
		dec->frontend_decoded_count = dec_fb->frontend_decoded_count;
		dec->backend_decoded_count = dec_fb->backend_decoded_count;
		dec->ins_offset = dec_fb->ins_offset;
		for (i = 0; i < 256 * 4; i++)
			dec->instruction[i] = dec_fb->instruction[i];
		for (i = 0; i < MAX_FB_IFBUF_NUM; i++) {
			dec->next_bk[i] = dec_fb->next_bk[i];
			dec->next_be_decode_pic[i] = dec_fb->next_be_decode_pic[i];
		}
	}
	dec->front_pause_flag = dec_fb->front_pause_flag;
	dec->fb_buf_mmu0 = dec_fb->fb_buf_mmu0;
	dec->fb_buf_mmu1 = dec_fb->fb_buf_mmu1;
	dec->fb_buf_mpred_imp0 = dec_fb->fb_buf_mpred_imp0;
	dec->fb_buf_mpred_imp1 = dec_fb->fb_buf_mpred_imp1;
	dec->fb_buf_scalelut = dec_fb->fb_buf_scalelut;
	dec->fb_buf_vcpu_imem = dec_fb->fb_buf_vcpu_imem;
	dec->fb_buf_sys_imem = dec_fb->fb_buf_sys_imem;
	dec->fb_buf_lmem0 = dec_fb->fb_buf_lmem0;
	dec->fb_buf_lmem1 = dec_fb->fb_buf_lmem1;
	dec->fb_buf_parser_sao0 = dec_fb->fb_buf_parser_sao0;
	dec->fb_buf_parser_sao1 = dec_fb->fb_buf_parser_sao1;
	dec->fr = dec_fb->fr;
	dec->bk = dec_fb->bk;
	dec->fb_buf_sys_imem_addr = dec_fb->fb_buf_sys_imem_addr;
	dec->sys_imem_ptr_v = dec_fb->sys_imem_ptr_v;
}

static void release_free_mmu_buffers(struct AVS2Decoder_s *dec)
{
	int ii;

	if (!(dec->error_proc_policy & 0x2))
		return ;

	for (ii = 0; ii < dec->avs2_dec.ref_maxbuffer; ii++) {
		struct avs2_frame_s *pic = dec->avs2_dec.fref[ii];
		if (pic->bg_flag == 0 &&
			pic->is_output == -1 &&
			pic->index != INVALID_IDX &&
#ifdef NEW_FRONT_BACK_CODE
			pic->backend_ref == 0 &&
#endif
			pic->vf_ref == 0) {
			if (pic->referred_by_others == 0) {
#if 1
				struct aml_buf *aml_buf = index_to_aml_buf(dec, pic->index);
#ifdef AVS2_10B_MMU
				if (dec->front_back_mode)
					decoder_mmu_box_free_idx(aml_buf->fbc->mmu_1,
					aml_buf->fbc->index);
				dec->cur_fb_idx_mmu = INVALID_IDX;

				decoder_mmu_box_free_idx(aml_buf->fbc->mmu,
				aml_buf->fbc->index);
#endif
#ifdef AVS2_10B_MMU_DW
				if (dec->dw_mmu_enable && dec->dw_mmu_box) {
					decoder_mmu_box_free_idx(dec->dw_mmu_box, pic->index);
				if (dec->front_back_mode && dec->dw_mmu_box_1)
					decoder_mmu_box_free_idx(dec->dw_mmu_box_1,
					pic->index);
				}

#endif
#endif
#ifndef MV_USE_FIXED_BUF
				decoder_bmmu_box_free_idx(
				dec->bmmu_box,
				MV_BUFFER_IDX(pic->index));
				pic->mpred_mv_wr_start_addr = 0;
#endif
			}
		}
	}
}

static int BackEnd_StartDecoding(struct AVS2Decoder_s *dec)
{
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	struct avs2_frame_s *pic = avs2_dec->next_be_decode_pic[avs2_dec->fb_rd_pos];
	int i = 0;

	avs2_print(dec, PRINT_FLAG_VDEC_STATUS,
		"Start BackEnd Decoding %d (wr pos %d, rd pos %d) poc %d\n",
		avs2_dec->backend_decoded_count, avs2_dec->fb_wr_pos, avs2_dec->fb_rd_pos, pic->poc);
	if (dec->front_back_mode != 1 &&
		dec->front_back_mode != 3) {
		copy_loopbufs_ptr(&avs2_dec->bk, &avs2_dec->next_bk[avs2_dec->fb_rd_pos]);
		print_loopbufs_ptr(dec, "bk", &avs2_dec->bk);
		avs2_hw_init(dec, 0, 1);
		WRITE_VREG(dec->backend_ASSIST_MBOX0_IRQ_REG, 1);
		return 0;
	}
#if 0
#ifdef AVS2_10B_MMU
	alloc_mmu(&avs2_mmumgr_0, pic->index, pic->width, pic->height/2+32+8, ((pic->depth == 0) ? 0 : DEPTH_BITS_10));
	alloc_mmu(&avs2_mmumgr_1, pic->index, pic->width, pic->height/2+32+8, ((pic->depth == 0) ? 0 : DEPTH_BITS_10));
#ifdef AVS2_10B_MMU_DW
	alloc_mmu(&avs2_mmumgr_dw0, pic->index, pic->width, pic->height/2+32+8, ((pic->depth == 0) ? 0 : DEPTH_BITS_10));
	alloc_mmu(&avs2_mmumgr_dw1, pic->index, pic->width, pic->height/2+32+8, ((pic->depth == 0) ? 0 : DEPTH_BITS_10));
#endif
	pic->mmu_alloc_flag = 1;
#endif
#else

	mutex_lock(&dec->fb_mutex);
	for (i = 0; (i < MAXREF) && (pic->error_mark == 0); i++) {
		if ((pic->ref_pic[i]) && (pic->ref_pic[i]->error_mark)) {
			avs2_print(dec, AVS2_DBG_BUFMGR,
				"%s ref pic(%d) has error_mark, skip\n", __func__, i);
			dec->gvs->error_frame_count++;
			if (pic->slice_type == I_IMG) {
				dec->gvs->i_concealed_frames++;
			} else if ((pic->slice_type == P_IMG) ||
				(pic->slice_type == F_IMG)) {
				dec->gvs->p_concealed_frames++;
			} else if (pic->slice_type == B_IMG) {
				dec->gvs->b_concealed_frames++;
			}
			pic->error_mark = 1;
			break;
		}
	}
	mutex_unlock(&dec->fb_mutex);

	if ((dec->error_proc_policy & 0x2) && pic->error_mark) {
		mutex_lock(&dec->fb_mutex);
		dec->gvs->drop_frame_count++;
		if (pic->slice_type == I_IMG) {
			dec->gvs->i_lost_frames++;
		} else if ((pic->slice_type == P_IMG) ||
			(pic->slice_type == F_IMG)) {
			dec->gvs->p_lost_frames++;
		} else if (pic->slice_type == B_IMG) {
			dec->gvs->b_lost_frames++;
		}
		mutex_unlock(&dec->fb_mutex);

		avs2_print(dec, AVS2_DBG_BUFMGR, "%s pic has error_mark, skip\n", __func__);
		pic_backend_ref_operation(dec, 0);
		return 1;
	}

	if (dec->front_back_mode == 1) {
		struct aml_buf *aml_buf = index_to_aml_buf(dec, pic->index);

#ifdef AVS2_10B_MMU
		decoder_mmu_box_alloc_idx(
			aml_buf->fbc->mmu,
			aml_buf->fbc->index,
			aml_buf->fbc->frame_size,
			dec->frame_mmu_map_addr);

		decoder_mmu_box_alloc_idx(
			aml_buf->fbc->mmu_1,
			aml_buf->fbc->index,
			aml_buf->fbc->frame_size,
			dec->frame_mmu_map_addr_1);

#ifdef AVS2_10B_MMU_DW
		decoder_mmu_box_alloc_idx(
			dec->dw_mmu_box,
			pic->index,
			aml_buf->fbc->frame_size,
			dec->dw_frame_mmu_map_addr);
		decoder_mmu_box_alloc_idx(
			dec->dw_mmu_box_1,
			pic->index,
			aml_buf->fbc->frame_size,
			dec->dw_frame_mmu_map_addr_1);
#endif
#endif

		pic->mmu_alloc_flag = 1;
	}
#endif
	copy_loopbufs_ptr(&avs2_dec->bk, &avs2_dec->next_bk[avs2_dec->fb_rd_pos]);
	print_loopbufs_ptr(dec, "bk", &avs2_dec->bk);

	if (dec->front_back_mode == 1)
		amhevc_reset_b();
	avs2_hw_init(dec, 0, 1);
	if (dec->front_back_mode == 3) {
		WRITE_VREG(dec->backend_ASSIST_MBOX0_IRQ_REG, 1);
	} else {
		config_bufstate_back_hw(avs2_dec);
		WRITE_VREG(PIC_DECODE_COUNT_DBE, avs2_dec->backend_decoded_count);
		WRITE_VREG(HEVC_DEC_STATUS_DBE, HEVC_BE_DECODE_DATA);
		WRITE_VREG(HEVC_SAO_CRC, 0);
		amhevc_start_b();
		vdec_profile(hw_to_vdec(dec), VDEC_PROFILE_DECODER_START, CORE_MASK_HEVC_BACK);
	}

	return 0;
}

static void init_pic_list_hw_fb(struct AVS2Decoder_s *dec)
{
	int i;
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	struct avs2_frame_s *pic;

	dec->avs2_dec.ins_offset = 0;
	/*WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x0);*/
#if 0
	WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
	(0x1 << 1) | (0x1 << 2));

#ifdef DUAL_CORE_64
	WRITE_VREG(HEVC2_HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
	(0x1 << 1) | (0x1 << 2));
#endif
#endif
	WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, (0x1 << 1) | (0x1 << 2));

	for (i = 0; i < dec->used_buf_num; i++) {
#if 0

		if (i == (dec->used_buf_num - 1))
			pic = avs2_dec->m_bg;
		else
			pic = avs2_dec->fref[i];
#else
		pic = avs2_dec->init_fref[i];
#endif
		if (pic->index < 0)
			break;
#ifdef AVS2_10B_MMU
#ifdef DUAL_CORE_64
		if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_GXLX2)
			WRITE_BACK_16(avs2_dec, HEVC2_MPP_ANC2AXI_TBL_CONF_ADDR,
				(0x1 << 1) | (pic->index << 8));
		else
			WRITE_BACK_16(avs2_dec, HEVC2_HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
				(0x1 << 1) | (pic->index << 8));
#endif
		WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC2AXI_TBL_DATA, pic->header_adr >> 5);
#else
		WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC2AXI_TBL_DATA, pic->mc_y_adr >> 5);
#endif
#ifndef LOSLESS_COMPRESS_MODE
		WRITE_BACK_32(avs2_dec, HEVCD_MPP_ANC2AXI_TBL_DATA, pic->mc_u_v_adr >> 5);
#endif
#ifdef DUAL_CORE_64
#ifdef AVS2_10B_MMU
		WRITE_BACK_32(avs2_dec, HEVC2_HEVCD_MPP_ANC2AXI_TBL_DATA,
			pic->header_adr >> 5);
#else
		WRITE_BACK_32(avs2_dec, HEVC2_HEVCD_MPP_ANC2AXI_TBL_DATA,
			pic->mc_y_adr >> 5);
#endif
#ifndef LOSLESS_COMPRESS_MODE
		WRITE_BACK_32(avs2_dec, HEVC2_HEVCD_MPP_ANC2AXI_TBL_DATA,
			pic->mc_u_v_adr >> 5);
#endif
	/*DUAL_CORE_64*/
#endif
	}
	WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x1);
#ifdef DUAL_CORE_64
	WRITE_BACK_8(avs2_dec, HEVC2_HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x1);
#endif
#if 0
	/*Zero out canvas registers in IPP -- avoid simulation X*/
	WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR,
	(0 << 8) | (0 << 1) | 1);
	for (i = 0; i < 32; i++) {
		WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#ifdef DUAL_CORE_64
		WRITE_BACK_8(avs2_dec, HEVC2_HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#endif
	}
#endif
}

static void config_mpred_hw_fb(struct AVS2Decoder_s *dec)
{
	uint32_t data32;
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	struct avs2_frame_s *cur_pic = avs2_dec->hc.cur_pic;
	struct avs2_frame_s *col_pic = avs2_dec->fref[0];

	int32_t mpred_mv_rd_start_addr ;
	//int32_t mpred_curr_lcu_x;
	//int32_t mpred_curr_lcu_y;
	int32_t mpred_mv_rd_end_addr;
	int32_t MV_MEM_UNIT_l;

	int32_t above_en;
	int32_t mv_wr_en;
	int32_t mv_rd_en;
	int32_t col_isIntra;

	if (avs2_dec->img.type  != I_IMG) {
		above_en = 1;
		mv_wr_en = 1;
		mv_rd_en = 1;
		col_isIntra = 0;
	} else {
		above_en = 1;
		mv_wr_en = 1;
		mv_rd_en = 0;
		col_isIntra = 0;
	}

	mpred_mv_rd_start_addr = col_pic->mpred_mv_wr_start_addr;
	//data32 = READ_VREG(HEVC_MPRED_CURR_LCU);
	//mpred_curr_lcu_x = data32 & 0xffff;
	//mpred_curr_lcu_y = (data32 >> 16) & 0xffff;

	MV_MEM_UNIT_l=avs2_dec->lcu_size_log2 == 6 ? 0x200 : avs2_dec->lcu_size_log2 == 5 ? 0x80 : 0x20;

	mpred_mv_rd_end_addr = mpred_mv_rd_start_addr + ((avs2_dec->lcu_x_num * avs2_dec->lcu_y_num) * MV_MEM_UNIT_l);

	//mpred_above_buf_start = buf_spec->mpred_above.buf_start;

	avs2_print(dec, AVS2_DBG_BUFMGR_MORE,
		"cur pic index %d  col pic index %d\n", cur_pic->index, col_pic->index);

	WRITE_VREG(HEVC_MPRED_MV_WR_START_ADDR, cur_pic->mpred_mv_wr_start_addr);
	WRITE_VREG(HEVC_MPRED_MV_RD_START_ADDR, col_pic->mpred_mv_wr_start_addr);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[MPRED CO_MV] write 0x%x  read 0x%x\n", cur_pic->mpred_mv_wr_start_addr, col_pic->mpred_mv_wr_start_addr);

	data32 =
		((avs2_dec->bk_img_is_top_field) << 13) |
		((avs2_dec->hd.background_picture_enable & 1) << 12) |
		((avs2_dec->hd.curr_RPS.num_of_ref & 7) << 8) |
		((avs2_dec->hd.b_pmvr_enabled & 1) << 6) |
		((avs2_dec->img.is_top_field & 1) << 5)  |
		((avs2_dec->img.is_field_sequence & 1) << 4) |
		((avs2_dec->img.typeb & 7) << 1) |
		(avs2_dec->hd.background_reference_enable & 0x1);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"HEVC_MPRED_CTRL9 <= 0x%x(num of ref %d)\n", data32, avs2_dec->hd.curr_RPS.num_of_ref);
		WRITE_VREG(HEVC_MPRED_CTRL9, data32);

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"%s: dis %d %d %d %d %d %d %d fref0_ref_poc %d %d %d %d %d %d %d\n",
		__func__, avs2_dec->fref[0]->imgtr_fwRefDistance,
		avs2_dec->fref[1]->imgtr_fwRefDistance,
		avs2_dec->fref[2]->imgtr_fwRefDistance,
		avs2_dec->fref[3]->imgtr_fwRefDistance,
		avs2_dec->fref[4]->imgtr_fwRefDistance,
		avs2_dec->fref[5]->imgtr_fwRefDistance,
		avs2_dec->fref[6]->imgtr_fwRefDistance,
		avs2_dec->fref[0]->ref_poc[0],
		avs2_dec->fref[0]->ref_poc[1],
		avs2_dec->fref[0]->ref_poc[2],
		avs2_dec->fref[0]->ref_poc[3],
		avs2_dec->fref[0]->ref_poc[4],
		avs2_dec->fref[0]->ref_poc[5],
		avs2_dec->fref[0]->ref_poc[6]
		);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"pic_distance %d, imgtr_next_P %d\n",
		avs2_dec->img.pic_distance, avs2_dec->img.imgtr_next_P);
	WRITE_VREG(HEVC_MPRED_CUR_POC, avs2_dec->img.pic_distance);
	WRITE_VREG(HEVC_MPRED_COL_POC, avs2_dec->img.imgtr_next_P);

	//below MPRED Ref_POC_xx_Lx registers must follow Ref_POC_xx_L0 -> Ref_POC_xx_L1 in pair write order!!!
	WRITE_VREG(HEVC_MPRED_L0_REF00_POC, avs2_dec->fref[0]->imgtr_fwRefDistance);
	WRITE_VREG(HEVC_MPRED_L1_REF00_POC, avs2_dec->fref[0]->ref_poc[0]);

	WRITE_VREG(HEVC_MPRED_L0_REF01_POC,avs2_dec->fref[1]->imgtr_fwRefDistance);
	WRITE_VREG(HEVC_MPRED_L1_REF01_POC,avs2_dec->fref[0]->ref_poc[1]);

	WRITE_VREG(HEVC_MPRED_L0_REF02_POC,avs2_dec->fref[2]->imgtr_fwRefDistance);
	WRITE_VREG(HEVC_MPRED_L1_REF02_POC,avs2_dec->fref[0]->ref_poc[2]);

	WRITE_VREG(HEVC_MPRED_L0_REF03_POC,avs2_dec->fref[3]->imgtr_fwRefDistance);
	WRITE_VREG(HEVC_MPRED_L1_REF03_POC,avs2_dec->fref[0]->ref_poc[3]);

	WRITE_VREG(HEVC_MPRED_L0_REF04_POC,avs2_dec->fref[4]->imgtr_fwRefDistance);
	WRITE_VREG(HEVC_MPRED_L1_REF04_POC,avs2_dec->fref[0]->ref_poc[4]);

	WRITE_VREG(HEVC_MPRED_L0_REF05_POC,avs2_dec->fref[5]->imgtr_fwRefDistance);
	WRITE_VREG(HEVC_MPRED_L1_REF05_POC,avs2_dec->fref[0]->ref_poc[5]);

	WRITE_VREG(HEVC_MPRED_L0_REF06_POC,avs2_dec->fref[6]->imgtr_fwRefDistance);
	WRITE_VREG(HEVC_MPRED_L1_REF06_POC,avs2_dec->fref[0]->ref_poc[6]);

	WRITE_VREG(HEVC_MPRED_MV_RD_END_ADDR,mpred_mv_rd_end_addr);
}

static void config_dw_fb(struct AVS2Decoder_s *dec, struct avs2_frame_s *pic)
{
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	int dw_mode = get_double_write_mode(dec);
	struct aml_vcodec_ctx * v4l2_ctx = dec->v4l2_ctx;
	uint32_t data = 0, data32;

	if ((dw_mode & 0x10) == 0) {
		WRITE_BACK_8(avs2_dec, HEVC_SAO_CTRL26, 0);

		if (((dw_mode & 0xf) == 8) ||
			((dw_mode & 0xf) == 9)) {
			data = 0xff; //data32 |= (0xff << 16);
			WRITE_BACK_8(avs2_dec, HEVC_SAO_CTRL26, 0xf);
		} else {
			if ((dw_mode & 0xf) == 2 ||
				(dw_mode & 0xf) == 3)
				data = 0xff; //data32 |= (0xff<<16);
			else if ((dw_mode & 0xf) == 4 ||
				(dw_mode & 0xf) == 5)
				data = 0x33; //data32 |= (0x33<<16);

			READ_WRITE_DATA16(avs2_dec, HEVC_SAO_CTRL5, 0, 9, 1); //data32 &= ~(1 << 9);
		}
		READ_WRITE_DATA16(avs2_dec, HEVC_SAO_CTRL5, data, 16, 8);
	} else {
		READ_WRITE_DATA16(avs2_dec, HEVC_SAO_CTRL5, 0, 16, 8);
	}

	/* m8baby test1902 */
	data32 = READ_VREG(HEVC_SAO_CTRL1);
	data32 &= (~0x3000);
	/* [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32 */
	data32 |= (MEM_MAP_MODE << 12);
	data32 &= (~0xff0);
	data32 |= ((dec->endian >> 8) & 0xfff); /* data32 |= 0x670; Big-Endian per 64-bit */
	data32 &= (~0x3); /*[1]:dw_disable [0]:cm_disable*/
	if (dw_mode == 0)
		data = 0x2; //data32 |= 0x2; /*disable double write*/
	else if (dw_mode & 0x10)
		data = 0x1; //data32 |= 0x1; /*disable cm*/

	/* swap uv */
	if ((v4l2_ctx->cap_pix_fmt == V4L2_PIX_FMT_NV21) ||
		(v4l2_ctx->cap_pix_fmt == V4L2_PIX_FMT_NV21M))
		data32 &= ~(1 << 8); /* NV21 */
	else
		data32 |= (1 << 8); /* NV12 */

	data32 &= (~(3 << 14));
	data32 |= (2 << 14);
	/*
	*  [31:24] ar_fifo1_axi_thred
	*  [23:16] ar_fifo0_axi_thred
	*  [15:14] axi_linealign, 0-16bytes, 1-32bytes, 2-64bytes
	*  [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	*  [11:08] axi_lendian_C
	*  [07:04] axi_lendian_Y
	*  [3]     reserved
	*  [2]     clk_forceon
	*  [1]     dw_disable:disable double write output
	*  [0]     cm_disable:disable compress output
	*/
	WRITE_BACK_32(avs2_dec, HEVC_SAO_CTRL1, data32);

	if (dw_mode == 0)
		data = 1; //data32 |= (0x1 << 8); /*enable first write*/
	else if (dw_mode == 0x10)
		data = 2; //data32 |= (0x1 << 9); /*double write only*/
	else
		data = 3; //data32 |= ((0x1 << 8) | (0x1 << 9));

	READ_WRITE_DATA16(avs2_dec, HEVC_DBLK_CFGB, data, 8, 2);

#ifdef LOSLESS_COMPRESS_MODE
	/*SUPPORT_10BIT*/

	data32 = pic->mc_y_adr;
	if (dw_mode && ((dw_mode & 0x20) == 0)) {
		WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, pic->dw_y_adr);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR, pic->dw_u_v_adr);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_WPTR, pic->dw_y_adr);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_C_WPTR, pic->dw_u_v_adr);
	} else {
		WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, 0xffffffff);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR, 0xffffffff);
	}
	if ((dw_mode & 0x10) == 0)
		WRITE_BACK_32(avs2_dec, HEVC_CM_BODY_START_ADDR, data32);

#ifdef AVS2_10B_MMU
	WRITE_BACK_32(avs2_dec, HEVC_CM_HEADER_START_ADDR, pic->header_adr);
#endif
#ifdef AVS2_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, 0);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR, 0);
		WRITE_BACK_32(avs2_dec, HEVC_CM_HEADER_START_ADDR2, pic->dw_header_adr);
	}
#endif
#else
	WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, pic->mc_y_adr);
	WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR, pic->mc_u_v_adr);
	WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_WPTR, pic->mc_y_adr);
	WRITE_BACK_32(avs2_dec, HEVC_SAO_C_WPTR, pic->mc_u_v_adr);
#endif
	WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_LENGTH, pic->luma_size);
	WRITE_BACK_32(avs2_dec, HEVC_SAO_C_LENGTH, pic->chroma_size);
}

static void config_sao_hw_fb(struct AVS2Decoder_s *dec)
{
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	struct avs2_frame_s *pic = avs2_dec->hc.cur_pic;
	//union param_u* params = &avs2_dec->param;
	//uint32_t data32; // data32_2;
	READ_WRITE_DATA16(avs2_dec, HEVC_SAO_CTRL0, avs2_dec->lcu_size_log2, 0, 4);

	config_dw_fb(dec, pic);

#if 0
#ifdef LOSLESS_COMPRESS_MODE
	if ((get_double_write_mode(dec) & 0x10) == 0)
		WRITE_BACK_32(avs2_dec, HEVC_CM_BODY_START_ADDR, pic->mc_y_adr);

	if ((get_double_write_mode(dec) & 0x10) == 0)
		WRITE_BACK_32(avs2_dec, HEVC_CM_BODY_START_ADDR, pic->mc_y_adr);
	if ((get_double_write_mode(dec) & 0x20) == 0) {
		WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, pic->dw_y_adr);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR, pic->dw_u_v_adr);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_WPTR, pic->dw_y_adr);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_C_WPTR, pic->dw_u_v_adr);
	} else {
		WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, 0xffffffff);
		WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR, 0xffffffff);
	}
	WRITE_BACK_32(avs2_dec, HEVC_CM_HEADER_START_ADDR, pic->header_adr);
#ifdef AVS2_10B_MMU_DW
	WRITE_BACK_32(avs2_dec, HEVC_CM_HEADER_START_ADDR2, pic->dw_header_adr);
	WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, 0);
	WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR, 0);
#endif
#ifdef AVS2_10B_MMU
	///WRITE_VREG(HEVC_CM_HEADER_START_ADDR, avs2_dec->cm_header_start + (pic->index * MMU_COMPRESS_HEADER_SIZE));
	//WRITE_BACK_32(avs2_dec, HEVC_CM_HEADER_START_ADDR, avs2_dec->cm_header_start + (pic->index * get_compress_header_size(dec)));
#endif
#ifdef AVS2_10B_MMU_DW
	//if (dec->dw_mmu_enable) {
	///WRITE_VREG(HEVC_CM_HEADER_START_ADDR2, pic->header_dw_adr);
	//   WRITE_BACK_32(avs2_dec, HEVC_CM_HEADER_START_ADDR2, pic->dw_header_adr);
	//}
#endif

#else
	///WRITE_VREG(HEVC_SAO_Y_START_ADDR, pic->mc_y_adr);
	WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_START_ADDR, pic->mc_y_adr);
#endif
	WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_LENGTH , pic->luma_size);

	///WRITE_VREG(HEVC_SAO_C_START_ADDR,DOUBLE_WRITE_CSTART_TEMP);
	//WRITE_BACK_32(avs2_dec, HEVC_SAO_C_START_ADDR,DOUBLE_WRITE_CSTART_TEMP);

	WRITE_BACK_32(avs2_dec, HEVC_SAO_C_LENGTH  , pic->chroma_size);
	///WRITE_VREG(HEVC_SAO_Y_WPTR ,DOUBLE_WRITE_YSTART_TEMP);
	//WRITE_BACK_32(avs2_dec, HEVC_SAO_Y_WPTR ,DOUBLE_WRITE_YSTART_TEMP);
	///WRITE_VREG(HEVC_SAO_C_WPTR ,DOUBLE_WRITE_CSTART_TEMP);
	//WRITE_BACK_32(avs2_dec, HEVC_SAO_C_WPTR ,DOUBLE_WRITE_CSTART_TEMP);
#endif
#ifdef AVS2_10B_NV21
	SHOULD NOT DEFINED !!
	data32 = READ_VREG(HEVC_SAO_CTRL1);
	data32 &= (~0x3000);
	data32 |= (MEM_MAP_MODE << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	data32 &= (~0x3);
	data32 |= 0x1; // [1]:dw_disable [0]:cm_disable
	WRITE_VREG(HEVC_SAO_CTRL1, data32);

	data32 = READ_VREG(HEVC_SAO_CTRL5); // [23:22] dw_v1_ctrl [21:20] dw_v0_ctrl [19:18] dw_h1_ctrl [17:16] dw_h0_ctrl
	data32 &= ~(0xff << 16);               // set them all 0 for H265_NV21 (no down-scale)
	WRITE_VREG(HEVC_SAO_CTRL5, data32);

	data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG);
	data32 &= (~0x30);
	data32 |= (MEM_MAP_MODE << 4); // [5:4]    -- address_format 00:linear 01:32x32 10:64x32
	WRITE_VREG(HEVCD_IPP_AXIIF_CONFIG, data32);
#endif

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] cfgSAO .done.\n");
}

static void config_dblk_hw_fb(struct AVS2Decoder_s *dec)
{
	/*
	* Picture level de-block parameter configuration here
	*/
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	union param_u *rpm_param = &avs2_dec->param;
	uint32_t data32;

#if 0 // NOT Dual Mode
	data32 = READ_VREG(HEVC_DBLK_CFG1);
	data32 = (((data32>>20)&0xfff)<<20) |
		(((avs2_dec->input.sample_bit_depth == 10) ? 0xa:0x0)<<16) |             // [16 +: 4]: {luma_bd[1:0],chroma_bd[1:0]}
		(((data32>>2)&0x3fff)<<2) |
		(((rpm_param->p.lcu_size == 6) ? 0:(rpm_param->p.lcu_size == 5) ? 1:2)<<0);// [ 0 +: 2]: lcu_size
#ifdef LPF_SPCC_ENABLE
	//data32 |= (hevc->pic_w <= 64)?(1<<20):0; // if pic width isn't more than one CTU, disable pipeline
	data32 |= (0x3<<20); // SPCC_ENABLE
#endif
	WRITE_VREG(HEVC_DBLK_CFG1, data32);
#else
	avs2_dec->instruction[avs2_dec->ins_offset] = 0x6450101;                                          //mfsp COMMON_REG_1, HEVC_DBLK_CFG1
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x1a<<22) | ((((avs2_dec->input.sample_bit_depth == 10) ? 0xa:0x0)&0xffff)<<6);   // movi COMMON_REG_0, data
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x, bit_depth %x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset], avs2_dec->input.sample_bit_depth);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = 0x9604040;                                          //ins  COMMON_REG_1, COMMON_REG_0, 16, 4
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x1a<<22) | ((((rpm_param->p.lcu_size == 6) ? 0:(rpm_param->p.lcu_size == 5) ? 1:2))<<6);   // movi COMMON_REG_0, data
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x, rpm_param->p.lcu_size %x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset], rpm_param->p.lcu_size);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = 0x9402040;                                          //ins  COMMON_REG_1, COMMON_REG_0, 0, 2
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#ifdef LPF_SPCC_ENABLE
	avs2_dec->instruction[avs2_dec->ins_offset] = (0x1a<<22) | (3<<6);   // movi COMMON_REG_0, data
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
	avs2_dec->instruction[avs2_dec->ins_offset] = 0x9682040;                                          //ins  COMMON_REG_1, COMMON_REG_0, 20, 2
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#endif

	avs2_dec->instruction[avs2_dec->ins_offset] = 0x6050101;                                          //mtsp COMMON_REG_1, HEVC_DBLK_CFG1
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"instruction[%3d] = %8x\n",  avs2_dec->ins_offset, avs2_dec->instruction[avs2_dec->ins_offset]);
	avs2_dec->ins_offset++;
#endif

	data32 = (avs2_dec->img.height<<16) | avs2_dec->img.width;
	//WRITE_VREG(HEVC_DBLK_CFG2, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFG2, data32);

	data32 = ((avs2_dec->input.crossSliceLoopFilter & 0x1) << 27) |// [27 +: 1]: cross_slice_loopfilter_enable_flag
		((rpm_param->p.loop_filter_disable & 0x1) << 26) |               // [26 +: 1]: loop_filter_disable
		((avs2_dec->input.useNSQT & 0x1) << 25) |                        // [25 +: 1]: useNSQT
		((avs2_dec->img.type & 0x7) << 22) |                             // [22 +: 3]: imgtype
		((rpm_param->p.alpha_c_offset & 0x1f) << 17) |                    // [17 +: 5]: alpha_c_offset (-8~8)
		((rpm_param->p.beta_offset & 0x1f) << 12) |                       // [12 +: 5]: beta_offset (-8~8)
		((rpm_param->p.chroma_quant_param_delta_cb & 0x3f) << 6) |         // [ 6 +: 6]: chroma_quant_param_delta_u (-16~16)
		((rpm_param->p.chroma_quant_param_delta_cr & 0x3f) << 0);          // [ 0 +: 6]: chroma_quant_param_delta_v (-16~16)
	///WRITE_VREG(HEVC_DBLK_CFG9, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFG9, data32);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: crossslice(%d),lfdisable(%d),bitDepth(%d),lcuSize(%d),NSQT(%d)\n",
		avs2_dec->input.crossSliceLoopFilter, rpm_param->p.loop_filter_disable,
		avs2_dec->input.sample_bit_depth, avs2_dec->lcu_size,avs2_dec->input.useNSQT);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: alphaCOffset(%d),betaOffset(%d),quantDeltaCb(%d),quantDeltaCr(%d)\n",
		rpm_param->p.alpha_c_offset, rpm_param->p.beta_offset,
		rpm_param->p.chroma_quant_param_delta_cb, rpm_param->p.chroma_quant_param_delta_cr);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: .done.\n");
}

static void reconstructCoefInfo(struct AVS2Decoder_s *dec,
	int32_t compIdx, struct ALFParam_s *alfParam);
static void config_alf_hw_fb(struct AVS2Decoder_s *dec)
{
	/*
	* Picture level ALF parameter configuration here
	*/
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	uint32_t data32;
	int32_t i,j;
	int32_t m_filters_per_group;
	struct ALFParam_s *m_alfPictureParam_y = &avs2_dec->m_alfPictureParam[0];
	struct ALFParam_s *m_alfPictureParam_cb = &avs2_dec->m_alfPictureParam[1];
	struct ALFParam_s *m_alfPictureParam_cr = &avs2_dec->m_alfPictureParam[2];

	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[t]alfy,cidx(%d),flag(%d),filters_per_group(%d),filterPattern[0]=0x%x,[15]=0x%x\n",
		m_alfPictureParam_y->componentID,
		m_alfPictureParam_y->alf_flag,
		m_alfPictureParam_y->filters_per_group,
		m_alfPictureParam_y->filterPattern[0],m_alfPictureParam_y->filterPattern[15]);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[t]alfy,num_coeff(%d),coeffmulti[0][0]=0x%x,[0][1]=0x%x,[1][0]=0x%x,[1][1]=0x%x\n",
		m_alfPictureParam_y->num_coeff,
		m_alfPictureParam_y->coeffmulti[0][0],
		m_alfPictureParam_y->coeffmulti[0][1],
		m_alfPictureParam_y->coeffmulti[1][0],
		m_alfPictureParam_y->coeffmulti[1][1]);

	// Cr
	for (i=0;i<16;i++) dec->m_varIndTab[i] = 0;
	for (j=0;j<16;j++) for (i=0;i<9;i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 2, m_alfPictureParam_cr);
	data32 = ((dec->m_filterCoeffSym[0][4] & 0xf ) << 28) |
		((dec->m_filterCoeffSym[0][3] & 0x7f) << 21) |
		((dec->m_filterCoeffSym[0][2] & 0x7f) << 14) |
		((dec->m_filterCoeffSym[0][1] & 0x7f) <<  7) |
		((dec->m_filterCoeffSym[0][0] & 0x7f) <<  0);
	///WRITE_VREG(HEVC_DBLK_CFGD, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
	data32 = ((dec->m_filterCoeffSym[0][8] & 0x7f) << 24) |
		((dec->m_filterCoeffSym[0][7] & 0x7f) << 17) |
		((dec->m_filterCoeffSym[0][6] & 0x7f) << 10) |
		((dec->m_filterCoeffSym[0][5] & 0x7f) <<  3) |
		(((dec->m_filterCoeffSym[0][4]>>4) & 0x7 ) <<  0);
	///WRITE_VREG(HEVC_DBLK_CFGD, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_cr(%d), alf_cr_coef(%d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_cr->alf_flag,
		dec->m_filterCoeffSym[0][0],dec->m_filterCoeffSym[0][1],dec->m_filterCoeffSym[0][2],
		dec->m_filterCoeffSym[0][3],dec->m_filterCoeffSym[0][4],dec->m_filterCoeffSym[0][5],
		dec->m_filterCoeffSym[0][6],dec->m_filterCoeffSym[0][7],dec->m_filterCoeffSym[0][8]);

	// Cb
	for (j=0;j<16;j++) for (i=0;i<9;i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 1, m_alfPictureParam_cb);
	data32 = ((dec->m_filterCoeffSym[0][4] & 0xf ) << 28) |
		((dec->m_filterCoeffSym[0][3] & 0x7f) << 21) |
		((dec->m_filterCoeffSym[0][2] & 0x7f) << 14) |
		((dec->m_filterCoeffSym[0][1] & 0x7f) <<  7) |
		((dec->m_filterCoeffSym[0][0] & 0x7f) <<  0);
	///WRITE_VREG(HEVC_DBLK_CFGD, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
		data32 = ((dec->m_filterCoeffSym[0][8] & 0x7f) << 24) |
		((dec->m_filterCoeffSym[0][7] & 0x7f) << 17) |
		((dec->m_filterCoeffSym[0][6] & 0x7f) << 10) |
		((dec->m_filterCoeffSym[0][5] & 0x7f) <<  3) |
		(((dec->m_filterCoeffSym[0][4]>>4) & 0x7 ) <<  0);
	///WRITE_VREG(HEVC_DBLK_CFGD, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_cb(%d), alf_cb_coef(%d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_cb->alf_flag,
		dec->m_filterCoeffSym[0][0], dec->m_filterCoeffSym[0][1], dec->m_filterCoeffSym[0][2],
		dec->m_filterCoeffSym[0][3], dec->m_filterCoeffSym[0][4], dec->m_filterCoeffSym[0][5],
		dec->m_filterCoeffSym[0][6], dec->m_filterCoeffSym[0][7], dec->m_filterCoeffSym[0][8]);

	// Y
	for (j=0;j<16;j++) for (i=0;i<9;i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 0, m_alfPictureParam_y);
	data32 = ((dec->m_varIndTab[7] & 0xf) << 28) | ((dec->m_varIndTab[6] & 0xf) << 24) |
		((dec->m_varIndTab[5] & 0xf) << 20) | ((dec->m_varIndTab[4] & 0xf) << 16) |
		((dec->m_varIndTab[3] & 0xf) << 12) | ((dec->m_varIndTab[2] & 0xf) <<  8) |
		((dec->m_varIndTab[1] & 0xf) <<  4) | ((dec->m_varIndTab[0] & 0xf) <<  0);
	///WRITE_VREG(HEVC_DBLK_CFGD, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
		data32 = ((dec->m_varIndTab[15] & 0xf) << 28) | ((dec->m_varIndTab[14] & 0xf) << 24) |
		((dec->m_varIndTab[13] & 0xf) << 20) | ((dec->m_varIndTab[12] & 0xf) << 16) |
		((dec->m_varIndTab[11] & 0xf) << 12) | ((dec->m_varIndTab[10] & 0xf) <<  8) |
		((dec->m_varIndTab[ 9] & 0xf) <<  4) | ((dec->m_varIndTab[ 8] & 0xf) <<  0);
	///WRITE_VREG(HEVC_DBLK_CFGD, data32);
	WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_y(%d), alf_y_tab(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n",m_alfPictureParam_y->alf_flag,
		dec->m_varIndTab[0], dec->m_varIndTab[1], dec->m_varIndTab[2], dec->m_varIndTab[3],
		dec->m_varIndTab[4], dec->m_varIndTab[5], dec->m_varIndTab[6], dec->m_varIndTab[7],
		dec->m_varIndTab[8], dec->m_varIndTab[9], dec->m_varIndTab[10], dec->m_varIndTab[11],
		dec->m_varIndTab[12], dec->m_varIndTab[13], dec->m_varIndTab[14], dec->m_varIndTab[15]);

	m_filters_per_group = (m_alfPictureParam_y->alf_flag == 0) ? 1 : m_alfPictureParam_y->filters_per_group;
	for (i=0;i<m_filters_per_group;i++) {
		data32 = ((dec->m_filterCoeffSym[i][4] & 0xf ) << 28) |
			((dec->m_filterCoeffSym[i][3] & 0x7f) << 21) |
			((dec->m_filterCoeffSym[i][2] & 0x7f) << 14) |
			((dec->m_filterCoeffSym[i][1] & 0x7f) <<  7) |
			((dec->m_filterCoeffSym[i][0] & 0x7f) <<  0);
		///WRITE_VREG(HEVC_DBLK_CFGD, data32);
		WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
		data32 = ((i == m_filters_per_group-1) << 31) | // [31] last indication
			((dec->m_filterCoeffSym[i][8] & 0x7f) << 24) |
			((dec->m_filterCoeffSym[i][7] & 0x7f) << 17) |
			((dec->m_filterCoeffSym[i][6] & 0x7f) << 10) |
			((dec->m_filterCoeffSym[i][5] & 0x7f) << 3) |
			(((dec->m_filterCoeffSym[i][4] >> 4) & 0x7) <<  0);
		///WRITE_VREG(HEVC_DBLK_CFGD, data32);
		WRITE_BACK_32(avs2_dec, HEVC_DBLK_CFGD, data32);
	}
	if (debug & AVS2_DBG_BUFMGR_DETAIL) {
		for (i=0;i<m_filters_per_group;i++)
			avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
				"[c] alf_y_coef[%d](%d %d %d %d %d %d %d %d %d)\n",i,
				dec->m_filterCoeffSym[i][0], dec->m_filterCoeffSym[i][1],
				dec->m_filterCoeffSym[i][2], dec->m_filterCoeffSym[i][3],
				dec->m_filterCoeffSym[i][4], dec->m_filterCoeffSym[i][5],
				dec->m_filterCoeffSym[i][6], dec->m_filterCoeffSym[i][7],
				dec->m_filterCoeffSym[i][8]);
	}
	avs2_print(dec, AVS2_DBG_BUFMGR_DETAIL,
		"[c] cfgALF .done.\n");
}

static void  config_mcrcc_axi_hw_fb(struct AVS2Decoder_s *dec)
{
	struct avs2_decoder *avs2_dec = &dec->avs2_dec;
	//uint32_t rdata32;
	//uint32_t rdata32_2;

	///WRITE_VREG_V(P_HEVCD_MCRCC_CTL1, 0x2); // reset mcrcc
	WRITE_BACK_8(avs2_dec, HEVCD_MCRCC_CTL1, 0x2); // reset mcrcc

	if (avs2_dec->img.type  == I_IMG) { // I-PIC
		///WRITE_VREG_V(P_HEVCD_MCRCC_CTL1, 0x0); // remove reset -- disables clock
		WRITE_BACK_8(avs2_dec, HEVCD_MCRCC_CTL1, 0x0); // remove reset -- disables clock
		return;
	}

#if 0
	mcrcc_get_hitrate();
	decomp_get_hitrate();
	decomp_get_comprate();
#endif

	if ((avs2_dec->img.type  == B_IMG) || (avs2_dec->img.type  == F_IMG)) {  // B-PIC or F_PIC
		// Programme canvas0
		///WRITE_VREG_V(P_HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 0);
		WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0 << 1) | 0);
		///rdata32 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		///rdata32 = rdata32 & 0xffff;
		///rdata32 = rdata32 | ( rdata32 << 16);
		///WRITE_VREG_V(P_HEVCD_MCRCC_CTL2, rdata32);
		READ_INS_WRITE(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL2, 0, 16, 16);

		// Programme canvas1
		///WRITE_VREG_V(P_HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (16 << 8) | (1<<1) | 0);
		WRITE_BACK_16(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 1, (16 << 8) | (1<<1) | 0);
		///rdata32_2 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		///rdata32_2 = rdata32_2 & 0xffff;
		///rdata32_2 = rdata32_2 | ( rdata32_2 << 16);
		///if ( rdata32 == rdata32_2 ) {
		///    rdata32_2 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		///    rdata32_2 = rdata32_2 & 0xffff;
		///    rdata32_2 = rdata32_2 | ( rdata32_2 << 16);
		///}
		///WRITE_VREG_V(P_HEVCD_MCRCC_CTL3, rdata32_2);
		READ_CMP_WRITE(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL3, 1, 0, 16, 16);
	} else { // P-PIC
		///WRITE_VREG_V(P_HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (1<<1) | 0);
		WRITE_BACK_8(avs2_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (1 << 1) | 0);
		///rdata32 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		///rdata32 = rdata32 & 0xffff;
		///rdata32 = rdata32 | ( rdata32 << 16);
		///WRITE_VREG_V(P_HEVCD_MCRCC_CTL2, rdata32);
		READ_INS_WRITE(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL2, 0, 16, 16);

		// Programme canvas1
		///rdata32 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		///rdata32 = rdata32 & 0xffff;
		///rdata32 = rdata32 | ( rdata32 << 16);
		///WRITE_VREG_V(P_HEVCD_MCRCC_CTL3, rdata32);
		READ_INS_WRITE(avs2_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL3, 0, 16, 16);
	}

	///WRITE_VREG_V(P_HEVCD_MCRCC_CTL1, 0xff0); // enable mcrcc progressive-mode
	WRITE_BACK_16(avs2_dec, HEVCD_MCRCC_CTL1, 0, 0xff0); // enable mcrcc progressive-mode
	return;
}

static void config_other_hw_fb(struct AVS2Decoder_s *dec)
{

}

#endif
