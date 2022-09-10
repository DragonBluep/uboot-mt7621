/*
*	TFTP server     File: cmd_tftpServer.c
*/

#include <image.h>
#if defined(CONFIG_ASUS_PRODUCT)
#if !defined(CONFIG_SOC_MT7621)
#include <gpio.h>
#endif
#include <bootm.h>
#include <cmd_tftpServer.h>
#include <malloc.h>
#include <errno.h>
#include <linux/delay.h>
#if !defined(CONFIG_SOC_MT7621)
#include <flash_wrapper.h>
#endif
int reset_to_default(void);

#if defined(CFG_UBI_SUPPORT)
#include "../drivers/mtd/ubi/ubi-media.h"

#if defined(CONFIG_SOC_MT7621)
extern int boot_write_img(ulong buf_addr, ulong addr, ulong size);
#endif

#endif



#ifdef CFG_DIRECT_FLASH_TFTP
extern flash_info_t flash_info[CFG_MAX_FLASH_BANKS];/* info for FLASH chips   */
#endif

const unsigned char trx_magic[4] =		"\x27\x05\x19\x56";	/* Image Magic Number */
const unsigned char nvram_magic[4] =		"\x46\x4C\x53\x48";	/* 'FLSH' */
const unsigned char nvram_magic_mac0[4] =	"\x4D\x41\x43\x30";	/* 'MAC0' Added by PaN */
const unsigned char nvram_magic_mac1[4] =	"\x4D\x41\x43\x31";	/* 'MAC1' Added by PaN */
const unsigned char nvram_magic_rdom[4] =	"\x52\x44\x4F\x4D";	/* 'RDOM' Added by PaN */
const unsigned char nvram_magic_asus[4] =	"\x41\x53\x55\x53";	/* 'ASUS' Added by PaN */

static int	TftpServerPort;		/* The UDP port at their end		*/
static int	TftpOurPort;		/* The UDP port at our end		*/
static int	TftpTimeoutCount;
static ulong	TftpBlock;		/* packet sequence number		*/
static ulong	TftpLastBlock;		/* last packet sequence number received */
static ulong	TftpBlockWrapOffset;	/* memory offset due to wrapping	*/
static int	TftpState;

int g_rescue_mode = 0;
int g_led_intv = 1;


#if !defined(CONFIG_SOC_MT7621)
uchar asuslink[] = "ASUSSPACELINK";
uchar maclink[] = "snxxxxxxxxxxx";
uint16_t RescueAckFlag = 0;
#else
extern uchar asuslink[];
extern uchar maclink[];
extern uint16_t RescueAckFlag ;
#endif
#define PTR_SIZE	0x800000	// 0x390000
#define BOOTBUF_SIZE	0x30000
uchar MAC0[20], RDOM[7], ASUS[24], nvramcmd[60];
unsigned char *ptr, *tail;
uint32_t copysize = 0;
uint32_t offset = 0;
int rc = 0;
int MAC_FLAG = 0;
int env_loc = 0;
int ubi_damaged = 0;

static void _tftpd_open(void);
static void TftpHandler(uchar * pkt, unsigned dport, struct in_addr sip, unsigned sport, unsigned len);
static void RAMtoFlash(void);
#if !defined(CONFIG_SOC_MT7621)
static void SolveTRX(void);
#else
extern void SolveTRX(void);
#endif
extern image_header_t header;
extern int do_bootm(cmd_tbl_t *, int, int, char * const []);
extern int do_reset(cmd_tbl_t *, int, int, char * const []);

extern struct in_addr TempServerIP;
extern bootm_headers_t images;		/* pointers to os/initrd/fdt images */


#if !defined(CONFIG_SOC_MT7621)
/*
 * @return:
 * 	>0:	firmware good, length of firmware
 *	=0:	not defined
 *	<0:	invalid firmware
 */
int check_trx(int argc, char * const argv[])
{
	cmd_tbl_t cmd_t;

	images.verify = 1;
	/*
	if (ptr)
		images.rescue_image = 1;
	*/
	argc--; argv++;
	if (!do_bootm_states(&cmd_t, 0, argc, argv, BOOTM_STATE_START | BOOTM_STATE_FINDOS, &images, 1))
		return sizeof(image_header_t) + images.os.image_len;

	return -1;
}
#endif

int program_bootloader(ulong addr, ulong len)
{
	void *hdr = (void *)addr;
	int ret = 0;
	const ulong max_xfer_size = MAX_TRX_IMAGE_SIZE;

	if (len > max_xfer_size) {
		printf("Abort: Bootloader is too big or download aborted!\n");
		return -1;
	}

#if !defined(CONFIG_SOC_MT7621)
	switch (genimg_get_format(hdr)) {
	case IMAGE_FORMAT_LEGACY:
		/* Only FIT image is supported! */
		return -2;

#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		if (fit_check_format(hdr, len)) {
			puts("Bad FIT image format!\n");
			return -3;
		}

		if (!fit_all_image_verify(hdr)) {
			puts("Bad hash in FIT image!\n");
			return -4;
		}

		break;
#endif
	default:
		puts("Unknown FIT image format!\n");
		return -5;
	}
	//fit_print_contents(addr);
	ret = program_bootloader_fit_image(addr);
#else
	//for uboot partition:0x0000~0xe0000
	if(boot_write_img(addr, 0, NAND_NVRAM_OFFSET))
		ret = 1; //error
#endif	
	if (ret)
		printf("program err ret:%d\n", ret);
	return ret;
}

static void TftpdSend(void)
{
	volatile uchar *pkt;
	volatile uchar *xp;
	volatile ushort *s;
	int	len = 0;
	/*
	*	We will always be sending some sort of packet, so
	*	cobble together the packet headers now.
	*/
	pkt = net_tx_packet + net_eth_hdr_size() + IP_UDP_HDR_SIZE;

	switch (TftpState)
	{
	case STATE_RRQ:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_DATA);
		*s++ = htons(TftpBlock);/*fullfill the data part*/
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

	case STATE_WRQ:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

#ifdef ET_DEBUG
		printf("send option \"timeout %s\"\n", (char *)pkt);
#endif
		pkt += strlen((char *)pkt) + 1;
		len = pkt - xp;
		break;

	case STATE_DATA:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

	case STATE_FINISHACK:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_FINISHACK);
		*s++ = htons(RescueAckFlag);
		pkt = (uchar*)s;
		len = pkt - xp;
		break;

	case STATE_TOO_LARGE:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(3);
		pkt = (uchar*)s;
		strcpy((char *)pkt, "File too large");
		pkt += 14 /*strlen("File too large")*/ + 1;
		len = pkt - xp;
		break;

	case STATE_BAD_MAGIC:
		xp = pkt;
		s = (ushort*)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(2);
		pkt = (uchar*)s;
		strcpy((char *)pkt, "File has bad magic");
		pkt += 18 /*strlen("File has bad magic")*/ + 1;
		len = pkt - xp;
		break;
	}

	net_send_udp_packet(net_server_ethaddr, net_server_ip, TftpServerPort, TftpOurPort, len);
}

static void TftpTimeout(void)
{
	puts("T ");
	net_set_timeout_handler(TFTPD_TIMEOUT * CONFIG_SYS_HZ, TftpTimeout);
	TftpdSend();
}

static void TftpdTimeout(void)
{
	puts("D ");
	net_set_timeout_handler(TFTPD_TIMEOUT * CONFIG_SYS_HZ, TftpdTimeout);
}
#if !defined(CONFIG_SOC_MT7621)
static char* get_addr_name(ulong addr)
{
	static char name[128] = "image";

	if (addr == CFG_KERN_ADDR)
		strcpy(name, "1st firmware");
#if defined(CFG_DUAL_TRX)
	else if (addr == cfg_kern2_addr)
		strcpy(name, "2nd firmware");
#endif

	return name;
}

// return 0: success
// others: fail
static int get_firmware_image(ulong img_addr, ulong ram_addr, ulong *total_len_pt)
{
	ulong h_size, d_size;

	/* get header size */
	h_size = sizeof(struct fdt_header);
	*total_len_pt = 0;

	/* read in header */
	debug("   Reading image header from flash address "
		"%08lx to RAM address %08lx\n", ra_flash_offset(img_addr), ram_addr);

	if (ra_flash_read((uchar*) ram_addr, img_addr, h_size))
		return -1;

	/* get data size */
	switch (genimg_get_format((void *)ram_addr)) {
	case IMAGE_FORMAT_LEGACY:
		d_size = image_get_data_size((const image_header_t *)ram_addr);
		debug("   Legacy format image found at 0x%08lx, size 0x%08lx\n", ram_addr, d_size);
		break;
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
			d_size = fit_get_size((const void *)ram_addr) - h_size;
			printf("   FIT/FDT format image found at 0x%lx,size 0x%lx\n", ram_addr, d_size);
			break;
#endif
	default:
		printf("   No valid %s found at 0x%08lx\n",
			get_addr_name(img_addr), ra_flash_offset(img_addr));
		return -2;
	}

	/* read in image data */
#if defined(CFG_UBI_SUPPORT)
	debug("   Reading image full data from flash address "
		"%lx to RAM address %lx\n", ra_flash_offset(img_addr), ram_addr );
	if (ra_flash_read((uchar*) ram_addr, img_addr , d_size + h_size))
		return -1;
#else
	printf("   Reading image remaining data from flash address "
		"%lx to RAM address %lx\n", ra_flash_offset(img_addr + h_size),
		ram_addr + h_size);

	if (ra_flash_read((uchar*)(ram_addr + h_size), img_addr + h_size, d_size))
		return -1;
#endif
	*total_len_pt = h_size + d_size;
	return 0;
}

static ulong get_fitimg_total_size(ulong flash_addr)
{
	ulong h_size, d_size;
	unsigned char tmp_buf[sizeof(struct fdt_header)];
	struct fdt_header *fdt_pt;
	h_size = sizeof(struct fdt_header);
	ra_flash_read(tmp_buf, flash_addr, h_size);
	fdt_pt = (struct fdt_header *)tmp_buf;
	if (fdt_magic(fdt_pt) == FDT_MAGIC)
		d_size = fit_get_size(fdt_pt);
	else
		d_size = 0;
	return d_size;
}

static int load_asus_firmware(void)
{
	int ret;
	char *argv[2] = {"", NULL};
	char ram_addr_str[11+20];
	ulong img_len1;
	cmd_tbl_t c, *cmdtp = &c;

	sprintf(ram_addr_str, "0x%X#config-1", CONFIG_SYS_LOAD_ADDR);
#if defined(CFG_DUAL_TRX)
	argv[0] = &ram_addr_str[0];
#else
	argv[1] = &ram_addr_str[0];
#endif
	ret = get_firmware_image(CFG_KERN_ADDR, CONFIG_SYS_LOAD_ADDR, &img_len1);
	if (ret == 0) {
#if defined(CFG_DUAL_TRX)
		// check image integrity
		ret = do_bootm_states(cmdtp, 0, 1, argv, BOOTM_STATE_START |
			BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER
			, &images, 1);
#else
		ret = do_bootm(cmdtp, 0, 2, argv);
#endif
	}
#if defined(CFG_DUAL_TRX)
	if (ret) { /* 1st image fail, try 2nd image */
		ulong img_len2;
		ret = get_firmware_image(CFG_KERN2_ADDR, CONFIG_SYS_LOAD_ADDR, &img_len2);
		if (ret == 0) { /* 2nd image success */
			// check image integrity
			ret = do_bootm_states(cmdtp, 0, 1, argv, BOOTM_STATE_START |
				BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER
				, &images, 1);
			/* copy linux2 to linux1 */
			if (ret == 0) { // correct image
				printf("Copy firmware from linux2(%x) to linux1(%x), length %lx\n", CFG_KERN2_ADDR, CFG_KERN_ADDR, img_len2);
				ra_flash_erase_write((uchar *)CONFIG_SYS_LOAD_ADDR, CFG_KERN_ADDR, img_len2, 0);
			}
		}
	} else {
		/* check linux2 & copy linux1 to linux2 if neccessary */
		ulong img_len2;
		img_len2 = get_fitimg_total_size(CFG_KERN2_ADDR);
		if (img_len1 &&  img_len1 != img_len2) {
			printf("Copy firmware from linux1(%x) to linux2(%x), length %lx\n", CFG_KERN_ADDR, CFG_KERN2_ADDR, img_len1);
			ra_flash_erase_write((uchar *)CONFIG_SYS_LOAD_ADDR, CFG_KERN2_ADDR, img_len1, 0);
		}
	}
	if (ret == 0)
		ret = do_bootm_states(cmdtp, 0, 1, argv, BOOTM_STATE_LOADOS | BOOTM_STATE_OS_PREP | BOOTM_STATE_OS_GO, &images, 1);
#endif
	if (ret) {
#if defined(PLAX56_XP4) || defined(ETJ)
		asus_purple_led_on();
#endif
		printf(" \nHello!! Enter Recuse Mode: (Check error)\n\n");
		if (net_loop(TFTPD) < 0)
			return -1;
	}

	return 0;
}

int do_tftpd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;

	const int press_times = 1;
	int i = 0;

	if (DETECT())		/* Reset to default button */
	{
		wan_red_led_on();
		printf(" \n## Enter Rescue Mode ##\n");
		env_set("autostart", "no");
#if defined(PLAX56_XP4) || defined(ETJ)
		asus_purple_led_on();
#endif
		/* Wait forever for an image */
		if (net_loop(TFTPD) < 0)
			return 1;
	}
	else if (DETECT_WPS())	/* WPS button */
	{
		/* Make sure WPS button is pressed at least press_times * 0.01s. */
		while (DETECT_WPS() && i++ < press_times) {
			mdelay(10);
		}

		if (i >= press_times) {
			while (DETECT_WPS()) {
				mdelay(90);
				i++;
				if (i & 1) {
#if defined(PLAX56_XP4) || defined(ETJ)
					asus_red_led_on();
#else
					power_led_on();
#endif
				}
				else {
#if defined(PLAX56_XP4) || defined(ETJ)
					asus_red_led_off();
#else
					power_led_off();
#endif
				}
			}
			leds_off();

			reset_to_default();
			do_reset (NULL, 0, 0, NULL);
		}
	}
	else
	{
		ret = load_asus_firmware();
	}

	return ret;
}

U_BOOT_CMD(
	tftpd, 1, 1, do_tftpd,
	"tftpd\t -load the data by tftp protocol\n",
	NULL
);
#endif //CONFIG_SOC_MT7621
void TftpdStart(void)
{
	ptr = (unsigned char*) CONFIG_SYS_LOAD_ADDR;

#if defined(CONFIG_NET_MULTI)
	printf("Using %s device\n", eth_get_name());
#endif
	//puts(" \nTFTP from server ");	print_IPaddr(net_server_ip);
	printf("\nOur IP address is:(%pI4)\nWait for TFTP request...\n", &net_ip);
	/* Check if we need to send across this subnet */
	if (net_gateway.s_addr && net_netmask.s_addr) {
		struct in_addr OurNet, ServerNet;

		OurNet.s_addr = net_ip.s_addr & net_netmask.s_addr;
		ServerNet.s_addr = net_server_ip.s_addr & net_netmask.s_addr;

		if (OurNet.s_addr != ServerNet.s_addr)
			printf("sending through gateway %pI4\n", &net_gateway);
	}

	memset(ptr,0,sizeof(ptr));
	_tftpd_open();
}

static void _tftpd_open()
{

	printf("tftpd open\n");	// tmp test
	net_set_timeout_handler(TFTPD_TIMEOUT * CONFIG_SYS_HZ * 2, TftpdTimeout);
	net_set_udp_handler(TftpHandler);

	TftpOurPort = PORT_TFTP;
	TftpTimeoutCount = 0;
	TftpState = STATE_RRQ;
	TftpBlock = 0;

	/* zero out server ether in case the server ip has changed */
	memset(net_server_ethaddr, 0, 6);
}


static void TftpHandler(uchar * pkt, unsigned dport, struct in_addr sip, unsigned sport, unsigned len)
{
	ushort proto;
	volatile ushort *s;

	if (dport != TftpOurPort)
	{
		return;
	}
	/* don't care the packets that donot send to TFTP port */

	if (TftpState != STATE_RRQ && sport != TftpServerPort)
	{
		return;
	}

	if (len < 2)
	{
		return;
	}

	len -= 2;
	/* warning: don't use increment (++) in ntohs() macros!! */
	s = (ushort*)pkt;
	proto = *s++;
	pkt = (uchar*)s;

	switch (ntohs(proto))
	{
	case TFTP_RRQ:

		printf("\n Get read request from:(%pI4)\n", &TempServerIP);
		net_copy_ip(&net_server_ip,&TempServerIP);

		TftpServerPort = sport;
		TftpBlock = 1;
		TftpBlockWrapOffset = 0;
		TftpState = STATE_RRQ;

		if (!memcmp(pkt, asuslink, 13) || !memcmp(pkt, maclink, 13)) {
			/* it's the firmware transmitting situation */
			/* here get the IP address in little-endian from the first packet. */
			NetCopy_LEIP(&net_ip, pkt + 13);
			printf("Firmware Restoration assigns IP address: %pI4\n", &net_ip);
		}

                /* Adjust subnetmask if net_ip and net_server_ip are not in the same domain.
		 * If there are two IP address, in different subnet, are assigned to one network adapter.
		 *
		 * Sometimes, IP address of PC may changes, e.g. 169.254.x.y ==> 192.168.1.x.
		 * But, Firmware Restoration doesn't updates it's IP address and assigns IP address in first subnet to us.
		 * Below patch guarantees TFTP packst which is used to response ASPSSPACELINK is send to PC.
		 * But, Firmware Restoration may not be able to send TFTP data packet which contains rescue image to us
		 * due to those packets are sended to PC's default gateway instead.
		 */
		if ((net_ip.s_addr & net_netmask.s_addr) != (net_server_ip.s_addr & net_netmask.s_addr)) {
			while ( net_netmask.s_addr &&
				((net_ip.s_addr & net_netmask.s_addr) !=
				 (net_server_ip.s_addr & net_netmask.s_addr))) {
                                net_netmask.s_addr <<= 1;
                        }
			printf("New netmask: %pI4\n", &net_netmask);
		}

		TftpdSend();//send a vacant Data packet as a ACK
		break;

	case TFTP_WRQ:
		TftpServerPort = sport;
		TftpBlock = 0;
		TftpState = STATE_WRQ;
		TftpdSend();
		tail = ptr;
		copysize = 0;
		break;

	case TFTP_DATA:
		//printf("case TFTPDATA\n");	// tmp test
		if (len < 2)
			return;
		len -= 2;
		TftpBlock = ntohs(*(ushort *)pkt);
		/*
		* RFC1350 specifies that the first data packet will
		* have sequence number 1. If we receive a sequence
		* number of 0 this means that there was a wrap
		* around of the (16 bit) counter.
		*/
		if (TftpBlock == 0)
		{
			printf("\n\t %lu MB reveived\n\t ", TftpBlockWrapOffset>>20);
		}
		else
		{
			if (((TftpBlock - 1) % 10) == 0)
			{/* print out progress bar */
				puts("#");
			}
			else
				if ((TftpBlock % (10 * HASHES_PER_LINE)) == 0)
				{
					puts("\n");
				}
#if defined(PLAX56_XP4) || defined(ETJ) // rescue process indication
			if (((TftpBlock - 1) % 1600) == 0)
				asus_purple_led_on();
			else if (((TftpBlock - 1) % 800) == 0)
				asus_yellow_led_on();
#endif
		}
		if (TftpState == STATE_WRQ)
		{
			/* first block received */
			TftpState = STATE_DATA;
			TftpServerPort = sport;
			TftpLastBlock = 0;
			TftpBlockWrapOffset = 0;
			printf("\n First block received  \n");
			//printf("Load Addr is %x\n", ptr);	// tmp test
			//ptr = 0x80100000;

			if (TftpBlock != 1)
			{	/* Assertion */
				printf("\nTFTP error: "
					"First block is not block 1 (%ld)\n"
					"Starting again\n\n",
					TftpBlock);
				net_start_again();
				break;
			}
		}

		if (TftpBlock == TftpLastBlock)
		{	/* Same block again; ignore it. */
			printf("\n Same block again; ignore it \n");
			break;
		}
		TftpLastBlock = TftpBlock;
		net_set_timeout_handler(TFTPD_TIMEOUT * CONFIG_SYS_HZ, TftpTimeout);

		copysize += len;	/* the total length of the data */
		(void)memcpy(tail, pkt + 2, len);/* store the data part to RAM */
		tail += len;

		/*
		*	Acknowledge the block just received, which will prompt
		*	the Server for the next one.
		*/
		TftpdSend();

		if (len < TFTP_BLOCK_SIZE)
		{
		/*
		*	We received the whole thing.  Try to run it.
		*/
			puts("\ndone\n");
			TftpState = STATE_FINISHACK;
			net_set_state(NETLOOP_SUCCESS);
			RAMtoFlash();
		}
		break;

	case TFTP_ERROR:
		printf("\nTFTP error: '%s' (%d)\n",
			pkt + 2, ntohs(*(ushort *)pkt));
		puts("Starting again\n\n");
		net_start_again();
		break;

	default:
		break;

	}
}

void flash_perrormsg (int err)
{
	printf("Flash error code: %d\n", err);
}

#if defined(CFG_UBI_SUPPORT)
/* Program UBI image with or without OOB information to UBI_DEV.  */
int SolveUBI(void)
{
	printf("Solve UBI, ptr=0x%p\n", ptr);
	return __SolveUBI(ptr, CFG_UBI_DEV_OFFSET, copysize);
}
#endif

static void RAMtoFlash(void)
{
	int i=0, parseflag=0;
	void *hdr;
	int j=0;
	uchar mac_temp[7],secretcode[9];
	uint8_t SCODE[5] = {0x53, 0x43, 0x4F, 0x44, 0x45};
	char *end;
	unsigned char *tmp;

	printf("RAMtoFLASH\n");	// tmp test
	/* Check for the TRX magic. If it's a TRX image, then proceed to flash it. */
	if (!memcmp(ptr, trx_magic, sizeof(trx_magic)) && (copysize > MAX_TRX_IMAGE_SIZE))
	{
		printf("Chk trx magic\n");	// tmp test
		printf("Download of 0x%x bytes completed\n", copysize);
		printf("Check TRX and write it to FLASH \n");
		SolveTRX();
	}
#if defined(CFG_UBI_SUPPORT)
	else if (be32_to_cpu(*(uint32_t *)ptr) == UBI_EC_HDR_MAGIC) {
		int r;

		printf("Check UBI magic\n");
		printf("Download of 0x%x bytes completed\n", copysize);
		if ((r = SolveUBI()) != 0) {
			printf("rescue UBI image failed! (r = %d)\n", r);
			net_set_state(NETLOOP_FAIL);
			TftpState = STATE_FINISHACK;
			RescueAckFlag = 0x0000;
			for (i = 0; i < 6; i++)
				TftpdSend();
			return;
		} else {
			printf("done. %d bytes written\n", copysize);
			TftpState = STATE_FINISHACK;
			RescueAckFlag = 0x0001;
			for (i = 0; i < 6; i++)
				TftpdSend();
		}

		printf("SYSTEM RESET!!!\n\n");
		do_reset (NULL, 0, 0, NULL);
	}
#endif
	else
	{
		const char *q;
		unsigned int magic_offset = 0x2FFF0;

		if (MAX_TRX_IMAGE_SIZE > 0x30000)
			magic_offset = MAX_TRX_IMAGE_SIZE - 7;

		printf("Magic number offset: %x\n", magic_offset);
		q = (char*)(ptr + magic_offset);
		printf("upgrade boot code\n");	// tmp test

		if (copysize > 0 && copysize <= MAX_TRX_IMAGE_SIZE)		// uboot
		{
			/* ptr + magic_offset may not aligned to 4-bytes boundary.
			 * use char pointer to access it instead.
			 */
			if (copysize >= (magic_offset+4) &&
				!memcmp(q, nvram_magic, sizeof(nvram_magic)) &&
				memcmp(ptr, nvram_magic_mac0, sizeof(nvram_magic_mac0)) &&
				memcmp(ptr, nvram_magic_mac1, sizeof(nvram_magic_mac1)) &&
				memcmp(ptr, nvram_magic_rdom, sizeof(nvram_magic_rdom)) &&
				memcmp(ptr, nvram_magic_asus, sizeof(nvram_magic_asus)))
			{
				printf(" Download of 0x%x bytes completed\n", copysize);
				printf("Write bootloader binary to FLASH \n");
				parseflag = 1;
			}
			else if (!memcmp(ptr, nvram_magic_mac0, sizeof(nvram_magic_mac0)))
			{
				printf("  Download of 0x%x bytes completed\n", copysize);
				memset(MAC0, 0, sizeof(MAC0));
				for (i=0; i<17; i++)
					MAC0[i] = ptr[4+i];

				i = i + 4;
				/* Debug message */
				if ((ptr[i]==SCODE[0])
					&&(ptr[i+1]== SCODE[1])
					&&(ptr[i+2]== SCODE[2])
					&&(ptr[i+3]== SCODE[3])
					&&(ptr[i+4]== SCODE[4]))
				{
					for (i=26,j=0; i<34; i++)
					{
						secretcode[j] = ptr[i];
						printf("secretcode[%d]=%x, ptr[%d]=%x\n", j, secretcode[j], i, ptr[i]);
						j++;
					}
					secretcode[j]='\0';
					printf("Write secret code = %s to FLASH\n", secretcode);
					rc = replace(0x100, secretcode, 9);

				}
				tmp=MAC0;
				for (i=0; i<6; i++)
				{
					mac_temp[i] = tmp ? simple_strtoul((char*) tmp, &end, 16) : 0;
					if (tmp)
						tmp= (uchar*) ((*end) ? end+1 : end);
				}
				mac_temp[i] = '\0';

				printf("Write MAC0 = %s  to FLASH \n", MAC0);
				MAC_FLAG=1;
				rc = replace(0x4, mac_temp, 7);
				sprintf((char*) nvramcmd, "nvram set et0macaddr=%s", (char*) MAC0);
				printf("%s\n", nvramcmd);

				parseflag = 2;
			}
			else if (!memcmp(ptr, nvram_magic_rdom, sizeof(nvram_magic_rdom)))
			{
				for (i=0; i<6; i++)
					RDOM[i] = ptr[8+i];
				RDOM[i] = '\0';
				rc = replace(0x4e, RDOM, 7);
				printf("Write RDOM = %s  to FLASH \n", RDOM);
				sprintf((char*) nvramcmd, "nvram set regulation_domain=%s", (char*) RDOM);
				printf("%s\n", nvramcmd);
				parseflag = 3;
			}
			else if (!memcmp(ptr, nvram_magic_asus, sizeof(nvram_magic_asus)))
			{
				memset(ASUS, 0, sizeof(ASUS));
				for (i=0; i<23; i++)
					ASUS[i] = ptr[4+i];
				memset(MAC0, 0, sizeof(MAC0));
				for (i=0; i<17; i++)
					MAC0[i] = ASUS[i];
				memset(RDOM, 0, sizeof(RDOM));
				rc = replace(0x4, mac_temp, 7);
				for (i=0; i<6; i++)
					RDOM[i] = ASUS[17+i];
				printf("Write MAC0 = %s  to FLASH \n", (char*) MAC0);
				sprintf((char*) nvramcmd, "nvram set et0macaddr=%s", (char*) MAC0);
				printf("%s\n", nvramcmd);
				rc = replace(0x4e, RDOM, 7);
				printf("Write RDOM = %s  to FLASH \n", RDOM);
				sprintf((char*) nvramcmd, "nvram set regulation_domain=%s", (char*) RDOM);
				printf("%s\n", nvramcmd);
				parseflag = 4;
			}
			else
			{
				parseflag = -1;
				printf("Download of 0x%x bytes completed\n", copysize);
				printf("Warning, Bad Magic word!!\n");
				net_set_state(STATE_BAD_MAGIC);
				TftpdSend();
				copysize = 0;
			}

		}
		else if (copysize > MAX_TRX_IMAGE_SIZE)
		{
			parseflag = 0;
			printf("    Download of 0x%x bytes completed\n", copysize);
		}
		else
		{
			parseflag = -1;
			copysize = 0;
			printf("Downloading image time out\n");
		}


		if (copysize == 0)
			return;    /* 0 bytes, don't flash */

		if (parseflag != 0)
			copysize = MAX_TRX_IMAGE_SIZE;

		if (parseflag == 1 && memcmp(ptr, trx_magic, sizeof(trx_magic))) {
			printf("Not uboot trx image!\n");
			parseflag = -1;
			rc = -1;
		}

#if !defined(CONFIG_SOC_MT7621)
		/* skip .trx header */
		hdr = (void*) (ptr + sizeof(image_header_t));
		copysize -= sizeof(image_header_t);
		if (parseflag == 1 && genimg_get_format(hdr) != IMAGE_FORMAT_FIT) {
			puts("Unknown uboot FIT image format!\n");
			parseflag = -1;
			rc = -1;
		}
#else
	       hdr= (unsigned long)ptr;
#endif		
		printf("parseflag %d\n", parseflag);
		if (parseflag == 1 && (rc = program_bootloader((ulong)hdr, copysize)) != 0)
			parseflag = -1;

		if (rc) {
			printf("rescue failed!\n");
			flash_perrormsg(rc);
			net_set_state(NETLOOP_FAIL);
			/* Network loop state */
			TftpState = STATE_FINISHACK;
			RescueAckFlag= 0x0000;
			for (i=0; i<6; i++)
				TftpdSend();
			return;
		}
		else {
			printf("done. %d bytes written\n",copysize);
			TftpState = STATE_FINISHACK;
			RescueAckFlag= 0x0001;
			for (i=0; i<6; i++)
				TftpdSend();
			if (parseflag != 0)
			{
				printf("SYSTEM RESET \n\n");
				udelay(500);
				do_reset(NULL, 0, 0, NULL);
			}
			return;
		}
	}
}

#if !defined(CONFIG_SOC_MT7621)
static void SolveTRX(void)
{
	int  i = 0;
	int rc = 0;
	ulong count = 0;
	int e_end = 0;
	int reset = 0;

	printf("Solve TRX, ptr=0x%x\n", (uint) ptr);
	image_load_addr = (unsigned long)ptr;
	if (check_trx(0, NULL) <= 0) {
		rc = reset = 1;
		printf("Check trx error!\n\n");
	}
	else {
		count = copysize;

		if (count == 0) {
			puts ("Zero length ???\n");
			return;
		}

		e_end = CFG_KERN_ADDR + count;

		printf("\n Erase kernel block !!\n From %x To %x (%lu/h:%lx)\n",
			CFG_KERN_ADDR, e_end, count, count);
		rc = ra_flash_erase_write(ptr+sizeof(image_header_t), CFG_KERN_ADDR, count-sizeof(image_header_t), 0);
#if defined(CFG_DUAL_TRX)
		if (rc)
			printf("Write 1st firmware fail. (r = %d)\n", rc);
		rc = ra_flash_erase_write(ptr+sizeof(image_header_t), CFG_KERN2_ADDR, count-sizeof(image_header_t), 0);
		if (rc)
			printf("Write 2nd firmware fail. (r = %d)\n", rc);
#else
		if (rc)
			printf("Write firmware fail. (r = %d)\n", rc);
#endif
	}

	if (rc) {
		printf("rescue failed! (%d)\n", rc);
		net_set_state(NETLOOP_FAIL);
		TftpState = STATE_FINISHACK;
		RescueAckFlag= 0x0000;
		for (i=0; i<6; i++)
			TftpdSend();
	}
	else {
		printf("done. %lu bytes written\n", count);
		TftpState = STATE_FINISHACK;
		RescueAckFlag= 0x0001;
		for (i=0; i<6; i++)
			TftpdSend();
		reset = 1;
	}

	if (reset) {
		printf("\nSYSTEM RESET!!!\n\n");
		udelay(500);
		do_reset(NULL, 0, 0, NULL);
	}
}
#endif
#endif	/* CONFIG_ASUS_PRODUCT */
