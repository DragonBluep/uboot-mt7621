/*
 * NMRP support driver
 *
 * Shi Dong <shidong@twsz.com> 2020
 *
 */


#ifndef __NMRP_H__
#define __NMRP_H__


#ifndef IP_ADDR_LEN
#define IP_ADDR_LEN 4
#endif

typedef struct
{
    unsigned short type;

    unsigned short len;
    unsigned char value;
} nmrp_opt_t;

typedef struct
{
    unsigned short reserved;
    unsigned char code;
    unsigned char id;
    unsigned short length;
    nmrp_opt_t opt;
} nmrp_t;

#define NMRP_HDR_LEN (sizeof(nmrp_t) - sizeof(nmrp_opt_t))
//#define MIN_ETHER_NMRP_LEN (sizeof(Ethernet_t) + NMRP_HDR_LEN)

#define PROT_NMRP       0x0912

extern int NmrpState;

/* NMRP codes */
enum _nmrp_codes_
{
    NMRP_CODE_ADVERTISE      = 0x01,
    NMRP_CODE_CONF_REQ       = 0x02,
    NMRP_CODE_CONF_ACK       = 0x03,
    NMRP_CODE_CLOSE_REQ      = 0x04,
    NMRP_CODE_CLOSE_ACK      = 0x05,
    NMRP_CODE_KEEP_ALIVE_REQ = 0x06,
    NMRP_CODE_KEEP_ALIVE_ACK = 0x07,
    NMRP_CODE_TFTP_UL_REQ    = 0x10
};

/* NMRP option types */
enum _nmrp_option_types_
{
    NMRP_OPT_MAGIC_NO   = 0x0001,
    NMRP_OPT_DEV_IP     = 0x0002,
    NMRP_OPT_DEV_REGION = 0x0004,
    NMRP_OPT_FW_UP      = 0x0101,
    NMRP_OPT_ST_UP      = 0x0102,
    NMRP_OPT_FILE_NAME  = 0x0181
};

/* NMRP Region value */
enum _nmrp_region_values_
{
    NMRP_REGION_NA = 0x0001,
    NMRP_REGION_WW = 0x0002,
    NMRP_REGION_GR = 0x0003,
    NMRP_REGION_PR = 0x0004,
    NMRP_REGION_RU = 0x0005,
    NMRP_REGION_BZ = 0x0006,
    NMRP_REGION_IN = 0x0007,
    NMRP_REGION_KO = 0x0008,
    NMRP_REGION_JP = 0x0009
};

/*  NMRP REQ max retries */
enum _nmrp_req_max_retries_ {
    NMRP_MAX_RETRY_CONF    = 5,
    NMRP_MAX_RETRY_CLOSE   = 2,
    NMRP_MAX_RETRY_TFTP_UL = 4,
    NMRP_MAX_RETRY_CLOSE2  = 11  /*6s超时*/
};

/* NMRP timeouts */
enum _nmrp_timeouts_ {
	NMRP_TIMEOUT_REQ       = 1,	 /* 0.5 sec */
	NMRP_TIMEOUT_LISTEN    = 6, /* 3 sec 修改为6秒 ，网口协商需要时间*/
	NMRP_TIMEOUT_CONFIG    = 3,
	NMRP_TIMEOUT_ACTIVE    = 60, /* 1 minute */
	NMRP_TIMEOUT_CLOSEING  = 2,
	NMRP_TIMEOUT_CLOSE     = 6,	 /* 6 sec */
	NMRP_TIMEOUT_ADVERTISE = 1	 /* 0.5 sec */
};

#define NMRP_STATE_INIT          0
#define NMRP_STATE_LISTENING     1
#define NMRP_STATE_CONFIGING     2
#define NMRP_STATE_TFTPWAITING   3
#define NMRP_STATE_TFTPUPLOADING 4
#define NMRP_STATE_FILEUPLOADING 5
#define NMRP_STATE_CLOSING       6
#define NMRP_STATE_CLOSED        7
#define NMRP_STATE_KEEP_ALIVE    8

#define NMRP_HEADER_LEN 6
#define NMRP_OPT_HEADER_LEN 4
#define MAGICNO htonl(0x4E544752)
#define IP_LEN 4
#define MAGIC_NO_LEN 4
#define DEV_REGION_LEN 2
#define FIRMWARE_FILENAME "firmware"
#define FIRMWARE_FILENAME_LEN 8
#define FIRMWARE_FILENAME_OPT_LEN (NMRP_OPT_HEADER_LEN + FIRMWARE_FILENAME_LEN)
#define STRING_TABLE_BITMASK_LEN 32
#define STRING_TABLE_NUMBER_MAX STRING_TABLE_BITMASK_LEN
#define STRING_TABLE_FILENAME_PREFIX "string table "
#define STRING_TABLE_FILENAME_PREFIX_LEN 13
#define STRING_TABLE_FILENAME_EXAMPLE "string table 01"
#define STRING_TABLE_FILENAME_LEN (STRING_TABLE_FILENAME_PREFIX_LEN + 2)
#define STRING_TABLE_FILENAME_OPT_LEN (NMRP_OPT_HEADER_LEN + STRING_TABLE_FILENAME_LEN)

#define NMRP_MAX_OPT_PER_MSG 6

typedef struct {
    unsigned short type;
    unsigned short len;
    union {
        unsigned int magicno;
        struct{
            unsigned char addr[IP_LEN];
            unsigned char mask[IP_LEN];
        }ip;
        unsigned short region;
        unsigned int string_table_bitmask;
    } value;
}__attribute__ ((packed)) nmrp_parsed_opt_t;

typedef struct {
    unsigned short reserved;
    unsigned char code;
    unsigned char id;
    unsigned short length;
    int numOptions;
    nmrp_parsed_opt_t options[NMRP_MAX_OPT_PER_MSG];
}__attribute__ ((packed)) nmrp_parsed_msg_t;

typedef struct {
    uchar *eb_ptr;
    int eb_length;
}pkt_info_t;

void NmrpInit(void);
void NmrpStart(void);
void Nmrp_cleanup(void);
void NmrpReceive(struct ethernet_hdr *et, struct ip_udp_hdr *ip, int len);
void Nmrp_setState(int state);
int Nmrp_getState(void);

#endif /* __SNTP_H__ */
