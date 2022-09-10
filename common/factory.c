// vim:cin
/* 
 * Copyright 2013, ASUSTeK Inc.
 * All Rights Reserved.
 */

#include <common.h>
#if defined(CONFIG_BOARD_MT7621_NAND_RFB)
#include <configs/mt7621_nand.h>
#endif

#define EEPROM_SET_HEADER_OFFSET	(MAX_NAND_BLOCK_SIZE-MAX_NAND_PAGE_SIZE)
#define MAX_EEPROM_SET_LENGTH		EEPROM_SET_HEADER_OFFSET

#define FACTORY_IMAGE_MAGIC	0x46545259	/* 'F', 'T', 'R', 'Y' */
typedef struct eeprom_set_hdr_s {
	uint32_t ih_magic;	/* Image Header Magic Number = 'F', 'T', 'R', 'Y' */
	uint32_t ih_hcrc;	/* Image Header CRC Checksum    */
	uint32_t ih_hdr_ver;	/* Image Header Version Number  */
	uint32_t ih_write_ver;	/* Number of writes             */
	uint32_t ih_dcrc;	/* Image Data CRC Checksum      */
} eeprom_set_hdr_t;

extern int do_nmbm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
//(r = ra_flash_read(factory, CONFIG_NAND_RF_OFFSET, CONFIG_MAX_NAND_BLOCK_SIZE)) != 0)
int ra_flash_read(uchar *buf, ulong addr, ulong len)
{
	int ret = 0;
    char s_mem_addr[32] = "";
    char s_addr[32] = "";
    char s_size[32] = "";
    char *nmbm_read_argv[] = { NULL, "nmbm0", "read", s_mem_addr, s_addr, s_size };

	if (!buf || !len || !addr) {
		debug("%s(): invalid parameter. buf %p addr 0x%08lx len 0x%08lx\n",
			__func__, buf, addr, len);
		return -1;
	}

    snprintf(s_mem_addr, sizeof(s_mem_addr), "%p", buf);
    snprintf(s_addr, sizeof(s_addr), "0x%lx", addr);
    snprintf(s_size, sizeof(s_size), "0x%lx", len);

    ret = do_nmbm(NULL, 0, ARRAY_SIZE(nmbm_read_argv), nmbm_read_argv);
    if (ret != 0)
    {
        printf("nmbm nmbm0 read %s %s %s, error!!\n", s_mem_addr, s_addr, s_size);
        return -1;
    }

    return 0;
}

int ra_flash_erase_write(uchar *buf, ulong addr, ulong len)
{
    int rc = 0;

    char s_mem_addr[32] = "";
    char s_addr[32] = "";
    char s_size[32] = "";
    char *nmbm_erase_argv[] = { NULL, "nmbm0", "erase", s_addr, s_size };
    char *nmbm_write_argv[] = { NULL, "nmbm0", "write", s_mem_addr, s_addr, s_size };

	if (!buf || !len || !addr) {
		debug("%s(): invalid parameter. buf %p addr 0x%08lx len 0x%08lx\n",
			__func__, buf, addr, len);
		return -1;
	}

    snprintf(s_mem_addr, sizeof(s_mem_addr), "%p", buf);
    snprintf(s_addr, sizeof(s_addr), "0x%lx", addr);
    snprintf(s_size, sizeof(s_size), "0x%lx", len);


    rc = do_nmbm(NULL, 0, ARRAY_SIZE(nmbm_erase_argv), nmbm_erase_argv);
    if (rc != 0)
    {
        printf("nmbm nmbm0 erase.spread %s %s, error!!\n", s_addr, s_size);
        return -1;
    }

    rc = do_nmbm(NULL, 0, ARRAY_SIZE(nmbm_write_argv), nmbm_write_argv);
    if (rc != 0)
    {
        printf("nmbm nmbm0 write %s %s %s, error !!\n", s_mem_addr, s_addr, s_size);
        return -1;
    }

    return 0;
}
//extern uint32_t crc32 (uint32_t, const unsigned char *, unsigned int);
/* Check EEPROM set checksum in RAM.
 * @buf:	pointer to one copy of EEPROM set.
 * 		length of the EEPROM set must greater than or equal to LEB_SIZE
 * @return:
 *     >=0:	OK, write times of the EEPROM set
 * 	-1:	Invalid parameter.
 * 	-2:	Invalid magic number.
 * 	-3:	Invalid header checksum.
 * 	-4:	Invalid data checksum
 *  otherwise:	Not defined.
 */
static int check_eeprom_checksum(unsigned char *buf)
{
	unsigned long hcrc, checksum;
	eeprom_set_hdr_t header, *hdr = &header;

	if (!buf)
		return -1;

	memcpy(hdr, buf + EEPROM_SET_HEADER_OFFSET, sizeof(eeprom_set_hdr_t));
	if (hdr->ih_magic != htonl(FACTORY_IMAGE_MAGIC))
		return -2;

	hcrc = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;
	/* check header checksum */
	checksum = crc32(0, (void *) hdr, sizeof(eeprom_set_hdr_t));
	if (hcrc != checksum) {
		printf("Header checksum mismatch. (%lx/%lx)\n", hcrc, checksum);
		return -3;
	}

	/* check image checksum */
	checksum = crc32(0, (void *) buf, EEPROM_SET_HEADER_OFFSET);
	if (ntohl(hdr->ih_dcrc) != checksum) {
		printf("Data checksum mismatch. (%x/%lx)\n", ntohl(hdr->ih_dcrc), checksum);
		return -4;
	}

	return ntohl(hdr->ih_write_ver);
}

/**
 * Initialize Flash layout.
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int ra_flash_init_layout(void)
{
	unsigned char factory[MAX_NAND_BLOCK_SIZE], factory2[MAX_NAND_BLOCK_SIZE];
	int r;
	int factory_r = 0, factory2_r = 0;

	if ((r = ra_flash_read(factory, NAND_RF_OFFSET, MAX_NAND_BLOCK_SIZE)) != 0) {
		printf("Read Factory fail! (r = %d)\n", r);
		factory_r = -2;
	}
	else
	{
		if ((r = check_eeprom_checksum(factory)) < 0) {
			printf("Invalid Factory's EEPROM! (r = %d)\n", factory_r);
			factory_r = -3;
		}
		else
		{
			printf("Check Factory's EEPROM successfully! (r = %d)\n", r);
			factory_r = 1;
		}
	}

	if ((r = ra_flash_read(factory2, NAND_RF_OFFSET_2, MAX_NAND_BLOCK_SIZE)) != 0) {
		printf("Read Factory2 fail! (r = %d)\n", r);
		factory2_r = -2;
	}
	else
	{
		if ((r = check_eeprom_checksum(factory2)) < 0) {
			printf("Invalid Factory2's EEPROM! (r = %d)\n", r);
			factory2_r = -3;
		}
		else
		{
			printf("Check Factory2's EEPROM successfully! (r = %d)\n", r);
			factory2_r = 1;
		}
	}

	if (factory_r == 1 && factory2_r == -3) {
		printf("Copy Factory's EEPROM to Factory2's one\n");
		r = ra_flash_erase_write(factory, NAND_RF_OFFSET_2, MAX_NAND_BLOCK_SIZE);
		if (r)
			printf("Write EEPROM set to 0x%x fail. (r = %d)\n", NAND_RF_OFFSET_2, r);
	}
	else if (factory_r == -3 && factory2_r == 1) {
		printf("Copy Factory2's EEPROM to Factory's one\n");
		r = ra_flash_erase_write(factory2, NAND_RF_OFFSET, MAX_NAND_BLOCK_SIZE);
		if (r)
			printf("Write EEPROM set to 0x%x fail. (r = %d)\n", NAND_RF_OFFSET, r);
	}

	return 0;
}

/* Update header checksum, data checksum, write times, etc.
 * @return:
 * 	0:	Success
 * 	-1:	Invalid parameter.
 */
int update_eeprom_checksum(unsigned char *buf)
{
	unsigned long checksum;
	eeprom_set_hdr_t *hdr;

	if (!buf)
		return -1;

	hdr = (eeprom_set_hdr_t *) (buf + EEPROM_SET_HEADER_OFFSET);
	checksum = crc32(0, (void *)buf, MAX_EEPROM_SET_LENGTH);

	/* reset write version to 0 if header magic number is incorrect or wrap */
	if (hdr->ih_magic != htonl(FACTORY_IMAGE_MAGIC) ||
	    ntohl(hdr->ih_write_ver) >= 0x7FFFFFFFU)
		hdr->ih_write_ver = htonl(0);

	/* Build new header */
	hdr->ih_magic = htonl(FACTORY_IMAGE_MAGIC);
	hdr->ih_hcrc = 0;
	hdr->ih_hdr_ver = htonl(1);
	hdr->ih_write_ver = htonl(ntohl(hdr->ih_write_ver) + 1);
	hdr->ih_dcrc = htonl(checksum);

	checksum = crc32(0, (void *)hdr, sizeof(eeprom_set_hdr_t));
	hdr->ih_hcrc = htonl(checksum);

	debug("header/data checksum: 0x%08x/0x%08x\n", ntohl(hdr->ih_hcrc), ntohl(hdr->ih_dcrc));
	return 0;
}

