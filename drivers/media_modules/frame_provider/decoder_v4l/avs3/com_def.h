/* ====================================================================================================================

  The copyright in this software is being made available under the License included below.
  This software may be subject to other third party and contributor rights, including patent rights, and no such
  rights are granted under this license.

  Copyright (c) 2018, HUAWEI TECHNOLOGIES CO., LTD. All rights reserved.
  Copyright (c) 2018, SAMSUNG ELECTRONICS CO., LTD. All rights reserved.
  Copyright (c) 2018, PEKING UNIVERSITY SHENZHEN GRADUATE SCHOOL. All rights reserved.
  Copyright (c) 2018, PENGCHENG LABORATORY. All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted only for
  the purpose of developing standards within Audio and Video Coding Standard Workgroup of China (AVS) and for testing and
  promoting such standards. The following conditions are required to be met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
      the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
      the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name of HUAWEI TECHNOLOGIES CO., LTD. or SAMSUNG ELECTRONICS CO., LTD. may not be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

* ====================================================================================================================
*/

#ifndef _COM_DEF_H_
#define _COM_DEF_H_
#define AML

#include "com_typedef.h"
#include "com_port.h"
#include <linux/amlogic/media/utils/amstream.h>

#if TSCPM
#define MAX_INT                     2147483647  ///< max. value of signed 32-bit integer
#endif

/* profile & level */
#if PHASE_2_PROFILE
#define PROFILE_ID 0x32
#else
#define PROFILE_ID 0x22
#endif
#define LEVEL_ID 0x6A

/* MCABAC (START) */
#define PROB_BITS                         11 // LPS_PROB(10-bit) + MPS(1-bit)
#define PROB_MASK                         ((1 << PROB_BITS) - 1) // mask for LPS_PROB + MPS
#define MAX_PROB                          ((1 << PROB_BITS) - 1) // equal to PROB_LPS + PROB_MPS, 0x7FF
#define HALF_PROB                         (MAX_PROB >> 1)
#define QUAR_HALF_PROB                    (1 << (PROB_BITS - 3))
#define LG_PMPS_SHIFTNO                   2
#if CABAC_MULTI_PROB
#define PROB_INIT                         (HALF_PROB << 1) + (HALF_PROB << PROB_BITS)
#else
#define PROB_INIT                         (HALF_PROB << 1) /* 1/2 of initialization = (HALF_PROB << 1)+ MPS(0) */
#endif
#define READ_BITS_INIT                    16
#define DEC_RANGE_SHIFT                   (READ_BITS_INIT - (PROB_BITS - 2))

#define COM_BSR_IS_BYTE_ALIGN(bs)         !((bs)->leftbits & 0x7)

#if CABAC_MULTI_PROB
#define MAX_WINSIZE                        9
#define MIN_WINSIZE                        2
#define MCABAC_SHIFT_I                     5
#define MCABAC_SHIFT_B                     5
#define MCABAC_SHIFT_P                     5
#define CYCNO_SHIFT_BITS                   21   // PROB_BITS
#define MCABAC_PROB_BITS                   10
#define MCABAC_PROB_MASK                   ((1 << MCABAC_PROB_BITS) - 1)
#define COUNTER_THR_I                      8
#define COUNTER_THR_B                      16
#define COUNTER_THR_P                      16
#endif

/* MCABAC (END) */

/* Multiple Reference (START) */
#define MAX_NUM_ACTIVE_REF_FRAME_B         2  /* Maximum number of active reference frames for RA condition */
#define MAX_NUM_ACTIVE_REF_FRAME_LDB       4  /* Maximum number of active reference frames for LDB condition */
#define MV_SCALE_PREC                      14 /* Scaling precision for motion vector prediction (2^MVP_SCALING_PRECISION) */
/* Multiple Reference (END) */

/* Max. and min. Quantization parameter */
#define MIN_QUANT                          0
#define MAX_QUANT_BASE                     63

/* BIO (START) */
#if BIO
#define BIO_MAX_SIZE                      128
#define BIO_CLUSTER_SIZE                  4
#define BIO_WINDOW_SIZE                   0
#define BIO_AVG_WIN_SIZE                  (BIO_CLUSTER_SIZE + (BIO_WINDOW_SIZE << 1))
#define LIMITBIO                          768
#define DENOMBIO                          2
#endif
/* BIO (END) */

/* IST (START)*/
#if IST
#define IST_MAX_COEF_SIZE                  16
#endif
/* IST (END)*/

/* SBT (START) */
#if SBT
#define get_sbt_idx(s)                     (s & 0xf)
#define get_sbt_pos(s)                     ((s >> 4) & 0xf)
#define get_sbt_info(idx, pos)             (idx + (pos << 4))
#define is_sbt_horizontal(idx)             (idx == 2 || idx == 4)
#define is_sbt_quad_size(idx)              (idx == 3 || idx == 4)
//encoder only
#define SBT_FAST                           1 // early skip fast algorithm
#define SBT_SAVELOAD                       1 // save & load
#endif
/* SBT (END) */

/* AMVR (START) */
#define MAX_NUM_MVR                        5  /* 0 (1/4-pel) ~ 4 (4-pel) */
#if BD_AFFINE_AMVR
#define MAX_NUM_AFFINE_MVR                 3
#endif
#define FAST_MVR_IDX                       2
#define SKIP_MVR_IDX                       1
#if BIO
#define BIO_MAX_MVR                        1
#endif
/* AMVR (END)  */

/* ABVR (START) */
#if IBC_ABVR
#define MAX_NUM_BVR                        2 /* 1/4-pel */
#endif
/* ABVR (END) */

/* DB_AVS2 (START) */
#define EDGE_TYPE_LUMA                     1
#define EDGE_TYPE_ALL                      2
#define LOOPFILTER_DIR_TYPE                2
#define LOOPFILTER_SIZE_IN_BIT             2
#define LOOPFILTER_SIZE                    (1 << LOOPFILTER_SIZE_IN_BIT)
#define LOOPFILTER_GRID                    8
#define DB_CROSS_SLICE                     1
/* DB_AVS2 (END) */

/* SAO_AVS2 (START) */
#define NUM_BO_OFFSET                      32
#define MAX_NUM_SAO_CLASSES                32
#define NUM_SAO_BO_CLASSES_LOG2            5
#define NUM_SAO_BO_CLASSES_IN_BIT          5
#define NUM_SAO_EO_TYPES_LOG2              2
#define NUM_SAO_BO_CLASSES                 (1 << NUM_SAO_BO_CLASSES_LOG2)
#define SAO_SHIFT_PIX_NUM                  4
/* SAO_AVS2 (END) */

/* ESAO_AVS3 (START) */
#if ESAO
#define ESAO_LUMA_TYPES                    2  // 1 : only 17 classes; 2: both 17 and 9 classes are applied
#define NUM_ESAO_LUMA_TYPE0                17   //
#define NUM_ESAO_LUMA_TYPE1                9

//Y
#define ESAO_LABEL_NUM_Y                   16   //
#define ESAO_LABEL_NUM_IN_BIT_Y            4 //bits
#define ESAO_LABEL_CLASSES_Y               272  //16*17
//U
#define ESAO_LABEL_NUM_U                   96
#define ESAO_LABEL_NUM_IN_BIT_U            7
#define ESAO_LABEL_CLASSES_U               272
//V
#define ESAO_LABEL_NUM_V                   96
#define ESAO_LABEL_NUM_IN_BIT_V            7
#define ESAO_LABEL_CLASSES_V               272

//maximum value among  Y, U and V
#define ESAO_LABEL_NUM_MAX                 96
#define ESAO_LABEL_NUM_IN_BIT_MAX          7
#define ESAO_LABEL_CLASSES_MAX             272
#endif
/* ESAO_AVS3 (END) */

#if CCSAO
/* CCSAO_AVS3 (START) */
#if CCSAO_ENHANCEMENT
#define CCSAO_SET_NUM                      4
#define CCSAO_SET_NUM_BIT                  2
#endif

#define CCSAO_TYPE_NUM                     9
#define CCSAO_TYPE_NUM_BIT                 4

#define CCSAO_BAND_NUM                     16
#define CCSAO_BAND_NUM_BIT                 4

#if CCSAO_ENHANCEMENT
#define CCSAO_BAND_NUM_C                   2
#define CCSAO_BAND_NUM_BIT_C               1
#define CCSAO_CLASS_NUM                    (CCSAO_BAND_NUM * CCSAO_BAND_NUM_C)
#else
#define CCSAO_CLASS_NUM                    CCSAO_BAND_NUM
#endif
/* CCSAO_AVS3 (END) */
#endif

/* ALF_AVS2 (START) */
#if ALF_SHAPE
#define ALF_MAX_NUM_COEF_SHAPE2            15
#endif
#define ALF_MAX_NUM_COEF                   9
#if ALF_IMP
#define ITER_NUM                            4
#define INTERVAL_NUM_X 8
#define INTERVAL_NUM_Y 8
#define NO_VAR_BINS                        INTERVAL_NUM_X * INTERVAL_NUM_Y
#define NO_VAR_BINS_16                     16
#else
#define NO_VAR_BINS                        16
#endif
#define ALF_REDESIGN_ITERATION             3
#define LOG2_VAR_SIZE_H                    2
#define LOG2_VAR_SIZE_W                    2
#define ALF_FOOTPRINT_SIZE                 7
#define DF_CHANGED_SIZE                    3
#define ALF_NUM_BIT_SHIFT                  6
/* ALF_AVS2 (END) */

/* DMVR_AVS2 (START) */
#if DMVR
#define DMVR_IMPLIFICATION_SUBCU_SIZE      16
#define DMVR_ITER_COUNT                    2
#define REF_PRED_POINTS_NUM                9
#define REF_PRED_EXTENTION_PEL_COUNT       1
#define REF_PRED_POINTS_PER_LINE_NUM       3
#define REF_PRED_POINTS_LINES_NUM          3
#define DMVR_NEW_VERSION_ITER_COUNT        8
#endif
/* DMVR_AVS2 (END) */

/* INTERPF_AVS2 (START) */
#if INTERPF
#define NUM_RDO_INTER_FILTER               6  //number of additional RDO
#endif
/* INTERPF_AVS2 (END) */

/* IBC (START) */
#if USE_IBC
#define IBC_SEARCH_RANGE                     64
#define IBC_NUM_CANDIDATES                   64
#define IBC_FAST_METHOD_BUFFERBV             0X01
#define IBC_FAST_METHOD_ADAPTIVE_SEARCHRANGE 0X02
#define IBC_BITSTREAM_FLAG_RESTRIC_LOG2      6 // restrict coded flag size
#define IBC_MAX_CU_LOG2                      4 /* max block size for ibc search in unit of log2 */
#define IBC_MAX_CAND_SIZE                    (1 << IBC_MAX_CU_LOG2)
#if BVD_CODING
#define BVD_EXG_ORDER                        2
#endif
#endif
/* IBC (END) */

/* BVP (START) */
#if IBC_BVP
#define ALLOWED_HBVP_NUM                   12
#define MAX_NUM_BVP                        7
#define CBVP_TH_SIZE                       32
#define CBVP_TH_CNT                        2
#endif
/* BVP (END) */

/* SP (START) */
#if USE_SP
#if SP_SVP
#define SP_SEARCH_RANGE                     64
#endif
#define SP_STRING_INFO_NO                  1024//  1 << ( MAX_CU_LOG2 + MAX_CU_LOG2 - 4)//64*64 / 4 = 1024 //number of max sp string_copy_info in a CU
#define SP_MAX_SPS_CANDS                   256
#define SP_RECENT_CANDS                    12
#define SP_EXG_ORDER                       3
#if EVS_UBVS_MODE
#define MAX_CU_SIZE_IN_BIT                 6
#define MAX_SRB_SIZE                       15
#define MAX_SSRB_SIZE                      17
#define MAX_SRB_PRED_SIZE                  28
#define UV_WEIGHT                          0.25
#define EVS_PV_MAX                         10
#endif
#define GET_TRAV_X(trav_idx, cu_width)                  ((trav_idx) & ((cu_width) - 1))
#define GET_TRAV_Y(trav_idx, cu_width_log2)             ((trav_idx) >> (cu_width_log2))
#define IS_VALID_SP_CU_SIZE(cu_width, cu_height)        (((cu_width) < 4 || (cu_height) < 4 || (cu_width) > 32 || (cu_height) > 32) ? 0 : 1)
#if EVS_UBVS_MODE
#define IS_VALID_CS2_CU_SIZE(cu_width, cu_height)       (((cu_width) < 8 || (cu_height) < 8 || (cu_width) > 32 || (cu_height) > 32) ? 0 : 1)
#endif
#endif
/* SP (END) */

/* MVAP (START) */
#if MVAP
#define MIN_SUB_BLOCK_SIZE                 8
typedef enum _MVAP_ANGULAR_PRED_MODE
{
	HORIZONTAL,                    ///< horizontal
	VERTICAL,                      ///< vertical
	HORIZONTAL_UP,                 ///< horizontal up
	HORIZONTAL_DOWN,               ///< horizontal down
	VERTICAL_RIGHT,                ///< vertical right
	ALLOWED_MVAP_NUM = 5           ///< allowed mvap number
} MVAP_ANGULAR_PRED_MODE;
#endif
/* MVAP (END) */

/* ETMVP (START) */
#if ETMVP
#define MIN_ETMVP_SIZE                 8
#define MIN_ETMVP_MC_SIZE              8
#define MAX_ETMVP_NUM                  5
#endif
/* ETMVP (END) */

/* Common stuff (START) */
#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#define FORCE_INLINE __forceinline
//#define INLINE __inline
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#define FORCE_INLINE __attribute__((always_inline))
//#define INLINE __inline__
#endif
#endif
typedef int BOOL;
#define TRUE  1
#define FALSE 0

/* AFFINE (START) */
#define VER_NUM                             4
#define AFFINE_MAX_NUM_LT                   3 ///< max number of motion candidates in top-left corner
#define AFFINE_MAX_NUM_RT                   2 ///< max number of motion candidates in top-right corner
#define AFFINE_MAX_NUM_LB                   2 ///< max number of motion candidates in left-bottom corner
#define AFFINE_MAX_NUM_RB                   1 ///< max number of motion candidates in right-bottom corner
#define AFFINE_MIN_BLOCK_SIZE               4 ///< Minimum affine MC block size

#define AFF_MAX_NUM_MRG                     5 // maximum affine merge candidates
#define AFF_MODEL_CAND                      2 // maximum affine model based candidate

#define MAX_MEMORY_ACCESS_BI                ((8 + 7) * (8 + 7) / 64)
#define MAX_MEMORY_ACCESS_UNI               ((8 + 7) * (4 + 7) / 32)

// AFFINE ME configuration (non-normative)
#define AF_ITER_UNI                         7 // uni search iteration time
#define AF_ITER_BI                          5 // bi search iteration time
#define AFFINE_BI_ITER                      1

#define AFF_SIZE                            16
/* AFFINE (END) */

/* MIPF (START) */
#if MIPF
#define MIPF_TH_SIZE                       64
#define MIPF_TH_SIZE_CHROMA                32
#define MIPF_TH_DIST                       1
#define MIPF_TH_DIST_CHROMA                2
#endif
/* MIPF (END) */

/* IPF (START) */
#define NUM_IPF_CTX                         1 ///< number of context models for MPI Idx coding
#if DT_PARTITION
#define DT_INTRA_BOUNDARY_FILTER_OFF        1 ///< turn off boundary filter if intra DT is ON
#endif
/* IPF (END) */

/* For debugging (START) */
#define ENC_DEC_TRACE                       0
#if ENC_DEC_TRACE
#define MVF_TRACE                           0 ///< use for tracing MVF
#define TRACE_REC                           0 ///< trace reconstructed pixels
#define TRACE_RDO                           0 //!< Trace only encode stream (0), only RDO (1) or all of them (2)
#define TRACE_BIN                           1 //!< trace each bin
#if TRACE_RDO
#define TRACE_RDO_EXCLUDE_I                 0 //!< Exclude I frames
#endif
extern FILE *fp_trace;
extern int fp_trace_print;
extern int fp_trace_counter;
#if TRACE_RDO == 1
#define COM_TRACE_SET(A) fp_trace_print =! A
#elif TRACE_RDO == 2
#define COM_TRACE_SET(A)
#else
#define COM_TRACE_SET(A) fp_trace_print = A
#endif
#define COM_TRACE_STR(STR) if(fp_trace_print) { fprintf(fp_trace, STR); fflush(fp_trace); }
#define COM_TRACE_INT(INT) if(fp_trace_print) { fprintf(fp_trace, "%d ", INT); fflush(fp_trace); }
#define COM_TRACE_COUNTER  COM_TRACE_INT(fp_trace_counter++); COM_TRACE_STR("\t")
#define COM_TRACE_MV(X, Y) if(fp_trace_print) { fprintf(fp_trace, "(%d, %d) ", X, Y); fflush(fp_trace); }
#define COM_TRACE_FLUSH    if(fp_trace_print) fflush(fp_trace)
#else
#define COM_TRACE_SET(A)
#define COM_TRACE_STR(str)
#define COM_TRACE_INT(INT)
#define COM_TRACE_COUNTER
#define COM_TRACE_MV(X, Y)
#define COM_TRACE_FLUSH
#endif
/* For debugging (END) */

#define STRIDE_IMGB2PIC(s_imgb)          ((s_imgb) >> 1)

#define Y_C                                0  /* Y luma */
#define U_C                                1  /* Cb Chroma */
#define V_C                                2  /* Cr Chroma */
#define N_C                                3  /* number of color component */

#define REFP_0                             0
#define REFP_1                             1
#define REFP_NUM                           2

#if ST_CHROMA
#define CHANNEL_LUMA                       0  /* Y Luma */
#define CHANNEL_CHROMA                     1  /* Cb and Cr Chroma */
#define MAX_NUM_CHANNEL                    2  /* number of color channel */
#endif
/* X direction motion vector indicator */
#define MV_X                               0
/* Y direction motion vector indicator */
#define MV_Y                               1
/* Maximum count (dimension) of motion */
#define MV_D                               2

#define N_REF                              2  /* left, up, right */

#define NUM_NEIB                           1  //since SUCO is not implemented

#define MINI_SIZE_LOG2                     3
#define MINI_SIZE                          (1 << MINI_SIZE_LOG2)
#define MAX_CU_LOG2                        7
#define MIN_CU_LOG2                        2
#define MAX_CU_SIZE                       (1 << MAX_CU_LOG2)
#define MIN_CU_SIZE                       (1 << MIN_CU_LOG2)
#define MAX_CU_DIM                        (MAX_CU_SIZE * MAX_CU_SIZE)
#define MIN_CU_DIM                        (MIN_CU_SIZE * MIN_CU_SIZE)
#define MAX_CU_DEPTH                       6  /* 128x128 ~ 4x4 */
#if AWP
#define AWP_ANGLE_NUM                     (8)
#define AWP_RWFERENCE_SET_NUM             (7)
#define AWP_MODE_NUM                      (AWP_ANGLE_NUM * AWP_RWFERENCE_SET_NUM)
#define AWP_MV_LIST_LENGTH                 5
#define MIN_AWP_SIZE_LOG2                  3 /*  8 */
#define MAX_AWP_SIZE_LOG2                  6 /* 64 */
#define MIN_AWP_SIZE                      (1 << MIN_AWP_SIZE_LOG2)
#define MAX_AWP_SIZE                      (1 << MAX_AWP_SIZE_LOG2)
#define MAX_AWP_DIM                       (MAX_AWP_SIZE * MAX_AWP_SIZE)

// encoder para
#define AWP_RDO_NUM                        7
#define CANDIDATES_PER_PARTITION            2
#endif
#if AWP_MVR
#define NUM_PARTITION_FOR_AWP_MVR_RD      42
#endif
#define MAX_TR_LOG2                        6  /* 64x64 */
#define MIN_TR_LOG2                        1  /* 2x2 */
#define MAX_TR_SIZE                        (1 << MAX_TR_LOG2)
#define MIN_TR_SIZE                        (1 << MIN_TR_LOG2)
#define MAX_TR_DIM                         (MAX_TR_SIZE * MAX_TR_SIZE)
#define MIN_TR_DIM                         (MIN_TR_SIZE * MIN_TR_SIZE)

#if TB_SPLIT_EXT
#define MAX_NUM_PB                         4
#define MAX_NUM_TB                         4
#define PB0                                0  // default PB idx
#define TB0                                0  // default TB idx
#define TBUV0                              0  // default TB idx for chroma

#define NUM_SL_INTER                       10
#define NUM_SL_INTRA                       8
#endif

/* maximum CB count in a LCB */
#define MAX_CU_CNT_IN_LCU                  (MAX_CU_DIM/MIN_CU_DIM)
/* pixel position to SCB position */
#define PEL2SCU(pel)                       ((pel) >> MIN_CU_LOG2)

#define PIC_PAD_SIZE_L                     (MAX_CU_SIZE + 16)
#define PIC_PAD_SIZE_C                     (PIC_PAD_SIZE_L >> 1)

#define NUM_AVS2_SPATIAL_MV                3
#define NUM_SKIP_SPATIAL_MV                6
#define MVPRED_L                           0
#define MVPRED_U                           1
#define MVPRED_UR                          2
#define MVPRED_xy_MIN                      3

/* for GOP 16 test, increase to 32 */
/* maximum reference picture count. Originally, Max. 16 */
/* for GOP 16 test, increase to 32 */
#define MAX_NUM_REF_PICS                   17

#define MAX_NUM_ACTIVE_REF_FRAME           4

/* DPB Extra size */
#define EXTRA_FRAME                        MAX_NUM_REF_PICS

/* maximum picture buffer size */
#ifdef AML
#define MAX_PB_SIZE                       (32)
#else
#define MAX_PB_SIZE                       (MAX_NUM_REF_PICS + EXTRA_FRAME)
#endif
/* Neighboring block availability flag bits */
#define AVAIL_BIT_UP                       0
#define AVAIL_BIT_LE                       1
#define AVAIL_BIT_UP_LE                    2


/* Neighboring block availability flags */
#define AVAIL_UP                          (1 << AVAIL_BIT_UP)
#define AVAIL_LE                          (1 << AVAIL_BIT_LE)
#define AVAIL_UP_LE                       (1 << AVAIL_BIT_UP_LE)


/* MB availability check macro */
#define IS_AVAIL(avail, pos)            (((avail) & (pos)) == (pos))
/* MB availability set macro */
#define SET_AVAIL(avail, pos)             (avail) |= (pos)
/* MB availability remove macro */
#define REM_AVAIL(avail, pos)             (avail) &= (~(pos))
/* MB availability into bit flag */
#define GET_AVAIL_FLAG(avail, bit)      (((avail) >> (bit)) & 0x1)

/*****************************************************************************
 * slice type
 *****************************************************************************/
#define SLICE_I                            COM_ST_I
#define SLICE_P                            COM_ST_P
#define SLICE_B                            COM_ST_B

#define IS_INTRA_SLICE(slice_type)       ((slice_type) == SLICE_I))
#define IS_INTER_SLICE(slice_type)      (((slice_type) == SLICE_P) || ((slice_type) == SLICE_B))

/*****************************************************************************
 * prediction mode
 *****************************************************************************/
#define MODE_INVALID                      -1
#define MODE_INTRA                         0
#define MODE_INTER                         1
#define MODE_SKIP                          2
#define MODE_DIR                           3
#if USE_IBC
#define MODE_IBC                           4
#endif

/*****************************************************************************
 * prediction direction
 *****************************************************************************/
typedef enum _INTER_PRED_DIR
{
	PRED_L0 = 0,
	PRED_L1 = 1,
	PRED_BI = 2,
	PRED_DIR_NUM = 3,
} INTER_PRED_DIR;

#if BD_AFFINE_AMVR
#define Tab_Affine_AMVR(x)                 ((x == 0) ? 2 : ((x == 1) ? 4 : 0) )
#endif

/*****************************************************************************
 * skip / direct mode
 *****************************************************************************/
#define TRADITIONAL_SKIP_NUM               (PRED_DIR_NUM + 1)
#define ALLOWED_HMVP_NUM                   8
#define MAX_SKIP_NUM                       (TRADITIONAL_SKIP_NUM + ALLOWED_HMVP_NUM)

#define UMVE_BASE_NUM                      2
#define UMVE_REFINE_STEP                   5
#define UMVE_MAX_REFINE_NUM                (UMVE_REFINE_STEP * 4)
#if UMVE_ENH
#define UMVE_REFINE_STEP_SEC_SET           8
#define UMVE_MAX_REFINE_NUM_SEC_SET        (UMVE_REFINE_STEP_SEC_SET * 4)
#endif

#if AFFINE_UMVE
#define AFFINE_UMVE_BASE_NUM               AFF_MAX_NUM_MRG
#define AFFINE_UMVE_REFINE_STEP            5
#define AFFINE_UMVE_DIR                    4
#define AFFINE_UMVE_MAX_REFINE_NUM         (AFFINE_UMVE_REFINE_STEP * AFFINE_UMVE_DIR)
#endif

#if AWP_MVR
#define AWP_MVR_REFINE_STEP                5
#define AWP_MVR_DIR                        4
#define AWP_MVR_MAX_REFINE_NUM             (AWP_MVR_REFINE_STEP * AWP_MVR_DIR)
#endif

#if INTERPF
#define MAX_INTER_SKIP_RDO                 (MAX_SKIP_NUM + NUM_RDO_INTER_FILTER)
#else
#define MAX_INTER_SKIP_RDO                 MAX_SKIP_NUM
#endif

#if EXT_AMVR_HMVP
#define THRESHOLD_MVPS_CHECK               1.1
#endif

#if SUB_TMVP
#define SBTMVP_MIN_SIZE                    16
#define SBTMVP_NUM                         4
#define SBTMVP_NUM_1D                      2
#endif

/*****************************************************************************
 * intra prediction direction
 *****************************************************************************/
#define IPD_DC                             0
#define IPD_PLN                            1  /* Luma, Planar */
#define IPD_BI                             2  /* Luma, Bilinear */

#define IPD_DM_C                           0  /* Chroma, DM*/
#define IPD_DC_C                           1  /* Chroma, DC */
#define IPD_HOR_C                          2  /* Chroma, Horizontal*/
#define IPD_VER_C                          3  /* Chroma, Vertical */
#define IPD_BI_C                           4  /* Chroma, Bilinear */
#if TSCPM
#define IPD_TSCPM_C                        5
#if ENHANCE_TSPCM || PMC || EPMC
#define IPD_TSCPM_L_C                      6
#define IPD_TSCPM_T_C                      7
#endif
#endif

#if PMC
#define IPD_MCPM_C                         8
#define IPD_MCPM_L_C                       9
#define IPD_MCPM_T_C                       10
#endif
#if PMC && EPMC
#define IPD_EMCPM_C                         11
#define IPD_EMCPM_L_C                       12
#define IPD_EMCPM_T_C                       13
#define IPD_EMCPM2_C                        14
#define IPD_EMCPM2_L_C                      15
#define IPD_EMCPM2_T_C                      16
#elif EPMC
#define IPD_EMCPM_C                         8
#define IPD_EMCPM_L_C                       9
#define IPD_EMCPM_T_C                       10
#define IPD_EMCPM2_C                        11
#define IPD_EMCPM2_L_C                      12
#define IPD_EMCPM2_T_C                      13
#endif
#define IPD_CHROMA_CNT                     5

#define IPD_INVALID                       (-1)

#define IPD_RDO_CNT                        5

#define IPD_HOR                            24 /* Luma, Horizontal */
#define IPD_VER                            12 /* Luma, Vertical */
#if EIPM
#define IPD_CNT                            66
#define IPD_HOR_EXT                        58
#define IPD_VER_EXT                        43
#define IPD_DIA_R_EXT                      50
#define IPD_DIA_L_EXT                      34
#else
#define IPD_CNT                            33
#endif
#if IPCM
#define IPD_IPCM                           33
#endif

#define IPD_DIA_R                         18 /* Luma, Right diagonal */
#define IPD_DIA_L                         3  /* Luma, Left diagonal */
#define IPD_DIA_U                         32 /* Luma, up diagonal */

#if FIMC
#define NUM_MPM                                 2 // Fixed for anchor
#define CNTMPM_INIT_NUM                   NUM_MPM //
#if EIPM
#define CNTMPM_TABLE_LENGTH               IPD_CNT // table length
#else
#define CNTMPM_TABLE_LENGTH    (IPD_CNT + !!IPCM) // table length
#endif
#define CNTMPM_BASE_VAL                        70 //
#define CNTMPM_MAX_NUM        CNTMPM_TABLE_LENGTH // Fixed for anchor
#endif

/*****************************************************************************
* Transform
*****************************************************************************/


/*****************************************************************************
 * reference index
 *****************************************************************************/
#define REFI_INVALID                      (-1)
#define REFI_IS_VALID(refi)               ((refi) >= 0)
#define SET_REFI(refi, idx0, idx1)        (refi)[REFP_0] = (idx0); (refi)[REFP_1] = (idx1)

/*****************************************************************************
 * macros for CU map

 - [ 0: 6] : slice number (0 ~ 128)
 - [ 7:14] : reserved
 - [15:15] : 1 -> intra CU, 0 -> inter CU
 - [16:22] : QP
 - [23:23] : skip mode flag
 - [24:24] : luma cbf
 - [25:25] : ibc flag
 - [26:30] : reserved
 - [31:31] : 0 -> no encoded/decoded CU, 1 -> encoded/decoded CU
 *****************************************************************************/
/* set intra CU flag to map */
#define MCU_SET_INTRA_FLAG(m)           (m) = ((m) | (1 << 15))
/* get intra CU flag from map */
#define MCU_GET_INTRA_FLAG(m)           (int)(((m) >> 15) & 1)
/* clear intra CU flag in map */
#define MCU_CLEAR_INTRA_FLAG(m)         (m) = ((m) & 0xFFFF7FFF)

/* set QP to map */
#if CUDQP_PLATFORM_BUGFIX
#define MCU_SET_QP(m, qp)               (m) = ((m & 0xFF80FFFF) | ((qp) & 0x7F) << 16)
#else
#define MCU_SET_QP(m, qp)               (m) = ((m) | ((qp) & 0x7F) << 16)
#endif
/* get QP from map */
#define MCU_GET_QP(m)                   (int)(((m) >> 16) & 0x7F)

/* set skip mode flag */
#define MCU_SET_SF(m)                   (m) = ((m) | (1 << 23))
/* get skip mode flag */
#define MCU_GET_SF(m)                   (int)(((m) >> 23) & 1)
/* clear skip mode flag */
#define MCU_CLR_SF(m)                   (m) = ((m) & (~(1 << 23)))

/* set cu cbf flag */
#define MCU_SET_CBF(m)                  (m) = ((m) | (1 << 24))
/* get cu cbf flag */
#define MCU_GET_CBF(m)                  (int)(((m) >> 24) & 1)
/* clear cu cbf flag */
#define MCU_CLR_CBF(m)                  (m) = ((m) & (~(1 << 24)))

#if USE_IBC
/* set ibc mode flag */
#define MCU_SET_IBC(m)                  (m) = ((m) | (1 << 25))
/* get ibc mode flag */
#define MCU_GET_IBC(m)                  (int)(((m) >> 25) & 1)
/* clear ibc mode flag */
#define MCU_CLR_IBC(m)                  (m) = ((m) & (~(1 << 25)))
#endif

/* set encoded/decoded flag of CU to map */
#define MCU_SET_CODED_FLAG(m)           (m) = ((m) | (1 << 31))
/* get encoded/decoded flag of CU from map */
#define MCU_GET_CODED_FLAG(m)           (int)(((m) >> 31) & 1)
/* clear encoded/decoded CU flag to map */
#define MCU_CLR_CODED_FLAG(m)           (m) = ((m) & 0x7FFFFFFF)

/* multi bit setting: intra flag, encoded/decoded flag, slice number */
#define MCU_SET_IF_COD_SN_QP(m, i, sn, qp) \
	(m) = (((m) & 0xFF807F80) | ((sn) & 0x7F) | ((qp) << 16) | ((i) << 15) | (1 << 31))

#define MCU_IS_COD_NIF(m)               ((((m) >> 15) & 0x10001) == 0x10000)

#if BGC
/*
- [10:11] : bgc flag and bgc idx
*/
#define MCU_SET_BGC_FLAG(m)             (m) = ((m) | (1 << 10))
#define MCU_GET_BGC_FLAG(m)             (int)(((m) >> 10) & 1)
#define MCU_CLR_BGC_FLAG(m)             (m) = ((m) & (~(1 << 10)))
#define MCU_SET_BGC_IDX(m)              (m) = ((m) | (1 << 11))
#define MCU_GET_BGC_IDX(m)              (int)(((m) >> 11) & 1)
#define MCU_CLR_BGC_IDX(m)              (m) = ((m) & (~(1 << 11)))
#endif

/*
- [8:9] : affine vertex number, 00: 1(trans); 01: 2(affine); 10: 3(affine); 11: 4(affine)
*/

/* set affine CU mode to map */
#define MCU_SET_AFF(m, v)               (m) = ((m & 0xFFFFFCFF) | ((v) & 0x03) << 8)
/* get affine CU mode from map */
#define MCU_GET_AFF(m)                  (int)(((m) >> 8) & 0x03)
/* clear affine CU mode to map */
#define MCU_CLR_AFF(m)                  (m) = ((m) & 0xFFFFFCFF)

/* set x scu offset & y scu offset relative to top-left scu of CU to map */
#define MCU_SET_X_SCU_OFF(m, v)         (m) = ((m & 0xFFFF00FF) | ((v) & 0xFF) << 8)
#define MCU_SET_Y_SCU_OFF(m, v)         (m) = ((m & 0xFF00FFFF) | ((v) & 0xFF) << 16)
/* get x scu offset & y scu offset relative to top-left scu of CU in map */
#define MCU_GET_X_SCU_OFF(m)            (int)(((m) >> 8) & 0xFF)
#define MCU_GET_Y_SCU_OFF(m)            (int)(((m) >> 16) & 0xFF)

/* set cu_width_log2 & cu_height_log2 to map */
#define MCU_SET_LOGW(m, v)              (m) = ((m & 0xF0FFFFFF) | ((v) & 0x0F) << 24)
#define MCU_SET_LOGH(m, v)              (m) = ((m & 0x0FFFFFFF) | ((v) & 0x0F) << 28)
/* get cu_width_log2 & cu_height_log2 to map */
#define MCU_GET_LOGW(m)                 (int)(((m) >> 24) & 0x0F)
#define MCU_GET_LOGH(m)                 (int)(((m) >> 28) & 0x0F)

#if BET_SPLIT_DECISION
/* save to map_cu_mode*/
#define MCU_SET_QT_DEPTH(m, v)       (m) = ((m & 0xFFFFFFF0) | ((v) & 0x0F) << 0)
#define MCU_SET_BET_DEPTH(m, v)      (m) = ((m & 0xFFFFFF0F) | ((v) & 0x0F) << 4)
#define MCU_GET_QT_DEPTH(m)          (int)(((m) >> 0) & 0x0F)
#define MCU_GET_BET_DEPTH(m)         (int)(((m) >> 4) & 0x0F)
#endif

#if TB_SPLIT_EXT
//set
#define MCU_SET_TB_PART_LUMA(m,v)       (m) =  ((m & 0xFFFFFF00)  | ((v) & 0xFF) << 0)
//get
#define MCU_GET_TB_PART_LUMA(m)         (int)(((m) >> 0) & 0xFF)
#endif

#if SBT
//set
#define MCU_SET_SBT_INFO(m,v)           (m) = ((m & 0xFFFF00FF) | ((v) & 0xFF) << 8)
//get
#define MCU_GET_SBT_INFO(m)             (int)(((m) >> 8) & 0xFF)
#endif

#if USE_SP
#define MSP_SET_SP_INFO(m)              (m) = (m | 0x80)
#define MSP_GET_SP_INFO(m)              (int)(((m) >> 7) & 1)
#define MSP_CLR_SP_INFO(m)              (m) = (m & 0x7F)
#if EVS_UBVS_MODE
#define MSP_SET_CS2_INFO(m)             (m) = ((m) | 0x40)
#define MSP_GET_CS2_INFO(m)             (int)(((m) >> 6) & 1)
#define MSP_CLR_CS2_INFO(m)             (m) = (m & 0xBF)
#endif
#endif

typedef u32 SBAC_CTX_MODEL;
#define NUM_POSITION_CTX                   2
#define NUM_SBAC_CTX_AFFINE_MVD_FLAG       2

#if IIP
#define NUM_IIP_CTX                        1
#endif

#if SEP_CONTEXT
#define NUM_SBAC_CTX_SKIP_FLAG             4
#else
#define NUM_SBAC_CTX_SKIP_FLAG             3
#endif
#if USE_IBC
#define NUM_SBAC_CTX_IBC_FLAG              3
#endif
#if USE_SP
#define NUM_SOIF_CTX                       1 ///< number of context models for SP or IBC flag coding
#define NUM_GSF_CTX                        1 ///< number of context models for SP flag coding
#define NUM_SP_CDF_CTX                     1 ///< number of context models for SP copy direction flag
#define NUM_SP_IMF_CTX                     1 ///< number of context models for SP is matched flag
#if SP_CODE_TEXT_BUGFIX
#define NUM_SP_STRLEN_CTX                  4 ///< number of context models for SP matched length
#else
#define NUM_SP_STRLEN_CTX                  3 ///< number of context models for SP matched length
#endif
#define NUM_SP_ABO_CTX                     1 ///< number of context models for SP is above offset flag
#define NUM_SP_OYZ_CTX                     1 ///< number of context models for SP is offset Y Zero flag
#define NUM_SP_OXZ_CTX                     1 ///< number of context models for SP is offset X Zero flag
#if !SP_CODE_TEXT_BUGFIX
#define NUM_SP_SLF_CTX                     1 ///< number of context models for SP whether using special length buffer flag
#endif
#define NUM_SP_NRF_CTX                     1
#define NUM_SP_NID_CTX                     (SP_RECENT_CANDS - 1)
#define NUM_SP_PIMF_CTX                    1 ///< number of context models for SP pixel is matched flag
#if EVS_UBVS_MODE
#define NUM_CS2F_CTX                       1 ///< number of context models for CS2 flag coding
#define NUM_SP_MODE_CTX                    1
#define NUM_SP_STRING_TYPE_CTX             1
#define NUM_SP_STRING_SCAN_MODE_CTX        1
#define NUM_SP_LOREF_MAX_LENGTH_CTX        4
#define NUM_SP_SRB_FLAG_CTX                1
#define NUM_SP_SRB_LOREFFLAG_CTX           1
#define NUM_SP_SRB_TOPRUN_CTX              3
#define NUM_SP_SRB_LEFTRUN_CTX             5
#endif
#endif
#define NUM_SBAC_CTX_SKIP_IDX              (MAX_SKIP_NUM - 1)

#if ETMVP
#define NUM_SBAC_CTX_ETMVP_IDX             (MAX_ETMVP_NUM - 1)
#define NUM_SBAC_CTX_ETMVP_FLAG            1
#endif

#if SEP_CONTEXT
#define NUM_SBAC_CTX_DIRECT_FLAG           2
#else
#define NUM_SBAC_CTX_DIRECT_FLAG           1
#endif
#if INTERPF
#define NUM_SBAC_CTX_INTERPF               2
#endif
#define NUM_SBAC_CTX_UMVE_BASE_IDX         1
#define NUM_SBAC_CTX_UMVE_STEP_IDX         1
#define NUM_SBAC_CTX_UMVE_DIR_IDX          2

#if SEP_CONTEXT
#define NUM_SBAC_CTX_SPLIT_FLAG            4
#else
#define NUM_SBAC_CTX_SPLIT_FLAG            3
#endif
#define NUM_SBAC_CTX_SPLIT_MODE            3
#define NUM_SBAC_CTX_BT_SPLIT_FLAG         9
#if SEP_CONTEXT
#define NUM_SBAC_CTX_SPLIT_DIR             5
#else
#define NUM_SBAC_CTX_SPLIT_DIR             3
#endif
#define NUM_QT_CBF_CTX                     3       /* number of context models for QT CBF */
#if SEP_CONTEXT
#define NUM_CTP_ZERO_FLAG_CTX              2       /* number of context models for ctp_zero_flag */
#define NUM_PRED_MODE_CTX                  6       /* number of context models for prediction mode */
#else
#define NUM_CTP_ZERO_FLAG_CTX              1       /* number of context models for ctp_zero_flag */
#define NUM_PRED_MODE_CTX                  5       /* number of context models for prediction mode */
#endif
#if MODE_CONS
#define NUM_CONS_MODE_CTX                  1       /* number of context models for constrained prediction mode */
#endif
#define NUM_TB_SPLIT_CTX                   1
#if SBT
#define NUM_SBAC_CTX_SBT_INFO              7
#endif
#if CUDQP
#define NUM_SBAC_CTX_CU_QP_DELTA_ABS       4
#endif

#define NUM_SBAC_CTX_DELTA_QP              4

#if SEP_CONTEXT
#define NUM_INTER_DIR_CTX                  3       /* number of context models for inter prediction direction */
#else
#define NUM_INTER_DIR_CTX                  2       /* number of context models for inter prediction direction */
#endif
#define NUM_REFI_CTX                       3

#define NUM_MVR_IDX_CTX                   (MAX_NUM_MVR - 1)
#if IBC_ABVR
#define NUM_BVR_IDX_CTX                   (MAX_NUM_BVR - 1)
#endif
#if IBC_BVP
#define NUM_BVP_IDX_CTX                   (MAX_NUM_BVP - 1)
#endif
#if BVD_CODING
#define NUM_BV_RES_CTX                     8       /* number of context models for block vector difference */
#endif
#if BD_AFFINE_AMVR
#define NUM_AFFINE_MVR_IDX_CTX            (MAX_NUM_AFFINE_MVR - 1)
#endif
#if SIBC
#define NUM_SIBC_IDX_CTX                   2
#endif

#if EXT_AMVR_HMVP
#define NUM_EXTEND_AMVR_FLAG               1
#endif
#define NUM_MV_RES_CTX                     3       /* number of context models for motion vector difference */

#if PMC && EPMC
#define NUM_INTRA_DIR_CTX                  14
#elif PMC || EPMC
#define NUM_INTRA_DIR_CTX                  13
#else
#if EIPM
#if TSCPM
#if ENHANCE_TSPCM
#define NUM_INTRA_DIR_CTX                  12
#else
#define NUM_INTRA_DIR_CTX                  11
#endif
#else
#define NUM_INTRA_DIR_CTX                  10
#endif
#else
#if TSCPM
#if ENHANCE_TSPCM
#define NUM_INTRA_DIR_CTX                  11
#else
#define NUM_INTRA_DIR_CTX                  10
#endif
#else
#define NUM_INTRA_DIR_CTX                  9
#endif
#endif
#endif

#define NUM_SBAC_CTX_AFFINE_FLAG           1
#define NUM_SBAC_CTX_AFFINE_MRG            AFF_MAX_NUM_MRG - 1

#if AWP
#define NUM_SBAC_CTX_AWP_FLAG              1
#define NUM_SBAC_CTX_AWP_IDX               2
#endif

#if SMVD
#define NUM_SBAC_CTX_SMVD_FLAG             1
#endif

#if DT_SYNTAX
#define NUM_SBAC_CTX_PART_SIZE             6
#endif

#define NUM_SAO_MERGE_FLAG_CTX             3
#define NUM_SAO_MODE_CTX                   1
#define NUM_SAO_OFFSET_CTX                 1

#define NUM_ALF_COEFF_CTX                  1
#define NUM_ALF_LCU_CTX                    1

#if ESAO
#define NUM_ESAO_LCU_CTX                   1
#define NUM_ESAO_OFFSET_CTX                1
#define NUM_ESAO_UV_COUNT_CTX              1
#endif

#if CCSAO
#define NUM_CCSAO_LCU_CTX                  1
#define NUM_CCSAO_OFFSET_CTX               1
#endif

#if SRCC
/* Scan-Region based coefficient coding (SRCC) (START) */
#define LOG2_CG_SIZE                       4

#define NUM_PREV_0VAL                      5
#define NUM_PREV_12VAL                     5

#define SCAN_REGION_GROUP                  14

#define NUM_CTX_SCANR_LUMA                 25
#define NUM_CTX_SCANR_CHROMA               3
#define NUM_CTX_SCANR                      (NUM_CTX_SCANR_LUMA + NUM_CTX_SCANR_CHROMA)

#define NUM_CTX_GT0_LUMA                   51
#define NUM_CTX_GT0_LUMA_TU                17

#define NUM_CTX_GT0_CHROMA                 9   /* number of context models for chroma gt0 flag */
#define NUM_CTX_GT0                        (NUM_CTX_GT0_LUMA + NUM_CTX_GT0_CHROMA)  /* number of context models for gt0 flag */

#define NUM_CTX_GT1_LUMA                   17
#define NUM_CTX_GT1_CHROMA                 5
#define NUM_CTX_GT1                        (NUM_CTX_GT1_LUMA + NUM_CTX_GT1_CHROMA)  /* number of context models for gt1/2 flag */

/* Scan-Region based coefficient coding (SRCC) (END) */
#endif

#if ETS
#define NUM_ETS_IDX_CTX                    1
#endif

#if EST
#define NUM_EST_IDX_CTX                    1
#endif

#if ST_CHROMA
#define NUM_ST_CHROMA_IDX_CTX              1
#endif

#define RANK_NUM 6
#define NUM_SBAC_CTX_LEVEL                 (RANK_NUM * 4)
#define NUM_SBAC_CTX_RUN                   (RANK_NUM * 4)

#define NUM_SBAC_CTX_RUN_RDOQ              (RANK_NUM * 4)


#define NUM_SBAC_CTX_LAST1                 RANK_NUM
#define NUM_SBAC_CTX_LAST2                 12

#define NUM_SBAC_CTX_LAST                  2


/* context models for arithmetic coding */
typedef struct _COM_SBAC_CTX
{
	SBAC_CTX_MODEL   skip_flag       [NUM_SBAC_CTX_SKIP_FLAG];
#if USE_IBC
	SBAC_CTX_MODEL   ibc_flag        [NUM_SBAC_CTX_IBC_FLAG];
#endif
#if USE_SP
	SBAC_CTX_MODEL   sp_or_ibc_flag  [NUM_SOIF_CTX];
	SBAC_CTX_MODEL   sp_flag         [NUM_GSF_CTX];
	SBAC_CTX_MODEL   sp_copy_direct_flag[NUM_SP_CDF_CTX];
	SBAC_CTX_MODEL   sp_is_matched_flag[NUM_SP_IMF_CTX];
	SBAC_CTX_MODEL   sp_string_length[NUM_SP_STRLEN_CTX];
	SBAC_CTX_MODEL   sp_above_offset [NUM_SP_ABO_CTX];
	SBAC_CTX_MODEL   sp_offset_y_zero[NUM_SP_OYZ_CTX];
	SBAC_CTX_MODEL   sp_offset_x_zero[NUM_SP_OXZ_CTX];
#if !SP_CODE_TEXT_BUGFIX
	SBAC_CTX_MODEL   sp_special_len_flag[NUM_SP_SLF_CTX];
#endif
	SBAC_CTX_MODEL   sp_n_recent_flag[NUM_SP_NRF_CTX];
	SBAC_CTX_MODEL   sp_n_index      [NUM_SP_NID_CTX];
#if SP_SLR
	SBAC_CTX_MODEL   sp_pixel_is_matched_flag[NUM_SP_PIMF_CTX];
#endif
#if EVS_UBVS_MODE
	SBAC_CTX_MODEL   sp_cs2_flag[NUM_CS2F_CTX];
	SBAC_CTX_MODEL   sp_mode_context[NUM_SP_MODE_CTX];
	SBAC_CTX_MODEL   sp_str_type_context[NUM_SP_STRING_TYPE_CTX];
	SBAC_CTX_MODEL   sp_str_scanmode_context[NUM_SP_STRING_SCAN_MODE_CTX];
	SBAC_CTX_MODEL   sp_lo_ref_maxlength_context[NUM_SP_LOREF_MAX_LENGTH_CTX];
	//For SRB
	SBAC_CTX_MODEL   sp_evs_present_flag_context[NUM_SP_SRB_FLAG_CTX];
	SBAC_CTX_MODEL   sp_SRB_lo_ref_color_flag_context[NUM_SP_SRB_LOREFFLAG_CTX];
	SBAC_CTX_MODEL   sp_SRB_copy_toprun_context[NUM_SP_SRB_TOPRUN_CTX];
#endif
#endif
	SBAC_CTX_MODEL   skip_idx_ctx    [NUM_SBAC_CTX_SKIP_IDX];
	SBAC_CTX_MODEL   direct_flag     [NUM_SBAC_CTX_DIRECT_FLAG];
#if INTERPF
	SBAC_CTX_MODEL   inter_filter_flag[NUM_SBAC_CTX_INTERPF];
#endif
#if BGC
	SBAC_CTX_MODEL   bgc_flag;
	SBAC_CTX_MODEL   bgc_idx;
#endif
#if AWP
	SBAC_CTX_MODEL   umve_awp_flag;
#else
	SBAC_CTX_MODEL   umve_flag;
#endif
	SBAC_CTX_MODEL   umve_base_idx   [NUM_SBAC_CTX_UMVE_BASE_IDX];
	SBAC_CTX_MODEL   umve_step_idx   [NUM_SBAC_CTX_UMVE_STEP_IDX];
	SBAC_CTX_MODEL   umve_dir_idx    [NUM_SBAC_CTX_UMVE_DIR_IDX];
#if AFFINE_UMVE
	SBAC_CTX_MODEL   affine_umve_flag;
	SBAC_CTX_MODEL   affine_umve_step_idx   [NUM_SBAC_CTX_UMVE_STEP_IDX];
	SBAC_CTX_MODEL   affine_umve_dir_idx    [NUM_SBAC_CTX_UMVE_DIR_IDX];
#endif
	SBAC_CTX_MODEL   inter_dir       [NUM_INTER_DIR_CTX];
	SBAC_CTX_MODEL   intra_dir       [NUM_INTRA_DIR_CTX];
	SBAC_CTX_MODEL   pred_mode       [NUM_PRED_MODE_CTX];
#if MODE_CONS
	SBAC_CTX_MODEL   cons_mode       [NUM_CONS_MODE_CTX];
#endif
#if IIP
	SBAC_CTX_MODEL   iip_flag        [NUM_IIP_CTX];
#endif
	SBAC_CTX_MODEL   ipf_flag        [NUM_IPF_CTX];
	SBAC_CTX_MODEL   refi            [NUM_REFI_CTX];
	SBAC_CTX_MODEL   mvr_idx         [NUM_MVR_IDX_CTX];
#if IBC_ABVR
	SBAC_CTX_MODEL   bvr_idx         [NUM_BVR_IDX_CTX];
#endif
#if BD_AFFINE_AMVR
	SBAC_CTX_MODEL   affine_mvr_idx  [NUM_AFFINE_MVR_IDX_CTX];
#endif
#if EXT_AMVR_HMVP
	SBAC_CTX_MODEL   mvp_from_hmvp_flag[NUM_EXTEND_AMVR_FLAG];
#endif
#if IBC_BVP
	SBAC_CTX_MODEL   cbvp_idx[NUM_BVP_IDX_CTX];
#endif
#if SIBC
	SBAC_CTX_MODEL   sibc_flag       [NUM_SIBC_IDX_CTX];
#endif
#if BVD_CODING
	SBAC_CTX_MODEL   bvd             [2][NUM_BV_RES_CTX];
#endif
	SBAC_CTX_MODEL   mvd             [2][NUM_MV_RES_CTX];
	SBAC_CTX_MODEL   ctp_zero_flag   [NUM_CTP_ZERO_FLAG_CTX];
	SBAC_CTX_MODEL   cbf             [NUM_QT_CBF_CTX];
#if CUDQP
	SBAC_CTX_MODEL   cu_qp_delta_abs [NUM_SBAC_CTX_CU_QP_DELTA_ABS];
#endif
	SBAC_CTX_MODEL   tb_split        [NUM_TB_SPLIT_CTX];
#if SBT
	SBAC_CTX_MODEL   sbt_info        [NUM_SBAC_CTX_SBT_INFO];
#endif
#if SRCC
	SBAC_CTX_MODEL   cc_gt0          [NUM_CTX_GT0];
	SBAC_CTX_MODEL   cc_gt1          [NUM_CTX_GT1];
	SBAC_CTX_MODEL   cc_scanr_x      [NUM_CTX_SCANR];
	SBAC_CTX_MODEL   cc_scanr_y      [NUM_CTX_SCANR];
#endif
	SBAC_CTX_MODEL   run             [NUM_SBAC_CTX_RUN];
	SBAC_CTX_MODEL   run_rdoq        [NUM_SBAC_CTX_RUN];
	SBAC_CTX_MODEL   last1           [NUM_SBAC_CTX_LAST1 * 2];
	SBAC_CTX_MODEL   last2           [NUM_SBAC_CTX_LAST2 * 2 - 2];
	SBAC_CTX_MODEL   level           [NUM_SBAC_CTX_LEVEL];
	SBAC_CTX_MODEL   split_flag      [NUM_SBAC_CTX_SPLIT_FLAG];
	SBAC_CTX_MODEL   bt_split_flag   [NUM_SBAC_CTX_BT_SPLIT_FLAG];
	SBAC_CTX_MODEL   split_dir       [NUM_SBAC_CTX_SPLIT_DIR];
	SBAC_CTX_MODEL   split_mode      [NUM_SBAC_CTX_SPLIT_MODE];
	SBAC_CTX_MODEL   affine_flag     [NUM_SBAC_CTX_AFFINE_FLAG];
	SBAC_CTX_MODEL   affine_mrg_idx  [NUM_SBAC_CTX_AFFINE_MRG];
#if ETMVP
	SBAC_CTX_MODEL   etmvp_flag[NUM_SBAC_CTX_ETMVP_FLAG];
	SBAC_CTX_MODEL   etmvp_idx[NUM_SBAC_CTX_ETMVP_IDX];
#endif
#if AWP
	SBAC_CTX_MODEL   awp_flag        [NUM_SBAC_CTX_AWP_FLAG];
	SBAC_CTX_MODEL   awp_idx         [NUM_SBAC_CTX_AWP_IDX];
#endif
#if AWP_MVR
	SBAC_CTX_MODEL   awp_mvr_flag;
	SBAC_CTX_MODEL   awp_mvr_step_idx[NUM_SBAC_CTX_UMVE_STEP_IDX];
	SBAC_CTX_MODEL   awp_mvr_dir_idx [NUM_SBAC_CTX_UMVE_DIR_IDX];
#endif
#if SMVD
	SBAC_CTX_MODEL   smvd_flag       [NUM_SBAC_CTX_SMVD_FLAG];
#endif
#if DT_SYNTAX
	SBAC_CTX_MODEL   part_size       [NUM_SBAC_CTX_PART_SIZE];
#endif
#if ETS
	SBAC_CTX_MODEL   ets_flag        [NUM_ETS_IDX_CTX];
#endif
#if EST
	SBAC_CTX_MODEL   est_flag        [NUM_EST_IDX_CTX];
#endif
#if ST_CHROMA
	SBAC_CTX_MODEL   st_chroma_flag[NUM_ST_CHROMA_IDX_CTX];
#endif

	SBAC_CTX_MODEL   sao_merge_flag  [NUM_SAO_MERGE_FLAG_CTX];
	SBAC_CTX_MODEL   sao_mode        [NUM_SAO_MODE_CTX];
	SBAC_CTX_MODEL   sao_offset      [NUM_SAO_OFFSET_CTX];
	SBAC_CTX_MODEL   alf_lcu_enable  [NUM_ALF_LCU_CTX];
	SBAC_CTX_MODEL   delta_qp        [NUM_SBAC_CTX_DELTA_QP];
#if ESAO
	SBAC_CTX_MODEL   esao_lcu_enable [NUM_ESAO_LCU_CTX];
	SBAC_CTX_MODEL   esao_offset     [NUM_ESAO_OFFSET_CTX];
	SBAC_CTX_MODEL   esao_chroma_mode_flag[NUM_ESAO_UV_COUNT_CTX];
#endif
#if CCSAO
	SBAC_CTX_MODEL   ccsao_lcu_flag  [NUM_CCSAO_LCU_CTX];
	SBAC_CTX_MODEL   ccsao_offset    [NUM_CCSAO_OFFSET_CTX];
#endif
} COM_SBAC_CTX;

#define COEF_SCAN_ZIGZAG                    0
#define COEF_SCAN_TYPE_NUM                  1

/* Maximum transform dynamic range (excluding sign bit) */
#define MAX_TX_DYNAMIC_RANGE               15
#if SRCC
#define MAX_TX_VAL                       ((1 << MAX_TX_DYNAMIC_RANGE) - 1)
#define MIN_TX_VAL                      (-(1 << MAX_TX_DYNAMIC_RANGE))
#endif

#define QUANT_SHIFT                        14

/* neighbor CUs
   neighbor position:

   D     B     C

   A     X,<G>

   E          <F>
*/
#define MAX_NEB                            5
#define NEB_A                              0  /* left */
#define NEB_B                              1  /* up */
#define NEB_C                              2  /* up-right */
#define NEB_D                              3  /* up-left */
#define NEB_E                              4  /* low-left */

#define NEB_F                              5  /* co-located of low-right */
#define NEB_G                              6  /* co-located of X */
#define NEB_X                              7  /* center (current block) */
#define NEB_H                              8  /* right */
#define NEB_I                              9  /* low-right */
#define MAX_NEB2                           10

/* SAO_AVS2(START) */
typedef enum _SAO_MODE   //mode
{
	SAO_MODE_OFF = 0,
	SAO_MODE_MERGE,
	SAO_MODE_NEW,
	NUM_SAO_MODES
} SAO_MODE;

typedef enum _SAO_MODE_MERGE_TYPE
{
	SAO_MERGE_LEFT = 0,
	SAO_MERGE_ABOVE,
	NUM_SAO_MERGE_TYPES
} SAO_MODE_MERGE_TYPE;

typedef enum _SAO_MODE_NEW_TYPE   //NEW: types
{
	SAO_TYPE_EO_0,
	SAO_TYPE_EO_90,
	SAO_TYPE_EO_135,
	SAO_TYPE_EO_45,
	SAO_TYPE_BO,
	NUM_SAO_NEW_TYPES
} SAO_MODE_NEW_TYPE;

typedef enum _SAO_EO_CLASS   // EO Groups, the assignments depended on how you implement the edgeType calculation
{
	SAO_CLASS_EO_FULL_VALLEY = 0,
	SAO_CLASS_EO_HALF_VALLEY = 1,
	SAO_CLASS_EO_PLAIN = 2,
	SAO_CLASS_EO_HALF_PEAK = 3,
	SAO_CLASS_EO_FULL_PEAK = 4,
	SAO_CLASS_BO = 5,
	NUM_SAO_EO_CLASSES = SAO_CLASS_BO,
	NUM_SAO_OFFSET
} SAO_EO_CLASS;

typedef struct _SAO_STAT_DATA
{
	long long int diff[MAX_NUM_SAO_CLASSES];
	int count[MAX_NUM_SAO_CLASSES];
} SAO_STAT_DATA;

typedef struct _SAO_BLK_PARAM
{
	int mode_idc; //NEW, MERGE, OFF
	int type_idc; //NEW: EO_0, EO_90, EO_135, EO_45, BO. MERGE: left, above
	int start_band; //BO: starting band index
	int start_band2;
	int delta_band;
	int offset[MAX_NUM_SAO_CLASSES];
} SAO_BLK_PARAM;
/* SAO_AVS2(END) */

#if DBR
typedef struct _DBR_PARAM
{
	int dbr_horizontal_enabled;
	int thresh_horizontal_index;
	int horizontal_offsets[6];
	int dbr_vertical_enabled;
	int thresh_vertical_index;
	int vertical_offsets[6];
} DBR_PARAM;
#endif

#if ESAO
/* ESAO_AVS3(START) */
typedef struct _ESAO_STAT_DATA
{
	long long diff[ESAO_LABEL_CLASSES_MAX];
	int count[ESAO_LABEL_CLASSES_MAX];
} ESAO_STAT_DATA;

typedef struct _ESAO_BLK_PARAM
{
	int  offset[ESAO_LABEL_CLASSES_MAX];
	int *lcu_flag;
}ESAO_BLK_PARAM;

typedef struct _ESAO_FUNC_POINTER
{
	void (*esao_on_block_for_luma)(pel* p_dst, int i_dst, pel * p_src, int i_src, int i_block_w, int i_block_h, int bit_depth, int bo_value, const int *esao_offset, int lcu_available_left, int lcu_available_right,
		int lcu_available_up, int lcu_available_down, int lcu_available_upleft, int lcu_available_upright, int lcu_available_leftdown, int lcu_available_rightdwon, int luma_type);
	void (*esao_on_block_for_chroma)(pel * p_dst, int i_dst, pel * p_src, int i_src, int i_block_w, int i_block_h, int bo_value, int shift_value, int bit_depth, const int *esao_offset);
}ESAO_FUNC_POINTER;
/* ESAO_AVS3(END) */
#endif

#if CCSAO
/* CCSAO_AVS3(START) */
typedef struct
{
	long long diff [CCSAO_CLASS_NUM];
	int       count[CCSAO_CLASS_NUM];
}CCSAO_STAT_DATA;

#if CCSAO_ENHANCEMENT
typedef struct
{
	long long diff [CCSAO_SET_NUM][CCSAO_CLASS_NUM];
	int       count[CCSAO_SET_NUM][CCSAO_CLASS_NUM];
}CCSAO_REFINE_DATA;

typedef struct
{
	int       lcu_pos;
	double    lcu_cost;
}CCSAO_LCU_COST;

typedef struct
{
	BOOL      en;
	int       set;
	int       cnt;
}CCSAO_SET_CNT;

typedef struct
{
	int       set_num;
	int       type  [CCSAO_SET_NUM];
	int       mode  [CCSAO_SET_NUM];
	int       mode_c[CCSAO_SET_NUM];
	int       offset[CCSAO_SET_NUM][CCSAO_CLASS_NUM];
	int      *lcu_flag;
}CCSAO_BLK_PARAM;
#else
typedef struct
{
	int       type;
	int       mode;
	int       offset[CCSAO_CLASS_NUM];
	int      *lcu_flag;
}CCSAO_BLK_PARAM;
#endif

typedef struct
{
	void(*ccsao_on_block_for_chroma)(pel *p_dst, int i_dst, pel *p_src, int i_src,
#if CCSAO_ENHANCEMENT
		pel *p_src2, int i_src2,
#endif
		int lcu_width_c, int lcu_height_c, int bit_depth, int type, int band_num,
#if CCSAO_ENHANCEMENT
		int band_num_c,
#endif
		const int *ccsao_offset,
		int lcu_available_left, int lcu_available_right, int lcu_available_up, int lcu_available_down, int lcu_available_upleft, int lcu_available_upright, int lcu_available_leftdown, int lcu_available_rightdwon);
}CCSAO_FUNC_POINTER;
/* CCSAO_AVS3(END) */
#endif

/* ALF_AVS2(START) */
typedef struct _ALF_PARAM
{
	int alf_flag;
	int num_coeff;
	int filters_per_group;
#if ALF_IMP
	int dir_index;
	int max_filter_num;
#endif
	int component_id;
#ifdef AML
	int32_t filter_pattern[NO_VAR_BINS];
	int32_t coeff_multi[NO_VAR_BINS][ALF_MAX_NUM_COEF];
#else
	int *filter_pattern;
	int **coeff_multi;
#endif
} ALF_PARAM;

typedef struct _ALF_CORR_DATA
{
	double ***E_corr; //!< auto-correlation matrix
	double  **y_corr; //!< cross-correlation
	double   *pix_acc;
	int       component_id;
} ALF_CORR_DATA;
/* ALF_AVS2(END) */

#ifdef AML
typedef struct avs3_frame_s {
#if 0
	int32_t imgcoi_ref;
	byte **referenceFrame[3];
	int32_t **refbuf;
	int32_t ** *mvbuf;
	double saorate[NUM_SAO_COMPONENTS];
	byte ** *ref;

	int32_t imgtr_fwRefDistance;
	int32_t refered_by_others;
	int32_t is_output;
#if M3480_TEMPORAL_SCALABLE
	int32_t temporal_id;          //temporal level setted in configure file
#endif
	byte **oneForthRefY;
#if FIX_MAX_REF
	int32_t ref_poc[MAXREF];
#else
	int32_t ref_poc[4];
#endif
#endif
	int index;
	int used;
	u32 mmu_alloc_flag;
	u32 lcu_size_log2;
	u32 header_adr;
	u32 header_dw_adr;
	u32 mc_y_adr;
	u32 mc_u_v_adr;
	u32 mc_canvas_y;
	u32 mc_canvas_u_v;
	u32 mpred_mv_wr_start_addr;
	u8 bg_flag;
	u32 refered_by_others;
	u32 is_output;
	int vf_ref;
	u8 slice_type;
	int list0_num_refp;
	int list0_ptr[MAX_NUM_REF_PICS];
	int backend_ref;
#ifdef NEW_FB_CODE
	unsigned char back_done_mark;
	//unsigned char flush_mark;
#endif
#ifdef NEW_FRONT_BACK_CODE
	int list0_index[MAX_NUM_REF_PICS];
	int list1_num_refp;
	int list1_index[MAX_NUM_REF_PICS];
	int width;
	int height;
	int depth;
#endif
#ifdef AML
	int error_mark;
	int32_t decoded_lcu;

	unsigned long cma_alloc_addr;
	int buf_size;
	int BUF_index;
	int lcu_total;
	int comp_body_size;
	uint32_t dw_y_adr;
	uint32_t dw_u_v_adr;
	unsigned long dw_header_adr;
	int y_canvas_index;
	int uv_canvas_index;
	struct canvas_config_s canvas_config[2];
	int double_write_mode;

	u32 stream_offset;
	u32 pts;
	u64 pts64;
	/* picture qos information*/
	struct vframe_qos_s vqos;

	u32 hw_decode_time;
	u32 frame_size; // For frame base mode
	char *cuva_data_buf;
	int  cuva_data_size;
	u64 timestamp;
	uint32_t luma_size;
	uint32_t chroma_size;
	bool in_dpb;
	u64 time;
	int is_display;
	s32 poc;
	u32 hw_front_decode_time;
	struct vdec_info vinfo;
	u32 stream_size; // For stream base mode
#endif
	u32 tw_y_adr;
	u32 tw_u_v_adr;
	u32 luma_size_ex;
	u32 chroma_size_ex;

	//int tw_y_canvas_index;
	//int tw_uv_canvas_index;
	struct canvas_config_s tw_canvas_config[2];
	u32 triple_write_mode;
	uint32_t luma_size_tw;
	uint32_t chroma_size_tw;
} avs3_frame_t;
#endif
/* picture store structure */
typedef struct _COM_PIC
{
	/* Start address of Y component (except padding) */
	pel             *y;
	/* Start address of U component (except padding)  */
	pel             *u;
	/* Start address of V component (except padding)  */
	pel             *v;
#if EVS_UBVS_MODE
	/* Start address of U component format 444 (except padding)  */
	pel             *u444;
	/* Start address of V component format 444 (except padding)  */
	pel             *v444;
#endif
	/* Stride of luma picture */
	int              stride_luma;
	/* Stride of chroma picture */
	int              stride_chroma;
	/* Width of luma picture */
	int              width_luma;
	/* Height of luma picture */
	int              height_luma;
	/* Width of chroma picture */
	int              width_chroma;
	/* Height of chroma picture */
	int              height_chroma;
	/* padding size of luma */
	int              padsize_luma;
	/* padding size of chroma */
	int              padsize_chroma;
	/* image buffer */
	COM_IMGB        *imgb;
	/* decoding temporal reference of this picture */
	int              dtr;
	/* presentation temporal reference of this picture */
	int              ptr;
	int              picture_output_delay;
	/* 0: not used for reference buffer, reference picture type */
	u8               is_ref;
	/* needed for output? */
	u8               need_for_out;
	/* scalable layer id */
	u8               temporal_id;
	s16            (*map_mv)[REFP_NUM][MV_D];
	s8             (*map_refi)[REFP_NUM];
#if ETMVP || SUB_TMVP || AWP
	u32              list_ptr[REFP_NUM][MAX_NUM_REF_PICS];
#else
	u32              list_ptr[MAX_NUM_REF_PICS];
#endif
#ifdef AML
	avs3_frame_t buf_cfg;
#endif
} COM_PIC;

#if LIBVC_ON
#define MAX_NUM_COMPONENT                   3
#define MAX_NUM_LIBPIC                      16
#define MAX_CANDIDATE_PIC                   256
#define MAX_DIFFERENCE_OF_RLPIC_AND_LIBPIC  1.7e+308; ///< max. value of Double-type value

typedef struct _PIC_HIST_FEATURE
{
	int num_component;
	int num_of_hist_interval;
	int length_of_interval;
	int * list_hist_feature[MAX_NUM_COMPONENT];
} PIC_HIST_FEATURE;

// ====================================================================================================================
// Information of libpic
// ====================================================================================================================

typedef struct _LibVCData
{
	int num_candidate_pic;
	int list_poc_of_candidate_pic[MAX_CANDIDATE_PIC]; // for now, the candidate set contain only pictures from input sequence.
	COM_IMGB * list_candidate_pic[MAX_CANDIDATE_PIC];
	PIC_HIST_FEATURE list_hist_feature_of_candidate_pic[MAX_CANDIDATE_PIC];

	int num_lib_pic;
	int list_poc_of_libpic[MAX_NUM_LIBPIC];

	COM_PIC * list_libpic_outside[MAX_NUM_LIBPIC];
	int num_libpic_outside;
	int list_library_index_outside[MAX_NUM_LIBPIC];

	int num_RLpic;
	int list_poc_of_RLpic[MAX_CANDIDATE_PIC];
	int list_libidx_for_RLpic[MAX_CANDIDATE_PIC]; //the idx of library picture instead of poc of library picture in origin sequence is set as new poc.

	int bits_dependencyFile;
	int bits_libpic;

	int library_picture_enable_flag;
#if IPPPCRR
#if LIB_PIC_UPDATE
	int lib_pic_update;
	int update;
	int countRL;
	int encode_skip;
	int end_of_intra_period;
#else
	int first_pic_as_libpic;
#endif
#endif
	int is_libpic_processing;
	int is_libpic_prepared;
#if CRR_ENC_OPT_CFG
	int lib_in_l0, lib_in_l1, pb_ref_lib, rl_ref_lib, max_list_refnum;
	int libpic_idx;
#endif

} LibVCData;
#endif

/*****************************************************************************
* picture buffer allocator
*****************************************************************************/
typedef struct _PICBUF_ALLOCATOR
{
	/* width */
	int              width;
	/* height */
	int              height;
	/* pad size for luma */
	int              pad_l;
	/* pad size for chroma */
	int              pad_c;
	/* arbitrary data, if needs */
	int              ndata[4];
	/* arbitrary address, if needs */
	void            *pdata[4];
} PICBUF_ALLOCATOR;

/*****************************************************************************
* picture manager for DPB in decoder and RPB in encoder
*****************************************************************************/
typedef struct _COM_PM
{
	/* picture store (including reference and non-reference) */
	COM_PIC          *pic[MAX_PB_SIZE];
	/* address of reference pictures */
	COM_PIC          *pic_ref[MAX_NUM_REF_PICS];
	/* maximum reference picture count */
	int               max_num_ref_pics;
	/* current count of available reference pictures in PB */
	int               cur_num_ref_pics;
	/* number of reference pictures */
	int               num_refp[REFP_NUM];
	/* next output POC */
	int               ptr_next_output;
	/* POC increment */
	int               ptr_increase;
	/* max number of picture buffer */
	int               max_pb_size;
	/* current picture buffer size */
	int               cur_pb_size;
	/* address of leased picture for current decoding/encoding buffer */
	COM_PIC          *pic_lease;
	/* picture buffer allocator */
	PICBUF_ALLOCATOR  pa;
#if LIBVC_ON
	/* information for LibVC */
	LibVCData        *libvc_data;
	int               is_library_buffer_empty;
	COM_PIC          *pb_libpic; //buffer for libpic
	int               cur_libpb_size;
	int               pb_libpic_library_index;
#endif
	/*AML*/
	void              *hw;
	spinlock_t        pm_lock;
	/**/
} COM_PM;

/* reference picture structure */
typedef struct _COM_REFP
{
	/* address of reference picture */
	COM_PIC         *pic;
	/* PTR of reference picture */
	int              ptr;
	s16            (*map_mv)[REFP_NUM][MV_D];
	s8             (*map_refi)[REFP_NUM];
#if ETMVP || SUB_TMVP || AWP
	u32            (*list_ptr)[MAX_NUM_REF_PICS];
#else
	u32             *list_ptr;
#endif
#if LIBVC_ON
	int              is_library_picture;
#endif
} COM_REFP;

typedef struct _COM_MOTION
{
	s16              mv[REFP_NUM][MV_D];
	s8               ref_idx[REFP_NUM];
} COM_MOTION;

/*****************************************************************************
* chunk header
*****************************************************************************/
typedef struct _COM_CNKH
{
	/* version: 3bit */
	int              ver;
	/* chunk type: 4bit */
	int              ctype;
	/* broken link flag: 1bit(should be zero) */
	int              broken;
} COM_CNKH;

/*****************************************************************************
* sequence header
*****************************************************************************/
typedef struct _COM_SQH
{
	u8               profile_id;                 /* 8 bits */
	u8               level_id;                   /* 8 bits */
	u8               progressive_sequence;       /* 1 bit  */
	u8               field_coded_sequence;       /* 1 bit  */
#if LIBVC_ON
	u8               library_stream_flag;     /* 1 bit  */
	u8               library_picture_enable_flag;     /* 1 bit  */
	u8               duplicate_sequence_header_flag;     /* 1 bit  */
#endif
	u8               chroma_format;              /* 2 bits */
	u8               encoding_precision;         /* 3 bits */
	u8               output_reorder_delay;       /* 5 bits */
	u8               sample_precision;           /* 3 bits */
	u8               aspect_ratio;               /* 4 bits */
	u8               frame_rate_code;            /* 4 bits */
	u32              bit_rate_lower;             /* 18 bits */
	u32              bit_rate_upper;             /* 18 bits */
	u8               low_delay;                  /* 1 bit */
	u8               temporal_id_enable_flag;    /* 1 bit */
	u32              bbv_buffer_size;            /* 18 bits */
	u8               max_dpb_size;               /* 4 bits */
	int              horizontal_size;            /* 14 bits */
	int              vertical_size;              /* 14 bits */
	u8               log2_max_cu_width_height;   /* 3 bits */
	u8               min_cu_size;
	u8               max_part_ratio;
	u8               max_split_times;
	u8               min_qt_size;
	u8               max_bt_size;
	u8               max_eqt_size;
	u8               max_dt_size;
#if HLS_RPL
	int              rpl1_index_exist_flag;
	int              rpl1_same_as_rpl0_flag;
	COM_RPL          rpls_l0[MAX_NUM_RPLS];
	COM_RPL          rpls_l1[MAX_NUM_RPLS];
	int              rpls_l0_num;
	int              rpls_l1_num;
	int              num_ref_default_active_minus1[2];
#endif
#if IPCM
	int              ipcm_enable_flag;
#endif
	u8               amvr_enable_flag;
#if IBC_ABVR
	u8               abvr_enable_flag;
#endif
#if USE_SP
	int              sp_enable_flag;
#if EVS_UBVS_MODE
	int              evs_ubvs_enable_flag;
#endif
#endif
	int              umve_enable_flag;
#if UMVE_ENH
	int              umve_enh_enable_flag;
#endif
#if AWP
	int              awp_enable_flag;
#endif
#if AWP_MVR
	int              awp_mvr_enable_flag;
#endif
#if ETMVP
	int              etmvp_enable_flag;
#endif
#if SUB_TMVP
	int              sbtmvp_enable_flag;
#endif
#if MIPF
	int              mipf_enable_flag;
#endif
#if DBK_SCC
	int              loop_filter_type_enable_flag;
#endif
#if DBR
	int              dbr_enable_flag;
#endif
	int              ipf_enable_flag;
#if EXT_AMVR_HMVP
	int              emvr_enable_flag;
#endif
	u8               affine_enable_flag;
#if ASP
	u8               asp_enable_flag;
#endif
#if AFFINE_UMVE
	u8               affine_umve_enable_flag;
#endif
#if SMVD
	u8               smvd_enable_flag;
#endif
#if BIO
	u8               bio_enable_flag;
#endif
#if BGC
	u8               bgc_enable_flag;
#endif
#if DMVR
	u8               dmvr_enable_flag;
#endif
#if INTERPF
	u8               interpf_enable_flag;
#endif
#if MVAP
	u8               mvap_enable_flag;
	u8               num_of_mvap_cand;
#endif
#if DT_PARTITION
	u8               dt_intra_enable_flag;
#endif
	u8               num_of_hmvp_cand;
#if IBC_BVP
	u8               num_of_hbvp_cand;
#endif
#if SIBC
	u8               sibc_enable_flag;
#endif
#if TSCPM
	u8               tscpm_enable_flag;
#if ENHANCE_TSPCM
	u8               enhance_tscpm_enable_flag;
#endif
#if PMC
	u8               pmc_enable_flag;
#endif
#endif
#if IPF_CHROMA
	u8               chroma_ipf_enable_flag;
#endif
#if IIP
	u8               iip_enable_flag;
#endif
#if FIMC
	u8               fimc_enable_flag;
#endif
#if IST
	u8               ist_enable_flag;
#endif
#if ISTS
	u8               ists_enable_flag;
#endif
#if TS_INTER
	u8               ts_inter_enable_flag;
#endif
#if EST
	u8               est_enable_flag;
#endif
#if ST_CHROMA
	u8               st_chroma_enable_flag;
#endif
#if SRCC
	u8               srcc_enable_flag;
#endif
#if CABAC_MULTI_PROB
	u8               mcabac_enable_flag;
#endif
#if EIPM
	u8               eipm_enable_flag;
#endif
	u8               sample_adaptive_offset_enable_flag;
#if ESAO
	u8               esao_enable_flag;
#endif
#if CCSAO
	u8               ccsao_enable_flag;
#endif
#if ALF_SHAPE || ALF_IMP
	u8               adaptive_filter_shape_enable_flag;
#endif
	u8               adaptive_leveling_filter_enable_flag;
	u8               secondary_transform_enable_flag;
	u8               position_based_transform_enable_flag;
#if SBT
	u8               sbt_enable_flag;
#endif

	u8               wq_enable;
	u8               seq_wq_mode;
	u8               wq_4x4_matrix[16];
	u8               wq_8x8_matrix[64];
#if PATCH
	u8               patch_stable;
	u8               cross_patch_loop_filter;
	u8               patch_ref_colocated;
	u8               patch_uniform;
	u8               patch_width_minus1;
	u8               patch_height_minus1;
	u8               patch_columns;
	u8               patch_rows;
	int              column_width[64];
	int              row_height[32];
#endif
#if USE_IBC
	u8               ibc_flag;
#endif
#if ESAO
	u8              pic_esao_on[3];
	u8              esao_adaptive_param[3];
	u8              esao_lcu_enable[3];
#endif
} COM_SQH;


/*****************************************************************************
* picture header
*****************************************************************************/
typedef struct _COM_PIC_HEADER
{
	/* info from sqh */
	u8  low_delay;
	/* decoding temporal reference */
#if HLS_RPL
	s32 poc;
	int              rpl_l0_idx;         //-1 means this slice does not use RPL candidate in SPS for RPL0
	int              rpl_l1_idx;         //-1 means this slice does not use RPL candidate in SPS for RPL1
	COM_RPL          rpl_l0;
	COM_RPL          rpl_l1;
	u8               num_ref_idx_active_override_flag;
	u8               ref_pic_list_sps_flag[2];
#endif
	u32              dtr;
	u8               slice_type;
	u8               temporal_id;
	u8               loop_filter_disable_flag;
	u32              bbv_delay;
	u16              bbv_check_times;

#if DBK_SCC
	u8               loop_fitler_type;
#endif
	u8               loop_filter_parameter_flag;
	int              alpha_c_offset;
	int              beta_offset;

	u8               chroma_quant_param_disable_flag;
	s8               chroma_quant_param_delta_cb;
	s8               chroma_quant_param_delta_cr;

	/*Flag and coeff for ALF*/
	int              tool_alf_on;
#if ALF_SHAPE || ALF_IMP
	int              tool_alf_shape_on;
#endif
	int             *pic_alf_on;
	ALF_PARAM       **alf_picture_param;
#if DBR
	DBR_PARAM        ph_dbr_param;
#endif

	int              fixed_picture_qp_flag;
#if CUDQP
	u8               cu_delta_qp_flag;
	u8               cu_qp_group_size;
	int              cu_qp_group_area_size;
#endif
	int              random_access_decodable_flag;
	int              time_code_flag;
	int              time_code;
	int              decode_order_index;
	int              picture_output_delay;
	int              progressive_frame;
	int              picture_structure;
	int              top_field_first;
	int              repeat_first_field;
	int              top_field_picture_flag;
	int              picture_qp;
	int              affine_subblock_size_idx;
#if LIBVC_ON
	int              is_RLpic_flag;  // only reference libpic
	int              library_picture_index;
#endif
	int              pic_wq_enable;
	int              pic_wq_data_idx;
	int              wq_param;
	int              wq_model;
	int              wq_param_vector[6];
	u8               wq_4x4_matrix[16];
	u8               wq_8x8_matrix[64];
#if USE_IBC
	u8               ibc_flag;
#endif
#if USE_SP
	u8               sp_pic_flag;
#if EVS_UBVS_MODE
	u8               evs_ubvs_pic_flag;
#endif
#endif
#if FIMC
	u8               fimc_pic_flag;
#endif
#if ISTS
	u8               ph_ists_enable_flag;
#endif
#if TS_INTER
	u8               ph_ts_inter_enable_flag;
#endif
#if AWP
	u8               ph_awp_refine_flag;
#endif
#if AWP || FAST_LD
	int              ph_poc[REFP_NUM][MAX_NUM_ACTIVE_REF_FRAME];
#endif
#if ENC_ME_IMP
	BOOL             is_lowdelay;
#endif
#if FAST_LD
	int              l1idx_to_l0idx[MAX_NUM_ACTIVE_REF_FRAME];
#endif
#if UMVE_ENH
	u8               umve_set_flag;
#endif
#if EPMC
	u8               ph_epmc_model_flag;
#endif
#if ESAO
	u8               pic_esao_on[N_C];
	u8               esao_adaptive_param[N_C];
	u8               esao_luma_type;            //0:17 1 :9
	u8               esao_lcu_enable[N_C];      //lcu mode control flag
	int              esao_chroma_band_length[2];
	int              esao_chroma_start_band[2];
	int              esao_chroma_band_flag[2];
#if ESAO_PH_SYNTAX
	ESAO_BLK_PARAM   pic_esao_params[N_C];
#endif
#endif
#if CCSAO
	u8               pic_ccsao_on  [N_C-1];
	u8               ccsao_lcu_ctrl[N_C-1];
#if CCSAO_ENHANCEMENT
	u8               ccsao_set_num   [N_C-1];
	u8               ccsao_type      [N_C-1][CCSAO_SET_NUM];
	u8               ccsao_band_num  [N_C-1][CCSAO_SET_NUM];
	u8               ccsao_band_num_c[N_C-1][CCSAO_SET_NUM];
#else
	u8               ccsao_type    [N_C-1];
	u8               ccsao_band_num[N_C-1];
#endif
#if CCSAO_PH_SYNTAX
	CCSAO_BLK_PARAM  pic_ccsao_params[N_C-1];
#endif
#endif
	int g_DOIPrev;
	int g_CountDOICyCleTime;
} COM_PIC_HEADER;

typedef struct _COM_SH_EXT
{
	u8               slice_sao_enable[N_C];
	u8               fixed_slice_qp_flag;
	u8               slice_qp;
} COM_SH_EXT;

#if TB_SPLIT_EXT
typedef struct _COM_PART_INFO
{
	u8 num_sub_part;
	int sub_x[MAX_NUM_PB]; //sub part x, y, w and h
	int sub_y[MAX_NUM_PB];
	int sub_w[MAX_NUM_PB];
	int sub_h[MAX_NUM_PB];
	int sub_scup[MAX_NUM_PB];
} COM_PART_INFO;
#endif

#if CUDQP
typedef struct _COM_CU_QP_GROUP
{
	int num_delta_qp;
	int cu_qp_group_x;
	int cu_qp_group_y;
	int pred_qp;
	//encoder only
	int target_qp_for_qp_group;
	int qp_group_width;
	int qp_group_height;
} COM_CU_QP_GROUP;
#endif

typedef struct _COM_POS
{
	u8 a;    // 1/4 position
	u8 b;    // 2/4 position
	u8 c;    // 3/4 position
} COM_POS;

typedef struct _COM_POS_INFO
{
	COM_POS x;   // x dimension
	COM_POS y;   // y dimension
} COM_POS_INFO;
/*****************************************************************************
* user data types
*****************************************************************************/
#define COM_UD_PIC_SIGNATURE              0x10
#define COM_UD_END                        0xFF

#if USE_SP
/*****************************************************************************
* SP mode structure
*****************************************************************************/
typedef struct _COM_SP_INFO
{
	u8  is_matched;
	u16 length;
	s16 offset_x;
	s16 offset_y;
#if SP_SLR
	int match_dict[4];
	pel pixel[4][N_C];
#else
	pel pixel[N_C];
#endif
#if !SP_CODE_TEXT_BUGFIX
	u8  sp_length_type;
#endif
	u8  n_recent_flag;
	s8  n_recent_idx;
#if SP_SVP
	s16 svp_x;
	s16 svp_y;
	s8  n_offset_num;
#endif
} COM_SP_INFO;
#if EVS_UBVS_MODE
typedef struct _COM_SP_PIX
{
	pel Y;
	pel U;
	pel V;
}COM_SP_PIX;
typedef struct _COM_SP_EVS_INFO
{
	u8  is_matched;
	u16 length;
	pel pixel[N_C];
	u8  match_type;
	u8  srb_index;
	u16 srb_bits;
	u16 srb_dist;
	u8  esc_flag;
	int pos;
	s8  valid_flag;
	u8  allcomp_adr_update_flag;
}COM_SP_EVS_INFO;
#endif
#endif
/*****************************************************************************
* for binary and triple tree structure
*****************************************************************************/
typedef enum _SPLIT_MODE
{
	NO_SPLIT        = 0,
	SPLIT_BI_VER    = 1,
	SPLIT_BI_HOR    = 2,
#if EQT
	SPLIT_EQT_VER   = 3,
	SPLIT_EQT_HOR   = 4,
	SPLIT_QUAD      = 5,
#else
	SPLIT_QUAD      = 3,
#endif
	NUM_SPLIT_MODE
} SPLIT_MODE;

typedef enum _SPLIT_DIR
{
	SPLIT_VER = 0,
	SPLIT_HOR = 1,
	SPLIT_QT = 2,
} SPLIT_DIR;

#if MODE_CONS
typedef enum _CONS_PRED_MODE
{
	NO_MODE_CONS = 0,
	ONLY_INTER = 1,
	ONLY_INTRA = 2,
} CONS_PRED_MODE;
#endif

#if CHROMA_NOT_SPLIT
typedef enum _TREE_STATUS
{
	TREE_LC = 0,
	TREE_L  = 1,
	TREE_C  = 2,
} TREE_STATUS;
#endif

typedef enum _CHANNEL_TYPE
{
	CHANNEL_LC = 0,
	CHANNEL_L  = 1,
	CHANNEL_C  = 2,
#if PMC || EPMC
	CHANNEL_U  = 3,
	CHANNEL_V  = 4,
#endif
} CHANNEL_TYPE;

#if TB_SPLIT_EXT
typedef enum _PART_SIZE
{
	SIZE_2Nx2N,           ///< symmetric partition,  2Nx2N
	SIZE_2NxnU,           ///< asymmetric partition, 2Nx( N/2) + 2Nx(3N/2)
	SIZE_2NxnD,           ///< asymmetric partition, 2Nx(3N/2) + 2Nx( N/2)
	SIZE_nLx2N,           ///< asymmetric partition, ( N/2)x2N + (3N/2)x2N
	SIZE_nRx2N,           ///< asymmetric partition, (3N/2)x2N + ( N/2)x2N
	SIZE_NxN,             ///< symmetric partition, NxN
	SIZE_2NxhN,           ///< symmetric partition, 2N x 0.5N
	SIZE_hNx2N,           ///< symmetric partition, 0.5N x 2N
	NUM_PART_SIZE
} PART_SIZE;
#endif

typedef enum _BLOCK_SHAPE
{
	NON_SQUARE_18,
	NON_SQUARE_14,
	NON_SQUARE_12,
	SQUARE,
	NON_SQUARE_21,
	NON_SQUARE_41,
	NON_SQUARE_81,
	NUM_BLOCK_SHAPE,
} BLOCK_SHAPE;

typedef enum _CTX_NEV_IDX
{
	CNID_SKIP_FLAG,
	CNID_PRED_MODE,
	CNID_AFFN_FLAG,
	NUM_CNID,
} CTX_NEV_IDX;

#if AWP
typedef enum _AWP_ANGLE_RATIO
{
	AWP_ANGLE_1,  // 1:1
	AWP_ANGLE_2,  // 1:2 2:1
	AWP_ANGLE_4,  // 1:4 4:1
	AWP_ANGLE_8,  // 1:8 8:1
	AWP_ANGLE_16,
	AWP_ANGLE_32,
	AWP_ANGLE_64,
	AWP_ANGLE_HOR,
	AWP_ANGLE_VER
} AWP_ANGLE_RATIO;
#endif

#if DMVR
#define DMVR_PAD_LENGTH                                        2
#define EXTRA_PIXELS_FOR_FILTER                                7 // Maximum extraPixels required for final MC based on fiter size
#define PAD_BUFFER_STRIDE                               ((MAX_CU_SIZE + EXTRA_PIXELS_FOR_FILTER + (DMVR_ITER_COUNT * 2)))
static const int NTAPS_LUMA = 8; ///< Number of taps for luma
static const int NTAPS_CHROMA = 4; ///< Number of taps for chroma
#endif

/*****************************************************************************
* mode decision structure
*****************************************************************************/
typedef struct _COM_MODE
{
	/* CU position X in a frame in SCU unit */
	int            x_scu;
	/* CU position Y in a frame in SCU unit */
	int            y_scu;

	/* depth of current CU */
	int            cud;

	/* width of current CU */
	int            cu_width;
	/* height of current CU */
	int            cu_height;
	/* log2 of cu_width */
	int             cu_width_log2;
	/* log2 of cu_height */
	int             cu_height_log2;
	/* position of CU */
	int            x_pos;
	int            y_pos;
	/* CU position in current frame in SCU unit */
	int            scup;

	int  cu_mode;
#if TB_SPLIT_EXT
	//note: DT can apply to intra CU and only use normal amvp for inter CU (no skip, direct, amvr, affine, hmvp)
	int  pb_part;
	int  tb_part;
	COM_PART_INFO  pb_info;
	COM_PART_INFO  tb_info;
#endif
#if SBT
	u8   sbt_info;
	int  sbt_hor_trans;
	int  sbt_ver_trans;
#endif

	pel  rec[N_C][MAX_CU_DIM];
	s16  coef[N_C][MAX_CU_DIM];
	pel  pred[N_C][MAX_CU_DIM];


#if TB_SPLIT_EXT
	int  num_nz[MAX_NUM_TB][N_C];
#else
	int  num_nz[N_C];
#endif

	/* reference indices */
	s8   refi[REFP_NUM];

	/* MVR indices */
	u8   mvr_idx;
#if INTERPF
	u8   inter_filter_flag;
#endif
#if BGC
	u8   bgc_flag;
	u8   bgc_idx;
#if UMVE_ENH
	s8   bgc_flag_cands[MAX_SKIP_NUM + UMVE_MAX_REFINE_NUM_SEC_SET * UMVE_BASE_NUM];
	s8   bgc_idx_cands[MAX_SKIP_NUM + UMVE_MAX_REFINE_NUM_SEC_SET * UMVE_BASE_NUM];
#else
	s8   bgc_flag_cands[MAX_SKIP_NUM + UMVE_MAX_REFINE_NUM * UMVE_BASE_NUM];
	s8   bgc_idx_cands[MAX_SKIP_NUM + UMVE_MAX_REFINE_NUM * UMVE_BASE_NUM];
#endif
#endif
	u8   umve_flag;
	u8   umve_idx;
	u8   skip_idx;
#if EXT_AMVR_HMVP
	u8   mvp_from_hmvp_flag;
#endif
#if IBC_BVP
	u8   cbvp_idx;
#if CBVP_LIST_SIMP
	s8   cnt_hbvp_cands;
#endif
#endif

#if SP_SVP
	u8   svp_idx;
#endif

#if AFFINE_UMVE
	u8   affine_umve_flag;
	s8   affine_umve_idx[VER_NUM];
	s8   best_affine_merge_index;
#endif

	/* mv difference */
	s16  mvd[REFP_NUM][MV_D];
	/* mv */
	s16  mv[REFP_NUM][MV_D];

	u8   affine_flag;
	CPMV affine_mv[REFP_NUM][VER_NUM][MV_D];
	s16  affine_mvd[REFP_NUM][VER_NUM][MV_D];

#if AWP
	/* awp flag*/
	u8   awp_flag;
	/* awp cand idx*/
	u8   awp_idx0;
	u8   awp_idx1;
	/* awp mv & refidx*/
	s16  awp_mv0[REFP_NUM][MV_D];
	s16  awp_mv1[REFP_NUM][MV_D];
	s8   awp_refi0[REFP_NUM];
	s8   awp_refi1[REFP_NUM];
#endif

#if AWP_MVR
	u8   awp_mvr_flag0;
	u8   awp_mvr_idx0;
	u8   awp_mvr_flag1;
	u8   awp_mvr_idx1;
#endif

#if SMVD
	u8   smvd_flag;
#endif

#if DMVR
	pel  dmvr_template[MAX_CU_DIM];
	pel  dmvr_ref_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + (2 * (DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT)) * (MAX_CU_SIZE + (2 * (DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT))];
	u8   dmvr_enable;
	pel  dmvr_padding_buf[2][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE];
#endif

#if ETMVP
	u8   etmvp_flag;
#endif

	/* intra prediction mode */
#if TB_SPLIT_EXT
	u8   mpm[MAX_NUM_PB][2];
	s8   ipm[MAX_NUM_PB][2];
#else
	u8   mpm[2];
	s8   ipm[2];
#endif
#if USE_IBC
	// ibc flag for MODE_IBC
	u8   ibc_flag;
#endif
#if IBC_ABVR
	u8   bvr_idx;
#endif
#if SIBC
	u8   sibc_flag;
#endif
#if IIP
	u8   iip_flag;
#endif
	u8   ipf_flag;
	/* intra transform type */
	u8   slice_type;
#if IST
	u8   ist_tu_flag;
#endif
#if ISTS
	u8   ph_ists_enable_flag;
#endif
#if TS_INTER
	u8   ph_ts_inter_enable_flag;
#endif
#if AWP
	u8   ph_awp_refine_flag;
#endif
#if EST
	u8   est_flag;
#endif
#if ST_CHROMA
	u8   st_chroma_flag;
#endif
#if USE_SP
	u8   sp_flag;
	u8   sp_copy_direction;
	u16  sub_string_no;
	COM_SP_INFO string_copy_info[SP_STRING_INFO_NO];
	double cur_bst_rdcost;
	u8   is_sp_pix_completed;
#endif
#if EVS_UBVS_MODE
	u8                   cs2_flag;
	u8                   evs_copy_direction;
	int                  evs_sub_string_no;
	COM_SP_EVS_INFO*     evs_str_copy_info; //evs_str_copy_info[SP_STRING_INFO_NO];
	int                  unpred_pix_num;
	COM_SP_PIX*          unpred_pix_info;   //unpred_pix_info[SP_STRING_INFO_NO];
	u8                   equal_val_str_present_flag;
	u8                   unpredictable_pix_present_flag;
	s16*                 p_SRB[N_C];
	u8                   pvbuf_size;
	u8*                  pvbuf_reused_flag;
	u8                   all_comp_flag[MAX_SRB_SIZE];//1:LUMA AND CHROMA 0: ONLY LUMA
	u8                   all_comp_pre_flag[MAX_SRB_PRED_SIZE];
	u16                  temp_m_pvbuf[2][MAX_SRB_SIZE];
	s16*                 p_SRB_prev[N_C];
	u8                   pvbuf_size_prev;
	u16                  m_pvbuf_prev[2][MAX_SRB_PRED_SIZE];
	u16                  m_pvbuf[2][MAX_SRB_SIZE];
	u16                  m_pvbuf_new[2][MAX_SRB_PRED_SIZE];
	u8                   m_dpb_idx[MAX_SRB_SIZE];
	u8                   m_dpb_idx_prev[MAX_SRB_PRED_SIZE];
#endif
} COM_MODE;

/*****************************************************************************
* map structure
*****************************************************************************/
typedef struct _COM_MAP
{
	/* SCU map for CU information */
	u32                    *map_scu;
	/* LCU split information */
	s8                     (*map_split)[MAX_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU];
	/* decoded motion vector for every blocks */
	s16                    (*map_mv)[REFP_NUM][MV_D];
	/* reference frame indices */
	s8                     (*map_refi)[REFP_NUM];
	/* intra prediction modes */
	s8                     *map_ipm;
	u32                    *map_cu_mode;
#if TB_SPLIT_EXT
	u32                    *map_pb_tb_part;
#endif
	s8                     *map_depth;
	s8                     *map_delta_qp;
	s8                     *map_patch_idx;
#if USE_SP
	u8                     *map_usp;
#endif
#if PATCH
	u32                    *map_scu_temp;
	u32                    *map_cu_mode_temp;
	s16                    (*map_mv_temp)[REFP_NUM][MV_D];
	s8                     (*map_refi_temp)[REFP_NUM];
#if USE_SP
	u8                     *map_usp_temp;
#endif
#endif
} COM_MAP;

/*****************************************************************************
* common info
*****************************************************************************/
typedef struct _COM_INFO
{
	/* current chunk header */
	COM_CNKH                cnkh;

	/* current picture header */
	COM_PIC_HEADER          pic_header;

	/* current slice header */
	COM_SH_EXT              shext;

	/* sequence header */
	COM_SQH                 sqh;

	/* decoding picture width */
	int                     pic_width;
	/* decoding picture height */
	int                     pic_height;
	/* maximum CU width and height */
	int                     max_cuwh;
	/* log2 of maximum CU width and height */
	int                     log2_max_cuwh;

	/* picture width in LCU unit */
	int                     pic_width_in_lcu;
	/* picture height in LCU unit */
	int                     pic_height_in_lcu;

	/* picture size in LCU unit (= w_lcu * h_lcu) */
	int                     f_lcu;
	/* picture width in SCU unit */
	int                     pic_width_in_scu;
	/* picture height in SCU unit */
	int                     pic_height_in_scu;
	/* picture size in SCU unit (= pic_width_in_scu * h_scu) */
	int                     f_scu;

	int                     bit_depth_internal;
	int                     bit_depth_input;
	int                     qp_offset_bit_depth;
#if ASP
	BOOL                    skip_me_asp;
	BOOL                    skip_umve_asp;
#endif
#if BGC
	pel                    *pred_tmp;
#endif
} COM_INFO;

typedef enum _MSL_IDX
{
	MSL_SKIP,  //skip
	MSL_MERGE,  //merge or direct
	MSL_LIS0,  //list 0
	MSL_LIS1,  //list 1
	MSL_BI,    //bi pred
	NUM_MODE_SL,
} MSL_IDX;
/*****************************************************************************
* patch info
*****************************************************************************/
#if PATCH
typedef struct _PATCH_INFO
{
	int                     stable;
	int                     cross_patch_loop_filter;
	int                     ref_colocated;
	int                     uniform;
	int                     height;
	int                     width;
	int                     columns;
	int                     rows;
	int                     idx;
	/*patch's location(up left) in pixel*/
	int                     x_pel;
	int                     y_pel;
	/*all the cu in a patch are skip_mode*/
	int                     skip_patch_flag;
	/*pointer the width of each patch*/
	int                    *width_in_lcu;
	int                    *height_in_lcu;
	/*count as the patch*/
	int                     x_pat;
	int                     y_pat;
	/*define the boundary of a patch*/
	int                     left_pel;
	int                     right_pel;
	int                     up_pel;
	int                     down_pel;
	/*patch_sao_enable_flag info of all patches*/
	s8                     *patch_sao_enable;
} PATCH_INFO;
#endif
typedef enum _TRANS_TYPE
{
	DCT2,
	DCT8,
	DST7,
	NUM_TRANS_TYPE
} TRANS_TYPE;

#if FIMC
typedef struct _COM_CNTMPM
{
	u32 freqT [CNTMPM_TABLE_LENGTH]; // freqT [ mode ]
	u8  modeT [2];                   // modeT [ order]
} COM_CNTMPM;
#endif

#if PMC || EPMC
#define IS_RIGHT_CBF_U(cbf_u)               ((cbf_u) > 0)
#define V_QP_OFFSET                         1
#endif

#if EPMC
#define MOD_IDX                             2        //2 is -2
#define MOD2_IDX                            1        //1 is -0.5
#define TH_CBCR                             -0.10
#endif

#if IIP
#define MAX_IIP_BLK                       4096 //the number is included
#define MIN_IIP_BLK                       64 //the number is included
#define STNUM                             5 // anchor is 3
#else
#define STNUM                             3 // anchor is 3
#endif

#if ASP
#define ASP_SHIFT                          3
#define DMV_BUF_SIZE                       8 * 8
#endif
#if 0
#include "com_tbl.h"
#include "com_util.h"
#include "com_recon.h"
#include "com_ipred.h"
#include "com_tbl.h"
#include "com_itdq.h"
#include "com_picman.h"
#include "com_mc.h"
#include "com_img.h"
#endif

#endif /* _COM_DEF_H_ */
