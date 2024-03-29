/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 * @file    bodydef.h
 *
 * @brief   CIMaX+ USB Driver for linux based operating systems.
 *
 * bruno Tonelli   <bruno.tonelli@smardtv.com>
 *                          & Franck Descours <franck.descours@smardtv.com>
 *                            for SmarDTV France, La Ciotat
 */
#ifndef __BODYDEF_H
#define __BODYDEF_H

struct reg_s {
	__u8     RegisterName[50];
	__u16    RegAddr;
};

/*=======================================================================
 Input/Output Ports and Data Direction Registers
=======================================================================*/
struct reg_s cimax_reg_map[] = {
	{"BUFFIN_CFG"                      , 0x0000},
	{"BUFFIN_ADDR_LSB"                 , 0x0001},
	{"BUFFIN_ADDR_MSB"                 , 0x0002},
	{"BUFFIN_DATA"                     , 0x0003},
	{"BUFFOUT_CFG"                     , 0x0004},
	{"BUFFOUT_ADDR_LSB"                , 0x0005},
	{"BUFFOUT_ADDR_MSB"                , 0x0006},
	{"BUFFOUT_DATA"                    , 0x0007},
	{"BOOT_Key"                        , 0x0008},
	{"BOOT_Status"                     , 0x0009},
	{"BOOT_Test "                      , 0x000A},
	{"RxDMA_Ctrl"                      , 0x0010},
	{"RxDMA_Status"                    , 0x0011},
	{"RxDMA_DbgL"                      , 0x0012},
	{"RxDMA_DbgH"                      , 0x0013},
	{"SPI_Slave_Ctrl"                  , 0x0018},
	{"SPI_Slave_Status"                , 0x0019},
	{"SPI_Slave_Rx"                    , 0x001A},
	{"SPI_Slave_Tx"                    , 0x001B},
	{"SPI_Slave_Mask"                  , 0x001C},
	{"UCSG_Ctrl"                       , 0x0020},
	{"UCSG_Status"                     , 0x0021},
	{"UCSG_RxData"                     , 0x0022},
	{"UCSG_TxData"                     , 0x0023},
	{"PCtrl_Ctrl"                      , 0x0028},
	{"PCtrl_Status"                    , 0x0029},
	{"PCtrl_NbByte_LSB"                , 0x002A},
	{"PCtrl_NbByte_MSB"                , 0x002B},
	{"SPI_Master_Ctl"                  , 0x0030},
	{"SPI_Master_NCS"                  , 0x0031},
	{"SPI_Master_Status"               , 0x0032},
	{"SPI_Master_TxBuf"                , 0x0033},
	{"SPI_Master_RxBuf"                , 0x0034},
	{"BISTRAM_Ctl"                     , 0x0038},
	{"BISTRAM_Bank"                    , 0x0039},
	{"BISTRAM_Pat"                     , 0x003A},
	{"BISTRAM_SM"                      , 0x003B},
	{"BISTRAM_AddrLSB"                 , 0x003C},
	{"BISTROM_Config"                  , 0x0040},
	{"BISTROM_SignatureLSB"            , 0x0041},
	{"BISTROM_SignatureMSB"            , 0x0042},
	{"BISTROM_StartAddrLSB"            , 0x0043},
	{"BISTROM_StartAddrMSB"            , 0x0044},
	{"BISTROM_StopAddrLSB"             , 0x0045},
	{"BISTROM_StopAddrMSB"             , 0x0046},
	{"CkMan_Config"                    , 0x0048},
	{"CkMan_Select"                    , 0x0049},
	{"CkMan_Test"                      , 0x004A},
	{"Revision_Number"                 , 0x004B},
	{"CkMan_PD_Key"                    , 0x004C},
	{"USB_Power_Mode"                  , 0x004D},
	{"ResMan_Config"                   , 0x0050},
	{"ResMan_Status"                   , 0x0051},
	{"ResMan_WD"                      , 0x0052},
	{"ResMan_WD_MSB"                   , 0x0053},
	{"TxDMA_Ctrl"                      , 0x0058},
	{"TxDMA_Status"                    , 0x0059},
	{"TxDMA_StartAddrL"                , 0x005A},
	{"TxDMA_StartAddrH"                , 0x005B},
	{"TxDMA_StopAddrL"                 , 0x005C},
	{"TxDMA_StopAddrH"                 , 0x005D},
	{"CPU_Test"                      , 0x0060},
	{"IrqMan_Config0"                  , 0x0068},
	{"IrqMan_Config1"                  , 0x0069},
	{"IrqMan_Irq0"                     , 0x006A},
	{"IrqMan_NMI"                      , 0x006B},
	{"IrqMan_SleepKey"                 , 0x006C},
	{"Tim_Config"                      , 0x0070},
	{"Tim_Value_LSB"                   , 0x0071},
	{"Tim_Value_MSB"                   , 0x0072},
	{"Tim_Comp_LSB"                    , 0x0073},
	{"Tim_Comp_MSB"                    , 0x0074},
	{"TI_Config"                       , 0x0076},
	{"TI_Data"                      , 0x0077},
	{"TI_Reg0"                      , 0x0078},
	{"TI_Reg1"                      , 0x0079},
	{"TI_Reg2"                      , 0x007A},
	{"TI_Reg3"                      , 0x007B},
	{"TI_Reg4"                      , 0x007C},
	{"TI_ROM1"                      , 0x007D},
	{"TI_ROM2"                      , 0x007E},
	{"TI_ROM3"                      , 0x007F},
	{"DVBCI_START_ADDR"                , 0x0100},
	{"DVBCI_END_ADDR"                  , 0x017F},
	{"DATA"                      , 0x0180},
	{"CTRL"                      , 0x0181},
	{"QB_HOST"                      , 0x0182},
	{"LEN_HOST_LSB"                    , 0x0183},
	{"LEN_HOST_MSB"                    , 0x0184},
	{"FIFO_TX_TH_LSB"                  , 0x0185},
	{"FIFO_TX_TH_MSB"                  , 0x0186},
	{"FIFO_TX_D_NB_LSB"                , 0x0187},
	{"FIFO_TX_D_NB_MSB"                , 0x0188},
	{"QB_MOD_CURR"                     , 0x0189},
	{"LEN_MOD_CURR_LSB"                , 0x018A},
	{"LEN_MOD_CURR_MSB"                , 0x018B},
	{"QB_MOD"                      , 0x018C},
	{"LEN_MOD_LSB"                     , 0x018D},
	{"LEN_MOD_MSB"                     , 0x018E},
	{"FIFO_RX_TH_LSB"                  , 0x018F},
	{"FIFO_RX_TH_MSB"                  , 0x0190},
	{"FIFO_RX_D_NB_LSB"                , 0x0191},
	{"FIFO_RX_D_NB_MSB"                , 0x0192},
	{"IT_STATUS_0"                     , 0x0193},
	{"IT_STATUS_1"                     , 0x0194},
	{"IT_MASK_0"                      , 0x0195},
	{"IT_MASK_1"                      , 0x0196},
	{"IT_HOST_PIN_CFG"                 , 0x0200},
	{"CFG_0"                      , 0x0201},
	{"CFG_1"                      , 0x0202},
	{"CFG_2"                      , 0x0203},
	{"IT_HOST"                      , 0x0204},
	{"MOD_IT_STATUS"                   , 0x0205},
	{"MOD_IT_MASK"                     , 0x0206},
	{"MOD_CTRL_A"                      , 0x0207},
	{"MOD_CTRL_B"                      , 0x0208},
	{"DEST_SEL"                      , 0x0209},
	{"CAM_MSB_ADD"                     , 0x020A},
	{"GPIO0_DIR"                      , 0x020B},
	{"GPIO0_DATA_IN"                   , 0x020C},
	{"GPIO0_DATA_OUT"                  , 0x020D},
	{"GPIO0_STATUS"                    , 0x020E},
	{"GPIO0_IT_MASK"                   , 0x020F},
	{"GPIO0_DFT"                      , 0x0210},
	{"GPIO0_MASK_DATA"                 , 0x0211},
	{"GPIO1_DIR"                      , 0x0212},
	{"GPIO1_DATA_IN"                   , 0x0213},
	{"GPIO1_DATA_OUT"                  , 0x0214},
	{"GPIO1_STATUS"                    , 0x0215},
	{"GPIO1_IT_MASK"                   , 0x0216},
	{"MEM_ACC_TIME_A"                  , 0x0217},
	{"MEM_ACC_TIME_B"                  , 0x0218},
	{"IO_ACC_TIME_A"                   , 0x0219},
	{"IO_ACC_TIME_B"                   , 0x021A},
	{"EXT_CH_ACC_TIME_A"               , 0x021B},
	{"EXT_CH_ACC_TIME_B"               , 0x021C},
	{"PAR_IF_0"                      , 0x021D},
	{"PAR_IF_1"                      , 0x021E},
	{"PAR_IF_CTRL"                     , 0x021F},
	{"PCK_LENGTH"                      , 0x0220},
	{"USB2TS_CTRL"                     , 0x0221},
	{"USB2TS0_RDL"                    , 0x0222},
	{"USB2TS1_RDL"                     , 0x0223},
	{"TS2USB_CTRL"                     , 0x0224},
	{"TSOUT_PAR_CTRL"                  , 0x0225},
	{"TSOUT_PAR_CLK_SEL"               , 0x0226},
	{"S2P_CH0_CTRL"                    , 0x0227},
	{"S2P_CH1_CTRL"                    , 0x0228},
	{"P2S_CH0_CTRL"                    , 0x0229},
	{"P2S_CH1_CTRL"                    , 0x022A},
	{"TS_IT_STATUS"                    , 0x022B},
	{"TS_IT_MASK"                      , 0x022C},
	{"IN_SEL"                      , 0x022D},
	{"OUT_SEL"                      , 0x022E},
	{"ROUTER_CAM_CH"                   , 0x022F},
	{"ROUTER_CAM_MOD"                  , 0x0230},
	{"FIFO_CTRL"                      , 0x0231},
	{"FIFO1_2_STATUS"                  , 0x0232},
	{"FIFO3_4_STATUS"                  , 0x0233},
	{"GAP_REMOVER_CH0_CTRL"            , 0x0234},
	{"GAP_REMOVER_CH1_CTRL"            , 0x0235},
	{"SYNC_RTV_CTRL"                   , 0x0236},
	{"SYNC_RTV_CH0_SYNC_NB"            , 0x0237},
	{"SYNC_RTV_CH0_PATTERN"            , 0x0238},
	{"SYNC_RTV_CH1_SYNC_NB"            , 0x0239},
	{"SYNC_RTV_CH1_PATTERN"            , 0x023A},
	{"SYNC_RTV_OFFSET_PATT"            , 0x023B},
	{"CTRL_FILTER"                     , 0x023D},
	{"PID_EN_FILTER_CH0"               , 0x023E},
	{"PID_EN_FILTER_CH1"               , 0x023F},
	{"PID_LSB_FILTER_CH0_0"            , 0x0240},
	{"PID_MSB_FILTER_CH0_0"            , 0x0241},
	{"PID_LSB_FILTER_CH0_1"            , 0x0242},
	{"PID_MSB_FILTER_CH0_1"            , 0x0243},
	{"PID_LSB_FILTER_CH0_2"            , 0x0244},
	{"PID_MSB_FILTER_CH0_2"            , 0x0245},
	{"PID_LSB_FILTER_CH0_3"            , 0x0246},
	{"PID_MSB_FILTER_CH0_3"            , 0x0247},
	{"PID_LSB_FILTER_CH0_4"            , 0x0248},
	{"PID_MSB_FILTER_CH0_4"            , 0x0249},
	{"PID_LSB_FILTER_CH0_5"            , 0x024A},
	{"PID_MSB_FILTER_CH0_5"            , 0x024B},
	{"PID_LSB_FILTER_CH0_6"            , 0x024C},
	{"PID_MSB_FILTER_CH0_6"            , 0x024D},
	{"PID_LSB_FILTER_CH0_7"            , 0x024E},
	{"PID_MSB_FILTER_CH0_7"            , 0x024F},
	{"PID_LSB_FILTER_CH1_0"            , 0x0260},
	{"PID_MSB_FILTER_CH1_0"            , 0x0261},
	{"PID_LSB_FILTER_CH1_1"            , 0x0262},
	{"PID_MSB_FILTER_CH1_1"            , 0x0263},
	{"PID_LSB_FILTER_CH1_2"            , 0x0264},
	{"PID_MSB_FILTER_CH1_2"            , 0x0265},
	{"PID_LSB_FILTER_CH1_3"            , 0x0266},
	{"PID_MSB_FILTER_CH1_3"            , 0x0267},
	{"PID_LSB_FILTER_CH1_4"            , 0x0268},
	{"PID_MSB_FILTER_CH1_4"            , 0x0269},
	{"PID_LSB_FILTER_CH1_5"            , 0x026A},
	{"PID_MSB_FILTER_CH1_5"            , 0x026B},
	{"PID_LSB_FILTER_CH1_6"            , 0x026C},
	{"PID_MSB_FILTER_CH1_6"            , 0x026D},
	{"PID_LSB_FILTER_CH1_7"            , 0x026E},
	{"PID_MSB_FILTER_CH1_7"            , 0x026F},
	{"PID_OLD_LSB_REMAPPER_0"          , 0x0280},
	{"PID_OLD_MSB_REMAPPER_0"          , 0x0281},
	{"PID_OLD_LSB_REMAPPER_1"          , 0x0282},
	{"PID_OLD_MSB_REMAPPER_1"          , 0x0283},
	{"PID_OLD_LSB_REMAPPER_2"          , 0x0284},
	{"PID_OLD_MSB_REMAPPER_2"          , 0x0285},
	{"PID_OLD_LSB_REMAPPER_3"          , 0x0286},
	{"PID_OLD_MSB_REMAPPER_3"          , 0x0287},
	{"PID_OLD_LSB_REMAPPER_4"          , 0x0288},
	{"PID_OLD_MSB_REMAPPER_4"          , 0x0289},
	{"PID_OLD_LSB_REMAPPER_5"          , 0x028A},
	{"PID_OLD_MSB_REMAPPER_5"          , 0x028B},
	{"PID_OLD_LSB_REMAPPER_6"          , 0x028C},
	{"PID_OLD_MSB_REMAPPER_6"          , 0x028D},
	{"PID_OLD_LSB_REMAPPER_7"          , 0x028E},
	{"PID_OLD_MSB_REMAPPER_7"          , 0x028F},
	{"PID_NEW_LSB_REMAPPER_0"          , 0x02A0},
	{"PID_NEW_MSB_REMAPPER_0"          , 0x02A1},
	{"PID_NEW_LSB_REMAPPER_1"          , 0x02A2},
	{"PID_NEW_MSB_REMAPPER_1"          , 0x02A3},
	{"PID_NEW_LSB_REMAPPER_2"          , 0x02A4},
	{"PID_NEW_MSB_REMAPPER_2"          , 0x02A5},
	{"PID_NEW_LSB_REMAPPER_3"          , 0x02A6},
	{"PID_NEW_MSB_REMAPPER_3"          , 0x02A7},
	{"PID_NEW_LSB_REMAPPER_4"          , 0x02A8},
	{"PID_NEW_MSB_REMAPPER_4"          , 0x02A9},
	{"PID_NEW_LSB_REMAPPER_5"         , 0x02AA},
	{"PID_NEW_MSB_REMAPPER_5"          , 0x02AB},
	{"PID_NEW_LSB_REMAPPER_6"          , 0x02AC},
	{"PID_NEW_MSB_REMAPPER_6"          , 0x02AD},
	{"PID_NEW_LSB_REMAPPER_7"          , 0x02AE},
	{"PID_NEW_MSB_REMAPPER_7"          , 0x02AF},
	{"MERGER_DIV_MICLK"                , 0x02C0},
	{"PID_AND_SYNC_REMAPPER_CTRL"      , 0x02C1},
	{"PID_EN_REMAPPER"                 , 0x02C2},
	{"SYNC_SYMBOL"                     , 0x02C3},
	{"PID_AND_SYNC_REMAPPER_INV_CTRL"  , 0x02C4},
	{"BITRATE_CH0_LSB"                 , 0x02C5},
	{"BITRATE_CH0_MSB"                 , 0x02C6},
	{"BITRATE_CH1_LSB"                 , 0x02C7},
	{"BITRATE_CH1_MSB"                 , 0x02C8},
	{"STATUS_CLK_SWITCH_0"             , 0x02C9},
	{"STATUS_CLK_SWITCH_1"             , 0x02CA},
	{"RESET_CLK_SWITCH_0"              , 0x02CB},
	{"RESET_CLK_SWITCH_1"              , 0x02CC},
	{"PAD_DRVSTR_CTRL"                 , 0x02CD},
	{"PAD_PUPD_CTRL"                   , 0x02CE},
	{"PRE_HEADER_ADDER_CH0_0"          , 0x02D0},
	{"PRE_HEADER_ADDER_CH0_1"          , 0x02D1},
	{"PRE_HEADER_ADDER_CH0_2"          , 0x02D2},
	{"PRE_HEADER_ADDER_CH0_3"          , 0x02D3},
	{"PRE_HEADER_ADDER_CH0_4"          , 0x02D4},
	{"PRE_HEADER_ADDER_CH0_5"          , 0x02D5},
	{"PRE_HEADER_ADDER_CH0_6"          , 0x02D6},
	{"PRE_HEADER_ADDER_CH0_7"          , 0x02D7},
	{"PRE_HEADER_ADDER_CH0_8"          , 0x02D8},
	{"PRE_HEADER_ADDER_CH0_9"          , 0x02D9},
	{"PRE_HEADER_ADDER_CH0_10"         , 0x02DA},
	{"PRE_HEADER_ADDER_CH0_11"         , 0x02DB},
	{"PRE_HEADER_ADDER_CH1_0"          , 0x02E0},
	{"PRE_HEADER_ADDER_CH1_1"          , 0x02E1},
	{"PRE_HEADER_ADDER_CH1_2"          , 0x02E2},
	{"PRE_HEADER_ADDER_CH1_3"          , 0x02E3},
	{"PRE_HEADER_ADDER_CH1_4"          , 0x02E4},
	{"PRE_HEADER_ADDER_CH1_5"          , 0x02E5},
	{"PRE_HEADER_ADDER_CH1_6"          , 0x02E6},
	{"PRE_HEADER_ADDER_CH1_7"          , 0x02E7},
	{"PRE_HEADER_ADDER_CH1_8"          , 0x02E8},
	{"PRE_HEADER_ADDER_CH1_9"          , 0x02E9},
	{"PRE_HEADER_ADDER_CH1_10"         , 0x02EA},
	{"PRE_HEADER_ADDER_CH1_11"         , 0x02EB},
	{"PRE_HEADER_ADDER_CTRL"           , 0x02EC},
	{"PRE_HEADER_ADDER_LEN"            , 0x02ED},
	{"PRE_HEADER_REMOVER_CTRL"         , 0x02EE},
	{"FSM_DVB"                      , 0x02F0},
	{"TS2USB_FSM_DEBUG"                , 0x02F2},
	{"TSOUT_PAR_FSM_DEBUG"             , 0x02F3},
	{"GAP_REMOVER_FSM_DEBUG"           , 0x02F4},
	{"PID_AND_SYNC_REMAPPER_FSM_DEBUG" , 0x02F5},
	{"PRE_HEADER_ADDER_FSM_DEBUG"      , 0x02F6},
	{"SYNC_RTV_FSM_DEBUG"              , 0x02F7},
	{"CHECK_PHY_CLK"                   , 0x0E00},
	{"CONTROL1"                      , 0x0E01},
	{"WAKE_UP"                      , 0x0E02},
	{"CONTROL2"                      , 0x0E03},
	{"PHY_RELATED"                     , 0x0E04},
	{"EP_CFG"                      , 0x0E05},
	{"MAX_PKT_EP1L"                    , 0x0E06},
	{"MAX_PKT_EP1H"                    , 0x0E07},
	{"MAX_PKT_EP2L"                    , 0x0E08},
	{"MAX_PKT_EP2H"                    , 0x0E09},
	{"MAX_PKT_EP3L"                    , 0x0E0A},
	{"MAX_PKT_EP3H"                    , 0x0E0B},
	{"MAX_PKT_EP4L"                    , 0x0E0C},
	{"MAX_PKT_EP4H"                    , 0x0E0D},
	{"EPS_STALL_SET"                   , 0x0E10},
	{"EPS_STALL_CLR"                   , 0x0E11},
	{"EPS_ENABLE"                      , 0x0E12},
	{"DMA_ACC_EPS"                     , 0x0E13},
	{"CPU_ACC_EPS_EN"                  , 0x0E14},
	{"SETUP_BYTE0"                     , 0x0E15},
	{"SETUP_BYTE1"                     , 0x0E16},
	{"SETUP_BYTE2"                     , 0x0E17},
	{"SETUP_BYTE3"                     , 0x0E18},
	{"SETUP_BYTE4"                     , 0x0E19},
	{"SETUP_BYTE5"                     , 0x0E1A},
	{"SETUP_BYTE6"                     , 0x0E1B},
	{"SETUP_BYTE7"                     , 0x0E1C},
	{"SETUP_DT_VLD"                    , 0x0E1D},
	{"CLR_EPS_TOG"                     , 0x0E1E},
	{"EP0_CTRL"                      , 0x0E20},
	{"EP0_DATA_CNT"                    , 0x0E21},
	{"EP0_DATA"                      , 0x0E22},
	{"EP1_CTRL"                      , 0x0E30},
	{"EP1_DATA_CNTL"                   , 0x0E31},
	{"EP1_DATA_CNTH"                   , 0x0E32},
	{"EP1_DATA"                      , 0x0E33},
	{"EP1_HEADER"                      , 0x0E34},
	{"EP2_CTRL"                      , 0x0E40},
	{"EP2_DATA_CNTL"                   , 0x0E41},
	{"EP2_DATA_CNTH"                   , 0x0E42},
	{"EP2_DATA"                      , 0x0E43},
	{"EP2_HEADER"                      , 0x0E44},
	{"EP3_DATA_CNTL"                   , 0x0E50},
	{"EP3_DATA_CNTH"                   , 0x0E51},
	{"EP3_DATA"                      , 0x0E52},
	{"EP3_HEADER"                      , 0x0E53},
	{"EP3_HEADER_CNT"                  , 0x0E54},
	{"EP3_HEADER_DATA"                 , 0x0E55},
	{"EP4_DATA_CNTL"                   , 0x0E60},
	{"EP4_DATA_CNTH"                   , 0x0E61},
	{"EP4_DATA"                      , 0x0E62},
	{"EP4_HEADER"                      , 0x0E63},
	{"EP4_HEADER_CNT"                  , 0x0E64},
	{"EP4_HEADER_DATA"                 , 0x0E65},
	{"EP5_CTRL"                      , 0x0E70},
	{"EP5_DATA_CNTL"                   , 0x0E71},
	{"EP5_DATA_CNTH"                   , 0x0E72},
	{"EP5_DATA"                      , 0x0E73},
	{"MAX_PKT_EP5L"                    , 0x0E74},
	{"MAX_PKT_EP5H"                    , 0x0E75},
	{"EP6_DATA_CNTL"                   , 0x0E80},
	{"EP6_DATA_CNTH"                   , 0x0E81},
	{"EP6_DATA"                      , 0x0E82},
	{"MAX_PKT_EP6L"                    , 0x0E83},
	{"MAX_PKT_EP6H"                    , 0x0E84},
	{"FRAME_NUML"                      , 0x0E90},
	{"FRAME_NUMH"                      , 0x0E91},
	{"FRAME_TIMEL"                     , 0x0E92},
	{"FRAME_TIMEH"                     , 0x0E93},
	{"STC_DIVL"                      , 0x0E94},
	{"STC_DIVM"                      , 0x0E95},
	{"STC_DIVH"                      , 0x0E96},
	{"USB_STATUS"                      , 0x0E97},
	{"DEV_STATE1"                      , 0x0E98},
	{"DEV_STATE2"                      , 0x0E99},
	{"DEV_STATE3"                      , 0x0E9A},
	{"DEV_STATE4"                      , 0x0E9B},
	{"INTR_EN1"                      , 0x0EA0},
	{"INTR_EN2"                      , 0x0EA1},
	{"INTR_EN3"                      , 0x0EA2},
	{"INTR_EN4"                      , 0x0EA3},
	{"INTR_SRC1"                      , 0x0EB0},
	{"INTR_SRC2"                      , 0x0EB1},
	{"INTR_SRC3"                      , 0x0EB2},
	{"INTR_SRC4"                      , 0x0EB3},
	{"INTR_FLAG1"                      , 0x0EC0},
	{"INTR_FLAG2"                      , 0x0EC1},
	{"INTR_FLAG3"                      , 0x0EC2},
	{"INTR_FLAG4"                      , 0x0EC3},
	{"EP0_INAK_CNT"                    , 0x0ED0},
	{"EP0_ONAK_CNT"                    , 0x0ED1},
	{"EP1_NAK_CNT"                     , 0x0ED2},
	{"EP2_NAK_CNT"                     , 0x0ED3},
	{"EP3_NAK_CNT"                     , 0x0ED4},
	{"EP4_NAK_CNT"                     , 0x0ED5},
	{"EP5_NAK_CNT"                     , 0x0ED6},
	{"EP6_NAK_CNT"                     , 0x0ED7},
	{"NAK_CNT_LEVEL"                   , 0x0ED8},
	{"CC2_Buffer_out"                  , 0x2000},
	{"CC2_Buffer_in"                   , 0x4000},
	{"nmb_vector_address_lsb"          , 0xFFFA},
	{"nmb_vector_address_msb"          , 0xFFFB},
	{"reset_vector_address_lsb"        , 0xFFFC},
	{"reset_vector_address_msb"        , 0xFFFD},
	{"irb_vector_address_lsb"          , 0xFFFE},
	{"irb_vector_address_msb"          , 0xFFFF}
};
#endif
