#ifndef __VDEC_DEBUG_UTILS__
#define __VDEC_DEBUG_UTILS__

#include "vdec.h"

#define VDEC_DEBUG_MODULE  "VDEC_DEBUG"

#define LPRINT0
#define LPRINT1(...)        printk(__VA_ARGS__)

#define ERRP(con, rt, p, ...) do {    \
    if (con) {                        \
        LPRINT##p(__VA_ARGS__);       \
        rt;                           \
    }                                 \
} while(0)


#define _VDP_  'P'
#define VDBG_IOC_PORT_CFG      _IOW((_VDP_), 0x01, int)
#define VDBG_IOC_GET_DATA      _IOW((_VDP_), 0x02, int)
#define VDBG_IOC_DATA_DONE     _IOW((_VDP_), 0x03, int)
#define VDBG_IOC_BUF_RESET     _IOW((_VDP_), 0x0a, int)
#define VDBG_IOC_GET_VINFO     _IOW((_VDP_), 0x0b, int)

struct debug_config_param {
	int type;
	int id;
	u32 pic_start;  // yuv dump start pic
	u32 pic_num;    // yuv dump pic num
	u32 mode;       // dump es mode;

	char *buf;
	u32 buf_size;

	u32 reserved[32];
};

//fatal_error
#define DEBUG_PORT_FATAL_ERROR_NOMEM 0x1

//debug_flag
#define PORT_VDBG_FLAG_ERR   0
#define PORT_VDBG_FLAG_DBG   1
#define PORT_VDBG_FLAG_INFO  2

#define MAX_INSTANCE_NUM    9

struct amvdec_debug_port_t {
	struct list_head head;
	struct mutex mlock;

	ulong buf_start;  //phy addr
	u32 buf_size;
	char *wp;
	char *rp;
	char *buf_vaddr;

	u32 packet_cnt;
	wait_queue_head_t poll_wait;
	wait_queue_head_t wait_data_done;

	u32 enable[MAX_INSTANCE_NUM];
	u32 fatal_error;
	u32 debug_flag;
};


#define PADING_SIZE    (1024)
#define PACKET_HEADER  (0xaa55aa55)

enum data_type{
	TYPE_INFO,
	TYPE_YUV,
	TYPE_CRC,
	TYPE_ES,
	TYPE_MAX
};

struct port_data_packet {
	int header;
	int id;
	int type;
	u32 data_size;
	u32 crc;
	u32 private_data_size;
	char private[0];   //private to PADING_SIZE end
};

struct port_vdec_info {
	int id;
	int format;
	int double_write;
	int stream_w;
	int stream_h;
	int dw_w;
	int dw_h;
	int plane_num;
	int bitdepth;
	int is_interlace;
	u32 reserved[32];
};

#endif
