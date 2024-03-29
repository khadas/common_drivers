// Copyright 2018 Google Inc. All Rights Reserved.

#if !defined(__IMX224_SEQ_H__)
#define __IMX224_SEQ_H__

/*
static acam_reg_t init[] = {
    //wait command - address is 0xFFFF
    { 0xFFFF, 20 },
    //stop sequence - address is 0x0000
    { 0x0000, 0x0000, 0x0000, 0x0000 }
};
*/

static acam_reg_t setting_QVGA_1280_960_2lane_12bit_30fps[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x01,0xFF,1},  /* ADBIT: 0x00 - 10 bit 0x01 - 12 bit */
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x02,0xFF,1},
{0x300A,0xF0,0xFF,1},
{0x300C,0x00,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0x94,0xFF,1},
{0x301C,0x11,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x00,0xFF,1},
{0x31E8,0x01,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x10,0xFF,1},
{0x3346,0x01,0xFF,1}, /* Physical lane number */
{0x3354,0x01,0xFF,1},
{0x3357,0xD1,0xFF,1},
{0x3358,0x03,0xFF,1},
{0x336B,0x37,0xFF,1},
{0x337D,0x0C,0xFF,1}, /* CSI FMT - RAW10: 0x0A0A, RAW12: 0x0C0C */
{0x337E,0x0C,0xFF,1},
{0x337F,0x01,0xFF,1}, /* CSI lanes - 0x00 - 1 lane, 0x01 - 2 lanes, 0x04 - 4 lanes */
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x5F,0xFF,1},
{0x3383,0x17,0xFF,1},
{0x3384,0x37,0xFF,1},
{0x3385,0x17,0xFF,1},
{0x3386,0x17,0xFF,1},
{0x3387,0x17,0xFF,1},
{0x3388,0x4F,0xFF,1},
{0x3389,0x27,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t setting_QVGA_1280_960_2lane_10bit_30fps[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x00,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x02,0xFF,1},
{0x300A,0x3C,0xFF,1},
{0x300C,0x00,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0x94,0xFF,1},
{0x301C,0x11,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x00,0xFF,1},
{0x31E8,0x00,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x10,0xFF,1},
{0x3346,0x01,0xFF,1}, /* Physical lane number */
{0x3354,0x01,0xFF,1},
{0x3357,0xD1,0xFF,1},
{0x3358,0x03,0xFF,1},
{0x336B,0x37,0xFF,1},
{0x337D,0x0A,0xFF,1},
{0x337E,0x0A,0xFF,1},
{0x337F,0x01,0xFF,1}, /* CSI lanes - 0x00 - 1 lane, 0x01 - 2 lanes, 0x04 - 4 lanes */
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x5F,0xFF,1},
{0x3383,0x17,0xFF,1},
{0x3384,0x37,0xFF,1},
{0x3385,0x17,0xFF,1},
{0x3386,0x17,0xFF,1},
{0x3387,0x17,0xFF,1},
{0x3388,0x4F,0xFF,1},
{0x3389,0x27,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t setting_QVGA_1280_960_4lane_10bit_30fps[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x00,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x02,0xFF,1},
{0x300A,0x3C,0xFF,1},
{0x300C,0x00,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0x94,0xFF,1},
{0x301C,0x11,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x00,0xFF,1},
{0x31E8,0x00,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x20,0xFF,1},
{0x3346,0x03,0xFF,1},
{0x3354,0x01,0xFF,1},
{0x3357,0xD1,0xFF,1},
{0x3358,0x03,0xFF,1},
{0x336B,0x27,0xFF,1},
{0x337D,0x0A,0xFF,1},
{0x337E,0x0A,0xFF,1},
{0x337F,0x03,0xFF,1},
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x57,0xFF,1},
{0x3383,0x0F,0xFF,1},
{0x3384,0x27,0xFF,1},
{0x3385,0x0F,0xFF,1},
{0x3386,0x0F,0xFF,1},
{0x3387,0x07,0xFF,1},
{0x3388,0x37,0xFF,1},
{0x3389,0x1F,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t setting_QVGA_1280_960_2lane_12bit_60fps[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x01,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x01,0xFF,1}, /* FPS: 0x01: 60 fps, 0x02: 30 fps */
{0x300A,0xF0,0xFF,1},
{0x300C,0x00,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0xCA,0xFF,1}, /* HMAX: 0x08CA - 60fps, 0x1194 - 30 fps */
{0x301C,0x08,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x00,0xFF,1},
{0x31E8,0x01,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x00,0xFF,1},
{0x3346,0x01,0xFF,1}, /* Physical lane num - 0x1 - 2 lanes, 0x3h - 4 lanes */
{0x3354,0x01,0xFF,1},
{0x3357,0xD1,0xFF,1},
{0x3358,0x03,0xFF,1},
{0x336B,0x57,0xFF,1},
{0x337D,0x0C,0xFF,1},
{0x337E,0x0C,0xFF,1},
{0x337F,0x01,0xFF,1}, /* CSI lane mode: 0x1h - 2 lanes, 0x3h - 4 lanes */
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x6F,0xFF,1},
{0x3383,0x27,0xFF,1},
{0x3384,0x4F,0xFF,1},
{0x3385,0x2F,0xFF,1},
{0x3386,0x2F,0xFF,1},
{0x3387,0x2F,0xFF,1},
{0x3388,0x9F,0xFF,1},
{0x3389,0x37,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t setting_QVGA_1280_960_2lane_10bit_60fps[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x00,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x01,0xFF,1}, /* FPS: 0x01: 60 fps, 0x02: 30 fps */
{0x300A,0x3C,0xFF,1},
{0x300C,0x00,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0xCA,0xFF,1}, /* HMAX: 0x08CA - 60fps, 0x1194 - 30 fps */
{0x301C,0x08,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x00,0xFF,1},
{0x31E8,0x00,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x00,0xFF,1},
{0x3346,0x01,0xFF,1}, /* Physical lane num - 0x1 - 2 lanes, 0x3h - 4 lanes */
{0x3354,0x01,0xFF,1},
{0x3357,0xD1,0xFF,1},
{0x3358,0x03,0xFF,1},
{0x336B,0x57,0xFF,1},
{0x337D,0x0A,0xFF,1},
{0x337E,0x0A,0xFF,1},
{0x337F,0x01,0xFF,1}, /* CSI lane mode: 0x1h - 2 lanes, 0x3h - 4 lanes */
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x6F,0xFF,1},
{0x3383,0x27,0xFF,1},
{0x3384,0x4F,0xFF,1},
{0x3385,0x2F,0xFF,1},
{0x3386,0x2F,0xFF,1},
{0x3387,0x2F,0xFF,1},
{0x3388,0x9F,0xFF,1},
{0x3389,0x37,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t setting_QVGA_1280_960_4lane_10bit_60fps[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x00,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x01,0xFF,1}, /* FPS: 0x01: 60 fps, 0x02: 30 fps */
{0x300A,0x3C,0xFF,1},
{0x300C,0x00,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0xCA,0xFF,1}, /* HMAX: 0x08CA - 60fps, 0x1194 - 30 fps */
{0x301C,0x08,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x00,0xFF,1},
{0x31E8,0x00,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x10,0xFF,1},
{0x3346,0x03,0xFF,1},
{0x3354,0x01,0xFF,1},
{0x3357,0xD1,0xFF,1},
{0x3358,0x03,0xFF,1},
{0x336B,0x37,0xFF,1},
{0x337D,0x0A,0xFF,1},
{0x337E,0x0A,0xFF,1},
{0x337F,0x03,0xFF,1},
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x5F,0xFF,1},
{0x3383,0x17,0xFF,1},
{0x3384,0x37,0xFF,1},
{0x3385,0x17,0xFF,1},
{0x3386,0x17,0xFF,1},
{0x3387,0x17,0xFF,1},
{0x3388,0x4F,0xFF,1},
{0x3389,0x27,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t setting_QVGA_1280_960_2lane_12bit_30fps_WDR[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x01,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x01,0xFF,1},
{0x300A,0xF0,0xFF,1},
{0x300C,0x11,0xFF,1}, /* 2 frame DOL mode */
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0xCA,0xFF,1},
{0x301C,0x08,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3020,0x04,0xFF,1}, //1a
{0x3021,0x00,0xFF,1},
{0x3022,0x00,0xFF,1},
{0x3023,0x57,0xFF,1}, //c5
{0x3024,0x00,0xFF,1},
{0x3025,0x00,0xFF,1},
{0x302C,0x47,0xFF,1}, //c1
{0x302D,0x00,0xFF,1},
{0x302E,0x00,0xFF,1},
{0x3043,0x05,0xFF,1},
{0x3044,0x01,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x3109,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x07,0xFF,1},
{0x31E8,0x01,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x00,0xFF,1},
{0x3346,0x01,0xFF,1}, /* Physical lane number */
{0x3354,0x00,0xFF,1},
{0x3357,0x46,0xFF,1},  //6a
{0x3358,0x08,0xFF,1},
{0x336B,0x57,0xFF,1},
{0x337D,0x0C,0xFF,1},
{0x337E,0x0C,0xFF,1},
{0x337F,0x01,0xFF,1}, /* CSI lanes - 0x00 - 1 lane, 0x01 - 2 lanes, 0x04 - 4 lanes */
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x6F,0xFF,1},
{0x3383,0x27,0xFF,1},
{0x3384,0x4F,0xFF,1},
{0x3385,0x2F,0xFF,1},
{0x3386,0x2F,0xFF,1},
{0x3387,0x2F,0xFF,1},
{0x3388,0x9F,0xFF,1},
{0x3389,0x37,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};
static acam_reg_t setting_QVGA_1280_960_2lane_10bit_30fps_WDR[] = {
{0x3000, 0x01, 0x00, 1}, /* standby */
{0xFFFF, 1},
{0x3005,0x00,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x00,0xFF,1},
{0x3009,0x01,0xFF,1},
{0x300A,0x3C,0xFF,1},
{0x300C,0x11,0xFF,1}, /* 2 frame DOL mode */
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0xCA,0xFF,1},
{0x301C,0x08,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3020,0x04,0xFF,1}, //1a
{0x3021,0x00,0xFF,1},
{0x3022,0x00,0xFF,1},
{0x3023,0x57,0xFF,1}, //c5
{0x3024,0x00,0xFF,1},
{0x3025,0x00,0xFF,1},
{0x302C,0x47,0xFF,1}, //c1
{0x302D,0x00,0xFF,1},
{0x302E,0x00,0xFF,1},
{0x3043,0x05,0xFF,1},
{0x3044,0x01,0xFF,1},
{0x3049,0x0A,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x3109,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x07,0xFF,1},
{0x31E8,0x01,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
{0x3344,0x00,0xFF,1},
{0x3346,0x01,0xFF,1}, /* Physical lane number */
{0x3354,0x00,0xFF,1},
{0x3357,0x46,0xFF,1}, //6a
{0x3358,0x08,0xFF,1},
{0x336B,0x57,0xFF,1},
{0x337D,0x0A,0xFF,1},
{0x337E,0x0A,0xFF,1},
{0x337F,0x01,0xFF,1}, /* CSI lanes - 0x00 - 1 lane, 0x01 - 2 lanes, 0x04 - 4 lanes */
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x6F,0xFF,1},
{0x3383,0x27,0xFF,1},
{0x3384,0x4F,0xFF,1},
{0x3385,0x2F,0xFF,1},
{0x3386,0x2F,0xFF,1},
{0x3387,0x2F,0xFF,1},
{0x3388,0x9F,0xFF,1},
{0x3389,0x37,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0xFFFF, 1},
{0x3002, 0x00, 0x00, 1}, /* master mode start */
{0xFFFF, 1},
{0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
{0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t setting_1280_720_1lane_10bit_30fps[] = {
    {0x3003, 0x01, 0x00, 1}, /* sw_reset */
    {0xFFFF, 1},
    {0x3000, 0x01, 0x00, 1}, /* standby */
    {0xFFFF, 1},
{0x3005,0x00,0xFF,1},
{0x3006,0x00,0xFF,1},
{0x3007,0x10,0xFF,1},
{0x3009,0x02,0xFF,1},
{0x300A,0x3C,0xFF,1},
{0x300C,0x00,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3012,0x2C,0xFF,1},
{0x3013,0x01,0xFF,1},
{0x3016,0x09,0xFF,1},
{0x3018,0x4C,0xFF,1},
{0x3019,0x04,0xFF,1},
{0x301B,0x94,0xFF,1},
{0x301C,0x11,0xFF,1},
{0x301D,0xC2,0xFF,1},
{0x3054,0x66,0xFF,1},
{0x305C,0x20,0xFF,1},
{0x305D,0x00,0xFF,1},
{0x305E,0x20,0xFF,1},
{0x305F,0x00,0xFF,1},
{0x3344,0x00,0xFF,1},
{0x3346,0x00,0xFF,1},
{0x3353,0x04,0xFF,1},
{0x3354,0x01,0xFF,1},
{0x3357,0xD9,0xFF,1},
{0x3358,0x02,0xFF,1},
{0x336B,0x57,0xFF,1},
{0x336C,0x1F,0xFF,1},
{0x3370,0x02,0xFF,1},
{0x3371,0x01,0xFF,1},
{0x337D,0x0A,0xFF,1},
{0x337E,0x0A,0xFF,1},
{0x337F,0x00,0xFF,1},
{0x3380,0x20,0xFF,1},
{0x3381,0x25,0xFF,1},
{0x3382,0x6F,0xFF,1},
{0x3383,0x27,0xFF,1},
{0x3384,0x4F,0xFF,1},
{0x3385,0x2F,0xFF,1},
{0x3386,0x2F,0xFF,1},
{0x3387,0x2F,0xFF,1},
{0x3388,0x9F,0xFF,1},
{0x3389,0x37,0xFF,1},
{0x338D,0xB4,0xFF,1},
{0x338E,0x01,0xFF,1},
{0x300F,0x00,0xFF,1},
{0x3070,0x02,0xFF,1},
{0x3071,0x01,0xFF,1},
{0x309E,0x22,0xFF,1},
{0x30A5,0xFB,0xFF,1},
{0x30A6,0x02,0xFF,1},
{0x30B3,0xFF,0xFF,1},
{0x30B4,0x01,0xFF,1},
{0x30B5,0x42,0xFF,1},
{0x30B8,0x10,0xFF,1},
{0x30C2,0x01,0xFF,1},
{0x310F,0x0F,0xFF,1},
{0x3110,0x0E,0xFF,1},
{0x3111,0xE7,0xFF,1},
{0x3112,0x9C,0xFF,1},
{0x3113,0x83,0xFF,1},
{0x3114,0x10,0xFF,1},
{0x3115,0x42,0xFF,1},
{0x3128,0x1E,0xFF,1},
{0x3144,0x00,0xFF,1},
{0x31ED,0x38,0xFF,1},
{0x320C,0xCF,0xFF,1},
{0x324C,0x40,0xFF,1},
{0x324D,0x03,0xFF,1},
{0x3261,0xE0,0xFF,1},
{0x3262,0x02,0xFF,1},
{0x326E,0x2F,0xFF,1},
{0x326F,0x30,0xFF,1},
{0x3270,0x03,0xFF,1},
{0x3298,0x00,0xFF,1},
{0x329A,0x12,0xFF,1},
{0x329B,0xF1,0xFF,1},
{0x329C,0x0C,0xFF,1},
    {0xFFFF, 1},
    {0x3002, 0x00, 0x00, 1}, /* master mode start */
    {0xFFFF, 1},
    {0x3049, 0x0A, 0x00, 1}, /* XVSOUTSEL XHSOUTSEL */
    {0x0000, 0x0000, 0x0000, 0x0000},

};

static acam_reg_t settings_context_imx224[] = {
    { 0x18ec8, 0x0L, 0x1f,1 },
    { 0x1ae7c, 0xa0b9beb9L, 0xffffffff,4 },
    { 0x1ae84, 0x5aL, 0xff,1 },
    { 0x1ae84, 0x32L, 0xff,1 },
    { 0x1ae84, 0x50L, 0xff,1 },
    { 0x1ae84, 0x5aL, 0xff,1 },
    { 0x1aea8, 0x5fa0L, 0xffff,2 },
    { 0x80L,   0x1L,  0xff,1 },
    { 0x19284, 0x4000L, 0xffff,2 },
    { 0x19288, 0x400L, 0xffff,2 },
    { 0x1abc0, 0x400L, 0xffff,2 },
    { 0x1abc4, 0x4000L, 0xffff,2 },
    //stop sequence - address is 0x0000
    { 0x0000, 0x0000, 0x0000, 0x0000 }
};

static const acam_reg_t *imx224_seq_table[] = {
    setting_QVGA_1280_960_2lane_12bit_30fps,
    setting_QVGA_1280_960_2lane_12bit_60fps,
    setting_QVGA_1280_960_2lane_12bit_30fps_WDR,
    setting_QVGA_1280_960_2lane_10bit_30fps,
    setting_QVGA_1280_960_2lane_10bit_60fps,
    setting_QVGA_1280_960_2lane_10bit_30fps_WDR,
    setting_QVGA_1280_960_4lane_10bit_30fps,
    setting_QVGA_1280_960_4lane_10bit_60fps,
    setting_1280_720_1lane_10bit_30fps,
};

static const acam_reg_t *isp_seq_table[] = {
    settings_context_imx224,
};


#define SENSOR_IMX224_SEQUENCE_DEFAULT imx224_seq_table
#define SENSOR_IMX224_ISP_CONTEXT_SEQUENCE isp_seq_table

// These are indexes to imx224_seq_table used in IMX224 driver
#define SENSOR_IMX224_SEQUENCE_QVGA_30FPS_12BIT_2LANES      0
#define SENSOR_IMX224_SEQUENCE_QVGA_60FPS_12BIT_2LANES      1
#define SENSOR_IMX224_SEQUENCE_QVGA_30FPS_12BIT_2LANES_WDR  2
#define SENSOR_IMX224_SEQUENCE_QVGA_30FPS_10BIT_2LANES      3
#define SENSOR_IMX224_SEQUENCE_QVGA_60FPS_10BIT_2LANES      4
#define SENSOR_IMX224_SEQUENCE_QVGA_30FPS_10BIT_2LANES_WDR  5
#define SENSOR_IMX224_SEQUENCE_QVGA_30FPS_10BIT_4LANES      6
#define SENSOR_IMX224_SEQUENCE_QVGA_60FPS_10BIT_4LANES      7
#define SENSOR_IMX224_SEQUENCE_720P_30FPS_10BIT_1LANE       8

#define SENSOR_IMX224_CONTEXT_SEQ  0

#define SENSOR_IMX224_SEQUENCE_DEFAULT_INIT SENSOR_IMX224_SEQUENCE_QVGA_30FPS_10BIT_2LANE
#endif // __IMX224_SEQ_H__
