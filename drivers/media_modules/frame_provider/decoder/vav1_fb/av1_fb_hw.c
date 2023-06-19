
#include "../../../include/regs/dos_registers.h"

/*todo*/
#if 1
#define DOUBLE_WRITE_VH0_HALF 0
#define DOUBLE_WRITE_VH1_HALF 0
#define DOUBLE_WRITE_VH0_TEMP 0
#define DOUBLE_WRITE_VH1_TEMP 0
#define DOUBLE_WRITE_YSTART_TEMP 0
#define DOUBLE_WRITE_CSTART_TEMP 0
#endif

static void mcrcc_get_hitrate(struct AV1HW_s *hw, unsigned reset_pre);
static void decomp_get_hitrate(struct AV1HW_s *hw);
static void decomp_get_comprate(struct AV1HW_s *hw);
static  uint32_t  mcrcc_get_abs_frame_distance(struct AV1HW_s *hw, uint32_t refid, uint32_t ref_ohint, uint32_t curr_ohint, uint32_t ohint_bits_min1);

void av1_loop_filter_init_fb(struct AV1HW_s *hw);

#if 1
void print_hevc_b_data_path_monitor(AV1Decoder* pbi, int frame_count)
{
		uint32_t total_clk_count;
		uint32_t path_transfer_count;
		uint32_t path_wait_count;
		uint32_t path_status;
	u32 path_wait_ratio;

		printk("\n[WAITING DATA/CMD] Parser/IQIT/IPP/DBLK/OW/DDR/MPRED_IPP_CMD/IPP_DBLK_CMD\n");

	//---------------------- CORE 0 -------------------------------
		WRITE_VREG(HEVC_PATH_MONITOR_CTRL, 0); // Disable monitor and set rd_idx to 0
		total_clk_count = READ_VREG(HEVC_PATH_MONITOR_DATA);

		WRITE_VREG(HEVC_PATH_MONITOR_CTRL, (1<<4)); // Disable monitor and set rd_idx to 1

	// parser --> iqit
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk("[P%d HEVC CORE0 PATH] WAITING Ratio : %d",
		frame_count,
		path_wait_ratio);

	// iqit --> ipp
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// dblk <-- ipp
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// dblk --> ow
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// <--> DDR
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// IMP
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// path status
	path_status = READ_VREG(HEVC_PATH_MONITOR_DATA);

	// CMD
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

#if 0
		WRITE_VREG(P_HEVC_PARSER_IF_MONITOR_CTRL, (2<<4)); // Disable monitor and set rd_idx to 2

	// Parser-Mpred CMD
		path_transfer_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA);
		path_wait_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0.0;
	else path_wait_ratio = (float)path_wait_count/(float)path_transfer_count;
		printk(" %.2f", path_wait_ratio);

	// Parser-SAO CMD
		path_transfer_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA);
		path_wait_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0.0;
	else path_wait_ratio = (float)path_wait_count/(float)path_transfer_count;
		printk(" %.2f\n", path_wait_ratio);
	}
#else
	printk("\n");
#endif
	//---------------------- CORE 1 -------------------------------
		WRITE_VREG(HEVC_PATH_MONITOR_CTRL_DBE1, 0); // Disable monitor and set rd_idx to 0
		total_clk_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);

		WRITE_VREG(HEVC_PATH_MONITOR_CTRL_DBE1, (1<<4)); // Disable monitor and set rd_idx to 1

	// parser --> iqit
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk("[P%d HEVC CORE1 PATH] WAITING Ratio : %d",
		frame_count,
		path_wait_ratio);

	// iqit --> ipp
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// dblk <-- ipp
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// dblk --> ow
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// <--> DDR
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// IMP
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

	// path status
	path_status = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);

	// CMD
		path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
		printk(" %d", path_wait_ratio);

# if 0
		WRITE_VREG(P_HEVC_PARSER_IF_MONITOR_CTRL, (2<<4)); // Disable monitor and set rd_idx to 2

	// Parser-Mpred CMD
		path_transfer_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0.0;
	else path_wait_ratio = (float)path_wait_count/(float)path_transfer_count;
		printk(" %.2f", path_wait_ratio);

	// Parser-SAO CMD
		path_transfer_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA_DBE1);
		path_wait_count = READ_VREG(P_HEVC_PARSER_IF_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0.0;
	else path_wait_ratio = (float)path_wait_count/(float)path_transfer_count;
		printk(" %.2f\n", path_wait_ratio);
	}
#else
	printk("\n");
#endif

}
#endif

static void copy_loopbufs_ptr(buff_ptr_t* trg, buff_ptr_t* src)
{
	trg->mmu0_ptr = src->mmu0_ptr;
	trg->mmu1_ptr = src->mmu1_ptr;
	trg->lcu_info_data0_ptr = src->lcu_info_data0_ptr;
	trg->lcu_info_data1_ptr = src->lcu_info_data1_ptr;
	trg->mpred_imp0_ptr = src->mpred_imp0_ptr;
	trg->mpred_imp1_ptr = src->mpred_imp1_ptr;
	trg->cu_info_data0_ptr = src->cu_info_data0_ptr;
	trg->cu_info_data1_ptr = src->cu_info_data1_ptr;
	trg->gmwm_data_ptr = src->gmwm_data_ptr;
	trg->lrf_data_ptr = src->lrf_data_ptr;
	trg->tldat_data0_ptr = src->tldat_data0_ptr;
	trg->tldat_data1_ptr = src->tldat_data1_ptr;
	trg->tile_header_param_ptr = src->tile_header_param_ptr;
	trg->fgs_ucode_ptr = src->fgs_ucode_ptr;
	trg->sys_imem_ptr = src->sys_imem_ptr;
	trg->sys_imem_ptr_v = src->sys_imem_ptr_v;
}

#define BUF_BLOCK_NUM                   1024//1024

#define IFBUF_LCU_INFO_DATA0_BLKSIZE    0x400
#define IFBUF_LCU_INFO_DATA1_BLKSIZE    0x400
#define IFBUF_MPRED_IMP0_BLKSIZE        0x4000
#define IFBUF_MPRED_IMP1_BLKSIZE        0x4000
#define IFBUF_CU_INFO_DATA0_BLKSIZE     0x6000 //0xc000
#define IFBUF_CU_INFO_DATA1_BLKSIZE     0x6000 //0xc000
#define IFBUF_GMWM_DATA_BLKSIZE         0x800 // just once per frame
#define IFBUF_LRF_DATA_BLKSIZE          0x400
#define IFBUF_TLDAT_DATA0_BLKSIZE       0x5000
#define IFBUF_TLDAT_DATA1_BLKSIZE       0x5000
#define IFBUF_TILE_HEADER_PARAM_BLKSIZE 0x80
#define IFBUF_FGS_UCODE_BLKSIZE         0x200 // just once per frame
#define FB_IFBUF_SYS_IMEM_BLOCK_SIZE    0x400

#define IFBUF_LCU_INFO_DATA0_SIZE      (IFBUF_LCU_INFO_DATA0_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_LCU_INFO_DATA1_SIZE      (IFBUF_LCU_INFO_DATA1_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_MPRED_IMP0_SIZE          (IFBUF_MPRED_IMP0_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_MPRED_IMP1_SIZE          (IFBUF_MPRED_IMP1_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_CU_INFO_DATA0_SIZE       (IFBUF_CU_INFO_DATA0_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_CU_INFO_DATA1_SIZE       (IFBUF_CU_INFO_DATA1_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_GMWM_DATA_SIZE           (IFBUF_GMWM_DATA_BLKSIZE * 2) // just once per frame, add 1 to avoid overflow
#define IFBUF_LRF_DATA_SIZE            (IFBUF_LRF_DATA_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_TLDAT_DATA0_SIZE         (IFBUF_TLDAT_DATA0_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_TLDAT_DATA1_SIZE         (IFBUF_TLDAT_DATA1_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_TILE_HEADER_PARAM_SIZE   (IFBUF_TILE_HEADER_PARAM_BLKSIZE * BUF_BLOCK_NUM)
#define IFBUF_FGS_UCODE_SIZE           (IFBUF_FGS_UCODE_BLKSIZE * 2) // just once per frame, add 1 to avoid overflow
#define IFBUF_SYS_IMEM_SIZE            (FB_IFBUF_SYS_IMEM_BLOCK_SIZE * 256)

/* todo */
#define IFBUF_BASE_ID                  ((WORK_SPACE_BUF_ID + 1))
#define IFBUF_LCU_INFO_DATA0_ID        (IFBUF_BASE_ID)
#define IFBUF_LCU_INFO_DATA1_ID        (IFBUF_BASE_ID + 1)
#define IFBUF_MPRED_IMP0_ID            (IFBUF_BASE_ID + 2)
#define IFBUF_MPRED_IMP1_ID            (IFBUF_BASE_ID + 3)
#define IFBUF_CU_INFO_DATA0_ID         (IFBUF_BASE_ID + 4)
#define IFBUF_CU_INFO_DATA1_ID         (IFBUF_BASE_ID + 5)
#define IFBUF_GMWM_DATA_ID             (IFBUF_BASE_ID + 6)
#define IFBUF_LRF_DATA_ID              (IFBUF_BASE_ID + 7)
#define IFBUF_TLDAT_DATA0_ID           (IFBUF_BASE_ID + 8)
#define IFBUF_TLDAT_DATA1_ID           (IFBUF_BASE_ID + 9)
#define IFBUF_TILE_HEADER_PARAM_ID     (IFBUF_BASE_ID + 10)
#define IFBUF_FGS_UCODE_ID             (IFBUF_BASE_ID + 11)

#ifdef PXP_DEBUG_CODE

static u32 dump_fb_opt;
module_param(dump_fb_opt, uint, 0664);

static void print_dump(const char *str, ulong ptr, u32 size)
{
	u32 i;
	char *vaddr;

	pr_info("%s [start %lx, size 0x%x]:\n", str, ptr, size);

	vaddr = codec_mm_vmap(ptr, size);
	if (vaddr == NULL) {
		pr_info("%s vmap failed, dump failed\n", str);
		return;
	}
	for (i = 0; i < size/4; i++) {
		pr_info("%08x ", ((u32 *)vaddr)[i]);
		if (((i+1) & 0xf) == 0) {
			pr_info("\n");
		}
	}
	codec_mm_unmap_phyaddr(vaddr);
	return;
}

static u32 cal_data_size(u32 cur, u32 bef, u32 end)
{
	if (cur >= bef)
		return (cur - bef);
	else {
		if (end > bef)
			return (end - bef);

		return (end - cur);
	}
	//return (a > b) ? (a - b) : (b - a);
}

static void dump_loopbuf(AV1Decoder* pbi)
{
	buff_ptr_t *before = &pbi->next_bk[pbi->fb_wr_pos];
	buff_ptr_t *curbuf = &pbi->fr;

	if (dump_fb_opt & 1) {
		print_dump("lcu_info_data0_ptr", before->lcu_info_data0_ptr,
			cal_data_size(curbuf->lcu_info_data0_ptr, before->lcu_info_data0_ptr, pbi->fb_buf_lcu_info_data0.buf_end));

		print_dump("lcu_info_data1_ptr", before->lcu_info_data1_ptr,
			cal_data_size(curbuf->lcu_info_data1_ptr, before->lcu_info_data1_ptr, pbi->fb_buf_lcu_info_data1.buf_end));
	}
	if (dump_fb_opt & 2) {
		print_dump("mpred_imp0_ptr", before->mpred_imp0_ptr,
			cal_data_size(curbuf->mpred_imp0_ptr, before->mpred_imp0_ptr, pbi->fb_buf_mpred_imp0.buf_end));

		print_dump("mpred_imp1_ptr", before->mpred_imp1_ptr,
			cal_data_size(curbuf->mpred_imp1_ptr, before->mpred_imp1_ptr, pbi->fb_buf_mpred_imp1.buf_end));
	}
	if (dump_fb_opt & 4) {
		print_dump("cu_info_data0_ptr", before->cu_info_data0_ptr,
			cal_data_size(curbuf->cu_info_data0_ptr, before->cu_info_data0_ptr, pbi->fb_buf_cu_info_data0.buf_end));

		print_dump("cu_info_data1_ptr", before->cu_info_data1_ptr,
			cal_data_size(curbuf->cu_info_data1_ptr, before->cu_info_data1_ptr,  pbi->fb_buf_cu_info_data1.buf_end));
	}
	if (dump_fb_opt & 8) {
		u32 size = cal_data_size(curbuf->gmwm_data_ptr, before->gmwm_data_ptr, pbi->fb_buf_gmwm_data.buf_end);
		if (size == 0)
			size = IFBUF_GMWM_DATA_SIZE;

		print_dump("gmwm_data_ptr", before->gmwm_data_ptr, size);
	}
	if (dump_fb_opt & 0x10) {
		print_dump("lrf_data_ptr", before->lrf_data_ptr,
			cal_data_size(curbuf->lrf_data_ptr, before->lrf_data_ptr, pbi->fb_buf_lrf_data.buf_end));
	}

	if (dump_fb_opt & 0x20) {
		print_dump("tldat_data0_ptr", before->tldat_data0_ptr,
			cal_data_size(curbuf->tldat_data0_ptr, before->tldat_data0_ptr, pbi->fb_buf_tldat_data0.buf_end));

		print_dump("tldat_data1_ptr", before->tldat_data1_ptr,
			cal_data_size(curbuf->tldat_data1_ptr, before->tldat_data1_ptr, pbi->fb_buf_tldat_data1.buf_end));
	}

	if (dump_fb_opt & 0x40) {
		print_dump("tile_header_param_ptr", before->tile_header_param_ptr,
			cal_data_size(curbuf->tile_header_param_ptr, before->tile_header_param_ptr, pbi->fb_buf_tile_header_param.buf_end));
	}
	if (dump_fb_opt & 0x80) {
		print_dump("fgs_ucode_ptr", before->fgs_ucode_ptr,
			cal_data_size(curbuf->fgs_ucode_ptr, before->fgs_ucode_ptr, pbi->fb_buf_fgs_ucode.buf_end));
	}
}
#endif

static void read_bufstate_front(AV1Decoder* pbi)
{
	pbi->fr.mmu0_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0);
	pbi->fr.mmu1_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	pbi->fr.lcu_info_data0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	pbi->fr.lcu_info_data1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	pbi->fr.mpred_imp0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	pbi->fr.mpred_imp1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	pbi->fr.cu_info_data0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	pbi->fr.cu_info_data1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	pbi->fr.gmwm_data_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	pbi->fr.lrf_data_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
	pbi->fr.tldat_data0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 9);
	pbi->fr.tldat_data1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 10);
	pbi->fr.tile_header_param_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 11);
	pbi->fr.fgs_ucode_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);

	pbi->fr.sys_imem_ptr = pbi->sys_imem_ptr;
	pbi->fr.sys_imem_ptr_v = pbi->sys_imem_ptr_v;
#ifdef PXP_DEBUG_CODE
	dump_loopbuf(pbi);
#endif
}

#ifdef NEW_FRONT_BACK_CODE

static void WRITE_BACK_RET(struct AV1HW_s *hw)
{
	struct AV1Decoder* pbi = hw->pbi;
	pbi->instruction[pbi->ins_offset] = 0xcc00000;   //ret
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"WRITE_BACK_RET()\ninstruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = 0;           //nop
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
}

static void WRITE_BACK_8(struct AV1HW_s *hw, uint32_t spr_addr, uint8_t data)
{
	struct AV1Decoder* pbi = hw->pbi;
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	pbi->instruction[pbi->ins_offset] = (0x20<<22) | ((spr_addr&0xfff)<<8) | (data&0xff);   //mtspi data, spr_addr
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
		pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (pbi->ins_offset < 512 && (pbi->ins_offset + 16) >= 512) {
		WRITE_BACK_RET(hw);
		pbi->ins_offset = 512;
	}
#endif
}

static void WRITE_BACK_16(struct AV1HW_s *hw, uint32_t spr_addr, uint8_t rd_addr, uint16_t data)
{
	struct AV1Decoder* pbi = hw->pbi;
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	pbi->instruction[pbi->ins_offset] = (0x1a<<22) | ((data&0xffff)<<6) | (rd_addr&0x3f);       // movi rd_addr, data[15:0]
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"%s(%x,%x,%x)\ninstruction[%3d] = %8x, data= %x\n",
		__func__, spr_addr, rd_addr, data,
		pbi->ins_offset, pbi->instruction[pbi->ins_offset], data&0xffff);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8) | (rd_addr&0x3f);  // mtsp rd_addr, spr_addr
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;

#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (pbi->ins_offset < 512 && (pbi->ins_offset + 16) >= 512) {
		WRITE_BACK_RET(hw);
		pbi->ins_offset = 512;
	}
#endif
}

static void WRITE_BACK_32(struct AV1HW_s *hw, uint32_t spr_addr, uint32_t data)
{
	struct AV1Decoder* pbi = hw->pbi;
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	pbi->instruction[pbi->ins_offset] = (0x1a<<22) | ((data&0xffff)<<6);   // movi COMMON_REG_0, data
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
		pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	data = (data & 0xffff0000)>>16;
	pbi->instruction[pbi->ins_offset] = (0x1b<<22) | (data<<6);                // mvihi COMMON_REG_0, data[31:16]
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8);    // mtsp COMMON_REG_0, spr_addr
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;

#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (pbi->ins_offset < 512 && (pbi->ins_offset + 16) >= 512) {
		WRITE_BACK_RET(hw);
		pbi->ins_offset = 512;
	}
#endif
}


static void READ_WRITE_DATA16(struct AV1HW_s *hw, uint32_t spr_addr, uint16_t data, uint8_t position, uint8_t size)
{
	struct AV1Decoder* pbi = hw->pbi;
	//spr_addr = ((spr_addr - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	pbi->instruction[pbi->ins_offset] = (0x19<<22) | ((spr_addr&0xfff)<<8);    //mfsp COMON_REG_0, spr_addr
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"%s(%x,%x,%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data, position, size,
		pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = (0x1a<<22) | (data<<6) | 1;        //movi COMMON_REG_1, data
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | (0<<6) | 1;  //ins COMMON_REG_0, COMMON_REG_1, position, size
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = (0x18<<22) | ((spr_addr&0xfff)<<8);    //mtsp COMMON_REG_0, spr_addr
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;

#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (pbi->ins_offset < 512 && (pbi->ins_offset + 16) >= 512) {
		WRITE_BACK_RET(hw);
		pbi->ins_offset = 512;
	}
#endif
}

static void READ_INS_WRITE(struct AV1HW_s *hw, uint32_t spr_addr0, uint32_t spr_addr1, uint8_t rd_addr, uint8_t position, uint8_t size)
{
	struct AV1Decoder* pbi = hw->pbi;
	//spr_addr0 = ((spr_addr0 - DOS_BASE_ADR) >> 0) & 0xfff;
	//spr_addr1 = ((spr_addr1 - DOS_BASE_ADR) >> 0) & 0xfff;
	spr_addr0 = (dos_reg_compat_convert(spr_addr0) & 0xfff);
	spr_addr1 = (dos_reg_compat_convert(spr_addr1) & 0xfff);
	pbi->instruction[pbi->ins_offset] = (0x19<<22) | ((spr_addr0&0xfff)<<8) | (rd_addr&0x3f);    //mfsp rd_addr, src_addr0
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"%s(%x,%x,%x,%x,%x)\n",
		__func__, spr_addr0, spr_addr1, rd_addr, position, size);
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = (0x25<<22) | ((position&0x1f)<<17) | ((size&0x1f)<<12) | ((rd_addr&0x3f)<<6) | (rd_addr&0x3f);   //ins rd_addr, rd_addr, position, size
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
	pbi->instruction[pbi->ins_offset] = (0x18<<22) | ((spr_addr1&0xfff)<<8) | (rd_addr&0x3f);    //mtsp rd_addr, src_addr1
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"instruction[%3d] = %8x\n", pbi->ins_offset, pbi->instruction[pbi->ins_offset]);
	pbi->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
	if (pbi->ins_offset < 512 && (pbi->ins_offset + 16) >= 512) {
		WRITE_BACK_RET(hw);
		pbi->ins_offset = 512;
	}
#endif
}

void av1_upscale_frame_init_be(struct AV1HW_s *hw)
{
	struct AV1Decoder* pbi = hw->pbi;
	BuffInfo_t* buf_spec = pbi->work_space_buf;
	PIC_BUFFER_CONFIG *pic = &hw->common.cur_frame->buf;
	av1_print(hw, AOM_DEBUG_HW_MORE,
				"[test.c] av1_upscale_frame_init_be\n");
	WRITE_BACK_32(hw, HEVC_DBLK_UPS1, buf_spec->ups_data.buf_start); // ups_temp_address start
	WRITE_BACK_8(hw, HEVC_DBLK_UPS2, pic->x0_qn_luma);         // x0_qn y
	WRITE_BACK_8(hw, HEVC_DBLK_UPS3, pic->x0_qn_chroma);       // x0_qn c
	WRITE_BACK_16(hw, HEVC_DBLK_UPS4, 0, pic->x_step_qn_luma);     // x_step y
	WRITE_BACK_16(hw, HEVC_DBLK_UPS5, 0, pic->x_step_qn_chroma);   // x_step c
	WRITE_BACK_32(hw, HEVC_DBLK_UPS1_DBE1, buf_spec->ups_data.buf_start); // ups_temp_address start
	WRITE_BACK_8(hw, HEVC_DBLK_UPS2_DBE1, pic->x0_qn_luma);         // x0_qn y
	WRITE_BACK_8(hw, HEVC_DBLK_UPS3_DBE1, pic->x0_qn_chroma);       // x0_qn c
	WRITE_BACK_16(hw, HEVC_DBLK_UPS4_DBE1, 0, pic->x_step_qn_luma);     // x_step y
	WRITE_BACK_16(hw, HEVC_DBLK_UPS5_DBE1, 0, pic->x_step_qn_chroma);   // x_step c
}

#endif

static void print_loopbufs_ptr(char* mark, buff_ptr_t* ptr)
{
	if (!debug)
		return;

	printk("==%s==:\n", mark);
	printk(
	"mmu0_ptr 0x%x, mmu1_ptr 0x%x, lcu_info_data0_ptr 0x%x lcu_info_data1_ptr 0x%x, mpred_imp0_ptr 0x%x, mpred_imp1_ptr 0x%x,\
		cu_info_data0_ptr 0x%x, cu_info_data1_ptr 0x%x, gmwm_data_ptr 0x%x, lrf_data_ptr 0x%x, tldat_data0_ptr 0x%x, \
		tldat_data1_ptr 0x%x, tile_header_param_ptr 0x%x, fgs_ucode_ptr 0x%x\n",
	ptr->mmu0_ptr,
	ptr->mmu1_ptr,
	ptr->lcu_info_data0_ptr,
	ptr->lcu_info_data1_ptr,
	ptr->mpred_imp0_ptr,
	ptr->mpred_imp1_ptr,
	ptr->cu_info_data0_ptr,
	ptr->cu_info_data1_ptr,
	ptr->gmwm_data_ptr,
	ptr->lrf_data_ptr,
	ptr->tldat_data0_ptr,
	ptr->tldat_data1_ptr,
	ptr->tile_header_param_ptr,
	ptr->fgs_ucode_ptr);
}

static void release_fb_bufstate(struct AV1HW_s *hw)
{
	struct AV1Decoder *pbi = hw->pbi;

	if (hw->fb_buf_mmu0_addr)
		dma_free_coherent(amports_get_dma_device(),
			pbi->fb_buf_mmu0.buf_size,
			hw->fb_buf_mmu0_addr,
			pbi->fb_buf_mmu0.buf_start);
	hw->fb_buf_mmu0_addr = NULL;

	if (hw->fb_buf_mmu1_addr)
		dma_free_coherent(amports_get_dma_device(),
		pbi->fb_buf_mmu1.buf_size,
		hw->fb_buf_mmu1_addr,
		pbi->fb_buf_mmu1.buf_start);
	hw->fb_buf_mmu1_addr = NULL;

	if (pbi->fb_buf_sys_imem_addr) {
		dma_free_coherent(amports_get_dma_device(),
			pbi->fb_buf_sys_imem.buf_size, pbi->fb_buf_sys_imem_addr,
			pbi->fb_buf_sys_imem.buf_start);
		pbi->fb_buf_sys_imem_addr = NULL;
	}

	if (hw->mmu_box_fb)
		decoder_mmu_box_free(hw->mmu_box_fb);
	hw->mmu_box_fb = NULL;
}

static int fb_buf_alloc(struct AV1HW_s *hw, buff_t *buf, u32 id, u32 ssize)
{
	int ret;
	ulong tmp_addr;
	bool tvp_flag = vdec_secure(hw_to_vdec(hw));

	buf->buf_size = ssize * hw->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(hw->bmmu_box, id, buf->buf_size, DRIVER_NAME, &tmp_addr);
	if (ret < 0) {
		buf->buf_start = 0;
		buf->buf_end = 0;
		buf->buf_size = 0;
		pr_err("%s, alloc id %d failed\n", __func__, id);
		return ret;
	}
	buf->buf_start = tmp_addr;
	buf->buf_end = tmp_addr + buf->buf_size;

	if (!tvp_flag)
		codec_mm_memset(tmp_addr, 0, buf->buf_size);

	return ret;
}

static int init_fb_bufstate(struct AV1HW_s *hw)
{
	struct AV1Decoder *pbi = hw->pbi;
	int ret = -1;
	dma_addr_t tmp_phy_adr;
	int mmu_4k_number = hw->fb_ifbuf_num * av1_mmu_page_num(hw, hw->max_pic_w,
			hw->max_pic_h, 1);
	int mmu_map_size = ((mmu_4k_number * 4) >> 6) << 6;
	int tvp_flag = vdec_secure(hw_to_vdec(hw)) ?
		CODEC_MM_FLAGS_TVP : 0;

	pbi->fb_buf_sys_imem.buf_size = IFBUF_SYS_IMEM_SIZE * hw->fb_ifbuf_num;
	pbi->fb_buf_sys_imem_addr =
		dma_alloc_coherent(amports_get_dma_device(),
		pbi->fb_buf_sys_imem.buf_size,
		&tmp_phy_adr, GFP_KERNEL);
	pbi->fb_buf_sys_imem.buf_start = tmp_phy_adr;
	if (pbi->fb_buf_sys_imem_addr == NULL) {
		pr_err("%s: failed to alloc fb_buf_sys_imem\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	memset(pbi->fb_buf_sys_imem_addr, 0, pbi->fb_buf_sys_imem.buf_size);
	pbi->fb_buf_sys_imem.buf_end = pbi->fb_buf_sys_imem.buf_start + pbi->fb_buf_sys_imem.buf_size;

	hw->mmu_box_fb = decoder_mmu_box_alloc_box(DRIVER_NAME,
		hw->index, 2, (mmu_4k_number << 12) * 2, tvp_flag);

	hw->fb_buf_mmu0_addr =
			dma_alloc_coherent(amports_get_dma_device(),
			mmu_map_size,
			&tmp_phy_adr, GFP_KERNEL);
	pbi->fb_buf_mmu0.buf_start = tmp_phy_adr;
	if (hw->fb_buf_mmu0_addr == NULL) {
		pr_err("%s: failed to alloc fb_mmu0_map\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	memset(hw->fb_buf_mmu0_addr, 0, mmu_map_size);
	pbi->fb_buf_mmu0.buf_size = mmu_map_size;
	pbi->fb_buf_mmu0.buf_end = pbi->fb_buf_mmu0.buf_start + mmu_map_size;

	hw->fb_buf_mmu1_addr =
			dma_alloc_coherent(amports_get_dma_device(),
			mmu_map_size,
			&tmp_phy_adr, GFP_KERNEL);
	pbi->fb_buf_mmu1.buf_start = tmp_phy_adr;
	if (hw->fb_buf_mmu1_addr == NULL) {
		pr_err("%s: failed to alloc fb_mmu1_map\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	memset(hw->fb_buf_mmu1_addr, 0, mmu_map_size);
	pbi->fb_buf_mmu1.buf_size = mmu_map_size;
	pbi->fb_buf_mmu1.buf_end = pbi->fb_buf_mmu1.buf_start + mmu_map_size;

	ret = decoder_mmu_box_alloc_idx(hw->mmu_box_fb, 0,
		mmu_4k_number, hw->fb_buf_mmu0_addr);
	if (ret != 0) {
		pr_err("%s: failed to alloc fb_mmu0 pages", __func__);
		ret = -ENOMEM;
		return ret;
	}

	ret = decoder_mmu_box_alloc_idx(hw->mmu_box_fb, 1,
		mmu_4k_number, hw->fb_buf_mmu1_addr);
	if (ret != 0) {
		pr_err("%s: failed to alloc fb_mmu1 pages", __func__);
		ret = -ENOMEM;
		return ret;
	}

	ret |= fb_buf_alloc(hw, &pbi->fb_buf_lcu_info_data0, IFBUF_LCU_INFO_DATA0_ID,  IFBUF_LCU_INFO_DATA0_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_lcu_info_data1, IFBUF_LCU_INFO_DATA1_ID,  IFBUF_LCU_INFO_DATA1_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_mpred_imp0,     IFBUF_MPRED_IMP0_ID,      IFBUF_MPRED_IMP0_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_mpred_imp1,     IFBUF_MPRED_IMP1_ID,      IFBUF_MPRED_IMP1_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_cu_info_data0,  IFBUF_CU_INFO_DATA0_ID,   IFBUF_CU_INFO_DATA0_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_cu_info_data1,  IFBUF_CU_INFO_DATA1_ID,   IFBUF_CU_INFO_DATA1_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_gmwm_data,      IFBUF_GMWM_DATA_ID,       IFBUF_GMWM_DATA_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_lrf_data,       IFBUF_LRF_DATA_ID,        IFBUF_LRF_DATA_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_tldat_data0,    IFBUF_TLDAT_DATA0_ID,     IFBUF_TLDAT_DATA0_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_tldat_data1,    IFBUF_TLDAT_DATA1_ID,     IFBUF_TLDAT_DATA1_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_tile_header_param, IFBUF_TILE_HEADER_PARAM_ID, IFBUF_TILE_HEADER_PARAM_SIZE);
	ret |= fb_buf_alloc(hw, &pbi->fb_buf_fgs_ucode,      IFBUF_FGS_UCODE_ID,       IFBUF_FGS_UCODE_SIZE);

	pr_info("%s, alloc ret %d\n", __func__, ret);

	pbi->fr.mmu0_ptr = pbi->fb_buf_mmu0.buf_start;
	pbi->bk.mmu0_ptr = pbi->fb_buf_mmu0.buf_start;
	pbi->fr.mmu1_ptr = pbi->fb_buf_mmu1.buf_start;
	pbi->bk.mmu1_ptr = pbi->fb_buf_mmu1.buf_start;

	pbi->fr.lcu_info_data0_ptr = pbi->fb_buf_lcu_info_data0.buf_start;
	pbi->bk.lcu_info_data0_ptr = pbi->fb_buf_lcu_info_data0.buf_start;
	pbi->fr.lcu_info_data1_ptr = pbi->fb_buf_lcu_info_data1.buf_start;
	pbi->bk.lcu_info_data1_ptr = pbi->fb_buf_lcu_info_data1.buf_start;

	pbi->fr.mpred_imp0_ptr = pbi->fb_buf_mpred_imp0.buf_start;
	pbi->bk.mpred_imp0_ptr = pbi->fb_buf_mpred_imp0.buf_start;
	pbi->fr.mpred_imp1_ptr = pbi->fb_buf_mpred_imp1.buf_start;
	pbi->bk.mpred_imp1_ptr = pbi->fb_buf_mpred_imp1.buf_start;

	pbi->fr.cu_info_data0_ptr = pbi->fb_buf_cu_info_data0.buf_start;
	pbi->bk.cu_info_data0_ptr = pbi->fb_buf_cu_info_data0.buf_start;
	pbi->fr.cu_info_data1_ptr = pbi->fb_buf_cu_info_data1.buf_start;
	pbi->bk.cu_info_data1_ptr = pbi->fb_buf_cu_info_data1.buf_start;

	pbi->fr.gmwm_data_ptr = pbi->fb_buf_gmwm_data.buf_start;
	pbi->bk.gmwm_data_ptr = pbi->fb_buf_gmwm_data.buf_start;

	pbi->fr.lrf_data_ptr = pbi->fb_buf_lrf_data.buf_start;
	pbi->bk.lrf_data_ptr = pbi->fb_buf_lrf_data.buf_start;

	pbi->fr.tldat_data0_ptr = pbi->fb_buf_tldat_data0.buf_start;
	pbi->bk.tldat_data0_ptr = pbi->fb_buf_tldat_data0.buf_start;
	pbi->fr.tldat_data1_ptr = pbi->fb_buf_tldat_data1.buf_start;
	pbi->bk.tldat_data1_ptr = pbi->fb_buf_tldat_data1.buf_start;

	pbi->fr.tile_header_param_ptr = pbi->fb_buf_tile_header_param.buf_start;
	pbi->bk.tile_header_param_ptr = pbi->fb_buf_tile_header_param.buf_start;

	pbi->fr.fgs_ucode_ptr = pbi->fb_buf_fgs_ucode.buf_start;
	pbi->bk.fgs_ucode_ptr = pbi->fb_buf_fgs_ucode.buf_start;

	pbi->fr.sys_imem_ptr = pbi->fb_buf_sys_imem.buf_start;
	pbi->bk.sys_imem_ptr = pbi->fb_buf_sys_imem.buf_start;
	pbi->fr.sys_imem_ptr_v = pbi->fb_buf_sys_imem_addr;

	print_loopbufs_ptr("init", &pbi->fr);

	return 0;
}

static void config_bufstate_front_hw(AV1Decoder* pbi)
{
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_START, pbi->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_END, pbi->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0, pbi->fr.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_START, pbi->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_END, pbi->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1, pbi->fr.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_lcu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_lcu_info_data0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.lcu_info_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.lcu_info_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_lcu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_lcu_info_data1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.lcu_info_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.lcu_info_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	// config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	// config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_cu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_cu_info_data0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.cu_info_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.cu_info_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_cu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_cu_info_data1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.cu_info_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.cu_info_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_gmwm_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_gmwm_data.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.gmwm_data_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.gmwm_data_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_lrf_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_lrf_data.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.lrf_data_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.lrf_data_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_tldat_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_tldat_data0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.tldat_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.tldat_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 9);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_tldat_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_tldat_data1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.tldat_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.tldat_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 10);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_tile_header_param.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_tile_header_param.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.tile_header_param_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.tile_header_param_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 11);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_fgs_ucode.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_fgs_ucode.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.fgs_ucode_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.fgs_ucode_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

//config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 12);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, pbi->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, pbi->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, pbi->fr.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, pbi->bk.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

}

static void config_bufstate_back_hw(AV1Decoder* pbi)
{
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_START, pbi->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_END, pbi->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0, pbi->bk.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_START, pbi->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_END, pbi->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1, pbi->bk.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_lcu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_lcu_info_data0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.lcu_info_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_lcu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_lcu_info_data1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.lcu_info_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	//config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_cu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_cu_info_data0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.cu_info_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_cu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_cu_info_data1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.cu_info_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_gmwm_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_gmwm_data.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.gmwm_data_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_lrf_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_lrf_data.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.lrf_data_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_tldat_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_tldat_data0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.tldat_data0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 9);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_tldat_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_tldat_data1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.tldat_data1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 10);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_tile_header_param.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_tile_header_param.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.tile_header_param_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 11);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_fgs_ucode.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_fgs_ucode.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.fgs_ucode_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

//config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 12);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, pbi->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, pbi->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, pbi->bk.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

}

static int32_t config_pic_size_fb(struct AV1HW_s *hw)
{
	u32 data32;
	u32 frame_width, frame_height;
	//AV1Decoder* pbi;
	int losless_comp_header_size, losless_comp_body_size, losless_comp_header_size_dw, losless_comp_body_size_dw;
	AV1_COMMON *cm = &hw->common;
	PIC_BUFFER_CONFIG *cur_pic_config = &cm->cur_frame->buf;

	uint16_t bit_depth = cur_pic_config->bit_depth;
	int dw_mode = get_double_write_mode(hw);

	frame_width = cur_pic_config->y_crop_width;
	frame_height = cur_pic_config->y_crop_height;

	av1_print(hw, AOM_DEBUG_HW_MORE,
		"#### config_pic_size ####, bit_depth = %d, width = %d, height %d\n",
		bit_depth, frame_width, frame_height);

	// use fixed maximum size
	// seg_map_size = ((frame_width + 127) >> 7) * ((frame_height + 127) >> 7) * 384 ;  // 128x128/4/4*3-bits = 384 Bytes

	// Set by  microcode for AV1
	// WRITE_VREG(P_HEVC_PARSER_PICTURE_SIZE, (frame_height << 16) | frame_width);
#ifndef NEW_FRONT_BACK_CODE
	WRITE_VREG(HEVC_ASSIST_PIC_SIZE_FB_READ, (frame_height << 16) | frame_width);
#endif

#ifndef NEW_FRONT_BACK_CODE
#ifdef AOM_AV1_MMU
	// if (cm->prev_fb_idx >= 0) release_unused_4k(cm->prev_fb_idx);
	// cm->prev_fb_idx = cm->new_fb_idx;
	//printk("DEBUG DEBUG] Before alloc_mmu, prev_fb_idx : %d, new_fb_idx : %d\r\n", cm->prev_fb_idx, cm->new_fb_idx);
#ifdef SIMU_CODE
	alloc_mmu(&av1_mmumgr_0, cm->cur_frame->buf.index, frame_width, frame_height/2+64+8,  bit_depth); // Last more may have 8lines more
	alloc_mmu(&av1_mmumgr_1, cm->cur_frame->buf.index, frame_width, frame_height/2+64+8,  bit_depth); // Last more may have 8lines more
	if (frame_height < 2160) {
	alloc_mmu(&av1_mmumgr_fb0, cm->cur_frame->buf.index, frame_width * 2, frame_height * 2,  bit_depth);
	alloc_mmu(&av1_mmumgr_fb1, cm->cur_frame->buf.index, frame_width * 2, frame_height * 2,  bit_depth);
	}
	else {
	alloc_mmu(&av1_mmumgr_fb0, cm->cur_frame->buf.index, frame_width, frame_height/2,  bit_depth);
	alloc_mmu(&av1_mmumgr_fb1, cm->cur_frame->buf.index, frame_width, frame_height/2,  bit_depth);
	}
#endif
#endif

#ifdef AOM_AV1_MMU_DW
	// if (cm->prev_fb_idx >= 0) release_unused_4k(cm->prev_fb_idx);
	// cm->prev_fb_idx = cm->new_fb_idx;
	//printk("DEBUG DEBUG] Before alloc_mmu, prev_fb_idx : %d, new_fb_idx : %d\r\n", cm->prev_fb_idx, cm->new_fb_idx);
#ifdef SIMU_CODE
	alloc_mmu(&av1_mmumgr_dw0, cm->cur_frame->buf.index, frame_width, frame_height/2+64+8,  bit_depth); // Last more may have 8lines more
	alloc_mmu(&av1_mmumgr_dw1, cm->cur_frame->buf.index, frame_width, frame_height/2+64+8,  bit_depth); // Last more may have 8lines more
#endif
	losless_comp_header_size_dw = compute_losless_comp_header_size_dw(frame_width, frame_height);
	losless_comp_body_size_dw = compute_losless_comp_body_size_dw(frame_width, frame_height, (bit_depth == AOM_BITS_10));
#endif
/*!NEW_FRONT_BACK_CODE*/
#else
#ifdef AOM_AV1_MMU_DW
	losless_comp_header_size_dw = compute_losless_comp_header_size_dw(frame_width, frame_height);
	losless_comp_body_size_dw = compute_losless_comp_body_size_dw(frame_width, frame_height, (bit_depth == AOM_BITS_10));
#endif
#endif

	losless_comp_header_size = compute_losless_comp_header_size(frame_width, frame_height);
	losless_comp_body_size = compute_losless_comp_body_size(frame_width, frame_height, (bit_depth == AOM_BITS_10));

	losless_comp_header_size_dw = compute_losless_comp_header_size_dw(frame_width, frame_height);
	losless_comp_body_size_dw = compute_losless_comp_body_size_dw(frame_width, frame_height, (bit_depth == AOM_BITS_10));

	av1_print(hw, AOM_DEBUG_HW_MORE,
		"%s: width %d height %d depth %d head_size 0x%x body_size 0x%x\r\n",
		__func__, frame_width, frame_height, bit_depth,
		losless_comp_header_size, losless_comp_body_size);
#ifdef LOSLESS_COMPRESS_MODE
	//data32 = READ_VREG(HEVC_SAO_CTRL5);
	if (bit_depth == AOM_BITS_10)
	data32 = 0;
	else
	data32 = 1;

	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL5, data32, 9, 1);

	if (dw_mode != 0x10) {
		WRITE_BACK_8(hw, HEVCD_MPP_DECOMP_CTL1, (0x1 << 4)); // bit[4] : paged_mem_mode
	} else {
		if (bit_depth == AOM_BITS_10) {
		WRITE_BACK_8(hw, HEVCD_MPP_DECOMP_CTL1, (0 << 3)); // bit[3] smem mdoe
		} else {
			WRITE_BACK_8(hw, HEVCD_MPP_DECOMP_CTL1, (1 << 3)); // bit[3] smem mdoe
		}
	}

	WRITE_BACK_32(hw, HEVCD_MPP_DECOMP_CTL2, (losless_comp_body_size >> 5));
	//WRITE_VREG(P_HEVCD_MPP_DECOMP_CTL3,(0xff<<20) | (0xff<<10) | 0xff); //8-bit mode
	WRITE_BACK_32(hw, HEVC_CM_BODY_LENGTH, losless_comp_body_size);
	WRITE_BACK_32(hw, HEVC_CM_HEADER_OFFSET, losless_comp_body_size);
	WRITE_BACK_32(hw, HEVC_CM_HEADER_LENGTH, losless_comp_header_size);
	if (dw_mode & 0x10) {
		WRITE_BACK_32(hw, HEVCD_MPP_DECOMP_CTL1, 0x1 << 31);
	}
#else
	WRITE_BACK_32(hw, HEVCD_MPP_DECOMP_CTL1,0x1 << 31);
#endif
#ifdef AOM_AV1_MMU_DW
	if (dw_mode & 0x20) {
		WRITE_BACK_32(hw, HEVC_CM_BODY_LENGTH2, losless_comp_body_size_dw);
		WRITE_BACK_32(hw, HEVC_CM_HEADER_OFFSET2, losless_comp_body_size_dw);
		WRITE_BACK_32(hw, HEVC_CM_HEADER_LENGTH2, losless_comp_header_size_dw);
	}
#endif
	return 0;
}

static void aom_config_work_space_hw_fb(struct AV1HW_s *hw, int front_flag, int back_flag)
{
	AV1Decoder* pbi = hw->pbi;
	BuffInfo_t* buf_spec = pbi->work_space_buf;
	uint32_t data32;

	int losless_comp_header_size =
		compute_losless_comp_header_size(hw->init_pic_w,
		hw->init_pic_h);
	int losless_comp_body_size =
		compute_losless_comp_body_size(hw->init_pic_w,
		hw->init_pic_h, hw->aom_param.p.bit_depth == 10);

//    if (debug) //printk("%s %x %x %x %x %x %x %x %x %x %x %x %x\n", __func__,
//			buf_spec->ipp.buf_start,
//			buf_spec->start_adr,
//			buf_spec->short_term_rps.buf_start,
//			buf_spec->vps.buf_start,
//			buf_spec->sps.buf_start,
//                      buf_spec->daala_top.buf_start,
//                      buf_spec->sao_up.buf_start,
//                      buf_spec->swap_buf.buf_start,
//			buf_spec->swap_buf2.buf_start,
//			buf_spec->scalelut.buf_start,
//			buf_spec->dblk_para.buf_start,
//			buf_spec->dblk_data.buf_start);
//
#ifdef NEW_FRONT_BACK_CODE
	//WRITE_VREG(HEVC_FGS_UCODE_ADR, buf_spec->fgs_ucode.buf_start );
	// WRITE_VREG(HEVC_FGS_UCODE_ADR_DBE, buf_spec->fgs_ucode.buf_start );
#else
/*!NEW_FRONT_BACK_CODE*/
/*
 WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0, FB_FRAME_MMU_MAP_ADDR_0);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1, FB_FRAME_MMU_MAP_ADDR_1);
*/
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0, hw->frame_mmu_map_phy_addr);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1, hw->frame_mmu_map_phy_addr_1);
	WRITE_VREG(LMEM_PARAM_ADR,buf_spec->lmem.buf_start );
	//WRITE_VREG(HEVC_FGS_UCODE_ADR,buf_spec->fgs_ucode.buf_start );
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_lcu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_lcu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_cu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_cu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_gmwm_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_lrf_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_tldat_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 9);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fb_tldat_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 10);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->tile_header_param.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 11);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, buf_spec->fgs_ucode.buf_start);
#ifdef AOM_AV1_HED_SAME_FB
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0,  hw->frame_mmu_map_phy_addr);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1,  hw->frame_mmu_map_phy_addr_1);
	//WRITE_VREG(HEVC_FGS_UCODE_ADR_DBE,buf_spec->fgs_ucode.buf_start );
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_lcu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_lcu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_cu_info_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_cu_info_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_gmwm_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_lrf_data.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_tldat_data0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 9);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fb_tldat_data1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 10);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->tile_header_param.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 11);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, buf_spec->fgs_ucode.buf_start);
#endif
/*!NEW_FRONT_BACK_CODE*/
#endif
	if (back_flag) {
		//WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE,buf_spec->ipp0.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE,buf_spec->ipp.buf_start);	//ipp == ipp0
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE_DBE1,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2_DBE1,buf_spec->ipp.buf_start); //ipp == ipp0
	}

	if (front_flag) {
		if ((debug & AOM_AV1_DEBUG_SEND_PARAM_WITH_REG) == 0) {
			WRITE_VREG(HEVC_RPM_BUFFER, (u32)hw->rpm_phy_addr);
		}
		WRITE_VREG(AOM_AV1_DAALA_TOP_BUFFER, buf_spec->daala_top.buf_start);
		WRITE_VREG(HEVC_SAO_UP, buf_spec->sao_up.buf_start);
		WRITE_VREG(HEVC_STREAM_SWAP_BUFFER, buf_spec->swap_buf.buf_start);
		WRITE_VREG(AV1_GMC_PARAM_BUFF_ADDR, buf_spec->gmc_buf.buf_start);
		WRITE_VREG(HEVC_SCALELUT, buf_spec->scalelut.buf_start);

		config_aux_buf(hw);
	}

	if (back_flag) {
		int dw_mode = get_double_write_mode(hw);
		WRITE_VREG(HEVC_DBLK_CFG4, buf_spec->dblk_para.buf_start); // cfg_addr_cif
		WRITE_VREG(HEVC_DBLK_CFG5, buf_spec->dblk_data.buf_start); // cfg_addr_xio
		WRITE_VREG(HEVC_DBLK_CFG4_DBE1, buf_spec->dblk_para.buf_start); // cfg_addr_cif
		WRITE_VREG(HEVC_DBLK_CFG5_DBE1, buf_spec->dblk_data.buf_start); // cfg_addr_xio

#ifdef LOSLESS_COMPRESS_MODE
		if (dw_mode != 0x10) {
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0x1 << 4)); // bit[4] : paged_mem_mode
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL2, 0);
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, (0x1 << 4)); // bit[4] : paged_mem_mode
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1, 0);
		} else {
			// if (cur_pic_config->bit_depth == AOM_BITS_10) WRITE_VREG(P_HEVCD_MPP_DECOMP_CTL1, (0<<3)); // bit[3] smem mdoe
			// else WRITE_VREG(P_HEVCD_MPP_DECOMP_CTL1, (1<<3)); // bit[3] smem mdoe
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,(losless_comp_body_size >> 5));
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1,(losless_comp_body_size >> 5));
		}

		//WRITE_VREG(P_HEVCD_MPP_DECOMP_CTL2,(losless_comp_body_size >> 5));
		//WRITE_VREG(P_HEVCD_MPP_DECOMP_CTL3,(0xff<<20) | (0xff<<10) | 0xff); //8-bit mode
		WRITE_VREG(HEVC_CM_BODY_LENGTH, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH, losless_comp_header_size);
		WRITE_VREG(HEVC_CM_BODY_LENGTH_DBE1, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET_DBE1, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH_DBE1, losless_comp_header_size);
		if (dw_mode & 0x10) {
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, 0x1 << 31);
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, 0x1 << 31);
		}
#else
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1,0x1 << 31);
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1,0x1 << 31);
#endif

		if (dw_mode != 0x10) {
			//WRITE_VREG(P_HEVC_SAO_MMU_VH0_ADDR, buf_spec->mmu_vbh.buf_start);
			//WRITE_VREG(P_HEVC_SAO_MMU_VH1_ADDR, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/2);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR, buf_spec->mmu_vbh.buf_start);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/4);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR_DBE1, buf_spec->mmu_vbh.buf_start  + buf_spec->mmu_vbh.buf_size/2);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR_DBE1,
				buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size/2 + buf_spec->mmu_vbh.buf_size/4);

			/* use HEVC_CM_HEADER_START_ADDR */
			data32 = READ_VREG(HEVC_SAO_CTRL5);
			data32 |= (1<<10);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
			data32 |= (1<<10);
			WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
		}

		if (dw_mode & 0x20) {
			WRITE_VREG(HEVC_CM_BODY_LENGTH2,losless_comp_body_size);
			WRITE_VREG(HEVC_CM_HEADER_OFFSET2,losless_comp_body_size);
			WRITE_VREG(HEVC_CM_HEADER_LENGTH2,losless_comp_header_size);
			WRITE_VREG(HEVC_CM_BODY_LENGTH2_DBE1,losless_comp_body_size);
			WRITE_VREG(HEVC_CM_HEADER_OFFSET2_DBE1,losless_comp_body_size);
			WRITE_VREG(HEVC_CM_HEADER_LENGTH2_DBE1,losless_comp_header_size);

			//WRITE_VREG(P_HEVC_SAO_MMU_VH0_ADDR2, buf_spec->mmu_vbh_dw.buf_start);
			//WRITE_VREG(P_HEVC_SAO_MMU_VH1_ADDR2, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/2);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2, buf_spec->mmu_vbh_dw.buf_start);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/4);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2_DBE1, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/2);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2_DBE1,
				buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size/2 + buf_spec->mmu_vbh_dw.buf_size/4);

			/* use HEVC_CM_HEADER_START_ADDR */
			data32 = READ_VREG(HEVC_SAO_CTRL5);
			data32 |= (1<<15);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
			data32 |= (1<<15);
			WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);

		} else {
			data32 = READ_VREG(HEVC_SAO_CTRL5);
			data32 &= ~(1<<15);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
			data32 &= ~(1<<15);
			WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
		}
	}

	if (front_flag) {
#ifdef CO_MV_COMPRESS
		data32 = READ_VREG(HEVC_MPRED_CTRL4);
		data32 |=  (1<<1);
		WRITE_VREG(HEVC_MPRED_CTRL4, data32);
#endif
#ifdef NEW_FRONT_BACK_CODE
		//new dual
		WRITE_VREG(LMEM_DUMP_ADR, buf_spec->lmem.buf_start);
#else
		WRITE_VREG(LMEM_PARAM_ADR, buf_spec->lmem.buf_start);
		WRITE_VREG(LMEM_PARAM_ADR_DBE, buf_spec->lmem.buf_start);
#endif
		WRITE_VREG(AOM_AV1_TILE_HDR_BUFFER,buf_spec->tile_header_param.buf_start);
	}
}

static void mcrcc_perfcount_reset_dual(struct AV1HW_s *hw)
{
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"[cache_util.c] Entered mcrcc_perfcount_reset_dual...\n");
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)0x0);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)0x0);
	return;
}

static void decomp_perfcount_reset_dual(struct AV1HW_s *hw)
{
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"[cache_util.c] Entered decomp_perfcount_reset_dual...\n");
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)0x0);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)0x0);
	return;
}

static void aom_init_decoder_hw_fb(struct AV1HW_s *hw, int32_t decode_pic_begin, int32_t decode_pic_num, int first_flag, int front_flag, int back_flag)
{
	uint32_t data32;
	int dw_mode = get_double_write_mode(hw);
	av1_print(hw, AOM_DEBUG_HW_MORE, "[test.c] Entering aom_init_decoder_hw\n");

	if (!efficiency_mode && front_flag) {
	//if (debug&AOM_AV1_DEBUG_BUFMGR)
		//printk("[test.c] Enable HEVC Parser Interrupt\n");
	data32 = READ_VREG(HEVC_PARSER_INT_CONTROL);
	data32 = data32 & 0x03ffffff;
	data32 = data32 |
		(3 << 29) |  // stream_buffer_empty_int_ctl ( 0x200 interrupt)
		(3 << 26) |  // stream_fifo_empty_int_ctl ( 4 interrupt)
		(1 << 24) |  // stream_buffer_empty_int_amrisc_enable
		(1 << 22) |  // stream_fifo_empty_int_amrisc_enable
		(1 << 10) |  // fed_fb_slice_done_int_cpu_enable
		(1 << 7) |  // dec_done_int_cpu_enable
		(1 << 4) |  // startcode_found_int_cpu_enable
		(0 << 3) |  // startcode_found_int_amrisc_enable
		(1 << 0)    // parser_int_enable
		;
	WRITE_VREG(HEVC_PARSER_INT_CONTROL, data32);

	//if (debug&AOM_AV1_DEBUG_BUFMGR)
		//printk("[test.c] Enable HEVC Parser Shift\n");

	data32 = READ_VREG(HEVC_SHIFT_STATUS);
	data32 = data32 |
		(0 << 1) |  // emulation_check_off // AOM_AV1 do not have emulation
		(1 << 0)    // startcode_check_on
		;
	WRITE_VREG(HEVC_SHIFT_STATUS, data32);

	WRITE_VREG(HEVC_SHIFT_CONTROL,
		(0 << 14) | // disable_start_code_protect
		(1 << 10) | // length_zero_startcode_en // for AOM_AV1
		(1 << 9) | // length_valid_startcode_en // for AOM_AV1
		(3 << 6) | // sft_valid_wr_position
		(2 << 4) | // emulate_code_length_sub_1
		(3 << 1) | // start_code_length_sub_1 // AOM_AV1 use 0x00000001 as startcode (4 Bytes)
		(1 << 0)   // stream_shift_enable
		);

	WRITE_VREG(HEVC_CABAC_CONTROL,
		(1 << 0)   // cabac_enable
		);

	WRITE_VREG(HEVC_PARSER_CORE_CONTROL,
		(1 << 0)   // hevc_parser_core_clk_en
		);

	}
#ifndef NEW_FRONT_BACK_CODE
	WRITE_VREG(HEVC_DEC_STATUS_REG, 0);
#endif

	if (back_flag) {
#if 0
		// Dual Core : back Microcode will always initial SCALELUT
		// Initial IQIT_SCALELUT memory -- just to avoid X in simulation
		//if (debug&AOM_AV1_DEBUG_BUFMGR)
		//printk("[test.c] Initial IQIT_SCALELUT memory -- just to avoid X in simulation...\n");
		printk("initial SCALELUT 0\n");
		WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0); // cfg_p_addr
		for (i=0; i<1024; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, 0);
		WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR_DBE1, 0); // cfg_p_addr
		for (i=0; i<1024; i++) WRITE_VREG(HEVC_IQIT_SCALELUT_DATA_DBE1, 0);
#endif
	}

	if (front_flag) {
/*
#ifdef ENABLE_SWAP_TEST
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 100);
#else
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 0);
#endif
*/
#ifdef NEW_FRONT_BACK_CODE
		if (first_flag)
			WRITE_VREG(HEVC_DECODE_COUNT, 0);
#else
		WRITE_VREG(HEVC_DECODE_PIC_BEGIN_REG, 0);
#endif
		WRITE_VREG(HEVC_DECODE_PIC_NUM_REG, decode_pic_num);
#if 0
		// Send parser_cmd
		//if (debug) //printk("[test.c] SEND Parser Command ...\n");
		WRITE_VREG(HEVC_PARSER_CMD_WRITE, (1<<16) | (0<<0));
		for (i=0; i<PARSER_CMD_NUMBER; i++) {
		WRITE_VREG(HEVC_PARSER_CMD_WRITE, parser_cmd[i]);
		}

		WRITE_VREG(HEVC_PARSER_CMD_SKIP_0, PARSER_CMD_SKIP_CFG_0);
		WRITE_VREG(HEVC_PARSER_CMD_SKIP_1, PARSER_CMD_SKIP_CFG_1);
		WRITE_VREG(HEVC_PARSER_CMD_SKIP_2, PARSER_CMD_SKIP_CFG_2);
#endif
	if (!efficiency_mode)
		WRITE_VREG(HEVC_PARSER_IF_CONTROL,
			//  (1 << 8) | // sao_sw_pred_enable
			(1 << 5) | // parser_sao_if_en
			(1 << 2) | // parser_mpred_if_en
			(1 << 0) // parser_scaler_if_en
			);

		// Changed to Start MPRED in microcode
		/*
		//printk("[test.c] Start MPRED\n");
		WRITE_VREG(P_HEVC_MPRED_INT_STATUS,
			(1<<31)
			);
		*/
	}

	if (!efficiency_mode && back_flag) {
		//if (debug) //printk("[test.c] Reset IPP\n");
		WRITE_VREG(HEVCD_IPP_TOP_CNTL,
			(0 << 1) | // enable ipp
			(1 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
			(0 << 1) | // enable ipp
			(1 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL,
			(3 << 4) | // av1
			(1 << 7) | // enable oslice_flush
			(1 << 1) | // enable ipp
			(0 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
			(3 << 4) | // av1
			(1 << 7) | // enable oslice_flush
			(1 << 1) | // enable ipp
			(0 << 0)   // software reset ipp and mpp
			);

#ifdef AOM_AV1_NV21
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, 0x1 << 31); // Enable NV21 reference read mode for MC
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, 0x1 << 31); // Enable NV21 reference read mode for MC
#endif

		WRITE_VREG(HEVCD_IPP_MULTICORE_CFG, 0x1);// muti core enable
		WRITE_VREG(HEVCD_IPP_MULTICORE_CFG_DBE1, 0x1);// muti core enable

#ifdef DYN_CACHE
		WRITE_VREG(HEVCD_IPP_DYN_CACHE,0x2b);//enable new mcrcc
		WRITE_VREG(HEVCD_IPP_DYN_CACHE_DBE1,0x2b);//enable new mcrcc
		av1_print(hw, AOM_DEBUG_HW_MORE, "HEVC DYN MCRCC\n");
#endif

		WRITE_VREG(HEVC_DBLK_MCP, 0x4);//dual core - core 0
		WRITE_VREG(HEVC_DBLK_MCP_DBE1, 0x5);//dual core - core 1

		// Initialize mcrcc and decomp perf counters
		mcrcc_perfcount_reset_dual(hw);
		decomp_perfcount_reset_dual(hw);
	}
	if (back_flag &&
		(dw_mode & 0x10)) {
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, 0x1 << 31); // Enable NV21 reference read mode for MC
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, 0x1 << 31); // Enable NV21 reference read mode for MC
	}
	av1_print(hw, AOM_DEBUG_HW_MORE, "[test.c] Leaving aom_init_decoder_hw\n");

	return;
}

static void init_pic_list_hw_fb(struct AV1HW_s *hw)
{
	int32_t i;
	struct AV1_Common_s *const cm = &hw->common;
	PIC_BUFFER_CONFIG* pic_config;
	int dw_mode = get_double_write_mode(hw);
	hw->pbi->ins_offset = 0;

	WRITE_BACK_8(hw, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, (0x1 << 1) | (0x1 << 2));

	for (i=0; i<FRAME_BUFFERS; i++) {
		pic_config = &cm->buffer_pool->frame_bufs[i].buf;
		if (pic_config->index >= 0) {
			if (dw_mode != 0x10) {
				WRITE_BACK_32(hw, HEVCD_MPP_ANC2AXI_TBL_DATA, pic_config->header_adr>>5);
			} else {
				WRITE_BACK_32(hw, HEVCD_MPP_ANC2AXI_TBL_DATA, pic_config->dw_y_adr>>5);
			}

#ifdef AOM_AV1_MMU_DW
			/*to do ..*/
#endif

#ifndef LOSLESS_COMPRESS_MODE
			WRITE_BACK_32(hw, HEVCD_MPP_ANC2AXI_TBL_DATA, pic_config->dw_u_v_adr>>5);
#else
			if (dw_mode & 0x10) {
				WRITE_BACK_32(hw, HEVCD_MPP_ANC2AXI_TBL_DATA, pic_config->dw_u_v_adr>>5);
			}
#endif
		}
	}
	WRITE_BACK_8(hw, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x1);

#if 0
	if (first_flag) {
	// Zero out canvas registers in IPP -- avoid simulation X
	WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (1 << 8) | (0<<1) | 1);
	WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR_DBE1, (1 << 8) | (0<<1) | 1);
	for (i=0; i<32; i++) {
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR_DBE1, 0);
		}
	}
#endif
}

#if 0
static void print_scratch_error(int32_t error_num)
{
	printk(" ERROR : HEVC_ASSIST_SCRATCH_TEST Error : %d\n", error_num);
}
#endif

void av1_hw_init(struct AV1HW_s *hw, int first_flag, int front_flag, int back_flag)
{
	uint32_t data32;
	int32_t decode_pic_begin = 0;//picParam[2];
	int32_t decode_pic_num = 0;//picParam[3];
	uint32_t tmp = 0;
	int dw_mode = get_double_write_mode(hw);

	if (hw->front_back_mode != 1) {
		if (front_flag)
		av1_hw_ctx_restore(hw);
		if (back_flag) {
		/* clear mailbox interrupt */
		WRITE_VREG(hw->ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(hw->ASSIST_MBOX0_MASK, 1);

			hw->stat |= STAT_ISR_REG;
		}
		return;
	}

	if (front_flag) {
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 | (3 << 7) | (1 << 10);
		tmp = (1 << 0) | (1 << 1) | (1 << 9);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32);
	}

	if (back_flag) {
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 | (3 << 7) | (1 << 11) | (1 << 12);
		tmp = (1 << 2) | (1 << 3) |  (1 << 13) | (1 << 5) | (1 << 6) | (1 << 14);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32);
	}

	aom_config_work_space_hw_fb(hw, front_flag, back_flag);

	if (back_flag) {
		/*
		if (picParam[4] == 1) {
		printk("[test.c] DoubleWrite Force 1:1 Compress.\n");
		WRITE_VREG(HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) | 1);  // used un-used bit to tel microcode set to compress 1:1
		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, READ_VREG(HEVC_SAO_CTRL5_DBE1) | 1);  // used un-used bit to tel microcode set to compress 1:1
		} else if (picParam[4] == 2) {
		printk("[test.c] DoubleWrite Force 2:1 Compress.\n");
		WRITE_VREG(HEVC_SAO_CTRL5, READ_VREG(HEVC_SAO_CTRL5) | 2);  // used un-used bit to tel microcode set to compress 2:1
		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, READ_VREG(HEVC_SAO_CTRL5_DBE1) | 2);  // used un-used bit to tel microcode set to compress 2:1
		}
		*/

#ifdef AOM_AV1_MMU
		if (dw_mode != 0x10) {
			WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR, hw->frame_mmu_map_phy_addr);
			WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR_DBE1, hw->frame_mmu_map_phy_addr_1); //new dual
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL, hw->frame_mmu_map_phy_addr);
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL_DBE1, hw->frame_mmu_map_phy_addr_1);
		}
#ifdef AOM_AV1_MMU_DW
		if (dw_mode & 0x20) {
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2, hw->dw_frame_mmu_map_phy_addr);
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2_DBE1, hw->dw_frame_mmu_map_phy_addr_1); //new dual
			//printk("WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2, 0x%x\n", hw->frame_mmu_map_phy_addr);
			//printk("WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2_DBE1, 0x%x\n", hw->frame_mmu_map_phy_addr_1);
		}
#endif
#endif
	}

	aom_init_decoder_hw_fb(hw, decode_pic_begin, decode_pic_num, first_flag, front_flag, back_flag);

	if (back_flag) {
		/* clear mailbox interrupt */
		WRITE_VREG(hw->backend_ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(hw->backend_ASSIST_MBOX0_MASK, 1);

#ifdef VPU_FILMGRAIN_DUMP
		WRITE_VREG(HEVC_FGS_TABLE_START, VPU_FILMGRAIN_FGS_TABLE_ADDR - 0x2000);
		WRITE_VREG(HEVC_FGS_TABLE_START_DBE1, VPU_FILMGRAIN_FGS_TABLE_ADDR - 0x2000);
#endif

#ifdef AOM_AV1_DBLK_INIT
		av1_print(hw, AOM_DEBUG_HW_MORE,
			"[test.c] av1_loop_filter_init (run once before decoding start)\n");
		av1_loop_filter_init_fb(hw);
#endif
		if (!efficiency_mode) {
#ifdef AOM_AV1_DBLK_INIT
			// video format is AOM_AV1
			data32 = (0x57 << 8) |  // 1st/2nd write both enable
					 (0x4  << 0);   // aom_av1 video format
			WRITE_VREG(HEVC_DBLK_CFGB, data32);
			WRITE_VREG(HEVC_DBLK_CFGB_DBE1, data32);
			av1_print(hw, AOM_DEBUG_HW_MORE," [DBLK DEBUG] CFGB : 0x%x\n", data32);
#endif

			// Set MCR fetch priorities
			data32 = 0x1 | (0x1 << 2) | (0x1 <<3) | (24 << 4) | (32 << 11) | (24 << 18) | (32 << 25);
			WRITE_VREG(HEVCD_MPP_DECOMP_AXIURG_CTL, data32);
			WRITE_VREG(HEVCD_MPP_DECOMP_AXIURG_CTL_DBE1, data32);
		}
	}

	if (front_flag) {
	//Start JT
#if 1
		if (debug & AV1_DEBUG_BUFMGR)
			printk("[test.c] Enable BitStream Fetch\n");
#if 0
		data32 = READ_VREG(HEVC_STREAM_CONTROL);
		data32 = data32 |
			(1 << 0) // stream_fetch_enable
			;
		WRITE_VREG(HEVC_STREAM_CONTROL, data32);

		if (debug & AV1_DEBUG_BUFMGR)
			printk("[test.c] Config STREAM_FIFO_CTL\n");
		data32 = READ_VREG(HEVC_STREAM_FIFO_CTL);
		data32 = data32 | (1 << 29); // stream_fifo_hole
		WRITE_VREG(HEVC_STREAM_FIFO_CTL, data32);

		if (first_flag) {
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

			WRITE_VREG(HEVC_STREAM_PACKET_LENGTH, 0x11223344);
			data32 = READ_VREG(HEVC_STREAM_PACKET_LENGTH);
			if (data32 != 0x11223344) { print_scratch_error(33); return; }

		}
		WRITE_VREG(HEVC_STREAM_PACKET_LENGTH, 0);
#endif
		if (!efficiency_mode) {
			WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x00000001); // AOM_AV1 use 4 Bytes Startcode
			WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x00000300);
		}
#endif
	// End JT
#if 1 // JT
		WRITE_VREG(HEVC_WAIT_FLAG, 1);
		/* disable PSCALE for hardware sharing */

#ifdef PXP_CODE
		/* clear mailbox interrupt */
		WRITE_VREG(hw->ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(hw->ASSIST_MBOX0_MASK, 1);
#endif
#ifdef DOS_PROJECT
#else
		WRITE_VREG(HEVC_PSCALE_CTRL, 0);
#endif
		//WRITE_VREG(DEBUG_REG1, 0x0);
#ifdef PXP_CODE
		WRITE_VREG(NAL_SEARCH_CTL, 0x8);
		WRITE_VREG(DECODE_STOP_POS, udebug_flag);
#else
		WRITE_VREG(DECODE_STOP_POS, 0x0);
#endif

		//WRITE_VREG(XIF_DOS_SCRATCH31, 0x0);
#if 0
		WRITE_VREG(P_HEVC_MPSR, 1);
		WRITE_VREG(P_HEVC_MPSR_DBE, 1);
#endif
#endif
		}
	if (!efficiency_mode && front_flag)
		init_pic_list_hw_fb(hw);
}

static int32_t config_mc_buffer_fb(struct AV1HW_s * hw)
{
	int32_t i;
	AV1_COMMON *cm = hw->pbi->common;
	PIC_BUFFER_CONFIG *cur_pic_config = &cm->cur_frame->buf;
	uint16_t bit_depth = cur_pic_config->bit_depth;
	unsigned char inter_flag = cur_pic_config->inter_flag;
	uint8_t scale_enable = 0;
	av1_print(hw, AOM_DEBUG_HW_MORE, " #### config_mc_buffer %s ####\n", inter_flag ? "inter" : "intra");

#ifdef DEBUG_PRINT
	if (debug & AOM_AV1_DEBUG_BUFMGR) printk("config_mc_buffer entered .....\n");
#endif

	WRITE_BACK_8(hw, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);
	WRITE_BACK_32(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
	(cur_pic_config->order_hint<<24) |
	(cur_pic_config->mc_canvas_u_v<<16)|(cur_pic_config->mc_canvas_u_v<<8)|cur_pic_config->mc_canvas_y);
	for (i = LAST_FRAME; i <= ALTREF_FRAME; i++) {
		PIC_BUFFER_CONFIG *pic_config; //cm->frame_refs[i].buf;
#ifdef NEW_FRONT_BACK_CODE
		if (inter_flag)
		pic_config = cur_pic_config->pic_refs[i];
		else
		pic_config = cur_pic_config;
#else
		if (inter_flag)
		pic_config = av1_get_ref_frame_spec_buf(cm, i);
		else
		pic_config = cur_pic_config;
#endif
		if (pic_config) {
		WRITE_BACK_32(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
			(pic_config->order_hint<<24) |
			(pic_config->mc_canvas_u_v<<16)|(pic_config->mc_canvas_u_v<<8)|pic_config->mc_canvas_y);
		if (inter_flag)
			av1_print(hw, AOM_DEBUG_HW_MORE, "refid 0x%x mc_canvas_u_v 0x%x mc_canvas_y 0x%x order_hint 0x%x\n",
			i,pic_config->mc_canvas_u_v,pic_config->mc_canvas_y,pic_config->order_hint);
		} else {
		WRITE_BACK_8(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
		}
	}

	WRITE_BACK_16(hw, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (16 << 8) | (0<<1) | 1);
	WRITE_BACK_32(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
	(cur_pic_config->order_hint<<24)|
	(cur_pic_config->mc_canvas_u_v<<16)|(cur_pic_config->mc_canvas_u_v<<8)|cur_pic_config->mc_canvas_y);
	for (i = LAST_FRAME; i <= ALTREF_FRAME; i++) {
		PIC_BUFFER_CONFIG *pic_config;
#ifdef NEW_FRONT_BACK_CODE
		if (inter_flag)
		pic_config = cur_pic_config->pic_refs[i];
		else
		pic_config = cur_pic_config;
#else
		if (inter_flag)
		pic_config = av1_get_ref_frame_spec_buf(cm, i);
		else
		pic_config = cur_pic_config;
#endif
		if (pic_config) {
		WRITE_BACK_32(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
			(pic_config->order_hint<<24)|
			(pic_config->mc_canvas_u_v<<16)|(pic_config->mc_canvas_u_v<<8)|pic_config->mc_canvas_y);
		} else {
		WRITE_BACK_8(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
		}
	}

	WRITE_BACK_8(hw, VP9D_MPP_REFINFO_TBL_ACCCONFIG, (0x1 << 2) | (0x0 <<3)); // auto_inc start index:0 field:0
	for (i = 0; i <= ALTREF_FRAME; i++)
	{
		int32_t ref_pic_body_size;
		struct scale_factors * sf = NULL;
		PIC_BUFFER_CONFIG *pic_config;

#ifdef NEW_FRONT_BACK_CODE
		if (inter_flag && i >= LAST_FRAME)
		pic_config = cur_pic_config->pic_refs[i];
		else
		pic_config = cur_pic_config;
#else
		if (inter_flag && i >= LAST_FRAME)
		pic_config = av1_get_ref_frame_spec_buf(cm, i);
		else
		pic_config = cur_pic_config;
#endif
		if (pic_config) {
			ref_pic_body_size = compute_losless_comp_body_size(pic_config->y_crop_width, pic_config->y_crop_height, (bit_depth == AOM_BITS_10));

			WRITE_BACK_32(hw, VP9D_MPP_REFINFO_DATA, pic_config->y_crop_width);
			WRITE_BACK_32(hw, VP9D_MPP_REFINFO_DATA, pic_config->y_crop_height);
		if (inter_flag && i >= LAST_FRAME) {
			av1_print(hw, AOM_DEBUG_HW_MORE, "refid %d: ref width/height(%d,%d), cur width/height(%d,%d) ref_pic_body_size 0x%x\n",
				i, pic_config->y_crop_width, pic_config->y_crop_height,
				cur_pic_config->y_crop_width, cur_pic_config->y_crop_height,
				ref_pic_body_size);
		}

		} else {
			ref_pic_body_size = 0;
			WRITE_BACK_8(hw, VP9D_MPP_REFINFO_DATA, 0);
			WRITE_BACK_8(hw, VP9D_MPP_REFINFO_DATA, 0);
		}

#ifdef NEW_FRONT_BACK_CODE
		if (inter_flag && i >= LAST_FRAME)
			sf = &cur_pic_config->ref_scale_factors[i];
#else
		if (inter_flag && i >= LAST_FRAME)
			sf = av1_get_ref_scale_factors(cm, i);
#endif
		if (sf != NULL && av1_is_scaled(sf)) {
			scale_enable |= (1<<i);
		}

		if (sf) {
			WRITE_BACK_32(hw, VP9D_MPP_REFINFO_DATA, sf->x_scale_fp);
			WRITE_BACK_32(hw, VP9D_MPP_REFINFO_DATA, sf->y_scale_fp);

			av1_print(hw, AOM_DEBUG_HW_MORE, "x_scale_fp %d, y_scale_fp %d\n", sf->x_scale_fp, sf->y_scale_fp);
		} else {
			WRITE_BACK_16(hw, VP9D_MPP_REFINFO_DATA, 0, REF_NO_SCALE); //1<<14
			WRITE_BACK_16(hw, VP9D_MPP_REFINFO_DATA, 0, REF_NO_SCALE);
		}
		if (get_double_write_mode(hw) != 0x10) {
			WRITE_BACK_8(hw, VP9D_MPP_REFINFO_DATA, 0);
		} else {
			WRITE_BACK_32(hw, VP9D_MPP_REFINFO_DATA, ref_pic_body_size >> 5);
		}
	}
	WRITE_BACK_8(hw, VP9D_MPP_REF_SCALE_ENBL, scale_enable);
#ifndef NEW_FRONT_BACK_CODE
	WRITE_BACK_8(hw, PARSER_REF_SCALE_ENBL, scale_enable);
#endif
	av1_print(hw, AOM_DEBUG_HW_MORE, "WRITE_VREG(P_PARSER_REF_SCALE_ENBL, 0x%x)\n", scale_enable);

	return 0;
}

static   u32   mcrcc_hit_rate_0;
static   u32   mcrcc_hit_rate_1;

static   u32   mcrcc_bypass_rate_0;
static   u32   mcrcc_bypass_rate_1;

static void mcrcc_get_hitrate_dual(int pic_num)
{
	unsigned   tmp;
	unsigned   raw_mcr_cnt;
	unsigned   hit_mcr_cnt;
	unsigned   byp_mcr_cnt_nchoutwin;
	unsigned   byp_mcr_cnt_nchcanv;
	unsigned   hit_mcr_0_cnt;
	unsigned   hit_mcr_1_cnt;
	u32      hitrate;
	printk("[cache_util.c] Entered mcrcc_get_hitrate_dual...\n");

	printk("[MCRCC CORE ] Picture : %d\n",pic_num);

	// CORE 0
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x0<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x1<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x2<<1));
	byp_mcr_cnt_nchoutwin = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x3<<1));
	byp_mcr_cnt_nchcanv = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);

	printk("[MCRCC CORE0] raw_mcr_cnt: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE0] hit_mcr_cnt: %d\n",hit_mcr_cnt);
	printk("[MCRCC CORE0] byp_mcr_cnt_nchoutwin: %d\n",byp_mcr_cnt_nchoutwin);
	printk("[MCRCC CORE0] byp_mcr_cnt_nchcanv: %d\n",byp_mcr_cnt_nchcanv);

	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x4<<1));
	tmp = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	printk("[MCRCC CORE0] miss_mcr_0_cnt: %d\n",tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x5<<1));
	tmp = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	printk("[MCRCC CORE0] miss_mcr_1_cnt: %d\n",tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x6<<1));
	hit_mcr_0_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	printk("[MCRCC CORE0] hit_mcr_0_cnt: %d\n",hit_mcr_0_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x7<<1));
	hit_mcr_1_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	printk("[MCRCC CORE0] hit_mcr_1_cnt: %d\n",hit_mcr_1_cnt);

	if ( raw_mcr_cnt != 0 ) {
		hitrate = (hit_mcr_0_cnt*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE0] CANV0_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
		hitrate = (hit_mcr_1_cnt*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE0] CANV1_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
		hitrate = (byp_mcr_cnt_nchcanv*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE0] NONCACH_CANV_BYP_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
		hitrate = (byp_mcr_cnt_nchoutwin*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE0] CACHE_OUTWIN_BYP_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
	}

	if ( raw_mcr_cnt != 0 )
	{
		hitrate = (hit_mcr_cnt*100/raw_mcr_cnt)*100;
			printk("[P%d MCRCC CORE0] MCRCC_HIT_RATE : %d.%d%%\n", pic_num, hitrate/100, hitrate%100);
		hitrate = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv)*100/raw_mcr_cnt)*100;
			printk("[P%d MCRCC CORE0] MCRCC_BYP_RATE : %d.%d%%\n", pic_num, hitrate/100, hitrate%100);
	} else
	{
			printk("[P%d MCRCC CORE0] MCRCC_HIT_RATE : na\n", pic_num);
			printk("[P%d MCRCC CORE0] MCRCC_BYP_RATE : na\n", pic_num);
	}

	mcrcc_hit_rate_0 = (hit_mcr_cnt/raw_mcr_cnt);
	mcrcc_bypass_rate_0 = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv)/raw_mcr_cnt);

	// CORE 0
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x0<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x1<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x2<<1));
	byp_mcr_cnt_nchoutwin = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x3<<1));
	byp_mcr_cnt_nchcanv = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);

	printk("[MCRCC CORE1] raw_mcr_cnt: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE1] hit_mcr_cnt: %d\n",hit_mcr_cnt);
	printk("[MCRCC CORE1] byp_mcr_cnt_nchoutwin: %d\n",byp_mcr_cnt_nchoutwin);
	printk("[MCRCC CORE1] byp_mcr_cnt_nchcanv: %d\n",byp_mcr_cnt_nchcanv);

	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x4<<1));
	tmp = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);
	printk("[MCRCC CORE1] miss_mcr_0_cnt: %d\n",tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x5<<1));
	tmp = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);
	printk("[MCRCC CORE1] miss_mcr_1_cnt: %d\n",tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x6<<1));
	hit_mcr_0_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);
	printk("[MCRCC CORE1] hit_mcr_0_cnt: %d\n",hit_mcr_0_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x7<<1));
	hit_mcr_1_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA_DBE1);
	printk("[MCRCC CORE1] hit_mcr_1_cnt: %d\n",hit_mcr_1_cnt);

	if ( raw_mcr_cnt != 0 ) {
		hitrate = (hit_mcr_0_cnt*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE1] CANV0_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
		hitrate = (hit_mcr_1_cnt*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE1] CANV1_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
		hitrate = (byp_mcr_cnt_nchcanv*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE1] NONCACH_CANV_BYP_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
		hitrate = (byp_mcr_cnt_nchoutwin*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE1] CACHE_OUTWIN_BYP_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
	}

	if ( raw_mcr_cnt != 0 )
	{
		hitrate = (hit_mcr_cnt*100/raw_mcr_cnt)*100;
			printk("[P%d MCRCC CORE1] MCRCC_HIT_RATE : %d.%d%%\n", pic_num, hitrate/100, hitrate%100);
		hitrate = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv)*100/raw_mcr_cnt)*100;
			printk("[P%d MCRCC CORE1] MCRCC_BYP_RATE : %d.%d%%\n", pic_num, hitrate/100, hitrate%100);
	} else
	{
			printk("[P%d MCRCC CORE1] MCRCC_HIT_RATE : na\n", pic_num);
			printk("[P%d MCRCC CORE1] MCRCC_BYP_RATE : na\n", pic_num);
	}

	mcrcc_hit_rate_1 = (hit_mcr_cnt/raw_mcr_cnt);
	mcrcc_bypass_rate_1 = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv)/raw_mcr_cnt);

	return;
}

static void decomp_get_hitrate_dual(int pic_num)
{
	unsigned   raw_mcr_cnt;
	unsigned   hit_mcr_cnt;
	u32      hitrate;
	printk("[cache_util.c] Entered decomp_get_hitrate_dual...\n");

	// CORE0
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x0<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x1<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);

	printk("[MCRCC CORE0] hcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE0] hcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if ( raw_mcr_cnt != 0 )
	{
		hitrate = ((hit_mcr_cnt*100)/raw_mcr_cnt)*100;
			printk("[MCRCC CORE0] DECOMP_HCACHE_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
	} else
	{
			printk("[MCRCC CORE0] DECOMP_HCACHE_HIT_RATE : na\n");
	}
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x2<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x3<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);

	printk("[MCRCC CORE0] dcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE0] dcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE0] DECOMP_DCACHE_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);

		hitrate = (hit_mcr_cnt*100/raw_mcr_cnt);
		hitrate = (mcrcc_hit_rate_0 + (mcrcc_bypass_rate_0 * hitrate))*100;
			printk("[MCRCC CORE0] MCRCC_DECOMP_DCACHE_EFFECTIVE_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);

	} else {
			printk("[MCRCC CORE0] DECOMP_DCACHE_HIT_RATE : na\n");
	}

	// CORE1
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x0<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x1<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1);

	printk("[MCRCC CORE1] hcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE1] hcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if ( raw_mcr_cnt != 0 )
	{
		hitrate = (hit_mcr_cnt*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE1] DECOMP_HCACHE_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);
	} else
	{
			printk("[MCRCC CORE1] DECOMP_HCACHE_HIT_RATE : na\n");
	}
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x2<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x3<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1);

	printk("[MCRCC CORE1] dcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE1] dcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt*100/raw_mcr_cnt)*100;
			printk("[MCRCC CORE1] DECOMP_DCACHE_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);

		hitrate = (hit_mcr_cnt*100/raw_mcr_cnt);
		hitrate = (mcrcc_hit_rate_1 + (mcrcc_bypass_rate_1 * hitrate))*100;
			printk("[MCRCC CORE1] MCRCC_DECOMP_DCACHE_EFFECTIVE_HIT_RATE : %d.%d%%\n", hitrate/100, hitrate%100);

	} else {
			printk("[MCRCC CORE1] DECOMP_DCACHE_HIT_RATE : na\n");
	}

	return;
}

static void decomp_get_comprate_dual(int pic_num)
{
	unsigned   raw_ucomp_cnt;
	unsigned   fast_comp_cnt;
	unsigned   slow_comp_cnt;
	u32      comprate;

	printk("[cache_util.c] Entered decomp_get_comprate_dual...\n");

	// CORE0
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x4<<1));
	fast_comp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x5<<1));
	slow_comp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x6<<1));
	raw_ucomp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);

	printk("[MCRCC CORE0] decomp_fast_comp_total: %d\n",fast_comp_cnt);
	printk("[MCRCC CORE0] decomp_slow_comp_total: %d\n",slow_comp_cnt);
	printk("[MCRCC CORE0] decomp_raw_uncomp_total: %d\n",raw_ucomp_cnt);

	if ( raw_ucomp_cnt != 0 )
	{
		comprate = ((fast_comp_cnt + slow_comp_cnt)*100/raw_ucomp_cnt)*100;
			printk("[MCRCC CORE0] DECOMP_COMP_RATIO : %d.%d%%\n", comprate/100, comprate%100);
	} else
	{
			printk("[MCRCC CORE0] DECOMP_COMP_RATIO : na\n");
	}

	// CORE1
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x4<<1));
	fast_comp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x5<<1));
	slow_comp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x6<<1));
	raw_ucomp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1);

	printk("[MCRCC CORE1] decomp_fast_comp_total: %d\n",fast_comp_cnt);
	printk("[MCRCC CORE1] decomp_slow_comp_total: %d\n",slow_comp_cnt);
	printk("[MCRCC CORE1] decomp_raw_uncomp_total: %d\n",raw_ucomp_cnt);

	if ( raw_ucomp_cnt != 0 ) {
		comprate = ((fast_comp_cnt + slow_comp_cnt)*100/raw_ucomp_cnt)*100;
			printk("[MCRCC CORE1] DECOMP_COMP_RATIO : %d.%d%%\n", comprate/100, comprate%100);
	} else {
			printk("[MCRCC CORE1] DECOMP_COMP_RATIO : na\n");
	}

	return;
}

#ifdef NEW_FRONT_BACK_CODE // will call before reset_b
void print_mcrcc_hit_info(int pic_num) {
	//printk("before call mcrcc_get_hitrate\r\n");
	mcrcc_get_hitrate_dual(pic_num);
	decomp_get_hitrate_dual(pic_num);
	decomp_get_comprate_dual(pic_num);
}
#endif

static void  config_mcrcc_axi_hw_nearest_ref_fb(struct AV1HW_s * hw)
{
	uint32_t i;
	//uint32_t rdata32;
	uint32_t dist_array[8];
	uint32_t refcanvas_array[2];
	uint32_t orderhint_bits;
	unsigned char is_inter;
	int cindex0;
	uint32_t last_ref_orderhint_dist; // large distance
	uint32_t curr_ref_orderhint_dist; // large distance
	int cindex1;
#if (defined NEW_FRONT_BACK_CODE)&&(!defined FB_BUF_DEBUG_NO_PIPLINE)
	AV1_COMMON *cm = &hw->common;
	PIC_BUFFER_CONFIG *curr_pic_config = &cm->cur_frame->buf;
#else
	AV1_COMMON *cm = &hw->common;
	PIC_BUFFER_CONFIG *curr_pic_config;
#endif
	int32_t  curr_orderhint;

	av1_print(hw, AOM_DEBUG_HW_MORE, "[test.c] #### config_mcrcc_axi_hw ####\n");

	WRITE_BACK_8(hw, HEVCD_MCRCC_CTL1, 0x2); // reset mcrcc

#if (defined NEW_FRONT_BACK_CODE)&&(!defined FB_BUF_DEBUG_NO_PIPLINE)
	is_inter = curr_pic_config->inter_flag;
#else
	is_inter = av1_frame_is_inter(&hw->common); //((pbi->common.frame_type != KEY_FRAME) && (!pbi->common.intra_only)) ? 1 : 0;
#endif
	if ( !is_inter ) { // I-PIC
		//WRITE_VREG(P_HEVCD_MCRCC_CTL1, 0x1); // remove reset -- disables clock

		WRITE_BACK_32(hw, HEVCD_MCRCC_CTL2, 0xffffffff); // Replace with current-frame canvas
		WRITE_BACK_32(hw, HEVCD_MCRCC_CTL3, 0xffffffff); //
		WRITE_BACK_16(hw, HEVCD_MCRCC_CTL1, 0, 0xff0); // enable mcrcc progressive-mode
		return;
	}

#ifndef NEW_FRONT_BACK_CODE
	//printk("before call mcrcc_get_hitrate\r\n");
	mcrcc_get_hitrate(hw, hw->m_ins_flag);
	decomp_get_hitrate(hw);
	decomp_get_comprate(hw);
#endif

	// Find absolute orderhint delta
#if (defined NEW_FRONT_BACK_CODE)&&(!defined FB_BUF_DEBUG_NO_PIPLINE)
	orderhint_bits = curr_pic_config->order_hint_bits_minus_1;
#else
	curr_pic_config =  &cm->cur_frame->buf;
	orderhint_bits = cm->seq_params.order_hint_info.order_hint_bits_minus_1;
#endif
	curr_orderhint = curr_pic_config->order_hint;
	for (i = LAST_FRAME; i <= ALTREF_FRAME; i++) {
		int32_t ref_orderhint = 0;
		PIC_BUFFER_CONFIG *pic_config;
		//int32_t  tmp;

#if (defined NEW_FRONT_BACK_CODE)&&(!defined FB_BUF_DEBUG_NO_PIPLINE)
		pic_config = curr_pic_config->pic_refs[i];
#else
		pic_config = av1_get_ref_frame_spec_buf(cm,i);
#endif
		if (pic_config)
			ref_orderhint = pic_config->order_hint;
		//tmp = curr_orderhint - ref_orderhint;
		//dist_array[i] =  (tmp < 0) ? -tmp : tmp;
		dist_array[i] =  mcrcc_get_abs_frame_distance(hw, i, ref_orderhint, curr_orderhint, orderhint_bits);;
	}

	// Get smallest orderhint distance refid
	cindex0 = LAST_FRAME;
	last_ref_orderhint_dist = 1023; // large distance
	curr_ref_orderhint_dist = 1023; // large distance
	for (i = LAST_FRAME; i <= ALTREF_FRAME; i++) {
		PIC_BUFFER_CONFIG *pic_config;
#if (defined NEW_FRONT_BACK_CODE)&&(!defined FB_BUF_DEBUG_NO_PIPLINE)
		pic_config = curr_pic_config->pic_refs[i];
#else
		pic_config = av1_get_ref_frame_spec_buf(cm, i);
#endif
		curr_ref_orderhint_dist = dist_array[i];
		if ( curr_ref_orderhint_dist < last_ref_orderhint_dist) {
		cindex0 = i;
		last_ref_orderhint_dist = curr_ref_orderhint_dist;
		}
	}
	WRITE_BACK_8(hw, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (cindex0 << 8) | (1<<1) | 0);
	refcanvas_array[0] = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR) & 0xffff;

	last_ref_orderhint_dist = 1023; // large distance
	curr_ref_orderhint_dist = 1023; // large distance
	// Get 2nd smallest orderhint distance refid
	cindex1 = LAST_FRAME;
	for (i = LAST_FRAME; i <= ALTREF_FRAME; i++) {
		PIC_BUFFER_CONFIG *pic_config;
#if (defined NEW_FRONT_BACK_CODE)&&(!defined FB_BUF_DEBUG_NO_PIPLINE)
		pic_config = curr_pic_config->pic_refs[i];
#else
		pic_config = av1_get_ref_frame_spec_buf(cm, i);
#endif
		curr_ref_orderhint_dist = dist_array[i];
		WRITE_BACK_8(hw, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (i << 8) | (1<<1) | 0);
		refcanvas_array[1] = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR) & 0xffff;
		av1_print(hw, AOM_DEBUG_HW_MORE, "[cache_util.c] curr_ref_orderhint_dist:%x last_ref_orderhint_dist:%x refcanvas_array[0]:%x refcanvas_array[1]:%x\n",
		curr_ref_orderhint_dist, last_ref_orderhint_dist, refcanvas_array[0],refcanvas_array[1]);
		if ( (curr_ref_orderhint_dist < last_ref_orderhint_dist) && (refcanvas_array[0] != refcanvas_array[1])) {
		cindex1 = i;
		last_ref_orderhint_dist = curr_ref_orderhint_dist;
		}
	}

	WRITE_BACK_8(hw, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (cindex0 << 8) | (1<<1) | 0);
	refcanvas_array[0] = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
	WRITE_BACK_8(hw, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (cindex1 << 8) | (1<<1) | 0);
	refcanvas_array[1] = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);

	av1_print(hw, AOM_DEBUG_HW_MORE, "[cache_util.c] refcanvas_array[0](index %d):%x refcanvas_array[1](index %d):%x\n",
		cindex0, refcanvas_array[0], cindex1, refcanvas_array[1]);

	// lowest delta_picnum
	//rdata32 = refcanvas_array[0];
	//rdata32 = rdata32 & 0xffff;
	//rdata32 = rdata32 | ( rdata32 << 16);
	//WRITE_VREG(HEVCD_MCRCC_CTL2, rdata32);
	//WRITE_VREG(HEVCD_MCRCC_CTL2_DBE1, rdata32);
	READ_INS_WRITE(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL2, 0, 16, 16);

	// 2nd-lowest delta_picnum
	//rdata32 = refcanvas_array[1];
	//rdata32 = rdata32 & 0xffff;
	//rdata32 = rdata32 | ( rdata32 << 16);
	//WRITE_VREG(HEVCD_MCRCC_CTL3, rdata32);
	//WRITE_VREG(HEVCD_MCRCC_CTL3_DBE1, rdata32);
	READ_INS_WRITE(hw, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL3, 0, 16, 16);

	WRITE_BACK_16(hw, HEVCD_MCRCC_CTL1, 0, 0xff0); // enable mcrcc progressive-mode
	return;
}

static void config_sao_hw_fb(struct AV1HW_s *hw, param_t* params)
{
	uint32_t data32; //, data32_2;
	AV1_COMMON *const cm = hw->pbi->common;
	PIC_BUFFER_CONFIG* pic_config = &cm->cur_frame->buf;
	//AV1Decoder* pbi = hw->pbi;
	//int32_t misc_flag0 = pbi->misc_flag0;
	//int32_t slice_deblocking_filter_disabled_flag = 0;
	BuffInfo_t* buf_spec = hw->pbi->work_space_buf;
	int32_t lcu_size = ((params->p.seq_flags >> 6) & 0x1) ? 128 : 64;
	int32_t mc_buffer_size_u_v = pic_config->lcu_total*lcu_size*lcu_size/2;
	int32_t mc_buffer_size_u_v_h = (mc_buffer_size_u_v + 0xffff)>>16; //64k alignment
	int dw_mode = get_double_write_mode(hw);

	av1_print(hw, AOM_DEBUG_HW_MORE,
		"####[config_sao_hw]#### lcu_size %d, lcu_total %d, mc_y_adr 0x%x, mc_uv_adr 0x%x, header_adr 0x%x, header_dw 0x%x\n",
		lcu_size, pic_config->lcu_total, pic_config->mc_y_adr, pic_config->mc_u_v_adr,
		pic_config->header_adr, pic_config->header_dw_adr);

	//data32 = READ_VREG(HEVC_SAO_CTRL9) | (1 << 1);
	//WRITE_VREG(HEVC_SAO_CTRL9, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL9, 1, 1, 1);

	data32 = READ_VREG(HEVC_SAO_CTRL5);
	data32 |= (0x1 << 14); /* av1 mode */
	data32 |= (0xff << 16); /* dw {v1,v0,h1,h0} ctrl_y_cbus */
	WRITE_BACK_32(hw, HEVC_SAO_CTRL5, data32);

	WRITE_BACK_8(hw, HEVC_SAO_CTRL0, lcu_size == 128 ? 0x7 : 0x6); /*lcu_size_log2*/
#ifdef LOSLESS_COMPRESS_MODE

#ifdef PXP_CODE
	//WRITE_VREG(HEVC_CM_BODY_START_ADDR, pic_config->mc_y_adr);
	//WRITE_VREG(HEVC_CM_BODY_START_ADDR_DBE1, pic_config->mc_y_adr);
	if (dw_mode &&
		(dw_mode & 0x20) == 0) {
		WRITE_BACK_32(hw, HEVC_SAO_Y_START_ADDR, pic_config->dw_y_adr);
	}
#else
	WRITE_BACK_32(hw, HEVC_SAO_Y_START_ADDR, DOUBLE_WRITE_YSTART_TEMP);
	WRITE_BACK_32(hw, HEVC_CM_BODY_START_ADDR, pic_config->dw_y_adr);
#endif

#ifdef AOM_AV1_MMU
	if ((dw_mode & 0x10) == 0) {
		WRITE_BACK_32(hw, HEVC_CM_HEADER_START_ADDR, pic_config->header_adr);
	}
#endif
#ifdef AOM_AV1_MMU_DW
	if (dw_mode & 0x20) {
		WRITE_BACK_32(hw, HEVC_CM_HEADER_START_ADDR2, pic_config->header_dw_adr);
	}
#endif

#else /*!LOSLESS_COMPRESS_MODE*/
	WRITE_BACK_32(hw, HEVC_SAO_Y_START_ADDR, pic_config->mc_y_adr);
#endif

	//printk("[config_sao_hw] sao_body_addr:%x\n", pic_config->mc_y_adr);
	//printk("[config_sao_hw] sao_header_addr:%x\n", pic_config->mc_y_adr + hw->losless_comp_body_size);

#ifdef VPU_FILMGRAIN_DUMP
	// Let Microcode to increase
	// WRITE_VREG(P_HEVC_FGS_TABLE_START, pic_config->fgs_table_adr);
#else
	WRITE_BACK_32(hw, HEVC_FGS_TABLE_START, pic_config->fgs_table_adr);
#endif
	WRITE_BACK_32(hw, HEVC_FGS_TABLE_LENGTH, FGS_TABLE_SIZE * 8);
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"[config_sao_hw] fgs_table adr:0x%x , length 0x%x bits\n",
		pic_config->fgs_table_adr, FGS_TABLE_SIZE * 8);

	data32 = (mc_buffer_size_u_v_h<<16)<<1;
	//printk("data32 = %x, mc_buffer_size_u_v_h = %x, lcu_total = %x\n", data32, mc_buffer_size_u_v_h, pic_config->lcu_total);
	WRITE_BACK_32(hw, HEVC_SAO_Y_LENGTH ,data32);

#ifndef LOSLESS_COMPRESS_MODE
	WRITE_BACK_32(hw, HEVC_SAO_C_START_ADDR, pic_config->dw_u_v_adr);
#else

#ifdef PXP_CODE
	if (dw_mode &&
		(dw_mode & 0x20) == 0) {
		WRITE_BACK_32(hw, HEVC_SAO_C_START_ADDR, pic_config->dw_u_v_adr);
	} else {
		//WRITE_VREG(HEVC_SAO_Y_START_ADDR, 0xffffffff);
		//WRITE_VREG(HEVC_SAO_C_START_ADDR, 0xffffffff);
	}
#else
	WRITE_BACK_32(hw, HEVC_SAO_C_START_ADDR, DOUBLE_WRITE_CSTART_TEMP);
#endif

#endif

	data32 = (mc_buffer_size_u_v_h<<16);
	WRITE_BACK_32(hw, HEVC_SAO_C_LENGTH  ,data32);

#ifndef LOSLESS_COMPRESS_MODE
	/* multi tile to do... */
	WRITE_BACK_32(hw, HEVC_SAO_Y_WPTR , pic_config->dw_y_adr);
	WRITE_BACK_32(hw, HEVC_SAO_C_WPTR , pic_config->dw_u_v_adr);
#else

#ifdef PXP_CODE
	if (dw_mode &&
		(dw_mode & 0x20) == 0) {
		WRITE_BACK_32(hw, HEVC_SAO_Y_WPTR, pic_config->dw_y_adr);
		WRITE_BACK_32(hw, HEVC_SAO_C_WPTR, pic_config->dw_u_v_adr);
	}
#else
	WRITE_BACK_32(hw, HEVC_SAO_Y_WPTR ,DOUBLE_WRITE_YSTART_TEMP);
	WRITE_BACK_32(hw, HEVC_SAO_C_WPTR ,DOUBLE_WRITE_CSTART_TEMP);
#endif
#endif

#ifndef AOM_AV1_NV21
#ifdef AOM_AV1_MMU_DW

#ifdef PXP_CODE
		if (hw->dw_mmu_enable) {
			WRITE_BACK_32(hw, HEVC_DW_VH0_ADDDR, buf_spec->mmu_vbh_dw.buf_start);
			WRITE_BACK_32(hw, HEVC_DW_VH1_ADDDR, buf_spec->mmu_vbh_dw.buf_start
				+ (DW_VBH_BUF_SIZE(buf_spec)));
		}
#else
		WRITE_BACK_8(hw, HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
		WRITE_BACK_8(hw, HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
#endif

#endif
#endif

#ifdef AOM_AV1_NV21
#ifdef DOS_PROJECT
	//data32 = READ_VREG( HEVC_SAO_CTRL1);
	//data32 &= (~0x3000);
	//data32 |= (hw->mem_map_mode << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1, MEM_MAP_MODE, 12, 2);
	//data32 &= (~0x3);
	//data32 |= 0x1; // [1]:dw_disable [0]:cm_disable
	//WRITE_VREG(HEVC_SAO_CTRL1, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1, 0x1, 0, 2);

	//data32 = READ_VREG(HEVC_SAO_CTRL1_DBE1);
	//data32 &= (~0x3000);
	//data32 |= (hw->mem_map_mode << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1_DBE1, MEM_MAP_MODE, 12, 2);
	//data32 &= (~0x3);
	//data32 |= 0x1; // [1]:dw_disable [0]:cm_disable
	//WRITE_VREG(HEVC_SAO_CTRL1_DBE1, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1_DBE1, 0x1, 0, 2);

	//data32 = READ_VREG(HEVC_SAO_CTRL5); // [23:22] dw_v1_ctrl [21:20] dw_v0_ctrl [19:18] dw_h1_ctrl [17:16] dw_h0_ctrl
	//data32 &= ~(0xff << 16);			   // set them all 0 for AOM_AV1_NV21 (no down-scale)
	//WRITE_VREG(HEVC_SAO_CTRL5, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL5, 0, 16, 8);
	//data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1); // [23:22] dw_v1_ctrl [21:20] dw_v0_ctrl [19:18] dw_h1_ctrl [17:16] dw_h0_ctrl
	//data32 &= ~(0xff << 16);			   // set them all 0 for AOM_AV1_NV21 (no down-scale)
	//WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL5_DBE1, 0, 16, 8);

	//data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG);
	//data32 &= (~0x30);
	//data32 |= (hw->mem_map_mode << 4); // [5:4]	 -- address_format 00:linear 01:32x32 10:64x32
	//WRITE_VREG(HEVCD_IPP_AXIIF_CONFIG, data32);
	READ_WRITE_DATA16(hw, HEVCD_IPP_AXIIF_CONFIG, MEM_MAP_MODE, 4, 2);
	//data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG_DBE1);
	//data32 &= (~0x30);
	//data32 |= (hw->mem_map_mode << 4); // [5:4]	 -- address_format 00:linear 01:32x32 10:64x32
	//WRITE_VREG(HEVCD_IPP_AXIIF_CONFIG_DBE1, data32);
	READ_WRITE_DATA16(hw, HEVCD_IPP_AXIIF_CONFIG_DBE1, MEM_MAP_MODE, 4, 2);
#else
// m8baby test1902
	//data32 = READ_VREG(HEVC_SAO_CTRL1);
	//data32 &= (~0x3000);
	//data32 |= (hw->mem_map_mode << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1, hw->mem_map_mode, 12, 2);
	//data32 &= (~0xff0);
	//data32 |= 0x670;	// Big-Endian per 64-bit
	//data32 |= 0x880;	// Big-Endian per 64-bit
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1, 0x880, 0, 12);
	//data32 &= (~0x3);
	//data32 |= 0x1; // [1]:dw_disable [0]:cm_disable
	//WRITE_VREG(HEVC_SAO_CTRL1, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1, 0x1, 0, 2);

	//data32 = READ_VREG(HEVC_SAO_CTRL1_DBE1);
	//data32 &= (~0x3000);
	//data32 |= (hw->mem_map_mode << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1_DBE1, hw->mem_map_mode, 12, 2);
	//data32 &= (~0xff0);
	//data32 |= 0x670;	// Big-Endian per 64-bit
	//data32 |= 0x880;	// Big-Endian per 64-bit
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1_DBE1, 0x880, 0, 12);
	//data32 &= (~0x3);
	//data32 |= 0x1; // [1]:dw_disable [0]:cm_disable
	//WRITE_VREG(HEVC_SAO_CTRL1_DBE1, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL1_DBE1, 0x1, 0, 2);

	//data32 = READ_VREG(HEVC_SAO_CTRL5); // [23:22] dw_v1_ctrl [21:20] dw_v0_ctrl [19:18] dw_h1_ctrl [17:16] dw_h0_ctrl
	//data32 &= ~(0xff << 16);			   // set them all 0 for AOM_AV1_NV21 (no down-scale)
	//WRITE_VREG(HEVC_SAO_CTRL5, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL5, 0, 16, 8);

	//data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1); // [23:22] dw_v1_ctrl [21:20] dw_v0_ctrl [19:18] dw_h1_ctrl [17:16] dw_h0_ctrl
	//data32 &= ~(0xff << 16);			   // set them all 0 for AOM_AV1_NV21 (no down-scale)
	//WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
	READ_WRITE_DATA16(hw, HEVC_SAO_CTRL5_DBE1, 0, 16, 8);

	//data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG);
	//data32 &= (~0x30);
	//data32 |= (hw->mem_map_mode << 4); // [5:4]	 -- address_format 00:linear 01:32x32 10:64x32
	READ_WRITE_DATA16(hw, HEVCD_IPP_AXIIF_CONFIG, hw->mem_map_mode, 4, 2);
	//data32 &= (~0xF);
	//data32 |= 0x8;	  // Big-Endian per 64-bit
	//WRITE_VREG(HEVCD_IPP_AXIIF_CONFIG, data32);
	READ_WRITE_DATA16(hw, HEVCD_IPP_AXIIF_CONFIG, 0x8, 0, 4);

	//data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG_DBE1);
	//data32 &= (~0x30);
	//data32 |= (hw->mem_map_mode << 4); // [5:4]	 -- address_format 00:linear 01:32x32 10:64x32
	READ_WRITE_DATA16(hw, HEVCD_IPP_AXIIF_CONFIG_DBE1, hw->mem_map_mode, 4, 2);
	//data32 &= (~0xF);
	//data32 |= 0x8;	  // Big-Endian per 64-bit
	//WRITE_VREG(HEVCD_IPP_AXIIF_CONFIG_DBE1, data32);
	READ_WRITE_DATA16(hw, HEVCD_IPP_AXIIF_CONFIG_DBE1, 0x8, 0, 4);
#endif
#else
/*CHANGE_DONE nnn*/
	av1_print(hw, AOM_DEBUG_HW_MORE, "%s, mem_map_mode %d, endian %x\n",
		__func__, hw->mem_map_mode, hw->endian);
	//data32 = READ_VREG(HEVC_DBLK_CFGB);
	//data32 &= (~0x300); /*[8]:first write enable (compress)  [9]:double write enable (uncompress)*/
	if (dw_mode== 0)
		data32 = 1; //data32 |= (0x1 << 8); /*enable first write*/
	else if (dw_mode & 0x10)
		data32 = 2; //data32 |= (0x1 << 9); /*double write only*/
	else
		data32 = 3; //data32|= ((0x1 << 8)	|(0x1 << 9));
	READ_WRITE_DATA16(hw, HEVC_DBLK_CFGB, data32, 8, 2); //WRITE_VREG(HEVC_DBLK_CFGB, data32);

	data32 = READ_VREG(HEVC_SAO_CTRL1);
	data32 &= (~0x3000);
	data32 |= (hw->mem_map_mode << 12); /* [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32 */
	data32 &= (~0xff0); 				/* data32 |= 0x670;  // Big-Endian per 64-bit */
	if (hw->dw_mmu_enable == 0)
		data32 |= ((hw->endian >> 8) & 0xfff);	/* Big-Endian per 64-bit */

	data32 &= (~0x3); 					/*[1]:dw_disable [0]:cm_disable*/
	if (dw_mode == 0)
		data32 |= 0x2; 					/*disable double write*/
	else if (dw_mode & 0x10)
		data32 |= 0x1; 					/*disable cm*/
	data32 &= (~(3 << 14));
	data32 |= (2 << 14);
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
	WRITE_BACK_32(hw, HEVC_SAO_CTRL1, data32);

	if (dw_mode & 0x10) {
		/*[23:22] dw_v1_ctrl
			*[21:20] dw_v0_ctrl
			*[19:18] dw_h1_ctrl
			*[17:16] dw_h0_ctrl
			*/
		//data32 = READ_VREG(HEVC_SAO_CTRL5);
		/*set them all 0 for H265_NV21 (no down-scale)*/
		//data32 &= ~(0xff << 16);
		//WRITE_VREG(HEVC_SAO_CTRL5, data32);
		READ_WRITE_DATA16(hw, HEVC_SAO_CTRL5, 0, 16, 8);

	} else {
		uint32_t data;
		if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_T7) {
			WRITE_BACK_8(hw, HEVC_SAO_CTRL26, 0);
		}
		//data32 = READ_VREG(HEVC_SAO_CTRL5);
		//data32 &= (~(0xff << 16));
		if ((dw_mode & 0xf) == 8) {
			WRITE_BACK_8(hw, HEVC_SAO_CTRL26, 0xf);
			data = 0xff;
		} else if ((dw_mode & 0xf) == 2 ||
			(dw_mode & 0xf) == 3)
			data = 0xff;
		else if ((dw_mode & 0xf) == 4 ||
			(dw_mode & 0xf) == 5)
			data = 0x33;
		//WRITE_VREG(HEVC_SAO_CTRL5, data32);
		READ_WRITE_DATA16(hw, HEVC_SAO_CTRL5, data, 16, 8);
	}

	data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG);
	data32 &= (~0x30);
	/* [5:4]	-- address_format 00:linear 01:32x32 10:64x32 */
	data32 |= (hw->mem_map_mode << 4);
	data32 &= (~0xf);
	data32 |= (hw->endian & 0xf);  /* valid only when double write only */
	data32 &= (~(3 << 8));
	data32 |= (2 << 8);
	/*
	* [3:0]   little_endian
	* [5:4]   address_format 00:linear 01:32x32 10:64x32
	* [7:6]   reserved
	* [9:8]   Linear_LineAlignment 00:16byte 01:32byte 10:64byte
	* [11:10] reserved
	* [12]	  CbCr_byte_swap
	* [31:13] reserved
	*/
	WRITE_BACK_32(hw, HEVCD_IPP_AXIIF_CONFIG, data32);
#endif
}

// instantiate this function once when decode is started
void av1_loop_filter_init_fb(struct AV1HW_s *hw)
{
	int32_t i;
	loop_filter_info_n *lfi = hw->lfi;
	struct loopfilter *lf = hw->lf;

	// init limits for given sharpness
	av1_update_sharpness(lfi, lf->sharpness_level);

	// Write to register
	for (i = 0; i < 32; i++) {
	uint32_t thr;
	thr = ((lfi->lfthr[i*2+1].lim & 0x3f)<<8) | (lfi->lfthr[i*2+1].mblim & 0xff);
	thr = (thr<<16) | ((lfi->lfthr[i*2].lim & 0x3f)<<8) | (lfi->lfthr[i*2].mblim & 0xff);
	WRITE_VREG(HEVC_DBLK_CFG9, thr);
	WRITE_VREG(HEVC_DBLK_CFG9_DBE1, thr);
	}

}

// perform this function per frame
void av1_loop_filter_frame_init_fb(struct AV1HW_s *hw, struct segmentation_lf *seg,
loop_filter_info_n *lfi, struct loopfilter *lf, int32_t pic_width)
{
	AV1Decoder* pbi = hw->pbi;
	BuffInfo_t* buf_spec = pbi->work_space_buf;
	int32_t i; //,dir;
	uint32_t lpf_data32;
	uint32_t cdef_data32;
	//int32_t filt_lvl[MAX_MB_PLANE], filt_lvl_r[MAX_MB_PLANE];
	//int32_t plane;
	//int32_t seg_id;
	// n_shift is the multiplier for lf_deltas
	// the multiplier is 1 for when filter_lvl is between 0 and 31;
	// 2 when filter_lvl is between 32 and 63

	// update limits if sharpness has changed
	av1_update_sharpness(lfi, lf->sharpness_level);

	// Write to register
	for (i = 0; i < 32; i++) {
	uint32_t thr;
	thr = ((lfi->lfthr[i*2+1].lim & 0x3f)<<8) | (lfi->lfthr[i*2+1].mblim & 0xff);
	thr = (thr<<16) | ((lfi->lfthr[i*2].lim & 0x3f)<<8) | (lfi->lfthr[i*2].mblim & 0xff);
	WRITE_BACK_32(hw, HEVC_DBLK_CFG9, thr);
	}

#ifdef DBG_LPF_DBLK_LVL
	filt_lvl[0] = lf->filter_level[0];
	filt_lvl[1] = lf->filter_level_u;
	filt_lvl[2] = lf->filter_level_v;

	filt_lvl_r[0] = lf->filter_level[1];
	filt_lvl_r[1] = lf->filter_level_u;
	filt_lvl_r[2] = lf->filter_level_v;

#ifdef DBG_LPF_PRINT
	printk("LF_PRINT: pic_cnt(%d) base_filter_level(%d,%d,%d,%d)\n",lf->lf_pic_cnt,lf->filter_level[0],lf->filter_level[1],lf->filter_level_u,lf->filter_level_v);
#endif

	for (plane = 0; plane < 3; plane++) {
	if (plane == 0 && !filt_lvl[0] && !filt_lvl_r[0])
	break;
	else if (plane == 1 && !filt_lvl[1])
	continue;
	else if (plane == 2 && !filt_lvl[2])
	continue;

	for (seg_id = 0; seg_id < MAX_SEGMENTS; seg_id++) { // MAX_SEGMENTS == 8
	for (dir = 0; dir < 2; ++dir) {
		int32_t lvl_seg = (dir == 0) ? filt_lvl[plane] : filt_lvl_r[plane];
		assert(plane >= 0 && plane <= 2);
		const uint8_t seg_lf_info_y0 = seg->seg_lf_info_y[seg_id] & 0xff;
		const uint8_t seg_lf_info_y1 = (seg->seg_lf_info_y[seg_id]>>8) & 0xff;
		const uint8_t seg_lf_info_u = seg->seg_lf_info_c[seg_id] & 0xff;
		const uint8_t seg_lf_info_v = (seg->seg_lf_info_c[seg_id]>>8) & 0xff;
		const uint8_t seg_lf_info = (plane == 2) ? seg_lf_info_v : (plane == 1) ? seg_lf_info_u : ((dir == 0) ?  seg_lf_info_y0 : seg_lf_info_y1);
		const int8_t seg_lf_active = ((seg->enabled) && ((seg_lf_info>>7) & 0x1));
		const int8_t seg_lf_data = conv2int8(seg_lf_info,7);
		const int8_t seg_lf_data_clip = (seg_lf_data>63) ? 63 : (seg_lf_data<-63) ? -63 : seg_lf_data;
		if (seg_lf_active) {
		lvl_seg = clamp(lvl_seg + (int32_t)seg_lf_data, 0, MAX_LOOP_FILTER);
		}

#ifdef DBG_LPF_PRINT
		printk("LF_PRINT:plane(%d) seg_id(%d) dir(%d) seg_lf_info(%d,0x%x),lvl_seg(0x%x)\n",plane,seg_id,dir,seg_lf_active,seg_lf_data_clip,lvl_seg);
#endif

		if (!lf->mode_ref_delta_enabled) {
		// we could get rid of this if we assume that deltas are set to
		// zero when not in use; encoder always uses deltas
		memset(lfi->lvl[plane][seg_id][dir], lvl_seg,
			sizeof(lfi->lvl[plane][seg_id][dir]));
		} else {
		int32_t ref, mode;
		const int32_t scale = 1 << (lvl_seg >> 5);
		const int32_t intra_lvl = lvl_seg + lf->ref_deltas[INTRA_FRAME] * scale;
		lfi->lvl[plane][seg_id][dir][INTRA_FRAME][0] =
		clamp(intra_lvl, 0, MAX_LOOP_FILTER);
#ifdef DBG_LPF_PRINT
		printk("LF_PRINT:ref_deltas[INTRA_FRAME](%d)\n",lf->ref_deltas[INTRA_FRAME]);
#endif
		for (ref = LAST_FRAME; ref < REF_FRAMES; ++ref) {         // LAST_FRAME == 1 REF_FRAMES == 8
		for (mode = 0; mode < MAX_MODE_LF_DELTAS; ++mode) {     // MAX_MODE_LF_DELTAS == 2
		const int32_t inter_lvl = lvl_seg + lf->ref_deltas[ref] * scale +
				        lf->mode_deltas[mode] * scale;
		lfi->lvl[plane][seg_id][dir][ref][mode] =
			clamp(inter_lvl, 0, MAX_LOOP_FILTER);
#ifdef DBG_LPF_PRINT
		printk("LF_PRINT:ref_deltas(%d) mode_deltas(%d)\n",lf->ref_deltas[ref],lf->mode_deltas[mode]);
#endif
		}
		}
		}
	}
	}
	}

#ifdef DBG_LPF_PRINT
	for (i = 0; i <= MAX_LOOP_FILTER; i++) {
	printk("LF_PRINT:(%2d) thr=%d,blim=%3d,lim=%2d\n",i,lfi->lfthr[i].hev_thr,lfi->lfthr[i].mblim,lfi->lfthr[i].lim);
	}
	for (plane = 0; plane < 3; plane++) {
	for (seg_id = 0; seg_id < MAX_SEGMENTS; seg_id++) { // MAX_SEGMENTS == 8
	for (dir = 0; dir < 2; ++dir) {
		int32_t mode;
		for (mode = 0; mode < 2; ++mode) {
		printk("assign {lvl[%d][%d][%d][0][%d],lvl[%d][%d][%d][1][%d],lvl[%d][%d][%d][2][%d],lvl[%d][%d][%d][3][%d],lvl[%d][%d][%d][4][%d],lvl[%d][%d][%d][5][%d],lvl[%d][%d][%d][6][%d],lvl[%d][%d][%d][7][%d]}={6'd%2d,6'd%2d,6'd%2d,6'd%2d,6'd%2d,6'd%2d,6'd%2d,6'd%2d};\n",
		plane,seg_id,dir,mode,plane,seg_id,dir,mode,plane,seg_id,dir,mode,plane,seg_id,dir,mode,plane,seg_id,dir,mode,plane,seg_id,dir,mode,plane,seg_id,dir,mode,plane,seg_id,dir,mode,
		lfi->lvl[plane][seg_id][dir][0][mode],lfi->lvl[plane][seg_id][dir][1][mode],lfi->lvl[plane][seg_id][dir][2][mode],lfi->lvl[plane][seg_id][dir][3][mode],lfi->lvl[plane][seg_id][dir][4][mode],lfi->lvl[plane][seg_id][dir][5][mode],lfi->lvl[plane][seg_id][dir][6][mode],lfi->lvl[plane][seg_id][dir][7][mode]);
		}
	}
	}
	}
#endif

	// Write to register
	//for (i = 0; i < 192; i++) {
	//  uint32_t level;
	//  level = ((lfi->lvl[i>>6&3][i>>3&7][1][i&7][1] & 0x3f)<<24) | ((lfi->lvl[i>>6&3][i>>3&7][1][i&7][0] & 0x3f)<<16) | ((lfi->lvl[i>>6&3][i>>3&7][0][i&7][1] & 0x3f)<<8) | (lfi->lvl[i>>6&3][i>>3&7][0][i&7][0] & 0x3f);
	//  if (!lf->filter_level[0] && !lf->filter_level[1]) level = 0;
	//  WRITE_VREG(P_HEVC_DBLK_CFGA, level);
	//}
#endif // DBG_LPF_DBLK_LVL

#ifdef DBG_LPF_DBLK_FORCED_OFF
	if (lf->lf_pic_cnt == 2) {
	printk("LF_PRINT: pic_cnt(%d) dblk forced off !!!\n",lf->lf_pic_cnt);
	WRITE_BACK_8(hw, HEVC_DBLK_DBLK0, 0);
	}
	else {
	WRITE_BACK_32(hw, HEVC_DBLK_DBLK0, lf->filter_level[0] | lf->filter_level[1]<<6 | lf->filter_level_u<<12 | lf->filter_level_v<<18);
	}
#else
	WRITE_BACK_32(hw, HEVC_DBLK_DBLK0, lf->filter_level[0] | lf->filter_level[1]<<6 | lf->filter_level_u<<12 | lf->filter_level_v<<18);
#endif
	for (i =0; i < 10; i++) WRITE_BACK_8(hw, HEVC_DBLK_DBLK1, ((i<2) ? lf->mode_deltas[i&1] : lf->ref_deltas[(i-2)&7]));
	for (i =0; i < 8; i++) WRITE_BACK_32(hw, HEVC_DBLK_DBLK2, (uint32_t)(seg->seg_lf_info_y[i]) | (uint32_t)(seg->seg_lf_info_c[i]<<16));

	// Set P_HEVC_DBLK_CFGB again
	//lpf_data32 = READ_VREG(HEVC_DBLK_CFGB);
	if (lf->mode_ref_delta_enabled) lpf_data32 = 1; // mode_ref_delta_enabled
	else                            lpf_data32 = 0;
	READ_WRITE_DATA16(hw, HEVC_DBLK_CFGB, lpf_data32, 28, 1);
	if (seg->enabled) lpf_data32 = 1; 			  // seg enable
	else              lpf_data32 = 0;
	READ_WRITE_DATA16(hw, HEVC_DBLK_CFGB, lpf_data32, 29, 1);
	if (pic_width >= 1280) lpf_data32 = 1;		  // dblk pipeline mode=1 for performance
	else                   lpf_data32 = 0;
	READ_WRITE_DATA16(hw, HEVC_DBLK_CFGB, lpf_data32, 4, 2);
	//WRITE_VREG(HEVC_DBLK_CFGB, lpf_data32);

	// Set CDEF
	WRITE_BACK_32(hw, HEVC_DBLK_CDEF0, buf_spec->cdef_data.buf_start);
	//cdef_data32 = (READ_VREG(HEVC_DBLK_CDEF1) & 0xffffff00);
	cdef_data32 = 17; // cdef_temp_address left offset
#ifdef DBG_LPF_CDEF_NO_PIPELINE
	cdef_data32 = (1<<17); // cdef test no pipeline for very small picture
#endif
	//WRITE_VREG(HEVC_DBLK_CDEF1, cdef_data32);
	READ_WRITE_DATA16(hw, HEVC_DBLK_CDEF1, cdef_data32, 0, 8);

	// Picture count
	lf->lf_pic_cnt++;
}

static void config_loop_filter_hw_fb(struct AV1HW_s *hw, union param_u *param)
{
	int i;
	//AV1Decoder* pbi = hw->pbi;
	loop_filter_info_n *lfi = hw->lfi;
	struct loopfilter *lf = hw->lf;
	struct segmentation_lf *seg_4lf = hw->seg_4lf;
	PIC_BUFFER_CONFIG *pic = &hw->common.cur_frame->buf;
	// reset lpf per frame
	//uint32_t dblk_reset_data32;
	//dblk_reset_data32 = (READ_VREG(HEVC_DBLK_CFG0)) | 0x3;
	//WRITE_VREG(HEVC_DBLK_CFG0, dblk_reset_data32);
	READ_WRITE_DATA16(hw, HEVC_DBLK_CFG0, 0x3, 0, 2);
	//dblk_reset_data32 = (READ_VREG(HEVC_DBLK_CFG0)) & ~(0x3);
	//WRITE_VREG(HEVC_DBLK_CFG0, dblk_reset_data32);
	READ_WRITE_DATA16(hw, HEVC_DBLK_CFG0, 0, 0, 2);
	//printk("[test.c ref_delta] cur_frame : %x prev_frame : %x - %x \n", cm->cur_frame, cm->prev_frame, get_primary_ref_frame_buf(cm));
	// get lf parameters from parser
	lf->mode_ref_delta_enabled      = (param->p.loop_filter_mode_ref_delta_enabled & 1);
	lf->mode_ref_delta_update       = ((param->p.loop_filter_mode_ref_delta_enabled >> 1) & 1);
	lf->sharpness_level             = param->p.loop_filter_sharpness_level;
	if (((param->p.loop_filter_mode_ref_delta_enabled)&3) == 3) { // enabled but and update
	if (pic->prev_frame <= 0) {
	// already initialized in Microcode
		lf->ref_deltas[0]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_0),7);
		lf->ref_deltas[1]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_0>>8),7);
		lf->ref_deltas[2]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_1),7);
		lf->ref_deltas[3]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_1>>8),7);
		lf->ref_deltas[4]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_2),7);
		lf->ref_deltas[5]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_2>>8),7);
		lf->ref_deltas[6]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_3),7);
		lf->ref_deltas[7]               = conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_3>>8),7);
		lf->mode_deltas[0]              = conv2int8((uint8_t)(param->p.loop_filter_mode_deltas_0),7);
		lf->mode_deltas[1]              = conv2int8((uint8_t)(param->p.loop_filter_mode_deltas_0>>8),7);
	}
	else {
		lf->ref_deltas[0]               = (param->p.loop_filter_ref_deltas_0 & 0x80) ?
				               conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_0),7) :
				pic->prev_frame->ref_deltas[0];
		lf->ref_deltas[1]               = (param->p.loop_filter_ref_deltas_0 & 0x8000) ?
				                       conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_0>>8),7) :
				pic->prev_frame->ref_deltas[1];
		lf->ref_deltas[2]               = (param->p.loop_filter_ref_deltas_1 & 0x80) ?
				               conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_1),7) :
				pic->prev_frame->ref_deltas[2];
		lf->ref_deltas[3]               = (param->p.loop_filter_ref_deltas_1 & 0x8000) ?
				                       conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_1>>8),7) :
				pic->prev_frame->ref_deltas[3];
		lf->ref_deltas[4]               = (param->p.loop_filter_ref_deltas_2 & 0x80) ?
				               conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_2),7) :
				pic->prev_frame->ref_deltas[4];
		lf->ref_deltas[5]               = (param->p.loop_filter_ref_deltas_2 & 0x8000) ?
				                       conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_2>>8),7) :
				pic->prev_frame->ref_deltas[5];
		lf->ref_deltas[6]               = (param->p.loop_filter_ref_deltas_3 & 0x80) ?
				               conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_3),7) :
				pic->prev_frame->ref_deltas[6];
		lf->ref_deltas[7]               = (param->p.loop_filter_ref_deltas_3 & 0x8000) ?
				                       conv2int8((uint8_t)(param->p.loop_filter_ref_deltas_3>>8),7) :
				pic->prev_frame->ref_deltas[7];
		lf->mode_deltas[0]               = (param->p.loop_filter_mode_deltas_0 & 0x80) ?
				               conv2int8((uint8_t)(param->p.loop_filter_mode_deltas_0),7) :
				pic->prev_frame->mode_deltas[0];
		lf->mode_deltas[1]               = (param->p.loop_filter_mode_deltas_0 & 0x8000) ?
				                       conv2int8((uint8_t)(param->p.loop_filter_mode_deltas_0>>8),7) :
				pic->prev_frame->mode_deltas[1];
	}
}
	//else if (param->p.loop_filter_mode_ref_delta_enabled == 1) { // enabled but no update
	else { // match c code -- not enabled, still need to copy prev to used for next
		if ((pic->prev_frame <= 0) | (param->p.loop_filter_mode_ref_delta_enabled & 4)) {
		av1_print(hw, AOM_DEBUG_HW_MORE, "[test.c] mode_ref_delta set to default\n");
		lf->ref_deltas[0]               = conv2int8((uint8_t)1,7);
		lf->ref_deltas[1]               = conv2int8((uint8_t)0,7);
		lf->ref_deltas[2]               = conv2int8((uint8_t)0,7);
		lf->ref_deltas[3]               = conv2int8((uint8_t)0,7);
		lf->ref_deltas[4]               = conv2int8((uint8_t)0xff,7);
		lf->ref_deltas[5]               = conv2int8((uint8_t)0,7);
		lf->ref_deltas[6]               = conv2int8((uint8_t)0xff,7);
		lf->ref_deltas[7]               = conv2int8((uint8_t)0xff,7);
		lf->mode_deltas[0]              = conv2int8((uint8_t)0,7);
		lf->mode_deltas[1]              = conv2int8((uint8_t)0,7);
	} else {
		av1_print(hw, AOM_DEBUG_HW_MORE, "[test.c] mode_ref_delta copy from prev_frame\n");
		lf->ref_deltas[0]               = pic->prev_frame->ref_deltas[0];
		lf->ref_deltas[1]               = pic->prev_frame->ref_deltas[1];
		lf->ref_deltas[2]               = pic->prev_frame->ref_deltas[2];
		lf->ref_deltas[3]               = pic->prev_frame->ref_deltas[3];
		lf->ref_deltas[4]               = pic->prev_frame->ref_deltas[4];
		lf->ref_deltas[5]               = pic->prev_frame->ref_deltas[5];
		lf->ref_deltas[6]               = pic->prev_frame->ref_deltas[6];
		lf->ref_deltas[7]               = pic->prev_frame->ref_deltas[7];
		lf->mode_deltas[0]              = pic->prev_frame->mode_deltas[0];
		lf->mode_deltas[1]              = pic->prev_frame->mode_deltas[1];
	}
}
	lf->filter_level[0]             = param->p.loop_filter_level_0;
	lf->filter_level[1]             = param->p.loop_filter_level_1;
	lf->filter_level_u              = param->p.loop_filter_level_u;
	lf->filter_level_v              = param->p.loop_filter_level_v;

	pic->cur_frame->ref_deltas[0] = lf->ref_deltas[0];
	pic->cur_frame->ref_deltas[1] = lf->ref_deltas[1];
	pic->cur_frame->ref_deltas[2] = lf->ref_deltas[2];
	pic->cur_frame->ref_deltas[3] = lf->ref_deltas[3];
	pic->cur_frame->ref_deltas[4] = lf->ref_deltas[4];
	pic->cur_frame->ref_deltas[5] = lf->ref_deltas[5];
	pic->cur_frame->ref_deltas[6] = lf->ref_deltas[6];
	pic->cur_frame->ref_deltas[7] = lf->ref_deltas[7];
	pic->cur_frame->mode_deltas[0] = lf->mode_deltas[0];
	pic->cur_frame->mode_deltas[1] = lf->mode_deltas[1];

	// get seg_4lf parameters from parser
	seg_4lf->enabled                = param->p.segmentation_enabled & 1;
#ifndef NEW_FRONT_BACK_CODE
	pic->cur_frame->segmentation_enabled = param->p.segmentation_enabled & 1;
	pic->cur_frame->intra_only = (param->p.segmentation_enabled >> 2) & 1;
	pic->cur_frame->segmentation_update_map = (param->p.segmentation_enabled >> 3) & 1;
#endif
if (param->p.segmentation_enabled & 1) { // segmentation_enabled
if (param->p.segmentation_enabled & 2) { // segmentation_update_data
		for (i=0;i<MAX_SEGMENTS;i++) {
		seg_4lf->seg_lf_info_y[i]   = param->p.seg_lf_info_y[i];
		seg_4lf->seg_lf_info_c[i]   = param->p.seg_lf_info_c[i];
#ifdef DBG_LPF_PRINT
		av1_print(hw, AOM_DEBUG_HW_MORE,
			" read seg_lf_info [%d] : 0x%x, 0x%x\n", i, seg_4lf->seg_lf_info_y[i], seg_4lf->seg_lf_info_c[i]);
#endif
		}
} // segmentation_update_data
else { // no segmentation_update_data
if (pic->prev_frame <= 0) {
		for (i=0;i<MAX_SEGMENTS;i++) {
		seg_4lf->seg_lf_info_y[i]   = 0;
		seg_4lf->seg_lf_info_c[i]   = 0;
		}
}
else {
		for (i=0;i<MAX_SEGMENTS;i++) {
		seg_4lf->seg_lf_info_y[i]   = pic->prev_frame->seg_lf_info_y[i];
		seg_4lf->seg_lf_info_c[i]   = pic->prev_frame->seg_lf_info_c[i];
#ifdef DBG_LPF_PRINT
			av1_print(hw, AOM_DEBUG_HW_MORE,
				" Reference seg_lf_info [%d] : 0x%x, 0x%x\n", i, seg_4lf->seg_lf_info_y[i], seg_4lf->seg_lf_info_c[i]);
#endif
		}
}
} // no segmentation_update_data
} // segmentation_enabled
else{
	for (i=0;i<MAX_SEGMENTS;i++) {
		seg_4lf->seg_lf_info_y[i]   = 0;
		seg_4lf->seg_lf_info_c[i]   = 0;
	}
} // NOT segmentation_enabled

	for (i=0;i<MAX_SEGMENTS;i++) {
		pic->cur_frame->seg_lf_info_y[i] = seg_4lf->seg_lf_info_y[i];
		pic->cur_frame->seg_lf_info_c[i] = seg_4lf->seg_lf_info_c[i];
#ifdef DBG_LPF_PRINT
		av1_print(hw, AOM_DEBUG_HW_MORE,
			" SAVE seg_lf_info [%d] : 0x%x, 0x%x\n", i, pic->cur_frame->seg_lf_info_y[i], pic->cur_frame->seg_lf_info_c[i]);
#endif
	}

	/*
	* Update loop filter Thr/Lvl table for every frame
	*/
	av1_print(hw, AOM_DEBUG_HW_MORE,
		"[test.c] av1_loop_filter_frame_init (run before every frame decoding start)\n");
	av1_loop_filter_frame_init_fb(hw, seg_4lf, lfi, lf, pic->dec_width);
}

void BackEnd_StartDecoding(struct AV1HW_s *hw)
{
	int ret = 0;
	AV1_COMMON *cm = &hw->common;
	AV1Decoder* pbi = hw->pbi;
	PIC_BUFFER_CONFIG* pic = pbi->next_be_decode_pic[pbi->fb_rd_pos];
	int dw_mode = get_double_write_mode(hw);

	if (front_back_debug & 2) {
		printk("Start BackEnd Decoding %d (wr pos %d, rd pos %d)\n",
			pbi->backend_decoded_count, pbi->fb_wr_pos, pbi->fb_rd_pos);
	}

	if (hw->front_back_mode == 1) {
		ATRACE_COUNTER(hw->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_START);
		if ((dw_mode & 0x10) == 0) {
			ret = av1_alloc_mmu(hw, hw->mmu_box,
				pic->index,
				pic->y_crop_width,
				pic->y_crop_height/2 + 64 + 8,
				hw->aom_param.p.bit_depth,
				hw->frame_mmu_map_addr);
			if (ret >= 0)
				cm->cur_fb_idx_mmu = pic->index;
			else
				pr_err("can't alloc need mmu1,idx %d ret =%d\n", pic->index, ret);

			ret = av1_alloc_mmu(hw, hw->mmu_box_1,
				pic->index,
				pic->y_crop_width,
				pic->y_crop_height/2 + 64 + 8,
				hw->aom_param.p.bit_depth,
				hw->frame_mmu_map_addr_1);
			if (ret >= 0)
				cm->cur_fb_idx_mmu = pic->index;
			else
				pr_err("can't alloc need mmu1_1,idx %d ret =%d\n", pic->index, ret);
#ifdef AOM_AV1_MMU_DW
			if (hw->dw_mmu_enable) {
				ret = av1_alloc_mmu_dw(hw, hw->mmu_box_dw,
					pic->index,
					pic->y_crop_width,
					pic->y_crop_height,
					hw->aom_param.p.bit_depth,
					hw->dw_frame_mmu_map_addr);
				if (ret >= 0)
					cm->cur_fb_idx_mmu_dw = pic->index;
				else
					pr_err("can't alloc need dw mmu1,idx %d ret =%d\n", pic->index, ret);

				ret = av1_alloc_mmu_dw(hw, hw->mmu_box_dw_1,
					pic->index,
					pic->y_crop_width,
					pic->y_crop_height,
					hw->aom_param.p.bit_depth,
					hw->dw_frame_mmu_map_addr_1);
			}
#endif
		}
#if 0
			if (crc_debug_flag & 0x40)
				mv_buffer_fill_zero(hw, &cm->cur_frame->buf);
#endif
		ATRACE_COUNTER(hw->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_END);
		pic->mmu_alloc_flag = 1;
	}

	if (front_back_debug)
		pr_info("%s, alloc mmu time %lld\n", __func__, div64_u64(local_clock() - hw->back_start_time, 1000));

	hw->back_start_time = local_clock();
	copy_loopbufs_ptr(&pbi->bk, &pbi->next_bk[pbi->fb_rd_pos]);
	if (debug & AOM_DEBUG_HW_MORE)
		print_loopbufs_ptr("bk", &pbi->bk);

#ifdef RESET_BACK_PER_PICTURE
	if (hw->front_back_mode == 1) {
#ifdef PRINT_HEVC_DATA_PATH_MONITOR
		if (pbi->backend_decoded_count > 0) {
			if (debug & AOM_DEBUG_HW_MORE) {
				print_hevc_b_data_path_monitor(pbi, pbi->backend_decoded_count - 1);
				print_mcrcc_hit_info(pbi->backend_decoded_count - 1);
			}
		}
#endif
		amhevc_reset_b();
	}
	if (efficiency_mode)
		WRITE_VREG(HEVC_EFFICIENCY_MODE_BACK, 1);
	else
		WRITE_VREG(HEVC_EFFICIENCY_MODE_BACK, 0);
	av1_hw_init(hw, pbi->backend_decoded_count == 0, 0, 1);
#else
	if (pbi->backend_decoded_count == 0) {
	//	amhevc_reset_b();
		av1_hw_init(hw, 1, 0, 1);
	}
#endif

#ifdef NEW_FB_CODE
	if (hw->front_back_mode != 1)
		return;
#endif
	config_bufstate_back_hw(pbi);

	WRITE_VREG(PIC_DECODE_COUNT_DBE, pbi->backend_decoded_count);
	WRITE_VREG(HEVC_DEC_STATUS_DBE, HEVC_BE_DECODE_DATA);
#if 0
#ifdef RESET_BACK_PER_PICTURE
	amhevc_start_b();
#else
	if (pbi->backend_decoded_count == 0)
		amhevc_start_b();
#endif
#endif
}

