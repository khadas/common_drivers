#include "../../../include/regs/dos_registers.h"
#include "../../../common/media_utils/media_utils.h"

#ifdef FOR_S5
ulong dos_reg_compat_convert(ulong adr);
#endif

/* to do */
#define DOUBLE_WRITE_VH0_TEMP    0
#define DOUBLE_WRITE_VH1_TEMP    0
#define DOUBLE_WRITE_VH0_HALF    0
#define DOUBLE_WRITE_VH1_HALF    0
//#define DOS_BASE_ADR  0xd0050000
#if 0
//#define HEVC_SAO_MMU_VH0_ADDR_DBE1 0
//#define HEVC_SAO_MMU_VH1_ADDR_DBE1 0
#define HEVCD_MPP_DECOMP_AXIURG_CTL 0

#define HEVC_MPRED_POC24_CTRL0 0
#define HEVC_MPRED_POC24_CTRL1 0

#define HEVCD_MCRCC_PERFMON_CTL_DBE1 0
#define HEVCD_MCRCC_PERFMON_DATA_DBE1 0
#define HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1 0
#define HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1 0
#endif
//#define DOUBLE_WRITE_YSTART_TEMP 0x02000000
//#define DOUBLE_WRITE_CSTART_TEMP 0x02900000

/**/
#define DOS_BASE_ADR  0x0

#define print_scratch_error(a)
//#define MEM_MAP_MODE    0

//typedef union param_u param_t;

#define PRINT_HEVC_DATA_PATH_MONITOR
//static   unsigned   mcrcc_hit_rate;
static   unsigned   mcrcc_hit_rate_0;
static   unsigned   mcrcc_hit_rate_1;
//static   unsigned   mcrcc_bypass_rate;
static   unsigned   mcrcc_bypass_rate_0;
static   unsigned   mcrcc_bypass_rate_1;

static void init_pic_list_hw_fb(struct AVS3Decoder_s *dec);

static void C_Reg_Rd(unsigned adr, unsigned *pval)
{
	*pval = READ_VREG(adr);
}

static void mcrcc_perfcount_reset_dual(struct AVS3Decoder_s *dec)
{
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[cache_util.c] Entered mcrcc_perfcount_reset_dual...\n");
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)0x0);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)0x0);
	return;
}

static void mcrcc_get_hitrate_dual(int pic_num)
{
	unsigned tmp;
	unsigned raw_mcr_cnt;
	unsigned hit_mcr_cnt;
	unsigned byp_mcr_cnt_nchoutwin;
	unsigned byp_mcr_cnt_nchcanv;
	unsigned hit_mcr_0_cnt;
	unsigned hit_mcr_1_cnt;
	unsigned hitrate;
	printk("[cache_util.c] Entered mcrcc_get_hitrate_dual...\n");

	printk("[MCRCC CORE ] Picture : %d\n", pic_num);

	// CORE 0
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x0<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &raw_mcr_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x1<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &hit_mcr_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x2<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &byp_mcr_cnt_nchoutwin);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x3<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &byp_mcr_cnt_nchcanv);

	printk("[MCRCC CORE0] raw_mcr_cnt: %d\n", raw_mcr_cnt);
	printk("[MCRCC CORE0] hit_mcr_cnt: %d\n", hit_mcr_cnt);
	printk("[MCRCC CORE0] byp_mcr_cnt_nchoutwin: %d\n", byp_mcr_cnt_nchoutwin);
	printk("[MCRCC CORE0] byp_mcr_cnt_nchcanv: %d\n", byp_mcr_cnt_nchcanv);

	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x4<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &tmp);
	printk("[MCRCC CORE0] miss_mcr_0_cnt: %d\n", tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x5<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &tmp);
	printk("[MCRCC CORE0] miss_mcr_1_cnt: %d\n", tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x6<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &hit_mcr_0_cnt);
	printk("[MCRCC CORE0] hit_mcr_0_cnt: %d\n", hit_mcr_0_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x7<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA, &hit_mcr_1_cnt);
	printk("[MCRCC CORE0] hit_mcr_1_cnt: %d\n", hit_mcr_1_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_0_cnt * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE0] CANV0_HIT_RATE : %d\n", hitrate);
		hitrate = (hit_mcr_1_cnt * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE0] CANV1_HIT_RATE : %d\n", hitrate);
		hitrate = (byp_mcr_cnt_nchcanv *100 / raw_mcr_cnt);
		printk("[MCRCC CORE0] NONCACH_CANV_BYP_RATE : %d\n", hitrate);
		hitrate = (byp_mcr_cnt_nchoutwin *100 / raw_mcr_cnt);
		printk("[MCRCC CORE0] CACHE_OUTWIN_BYP_RATE : %d\n", hitrate);
	}

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt * 100 / raw_mcr_cnt);
		printk("[P%d MCRCC CORE0] MCRCC_HIT_RATE : %d\n", pic_num, hitrate);
		hitrate = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv) * 100 / raw_mcr_cnt);
		printk("[P%d MCRCC CORE0] MCRCC_BYP_RATE : %d\n", pic_num, hitrate);
	} else {
		printk("[P%d MCRCC CORE0] MCRCC_HIT_RATE : na\n", pic_num);
		printk("[P%d MCRCC CORE0] MCRCC_BYP_RATE : na\n", pic_num);
	}

	mcrcc_hit_rate_0 = (hit_mcr_cnt * 100 / raw_mcr_cnt);
	mcrcc_bypass_rate_0 = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv) * 100 /raw_mcr_cnt);

	// CORE 0
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x0<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &raw_mcr_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x1<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &hit_mcr_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x2<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &byp_mcr_cnt_nchoutwin);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x3<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &byp_mcr_cnt_nchcanv);

	printk("[MCRCC CORE1] raw_mcr_cnt: %d\n", raw_mcr_cnt);
	printk("[MCRCC CORE1] hit_mcr_cnt: %d\n", hit_mcr_cnt);
	printk("[MCRCC CORE1] byp_mcr_cnt_nchoutwin: %d\n", byp_mcr_cnt_nchoutwin);
	printk("[MCRCC CORE1] byp_mcr_cnt_nchcanv: %d\n", byp_mcr_cnt_nchcanv);

	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x4<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &tmp);
	printk("[MCRCC CORE1] miss_mcr_0_cnt: %d\n", tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x5<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &tmp);
	printk("[MCRCC CORE1] miss_mcr_1_cnt: %d\n", tmp);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x6<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &hit_mcr_0_cnt);
	printk("[MCRCC CORE1] hit_mcr_0_cnt: %d\n", hit_mcr_0_cnt);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL_DBE1, (unsigned int)(0x7<<1));
	C_Reg_Rd(HEVCD_MCRCC_PERFMON_DATA_DBE1, &hit_mcr_1_cnt);
	printk("[MCRCC CORE1] hit_mcr_1_cnt: %d\n", hit_mcr_1_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_0_cnt * 100 /raw_mcr_cnt);
		printk("[MCRCC CORE1] CANV0_HIT_RATE : %d\n", hitrate);
		hitrate = (hit_mcr_1_cnt * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE1] CANV1_HIT_RATE : %d\n", hitrate);
		hitrate = (byp_mcr_cnt_nchcanv * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE1] NONCACH_CANV_BYP_RATE : %d\n", hitrate);
		hitrate = (byp_mcr_cnt_nchoutwin * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE1] CACHE_OUTWIN_BYP_RATE : %d\n", hitrate);
	}

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt * 100 / raw_mcr_cnt);
		printk("[P%d MCRCC CORE1] MCRCC_HIT_RATE : %d\n", pic_num, hitrate);
		hitrate = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv) * 100 /raw_mcr_cnt);
		printk("[P%d MCRCC CORE1] MCRCC_BYP_RATE : %d\n", pic_num, hitrate);
	} else {
		printk("[P%d MCRCC CORE1] MCRCC_HIT_RATE : na\n", pic_num);
		printk("[P%d MCRCC CORE1] MCRCC_BYP_RATE : na\n", pic_num);
	}

	mcrcc_hit_rate_1 = (hit_mcr_cnt * 100 /raw_mcr_cnt);
	mcrcc_bypass_rate_1 = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv) * 100 /raw_mcr_cnt);

	return;
}

static void decomp_perfcount_reset_dual(struct AVS3Decoder_s *dec)
{
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[cache_util.c] Entered decomp_perfcount_reset_dual...\n");
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)0x0);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)0x0);
	return;
}

static void decomp_get_hitrate_dual(int pic_num)
{
	unsigned raw_mcr_cnt;
	unsigned hit_mcr_cnt;
	unsigned hitrate;
	printk("[cache_util.c] Entered decomp_get_hitrate_dual...\n");

	// CORE0
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x0<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA, &raw_mcr_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x1<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA, &hit_mcr_cnt);

	printk("[MCRCC CORE0] hcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE0] hcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt * 100 /raw_mcr_cnt);
		printk("[MCRCC CORE0] DECOMP_HCACHE_HIT_RATE : %d\n", hitrate);
	} else {
		printk("[MCRCC CORE0] DECOMP_HCACHE_HIT_RATE : na\n");
	}
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x2<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA, &raw_mcr_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x3<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA, &hit_mcr_cnt);

	printk("[MCRCC CORE0] dcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE0] dcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE0] DECOMP_DCACHE_HIT_RATE : %d\n", hitrate);

		hitrate = (mcrcc_hit_rate_0 + (mcrcc_bypass_rate_0 * hit_mcr_cnt / raw_mcr_cnt ));
		printk("[MCRCC CORE0] MCRCC_DECOMP_DCACHE_EFFECTIVE_HIT_RATE : %d\n", hitrate);
	} else {
		printk("[MCRCC CORE0] DECOMP_DCACHE_HIT_RATE : na\n");
	}

	// CORE1
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x0<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1, &raw_mcr_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x1<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1, &hit_mcr_cnt);

	printk("[MCRCC CORE1] hcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE1] hcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE1] DECOMP_HCACHE_HIT_RATE : %d\n", hitrate);
	} else {
		printk("[MCRCC CORE1] DECOMP_HCACHE_HIT_RATE : na\n");
	}
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x2<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1, &raw_mcr_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x3<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1, &hit_mcr_cnt);

	printk("[MCRCC CORE1] dcache_raw_cnt_total: %d\n",raw_mcr_cnt);
	printk("[MCRCC CORE1] dcache_hit_cnt_total: %d\n",hit_mcr_cnt);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt * 100 / raw_mcr_cnt);
		printk("[MCRCC CORE1] DECOMP_DCACHE_HIT_RATE : %d\n", hitrate);

		hitrate = (mcrcc_hit_rate_1 + (mcrcc_bypass_rate_1 * hit_mcr_cnt / raw_mcr_cnt));
		printk("[MCRCC CORE1] MCRCC_DECOMP_DCACHE_EFFECTIVE_HIT_RATE : %d\n", hitrate);
	} else {
		printk("[MCRCC CORE1] DECOMP_DCACHE_HIT_RATE : na\n");
	}

	return;
}

static void decomp_get_comprate_dual(int pic_num)
{
	unsigned raw_ucomp_cnt;
	unsigned fast_comp_cnt;
	unsigned slow_comp_cnt;
	unsigned comprate;

	printk("[cache_util.c] Entered decomp_get_comprate_dual...\n");

	// CORE0
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x4<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA, &fast_comp_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x5<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA, &slow_comp_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x6<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA, &raw_ucomp_cnt);

	printk("[MCRCC CORE0] decomp_fast_comp_total: %d\n", fast_comp_cnt);
	printk("[MCRCC CORE0] decomp_slow_comp_total: %d\n", slow_comp_cnt);
	printk("[MCRCC CORE0] decomp_raw_uncomp_total: %d\n", raw_ucomp_cnt);

	if (raw_ucomp_cnt != 0) {
		comprate = ((fast_comp_cnt + slow_comp_cnt) *100 / raw_ucomp_cnt);
		printk("[MCRCC CORE0] DECOMP_COMP_RATIO : %d\n", comprate);
	} else {
		printk("[MCRCC CORE0] DECOMP_COMP_RATIO : na\n");
	}

	// CORE1
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x4<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1, &fast_comp_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x5<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1, &slow_comp_cnt);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL_DBE1, (unsigned int)(0x6<<1));
	C_Reg_Rd(HEVCD_MPP_DECOMP_PERFMON_DATA_DBE1, &raw_ucomp_cnt);

	printk("[MCRCC CORE1] decomp_fast_comp_total: %d\n", fast_comp_cnt);
	printk("[MCRCC CORE1] decomp_slow_comp_total: %d\n", slow_comp_cnt);
	printk("[MCRCC CORE1] decomp_raw_uncomp_total: %d\n", raw_ucomp_cnt);

	if (raw_ucomp_cnt != 0) {
		comprate = ((fast_comp_cnt + slow_comp_cnt) * 100 / raw_ucomp_cnt);
		printk("[MCRCC CORE1] DECOMP_COMP_RATIO : %d\n", comprate);
	} else {
		printk("[MCRCC CORE1] DECOMP_COMP_RATIO : na\n");
	}

	return;
}

static void print_mcrcc_hit_info(int pic_num) {
	mcrcc_get_hitrate_dual(pic_num);
	decomp_get_hitrate_dual(pic_num);
	decomp_get_comprate_dual(pic_num);
}

static void WRITE_BACK_RET(struct avs3_decoder *avs3_dec)
{
	struct AVS3Decoder_s *dec = container_of(avs3_dec, struct AVS3Decoder_s, avs3_dec);
	avs3_dec->instruction[avs3_dec->ins_offset] = 0xcc00000;   //ret
	avs3_print(dec, AVS3_DBG_REG,
		"WRITE_BACK_RET()\ninstruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = 0;           //nop
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
}

static void WRITE_BACK_8(struct avs3_decoder *avs3_dec, uint32_t spr_addr, uint8_t data)
{
	struct AVS3Decoder_s *dec = container_of(avs3_dec, struct AVS3Decoder_s, avs3_dec);
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x20 << 22) | ((spr_addr & 0xfff) << 8) | (data & 0xff);   //mtspi data, spr_addr
	avs3_print(dec, AVS3_DBG_REG,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
		avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPPORT
	if (avs3_dec->ins_offset < 256 && (avs3_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs3_dec);
		avs3_dec->ins_offset = 256;
	}
#endif
}

static void WRITE_BACK_16(struct avs3_decoder *avs3_dec, uint32_t spr_addr, uint8_t rd_addr, uint16_t data)
{
	struct AVS3Decoder_s *dec = container_of(avs3_dec, struct AVS3Decoder_s, avs3_dec);
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x1a << 22) | ((data & 0xffff) << 6) | (rd_addr & 0x3f);       // movi rd_addr, data[15:0]
	avs3_print(dec, AVS3_DBG_REG,
		"%s(%x,%x,%x)\ninstruction[%3d] = %8x, data= %x\n",
		__func__, spr_addr, rd_addr, data,
		avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset], data & 0xffff);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x18 << 22) | ((spr_addr & 0xfff) << 8) | (rd_addr & 0x3f);  // mtsp rd_addr, spr_addr
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPPORT
	if (avs3_dec->ins_offset < 256 && (avs3_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs3_dec);
		avs3_dec->ins_offset = 256;
	}
#endif
}

static void WRITE_BACK_32(struct avs3_decoder *avs3_dec, uint32_t spr_addr, uint32_t data)
{
	struct AVS3Decoder_s *dec = container_of(avs3_dec, struct AVS3Decoder_s, avs3_dec);
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x1a << 22) | ((data & 0xffff) << 6);   // movi COMMON_REG_0, data
	avs3_print(dec, AVS3_DBG_REG,
		"%s(%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data,
		avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	data = (data & 0xffff0000) >> 16;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x1b << 22) | (data << 6);                // mvihi COMMON_REG_0, data[31:16]
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x18 << 22) | ((spr_addr & 0xfff) << 8);    // mtsp COMMON_REG_0, spr_addr
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPPORT
	if (avs3_dec->ins_offset < 256 && (avs3_dec->ins_offset + 16) >= 256) {
	WRITE_BACK_RET(avs3_dec);
	avs3_dec->ins_offset = 256;
	}
#endif
}

static void READ_INS_WRITE(struct avs3_decoder *avs3_dec, uint32_t spr_addr0, uint32_t spr_addr1, uint8_t rd_addr, uint8_t position, uint8_t size)
{
	struct AVS3Decoder_s *dec = container_of(avs3_dec, struct AVS3Decoder_s, avs3_dec);
	spr_addr0 = (dos_reg_compat_convert(spr_addr0) & 0xfff);
	spr_addr1 = (dos_reg_compat_convert(spr_addr1) & 0xfff);
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x19 << 22) | ((spr_addr0 & 0xfff) << 8) | (rd_addr & 0x3f);    //mfsp rd_addr, src_addr0
	avs3_print(dec, AVS3_DBG_REG,
		"%s(%x,%x,%x,%x,%x)\n",
		__func__, spr_addr0, spr_addr1, rd_addr, position, size);
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x25 << 22) | ((position & 0x1f) << 17) | ((size & 0x1f) << 12) | ((rd_addr & 0x3f) << 6) | (rd_addr & 0x3f);   //ins rd_addr, rd_addr, position, size
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x18 << 22) | ((spr_addr1 & 0xfff) << 8) | (rd_addr & 0x3f);    //mtsp rd_addr, src_addr1
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPPORT
	if (avs3_dec->ins_offset < 256 && (avs3_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs3_dec);
		avs3_dec->ins_offset = 256;
	}
#endif
}

//Caution:  pc offset fixed to 4, the data of cmp_addr need ready before call this function
void READ_CMP_WRITE(struct avs3_decoder *avs3_dec, uint32_t spr_addr0, uint32_t spr_addr1, uint8_t rd_addr, uint8_t cmp_addr, uint8_t position, uint8_t size)
{
	struct AVS3Decoder_s *dec = container_of(avs3_dec, struct AVS3Decoder_s, avs3_dec);
	spr_addr0 = (dos_reg_compat_convert(spr_addr0) & 0xfff);
	spr_addr1 = (dos_reg_compat_convert(spr_addr1) & 0xfff);
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x19 << 22) | ((spr_addr0 & 0xfff) << 8) | (rd_addr & 0x3f);    //mfsp rd_addr, src_addr0
	avs3_print(dec, AVS3_DBG_REG,
		"%s(%x,%x,%x,%x,%x,%x)\n",
		__func__, spr_addr0, spr_addr1, rd_addr, cmp_addr, position, size);
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x25 << 22) | ((position & 0x1f) << 17) | ((size & 0x1f) << 12) | ((rd_addr & 0x3f)<<6) | (rd_addr & 0x3f);   //ins rd_addr, rd_addr, position, size
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x29 << 22) | (4 << 12) | ((rd_addr & 0x3f) << 6) | cmp_addr;     //cbne current_pc+4, rd_addr, cmp_addr
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = 0;                                                       //nop
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x19 << 22) | ((spr_addr0 & 0xfff) << 8) | (rd_addr & 0x3f);    //mfsp rd_addr, src_addr0
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x25 <<22) | ((position & 0x1f) << 17) | ((size & 0x1f) << 12) | ((rd_addr & 0x3f) << 6) | (rd_addr & 0x3f);   //ins rd_addr, rd_addr, position, size
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x18 << 22) | ((spr_addr1 & 0xfff) << 8) | (rd_addr & 0x3f);    //mtsp rd_addr, src_addr1
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPPORT
	if (avs3_dec->ins_offset < 256 && (avs3_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs3_dec);
		avs3_dec->ins_offset = 256;
	}
#endif
}

static void READ_WRITE_DATA16(struct avs3_decoder *avs3_dec, uint32_t spr_addr, uint16_t data, uint8_t position, uint8_t size)
{
	struct AVS3Decoder_s *dec = container_of(avs3_dec, struct AVS3Decoder_s, avs3_dec);
	spr_addr = (dos_reg_compat_convert(spr_addr) & 0xfff);
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x19 << 22) | ((spr_addr & 0xfff) << 8);    //mfsp COMON_REG_0, spr_addr
	avs3_print(dec, AVS3_DBG_REG,
		"%s(%x,%x,%x,%x)\ninstruction[%3d] = %8x\n",
		__func__, spr_addr, data, position, size,
		avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x1a << 22) | (data << 6) | 1;        //movi COMMON_REG_1, data
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x25 << 22) | ((position & 0x1f) << 17) | ((size & 0x1f) << 12) | (0 << 6) | 1;  //ins COMMON_REG_0, COMMON_REG_1, position, size
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x18 << 22) | ((spr_addr & 0xfff) << 8);    //mtsp COMMON_REG_0, spr_addr
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
#ifdef LARGE_INSTRUCTION_SPACE_SUPPORT
	if (avs3_dec->ins_offset < 256 && (avs3_dec->ins_offset + 16) >= 256) {
		WRITE_BACK_RET(avs3_dec);
		avs3_dec->ins_offset = 256;
	}
#endif
}

static int32_t config_mc_buffer_fb(struct AVS3Decoder_s *dec)
{
	int32_t i;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	avs3_frame_t *pic;

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"Entered config_mc_buffer....\n");
	if (avs3_dec->f_bg != NULL) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for background (canvas_y %d, canvas_u_v %d)\n",
			avs3_dec->f_bg->mc_canvas_y, avs3_dec->f_bg->mc_canvas_u_v);
		WRITE_BACK_16(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (15 << 8) | (0 << 1) | 1);   // L0:BG
		WRITE_BACK_32(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
			(avs3_dec->f_bg->mc_canvas_u_v << 16)|(avs3_dec->f_bg->mc_canvas_u_v << 8) | avs3_dec->f_bg->mc_canvas_y);
		WRITE_BACK_16(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (31 << 8) | (0 << 1) | 1);  // L1:BG
		WRITE_BACK_32(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
			(avs3_dec->f_bg->mc_canvas_u_v << 16) | (avs3_dec->f_bg->mc_canvas_u_v << 8) | avs3_dec->f_bg->mc_canvas_y);
	}
	if (avs3_dec->slice_type == SLICE_I)
		return 0;
	if (avs3_dec->slice_type == SLICE_P || avs3_dec->slice_type == SLICE_B) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for REF_0, img type %d\n", avs3_dec->slice_type);
		WRITE_BACK_8(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0 << 1) | 1);
		for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_0]; i++) {
			pic = &avs3_dec->ctx.refp[i][REFP_0].pic->buf_cfg;
			WRITE_BACK_32(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
				(pic->mc_canvas_u_v << 16)|(pic->mc_canvas_u_v << 8) | pic->mc_canvas_y);
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"L0 refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i, pic->mc_canvas_u_v, pic->mc_canvas_y);
		}
	}
	if (avs3_dec->slice_type == SLICE_B) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for REF_1\n");

		WRITE_BACK_16(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 0, (16 << 8) | (0 << 1) | 1);
		for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_1]; i++) {
			pic = &avs3_dec->ctx.refp[i][REFP_1].pic->buf_cfg;
			WRITE_BACK_32(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
				(pic->mc_canvas_u_v << 16) | (pic->mc_canvas_u_v << 8) | pic->mc_canvas_y);
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"L1 refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i, pic->mc_canvas_u_v, pic->mc_canvas_y);
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

#define FB_PARSER_SAO0_BLOCK_SIZE       (4 * 1024 * 4)
#define FB_PARSER_SAO1_BLOCK_SIZE       (4 * 1024 * 4)
#define FB_MPRED_IMP0_BLOCK_SIZE        (4 * 1024 * 4)
#define FB_MPRED_IMP1_BLOCK_SIZE        (4 * 1024 * 4)

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
	trg->scalelut_ptr_pre = src->scalelut_ptr_pre;
}

static void print_loopbufs_ptr(struct AVS3Decoder_s *dec, char* mark, buff_ptr_t* ptr)
{
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL, "==%s==:\n", mark);
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"mmu0_ptr %x, mmu1_ptr %x\nscalelut_ptr %x pre_ptr %x\nvcpu_imem_ptr %x\nsys_imem_ptr %x (vir 0x%x: %02x %02x %02x %02x)\nlmem0_ptr %x, lmem1_ptr %x\nparser_sao0_ptr %x, parser_sao1_ptr %x\nmpred_imp0_ptr %x, mpred_imp1_ptr %x\n",
		ptr->mmu0_ptr,
		ptr->mmu1_ptr,
		ptr->scalelut_ptr,
		ptr->scalelut_ptr_pre,
		ptr->vcpu_imem_ptr,
		ptr->sys_imem_ptr,
		ptr->sys_imem_ptr_v,
		((u8 *)(ptr->sys_imem_ptr_v))[0], ((u8 *)(ptr->sys_imem_ptr_v))[1],
		((u8 *)(ptr->sys_imem_ptr_v))[2], ((u8 *)(ptr->sys_imem_ptr_v))[3],
		ptr->lmem0_ptr,
		ptr->lmem1_ptr,
		ptr->parser_sao0_ptr,
		ptr->parser_sao1_ptr,
		ptr->mpred_imp0_ptr,
		ptr->mpred_imp1_ptr);
}

static void print_loopbufs_ptr2(struct AVS3Decoder_s *dec, char* mark, buff_ptr_t* p_ptr, buff_ptr_t* ptr)
{
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL, "==%s==:\n", mark);
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"mmu0_ptr (%x=>%x%s) mmu1_ptr (%x=>%x%s)\nscalelut_ptr (%x=>%x%s) pre_ptr (%x=>%x%s)\nvcpu_imem_ptr (%x=>%x%s)\nsys_imem_ptr (%x=>%x%s) (vir 0x%x: %02x %02x %02x %02x)\nlmem0_ptr (%x=>%x%s), lmem1_ptr (%x=>%x%s)\nparser_sao0_ptr (%x=>%x%s), parser_sao1_ptr (%x=>%x%s)\nmpred_imp0_ptr (%x=>%x%s), mpred_imp1_ptr (%x=>%x%s)\n",
		p_ptr->mmu0_ptr, ptr->mmu0_ptr, (ptr->mmu0_ptr<p_ptr->mmu0_ptr) ? " wrap" : "",
		p_ptr->mmu1_ptr, ptr->mmu1_ptr, (ptr->mmu1_ptr<p_ptr->mmu1_ptr) ? " wrap" : "",
		p_ptr->scalelut_ptr, ptr->scalelut_ptr, (ptr->scalelut_ptr<p_ptr->scalelut_ptr) ? " wrap" : "",
		p_ptr->scalelut_ptr_pre, ptr->scalelut_ptr_pre, (ptr->scalelut_ptr_pre<p_ptr->scalelut_ptr_pre) ? " wrap" : "",
		p_ptr->vcpu_imem_ptr, ptr->vcpu_imem_ptr, (ptr->vcpu_imem_ptr<p_ptr->vcpu_imem_ptr) ? " wrap" : "",
		p_ptr->sys_imem_ptr, ptr->sys_imem_ptr, (ptr->sys_imem_ptr<p_ptr->sys_imem_ptr) ? " wrap" : "",
		p_ptr->sys_imem_ptr_v,
		((u8 *)(ptr->sys_imem_ptr_v))[0], ((u8 *)(ptr->sys_imem_ptr_v))[1],
		((u8 *)(ptr->sys_imem_ptr_v))[2], ((u8 *)(ptr->sys_imem_ptr_v))[3],
		p_ptr->lmem0_ptr, ptr->lmem0_ptr, (ptr->lmem0_ptr<p_ptr->lmem0_ptr) ? " wrap" : "",
		p_ptr->lmem1_ptr, ptr->lmem1_ptr, (ptr->lmem1_ptr<p_ptr->lmem1_ptr) ? " wrap" : "",
		p_ptr->parser_sao0_ptr, ptr->parser_sao0_ptr, (ptr->parser_sao0_ptr<p_ptr->parser_sao0_ptr) ? " wrap" : "",
		p_ptr->parser_sao1_ptr, ptr->parser_sao1_ptr, (ptr->parser_sao1_ptr<p_ptr->parser_sao1_ptr) ? " wrap" : "",
		p_ptr->mpred_imp0_ptr, ptr->mpred_imp0_ptr, (ptr->mpred_imp0_ptr<p_ptr->mpred_imp0_ptr) ? " wrap" : "",
		p_ptr->mpred_imp1_ptr, ptr->mpred_imp1_ptr, (ptr->mpred_imp1_ptr<p_ptr->mpred_imp1_ptr) ? " wrap" : "");
}

static void print_loopbufs_adr_size(struct AVS3Decoder_s *dec)
{
	int i;
	uint32_t adr, size;
	char *name[9] = {"parser_sao0", "parser_sao1", "mpred_imp0", "mpred_imp1", "scalelut", "lmem0", "lmem1", "vcpu_imem", "sys_imem"};

	avs3_print_cont(dec, 0, "loopbuf memory:\n");
	for (i = 0; i < 9; i++) {
		WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, i);
		adr = READ_VREG(HEVC_ASSIST_RING_F_START);
		size = READ_VREG(HEVC_ASSIST_RING_F_END) - adr;
		avs3_print_cont(dec, 0, "%s start 0x%x size 0x%x\n", name[i], adr, size);
	}
	avs3_print_flush(dec);
}

static u32 dump_fb_mmu_buffer(struct AVS3Decoder_s *dec, void *mmu_map_adr, u32 mmu_map_size, u8 *file)
{
	int i;
	u8 *adr = (u8 *)mmu_map_adr;
	u32 page_phy_adr;
	loff_t off = 0;
	int mode = O_CREAT | O_WRONLY | O_TRUNC;
	struct file *fp = NULL;

	u32 total_check_sum = 0;
	if (file) {
		fp = media_open(file, mode, 0666);
	}

	for (i = 0; i < mmu_map_size; i += 4) {
		page_phy_adr = (adr[i] | (adr[i + 1] << 8) | (adr[i + 2] << 16) | (adr[i + 3] << 24)) << 12;
		if (page_phy_adr)
			d_dump(dec, page_phy_adr, 1024 * 4, fp, &off, &total_check_sum, 0);
		else
			avs3_print(dec, PRINT_FLAG_VDEC_DETAIL, "!!%s mmu_map 0x%p, page count %d, page %d is zero\n",
				__func__, mmu_map_adr, mmu_map_size / 4, i / 4);
	}
	if (fp) {
		media_close(fp, current->files);
	}
	return total_check_sum;
}

static void fb_mmu_buffer_fill_zero(struct AVS3Decoder_s *dec, void *mmu_map_adr, u32 mmu_map_size)
{
	int i;
	u32 page_phy_adr;
	u8 *adr = (u8 *)mmu_map_adr;
	for (i = 0; i < mmu_map_size; i += 4) {
		page_phy_adr = (adr[i] | (adr[i + 1] << 8) | (adr[i + 2] << 16) | (adr[i + 3] << 24)) << 12;
		if (page_phy_adr)
			d_dump(dec, page_phy_adr, 1024 * 4, NULL, NULL, NULL, 0);
		else
			avs3_print(dec, PRINT_FLAG_VDEC_DETAIL, "!!%s mmu_map 0x%p, page count %d, page %d is zero\n",
				__func__, mmu_map_adr, mmu_map_size / 4, i / 4);
	}
}

static void dump_loop_buffer(struct AVS3Decoder_s *dec, int count, u8 save_file)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int i;
	u32 total_check_sum;
	uint32_t adr, size;
	char *name[9] = {"parser_sao0", "parser_sao1", "mpred_imp0", "mpred_imp1", "scalelut", "lmem0", "lmem1", "vcpu_imem", "sys_imem"};
	char file[64];
	char mark[16];
	for (i = 0; i < 9; i++) {
		WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, i);
		adr = READ_VREG(HEVC_ASSIST_RING_F_START);
		size = READ_VREG(HEVC_ASSIST_RING_F_END) - adr;
		if (save_file) {
			if (count >= 0)
				sprintf(&file[0], "/data/tmp/%s_%d", name[i], count);
			else
				sprintf(&file[0], "/data/tmp/%s", name[i]);
		}
		sprintf(&mark[0], "%s %d", name[i], count);
		dump_or_fill_phy_buffer(dec, adr, size, save_file ? file : NULL, 0, mark);
	}
	//dump mmu0
	if (save_file) {
		if (count >= 0)
			sprintf(&file[0], "/data/tmp/%s_%d", "mmu0", count);
		else
			sprintf(&file[0], "/data/tmp/%s", "mmu0");
	}
	total_check_sum = dump_fb_mmu_buffer(dec, dec->fb_buf_mmu0_addr, avs3_dec->fb_buf_mmu0.buf_size, save_file ? file : NULL);
	avs3_print(dec, 0, "dump mmu0 %d (pages 0x%x) check_sum %x %s\n", count, avs3_dec->fb_buf_mmu0.buf_size / 4, total_check_sum, save_file ? file : "");

	//dump mmu1
	if (save_file) {
		if (count >= 0)
			sprintf(&file[0], "/data/tmp/%s_%d", "mmu1", count);
		else
			sprintf(&file[0], "/data/tmp/%s", "mmu1");
	}
	total_check_sum = dump_fb_mmu_buffer(dec, dec->fb_buf_mmu1_addr, avs3_dec->fb_buf_mmu1.buf_size, save_file ? file : NULL);
	avs3_print(dec, 0, "dump mmu1 %d (pages 0x%x) check_sum %x %s\n", count, avs3_dec->fb_buf_mmu1.buf_size / 4, total_check_sum, save_file ? file : "");

	//dump bufspec
	if (save_file) {
		if (count >= 0)
			sprintf(&file[0], "/data/tmp/%s_%d", "bufspec", count);
		else
			sprintf(&file[0], "/data/tmp/%s", "bufspec");
	}
	sprintf(&mark[0], "%s %d", "bufspec", count);
	dump_or_fill_phy_buffer(dec, dec->buf_start, dec->buf_size, save_file ? file : NULL, 0, mark);
}

static void loop_buffer_fill_zero(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int i;
	uint32_t adr, size;
	for (i = 0; i < 9; i++) {
		WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, i);
		adr = READ_VREG(HEVC_ASSIST_RING_F_START);
		size = READ_VREG(HEVC_ASSIST_RING_F_END) - adr;
		dump_or_fill_phy_buffer(dec, adr, size, NULL, 1, NULL);
	}
	fb_mmu_buffer_fill_zero(dec, dec->fb_buf_mmu0_addr, avs3_dec->fb_buf_mmu0.buf_size);
	fb_mmu_buffer_fill_zero(dec, dec->fb_buf_mmu1_addr, avs3_dec->fb_buf_mmu1.buf_size);

	dump_or_fill_phy_buffer(dec, dec->buf_start, dec->buf_size, NULL, 1, NULL);
}

static int init_mmu_fb_bufstate(struct AVS3Decoder_s *dec, int mmu_fb_4k_number)
{
	int ret;
	dma_addr_t tmp_phy_adr;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int mmu_map_size = ((mmu_fb_4k_number * 4) >> 6) << 6;
	int tvp_flag = vdec_secure(hw_to_vdec(dec)) ? CODEC_MM_FLAGS_TVP : 0;

	avs3_print(dec, AVS3_DBG_BUFMGR,
		"%s mmu_fb_4k_number = %d\n", __func__, mmu_fb_4k_number);

	if (mmu_fb_4k_number < 0)
		return -1;

	dec->mmu_box_fb = decoder_mmu_box_alloc_box(DRIVER_NAME,
		dec->index, 2,
		(mmu_fb_4k_number << 12) * 2,
		tvp_flag
		);

	dec->fb_buf_mmu0_addr =
		dma_alloc_coherent(amports_get_dma_device(),
		mmu_map_size,
		&tmp_phy_adr, GFP_KERNEL);
	avs3_dec->fb_buf_mmu0.buf_start = tmp_phy_adr;
	if (dec->fb_buf_mmu0_addr == NULL) {
		pr_err("%s: failed to alloc fb_mmu0_map\n", __func__);
		return -1;
	}
	memset(dec->fb_buf_mmu0_addr, 0, mmu_map_size);
	avs3_dec->fb_buf_mmu0.buf_size = mmu_map_size;
	avs3_dec->fb_buf_mmu0.buf_end = avs3_dec->fb_buf_mmu0.buf_start + mmu_map_size;

	dec->fb_buf_mmu1_addr =
		dma_alloc_coherent(amports_get_dma_device(),
		mmu_map_size,
		&tmp_phy_adr, GFP_KERNEL);
	avs3_dec->fb_buf_mmu1.buf_start = tmp_phy_adr;
	if (dec->fb_buf_mmu1_addr == NULL) {
		pr_err("%s: failed to alloc fb_mmu1_map\n", __func__);
		return -1;
	}
	memset(dec->fb_buf_mmu1_addr, 0, mmu_map_size);
	avs3_dec->fb_buf_mmu1.buf_size = mmu_map_size;
	avs3_dec->fb_buf_mmu1.buf_end = avs3_dec->fb_buf_mmu1.buf_start + mmu_map_size;

	ret = decoder_mmu_box_alloc_idx(
		dec->mmu_box_fb,
		0,
		mmu_fb_4k_number,
		dec->fb_buf_mmu0_addr);
	if (ret != 0) {
		pr_err("%s: failed to alloc fb_mmu0 pages", __func__);
		return -1;
	}

	ret = decoder_mmu_box_alloc_idx(
		dec->mmu_box_fb,
		1,
		mmu_fb_4k_number,
		dec->fb_buf_mmu1_addr);
	if (ret != 0) {
		pr_err("%s: failed to alloc fb_mmu1 pages", __func__);
		return -1;
	}

	dec->mmu_fb_4k_number = mmu_fb_4k_number;
	avs3_dec->fr.mmu0_ptr = avs3_dec->fb_buf_mmu0.buf_start;
	avs3_dec->bk.mmu0_ptr = avs3_dec->fb_buf_mmu0.buf_start;
	avs3_dec->fr.mmu1_ptr = avs3_dec->fb_buf_mmu1.buf_start;
	avs3_dec->bk.mmu1_ptr = avs3_dec->fb_buf_mmu1.buf_start;

	return 0;
}

static void init_fb_bufstate(struct AVS3Decoder_s *dec)
{
	/*simulation code: change to use linux APIs; also need write uninit_fb_bufstate()*/
	int ret;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	dma_addr_t tmp_phy_adr;
	unsigned long tmp_adr;
	int mmu_4k_number = dec->fb_ifbuf_num * avs3_mmu_page_num(dec, dec->init_pic_w, dec->init_pic_h, 1);

	ret = init_mmu_fb_bufstate(dec, mmu_4k_number);
	if (ret < 0) {
		avs3_print(dec, 0, "%s: failed to alloc mmu fb buffer\n", __func__);
		return ;
	}

	avs3_dec->fb_buf_scalelut.buf_size = IFBUF_SCALELUT_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_SCALELUT_ID, avs3_dec->fb_buf_scalelut.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_scalelut.buf_start = tmp_adr;
	avs3_dec->fb_buf_scalelut.buf_end = avs3_dec->fb_buf_scalelut.buf_start + avs3_dec->fb_buf_scalelut.buf_size;

	avs3_dec->fb_buf_vcpu_imem.buf_size = IFBUF_VCPU_IMEM_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_VCPU_IMEM_ID, avs3_dec->fb_buf_vcpu_imem.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_vcpu_imem.buf_start = tmp_adr;
	avs3_dec->fb_buf_vcpu_imem.buf_end = avs3_dec->fb_buf_vcpu_imem.buf_start + avs3_dec->fb_buf_vcpu_imem.buf_size;

	avs3_dec->fb_buf_sys_imem.buf_size = IFBUF_SYS_IMEM_SIZE * dec->fb_ifbuf_num;
	avs3_dec->fb_buf_sys_imem_addr = dma_alloc_coherent(amports_get_dma_device(),
		avs3_dec->fb_buf_sys_imem.buf_size, &tmp_phy_adr, GFP_KERNEL);
	avs3_dec->fb_buf_sys_imem.buf_start = tmp_phy_adr;
	if (avs3_dec->fb_buf_sys_imem_addr == NULL) {
		pr_err("%s: failed to alloc fb_buf_sys_imem\n", __func__);
		return;
	}
	avs3_dec->fb_buf_sys_imem.buf_end = avs3_dec->fb_buf_sys_imem.buf_start + avs3_dec->fb_buf_sys_imem.buf_size;

	avs3_dec->fb_buf_lmem0.buf_size = IFBUF_LMEM0_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_LMEM0_ID, avs3_dec->fb_buf_lmem0.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_lmem0.buf_start = tmp_adr;
	avs3_dec->fb_buf_lmem0.buf_end = avs3_dec->fb_buf_lmem0.buf_start + avs3_dec->fb_buf_lmem0.buf_size;

	avs3_dec->fb_buf_lmem1.buf_size = IFBUF_LMEM1_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_LMEM1_ID, avs3_dec->fb_buf_lmem1.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_lmem1.buf_start = tmp_adr;
	avs3_dec->fb_buf_lmem1.buf_end = avs3_dec->fb_buf_lmem1.buf_start + avs3_dec->fb_buf_lmem1.buf_size;

	avs3_dec->fb_buf_parser_sao0.buf_size = IFBUF_PARSER_SAO0_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_PARSER_SAO0_ID, avs3_dec->fb_buf_parser_sao0.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_parser_sao0.buf_start = tmp_adr;
	avs3_dec->fb_buf_parser_sao0.buf_end = avs3_dec->fb_buf_parser_sao0.buf_start + avs3_dec->fb_buf_parser_sao0.buf_size;

	avs3_dec->fb_buf_parser_sao1.buf_size = IFBUF_PARSER_SAO1_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUF_PARSER_SAO1_ID, avs3_dec->fb_buf_parser_sao1.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_parser_sao1.buf_start = tmp_adr;
	avs3_dec->fb_buf_parser_sao1.buf_end = avs3_dec->fb_buf_parser_sao1.buf_start + avs3_dec->fb_buf_parser_sao1.buf_size;

	avs3_dec->fb_buf_mpred_imp0.buf_size = IFBUF_MPRED_IMP0_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUFF_MPRED_IMP0_ID, avs3_dec->fb_buf_mpred_imp0.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_mpred_imp0.buf_start = tmp_adr;
	avs3_dec->fb_buf_mpred_imp0.buf_end = avs3_dec->fb_buf_mpred_imp0.buf_start + avs3_dec->fb_buf_mpred_imp0.buf_size;

	avs3_dec->fb_buf_mpred_imp1.buf_size = IFBUF_MPRED_IMP1_SIZE * dec->fb_ifbuf_num;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
		BMMU_IFBUFF_MPRED_IMP1_ID, avs3_dec->fb_buf_mpred_imp1.buf_size,
		DRIVER_NAME, &tmp_adr);
	avs3_dec->fb_buf_mpred_imp1.buf_start = tmp_adr;
	avs3_dec->fb_buf_mpred_imp1.buf_end = avs3_dec->fb_buf_mpred_imp1.buf_start + avs3_dec->fb_buf_mpred_imp1.buf_size;

	avs3_dec->fr.scalelut_ptr = avs3_dec->fb_buf_scalelut.buf_start;
	avs3_dec->bk.scalelut_ptr = avs3_dec->fb_buf_scalelut.buf_start;
	avs3_dec->fr.vcpu_imem_ptr = avs3_dec->fb_buf_vcpu_imem.buf_start;
	avs3_dec->bk.vcpu_imem_ptr = avs3_dec->fb_buf_vcpu_imem.buf_start;
	avs3_dec->fr.sys_imem_ptr = avs3_dec->fb_buf_sys_imem.buf_start;
	avs3_dec->bk.sys_imem_ptr = avs3_dec->fb_buf_sys_imem.buf_start;
	avs3_dec->fr.lmem0_ptr = avs3_dec->fb_buf_lmem0.buf_start;
	avs3_dec->bk.lmem0_ptr = avs3_dec->fb_buf_lmem0.buf_start;
	avs3_dec->fr.lmem1_ptr = avs3_dec->fb_buf_lmem1.buf_start;
	avs3_dec->bk.lmem1_ptr = avs3_dec->fb_buf_lmem1.buf_start;
	avs3_dec->fr.parser_sao0_ptr = avs3_dec->fb_buf_parser_sao0.buf_start;
	avs3_dec->bk.parser_sao0_ptr = avs3_dec->fb_buf_parser_sao0.buf_start;
	avs3_dec->fr.parser_sao1_ptr = avs3_dec->fb_buf_parser_sao1.buf_start;
	avs3_dec->bk.parser_sao1_ptr = avs3_dec->fb_buf_parser_sao1.buf_start;
	avs3_dec->fr.mpred_imp0_ptr = avs3_dec->fb_buf_mpred_imp0.buf_start;
	avs3_dec->bk.mpred_imp0_ptr = avs3_dec->fb_buf_mpred_imp0.buf_start;
	avs3_dec->fr.mpred_imp1_ptr = avs3_dec->fb_buf_mpred_imp1.buf_start;
	avs3_dec->bk.mpred_imp1_ptr = avs3_dec->fb_buf_mpred_imp1.buf_start;
	if (fbdebug_flag & 0x4) {
		avs3_dec->fr.scalelut_ptr_pre = avs3_dec->fr.scalelut_ptr;
		avs3_dec->bk.scalelut_ptr_pre = avs3_dec->bk.scalelut_ptr;
	} else {
		avs3_dec->fr.scalelut_ptr_pre = 0;
		avs3_dec->bk.scalelut_ptr_pre = 0;
	}

	avs3_dec->fr.sys_imem_ptr_v = avs3_dec->fb_buf_sys_imem_addr; //for linux

	print_loopbufs_ptr(dec, "init", &avs3_dec->fr);
}

static void uninit_mmu_fb_bufstate(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;

	if (dec->fb_buf_mmu0_addr) {
		dma_free_coherent(amports_get_dma_device(),
			avs3_dec->fb_buf_mmu0.buf_size, dec->fb_buf_mmu0_addr,
			avs3_dec->fb_buf_mmu0.buf_start);
		dec->fb_buf_mmu0_addr = NULL;
	}
	if (dec->fb_buf_mmu1_addr) {
		dma_free_coherent(amports_get_dma_device(),
			avs3_dec->fb_buf_mmu1.buf_size, dec->fb_buf_mmu1_addr,
			avs3_dec->fb_buf_mmu1.buf_start);
		dec->fb_buf_mmu1_addr = NULL;
	}

	if (dec->mmu_box_fb) {
		decoder_mmu_box_free(dec->mmu_box_fb);
		dec->mmu_box_fb = NULL;
	}
}

static void uninit_fb_bufstate(struct AVS3Decoder_s *dec)
{
	int i;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	for (i = 0; i < FB_LOOP_BUF_COUNT; i++) {
		if (i != BMMU_IFBUF_SYS_IMEM_ID)
		decoder_bmmu_box_free_idx(dec->bmmu_box, i);
	}

	if (avs3_dec->fb_buf_sys_imem_addr) {
		dma_free_coherent(amports_get_dma_device(),
			avs3_dec->fb_buf_sys_imem.buf_size, avs3_dec->fb_buf_sys_imem_addr,
			avs3_dec->fb_buf_sys_imem.buf_start);
		avs3_dec->fb_buf_sys_imem_addr = NULL;
	}
	uninit_mmu_fb_bufstate(dec);
}

static void config_bufstate_front_hw(struct avs3_decoder *avs3_dec)
{
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_START, avs3_dec->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0_END, avs3_dec->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0, avs3_dec->fr.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_START, avs3_dec->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1_END, avs3_dec->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1, avs3_dec->fr.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_parser_sao0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_parser_sao0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.parser_sao0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.parser_sao0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_parser_sao1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_parser_sao1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.parser_sao1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.parser_sao1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	//    config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.mpred_imp0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.mpred_imp1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	// config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_scalelut.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_scalelut.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.scalelut_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.scalelut_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, avs3_dec->fr.scalelut_ptr_pre);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_vcpu_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_vcpu_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.vcpu_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.vcpu_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.sys_imem_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, 0);

	//config lmem buffers
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_lmem0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_lmem0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.lmem0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.lmem0_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, FB_IFBUF_LMEM0_BLOCK_SIZE);

	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_F_START, avs3_dec->fb_buf_lmem1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_F_END, avs3_dec->fb_buf_lmem1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, avs3_dec->fr.lmem1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_RPTR, avs3_dec->bk.lmem1_ptr);
	//WRITE_VREG(HEVC_ASSIST_RING_F_THRESHOLD, FB_IFBUF_LMEM1_BLOCK_SIZE);
}

static void config_bufstate_back_hw(struct avs3_decoder *avs3_dec)
{
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_START, avs3_dec->fb_buf_mmu0.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0_END, avs3_dec->fb_buf_mmu0.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0, avs3_dec->bk.mmu0_ptr);

	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_START, avs3_dec->fb_buf_mmu1.buf_start);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1_END, avs3_dec->fb_buf_mmu1.buf_end);
	WRITE_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1, avs3_dec->bk.mmu1_ptr);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_parser_sao0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_parser_sao0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.parser_sao0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 1);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_parser_sao1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_parser_sao1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.parser_sao1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	//    config mpred_imp_if data write buffer start address
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 2);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_mpred_imp0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_mpred_imp0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.mpred_imp0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 3);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_mpred_imp1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_mpred_imp1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.mpred_imp1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	// config other buffers
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 4);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_scalelut.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_scalelut.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.scalelut_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, avs3_dec->bk.scalelut_ptr_pre);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 7);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_vcpu_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_vcpu_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.vcpu_imem_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 8);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_sys_imem.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_sys_imem.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.sys_imem_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	// config lmem buffers
	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 5);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_lmem0.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_lmem0.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.lmem0_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);

	WRITE_VREG(HEVC_ASSIST_RING_B_INDEX, 6);
	WRITE_VREG(HEVC_ASSIST_RING_B_START, avs3_dec->fb_buf_lmem1.buf_start);
	WRITE_VREG(HEVC_ASSIST_RING_B_END, avs3_dec->fb_buf_lmem1.buf_end);
	WRITE_VREG(HEVC_ASSIST_RING_B_RPTR, avs3_dec->bk.lmem1_ptr);
	WRITE_VREG(HEVC_ASSIST_RING_B_THRESHOLD, 0);
}

static void read_bufstate_front(struct avs3_decoder *avs3_dec)
{
	//uint32_t tmp;
	avs3_dec->fr.mmu0_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR0);
	avs3_dec->fr.mmu1_ptr = READ_VREG(HEVC_ASSIST_FB_MMU_MAP_ADDR1);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 4);
	avs3_dec->fr.scalelut_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	//avs3_dec->fr.scalelut_ptr_pre = READ_VREG(HEVC_ASSIST_RING_F_THRESHOLD);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 7);
	avs3_dec->fr.vcpu_imem_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 5);
	avs3_dec->fr.lmem0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 6);
	avs3_dec->fr.lmem1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 0);
	avs3_dec->fr.parser_sao0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 1);
	avs3_dec->fr.parser_sao1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 2);
	avs3_dec->fr.mpred_imp0_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);
	WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 3);
	avs3_dec->fr.mpred_imp1_ptr = READ_VREG(HEVC_ASSIST_RING_F_WPTR);

	avs3_dec->fr.sys_imem_ptr = avs3_dec->sys_imem_ptr;
	avs3_dec->fr.sys_imem_ptr_v = avs3_dec->sys_imem_ptr_v;
}

static int  compute_losless_comp_body_size(struct AVS3Decoder_s *dec,
	int width, int height,
	uint8_t is_bit_depth_10);
static  int  compute_losless_comp_header_size(struct AVS3Decoder_s *dec,
	int width, int height);

static void config_work_space_hw(struct AVS3Decoder_s *dec, uint8_t front_flag, uint8_t back_flag)
{
	uint32_t data32;
	DEC_CTX * ctx = &dec->avs3_dec.ctx;
	struct BuffInfo_s *buf_spec = dec->work_space_buf;
	u32 width = dec->avs3_dec.img.width ? dec->avs3_dec.img.width : dec->init_pic_w;
	u32 height = dec->avs3_dec.img.height ? dec->avs3_dec.img.height : dec->init_pic_h;
	u8 is_bit_depth_10 = (ctx->info.bit_depth_internal == 8) ? 0 : 1;
	int losless_comp_header_size = compute_losless_comp_header_size(dec, width, height);
	int losless_comp_body_size = compute_losless_comp_body_size(dec, width, height, is_bit_depth_10);
	int losless_comp_body_size_dw = losless_comp_body_size;
	int losless_comp_header_size_dw = losless_comp_header_size;
	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", __func__,
		buf_spec->ipp.buf_start,
		buf_spec->ipp1.buf_start,
		buf_spec->start_adr,
		buf_spec->short_term_rps.buf_start,
		buf_spec->rcs.buf_start,
		buf_spec->sps.buf_start,
		buf_spec->pps.buf_start,
		buf_spec->sbac_top.buf_start,
		buf_spec->sao_up.buf_start,
		buf_spec->swap_buf.buf_start,
		buf_spec->swap_buf2.buf_start,
		buf_spec->scalelut.buf_start,
		buf_spec->dblk_para.buf_start,
		buf_spec->dblk_data.buf_start,
		buf_spec->dblk_data2.buf_start);
	if (back_flag) {
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE,buf_spec->ipp.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE_DBE1,buf_spec->ipp1.buf_start);
		WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE2_DBE1,buf_spec->ipp.buf_start);
	}
	if (front_flag) {
		if ((debug & AVS3_DBG_SEND_PARAM_WITH_REG) == 0) {
			WRITE_VREG(HEVC_RPM_BUFFER, (u32)dec->rpm_phy_addr);
		}

		WRITE_VREG(AVS3_ALF_SWAP_BUFFER, buf_spec->short_term_rps.buf_start);
		WRITE_VREG(HEVC_RCS_BUFFER, buf_spec->rcs.buf_start);
		WRITE_VREG(AVS3_SBAC_TOP_BUFFER, buf_spec->sbac_top.buf_start);
	}
	if (back_flag) {
		WRITE_VREG(HEVC_SAO_UP, buf_spec->sao_up.buf_start);
		WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR, dec->frame_mmu_map_phy_addr);
		WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR_DBE1, dec->frame_mmu_map_phy_addr_1); //new dual
	}

	if (back_flag) {
#ifdef AVS3_10B_MMU_DW
		if (dec->dw_mmu_enable) {
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2, dec->dw_frame_mmu_map_phy_addr);
			WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2_DBE1, dec->dw_frame_mmu_map_phy_addr_1); //new dual
		}
#endif
		WRITE_VREG(HEVC_STREAM_SWAP_BUFFER2, buf_spec->swap_buf2.buf_start);

		WRITE_VREG(HEVC_DBLK_CFG4, buf_spec->dblk_para.buf_start);  // cfg_cpi_addr
		WRITE_VREG(HEVC_DBLK_CFG4_DBE1, buf_spec->dblk_para.buf_start);
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		" [DBLK DBG] CFG4: 0x%x\n", buf_spec->dblk_para.buf_start);
		WRITE_VREG(HEVC_DBLK_CFG5, buf_spec->dblk_data.buf_start); // cfg_xio_addr
		WRITE_VREG(HEVC_DBLK_CFG5_DBE1, buf_spec->dblk_data.buf_start);
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		" [DBLK DBG] CFG5: 0x%x\n", buf_spec->dblk_data.buf_start);
		WRITE_VREG(HEVC_DBLK_CFGE, buf_spec->dblk_data2.buf_start); // cfg_adp_addr
		WRITE_VREG(HEVC_DBLK_CFGE_DBE1, buf_spec->dblk_data2.buf_start);
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		" [DBLK DBG] CFGE: 0x%x\n", buf_spec->dblk_data2.buf_start);

#ifdef LOSLESS_COMPRESS_MODE
		data32 = READ_VREG(HEVC_SAO_CTRL5);
		data32 &= ~(1 << 9);
		WRITE_VREG(HEVC_SAO_CTRL5, data32);

		data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
		data32 &= ~(1 << 9);
		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
#ifdef AVS3_10B_MMU
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0x1 << 4)); // bit[4] : paged_mem_mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2, 0x0);
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, (0x1 << 4)); // bit[4] : paged_mem_mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1, 0x0);
#else
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0 << 3)); // bit[3] smem mode
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, (0 << 3)); // bit[3] smem mode

		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2, (losless_comp_body_size >> 5));
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL2_DBE1, (losless_comp_body_size >> 5));
#endif
		WRITE_VREG(HEVC_CM_BODY_LENGTH, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH, losless_comp_header_size);
		WRITE_VREG(HEVC_CM_BODY_LENGTH_DBE1, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET_DBE1, losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH_DBE1, losless_comp_header_size);
#else
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, 0x1 << 31);
		WRITE_VREG(HEVCD_MPP_DECOMP_CTL1_DBE1, 0x1 << 31);
#endif
#ifdef AVS3_10B_MMU
		WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR, buf_spec->mmu_vbh.buf_start);
		WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size / 4);
		WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR_DBE1, buf_spec->mmu_vbh.buf_start  + buf_spec->mmu_vbh.buf_size / 2);
		WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR_DBE1, buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size / 2 + buf_spec->mmu_vbh.buf_size / 4);

		/* use HEVC_CM_HEADER_START_ADDR */
		data32 = READ_VREG(HEVC_SAO_CTRL5);
		data32 |= (1 << 10);
		WRITE_VREG(HEVC_SAO_CTRL5, data32);

		data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
		data32 |= (1 << 10);
		WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
#endif
#ifdef AVS3_10B_MMU_DW
		if (dec->dw_mmu_enable) {
			WRITE_VREG(HEVC_CM_BODY_LENGTH2, losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_OFFSET2, losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_LENGTH2, losless_comp_header_size_dw);
			WRITE_VREG(HEVC_CM_BODY_LENGTH2_DBE1, losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_OFFSET2_DBE1, losless_comp_body_size_dw);
			WRITE_VREG(HEVC_CM_HEADER_LENGTH2_DBE1, losless_comp_header_size_dw);

			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2, buf_spec->mmu_vbh_dw.buf_start);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size / 4);
			WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2_DBE1, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size / 2);
			WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2_DBE1, buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size / 2 + buf_spec->mmu_vbh_dw.buf_size / 4);

#ifndef AVS3_10B_NV21
#ifdef AVS3_10B_MMU_DW
			WRITE_VREG(HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
			WRITE_VREG(HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
			WRITE_VREG(HEVC_DW_VH0_ADDDR_DBE1, DOUBLE_WRITE_VH0_HALF);
			WRITE_VREG(HEVC_DW_VH1_ADDDR_DBE1, DOUBLE_WRITE_VH1_HALF);
#endif
#endif
			/* use HEVC_CM_HEADER_START_ADDR */
			data32 = READ_VREG(HEVC_SAO_CTRL5);
			data32 |= (1 << 15);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			data32 = READ_VREG(HEVC_SAO_CTRL5_DBE1);
			data32 |= (1 << 15);
			WRITE_VREG(HEVC_SAO_CTRL5_DBE1, data32);
		}
#endif
	} //back_flag end
	if (front_flag) {
		WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, buf_spec->mpred_above.buf_start);
#ifdef CO_MV_COMPRESS
		data32 = READ_VREG(HEVC_MPRED_CTRL4);
		data32 |=  (1<<1 | 1<<26);
		WRITE_VREG(HEVC_MPRED_CTRL4, data32);
#else
		data32 = READ_VREG(HEVC_MPRED_CTRL4);
		data32 |=  (1<<26); //enable AVS3 mode
		WRITE_VREG(HEVC_MPRED_CTRL4, data32);
#endif

	}
}

static void hevc_init_decoder_hw(struct AVS3Decoder_s *dec, uint8_t front_flag, uint8_t back_flag)
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

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Entering hevc_init_decoder_hw\n");

#if 0 //def AVS3_10B_HED_FB
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Init AVS3_10B_HED_FB\n");
	data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
	data32 = data32 |
		(1 << 3) | // fb_read_avs2_enable0
		(1 << 6) | // fb_read_avs2_enable1
		(1 << 1) | // fb_avs2_enable
		(1 << 13) | // fb_read_avs3_enable0
		(1 << 14) | // fb_read_avs3_enable1
		(1 << 9) | // fb_avs3_enable
		(3 << 7)  //core0_en, core1_en,hed_fb_en
		;
	WRITE_VREG(HEVC_ASSIST_FB_CTL, data32); // new dual
#endif
	if (!efficiency_mode && front_flag) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Enable HEVC Parser Interrupt\n");
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

		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Enable HEVC Parser Shift\n");

		data32 = READ_VREG(HEVC_SHIFT_STATUS);
#ifdef AVS3
		data32 = data32 |
		(0 << 1) |  // emulation_check_on // AVS3 emulation on/off will be controlled in microcode according to startcode type
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
			(6 << 20) |  // emu_push_bits  (6-bits for AVS3)
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

	if (front_flag) {
		if (efficiency_mode)
			WRITE_VREG(HEVC_EFFICIENCY_MODE, 1);
		else
			WRITE_VREG(HEVC_EFFICIENCY_MODE, 0);
	}

	if (back_flag) {
		// Zero out canvas registers in IPP -- avoid simulation X
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0 << 1) | 1);
		for (i = 0; i < 32; i++) {
			WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#ifdef DUAL_CORE_64
			WRITE_VREG(HEVC2_HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#endif
		}
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR_DBE1, (0 << 8) | (0 << 1) | 1);
		for (i = 0; i < 32; i++) {
			WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR_DBE1, 0);
#ifdef DUAL_CORE_64
			WRITE_VREG(HEVC2_HEVCD_MPP_ANC_CANVAS_DATA_ADDR_DBE1, 0);
#endif
		}
	}

	if (front_flag) {
#if 0
#ifdef ENABLE_SWAP_TEST
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 100);
#else
		WRITE_VREG(HEVC_STREAM_SWAP_TEST, 0);
#endif
#endif
		if (!efficiency_mode) {
		WRITE_VREG(HEVC_PARSER_IF_CONTROL,
		(1 << 12) | // alf_on_sao
		//  (1 << 9) | // parser_alf_if_en
		//  (1 << 8) | // sao_sw_pred_enable
		(1 << 5) | // parser_sao_if_en
		(1 << 2) | // parser_mpred_if_en
		(1 << 0) // parser_scaler_if_en
		);
		}
	}

	// AVS3 default seq_wq_matrix config
	if (back_flag) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Config AVS3 default seq_wq_matrix ...\n");
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

		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Reset IPP\n");
		WRITE_VREG(HEVCD_IPP_TOP_CNTL,
			(0 << 1) | // enable ipp
			(1 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
			(1 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL,
			(1 << 1) | // enable ipp
			(1 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
			(0 << 0)   // software reset ipp and mpp
			);

		WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
			(0 << 1) | // enable ipp
			(1 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
			(1 << 0)   // software reset ipp and mpp
			);
		WRITE_VREG(HEVCD_IPP_TOP_CNTL_DBE1,
			(1 << 1) | // enable ipp
			(1 << 3) | // bit[5:3] 000:HEVC, 010:VP9 , 100:avs2, 110:av1 001:avs3
			(0 << 0)   // software reset ipp and mpp
			);

		// Init dblk
		data32 = READ_VREG(HEVC_DBLK_CFGB);
		data32 |= (5 << 0);
		WRITE_VREG(HEVC_DBLK_CFGB, data32); // [3:0] cfg_video_type -> AVS3
#define LPF_LINEBUF_MODE_CTU_BASED
		//must be defined for DUAL_CORE
#ifdef LPF_LINEBUF_MODE_CTU_BASED
		WRITE_VREG(HEVC_DBLK_CFG0, (0<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#else
		WRITE_VREG(HEVC_DBLK_CFG0, (1<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#endif
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Bitstream level Init for DBLK .Done.\n");

		data32 = READ_VREG(HEVC_DBLK_CFGB_DBE1);
		data32 |= (5 << 0);
		WRITE_VREG(HEVC_DBLK_CFGB_DBE1, data32); // [3:0] cfg_video_type -> AVS3
#ifdef LPF_LINEBUF_MODE_CTU_BASED
		WRITE_VREG(HEVC_DBLK_CFG0_DBE1, (0<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#else
		WRITE_VREG(HEVC_DBLK_CFG0_DBE1, (1<<18) | (1 << 0)); // [18]tile based line buffer storage mode [0] rst_sync(will be self cleared)
#endif
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Bitstream level Init for DBLK_DBE1 .Done.\n");
#if 1
		if (dec->front_back_mode == 1) {
			mcrcc_perfcount_reset_dual(dec);
			decomp_perfcount_reset_dual(dec);
		}
#endif
	}//back_flag end
	return;
}

extern void config_cuva_buf(struct AVS3Decoder_s *dec);
static int32_t avs3_hw_init(struct AVS3Decoder_s *dec, uint8_t front_flag, uint8_t back_flag)
{
	uint32_t data32;
	unsigned int decode_mode;
	uint32_t tmp = 0;
	avs3_print(dec, AVS3_DBG_BUFMGR_MORE, "%s front_flag %d back_flag %d\n", __func__, front_flag, back_flag);
	if (dec->front_back_mode != 1) {
		if (front_flag)
			avs3_hw_ctx_restore(dec);
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
		data32 = data32 | (3 << 7) | (1 << 9) | (1 << 1);
		tmp = (1 << 0) | (1 << 10);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32);
	}

	if (back_flag) {
		data32 = READ_VREG(HEVC_ASSIST_FB_CTL);
		data32 = data32 | (3 << 7) | (1 << 3) | (1 << 13) | (1 << 6) | (1 << 14);
		tmp = (1 << 2) |  (1 << 11) | (1 << 5) | (1 << 12);
		data32 &= ~tmp;
		WRITE_VREG(HEVC_ASSIST_FB_CTL, data32);
	}

	config_work_space_hw(dec, front_flag, back_flag);

	if (!efficiency_mode && dec->pic_list_init_flag && front_flag)
		init_pic_list_hw_fb(dec);

	hevc_init_decoder_hw(dec, front_flag, back_flag);
	//Start JT
	if (!efficiency_mode) {
	if (front_flag) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[test.c] Enable BitStream Fetch\n");
		WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x00000100);
		WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x00000000); // 0x000000 - 0x000003 emulate code for AVS3
	}
	}
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
			WRITE_VREG(HEVCD_IPP_DYN_CACHE, 0x2b);//enable new mcrcc
			WRITE_VREG(HEVCD_IPP_DYN_CACHE_DBE1, 0x2b);//enable new mcrcc
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "HEVC DYN MCRCC\n");
		}
#endif
	}
	if (front_flag) {
		WRITE_VREG(LMEM_DUMP_ADR, (u32)dec->lmem_phy_addr);
#if 1 // JT
		WRITE_VREG(HEVC_WAIT_FLAG, 1);
		/* clear mailbox interrupt */
		WRITE_VREG(dec->ASSIST_MBOX0_CLR_REG, 1);

		/* enable mailbox interrupt */
		WRITE_VREG(dec->ASSIST_MBOX0_MASK, 1);

		/* disable PSCALE for hardware sharing */
#ifdef DOS_PROJECT
#else
		WRITE_VREG(HEVC_PSCALE_CTRL, 0);
#endif

		//WRITE_VREG(DEBUG_REG1, 0x0);  //no debug
		WRITE_VREG(NAL_SEARCH_CTL, 0x8); //check SEQUENCE/I_PICTURE_START in ucode
		WRITE_VREG(DECODE_STOP_POS, udebug_flag);
#if (defined DEBUG_UCODE_LOG) || (defined DEBUG_CMD)
		WRITE_VREG(HEVC_DBG_LOG_ADR, dec->ucode_log_phy_addr);
#endif
#ifdef MULTI_INSTANCE_SUPPORT
		if (!dec->m_ins_flag)
			decode_mode = DECODE_MODE_SINGLE;
		else if (vdec_frame_based(hw_to_vdec(dec)))
			decode_mode = DECODE_MODE_MULTI_FRAMEBASE;
		else
			decode_mode = DECODE_MODE_MULTI_STREAMBASE;
#ifdef FOR_S5
		/*to do..*/
		dec->start_decoding_flag = 3;
		decode_mode |= (dec->start_decoding_flag << 16);
#endif
		avs3_print(dec, AVS3_DBG_BUFMGR_MORE, "%s set decode_mode 0x%x\n", __func__, decode_mode);
		WRITE_VREG(DECODE_MODE, decode_mode); //DECODE_MODE_MULTI_STREAMBASE
		//WRITE_VREG(HEVC_DECODE_SIZE, 0xffffffff);
		config_cuva_buf(dec);
#endif
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

static void release_free_mmu_buffers(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int ii;
	for (ii = 0; ii < avs3_dec->max_pb_size; ii++) {
		struct avs3_frame_s *pic = &avs3_dec->pic_pool[ii].buf_cfg;
		if (pic->used == 0 &&
			pic->vf_ref == 0 &&
#ifdef NEW_FRONT_BACK_CODE
			pic->backend_ref == 0 &&
#endif
			pic->mmu_alloc_flag) {
			struct aml_buf *aml_buf = index_to_aml_buf(dec, pic->index);
			pic->mmu_alloc_flag = 0;
			decoder_mmu_box_free_idx(aml_buf->fbc->mmu, aml_buf->fbc->index);
			avs3_print(dec, AVS3_DBG_BUFMGR_MORE, "%s decoder_mmu_box_free_idx index=%d\n", __func__, aml_buf->fbc->index);
			if (dec->front_back_mode)
				decoder_mmu_box_free_idx(aml_buf->fbc->mmu_1, aml_buf->fbc->index);
#ifdef AVS3_10B_MMU_DW
			if (dec->dw_mmu_enable && aml_buf->fbc->mmu_dw) {
				decoder_mmu_box_free_idx(aml_buf->fbc->mmu_dw, aml_buf->fbc->index);
				avs3_print(dec, AVS3_DBG_BUFMGR_MORE, "%s DW decoder_mmu_box_free_idx index=%d\n", __func__, aml_buf->fbc->index);
				if (dec->front_back_mode && aml_buf->fbc->mmu_dw_1)
					decoder_mmu_box_free_idx(aml_buf->fbc->mmu_dw_1, aml_buf->fbc->index);
			}
#endif
#ifndef MV_USE_FIXED_BUF
			decoder_bmmu_box_free_idx(dec->bmmu_box, MV_BUFFER_IDX(pic->index));
			pic->mpred_mv_wr_start_addr = 0;
#endif
		}
	}
}

static void print_hevc_b_data_path_monitor(int frame_count)
{
	uint32_t total_clk_count;
	uint32_t path_transfer_count;
	uint32_t path_wait_count;
	uint32_t path_status;
	unsigned path_wait_ratio;

	printk("\n[WAITING DATA/CMD] Parser/IQIT/IPP/DBLK/OW/DDR/MPRED_IPP_CMD/IPP_DBLK_CMD\n");

	//---------------------- CORE 0 -------------------------------
	WRITE_VREG(HEVC_PATH_MONITOR_CTRL, 0); // Disable monitor and set rd_idx to 0
	total_clk_count = READ_VREG(HEVC_PATH_MONITOR_DATA);

	WRITE_VREG(HEVC_PATH_MONITOR_CTRL, (1 << 4)); // Disable monitor and set rd_idx to 1

	// parser --> iqit
	path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 / path_transfer_count;
	printk("[P%d HEVC CORE0 PATH] WAITING Ratio : %d", frame_count, path_wait_ratio);

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
	else path_wait_ratio = path_wait_count * 100 /path_transfer_count;
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
	printk("\n");

	//---------------------- CORE 1 -------------------------------
	WRITE_VREG(HEVC_PATH_MONITOR_CTRL_DBE1, 0); // Disable monitor and set rd_idx to 0
	total_clk_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);

	WRITE_VREG(HEVC_PATH_MONITOR_CTRL_DBE1, (1<<4)); // Disable monitor and set rd_idx to 1

	// parser --> iqit
	path_transfer_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	path_wait_count = READ_VREG(HEVC_PATH_MONITOR_DATA_DBE1);
	if (path_transfer_count == 0) path_wait_ratio = 0;
	else path_wait_ratio = path_wait_count * 100 /path_transfer_count;
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
	else path_wait_ratio = path_wait_count * 100 /path_transfer_count;
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
	printk("\n");
}

static int BackEnd_StartDecoding(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct avs3_frame_s *pic = avs3_dec->next_be_decode_pic[avs3_dec->fb_rd_pos];
	struct aml_buf *aml_buf = index_to_aml_buf(dec, pic->index);
	int i = 0;
	struct avs3_frame_s *ref_pic = NULL;


	avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
		"Start BackEnd Decoding %d (wr pos %d, rd pos %d) pic index %d\n",
		avs3_dec->backend_decoded_count, avs3_dec->fb_wr_pos, avs3_dec->fb_rd_pos, pic->index);

	mutex_lock(&dec->fb_mutex);
	for (i = 0; (i < pic->list0_num_refp) && (pic->error_mark == 0); i++) {
		ref_pic = &avs3_dec->pic_pool[pic->list0_index[i]].buf_cfg;
		if (ref_pic->error_mark) {
			dec->gvs->error_frame_count++;
			if (pic->slice_type == SLICE_I) {
				dec->gvs->i_concealed_frames++;
			} else if (pic->slice_type == SLICE_P) {
				dec->gvs->p_concealed_frames++;
			} else if (pic->slice_type == SLICE_B) {
				dec->gvs->b_concealed_frames++;
			}
			pic->error_mark = 1;
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "%s:L0 ref_pic %d pic error\n",
				__func__, pic->list0_index[i]);
		}
	}

	for (i = 0; (i < pic->list1_num_refp) && (pic->error_mark == 0); i++) {
		ref_pic = &avs3_dec->pic_pool[pic->list1_index[i]].buf_cfg;
		if (ref_pic->error_mark) {
			dec->gvs->error_frame_count++;
			if (pic->slice_type == SLICE_I) {
				dec->gvs->i_concealed_frames++;
			} else if (pic->slice_type == SLICE_P) {
				dec->gvs->p_concealed_frames++;
			} else if (pic->slice_type == SLICE_B) {
				dec->gvs->b_concealed_frames++;
			}
			pic->error_mark = 1;
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "%s:L1 ref_pic %d pic error\n",
				__func__, pic->list1_index[i]);
		}
	}
	mutex_unlock(&dec->fb_mutex);

	if (pic->error_mark && (error_handle_policy & 0x4)) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"%s: error pic, skip\n", __func__);

		mutex_lock(&dec->fb_mutex);
		dec->gvs->drop_frame_count++;
		if (pic->slice_type == SLICE_I) {
			dec->gvs->i_lost_frames++;
		} else if (pic->slice_type == SLICE_P) {
			dec->gvs->p_lost_frames++;
		} else if (pic->slice_type == SLICE_B) {
			dec->gvs->b_lost_frames++;
		}
		mutex_unlock(&dec->fb_mutex);

		pic_backend_ref_operation(dec, pic, 0);

		return 1;
	}

	decoder_mmu_box_alloc_idx(aml_buf->fbc->mmu, aml_buf->fbc->index, aml_buf->fbc->frame_size, dec->frame_mmu_map_addr);
	decoder_mmu_box_alloc_idx(aml_buf->fbc->mmu_1, aml_buf->fbc->index, aml_buf->fbc->frame_size, dec->frame_mmu_map_addr_1);

	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s decoder_mmu_box_alloc_idx index=%d mmu_4k_number %d\n",
		__func__, aml_buf->fbc->index, aml_buf->fbc->frame_size);

	if (dec->dw_mmu_enable) {
		decoder_mmu_box_alloc_idx(aml_buf->fbc->mmu_dw, aml_buf->fbc->index, aml_buf->fbc->frame_size, dec->dw_frame_mmu_map_addr);
		decoder_mmu_box_alloc_idx(aml_buf->fbc->mmu_dw_1, aml_buf->fbc->index, aml_buf->fbc->frame_size, dec->dw_frame_mmu_map_addr_1);

		avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
			"%s DW decoder_mmu_box_alloc_idx index=%d mmu_4k_number %d\n",
			__func__, aml_buf->fbc->index, aml_buf->fbc->frame_size);
	}
	pic->mmu_alloc_flag = 1;

	copy_loopbufs_ptr(&avs3_dec->bk, &avs3_dec->next_bk[avs3_dec->fb_rd_pos]);
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"update loopbuf bk from next_bk[fb_rd_pos=%d]\n", avs3_dec->fb_rd_pos);
	print_loopbufs_ptr(dec, "bk", &avs3_dec->bk);
#ifdef NEW_FRONT_BACK_CODE
#ifdef PRINT_HEVC_DATA_PATH_MONITOR
	if (dec->front_back_mode == 1) {
		if (avs3_dec->backend_decoded_count > 0 && (debug & AVS3_DBG_CACHE)) {
			print_hevc_b_data_path_monitor(avs3_dec->backend_decoded_count-1);
			print_mcrcc_hit_info(avs3_dec->backend_decoded_count-1);
		}
	}
#endif
#endif
	if (dec->front_back_mode == 1)
		amhevc_reset_b();
	avs3_hw_init(dec, 0, 1);
	if (dec->front_back_mode == 3) {
		WRITE_VREG(dec->backend_ASSIST_MBOX0_IRQ_REG, 1);
	} else {
		config_bufstate_back_hw(avs3_dec);
		WRITE_VREG(PIC_DECODE_COUNT_DBE, avs3_dec->backend_decoded_count);
		WRITE_VREG(HEVC_DEC_STATUS_DBE, HEVC_BE_DECODE_DATA);
		WRITE_VREG(HEVC_SAO_CRC, 0);
		amhevc_start_b();
		vdec_profile(hw_to_vdec(dec), VDEC_PROFILE_DECODER_START, CORE_MASK_HEVC_BACK);
	}

	return 0;
}

static unsigned HEVC_MPRED_L0_REF_POC_ADR[] = {
	HEVC_MPRED_L0_REF00_POC,
	HEVC_MPRED_L0_REF01_POC,
	HEVC_MPRED_L0_REF02_POC,
	HEVC_MPRED_L0_REF03_POC,
	HEVC_MPRED_L0_REF04_POC,
	HEVC_MPRED_L0_REF05_POC,
	HEVC_MPRED_L0_REF06_POC,
	HEVC_MPRED_L0_REF07_POC,
	HEVC_MPRED_L0_REF08_POC,
	HEVC_MPRED_L0_REF09_POC,
	HEVC_MPRED_L0_REF10_POC,
	HEVC_MPRED_L0_REF11_POC,
	HEVC_MPRED_L0_REF12_POC,
	HEVC_MPRED_L0_REF13_POC,
	HEVC_MPRED_L0_REF14_POC,
	HEVC_MPRED_L0_REF15_POC,
	HEVC_MPRED_POC24_CTRL0
};

static unsigned HEVC_MPRED_L1_REF_POC_ADR[] = {
	HEVC_MPRED_L1_REF00_POC,
	HEVC_MPRED_L1_REF01_POC,
	HEVC_MPRED_L1_REF02_POC,
	HEVC_MPRED_L1_REF03_POC,
	HEVC_MPRED_L1_REF04_POC,
	HEVC_MPRED_L1_REF05_POC,
	HEVC_MPRED_L1_REF06_POC,
	HEVC_MPRED_L1_REF07_POC,
	HEVC_MPRED_L1_REF08_POC,
	HEVC_MPRED_L1_REF09_POC,
	HEVC_MPRED_L1_REF10_POC,
	HEVC_MPRED_L1_REF11_POC,
	HEVC_MPRED_L1_REF12_POC,
	HEVC_MPRED_L1_REF13_POC,
	HEVC_MPRED_L1_REF14_POC,
	HEVC_MPRED_L1_REF15_POC,
	HEVC_MPRED_POC24_CTRL1
};

static void init_pic_list_hw_fb(struct AVS3Decoder_s *dec)
{
	int i;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct avs3_frame_s *pic;
	/*WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x0);*/
	avs3_dec->ins_offset = 0;
#if 1
	WRITE_BACK_8(avs3_dec, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, (0x1 << 1) | (0x1 << 2));

#ifdef DUAL_CORE_64
	WRITE_BACK_8(avs3_dec, HEVC2_HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, (0x1 << 1) | (0x1 << 2));
#endif
#endif
	for (i = 0; i < dec->avs3_dec.max_pb_size; i++) {
		pic = &avs3_dec->pic_pool[i].buf_cfg;
		if (pic->index < 0)
			break;
#ifdef AVS3_10B_MMU
		WRITE_BACK_32(avs3_dec, HEVCD_MPP_ANC2AXI_TBL_DATA, pic->header_adr >> 5);
#else
		WRITE_BACK_32(avs3_dec, HEVCD_MPP_ANC2AXI_TBL_DATA, pic->mc_y_adr >> 5);
#endif
#ifndef LOSLESS_COMPRESS_MODE
		WRITE_BACK_32(avs3_dec, HEVCD_MPP_ANC2AXI_TBL_DATA, pic->mc_u_v_adr >> 5);
#endif
	}
	WRITE_BACK_8(avs3_dec, HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x1);

	/*Zero out canvas registers in IPP -- avoid simulation X*/
	WRITE_BACK_8(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR,
	(0 << 8) | (0 << 1) | 1);
	for (i = 0; i < 32; i++) {
		WRITE_BACK_8(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
	}
}

static void config_mpred_hw_fb(struct AVS3Decoder_s *dec)
{
	int32_t i;
	uint32_t data32;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	avs3_frame_t *cur_pic = avs3_dec->cur_pic;
	avs3_frame_t *col_pic;
	int32_t     above_ptr_ctrl =0;
	int32_t     mpred_mv_rd_start_addr ;
	//int32_t     mpred_curr_lcu_x;
	//int32_t     mpred_curr_lcu_y;
	int32_t     mpred_mv_rd_end_addr;
	int32_t     MV_MEM_UNIT_l;
	int32_t     above_en;
	int32_t     mv_wr_en;
	int32_t     mv_rd_en;
	int32_t     col_isIntra;
	int32_t     col_ptr;

	if (avs3_dec->slice_type == SLICE_P) {
		if (avs3_dec->ctx.refp[0][REFP_0].pic != NULL)
			col_pic = &avs3_dec->ctx.refp[0][REFP_0].pic->buf_cfg;
		else
			col_pic = cur_pic;
	} else if (avs3_dec->slice_type == SLICE_B) {
		if (avs3_dec->ctx.refp[0][REFP_1].pic != NULL)
			col_pic = &avs3_dec->ctx.refp[0][REFP_1].pic->buf_cfg;
		else
			col_pic = cur_pic;
	} else {
		col_pic = cur_pic;
	}

	if (avs3_dec->slice_type != SLICE_I) {
		above_en=1;
		mv_wr_en=1;
		if (col_pic->slice_type != SLICE_I)
			mv_rd_en=1;
		else
			mv_rd_en=0;
		col_isIntra=0;
	} else {
		above_en=1;
		mv_wr_en=1;
		mv_rd_en=0;
		col_isIntra=0;
	}

	mpred_mv_rd_start_addr = col_pic->mpred_mv_wr_start_addr;
	/*data32 = READ_VREG(HEVC_MPRED_CURR_LCU);
	mpred_curr_lcu_x = data32 & 0xffff;
	mpred_curr_lcu_y = (data32 >> 16) & 0xffff;*/

	MV_MEM_UNIT_l = get_mv_mem_unit(avs3_dec->lcu_size_log2);
	mpred_mv_rd_end_addr = mpred_mv_rd_start_addr + ((avs3_dec->lcu_x_num * avs3_dec->lcu_y_num) * MV_MEM_UNIT_l);

	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"cur pic index %d  slicetype %d col pic index %d slicetype %d\n",
		cur_pic->index, cur_pic->slice_type,
		col_pic->index, col_pic->slice_type);

	WRITE_VREG(HEVC_MPRED_MV_WR_START_ADDR, cur_pic->mpred_mv_wr_start_addr);
	WRITE_VREG(HEVC_MPRED_MV_RD_START_ADDR, col_pic->mpred_mv_wr_start_addr);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[MPRED CO_MV] write 0x%x  read 0x%x\n", cur_pic->mpred_mv_wr_start_addr, col_pic->mpred_mv_wr_start_addr);
#if 1
	data32 = READ_VREG(HEVC_MPRED_CTRL0);
	data32 &= (~(0x3 | (0xf << 8) | (0xf << 16)));
	data32 = (avs3_dec->slice_type |
		above_ptr_ctrl << 8 |
		above_en << 9 |
		mv_wr_en << 10 |
		mv_rd_en << 11 |
		avs3_dec->lcu_size_log2 << 16
		/*|cu_size_log2<<20*/
		);
	WRITE_VREG(HEVC_MPRED_CTRL0, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "HEVC_MPRED_CTRL0=0x%x\n", READ_VREG(HEVC_MPRED_CTRL0));

	data32 = READ_VREG(HEVC_MPRED_CTRL1);
	data32 &= (~0xf);
	data32 |= avs3_dec->ctx.info.sqh.num_of_hmvp_cand;
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"write HEVC_MPRED_CTRL1=0x%x, avs3_dec->ctx.info.sqh.num_of_hmvp_cand=%d\n",
		data32, avs3_dec->ctx.info.sqh.num_of_hmvp_cand);
	WRITE_VREG(HEVC_MPRED_CTRL1, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "HEVC_MPRED_CTRL1=0x%x\n", READ_VREG(HEVC_MPRED_CTRL1));

	data32 = (avs3_dec->img.width | avs3_dec->img.height << 16);
	WRITE_VREG(HEVC_MPRED_PIC_SIZE, data32);

	data32 = ((avs3_dec->lcu_x_num-1) | (avs3_dec->lcu_y_num - 1) << 16);
	WRITE_VREG(HEVC_MPRED_PIC_SIZE_LCU,data32);

	data32 = (avs3_dec->ctx.dpm.num_refp[REFP_0] | avs3_dec->ctx.dpm.num_refp[REFP_1] << 8 | 0);
	WRITE_VREG(HEVC_MPRED_REF_NUM,data32);

	data32 = 0;
	for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_0]; i++)
		data32 = data32 | (1 << i);
	WRITE_VREG(HEVC_MPRED_REF_EN_L0, data32);

	data32 = 0;
	for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_1]; i++)
		data32 = data32 | (1 << i);
	WRITE_VREG(HEVC_MPRED_REF_EN_L1,data32);
#endif
	WRITE_VREG(HEVC_MPRED_CUR_POC, avs3_dec->ctx.ptr & 0xffff);
	if (avs3_dec->slice_type == SLICE_P) {
		col_ptr = avs3_dec->ctx.refp[0][REFP_0].ptr;
	} else if (avs3_dec->slice_type == SLICE_B) {
		col_ptr = avs3_dec->ctx.refp[0][REFP_1].ptr;
	} else {
		col_ptr = avs3_dec->ctx.pic->ptr;
	}
	WRITE_VREG(HEVC_MPRED_COL_POC, col_ptr & 0xffff);
	//below MPRED Ref_POC_xx_Lx registers must follow Ref_POC_xx_L0 -> Ref_POC_xx_L1 in pair write order!!!
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"HEVC_MPRED_CUR_POC=0x%x, HEVC_MPRED_COL_POC=0x%x\n",
		READ_VREG(HEVC_MPRED_CUR_POC), READ_VREG(HEVC_MPRED_COL_POC));

	for (i = 0; i < MAX_NUM_REF_PICS; i++) {
		data32 = 0;
		if (i < cur_pic->list0_num_refp || i < col_pic->list0_num_refp) {
			if (i < cur_pic->list0_num_refp) {
				data32 |= cur_pic->list0_ptr[i] & 0xffff;
			}
			if (i < col_pic->list0_num_refp) {
				data32 |= ((col_pic->list0_ptr[i] & 0xffff) << 16);
			}
		}
		WRITE_VREG(HEVC_MPRED_L0_REF_POC_ADR[i], data32);
	}
	if (debug & AVS3_DBG_BUFMGR_DETAIL) {
		for (i = 0; i < MAX_NUM_REF_PICS; i++) {
			if (i < cur_pic->list0_num_refp || i < col_pic->list0_num_refp) {
				if (i < cur_pic->list0_num_refp) {
					avs3_print_cont(dec, AVS3_DBG_BUFMGR_DETAIL,
					"[%d] ", cur_pic->list0_ptr[i]);
				}
				if (i < col_pic->list0_num_refp) {
					avs3_print_cont(dec, AVS3_DBG_BUFMGR_DETAIL,
					"<%d> ", col_pic->list0_ptr[i]);
				}
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
					"HEVC_MPRED_L0_REF=0x%x (readback 0x%x)\n", i,
					data32, READ_VREG(HEVC_MPRED_L0_REF_POC_ADR[i]));
			}
		}
	}
	for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_1]; i++) {
		WRITE_VREG(HEVC_MPRED_L1_REF_POC_ADR[i], avs3_dec->ctx.refp[i][REFP_1].ptr & 0xffff);
	}
	if (debug) {
		for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_1]; i++) {
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"HEVC_MPRED_L1_REF%02d_POC=0x%x (readback 0x%x)\n", i, avs3_dec->ctx.refp[i][REFP_1].ptr & 0xffff, READ_VREG(HEVC_MPRED_L1_REF_POC_ADR[i]));
		}
	}
	WRITE_VREG(HEVC_MPRED_MV_RD_END_ADDR,mpred_mv_rd_end_addr);
}

static void config_dw_fb(struct AVS3Decoder_s *dec, struct avs3_frame_s *pic)
{

	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int dw_mode = get_double_write_mode(dec);
	uint32_t data = 0, data32;
	struct aml_vcodec_ctx * v4l2_ctx = dec->v4l2_ctx;
	if ((dw_mode & 0x10) == 0) {
		WRITE_BACK_8(avs3_dec, HEVC_SAO_CTRL26, 0);

		if (((dw_mode & 0xf) == 8) ||
			((dw_mode & 0xf) == 9)) {
			data = 0xff; //data32 |= (0xff << 16);
			WRITE_BACK_8(avs3_dec, HEVC_SAO_CTRL26, 0xf);
		} else {
			if ((dw_mode & 0xf) == 2 ||
				(dw_mode & 0xf) == 3)
				data = 0xff; //data32 |= (0xff << 16);
			else if ((dw_mode & 0xf) == 4 ||
				(dw_mode & 0xf) == 5)
				data = 0x33; //data32 |= (0x33 << 16);

			READ_WRITE_DATA16(avs3_dec, HEVC_SAO_CTRL5, 0, 9, 1); //data32 &= ~(1 << 9);
		}
		READ_WRITE_DATA16(avs3_dec, HEVC_SAO_CTRL5, data, 16, 8);
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
	WRITE_BACK_32(avs3_dec, HEVC_SAO_CTRL1, data32);

	if (dw_mode == 0)
		data = 1; //data32 |= (0x1 << 8); /*enable first write*/
	else if (dw_mode == 0x10)
		data = 2; //data32 |= (0x1 << 9); /*double write only*/
	else
		data = 3; //data32 |= ((0x1 << 8) | (0x1 << 9));
	READ_WRITE_DATA16(avs3_dec, HEVC_DBLK_CFGB, data, 8, 2);

	if (dw_mode & 0x10) {
		/* [23:22] dw_v1_ctrl
		*[21:20] dw_v0_ctrl
		*[19:18] dw_h1_ctrl
		*[17:16] dw_h0_ctrl
		*/
		READ_WRITE_DATA16(avs3_dec, HEVC_SAO_CTRL5, 0, 16, 8);
	}

#ifdef LOSLESS_COMPRESS_MODE
	/*SUPPORT_10BIT*/

	data32 = pic->mc_y_adr;
	if (dw_mode && ((dw_mode & 0x20) == 0)) {
		WRITE_BACK_32(avs3_dec, HEVC_SAO_Y_START_ADDR, pic->dw_y_adr);
		WRITE_BACK_32(avs3_dec, HEVC_SAO_C_START_ADDR, pic->dw_u_v_adr);
		WRITE_BACK_32(avs3_dec, HEVC_SAO_Y_WPTR, pic->dw_y_adr);
		WRITE_BACK_32(avs3_dec, HEVC_SAO_C_WPTR, pic->dw_u_v_adr);
	} else {
		WRITE_BACK_32(avs3_dec, HEVC_SAO_Y_START_ADDR, 0xffffffff);
		WRITE_BACK_32(avs3_dec, HEVC_SAO_C_START_ADDR, 0xffffffff);
	}
	if ((dw_mode & 0x10) == 0)
		WRITE_BACK_32(avs3_dec, HEVC_CM_BODY_START_ADDR, data32);

	if (dec->mmu_enable)
		WRITE_BACK_32(avs3_dec, HEVC_CM_HEADER_START_ADDR, pic->header_adr);
#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		WRITE_BACK_32(avs3_dec, HEVC_SAO_Y_START_ADDR, 0);
		WRITE_BACK_32(avs3_dec, HEVC_SAO_C_START_ADDR, 0);
		WRITE_BACK_32(avs3_dec, HEVC_CM_HEADER_START_ADDR2, pic->dw_header_adr);
	}
#endif
#else
	WRITE_BACK_32(avs3_dec, HEVC_SAO_Y_START_ADDR, pic->mc_y_adr);
	WRITE_BACK_32(avs3_dec, HEVC_SAO_C_START_ADDR, pic->mc_u_v_adr);
	WRITE_BACK_32(avs3_dec, HEVC_SAO_Y_WPTR, pic->mc_y_adr);
	WRITE_BACK_32(avs3_dec, HEVC_SAO_C_WPTR, pic->mc_u_v_adr);
#endif
	WRITE_BACK_32(avs3_dec, HEVC_SAO_Y_LENGTH, pic->luma_size);

	WRITE_BACK_32(avs3_dec, HEVC_SAO_C_LENGTH, pic->chroma_size);
}

static void config_sao_hw_fb(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	avs3_frame_t *pic = avs3_dec->cur_pic;

	config_dw_fb(dec, pic);
	READ_WRITE_DATA16(avs3_dec, HEVC_SAO_CTRL0, avs3_dec->lcu_size_log2, 0, 4);

#ifdef AVS3_10B_NV21
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

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[c] cfgSAO .done.\n");
}

static void config_dblk_hw_fb(struct AVS3Decoder_s *dec)
{
	/*
	* Picture level de-block parameter configuration here
	*/
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	union param_u *rpm_param = &avs3_dec->param;
	uint32_t data32;
	DEC_CTX * ctx = &avs3_dec->ctx;
	int32_t alpha_c_offset = rpm_param->p.pic_header_alpha_c_offset;
	int32_t beta_offset = rpm_param->p.pic_header_beta_offset;

	alpha_c_offset = (alpha_c_offset >= 16) ? 15 : ((alpha_c_offset < -16) ? -16 : alpha_c_offset);
	beta_offset = (beta_offset >= 16) ? 15 : ((beta_offset < -16) ? -16 : beta_offset);

	avs3_dec->instruction[avs3_dec->ins_offset] = 0x6450101;                                          //mfsp COMMON_REG_1, HEVC_DBLK_CFG1
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x1a<<22) | ((((ctx->info.bit_depth_internal == 10) ? 0xa:0x0)&0xffff)<<6);   // movi COMMON_REG_0, data
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = 0x9604040;                                          //ins  COMMON_REG_1, COMMON_REG_0, 16, 4
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = (0x1a<<22) | ((((ctx->info.log2_max_cuwh == 6) ? 0:(ctx->info.log2_max_cuwh == 5) ? 1:(ctx->info.log2_max_cuwh == 4) ? 2:3)&0xffff)<<6);   // movi COMMON_REG_0, data
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = 0x9402040;                                          //ins  COMMON_REG_1, COMMON_REG_0, 0, 2
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;
	avs3_dec->instruction[avs3_dec->ins_offset] = 0x6050101;                                          //mtsp COMMON_REG_1, HEVC_DBLK_CFG1
	avs3_print(dec, AVS3_DBG_REG,
		"instruction[%3d] = %8x\n", avs3_dec->ins_offset, avs3_dec->instruction[avs3_dec->ins_offset]);
	avs3_dec->ins_offset++;

	data32 = (avs3_dec->img.height << 16) | avs3_dec->img.width;
	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFG2, data32);

	data32 = ((rpm_param->p.sqh_cross_patch_loop_filter & 0x1) << 27) |// [27 +: 1]: cross_slice_loopfilter_enable_flag
		((rpm_param->p.pic_header_loop_filter_disable_flag & 0x1) << 26) |               // [26 +: 1]: loop_filter_disable
		((alpha_c_offset & 0x1f) << 17) |                    // [17 +: 5]: alpha_c_offset (-8~8)
		((beta_offset & 0x1f) << 12) |                       // [12 +: 5]: beta_offset (-8~8)
		((rpm_param->p.pic_header_chroma_quant_param_delta_cb & 0x3f) << 6) |         // [ 6 +: 6]: chroma_quant_param_delta_u (-16~16)
		((rpm_param->p.pic_header_chroma_quant_param_delta_cr & 0x3f) << 0);          // [ 0 +: 6]: chroma_quant_param_delta_v (-16~16)
	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFG9, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: crossslice(%d),lfdisable(%d),bitDepth(%d,%d),log2_lcuSize(%d)\n",
		rpm_param->p.sqh_cross_patch_loop_filter,rpm_param->p.pic_header_loop_filter_disable_flag,
		ctx->info.bit_depth_input,ctx->info.bit_depth_internal,ctx->info.log2_max_cuwh);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: alphaCOffset(%d crop to %d),betaOffset(%d crop to %d),quantDeltaCb(%d),quantDeltaCr(%d)\n",
		rpm_param->p.pic_header_alpha_c_offset, alpha_c_offset, rpm_param->p.pic_header_beta_offset, beta_offset,
		rpm_param->p.pic_header_chroma_quant_param_delta_cb,rpm_param->p.pic_header_chroma_quant_param_delta_cr);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: .done.\n");
}

static void reconstructCoefficients(struct AVS3Decoder_s *dec, ALFParam *alfParam)
{
	int32_t g, sum, i, coeffPred;
	for (g = 0; g < alfParam->filters_per_group; g++) {
		sum = 0;
		for (i = 0; i < alfParam->num_coeff - 1; i++) {
			sum += (2 * alfParam->coeff_multi[g][i]);
			dec->m_filterCoeffSym[g][i] = alfParam->coeff_multi[g][i];
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"[t] dec->m_filterCoeffSym[%d][%d]=0x%x\n", g,i,dec->m_filterCoeffSym[g][i]);
		}
		coeffPred = (1 << ALF_NUM_BIT_SHIFT) - sum;
		dec->m_filterCoeffSym[g][alfParam->num_coeff - 1] = coeffPred + alfParam->coeff_multi[g][alfParam->num_coeff - 1];
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"[t] dec->m_filterCoeffSym[%d][%d]=0x%x\n", g,(alfParam->num_coeff - 1), dec->m_filterCoeffSym[g][alfParam->num_coeff - 1]);
	}
}

static void reconstructCoefInfo(struct AVS3Decoder_s *dec, int32_t compIdx, ALFParam *alfParam)
{
	int32_t i;
	if (compIdx == ALF_Y) {
		if (alfParam->filters_per_group > 1) {
			for (i = 1; i < NO_VAR_BINS; ++i) {
				if (alfParam->filter_pattern[i]) {
					dec->m_varIndTab[i] = dec->m_varIndTab[i - 1] + 1;
				} else {
					dec->m_varIndTab[i] = dec->m_varIndTab[i - 1];
				}
			}
		}
	}
	reconstructCoefficients(dec, alfParam);
}

static void config_alf_hw_fb(struct AVS3Decoder_s *dec)
{
	/*
	* Picture level ALF parameter configuration here
	*/
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	uint32_t data32;
	int32_t i,j;
	int32_t m_filters_per_group;
#ifdef USE_FORCED_ALF_PARAM
	ALFParam forced_alf_cr;
	forced_alf_cr.alf_flag = 1;
	forced_alf_cr.num_coeff = 9;
	forced_alf_cr.filters_per_group = 1;
	forced_alf_cr.component_id = 2;
	forced_alf_cr.coeff_multi[0][0] = -3;
	forced_alf_cr.coeff_multi[0][1] = -3;
	forced_alf_cr.coeff_multi[0][2] = 4;
	forced_alf_cr.coeff_multi[0][3] = 7;
	forced_alf_cr.coeff_multi[0][4] = 6;
	forced_alf_cr.coeff_multi[0][5] = -1;
	forced_alf_cr.coeff_multi[0][6] = 3;
	forced_alf_cr.coeff_multi[0][7] = 6;
	forced_alf_cr.coeff_multi[0][8] = 0;
#endif
	ALFParam *m_alfPictureParam_y = &avs3_dec->m_alfPictureParam[0];
	ALFParam *m_alfPictureParam_cb = &avs3_dec->m_alfPictureParam[1];
#ifdef USE_FORCED_ALF_PARAM
	ALFParam *m_alfPictureParam_cr = &forced_alf_cr; // &avs3_dec->m_alfPictureParam[2];
#else
	ALFParam *m_alfPictureParam_cr = &avs3_dec->m_alfPictureParam[2];
#endif

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[t]alfy,cidx(%d),flag(%d),filters_per_group(%d),filter_pattern[0]=0x%x,[15]=0x%x\n",
		m_alfPictureParam_y->component_id,
		m_alfPictureParam_y->alf_flag,
		m_alfPictureParam_y->filters_per_group,
		m_alfPictureParam_y->filter_pattern[0],m_alfPictureParam_y->filter_pattern[15]);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[t]alfy,num_coeff(%d),coeff_multi[0][0]=0x%x,[0][1]=0x%x,[1][0]=0x%x,[1][1]=0x%x\n",
		m_alfPictureParam_y->num_coeff,
		m_alfPictureParam_y->coeff_multi[0][0],
		m_alfPictureParam_y->coeff_multi[0][1],
		m_alfPictureParam_y->coeff_multi[1][0],
		m_alfPictureParam_y->coeff_multi[1][1]);

	// Cr
	for (i = 0; i < 16; i++) dec->m_varIndTab[i] = 0;
	for (j = 0; j < 16; j++) for (i = 0; i < 9; i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 2, m_alfPictureParam_cr);
	data32 = ((dec->m_filterCoeffSym[0][4] & 0xf ) << 28) |
		((dec->m_filterCoeffSym[0][3] & 0x7f) << 21) |
		((dec->m_filterCoeffSym[0][2] & 0x7f) << 14) |
		((dec->m_filterCoeffSym[0][1] & 0x7f) <<  7) |
		((dec->m_filterCoeffSym[0][0] & 0x7f) <<  0);
	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
	data32 = ((dec->m_filterCoeffSym[0][8] & 0x7f) << 24) |
		((dec->m_filterCoeffSym[0][7] & 0x7f) << 17) |
		((dec->m_filterCoeffSym[0][6] & 0x7f) << 10) |
		((dec->m_filterCoeffSym[0][5] & 0x7f) <<  3) |
		(((dec->m_filterCoeffSym[0][4] >> 4) & 0x7 ) <<  0);

	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_cr(%d), alf_cr_coef(%d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_cr->alf_flag,
		dec->m_filterCoeffSym[0][0],dec->m_filterCoeffSym[0][1],dec->m_filterCoeffSym[0][2],
		dec->m_filterCoeffSym[0][3],dec->m_filterCoeffSym[0][4],dec->m_filterCoeffSym[0][5],
		dec->m_filterCoeffSym[0][6],dec->m_filterCoeffSym[0][7],dec->m_filterCoeffSym[0][8]);

	// Cb
	for (j = 0; j < 16; j++) for (i = 0; i < 9;i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 1, m_alfPictureParam_cb);
	data32 = ((dec->m_filterCoeffSym[0][4] & 0xf ) << 28) |
		((dec->m_filterCoeffSym[0][3] & 0x7f) << 21) |
		((dec->m_filterCoeffSym[0][2] & 0x7f) << 14) |
		((dec->m_filterCoeffSym[0][1] & 0x7f) <<  7) |
		((dec->m_filterCoeffSym[0][0] & 0x7f) <<  0);

	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
		data32 = ((dec->m_filterCoeffSym[0][8] & 0x7f) << 24) |
		((dec->m_filterCoeffSym[0][7] & 0x7f) << 17) |
		((dec->m_filterCoeffSym[0][6] & 0x7f) << 10) |
		((dec->m_filterCoeffSym[0][5] & 0x7f) <<  3) |
		(((dec->m_filterCoeffSym[0][4] >> 4) & 0x7 ) <<  0);

	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_cb(%d), alf_cb_coef(%d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_cb->alf_flag,
		dec->m_filterCoeffSym[0][0],dec->m_filterCoeffSym[0][1],dec->m_filterCoeffSym[0][2],
		dec->m_filterCoeffSym[0][3],dec->m_filterCoeffSym[0][4],dec->m_filterCoeffSym[0][5],
		dec->m_filterCoeffSym[0][6],dec->m_filterCoeffSym[0][7],dec->m_filterCoeffSym[0][8]);

	// Y
	for (j = 0;j < 16; j++) for (i = 0; i < 9; i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 0, m_alfPictureParam_y);
	data32 = ((dec->m_varIndTab[7] & 0xf) << 28) | ((dec->m_varIndTab[6] & 0xf) << 24) |
		((dec->m_varIndTab[5] & 0xf) << 20) | ((dec->m_varIndTab[4] & 0xf) << 16) |
		((dec->m_varIndTab[3] & 0xf) << 12) | ((dec->m_varIndTab[2] & 0xf) <<  8) |
		((dec->m_varIndTab[1] & 0xf) <<  4) | ((dec->m_varIndTab[0] & 0xf) <<  0);

	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
	data32 = ((dec->m_varIndTab[15] & 0xf) << 28) | ((dec->m_varIndTab[14] & 0xf) << 24) |
		((dec->m_varIndTab[13] & 0xf) << 20) | ((dec->m_varIndTab[12] & 0xf) << 16) |
		((dec->m_varIndTab[11] & 0xf) << 12) | ((dec->m_varIndTab[10] & 0xf) <<  8) |
		((dec->m_varIndTab[ 9] & 0xf) <<  4) | ((dec->m_varIndTab[ 8] & 0xf) <<  0);

	WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_y(%d), alf_y_tab(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_y->alf_flag,
		dec->m_varIndTab[ 0],dec->m_varIndTab[ 1],dec->m_varIndTab[ 2],dec->m_varIndTab[ 3],
		dec->m_varIndTab[ 4],dec->m_varIndTab[ 5],dec->m_varIndTab[ 6],dec->m_varIndTab[ 7],
		dec->m_varIndTab[ 8],dec->m_varIndTab[ 9],dec->m_varIndTab[10],dec->m_varIndTab[11],
		dec->m_varIndTab[12],dec->m_varIndTab[13],dec->m_varIndTab[14],dec->m_varIndTab[15]);

	m_filters_per_group = (m_alfPictureParam_y->alf_flag == 0) ? 1 : m_alfPictureParam_y->filters_per_group;
	for (i = 0; i < m_filters_per_group; i++) {
		data32 = ((dec->m_filterCoeffSym[i][4] & 0xf ) << 28) |
			((dec->m_filterCoeffSym[i][3] & 0x7f) << 21) |
			((dec->m_filterCoeffSym[i][2] & 0x7f) << 14) |
			((dec->m_filterCoeffSym[i][1] & 0x7f) <<  7) |
			((dec->m_filterCoeffSym[i][0] & 0x7f) <<  0);

		WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
		data32 = ((i == m_filters_per_group-1) << 31) | // [31] last indication
			((dec->m_filterCoeffSym[i][8] & 0x7f) << 24) |
			((dec->m_filterCoeffSym[i][7] & 0x7f) << 17) |
			((dec->m_filterCoeffSym[i][6] & 0x7f) << 10) |
			((dec->m_filterCoeffSym[i][5] & 0x7f) <<  3) |
			(((dec->m_filterCoeffSym[i][4]>>4) & 0x7) << 0);
		WRITE_BACK_32(avs3_dec, HEVC_DBLK_CFGD, data32);
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"[c] alf_y_coef[%d](%d %d %d %d %d %d %d %d %d)\n",i,dec->m_filterCoeffSym[i][0],dec->m_filterCoeffSym[i][1],dec->m_filterCoeffSym[i][2],
			dec->m_filterCoeffSym[i][3],dec->m_filterCoeffSym[i][4],dec->m_filterCoeffSym[i][5],
			dec->m_filterCoeffSym[i][6],dec->m_filterCoeffSym[i][7],dec->m_filterCoeffSym[i][8]);
	}
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "[c] cfgALF .done.\n");
}

static void  config_mcrcc_axi_hw_fb(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	WRITE_BACK_8(avs3_dec, HEVCD_MCRCC_CTL1, 0x2); // reset mcrcc
	if (avs3_dec->slice_type == SLICE_I) {
		WRITE_BACK_8(avs3_dec, HEVCD_MCRCC_CTL1, 0x0); // remove reset -- disables clock
		return;
	}

	if ((avs3_dec->slice_type == SLICE_B) || (avs3_dec->slice_type == SLICE_P)) {
		WRITE_BACK_8(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0 << 1) | 0);
		READ_INS_WRITE(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL2, 0, 16, 16);

		WRITE_BACK_16(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, 1, (16 << 8) | (1 << 1) | 0);

		READ_CMP_WRITE(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL3, 1, 0, 16, 16);
	} else { // P-PIC
		WRITE_BACK_8(avs3_dec, HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (1 << 1) | 0);
		READ_INS_WRITE(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL2, 0, 16, 16);
		READ_INS_WRITE(avs3_dec, HEVCD_MPP_ANC_CANVAS_DATA_ADDR, HEVCD_MCRCC_CTL3, 0, 16, 16);
	}

	WRITE_BACK_16(avs3_dec, HEVCD_MCRCC_CTL1, 0, 0xff0); // enable mcrcc progressive-mode
	return;
}

#endif
