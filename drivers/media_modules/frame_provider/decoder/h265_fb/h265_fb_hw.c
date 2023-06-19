#include "../../../include/regs/dos_registers.h"

#define DOS_PROJECT

/* to do */
//#define DOUBLE_WRITE_VH0_TEMP    0
//#define DOUBLE_WRITE_VH1_TEMP    0
//#define DOUBLE_WRITE_VH0_HALF    0
//#define DOUBLE_WRITE_VH1_HALF    0
//#define DOUBLE_WRITE_YSTART_TEMP    0
//#define DOUBLE_WRITE_CSTART_TEMP    0
//#define DOS_BASE_ADR  0

/**/

//#define MEM_MAP_MODE    0
static int compute_losless_comp_body_size(struct hevc_state_s *hevc,
	int width, int height, int mem_saving_mode);
static int compute_losless_comp_header_size(int width, int height);
static struct PIC_s *get_ref_pic_by_POC(struct hevc_state_s *hevc, int POC);
static int32_t hevc_hw_init(hevc_stru_t* hevc, uint8_t bit_depth, uint8_t front_flag, uint8_t back_flag);
static unsigned char is_ref_long_term(struct hevc_state_s *hevc, int poc);
//static void print_scratch_error(int error_num);
//static void parser_cmd_write(void);
static int get_double_write_mode(struct hevc_state_s *hevc);

typedef union param_u param_t;

static unsigned gbpc;
static void WRITE_BACK_RET(hevc_stru_t* hevc)
{
	hevc->instruction[hevc->ins_offset] = 0xcc00000;   //ret
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0;           //nop
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
}

static void WRITE_BACK_8(hevc_stru_t* hevc, uint32_t spr_addr, uint8_t data)
{
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	hevc->instruction[hevc->ins_offset] = (0x20<<22) | ((spr_addr&0xfff)<<8) | (data&0xff);   //mtspi data, spr_addr
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
		hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	gbpc = READ_VREG(HEVC_MPC_E_DBE);
	if ((gbpc >= 0x670 && gbpc < 0xc00) || print_reg_flag)
		pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", gbpc);
}

static void WRITE_BACK_16(hevc_stru_t* hevc, uint32_t spr_addr, uint8_t rd_addr, uint16_t data)
{
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	hevc->instruction[hevc->ins_offset] = (0x1a<<22) | ((data&0xffff)<<6) | (rd_addr&0x3f);       // movi rd_addr, data[15:0]
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"%s(%x,%x,%x)\ninstruction[%3d] = %8x, data= %x\n",
		__func__, spr_addr, rd_addr, data,
		hevc->ins_offset, hevc->instruction[hevc->ins_offset], data&0xffff);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8) | (rd_addr&0x3f);  // mtsp rd_addr, spr_addr
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	gbpc = READ_VREG(HEVC_MPC_E_DBE);
	if ((gbpc >= 0x670 && gbpc < 0xc00) || print_reg_flag)
		pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", gbpc);
}

static void WRITE_BACK_32(hevc_stru_t* hevc, uint32_t spr_addr, uint32_t data)
{
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	hevc->instruction[hevc->ins_offset] = (0x1a<<22) | ((data&0xffff)<<6);   // movi COMMON_REG_0, data
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
		hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	data = (data & 0xffff0000)>>16;
	hevc->instruction[hevc->ins_offset] = (0x1b<<22) | (data<<6);                // mvihi COMMON_REG_0, data[31:16]
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8);    // mtsp COMMON_REG_0, spr_addr
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	gbpc = READ_VREG(HEVC_MPC_E_DBE);
	if ((gbpc >= 0x670 && gbpc < 0xc00) || print_reg_flag)
		pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", gbpc);
}

static void READ_INS_WRITE(hevc_stru_t* hevc, uint32_t spr_addr0, uint32_t spr_addr1, uint8_t rd_addr, uint8_t position, uint8_t size)
{
	//spr_addr0 = ((spr_addr0 - DOS_BASE_ADR) >> 0) & 0xfff;
	//spr_addr1 = ((spr_addr1 - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr0 = (dos_reg_compat_convert(spr_addr0) & 0xfff);
	spr_addr1 = (dos_reg_compat_convert(spr_addr1) & 0xfff);
	hevc->instruction[hevc->ins_offset] = (0x19<<22) | ((spr_addr0&0xfff)<<8) | (rd_addr&0x3f);    //mfsp rd_addr, src_addr0
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"%s(%x,%x,%x,%x,%x)\n",
		__func__, spr_addr0, spr_addr1, rd_addr, position, size);
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | ((rd_addr&0x3f)<<6) | (rd_addr&0x3f);   //ins rd_addr, rd_addr, position, size
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x18<<22) | ((spr_addr1&0xfff)<<8) | (rd_addr&0x3f);    //mtsp rd_addr, src_addr1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
}

//Caution:  pc offset fixed to 4, the data of cmp_addr need ready before call this function
void READ_CMP_WRITE(hevc_stru_t* hevc, uint32_t spr_addr0, uint32_t spr_addr1, uint8_t rd_addr, uint8_t cmp_addr, uint8_t position, uint8_t size)
{
	spr_addr0 = (dos_reg_compat_convert(spr_addr0) & 0xfff);
	spr_addr1 = (dos_reg_compat_convert(spr_addr1) & 0xfff);
	hevc->instruction[hevc->ins_offset] = (0x19<<22) | ((spr_addr0&0xfff)<<8) | (rd_addr&0x3f);    //mfsp rd_addr, src_addr0
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"%s(%x,%x,%x,%x,%x,%x)\n",
		__func__, spr_addr0, spr_addr1, rd_addr, cmp_addr, position, size);
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | ((rd_addr&0x3f)<<6) | (rd_addr&0x3f);   //ins rd_addr, rd_addr, position, size
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
		hevc->instruction[hevc->ins_offset] = (0x29<<22) | (4<<12) | ((rd_addr&0x3f)<<6) | cmp_addr;     //cbne current_pc+4, rd_addr, cmp_addr
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
		hevc->instruction[hevc->ins_offset] = 0;                                                       //nop
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x19<<22) | ((spr_addr0&0xfff)<<8) | (rd_addr&0x3f);    //mfsp rd_addr, src_addr0
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | ((rd_addr&0x3f)<<6) | (rd_addr&0x3f);   //ins rd_addr, rd_addr, position, size
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x18<<22) | ((spr_addr1&0xfff)<<8) | (rd_addr&0x3f);    //mtsp rd_addr, src_addr1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
}

static void READ_WRITE_DATA16(hevc_stru_t* hevc, uint32_t spr_addr, uint16_t data, uint8_t position, uint8_t size)
{
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	hevc->instruction[hevc->ins_offset] = (0x19<<22) | ((spr_addr&0xfff)<<8);    //mfsp COMON_REG_0, spr_addr
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"%s(%x,%x,%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data, position, size,
		hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x1a<<22) | (data<<6) | 1;        //movi COMMON_REG_1, data
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | (0<<6) | 1;  //ins COMMON_REG_0, COMMON_REG_1, position, size
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8);    //mtsp COMMON_REG_0, spr_addr
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;

	gbpc = READ_VREG(HEVC_MPC_E_DBE);
	if ((gbpc >= 0x670 && gbpc < 0xc00) || print_reg_flag)
		pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", gbpc);
}

#if 0
void READ_BACK_32(hevc_stru_t* hevc, uint32_t spr_addr, uint8_t rd_addr)
{
	spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	hevc->instruction[hevc->ins_offset] = (0x19<<22) | ((spr_addr&0xfff)<<8) | (rd_addr&0x3f);  //mfsp rd_addr, spr_addr
	printk("instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
}
#endif

static void init_decode_head_hw_fb(struct hevc_state_s *hevc)
{

	struct BuffInfo_s *buf_spec = hevc->work_space_buf;
	//unsigned int data32;

	int losless_comp_header_size =
		compute_losless_comp_header_size(hevc->pic_w,
		hevc->pic_h);
	int losless_comp_body_size = compute_losless_comp_body_size(hevc,
		hevc->pic_w, hevc->pic_h, hevc->mem_saving_mode);

	hevc->losless_comp_body_size = losless_comp_body_size;

	if (hevc->mmu_enable) {
		WRITE_BACK_8(hevc, HEVCD_MPP_DECOMP_CTL1, (0x1 << 4));
		WRITE_BACK_8(hevc, HEVCD_MPP_DECOMP_CTL2, 0x0);
	} else {
	if (hevc->mem_saving_mode == 1)
		WRITE_BACK_8(hevc, HEVCD_MPP_DECOMP_CTL1,
		(1 << 3) | ((workaround_enable & 2) ? 1 : 0));
	else
		WRITE_BACK_8(hevc, HEVCD_MPP_DECOMP_CTL1,
		((workaround_enable & 2) ? 1 : 0));
	WRITE_BACK_32(hevc, HEVCD_MPP_DECOMP_CTL2, (losless_comp_body_size >> 5));
	/*
	*WRITE_VREG(HEVCD_MPP_DECOMP_CTL3,(0xff<<20) | (0xff<<10) | 0xff);
	*  //8-bit mode
	*/
	}
	WRITE_BACK_32(hevc, HEVC_CM_BODY_LENGTH, losless_comp_body_size);
	WRITE_BACK_32(hevc, HEVC_CM_HEADER_OFFSET, losless_comp_body_size);
	WRITE_BACK_32(hevc, HEVC_CM_HEADER_LENGTH, losless_comp_header_size);

	if (hevc->mmu_enable) {
		//data32 = READ_VREG(HEVC_SAO_CTRL9);
		//data32 |= 0x1;
		//WRITE_VREG(HEVC_SAO_CTRL9, data32);
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL9, 0x1, 0, 1);

		/* use HEVC_CM_HEADER_START_ADDR */
		//data32 = READ_VREG(HEVC_SAO_CTRL5);
		//data32 |= (1<<10);
		//WRITE_VREG(HEVC_SAO_CTRL5, data32);
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 0x1, 10, 1);
	}
#ifdef H265_10B_MMU_DW
	if (hevc->dw_mmu_enable) {
		//u32 data_tmp;
		//data_tmp = READ_VREG(HEVC_SAO_CTRL9);
		//data_tmp |= (1 << 10);
		//WRITE_VREG(HEVC_SAO_CTRL9, data_tmp);
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL9, 0x1, 10, 1);

		WRITE_BACK_32(hevc, HEVC_CM_BODY_LENGTH2,
		losless_comp_body_size);
		WRITE_BACK_32(hevc, HEVC_CM_HEADER_OFFSET2,
		losless_comp_body_size);
		WRITE_BACK_32(hevc, HEVC_CM_HEADER_LENGTH2,
		losless_comp_header_size);

		WRITE_BACK_32(hevc, HEVC_SAO_MMU_VH0_ADDR2,
		buf_spec->mmu_vbh_dw.buf_start);
		WRITE_BACK_32(hevc, HEVC_SAO_MMU_VH1_ADDR2,
		buf_spec->mmu_vbh_dw.buf_start + DW_VBH_BUF_SIZE(buf_spec));
		WRITE_BACK_32(hevc, HEVC_DW_VH0_ADDDR,
		buf_spec->mmu_vbh_dw.buf_start + (2 * DW_VBH_BUF_SIZE(buf_spec)));
		WRITE_BACK_32(hevc, HEVC_DW_VH1_ADDDR,
		buf_spec->mmu_vbh_dw.buf_start + (3 * DW_VBH_BUF_SIZE(buf_spec)));
		/* use HEVC_CM_HEADER_START_ADDR */
		//data32 |= (1 << 15);
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 0x1, 15, 1);
	} else
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 0x0, 15, 1); //data32 &= ~(1 << 15);

	//WRITE_VREG(HEVC_SAO_CTRL5, data32);
#endif
	if (!hevc->m_ins_flag)
		hevc_print(hevc, 0,
		"%s: (%d, %d) body_size 0x%x header_size 0x%x\n",
		__func__, hevc->pic_w, hevc->pic_h,
		losless_comp_body_size, losless_comp_header_size);

}

static void init_pic_list_hw_fb(struct hevc_state_s *hevc)
{
	int i;
	int cur_pic_num = MAX_REF_PIC_NUM;
	int dw_mode = get_double_write_mode(hevc);
	PRINT_LINE();
	hevc->ins_offset = 0;
	//WRITE_BACK_RET(hevc);//???????????debug only

	WRITE_BACK_8(hevc, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
		(0x1 << 1) | (0x1 << 2));

	for (i = 0; i < MAX_REF_PIC_NUM; i++) {
		if (hevc->m_PIC[i] == NULL ||
		hevc->m_PIC[i]->index == -1) {
		cur_pic_num = i;
		break;
		}
		if (hevc->mmu_enable && ((dw_mode & 0x10) == 0))
		WRITE_BACK_32(hevc, HEVCD_MPP_ANC2AXI_TBL_DATA,
			hevc->m_PIC[i]->header_adr>>5);
		else
		WRITE_BACK_32(hevc, HEVCD_MPP_ANC2AXI_TBL_DATA,
			hevc->m_PIC[i]->mc_y_adr >> 5);
		if (dw_mode & 0x10) {
		WRITE_BACK_32(hevc, HEVCD_MPP_ANC2AXI_TBL_DATA,
		hevc->m_PIC[i]->mc_u_v_adr >> 5);
		}
	}
	if (cur_pic_num == 0)
		return;

	WRITE_BACK_8(hevc, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x1);

	/* Zero out canvas registers in IPP -- avoid simulation X */
	WRITE_BACK_8(hevc, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR,
			(0 << 8) | (0 << 1) | 1);
	for (i = 0; i < 32; i++)
		WRITE_BACK_8(hevc, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);

#ifdef LOSLESS_COMPRESS_MODE
	if ((dw_mode & 0x10) == 0)
		init_decode_head_hw_fb(hevc);
#endif

}

static int32_t config_mc_buffer_fb(hevc_stru_t* hevc, PIC_t* cur_pic)
{
	int32_t i;
	PIC_t* pic;
	hevc_print(hevc, H265_DEBUG_BUFMGR,
		"%s (slice_type : %d)\n",
		__func__, cur_pic->slice_type);
	if (cur_pic->slice_type != 2) { //P and B pic
		//WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);
		WRITE_BACK_8(hevc,HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);
		for (i=0; i<cur_pic->RefNum_L0; i++) {
		pic = get_ref_pic_by_POC(hevc, cur_pic->m_aiRefPOCList0[cur_pic->slice_idx][i]);
		hevc_print(hevc, H265_DEBUG_BUFMGR,
			" L0 : %d : pic : 0x%x, POC : %d\n", i, pic, cur_pic->m_aiRefPOCList0[cur_pic->slice_idx][i]);
		if (pic) {
#if 0 //ndef FB_BUF_DEBUG_NO_PIPLINE
			for (j = 0; j < MAX_REF_PIC_NUM; j++) {
			if (pic == cur_pic->ref_pic[j])
				break;
			if (cur_pic->ref_pic[j] == NULL) {
				cur_pic->ref_pic[j] = pic;
				pic->backend_ref++;
				break;
			}
			}
#endif

			if (pic->error_mark) {
			cur_pic->error_mark = 1;
			if (debug)
				hevc_print(hevc, 0, " cur_pic->error_mark set to 1 because of pic->error_mark : %d\n", pic->error_mark);
			}
			//WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR, (pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			WRITE_BACK_32(hevc,HEVCD_MPP_ANC_CANVAS_DATA_ADDR, (pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			hevc_print(hevc, H265_DEBUG_BUFMGR,
			"refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i,pic->mc_canvas_u_v,pic->mc_canvas_y);
		}
		else{
			if (debug)
			hevc_print(hevc, 0, "Error %s, %dth poc (%d) of RPS is not in the pic list0\n", __func__, i, cur_pic->m_aiRefPOCList0[cur_pic->slice_idx][i]);
			cur_pic->error_mark = 1;
			//dump_lmem();
		}
		}
	}
	if (cur_pic->slice_type == 0) { //B pic
		hevc_print(hevc, H265_DEBUG_BUFMGR,
		"config_mc_buffer RefNum_L1\n");
		//WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (16 << 8) | (0<<1) | 1);
		WRITE_BACK_16(hevc, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (16 << 8) | (0<<1) | 1);
		for (i=0; i<cur_pic->RefNum_L1; i++) {
		pic = get_ref_pic_by_POC(hevc, cur_pic->m_aiRefPOCList1[cur_pic->slice_idx][i]);
		hevc_print(hevc, H265_DEBUG_BUFMGR,
			" L1 : %d : pic : 0x%x, POC : %d\n", i, pic, cur_pic->m_aiRefPOCList1[cur_pic->slice_idx][i]);
#if 0 //ndef FB_BUF_DEBUG_NO_PIPLINE
			for (j = 0; j < MAX_REF_PIC_NUM; j++) {
			if (pic == cur_pic->ref_pic[j])
				break;
			if (cur_pic->ref_pic[j] == NULL) {
				cur_pic->ref_pic[j] = pic;
				pic->backend_ref++;
				break;
			}
			}
#endif
		if (pic) {
			if (pic->error_mark) {
			cur_pic->error_mark = 1;
			hevc_print(hevc, H265_DEBUG_BUFMGR,
				" L1 cur_pic->error_mark set to 1 because of pic->error_mark : %d\n", pic->error_mark);
			}
			//WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR, (pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			WRITE_BACK_32(hevc,HEVCD_MPP_ANC_CANVAS_DATA_ADDR, (pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			hevc_print(hevc, H265_DEBUG_BUFMGR,
			"refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i,pic->mc_canvas_u_v,pic->mc_canvas_y);

		}
		else {
			if (debug)
			hevc_print(hevc, 0,
			"Error %s, %dth poc (%d) of RPS is not in the pic list1\n",
			__func__, i, cur_pic->m_aiRefPOCList1[cur_pic->slice_idx][i]);
			cur_pic->error_mark = 1;
			//dump_lmem();
		}
		}
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
}

static void print_loopbufs_ptr(struct hevc_state_s* hevc, char* mark, buff_ptr_t* ptr)
{
	hevc_print(hevc, H265_DEBUG_BUFMGR, "==%s==:\n", mark);
	hevc_print(hevc, H265_DEBUG_BUFMGR,
		"mmu0_ptr 0x%x, mmu1_ptr 0x%x, scalelut_ptr 0x%x pre_ptr 0x%x, vcpu_imem_ptr 0x%x, sys_imem_ptr 0x%x (vir 0x%x), lmem0_ptr 0x%x, lmem1_ptr 0x%x, parser_sao0_ptr 0x%x, parser_sao1_ptr 0x%x, mpred_imp0_ptr 0x%x, mpred_imp1_ptr 0x%x\n",
		ptr->mmu0_ptr,
		ptr->mmu1_ptr,
		ptr->scalelut_ptr,
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

static int init_mmu_fb_bufstate(struct hevc_state_s* hevc, int mmu_4k_number)
{
	int ret;
	dma_addr_t tmp_phy_adr;
	int mmu_map_size = ((mmu_4k_number * 4) >> 6) << 6;
	int tvp_flag = vdec_secure(hw_to_vdec(hevc)) ? CODEC_MM_FLAGS_TVP : 0;

	hevc_print(hevc, 0,
		"%s:mmu_4k_number = %d\n", __func__, mmu_4k_number);

	if (mmu_4k_number < 0)
		return -1;

	hevc->mmu_box_fb = decoder_mmu_box_alloc_box(DRIVER_NAME,
		hevc->index, 2, (mmu_4k_number << 12) * 2, tvp_flag);

	hevc->fb_buf_mmu0_addr =
			dma_alloc_coherent(amports_get_dma_device(),
			mmu_map_size, &tmp_phy_adr, GFP_KERNEL);
	hevc->fb_buf_mmu0.buf_start = tmp_phy_adr;
	if (hevc->fb_buf_mmu0_addr == NULL) {
		pr_err("%s: failed to alloc fb_mmu0_map\n", __func__);
		return -1;
	}
	memset(hevc->fb_buf_mmu0_addr, 0, mmu_map_size);
	hevc->fb_buf_mmu0.buf_size = mmu_map_size;
	hevc->fb_buf_mmu0.buf_end = hevc->fb_buf_mmu0.buf_start + mmu_map_size;

	hevc->fb_buf_mmu1_addr =
			dma_alloc_coherent(amports_get_dma_device(),
			mmu_map_size, &tmp_phy_adr, GFP_KERNEL);
	hevc->fb_buf_mmu1.buf_start = tmp_phy_adr;
	if (hevc->fb_buf_mmu1_addr == NULL) {
		pr_err("%s: failed to alloc fb_mmu1_map\n", __func__);
		return -1;
	}
	memset(hevc->fb_buf_mmu1_addr, 0, mmu_map_size);
	hevc->fb_buf_mmu1.buf_size = mmu_map_size;
	hevc->fb_buf_mmu1.buf_end = hevc->fb_buf_mmu1.buf_start + mmu_map_size;

	ret = decoder_mmu_box_alloc_idx(
			hevc->mmu_box_fb,
			0, mmu_4k_number, hevc->fb_buf_mmu0_addr);
	if (ret != 0) {
		pr_err("%s: failed to alloc fb_mmu0 pages", __func__);
		return -1;
	}

	ret = decoder_mmu_box_alloc_idx(
			hevc->mmu_box_fb,
			1, mmu_4k_number, hevc->fb_buf_mmu1_addr);
	if (ret != 0) {
		pr_err("%s: failed to alloc fb_mmu1 pages", __func__);
		return -1;
	}

	hevc->mmu_fb_4k_number = mmu_4k_number;
	hevc->fr.mmu0_ptr = hevc->fb_buf_mmu0.buf_start;
	hevc->bk.mmu0_ptr = hevc->fb_buf_mmu0.buf_start;
	hevc->fr.mmu1_ptr = hevc->fb_buf_mmu1.buf_start;
	hevc->bk.mmu1_ptr = hevc->fb_buf_mmu1.buf_start;

	return 0;
}

static int init_fb_bufstate(struct hevc_state_s* hevc)
{
/*simulation code: change to use linux APIs; also need write uninit_fb_bufstate()*/
	int ret;
	dma_addr_t tmp_phy_adr;
	unsigned long tmp_adr;
	int pic_w = hevc->max_pic_w ? hevc->max_pic_w : hevc->frame_width;
	int pic_h = hevc->max_pic_h ? hevc->max_pic_h : hevc->frame_height;
	int mmu_4k_number = hevc->fb_ifbuf_num * hevc_mmu_page_num(hevc, pic_w,
			pic_h, 0); //hevc->bit_depth_luma == 8);

	ret = init_mmu_fb_bufstate(hevc, mmu_4k_number);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc mmu fb buffer\n", __func__);
		return -1;
	}

	hevc->fb_buf_scalelut.buf_size = IFBUF_SCALELUT_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUF_SCALELUT_ID, hevc->fb_buf_scalelut.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_scalelut.buf_size);
	}
	hevc->fb_buf_scalelut.buf_start = tmp_adr;
	hevc->fb_buf_scalelut.buf_end = hevc->fb_buf_scalelut.buf_start + hevc->fb_buf_scalelut.buf_size;

	hevc->fb_buf_vcpu_imem.buf_size = IFBUF_VCPU_IMEM_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUF_VCPU_IMEM_ID, hevc->fb_buf_vcpu_imem.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_vcpu_imem.buf_size);
	}
	hevc->fb_buf_vcpu_imem.buf_start = tmp_adr;
	hevc->fb_buf_vcpu_imem.buf_end = hevc->fb_buf_vcpu_imem.buf_start + hevc->fb_buf_vcpu_imem.buf_size;

	hevc->fb_buf_sys_imem.buf_size = IFBUF_SYS_IMEM_SIZE * hevc->fb_ifbuf_num;
	hevc->fb_buf_sys_imem_addr =
			dma_alloc_coherent(amports_get_dma_device(),
			hevc->fb_buf_sys_imem.buf_size,
			&tmp_phy_adr, GFP_KERNEL);
	hevc->fb_buf_sys_imem.buf_start = tmp_phy_adr;
	if (hevc->fb_buf_sys_imem_addr == NULL) {
		pr_err("%s: failed to alloc fb_buf_sys_imem\n", __func__);
		return -1;
	}
	hevc->fb_buf_sys_imem.buf_end = hevc->fb_buf_sys_imem.buf_start + hevc->fb_buf_sys_imem.buf_size;

	hevc->fb_buf_lmem0.buf_size = IFBUF_LMEM0_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUF_LMEM0_ID, hevc->fb_buf_lmem0.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_lmem0.buf_size);
	}
	hevc->fb_buf_lmem0.buf_start = tmp_adr;
	hevc->fb_buf_lmem0.buf_end = hevc->fb_buf_lmem0.buf_start + hevc->fb_buf_lmem0.buf_size;

	hevc->fb_buf_lmem1.buf_size = IFBUF_LMEM1_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUF_LMEM1_ID, hevc->fb_buf_lmem1.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_lmem1.buf_size);
	}
	hevc->fb_buf_lmem1.buf_start = tmp_adr;
	hevc->fb_buf_lmem1.buf_end = hevc->fb_buf_lmem1.buf_start + hevc->fb_buf_lmem1.buf_size;

	hevc->fb_buf_parser_sao0.buf_size = IFBUF_PARSER_SAO0_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUF_PARSER_SAO0_ID, hevc->fb_buf_parser_sao0.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_parser_sao0.buf_size);
	}
	hevc->fb_buf_parser_sao0.buf_start = tmp_adr;
	hevc->fb_buf_parser_sao0.buf_end = hevc->fb_buf_parser_sao0.buf_start + hevc->fb_buf_parser_sao0.buf_size;

	hevc->fb_buf_parser_sao1.buf_size = IFBUF_PARSER_SAO1_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUF_PARSER_SAO1_ID, hevc->fb_buf_parser_sao1.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_parser_sao1.buf_size);
	}
	hevc->fb_buf_parser_sao1.buf_start = tmp_adr;
	hevc->fb_buf_parser_sao1.buf_end = hevc->fb_buf_parser_sao1.buf_start + hevc->fb_buf_parser_sao1.buf_size;

	hevc->fb_buf_mpred_imp0.buf_size = IFBUF_MPRED_IMP0_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUFF_MPRED_IMP0_ID, hevc->fb_buf_mpred_imp0.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_mpred_imp0.buf_size);
	}
	hevc->fb_buf_mpred_imp0.buf_start = tmp_adr;
	hevc->fb_buf_mpred_imp0.buf_end = hevc->fb_buf_mpred_imp0.buf_start + hevc->fb_buf_mpred_imp0.buf_size;

	hevc->fb_buf_mpred_imp1.buf_size = IFBUF_MPRED_IMP1_SIZE * hevc->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hevc->bmmu_box,
			BMMU_IFBUFF_MPRED_IMP1_ID, hevc->fb_buf_mpred_imp1.buf_size,
			DRIVER_NAME, &tmp_adr);
	if (ret) {
		hevc_print(hevc, 0, "%s: failed to alloc buffer, %d\n", __func__, __LINE__);
		return -1;
	} else {
		if (!vdec_secure(hw_to_vdec(hevc)))
			codec_mm_memset(tmp_adr, 0, hevc->fb_buf_mpred_imp1.buf_size);
	}
	hevc->fb_buf_mpred_imp1.buf_start = tmp_adr;
	hevc->fb_buf_mpred_imp1.buf_end = hevc->fb_buf_mpred_imp1.buf_start + hevc->fb_buf_mpred_imp1.buf_size;

	hevc->fr.scalelut_ptr = hevc->fb_buf_scalelut.buf_start;
	hevc->bk.scalelut_ptr = hevc->fb_buf_scalelut.buf_start;
	hevc->fr.vcpu_imem_ptr = hevc->fb_buf_vcpu_imem.buf_start;
	hevc->bk.vcpu_imem_ptr = hevc->fb_buf_vcpu_imem.buf_start;
	hevc->fr.sys_imem_ptr = hevc->fb_buf_sys_imem.buf_start;
	hevc->bk.sys_imem_ptr = hevc->fb_buf_sys_imem.buf_start;
	hevc->fr.lmem0_ptr = hevc->fb_buf_lmem0.buf_start;
	hevc->bk.lmem0_ptr = hevc->fb_buf_lmem0.buf_start;
	hevc->fr.lmem1_ptr = hevc->fb_buf_lmem1.buf_start;
	hevc->bk.lmem1_ptr = hevc->fb_buf_lmem1.buf_start;
	hevc->fr.parser_sao0_ptr = hevc->fb_buf_parser_sao0.buf_start;
	hevc->bk.parser_sao0_ptr = hevc->fb_buf_parser_sao0.buf_start;
	hevc->fr.parser_sao1_ptr = hevc->fb_buf_parser_sao1.buf_start;
	hevc->bk.parser_sao1_ptr = hevc->fb_buf_parser_sao1.buf_start;
	hevc->fr.mpred_imp0_ptr = hevc->fb_buf_mpred_imp0.buf_start;
	hevc->bk.mpred_imp0_ptr = hevc->fb_buf_mpred_imp0.buf_start;
	hevc->fr.mpred_imp1_ptr = hevc->fb_buf_mpred_imp1.buf_start;
	hevc->bk.mpred_imp1_ptr = hevc->fb_buf_mpred_imp1.buf_start;
	hevc->fr.scalelut_ptr_pre = 0;
	hevc->bk.scalelut_ptr_pre = 0;

	hevc->fr.sys_imem_ptr_v = hevc->fb_buf_sys_imem_addr; //for linux
	print_loopbufs_ptr(hevc, "init", &hevc->fr);

	return 0;
}

static void uninit_mmu_fb_bufstate(struct hevc_state_s* hevc)
{
	if (hevc->fb_buf_mmu0_addr) {
		dma_free_coherent(amports_get_dma_device(),
				hevc->fb_buf_mmu0.buf_size, hevc->fb_buf_mmu0_addr,
					hevc->fb_buf_mmu0.buf_start);
		hevc->fb_buf_mmu0_addr = NULL;
	}
	if (hevc->fb_buf_mmu1_addr) {
		dma_free_coherent(amports_get_dma_device(),
				hevc->fb_buf_mmu1.buf_size, hevc->fb_buf_mmu1_addr,
					hevc->fb_buf_mmu1.buf_start);
		hevc->fb_buf_mmu1_addr = NULL;
	}
	hevc->mmu_fb_4k_number = 0;

	if (hevc->mmu_box_fb) {
		decoder_mmu_box_free(hevc->mmu_box_fb);
		hevc->mmu_box_fb = NULL;
	}
}

static void uninit_fb_bufstate(struct hevc_state_s* hevc)
{
	int i;
	for (i = 0; i < FB_LOOP_BUF_COUNT; i++) {
		if (i != BMMU_IFBUF_SYS_IMEM_ID)
			decoder_bmmu_box_free_idx(hevc->bmmu_box, i);
	}

	if (hevc->fb_buf_sys_imem_addr) {
		dma_free_coherent(amports_get_dma_device(),
				hevc->fb_buf_sys_imem.buf_size, hevc->fb_buf_sys_imem_addr,
					hevc->fb_buf_sys_imem.buf_start);
		hevc->fb_buf_sys_imem_addr = NULL;
	}

	uninit_mmu_fb_bufstate(hevc);
}

static void config_bufstate_front_hw(struct hevc_state_s* hevc)
{
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_START, hevc->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_END, hevc->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0, hevc->fr.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_START, hevc->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_END, hevc->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1, hevc->fr.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_parser_sao0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_parser_sao0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.parser_sao0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.parser_sao0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_parser_sao1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_parser_sao1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.parser_sao1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.parser_sao1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

//    config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

// config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_scalelut.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_scalelut.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.scalelut_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.scalelut_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, hevc->fr.scalelut_ptr_pre);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_vcpu_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_vcpu_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.vcpu_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.vcpu_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

//config lmem buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_lmem0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_lmem0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.lmem0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.lmem0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, hevc->fb_buf_lmem1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, hevc->fb_buf_lmem1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, hevc->fr.lmem1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, hevc->bk.lmem1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
}

static void config_bufstate_back_hw(struct hevc_state_s* hevc)
{
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_START, hevc->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_END, hevc->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0, hevc->bk.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_START, hevc->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_END, hevc->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1, hevc->bk.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_parser_sao0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_parser_sao0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.parser_sao0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_parser_sao1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_parser_sao1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.parser_sao1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

//    config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.mpred_imp0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.mpred_imp1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

// config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_scalelut.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_scalelut.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.scalelut_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, hevc->bk.scalelut_ptr_pre);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_vcpu_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_vcpu_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.vcpu_imem_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.sys_imem_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

// config lmem buffers
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_lmem0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_lmem0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.lmem0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, hevc->fb_buf_lmem1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, hevc->fb_buf_lmem1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, hevc->bk.lmem1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

}

static void read_bufstate_front(struct hevc_state_s* hevc)
{
	hevc->fr.mmu0_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0);
	hevc->fr.mmu1_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	hevc->fr.scalelut_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	//hevc->fr.scalelut_ptr_pre = READ_VREG(HEVC_ASSIST_RING_F_THRESHOLD);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	hevc->fr.vcpu_imem_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	//WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
	//hevc->fr.sys_imem_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	hevc->fr.lmem0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	hevc->fr.lmem1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	hevc->fr.parser_sao0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	hevc->fr.parser_sao1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	hevc->fr.mpred_imp0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	hevc->fr.mpred_imp1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);

	hevc->fr.sys_imem_ptr = hevc->sys_imem_ptr;
	hevc->fr.sys_imem_ptr_v = hevc->sys_imem_ptr_v;
}

/**/
void BackEnd_StartDecoding(struct hevc_state_s* hevc)
{
	PIC_t* pic = hevc->next_be_decode_pic[hevc->fb_rd_pos];
	int ret;
	int cur_mmu_4k_number;
	int i;
	int dw_mode = get_double_write_mode(hevc);

	hevc_print(hevc, PRINT_FLAG_VDEC_STATUS,
		"Start BackEnd Decoding %d (wr pos %d, rd pos %d)\n",
		hevc->backend_decoded_count, hevc->fb_wr_pos, hevc->fb_rd_pos);

	mutex_lock(&hevc->fb_mutex);
	for (i = 0; (i < MAX_REF_PIC_NUM) && (pic->error_mark == 0); i++) {
		if (pic->ref_pic[i]) {
			if (pic->ref_pic[i]->error_mark) {
				hevc->gvs->error_frame_count++;
				if (pic->slice_type == I_SLICE) {
					hevc->gvs->i_concealed_frames++;
				} else if (pic->slice_type == P_SLICE) {
					hevc->gvs->p_concealed_frames++;
				} else if (pic->slice_type == B_SLICE) {
					hevc->gvs->b_concealed_frames++;
				}
				pic->error_mark = 1;
				break;
			}
		}
	}
	mutex_unlock(&hevc->fb_mutex);

	if ((pic->error_mark && (hevc->nal_skip_policy != 0)) ||
		(hevc->front_back_mode != 1 && hevc->front_back_mode != 3)) {

		mutex_lock(&hevc->fb_mutex);
		hevc->gvs->drop_frame_count++;
		if (pic->slice_type == I_SLICE) {
			hevc->gvs->i_lost_frames++;
		} else if (pic->slice_type == P_SLICE) {
			hevc->gvs->p_lost_frames++;
		} else if (pic->slice_type == B_SLICE) {
			hevc->gvs->b_lost_frames++;
		}
		mutex_unlock(&hevc->fb_mutex);

		copy_loopbufs_ptr(&hevc->bk, &hevc->next_bk[hevc->fb_rd_pos]);
		print_loopbufs_ptr(hevc, "bk", &hevc->bk);
		hevc_hw_init(hevc, pic->depth, 0, 1);

		WRITE_VREG(hevc->backend_ASSIST_MBOX0_IRQ_REG, 1);
		return;
	}

	if (dw_mode != 0x10) {
		cur_mmu_4k_number = hevc_mmu_page_num(hevc, pic->width, pic->height / 2 + 64 + 8, 0); // to do: !bit_depth_10);
		if (cur_mmu_4k_number < 0)
			return;
		ret = decoder_mmu_box_alloc_idx(
			hevc->mmu_box,
			pic->index,
			cur_mmu_4k_number,
			hevc->frame_mmu_map_addr);
		if (ret == 0)
			ret = decoder_mmu_box_alloc_idx(
				hevc->mmu_box_1,
				pic->index,
				cur_mmu_4k_number,
				hevc->frame_mmu_map_addr_1);
		if (hevc->dw_mmu_enable) {
			if (ret == 0)
			ret = decoder_mmu_box_alloc_idx(
				hevc->mmu_box_dw,
				pic->index,
				cur_mmu_4k_number,
				hevc->frame_dw_mmu_map_addr);
			if (ret == 0)
			ret = decoder_mmu_box_alloc_idx(
				hevc->mmu_box_dw_1,
				pic->index,
				cur_mmu_4k_number,
				hevc->frame_dw_mmu_map_addr_1);
		}
		if (ret != 0) {
			pr_err("%s: can not alloc mmu\n", __func__);
			return;
		}
	}
	ATRACE_COUNTER(hevc->trace.decode_back_run_time_name, TRACE_RUN_BACK_ALLOC_MMU_END);

	ATRACE_COUNTER(hevc->trace.decode_back_run_time_name, TRACE_RUN_BACK_CONFIGURE_REGISTER_START);
	copy_loopbufs_ptr(&hevc->bk, &hevc->next_bk[hevc->fb_rd_pos]);
	print_loopbufs_ptr(hevc, "bk", &hevc->bk);
#if 1 //def RESET_BACK_PER_PICTURE
	if (hevc->front_back_mode == 1)
		amhevc_reset_b();

	hevc_hw_init(hevc, pic->depth, 0, 1);

	if (hevc->front_back_mode == 3) {
		WRITE_VREG(hevc->backend_ASSIST_MBOX0_IRQ_REG, 1);
		ATRACE_COUNTER(hevc->trace.decode_back_run_time_name, TRACE_RUN_BACK_CONFIGURE_REGISTER_END);
	} else {
		u32 data32 = (((pic->slice_idx + 1) & 0xff) | ((pic->tile_cnt << 8) & 0xff00));

		hevc_print(hevc, PRINT_FLAG_VDEC_STATUS, "%s:data32 = 0x%x, pic->slice_idx %d pic->tile_cnt %d\n",
			__func__, data32, pic->slice_idx, pic->tile_cnt);
		WRITE_VREG(PIC_INFO_DBE, data32);
		WRITE_VREG(PIC_DECODE_COUNT_DBE, hevc->backend_decoded_count);
		WRITE_VREG(HEVC_DEC_STATUS_DBE, HEVC_BE_DECODE_DATA);
		WRITE_VREG(HEVC_SAO_CRC, 0);
		ATRACE_COUNTER(hevc->trace.decode_back_run_time_name, TRACE_RUN_BACK_CONFIGURE_REGISTER_END);
		//pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", READ_VREG(HEVC_MPC_E_DBE));
		//print_reg_flag = 1;
		ATRACE_COUNTER(hevc->trace.decode_back_run_time_name, TRACE_RUN_BACK_FW_START);

		amhevc_start_b();
		ATRACE_COUNTER(hevc->trace.decode_back_run_time_name, TRACE_RUN_BACK_FW_END);
		vdec_profile(hw_to_vdec(hevc), VDEC_PROFILE_DECODER_START, CORE_MASK_HEVC_BACK);
		//pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", READ_VREG(HEVC_MPC_E_DBE));
		//pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", READ_VREG(HEVC_MPC_E_DBE));
		//pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", READ_VREG(HEVC_MPC_E_DBE));
		//pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", READ_VREG(HEVC_MPC_E_DBE));
		//pr_info("[BE] HEVC_MPC_E_DBE=0x%x\n", READ_VREG(HEVC_MPC_E_DBE));
	}
#else
	config_bufstate_back_hw(hevc);
	WRITE_VREG(PIC_DECODE_COUNT_DBE, hevc->backend_decoded_count);
	WRITE_VREG(HEVC_DEC_STATUS_DBE, HEVC_BE_DECODE_DATA);
#endif
}

static void hevc_config_work_space_hw_fb(hevc_stru_t* hevc, uint8_t bit_depth, uint8_t front_flag, uint8_t back_flag)
{
	uint32_t data32;
	struct BuffInfo_s* buf_spec = hevc->work_space_buf;
	int losless_comp_header_size =
		compute_losless_comp_header_size(hevc->pic_w,
		hevc->pic_h);
	int dw_mode = get_double_write_mode(hevc);

	if (front_flag) {
		if ((debug & H265_DEBUG_SEND_PARAM_WITH_REG) == 0)
			WRITE_VREG(HEVC_RPM_BUFFER, (u32)hevc->rpm_phy_addr);
		//WRITE_VREG(HEVC_RPM_BUFFER, (0<<31)|buf_spec->rpm.buf_start); // bit31 - core_id
		WRITE_VREG(LMEM_STORE_ADR, buf_spec->lmem.buf_start);
		WRITE_VREG(HEVC_SHORT_TERM_RPS, buf_spec->short_term_rps.buf_start);
		WRITE_VREG(HEVC_VPS_BUFFER, buf_spec->vps.buf_start);
		WRITE_VREG(HEVC_SPS_BUFFER, buf_spec->sps.buf_start);
		WRITE_VREG(HEVC_PPS_BUFFER, buf_spec->pps.buf_start);

		//WRITE_VREG(HEVC_STREAM_SWAP_BUFFER, buf_spec->swap_buf.buf_start);
		//WRITE_VREG(HEVC_STREAM_SWAP_BUFFER2, buf_spec->swap_buf2.buf_start);

	#ifdef FB_BUF_DEBUG_SYSIMEM_NO_LOOP
		//WRITE_VREG(HEVC_SYS_IMEM_ADR_TMP, buf_spec->system_imem.buf_start);
	#endif
		WRITE_VREG(HEVC_SCALELUT, buf_spec->scalelut.buf_start);
	}

	if (back_flag) {
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE,buf_spec->ipp.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE_DBE1,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2_DBE1,buf_spec->ipp.buf_start);
		WRITE_VREG(HEVC_SAO_UP, buf_spec->sao_up.buf_start);

	#ifdef H265_10B_MMU
			if (dw_mode != 0x10) {
			WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR, hevc->frame_mmu_map_phy_addr);
			hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
				"WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR, 0x%x)\n",
				hevc->frame_mmu_map_phy_addr);
			WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR_DBE1, hevc->frame_mmu_map_phy_addr_1); //new dual
			hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
				"WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR_DBE1, 0x%x)\n",
				hevc->frame_mmu_map_phy_addr_1);
		}
	#else
		//WRITE_VREG(HEVC_STREAM_SWAP_BUFFER, buf_spec->swap_buf.buf_start);
	#endif

		WRITE_VREG(HEVC_DBLK_CFG3, 0x4010); // adp offset(cfg3[23:16]) should be 0
		WRITE_VREG(HEVC_DBLK_CFG3_DBE1, 0x4010); // adp offset(cfg3[23:16]) should be 0
	#ifdef HEVC_8K_LFTOFFSET_FIX
		WRITE_VREG(HEVC_DBLK_CFG3, 0x8020); // offset should x2 if 8k
		WRITE_VREG(HEVC_DBLK_CFG3_DBE1, 0x8020); // offset should x2 if 8k
	#endif
	#ifdef H265_10B_MMU_DW
		//WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR2, FRAME_MMU_MAP_ADDR_DW);
		if (hevc->dw_mmu_enable) {
		WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2, hevc->frame_dw_mmu_map_phy_addr);
		WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2_DBE1, hevc->frame_dw_mmu_map_phy_addr_1); //new dual
		}
		//printk("WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2, 0x%x\n", FRAME_MMU_MAP_ADDR_DW_0);
		//printk("WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2_DBE1, 0x%x\n", FRAME_MMU_MAP_ADDR_DW_1);
	#endif

		WRITE_VREG(HEVC_DBLK_CFG4, buf_spec->dblk_para.buf_start);
		WRITE_VREG(HEVC_DBLK_CFG4_DBE1, buf_spec->dblk_para.buf_start);
		hevc_print(hevc, H265_DEBUG_BUFMGR_MORE, " [DBLK DBG] CFG4: 0x%x\n", buf_spec->dblk_para.buf_start);
		WRITE_VREG(HEVC_DBLK_CFG5, buf_spec->dblk_data.buf_start);
		WRITE_VREG(HEVC_DBLK_CFG5_DBE1, buf_spec->dblk_data.buf_start);
		hevc_print(hevc, H265_DEBUG_BUFMGR_MORE, " [DBLK DBG] CFG5: 0x%x\n", buf_spec->dblk_data.buf_start);
		WRITE_VREG(HEVC_DBLK_CFGE, buf_spec->dblk_data2.buf_start);
		WRITE_VREG(HEVC_DBLK_CFGE_DBE1, buf_spec->dblk_data2.buf_start);
		hevc_print(hevc, H265_DEBUG_BUFMGR_MORE, " [DBLK DBG] CFGE: 0x%x\n", buf_spec->dblk_data2.buf_start);

	#ifdef LOSLESS_COMPRESS_MODE

		data32 = READ_VREG(HEVC_SAO_CTRL5);
		if (bit_depth != 0x00)
		data32 &= ~(1<<9);
		else
		data32 |= (1<<9);

		WRITE_VREG(HEVC_SAO_CTRL5, data32);

		data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
		if (bit_depth != 0x00)
		data32 &= ~(1<<9);
		else
		data32 |= (1<<9);

		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);


	if (dw_mode != 0x10) {
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1,(0x1<< 4)); // bit[4] : paged_mem_mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,0x0);
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1,(0x1<< 4)); // bit[4] : paged_mem_mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1,0x0);
	} else {
		if (bit_depth != 0x00) {
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0<<3)); // bit[3] smem mode
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, (0<<3)); // bit[3] smem mode
		}
		else {
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (1<<3)); // bit[3] smem mdoe
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, (1<<3)); // bit[3] smem mdoe
		}
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,(hevc->losless_comp_body_size >> 5));
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1,(hevc->losless_comp_body_size >> 5));
	}

		//WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,(hevc->losless_comp_body_size >> 5));
		//WRITE_VREG(HEVCD_MPP_DECOMP_CTL3,(0xff<<20) | (0xff<<10) | 0xff); //8-bit mode
		WRITE_VREG(HEVC_CM_BODY_LENGTH,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH,losless_comp_header_size);
		WRITE_VREG(HEVC_CM_BODY_LENGTH_DBE1,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET_DBE1,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH_DBE1,losless_comp_header_size);
		if (dw_mode & 0x10) {
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, 0x1 << 31);
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, 0x1 << 31);
		}
	#else
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1,0x1 << 31);
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1,0x1 << 31);
	#endif
	#ifdef H265_10B_MMU
		if (dw_mode != 0x10) {
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR, buf_spec->mmu_vbh.buf_start);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/4);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR_DBE1, buf_spec->mmu_vbh.buf_start  + buf_spec->mmu_vbh.buf_size/2);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR_DBE1, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/2 + buf_spec->mmu_vbh.buf_size/4);

			data32 = READ_VREG(HEVC_SAO_CTRL9);
			data32 |= 0x1;
			WRITE_VREG(HEVC_SAO_CTRL9, data32);
			data32 = READ_VREG(HEVC_SAO_CTRL9_DBE1);
			data32 |= 0x1;
			WRITE_VREG(HEVC_SAO_CTRL9_DBE1, data32);

			/* use HEVC_CM_HEADER_START_ADDR */
			data32 = READ_VREG(HEVC_SAO_CTRL5);
			data32 |= (1<<10);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
			data32 |= (1<<10);
			WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
		}
	#endif
	#ifdef H265_10B_MMU_DW
	if (hevc->dw_mmu_enable) {
		WRITE_VREG(HEVC_CM_BODY_LENGTH2,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET2,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH2,losless_comp_header_size);
		WRITE_VREG(HEVC_CM_BODY_LENGTH2_DBE1,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET2_DBE1,hevc->losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH2_DBE1,losless_comp_header_size);

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

	#ifdef H265_10B_MMU_DW
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
	}
	if (front_flag) {
	#ifdef CO_MV_COMPRESS
		data32 = READ_VREG(HEVC_MPRED_CTRL4);
		data32 |=  (1<<1);
		WRITE_VREG(HEVC_MPRED_CTRL4, data32);
	#endif
	}
}

static void hevc_init_decoder_hw_fb(hevc_stru_t* hevc, uint8_t front_flag, uint8_t back_flag)
{
	//uint32_t data32;
	int i;
	//int dw_mode = get_double_write_mode(hevc);

	//hevc_print(hevc, H265_DEBUG_BUFMGR, "%s (front_flag %d back_flag %d)\n",
	//    __func__, front_flag, back_flag);
#if 0
	printk("[test.c] Test Parser Register Read/Write\n");
	data32 = READ_VREG(HEVC_PARSER_VERSION);
	if (data32 != 0x00010001) { print_scratch_error(25); return; }
	WRITE_VREG(HEVC_PARSER_VERSION, 0x5a5a55aa);
	data32 = READ_VREG(HEVC_PARSER_VERSION);
	if (data32 != 0x5a5a55aa) { print_scratch_error(26); return; }

	// test Parser Reset
	WRITE_VREG(DOS_SW_RESET1, (1<<3)); // reset_whole parser
	WRITE_VREG(DOS_SW_RESET1, 0); // reset_whole parser
	data32 = READ_VREG(HEVC_PARSER_VERSION);
	if (data32 != 0x00010001) { print_scratch_error(27); return; }
#endif

#if 0 // JT
	if (debug&H265_DEBUG_BUFMGR)
		printk("[test.c] Enable BitStream Fetch\n");
	data32 = READ_VREG(HEVC_STREAM_CONTROL);
	data32 = data32 |
		(1 << 0) // stream_fetch_enable
		;
	WRITE_VREG(HEVC_STREAM_CONTROL, data32);

	data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
	if (data32 != 0x00000100) { print_scratch_error(29); return; }
	data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
	if (data32 != 0x00000300) { print_scratch_error(30); return; }
	WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x12345678);
	WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x9abcdef0);
	data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
	if (data32 != 0x12345678) { print_scratch_error(31); return; }
	data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
	if (data32 != 0x9abcdef0) { print_scratch_error(32); return; }
	WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x00000100);
	WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x00000300);
#endif // JT

#if 0 //def H265_10B_HED_FB
	//if (back_flag) {
		//printk("[test.c] Init H265_10B_HED_FB\n");
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 |
			(3 << 7)  //core0_en, core1_en,hed_fb_en
			;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32);
	//    data32 = READ_VREG(HEVC_ASSIST_FB_W_CTL);
	//    data32 = data32 |
	//             (1 << 0)    // hed_fb_wr_en
			;
	//    WRITE_VREG(HEVC_ASSIST_FB_W_CTL, data32);

	//#ifdef H265_10B_HED_SAME_FB
	//    printk("[test.c] Init H265_10B_HED_SAME_FB\n");
	//    data32 = READ_VREG(HEVC_ASSIST_FB_R_CTL);
	//    data32 = data32 |
	//             (1 << 0)    // hed_fb_rd_en
	//             ;
	//    WRITE_VREG(HEVC_ASSIST_FB_R_CTL, data32);
	//#endif
	//}
#endif

	if (front_flag) {
#if 0 //Move to ucode
		if (efficiency_mode == 0) {
			uint32_t data32;
			//if (debug&H265_DEBUG_BUFMGR)
			//	  printk("[test.c] Enable HEVC Parser Interrupt\n");
			data32 = READ_VREG(HEVC_PARSER_INT_CONTROL);
			data32 = data32 |
				(1 << 24) |  // stream_buffer_empty_int_amrisc_enable
				(1 << 22) |  // stream_fifo_empty_int_amrisc_enable
#if 1 //def H265_10B_HED_FB
				(1 << 10) |  // fed_fb_slice_done_int_cpu_enable
#endif
				(1 << 7) |	// dec_done_int_amrisc_enable
				(1 << 4) |	// startcode_found_int_cpu_enable
				(0 << 3) |	// startcode_found_int_amrisc_enable
				(1 << 0)	// parser_int_enable
				;
			WRITE_VREG(HEVC_PARSER_INT_CONTROL, data32);

			//if (debug&H265_DEBUG_BUFMGR)
			//	  printk("[test.c] Enable HEVC Parser Shift\n");

			data32 = READ_VREG(HEVC_SHIFT_STATUS);
			data32 = data32 |
				(1 << 1) |	// emulation_check_on
				(1 << 0)	// startcode_check_on
				;
			WRITE_VREG(HEVC_SHIFT_STATUS, data32);

			WRITE_VREG(HEVC_SHIFT_CONTROL,
				(0 << 14) | // disable_start_code_protect
				(3 << 6) | // sft_valid_wr_position
				(2 << 4) | // emulate_code_length_sub_1
				(2 << 1) | // start_code_length_sub_1
				(1 << 0)   // stream_shift_enable
				);

			WRITE_VREG(HEVC_CABAC_CONTROL,
				(1 << 0)   // cabac_enable
				);

			/* not used now
			WRITE_VREG(HEVC_PARSER_CORE_CONTROL,
				(1 << 0)   // hevc_parser_core_clk_en
				);
			*/
		}
#endif
		WRITE_VREG(HEVC_DEC_STATUS_REG, 0);

#if 0
		// Initial IQIT_SCALELUT memory -- just to avoid X in simulation
		if (debug&H265_DEBUG_BUFMGR)
		printk("[test.c] Initial IQIT_SCALELUT memory -- just to avoid X in simulation...\n");
		WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0); // cfg_p_addr
		for (i=0; i<1024; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, 0); //only for simulation
#endif
#if 0
// for real chip
	#ifdef ENABLE_SWAP_TEST
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 100);
	#else
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 0);
	#endif
		WRITE_VREG(HEVC_DECODE_PIC_BEGIN_REG, 0);
		//WRITE_VREG(HEVC_DECODE_PIC_NUM_REG, decode_pic_num);
#endif

#if 0
		// Send parser_cmd
		//if (debug) printk("[test.c] SEND Parser Command ...\n");
		WRITE_VREG(HEVC_PARSER_CMD_WRITE, (1<<16) | (0<<0));
#if 1
//for real chip
		parser_cmd_write();
#else
		for (i=0; i<PARSER_CMD_NUMBER; i++) {
		WRITE_VREG(HEVC_PARSER_CMD_WRITE, parser_cmd[i]);
		}
#endif
		WRITE_VREG(HEVC_PARSER_CMD_SKIP_0, PARSER_CMD_SKIP_CFG_0);
		WRITE_VREG(HEVC_PARSER_CMD_SKIP_1, PARSER_CMD_SKIP_CFG_1);
		WRITE_VREG(HEVC_PARSER_CMD_SKIP_2, PARSER_CMD_SKIP_CFG_2);
#endif
#if 0 //Move to ucode
		if (efficiency_mode == 0) {
		   WRITE_VREG(HEVC_PARSER_IF_CONTROL,
			   //  (1 << 8) | // sao_sw_pred_enable
			   (1 << 5) | // parser_sao_if_en
			   (1 << 2) | // parser_mpred_if_en
			   (1 << 0) // parser_scaler_if_en
			   );

		   // Changed to Start MPRED in microcode
		   /*
		   printk("[test.c] Start MPRED\n");
		   WRITE_VREG(HEVC_MPRED_INT_STATUS,
			   (1<<31)
		   );*/
		}
#endif
	}
	if (back_flag) {
#if 1
//only for simulation
		// Initial IQIT_SCALELUT memory -- just to avoid X in simulation
		//if (debug&H265_DEBUG_BUFMGR)
		//    printk("[test.c] Initial IQIT_SCALELUT memory -- just to avoid X in simulation...\n");
		WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0); // cfg_p_addr
		for (i=0; i<1024; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, 0); //only for simulation
#endif

		//if (debug) printk("[test.c] Reset IPP\n");
		WRITE_VREG(HEVCD_IPP_TOP_CNTL,
			(0 << 1) | // enable ipp
			(1 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL,
			(1 << 1) | // enable ipp
			(0 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
			(0 << 1) | // enable ipp
			(1 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
			(1 << 1) | // enable ipp
			(0 << 0)   // software reset ipp and mpp
			);

	}

	if (front_flag && back_flag) {
		// Initialize mcrcc and decomp perf counters
#if 0
//to do
		mcrcc_perfcount_reset();
		decomp_perfcount_reset();
#endif
	}
	//printk("[test.c] Leaving hevc_init_decoder_hw\n");
	return;
}

static int32_t hevc_hw_init(hevc_stru_t* hevc, uint8_t bit_depth, uint8_t front_flag, uint8_t back_flag)
{
	uint32_t data32 = 0;
	uint32_t tmp = 0;

	hevc_print(hevc, H265_DEBUG_BUFMGR,
		"%s front_flag %d back_flag %d\n", __func__, front_flag, back_flag);
	if (hevc->front_back_mode != 1) {
		if (front_flag)
		vh265_hw_ctx_restore(hevc);
		if (back_flag) {
		/* clear mailbox interrupt */
		WRITE_VREG(hevc->backend_ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(hevc->backend_ASSIST_MBOX0_MASK, 1);

		}
		return 0;
	}

	if (front_flag) {
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 | (3 << 7);
		tmp = (1 << 0) | (1 << 1) | (1 << 9) | (1 << 10);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32);
	}

	if (back_flag) {
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 | (3 << 7);
		tmp = (1 << 2) | (1 << 3) |(1 << 13) | (1 << 11) | (1 << 5) | (1 << 6) | (1 << 14) | (1 << 12);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32);
	}

	hevc_config_work_space_hw_fb(hevc , bit_depth, front_flag, back_flag);
	if (front_flag)
		config_bufstate_front_hw(hevc);
	if (back_flag)
		config_bufstate_back_hw(hevc);

	hevc_init_decoder_hw_fb(hevc, front_flag, back_flag);

	if (back_flag) {
	// Set MCR fetch priorities
		data32 = 0x1 | (0x1 << 2) | (0x1 <<3) | (24 << 4) | (32 << 11) | (24 << 18) | (32 << 25);
		WRITE_VREG(HEVCD_MPP_DECOMP_AXIURG_CTL, data32);
		WRITE_VREG(HEVCD_MPP_DECOMP_AXIURG_CTL_DBE1, data32);

	// Set IPP MULTICORE CFG
		WRITE_VREG(HEVCD_IPP_MULTICORE_CFG, 1);
		WRITE_VREG(HEVCD_IPP_MULTICORE_CFG_DBE1, 1);

#ifdef DYN_CACHE
		if (hevc->front_back_mode == 1) {
		//printk("HEVC DYN MCRCC\n");
		WRITE_VREG(HEVCD_IPP_DYN_CACHE,0x2b);//enable new mcrcc
		WRITE_VREG(HEVCD_IPP_DYN_CACHE_DBE1,0x2b);//enable new mcrcc
		}
#endif
	}
	if (front_flag) {
		//if (debug&H265_DEBUG_BUFMGR)
		//    printk("[test.c] Enable BitStream Fetch\n");

	//WRITE_VREG(HEVC_STREAM_START_ADDR, STREAM_BUFFER_DDR_START);
	//WRITE_VREG(HEVC_STREAM_END_ADDR, STREAM_BUFFER_DDR_END);
	//WRITE_VREG(HEVC_STREAM_WR_PTR, STREAM_BUFFER_DDR_END - 8);
	//WRITE_VREG(HEVC_STREAM_RD_PTR, STREAM_BUFFER_DDR_START);

		data32 = READ_VREG(HEVC_STREAM_CONTROL);
		data32 = data32 | (0xf << 25); // arwlen_axi_max
		WRITE_VREG(HEVC_STREAM_CONTROL, data32);
#if 0
		data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
		if (data32 != 0x00000100) { print_scratch_error(29); return -1; }
		data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
		if (data32 != 0x00000300) { print_scratch_error(30); return -1; }
		WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x12345678);
		WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x9abcdef0);
		data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
		if (data32 != 0x12345678) { print_scratch_error(31); return -1; }
		data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
		if (data32 != 0x9abcdef0) { print_scratch_error(32); return -1; }
#endif

#if 0 //Move to ucode
		if (efficiency_mode == 0) {
			WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x00000100);
			WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x00000300);

#if 0 //def SIMULATION
			if (decode_pic_begin == 0)
			WRITE_VREG(HEVC_WAIT_FLAG, 1);
			else
			WRITE_VREG(HEVC_WAIT_FLAG, 0);
#else
			WRITE_VREG(HEVC_WAIT_FLAG, 1);
#endif
		}
#endif
		/* disable PSCALE for hardware sharing */
	#ifdef DOS_PROJECT
	#else
		//WRITE_VREG(HEVC_PSCALE_CTRL, 0);
	#endif
		/* clear mailbox interrupt */
		WRITE_VREG(hevc->ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(hevc->ASSIST_MBOX0_MASK, 1);
#if 0 //Move to ucode
		if (efficiency_mode == 0) {
			WRITE_VREG(DEBUG_REG1, 0x0);  //no debug
		}
#endif
		WRITE_VREG(NAL_SEARCH_CTL, 0x8); //check vps/sps/pps/i-slice in ucode
		WRITE_VREG(DECODE_STOP_POS, udebug_flag);
	}
	if (back_flag) {
		/* clear mailbox interrupt */
		WRITE_VREG(hevc->backend_ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(hevc->backend_ASSIST_MBOX0_MASK, 1);
	}
#if 0
	//WRITE_VREG(XIF_DOS_SCRATCH31, 0x0);
	WRITE_VREG(HEVC_MPSR, 1);
	printk("Enable HEVC Front End MPSR\n");
	WRITE_VREG(HEVC_MPSR_DBE, 1);
	printk("Enable HEVC Back End MPSR\n");
#endif
	return 0;
}

#ifdef MCRCC_ENABLE
//static void  config_mcrcc_axi_hw (int32_t slice_type)
static void  config_mcrcc_axi_hw_fb(hevc_stru_t* hevc)
{
	int32_t slice_type;
	//uint32_t rdata32;
	//uint32_t rdata32_2;
	slice_type = hevc->cur_pic->slice_type;
	hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		"%s\n", __func__);
	//WRITE_VREG_V(P_HEVCD_MCRCC_CTL1, 0x2); // reset mcrcc
	WRITE_BACK_8(hevc, HEVCD_MCRCC_CTL1, 0x2); // reset mcrcc

	if ( slice_type  == 2 ) { // I-PIC
		//WRITE_VREG_V(P_HEVCD_MCRCC_CTL1, 0x0); // remove reset -- disables clock
		WRITE_BACK_8(hevc, HEVCD_MCRCC_CTL1, 0x0); // remove reset -- disables clock
		return;
	}

#if 0
if (hevc->new_pic) {
	mcrcc_get_hitrate();
	decomp_get_hitrate();
	decomp_get_comprate();
	mcrcc_perfcount_reset();
	decomp_perfcount_reset();
}
#endif

	if ( slice_type == 0 ) {  // B-PIC
		// Programme canvas0
		//WRITE_VREG_V(P_HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 0);  //mtspi COMMON_REG_0, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR
		//rdata32 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);                     //mfsp COMMON_REG_0, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		//rdata32 = rdata32 & 0xffff;
		//rdata32 = rdata32 | ( rdata32 << 16);                                      //ins COMMON_REG_0, COMMON_REG_0, 16, 16
		//WRITE_VREG_V(P_HEVCD_MCRCC_CTL2, rdata32);                                   //mtsp COMMON_REG_0, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		WRITE_BACK_8(hevc, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 0);
		READ_INS_WRITE(hevc, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL2, 0, 16, 16);

		// Programme canvas1
		//WRITE_VREG_V(P_HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (16 << 8) | (1<<1) | 0);    //movi COMMON_REG_1, (16 << 8) | (1<<1) | 0
		WRITE_BACK_16(hevc, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 1, (16 << 8) | (1<<1) | 0);  //mtsp COMMON_REG_1, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		//rdata32_2 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);                    //mfsp COMMON_REG_1, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		//rdata32_2 = rdata32_2 & 0xffff;
		//rdata32_2 = rdata32_2 | ( rdata32_2 << 16);                                 //ins COMMON_REG_1, COMMON_REG_1, 16, 16
		//if ( rdata32 == rdata32_2 ) {                                                //cbne current_pc+4, COMMON_REG_0, COMMON_REG_1  //nop
		//    rdata32_2 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);                //mfsp COMMON_REG_1, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		//    rdata32_2 = rdata32_2 & 0xffff;
		//    rdata32_2 = rdata32_2 | ( rdata32_2 << 16);                             //ins COMMON_REG_1, COMMON_REG_1, 16, 16
		//}
		//WRITE_VREG_V(P_HEVCD_MCRCC_CTL3, rdata32_2);                                  //mtsp COMMON_REG_1, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		READ_CMP_WRITE(hevc, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL3, 1, 0, 16, 16);

	} else { // P-PIC
		//WRITE_VREG_V(P_HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (1<<1) | 0);
	WRITE_BACK_8(hevc, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 0);
		//rdata32 = READ_VREG_V(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);                       //mfsp COMMON_REG_0, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		//rdata32 = rdata32 & 0xffff;
		//rdata32 = rdata32 | ( rdata32 << 16);                                      //ins COMMON_REG_0, COMMON_REG_0, 16, 16
		//WRITE_VREG_V(P_HEVCD_MCRCC_CTL2, rdata32);                                   //mtsp COMMON_REG_0, HEVCD_MCRCC_CTL2
	READ_INS_WRITE(hevc, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL2, 0, 16, 16);

		// Programme canvas1
		//rdata32 = READ_VREG_V(P_HEVCD_MPP_ANC_CANVAS_DATA_ADDR);                     //mfsp COMMON_REG_0, HEVCD_MPP_ANC_CANVAS_DATA_ADDR
		//rdata32 = rdata32 & 0xffff;
		//rdata32 = rdata32 | ( rdata32 << 16);                                      //ins COMMON_REG_0, COMMON_REG_0, 16, 16
		//WRITE_VREG_V(P_HEVCD_MCRCC_CTL3, rdata32);                                   //mtsp COMMON_REG_0, HEVCD_MCRCC_CTL2
		READ_INS_WRITE(hevc, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL3, 0, 16, 16);
	}

	//WRITE_VREG_V(P_HEVCD_MCRCC_CTL1, 0xff0); // enable mcrcc progressive-mode
	WRITE_BACK_16(hevc, HEVCD_MCRCC_CTL1, 0, 0xff0); // enable mcrcc progressive-mode
	return;
}

#endif // MCRCC_ENABLE

static void  config_title_hw_fb(hevc_stru_t* hevc, int32_t sao_vb_size, int32_t sao_mem_unit)
{
	PRINT_LINE();
	WRITE_BACK_32(hevc,HEVC_sao_mem_unit, sao_mem_unit);
	WRITE_BACK_32(hevc,HEVC_SAO_ABV, hevc->work_space_buf->sao_abv.buf_start);
	WRITE_BACK_32(hevc,HEVC_sao_vb_size, sao_vb_size);
	WRITE_BACK_32(hevc,HEVC_SAO_VB, hevc->work_space_buf->sao_vb.buf_start);
}

//static
void config_mpred_hw_fb(hevc_stru_t* hevc)
{
	int32_t i;
	uint32_t data32;
	PIC_t* cur_pic = hevc->cur_pic;
	PIC_t* col_pic = hevc->col_pic;
	int32_t     AMVP_MAX_NUM_CANDS_MEM=3;
	int32_t     AMVP_MAX_NUM_CANDS=2;
	int32_t     NUM_CHROMA_MODE=5;
	int32_t     DM_CHROMA_IDX=36;
	int32_t     above_ptr_ctrl =0;
	int32_t     buffer_linear =1;
	int32_t     cu_size_log2 =3;

	int32_t     mpred_mv_rd_start_addr ;
	//int32_t     mpred_curr_lcu_x;
	//int32_t     mpred_curr_lcu_y;
	int32_t     mpred_above_buf_start ;
	int32_t     mpred_mv_rd_ptr ;
	int32_t     mpred_mv_rd_ptr_p1 ;
	int32_t     mpred_mv_rd_end_addr;
	int32_t     MV_MEM_UNIT;
	int32_t     mpred_mv_wr_ptr ;
	int32_t     *ref_poc_L0, *ref_poc_L1;

	int32_t     above_en;
	int32_t     mv_wr_en;
	int32_t     mv_rd_en;
	int32_t     col_isIntra;

	if (hevc->slice_type != 2)
	{
		above_en=1;
		mv_wr_en=1;
		mv_rd_en=1;
		col_isIntra=0;
	}
	else
	{
		above_en=1;
		mv_wr_en=1;
		mv_rd_en=0;
		col_isIntra=0;
	}
#ifdef SAVE_NON_REF
	if (NON_REF_B) mv_wr_en = 0;
#endif

	mpred_mv_rd_start_addr=col_pic->mpred_mv_wr_start_addr;
	/*data32 = READ_VREG(HEVC_MPRED_CURR_LCU);
	mpred_curr_lcu_x   =data32 & 0xffff;
	mpred_curr_lcu_y   =(data32>>16) & 0xffff;*/

	MV_MEM_UNIT=hevc->lcu_size_log2 == 6 ? 0x200 : hevc->lcu_size_log2 == 5 ? 0x80 : 0x20;
	mpred_mv_rd_ptr = mpred_mv_rd_start_addr  + (hevc->slice_addr*MV_MEM_UNIT);

	mpred_mv_rd_ptr_p1  =mpred_mv_rd_ptr+MV_MEM_UNIT;
	mpred_mv_rd_end_addr=mpred_mv_rd_start_addr + ((hevc->lcu_x_num*hevc->lcu_y_num)*MV_MEM_UNIT);

	mpred_above_buf_start = hevc->work_space_buf->mpred_above.buf_start;

	mpred_mv_wr_ptr = cur_pic->mpred_mv_wr_start_addr  + (hevc->slice_addr*MV_MEM_UNIT);

	if (debug&H265_DEBUG_BUFMGR)
		printk("%s cur pic index %d  col pic index %d\n",
		__func__, cur_pic->index, col_pic->index);

	WRITE_VREG(HEVC_MPRED_MV_WR_START_ADDR,cur_pic->mpred_mv_wr_start_addr);
	WRITE_VREG(HEVC_MPRED_MV_RD_START_ADDR,mpred_mv_rd_start_addr);
	hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		"[MPRED CO_MV] write 0x%x  read 0x%x -- 0x%X\n", cur_pic->mpred_mv_wr_start_addr, mpred_mv_rd_start_addr, col_pic);

	data32 = ((hevc->lcu_x_num - hevc->tile_width_lcu)*MV_MEM_UNIT);
	WRITE_VREG(HEVC_MPRED_MV_WR_ROW_JUMP,data32);
	WRITE_VREG(HEVC_MPRED_MV_RD_ROW_JUMP,data32);

	data32 = READ_VREG(HEVC_MPRED_CTRL0);
	data32  =   (
		hevc->slice_type |
		hevc->new_pic<<2 |
		hevc->new_tile<<3|
		hevc->isNextSliceSegment<<4|
		hevc->TMVPFlag<<5|
		hevc->LDCFlag<<6|
		hevc->ColFromL0Flag<<7|
		above_ptr_ctrl<<8 |
		above_en<<9|
		mv_wr_en<<10|
		mv_rd_en<<11|
		col_isIntra<<12|
		buffer_linear<<13|
		hevc->LongTerm_Curr<<14|
		hevc->LongTerm_Col<<15|
		hevc->lcu_size_log2<<16|
		cu_size_log2<<20|
		hevc->plevel<<24
		);
	WRITE_VREG(HEVC_MPRED_CTRL0,data32);

	data32 = READ_VREG(HEVC_MPRED_CTRL1);
	data32  =   (
#ifdef DOS_PROJECT
//no set in m8baby test1902
	(data32 & (0x1<<24)) |  // Don't override clk_forced_on ,
#endif
		hevc->MaxNumMergeCand |
		AMVP_MAX_NUM_CANDS<<4 |
		AMVP_MAX_NUM_CANDS_MEM<<8|
		NUM_CHROMA_MODE<<12|
		DM_CHROMA_IDX<<16
		);
	WRITE_VREG(HEVC_MPRED_CTRL1,data32);

	data32  =   (
		hevc->pic_w|
		hevc->pic_h<<16
		);
	WRITE_VREG(HEVC_MPRED_PIC_SIZE,data32);

	data32  =   (
		(hevc->lcu_x_num-1)   |
		(hevc->lcu_y_num-1)<<16
		);
	WRITE_VREG(HEVC_MPRED_PIC_SIZE_LCU,data32);

	data32  =   (
		hevc->tile_start_lcu_x   |
		hevc->tile_start_lcu_y<<16
		);
	WRITE_VREG(HEVC_MPRED_TILE_START,data32);

	data32  =   (
		hevc->tile_width_lcu   |
		hevc->tile_height_lcu<<16
		);
	WRITE_VREG(HEVC_MPRED_TILE_SIZE_LCU,data32);

	data32  =   (
		hevc->RefNum_L0   |
		hevc->RefNum_L1<<8|
		0
		//col_RefNum_L0<<16|
		//col_RefNum_L1<<24
		);
	WRITE_VREG(HEVC_MPRED_REF_NUM,data32);

#ifdef SUPPORT_LONG_TERM_RPS
	data32 = 0;
	for (i = 0; i < hevc->RefNum_L0; i++) {
		if (is_ref_long_term(hevc,
		cur_pic->m_aiRefPOCList0
			[cur_pic->slice_idx][i]))
		data32 = data32 | (1 << i);
	}
	for (i = 0; i < hevc->RefNum_L1; i++) {
		if (is_ref_long_term(hevc,
		cur_pic->m_aiRefPOCList1
			[cur_pic->slice_idx][i]))
		data32 = data32 | (1 << (i + 16));
	}
	hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		"LongTerm_Ref 0x%x\n", data32);
#else
	data32  =   (
		hevc->LongTerm_Ref
		);
#endif
	WRITE_VREG(HEVC_MPRED_LT_REF,data32);

	data32=0;
	for (i=0;i<hevc->RefNum_L0;i++)data32=data32|(1<<i);
	WRITE_VREG(HEVC_MPRED_REF_EN_L0,data32);

	data32=0;
	for (i=0;i<hevc->RefNum_L1;i++)data32=data32|(1<<i);
	WRITE_VREG(HEVC_MPRED_REF_EN_L1,data32);

	WRITE_VREG(HEVC_MPRED_CUR_POC,hevc->curr_POC);
	WRITE_VREG(HEVC_MPRED_COL_POC,hevc->Col_POC);

	//below MPRED Ref_POC_xx_Lx registers must follow Ref_POC_xx_L0 -> Ref_POC_xx_L1 in pair write order!!!
	ref_poc_L0      = &(cur_pic->m_aiRefPOCList0[cur_pic->slice_idx][0]);
	ref_poc_L1      = &(cur_pic->m_aiRefPOCList1[cur_pic->slice_idx][0]);

	WRITE_VREG(HEVC_MPRED_L0_REF00_POC,ref_poc_L0[0]);
	WRITE_VREG(HEVC_MPRED_L1_REF00_POC,ref_poc_L1[0]);

	WRITE_VREG(HEVC_MPRED_L0_REF01_POC,ref_poc_L0[1]);
	WRITE_VREG(HEVC_MPRED_L1_REF01_POC,ref_poc_L1[1]);

	WRITE_VREG(HEVC_MPRED_L0_REF02_POC,ref_poc_L0[2]);
	WRITE_VREG(HEVC_MPRED_L1_REF02_POC,ref_poc_L1[2]);

	WRITE_VREG(HEVC_MPRED_L0_REF03_POC,ref_poc_L0[3]);
	WRITE_VREG(HEVC_MPRED_L1_REF03_POC,ref_poc_L1[3]);

	WRITE_VREG(HEVC_MPRED_L0_REF04_POC,ref_poc_L0[4]);
	WRITE_VREG(HEVC_MPRED_L1_REF04_POC,ref_poc_L1[4]);

	WRITE_VREG(HEVC_MPRED_L0_REF05_POC,ref_poc_L0[5]);
	WRITE_VREG(HEVC_MPRED_L1_REF05_POC,ref_poc_L1[5]);

	WRITE_VREG(HEVC_MPRED_L0_REF06_POC,ref_poc_L0[6]);
	WRITE_VREG(HEVC_MPRED_L1_REF06_POC,ref_poc_L1[6]);

	WRITE_VREG(HEVC_MPRED_L0_REF07_POC,ref_poc_L0[7]);
	WRITE_VREG(HEVC_MPRED_L1_REF07_POC,ref_poc_L1[7]);

	WRITE_VREG(HEVC_MPRED_L0_REF08_POC,ref_poc_L0[8]);
	WRITE_VREG(HEVC_MPRED_L1_REF08_POC,ref_poc_L1[8]);

	WRITE_VREG(HEVC_MPRED_L0_REF09_POC,ref_poc_L0[9]);
	WRITE_VREG(HEVC_MPRED_L1_REF09_POC,ref_poc_L1[9]);

	WRITE_VREG(HEVC_MPRED_L0_REF10_POC,ref_poc_L0[10]);
	WRITE_VREG(HEVC_MPRED_L1_REF10_POC,ref_poc_L1[10]);

	WRITE_VREG(HEVC_MPRED_L0_REF11_POC,ref_poc_L0[11]);
	WRITE_VREG(HEVC_MPRED_L1_REF11_POC,ref_poc_L1[11]);

	WRITE_VREG(HEVC_MPRED_L0_REF12_POC,ref_poc_L0[12]);
	WRITE_VREG(HEVC_MPRED_L1_REF12_POC,ref_poc_L1[12]);

	WRITE_VREG(HEVC_MPRED_L0_REF13_POC,ref_poc_L0[13]);
	WRITE_VREG(HEVC_MPRED_L1_REF13_POC,ref_poc_L1[13]);

	WRITE_VREG(HEVC_MPRED_L0_REF14_POC,ref_poc_L0[14]);
	WRITE_VREG(HEVC_MPRED_L1_REF14_POC,ref_poc_L1[14]);

	WRITE_VREG(HEVC_MPRED_L0_REF15_POC,ref_poc_L0[15]);
	WRITE_VREG(HEVC_MPRED_L1_REF15_POC,ref_poc_L1[15]);

	if (hevc->new_pic)
	{
		WRITE_VREG(HEVC_MPRED_ABV_START_ADDR,mpred_above_buf_start);
		WRITE_VREG(HEVC_MPRED_MV_WPTR,mpred_mv_wr_ptr);
		//WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_ptr);
		WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_start_addr);
	}
	else if (!hevc->isNextSliceSegment)
	{
		//WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_ptr_p1);
		WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_ptr);
	}

	WRITE_VREG(HEVC_MPRED_MV_RD_END_ADDR,mpred_mv_rd_end_addr);
}

static void config_dw_fb(hevc_stru_t* hevc, PIC_t* pic, u32 mc_buffer_size_u_v_h)
{

	int dw_mode = get_double_write_mode(hevc);
	uint32_t data = 0, data32;
	if ((dw_mode & 0x10) == 0) {
		WRITE_BACK_8(hevc, HEVC_SAO_CTRL26, 0);

		//data32 = READ_VREG(HEVC_SAO_CTRL5);
		//data32 &= (~(0xff << 16));
		if (((dw_mode & 0xf) == 8) ||
		((dw_mode & 0xf) == 9)) {
		data = 0xff; //data32 |= (0xff << 16);
		//WRITE_VREG(HEVC_SAO_CTRL5, data32);
		WRITE_BACK_8(hevc, HEVC_SAO_CTRL26, 0xf);
		} else {
		if ((dw_mode & 0xf) == 2 ||
			(dw_mode & 0xf) == 3)
			data = 0xff; //data32 |= (0xff<<16);
		else if ((dw_mode & 0xf) == 4 ||
			(dw_mode & 0xf) == 5)
			data = 0x33; //data32 |= (0x33<<16);

		if (hevc->mem_saving_mode == 1)
			READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 1, 9, 1); //data32 |= (1 << 9);
		else
			READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 0, 9, 1); //data32 &= ~(1 << 9);
		if (workaround_enable & 1)
			READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 1, 7, 1); //data32 |= (1 << 7);
		//WRITE_VREG(HEVC_SAO_CTRL5, data32);
		}
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, data, 16, 8);
	}

		/* m8baby test1902 */
		//data32 = READ_VREG(HEVC_SAO_CTRL1);
		//data32 &= (~0x3000);
		/* [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32 */
		//data32 |= (hevc->mem_map_mode << 12);
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL1, hevc->mem_map_mode, 12, 2);
		//data32 &= (~0xff0);
#ifdef H265_10B_MMU_DW
		if (hevc->dw_mmu_enable == 0)
		data = ((hevc->endian >> 8) & 0xfff); //endian: ((0x880 << 8) | 0x8) or ((0xff0 << 8) | 0xf)
#else
		data = ((hevc->endian >> 8) & 0xfff);    /* data32 |= 0x670; Big-Endian per 64-bit */
#endif
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL1, data, 0, 12);

		//data32 &= (~0x3); /*[1]:dw_disable [0]:cm_disable*/
		if (dw_mode == 0)
		data = 0x2; //data32 |= 0x2; /*disable double write*/
		else if (dw_mode & 0x10)
		data = 0x1; //data32 |= 0x1; /*disable cm*/
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL1, data, 0, 2);

		//data32 &= (~(3 << 14));
		//data32 |= (2 << 14);
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
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL1, 2, 14, 2);
		//WRITE_VREG(HEVC_SAO_CTRL1, data32);

		if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_G12A) {
			unsigned int data;
			data = (0x57 << 8) |  /* 1st/2nd write both enable*/
			(0x0  << 0);   /* h265 video format*/
			if (hevc->pic_w >= 1280)
			data |= (0x1 << 4); /*dblk pipeline mode=1 for performance*/
			data &= (~0x300); /*[8]:first write enable (compress)  [9]:double write enable (uncompress)*/
			if (dw_mode == 0)
			data |= (0x1 << 8); /*enable first write*/
			else if (dw_mode & 0x10)
			data |= (0x1 << 9); /*double write only*/
			else
			data |= ((0x1 << 8)  |(0x1 << 9));
			WRITE_BACK_32(hevc, HEVC_DBLK_CFGB, data);
			//WRITE_VREG(HEVC_DBLK_CFGB, data);
		}

		if (dw_mode & 0x10) {
		/* [23:22] dw_v1_ctrl
		*[21:20] dw_v0_ctrl
		*[19:18] dw_h1_ctrl
		*[17:16] dw_h0_ctrl
		*/
		//data32 = READ_VREG(HEVC_SAO_CTRL5);
		/*set them all 0 for H265_NV21 (no down-scale)*/
		//data32 &= ~(0xff << 16);
		//WRITE_VREG(HEVC_SAO_CTRL5, data32);
		READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 0, 16, 8);
		}

#ifdef LOSLESS_COMPRESS_MODE
/*SUPPORT_10BIT*/

	data32 = pic->mc_y_adr;
	if (dw_mode && ((dw_mode & 0x20) == 0)) {
		WRITE_BACK_32(hevc, HEVC_SAO_Y_START_ADDR, pic->dw_y_adr);
		WRITE_BACK_32(hevc, HEVC_SAO_C_START_ADDR, pic->dw_u_v_adr);
		WRITE_BACK_32(hevc, HEVC_SAO_Y_WPTR, pic->dw_y_adr);
		WRITE_BACK_32(hevc, HEVC_SAO_C_WPTR, pic->dw_u_v_adr);
	} else {
		WRITE_BACK_32(hevc, HEVC_SAO_Y_START_ADDR, 0xffffffff);
		WRITE_BACK_32(hevc, HEVC_SAO_C_START_ADDR, 0xffffffff);
	}
	if ((dw_mode & 0x10) == 0)
		WRITE_BACK_32(hevc, HEVC_CM_BODY_START_ADDR, data32);

	if (hevc->mmu_enable)
		WRITE_BACK_32(hevc, HEVC_CM_HEADER_START_ADDR, pic->header_adr);
#ifdef H265_10B_MMU_DW
	if (hevc->dw_mmu_enable) {
		WRITE_BACK_32(hevc, HEVC_SAO_Y_START_ADDR, 0);
		WRITE_BACK_32(hevc, HEVC_SAO_C_START_ADDR, 0);
		WRITE_BACK_32(hevc, HEVC_CM_HEADER_START_ADDR2, pic->header_dw_adr);
	}
#endif
#else
	WRITE_BACK_32(hevc, HEVC_SAO_Y_START_ADDR, pic->mc_y_adr);
	WRITE_BACK_32(hevc, HEVC_SAO_C_START_ADDR, pic->mc_u_v_adr);
	WRITE_BACK_32(hevc, HEVC_SAO_Y_WPTR, pic->mc_y_adr);
	WRITE_BACK_32(hevc, HEVC_SAO_C_WPTR, pic->mc_u_v_adr);
#endif
	data32 = (mc_buffer_size_u_v_h << 16) << 1;
	WRITE_BACK_32(hevc, HEVC_SAO_Y_LENGTH, data32);

	data32 = (mc_buffer_size_u_v_h << 16);
	WRITE_BACK_32(hevc, HEVC_SAO_C_LENGTH, data32);

}

static void config_sao_hw_fb(hevc_stru_t* hevc, param_t* params)
{
	uint32_t data32, data32_2;
	int32_t misc_flag0 = hevc->misc_flag0;
	int32_t slice_deblocking_filter_disabled_flag = 0;

	int32_t mc_buffer_size_u_v = hevc->lcu_total*hevc->lcu_size*hevc->lcu_size/2;
	int32_t mc_buffer_size_u_v_h = (mc_buffer_size_u_v + 0xffff)>>16;
	PIC_t* cur_pic = hevc->cur_pic;
	//int dw_mode = get_double_write_mode(hevc);
	hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		"%s config_sao_hw misc_flag0 : 0x%x\n", __func__, misc_flag0);

	config_dw_fb(hevc, cur_pic, mc_buffer_size_u_v_h);
#if 0 //def DW_NO_SCALE
	//WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) );
	READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL5, 0, 16, 8);
#endif
#ifdef DIFF_DW
/*
	if (READ_VREG(HEVC_SAO_CTRL9) & (1<<10) == 0) { // dw cm_mode : 0=old compress mode, 1=mmu mode
	if (frame_width > 4096) {
	WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) | (0x33<<16));
	}
	else if (frame_width > 1920) {
	WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) | (0xff<<16));
	}
	else if (frame_width > 1280) {
	WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) | (0x33<<16));
	}
	else {
	WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) | (0x00<<16));
	//WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) | (0x33<<16));
	//WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) | (0xff<<16));
	}
	} // only NV21 DW
*/
	if (frame_width > 4096)
	data32 = 0x33;
	else if (frame_width > 1920)
	data32 = 0xff;
	else if (frame_width > 1280)
	data32 = 0x33;
	else
	data32 = 0;
	hevc->instruction[hevc->ins_offset] = (0x1a<<22) | ((data32&0xff)<<6);                    //movi COMMON_REG_0, data32
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	//if (READ_VREG(HEVC_SAO_CTRL9) & (1<<10) == 0) {
	//  WRITE_VREG( HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) & ~(0xff<<16) | (data32<<16));
	//}
#if 0
	READ_BACK_32(HEVC_SAO_CTRL9, 1);                                              //mfsp COMMON_REG_1, HEVC_SAO_CTRL9
	hevc->instruction[hevc->ins_offset] = 0x9141041;                                          //ext  COMMON_REG_1, COMMON_REG_1, 10, 1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0xaa14041;                                          //cbeq next_pc+5, COMMON_REG_1, 1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x0000000;                                          //nop
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x6462301;                                          //mfsp COMMON_REG_1, HEVC_SAO_CTRL5
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x9608040;                                          //ins  COMMON_REG_1, COMMON_REG_0, 16, 8
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x6062301;                                          //mtsp COMMON_REG_1, HEVC_SAO_CTRL5
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
#endif
#endif

	//data32 = READ_VREG(HEVC_SAO_CTRL0);                                     //mfsp C0OMMON_REG_0, HEVC_SAO_CTRL0
	//data32 &= (~0xf);                                                         //movi COMMON_REG_1, hevc->lcu_size_log2
	//data32 |= hevc->lcu_size_log2;                                            //ins COMMON_REG_0, COMMON_REG_1, 0, 4
	//WRITE_VREG(HEVC_SAO_CTRL0, data32);                                     //mtsp COMMON_REG_0, HEVC_SAO_CTRL0
	READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL0, hevc->lcu_size_log2, 0, 4);

	data32  =   (
		hevc->pic_w|
		hevc->pic_h<<16
		);
	//WRITE_VREG(HEVC_SAO_PIC_SIZE , data32);
	WRITE_BACK_32(hevc,HEVC_SAO_PIC_SIZE, data32);

	data32  =   (
		(hevc->lcu_x_num-1)   |
		(hevc->lcu_y_num-1)<<16
		);
	//WRITE_VREG(HEVC_SAO_PIC_SIZE_LCU , data32);
	WRITE_BACK_32(hevc,HEVC_SAO_PIC_SIZE_LCU, data32);

	if (hevc->new_pic) {
	//WRITE_VREG(HEVC_SAO_Y_START_ADDR,0xffffffff);
	//WRITE_BACK_32(hevc,HEVC_SAO_Y_START_ADDR, 0xffffffff);
	}

	// DBLK CONFIG HERE
	if (hevc->new_pic) {

		data32 = (0xff << 8) |  // 1st/2nd write both enable
			(0x0  << 0);   // h265 video format
		if (hevc->pic_w >= 1280) data32 |= (0x1 << 4); // dblk pipeline mode=1 for performance
		//WRITE_VREG( HEVC_DBLK_CFGB, data32);
#if 0 //def FOR_S5
		data32 &= (~0x300); /*[8]:first write enable (compress)  [9]:double write enable (uncompress)*/
		if (dw_mode == 0)
		data32 |= (0x1 << 8); /*enable first write*/
		else if (dw_mode & 0x10)
		data32 |= (0x1 << 9); /*double write only*/
		else
		data32 |= ((0x1 << 8)  |(0x1 << 9));
#endif
		WRITE_BACK_32(hevc,HEVC_DBLK_CFGB, data32);
		hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		" [DBLK DEBUG] HEVC1 CFGB : 0x%x\n", data32);

		data32  =   (
		hevc->pic_w|
		hevc->pic_h<<16
		);
		//WRITE_VREG( HEVC_DBLK_CFG2, data32);
		WRITE_BACK_32(hevc, HEVC_DBLK_CFG2, data32);
		hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		" [DBLK DEBUG] HEVC1 CFG2 : 0x%x\n", data32);

		if ((misc_flag0>>PCM_ENABLE_FLAG_BIT)&0x1)
		data32 = ((misc_flag0>>PCM_LOOP_FILTER_DISABLED_FLAG_BIT)&0x1)<<3;
		else data32 = 0;
		data32 |= (((params->p.pps_cb_qp_offset&0x1f)<<4)|((params->p.pps_cr_qp_offset&0x1f)<<9));
		data32 |= (hevc->lcu_size == 64)?0:((hevc->lcu_size == 32)?1:2);
#ifdef LPF_SPCC_ENABLE
		//data32 |= (hevc->pic_w <= 64)?(1<<20):0; // if pic width isn't more than one CTU, disable pipeline
		data32 |= (0x3<<20);
#endif
		//WRITE_VREG( HEVC_DBLK_CFG1, data32);
		WRITE_BACK_32(hevc,HEVC_DBLK_CFG1, data32);
		hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		" [DBLK DEBUG] HEVC1 CFG1 : 0x%x\n", data32);

		data32 = 1<<28; // Debug only: sts1 chooses dblk_main
		//WRITE_VREG( HEVC_DBLK_STS1, data32);
#ifdef FOR_S5
/*to do...*/
#undef HEVC_DBLK_STS1
#define HEVC_DBLK_STS1 0x510
#endif
		WRITE_BACK_32(hevc,HEVC_DBLK_STS1, data32);

		hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		" [DBLK DEBUG] HEVC1 STS1 : 0x%x\n", data32);
	}

#ifdef DOS_PROJECT
	//data32 = READ_VREG( HEVC_SAO_CTRL1);
	//data32 &= (~0x3000);
	//data32 |= (MEM_MAP_MODE << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	//WRITE_VREG( HEVC_SAO_CTRL1, data32);
	//READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL1, hevc->mem_map_mode, 12, 2);
#ifdef SAVE_NON_REF
	if (NON_REF_B) {
	//WRITE_VREG(HEVC_SAO_CTRL1, data32 | (0x1<<0)); // disable compress output
	hevc->instruction[hevc->ins_offset] = 0x9800000;                                                       //setb COMMON_REG_0, 0
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x6060200;                                                       //mtsp COMMON_REG_0, HEVC_SAO_CTRL1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	} else {
	//WRITE_VREG(HEVC_SAO_CTRL1, data32 & 0xfffffffe);
	hevc->instruction[hevc->ins_offset] = 0x9c00000;                                                       //clrb COMMON_REG_0, 0
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x6060200;                                                       //mtsp COMMON_REG_0, HEVC_SAO_CTRL1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	}
#endif
	data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG);
	data32 &= (~0x30);
	data32 |= (hevc->mem_map_mode << 4); // [5:4]    -- address_format 00:linear 01:32x32 10:64x32
	data32 &= (~0xF);
	data32 |= (hevc->endian & 0xf);  /* valid only when double write only */
	data32 &= (~(3 << 8));
	data32 |= (2 << 8);
	WRITE_BACK_32(hevc, HEVCD_IPP_AXIIF_CONFIG, data32);
#else
// m8baby test1902
	//data32 = READ_VREG( HEVC_SAO_CTRL1);
	//data32 &= (~0x3000);
	//data32 |= (MEM_MAP_MODE << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	//data32 &= (~0xff0);
	//data32 |= 0x880;  // Big-Endian per 64-bit
	//WRITE_VREG( HEVC_SAO_CTRL1, data32);
#if 0
	data32 = (hevc->mem_map_mode<<12) | (0x88<<4);
	READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL1, data32, 4, 10);
#endif
#ifdef SAVE_NON_REF
	if (NON_REF_B) {
	//WRITE_VREG(HEVC_SAO_CTRL1, data32 | (0x1<<0)); // disable compress output
	hevc->instruction[hevc->ins_offset] = 0x9800000;                                                       //setb COMMON_REG_0, 0
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x6060200;                                                       //mtsp COMMON_REG_0, HEVC_SAO_CTRL1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	}
	else {
	//WRITE_VREG(HEVC_SAO_CTRL1, data32 & 0xfffffffe);
	hevc->instruction[hevc->ins_offset] = 0x9c00000;                                                       //clrb COMMON_REG_0, 0
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	hevc->instruction[hevc->ins_offset] = 0x6060200;                                                       //mtsp COMMON_REG_0, HEVC_SAO_CTRL1
	hevc_print(hevc, H265_DEBUG_DETAIL,
		"instruction[%3d] = %8x\n", hevc->ins_offset, hevc->instruction[hevc->ins_offset]);
	hevc->ins_offset++;
	}
#endif
	data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG);
	data32 &= (~0x30);
	data32 |= (hevc->mem_map_mode << 4); // [5:4]    -- address_format 00:linear 01:32x32 10:64x32
	data32 &= (~0xF);
	data32 |= (hevc->endian & 0xf);  /* valid only when double write only */
	data32 &= (~(3 << 8));
	data32 |= (2 << 8);
	WRITE_BACK_32(hevc, HEVCD_IPP_AXIIF_CONFIG, data32);
#endif
#if 0 // moved to same place config other VH0/1
//ndef AVS2_10B_NV21
#ifdef H265_10B_MMU_DW
#if 1
	WRITE_BACK_32(hevc,HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
	WRITE_BACK_32(hevc,HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
	WRITE_BACK_32(hevc,HEVC_DW_VH0_ADDDR_DBE1, DOUBLE_WRITE_VH0_HALF);
	WRITE_BACK_32(hevc,HEVC_DW_VH1_ADDDR_DBE1, DOUBLE_WRITE_VH1_HALF);
#else
	//WRITE_VREG(HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
	//WRITE_VREG(HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
	WRITE_BACK_32(hevc,HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
	WRITE_BACK_32(hevc,HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
#endif
#endif
#endif

	data32 = 0;
	//data32_2 = READ_VREG( HEVC_SAO_CTRL0);
	//data32_2 &= (~0x300);
	data32_2 = 0;
	//slice_deblocking_filter_disabled_flag = 0; //ucode has handle it , so read it from ucode directly
	//printk("\nconfig dblk HEVC_DBLK_CFG9: misc_flag0 %x tile_enabled %x; data32 is:", misc_flag0, tile_enabled);
	if (hevc->tile_enabled) {
		data32 |= ((misc_flag0>>LOOP_FILER_ACROSS_TILES_ENABLED_FLAG_BIT)&0x1)<<0;
		//data32_2 |= ((misc_flag0>>LOOP_FILER_ACROSS_TILES_ENABLED_FLAG_BIT)&0x1)<<8;
		data32_2 |= ((misc_flag0>>LOOP_FILER_ACROSS_TILES_ENABLED_FLAG_BIT)&0x1);
	}
	slice_deblocking_filter_disabled_flag = (misc_flag0>>SLICE_DEBLOCKING_FILTER_DISABLED_FLAG_BIT)&0x1;    //ucode has handle it , so read it from ucode directly
	if ((misc_flag0&(1<<DEBLOCKING_FILTER_OVERRIDE_ENABLED_FLAG_BIT))
		&&(misc_flag0&(1<<DEBLOCKING_FILTER_OVERRIDE_FLAG_BIT))) {
		//slice_deblocking_filter_disabled_flag =   (misc_flag0>>SLICE_DEBLOCKING_FILTER_DISABLED_FLAG_BIT)&0x1;    //ucode has handle it , so read it from ucode directly
		data32 |= slice_deblocking_filter_disabled_flag<<2;
		if (debug&H265_DEBUG_BUFMGR) printk("(1,%x)", data32);
		if (!slice_deblocking_filter_disabled_flag) {
		data32 |= (params->p.slice_beta_offset_div2&0xf)<<3;
		data32 |= (params->p.slice_tc_offset_div2&0xf)<<7;
		if (debug&H265_DEBUG_BUFMGR) printk("(2,%x)", data32);
		}
	}
	else{
		data32 |= ((misc_flag0>>PPS_DEBLOCKING_FILTER_DISABLED_FLAG_BIT)&0x1)<<2;
		if (debug&H265_DEBUG_BUFMGR) printk("(3,%x)", data32);
		if (((misc_flag0>>PPS_DEBLOCKING_FILTER_DISABLED_FLAG_BIT)&0x1) == 0) {
		data32 |= (params->p.pps_beta_offset_div2&0xf)<<3;
		data32 |= (params->p.pps_tc_offset_div2&0xf)<<7;
		if (debug&H265_DEBUG_BUFMGR) printk("(4,%x)", data32);
		}
	}
	if ((misc_flag0&(1<<PPS_LOOP_FILTER_ACROSS_SLICES_ENABLED_FLAG_BIT))&&
	((misc_flag0&(1<<SLICE_SAO_LUMA_FLAG_BIT))||(misc_flag0&(1<<SLICE_SAO_CHROMA_FLAG_BIT))||(!slice_deblocking_filter_disabled_flag))) {
		data32 |= ((misc_flag0>>SLICE_LOOP_FILTER_ACROSS_SLICES_ENABLED_FLAG_BIT)&0x1)<<1;
		//data32_2 |= ((misc_flag0>>SLICE_LOOP_FILTER_ACROSS_SLICES_ENABLED_FLAG_BIT)&0x1)<<9;
		data32_2 |= ((misc_flag0>>SLICE_LOOP_FILTER_ACROSS_SLICES_ENABLED_FLAG_BIT)&0x1)<<1;
		if (debug&H265_DEBUG_BUFMGR) printk("(5,%x)\n", data32);
	}
	else{
		data32 |= ((misc_flag0>>PPS_LOOP_FILTER_ACROSS_SLICES_ENABLED_FLAG_BIT)&0x1)<<1;
		//data32_2 |= ((misc_flag0>>PPS_LOOP_FILTER_ACROSS_SLICES_ENABLED_FLAG_BIT)&0x1)<<9;
		data32_2 |= ((misc_flag0>>PPS_LOOP_FILTER_ACROSS_SLICES_ENABLED_FLAG_BIT)&0x1)<<1;
		if (debug&H265_DEBUG_BUFMGR) printk("(6,%x)\n", data32);
	}
	//WRITE_VREG( HEVC_DBLK_CFG9, data32);
	WRITE_BACK_16(hevc, HEVC_DBLK_CFG9, 0, data32);
	hevc_print(hevc, H265_DEBUG_BUFMGR_MORE,
		" [DBLK DEBUG] HEVC1 CFG9 : 0x%x\n", data32);
	//WRITE_VREG( HEVC_SAO_CTRL0, data32_2);
	READ_WRITE_DATA16(hevc, HEVC_SAO_CTRL0, data32_2, 8, 2);

}

#endif

