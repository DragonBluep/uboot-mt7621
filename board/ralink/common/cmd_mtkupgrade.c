// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <command.h>
#include <console.h>
#include <cli.h>
#include <div64.h>
#include <environment.h>
#include <xyzModem.h>
#include <asm/reboot.h>
#include <linux/mtd/mtd.h>
#include <linux/sizes.h>
#include <jffs2/jffs2.h>

#include "spl_helper.h"
#include "flash_helper.h"
#include "dual_image.h"

#include <aes.h>

#define BUF_SIZE 1024

#define COLOR_PROMPT	"\x1b[0;33m"
#define COLOR_INPUT	"\x1b[4;36m"
#define COLOR_ERROR	"\x1b[93;41m"
#define COLOR_CAUTION	"\x1b[1;31m"
#define COLOR_NORMAL	"\x1b[0m"

enum file_type {
	TYPE_BL,
	TYPE_BL_ADV,
	TYPE_FW
};

#define ENCRPTED_IMG_TAG  "encrpted_img"
#define AES_KEY_256  "he9-4+M!)d6=m~we1,q2a3d1n&2*Z^%8$"
#define AES_NOR_IV  "J%1iQl8$=lm-;8AE@"

typedef  struct
{
    char model[32];        /*model name*/
    char region[32];       /*region*/
    char version[64];      /*version*/
    char dateTime[64];     /*date*/
    unsigned int productHwModel;  /*product hardware model*/
    char modelIndex;       /*model index - default 0:don't change model in nmrp upgrade - others: change model by index in nmrp upgrade*/
    char hwIdNum;          /*hw id list num*/
    char modelNum;         /*model list num*/
    char reserved0[13];    /*reserved*/
    char modelHwInfo[200]; /*save hw id list and model list*/
    char reserved[100];    /*reserved space, if add struct member, please adjust this reserved size to keep the head total size is 512 bytes*/
} __attribute__((__packed__)) image_head_t;

typedef  struct
{
    char checkSum[4];      /*checkSum*/
} __attribute__((__packed__)) image_tail_t;

static void cli_highlight_input(const char *prompt)
{
	printf(COLOR_INPUT "%s" COLOR_NORMAL " ", prompt);
}

static int env_read_cli_set(const char *varname, const char *defval,
			    const char *prompt, char *buffer, size_t bufsz)
{
	char input_buffer[CONFIG_SYS_CBSIZE + 1], *inbuf;
	char *argv[] = { "env", "set", NULL, NULL, NULL };
	const char *tmpstr;
	int repeatable;
	size_t inbufsz;

	if (buffer && bufsz) {
		inbuf = buffer;
		inbufsz = bufsz;
	} else {
		inbuf = input_buffer;
		inbufsz = sizeof(input_buffer);
	}

	tmpstr = env_get(varname);
	if (!tmpstr)
		tmpstr = defval;

	if (strlen(tmpstr) > inbufsz - 1) {
		strncpy(inbuf, tmpstr, inbufsz - 1);
		inbuf[inbufsz - 1] = 0;
	} else {
		strcpy(inbuf, tmpstr);
	}

	cli_highlight_input(prompt);
	if (cli_readline_into_buffer(NULL, inbuf, 0) == -1)
		return -1;

	if (!inbuf[0] || inbuf[0] == 10 || inbuf[0] == 13)
		return 1;

	argv[2] = (char *) varname;
	argv[3] = inbuf;

	return cmd_process(0, 4, argv, &repeatable, NULL);
}

static int env_update(const char *varname, const char *defval,
		      const char *prompt, char *buffer, size_t bufsz)
{
	while (1) {
		switch (env_read_cli_set(varname, defval, prompt,
			                 buffer, bufsz)) {
		case 0:
			return 0;
		case -1:
			printf("\n" COLOR_ERROR "*** Operation Aborted! ***"
			       COLOR_NORMAL "\n");
			return 1;
		}
	}
}

#ifdef CONFIG_CMD_TFTPBOOT
static int load_tftp(size_t addr, uint32_t *data_size, const char *env_name)
{
	char file_name[CONFIG_SYS_CBSIZE + 1];
	const char *save_tftp_info;
	uint32_t size;

	if (env_update("ipaddr", __stringify(CONFIG_IPADDR),
		       "Input U-Boot's IP address:", NULL, 0))
		return CMD_RET_FAILURE;

	if (env_update("serverip", __stringify(CONFIG_SERVERIP),
		       "Input TFTP server's IP address:", NULL, 0))
		return CMD_RET_FAILURE;

	if (env_update("netmask", __stringify(CONFIG_NETMASK),
		       "Input IP netmask:", NULL, 0))
		return CMD_RET_FAILURE;

	if (env_update(env_name, "", "Input file name:",
		       file_name, sizeof(file_name)))
		return CMD_RET_FAILURE;

	printf("\n");

	load_addr = addr;
	copy_filename(net_boot_file_name, file_name,
		      sizeof(net_boot_file_name));

	size = net_loop(TFTPGET);
	if ((int) size < 0) {
		printf("\n" COLOR_ERROR "*** TFTP client failure: %d ***"
		       COLOR_NORMAL "\n", size);
		printf("*** Operation Aborted! ***\n");
		return CMD_RET_FAILURE;
	}

	if (data_size)
		*data_size = size;

	save_tftp_info = env_get("mtkupgrade.save_tftp_info");
	if (save_tftp_info) {
		if (!strcmp(save_tftp_info, "yes"))
			env_save();
	}

	return CMD_RET_SUCCESS;
}
#endif

static int getcymodem(void)
{
	if (tstc())
		return (getc());
	return -1;
}

static int load_xymodem(int mode, size_t addr, uint32_t *data_size)
{
	connection_info_t info;
	char *buf = (char *) addr;
	size_t size = 0;
	int ret, err;
	char xyc;

	xyc = (mode == xyzModem_xmodem ? 'X' : 'Y');

	printf(COLOR_PROMPT "*** Starting %cmodem transmitting ***"
	       COLOR_NORMAL "\n\n", xyc);

	info.mode = mode;
	ret = xyzModem_stream_open(&info, &err);
	if (ret) {
		printf("\n" COLOR_ERROR "*** %cmodem error: %s ***" COLOR_NORMAL
		       "\n", xyc, xyzModem_error(err));
		printf("*** Operation Aborted! ***\n");
		return CMD_RET_FAILURE;
	}

	while ((ret = xyzModem_stream_read(buf + size, BUF_SIZE, &err)) > 0)
		size += ret;

	xyzModem_stream_close(&err);
	xyzModem_stream_terminate(false, &getcymodem);

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}

static int load_xmodem(size_t addr, uint32_t *data_size, const char *env_name)
{
	return load_xymodem(xyzModem_xmodem, addr, data_size);
}

static int load_ymodem(size_t addr, uint32_t *data_size, const char *env_name)
{
	return load_xymodem(xyzModem_ymodem, addr, data_size);
}

static int load_kermit(size_t addr, uint32_t *data_size, const char *env_name)
{
	char *argv[] = { "loadb", NULL, NULL };
	char saddr[16];
	int repeatable;
	size_t size = 0;
	int ret;

	printf(COLOR_PROMPT "*** Starting Kermit transmitting ***"
		COLOR_NORMAL "\n\n");

	sprintf(saddr, "0x%x", addr);
	argv[1] = saddr;

	ret = cmd_process(0, 2, argv, &repeatable, NULL);
	if (ret)
		return ret;

	size = env_get_hex("filesize", 0);
	if (!size)
		return CMD_RET_FAILURE;

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}

#ifdef CONFIG_CMD_LOADS
static int load_srecord(size_t addr, uint32_t *data_size, const char *env_name)
{
	char *argv[] = { "loads", NULL, NULL };
	char saddr[16];
	int repeatable;
	size_t size = 0;
	int ret;

	printf(COLOR_PROMPT "*** Starting S-Record transmitting ***"
		COLOR_NORMAL "\n\n");

	sprintf(saddr, "0x%x", addr);
	argv[1] = saddr;

	ret = cmd_process(0, 2, argv, &repeatable, NULL);
	if (ret)
		return ret;

	size = env_get_hex("filesize", 0);
	if (!size)
		return CMD_RET_FAILURE;

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}
#endif

struct load_method {
	const char *name;
	int(*load_func)(size_t addr, uint32_t *data_size, const char *env_name);
} load_methods[] = {
#ifdef CONFIG_CMD_TFTPBOOT
	{
		.name = "TFTP client",
		.load_func = load_tftp
	},
#endif
	{
		.name = "Xmodem",
		.load_func = load_xmodem
	},
	{
		.name = "Ymodem",
		.load_func = load_ymodem
	},
	{
		.name = "Kermit",
		.load_func = load_kermit
	},
#ifdef CONFIG_CMD_LOADS
	{
		.name = "S-Record",
		.load_func = load_srecord
	},
#endif
};

static int load_data(size_t addr, uint32_t *data_size, const char *env_name)
{
	int i;
	char c;

	printf(COLOR_PROMPT "Available load methods:" COLOR_NORMAL "\n");

	for (i = 0; i < ARRAY_SIZE(load_methods); i++) {
		printf("    %d - %s", i, load_methods[i].name);
		if (i == 0)
			printf(" (Default)");
		printf("\n");
	}

	printf("\n" COLOR_PROMPT "Select (enter for default):" COLOR_NORMAL " ");

	c = getc();
	printf("%c\n\n", c);

	if (c == '\r' || c == '\n')
		c = '0';

	i = c - '0';
	if (i < 0 || i >= ARRAY_SIZE(load_methods)) {
		printf(COLOR_ERROR "*** Invalid selection! ***"
			COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	if (load_methods[i].load_func(addr, data_size, env_name))
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

int get_mtd_part_info(const char *partname, uint64_t *off, uint64_t *size)
{
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;

	ret = mtdparts_init();
	if (ret)
		return ret;

	ret = find_dev_and_part(partname, &dev, &pnum, &part);
	if (ret)
		return ret;

	*off = part->offset;
	*size = part->size;
	return 0;
}

static int check_mtd_bootloader_size(size_t data_size, uint64_t *addr_limit)
{
	uint64_t part_off, part_size;
	const char *part_name = "u-boot";

	if (get_mtd_part_info(part_name, &part_off, &part_size)) {
		part_name = "Bootloader";
		if (get_mtd_part_info(part_name, &part_off, &part_size)) {
			if (addr_limit)
				*addr_limit = 0;

			return CMD_RET_SUCCESS;
		}
	}

	if (part_off == 0 && part_size < data_size) {
		printf("\n" COLOR_PROMPT "*** Warning: new bootloader will "
		       "overwrite mtd partition '%s' ***" COLOR_NORMAL "\n",
		       part_name);
		cli_highlight_input("Continue anyway? (N/y):");
		if (!confirm_yesno()) {
			printf(COLOR_ERROR "*** Operation Aborted! ***"
			       COLOR_NORMAL "\n");
			return CMD_RET_FAILURE;
		}
	}

	if (addr_limit)
		*addr_limit = part_off + part_size;

	return CMD_RET_SUCCESS;
}

static int prompt_countdown(const char *prompt, int delay)
{
	int i;
	int hit = 0;

	if (delay <= 0)
		return 0;

	printf("\n%s: %2d ", prompt, delay);

	while (delay > 0) {
		for (i = 0; i < 100; i++) {
			if (!tstc()) {
				mdelay(10);
				continue;
			}

			getc();
			hit = 1;
			delay = -1;

			break;
		}

		if (delay < 0)
			break;

		delay--;
		printf("\b\b\b%2d ", delay);
	}

	puts("\n");

	return hit;
}


static int do_write_bootloader(void *flash, size_t stock_stage2_off,
			       size_t data_addr, uint32_t data_size,
			       uint32_t stage1_size, int adv)
{
	uint64_t addr_limit;
	uint32_t erase_size;
	int ret;

	if (stock_stage2_off && stage1_size) {
		if (adv) {
			printf(COLOR_PROMPT "*** Caution: Bootblock upgrading "
			       "***" COLOR_NORMAL "\n\n");
			printf("This bootloader contains Bootblock.\n");
			printf(COLOR_CAUTION "Upgrading Bootblock is very "
			       "dangerous. Upgrade it only if you know what "
			       "you are doing!" COLOR_NORMAL "\n");
			cli_highlight_input("Upgrade Bootblock? (N/y):");

			if (!confirm_yesno()) {
				printf("Only second stage block will be "
				       "upgraded\n");
				data_addr += stage1_size;
				data_size -= stage1_size;
			} else {
				printf("Whole bootloader will be upgraded\n");
				stock_stage2_off = 0;
			}
		} else {
			data_addr += stage1_size;
			data_size -= stage1_size;
		}
	}

	ret = check_mtd_bootloader_size(stock_stage2_off + data_size,
					&addr_limit);
	if (ret != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	printf("\n");

	erase_size = ALIGN(data_size, mtk_board_get_flash_erase_size(flash));

	printf("Erasing from 0x%x to 0x%x, size 0x%x ... ", stock_stage2_off,
	       stock_stage2_off + erase_size - 1, erase_size);

	ret = mtk_board_flash_erase(flash, stock_stage2_off, erase_size,
				    addr_limit);

	if (ret) {
		printf("Fail\n");
		printf(COLOR_ERROR "*** Flash erasure [%x-%x] failed! ***"
		       COLOR_NORMAL "\n", stock_stage2_off,
		       stock_stage2_off + erase_size - 1);
		return CMD_RET_FAILURE;
	}

	printf("OK\n");

	printf("Writting from 0x%x to 0x%x, size 0x%x ... ", data_addr,
	       stock_stage2_off, data_size);

	ret = mtk_board_flash_write(flash, stock_stage2_off, data_size,
				    addr_limit, (void *)data_addr);

	if (ret) {
		printf("Fail\n");
		printf(COLOR_ERROR "*** Flash program [%x-%x] failed! ***"
		       COLOR_NORMAL "\n", stock_stage2_off,
		       stock_stage2_off + data_size - 1);
		return CMD_RET_FAILURE;
	}

	printf("OK\n");

	printf("Verifying from 0x%x to 0x%x, size 0x%x ... ", stock_stage2_off,
		stock_stage2_off + data_size - 1, data_size);

	ret = mtk_board_flash_verify(flash, stock_stage2_off, data_size,
				     addr_limit, (void *)data_addr);

	if (ret) {
		printf("Fail\n");
		printf(COLOR_ERROR "*** Verification [%x-%x] failed! ***"
		       COLOR_NORMAL "\n", stock_stage2_off,
		       stock_stage2_off + data_size - 1);
		printf(COLOR_ERROR "*** Bootloader is damaged, please retry! ***"
		       COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	printf("OK\n");

	printf("\n" COLOR_PROMPT "*** Bootloader upgrade completed! ***"
	       COLOR_NORMAL "\n");

	if (!prompt_countdown("Hit any key to stop reboot", 3)) {
		printf("\nRebooting ...\n\n");
		_machine_restart();
	}

	return CMD_RET_SUCCESS;
}

static int verify_stage2_integrity(const void *data, uint32_t size)
{
	struct image_header hdr;
	u32 data_chksum, data_size;

	/* Header checksum has already been validated */

	memcpy(&hdr, data, sizeof(hdr));

	if (image_get_magic(&hdr) != IH_MAGIC)
		return 1;

	data_chksum = image_get_dcrc(&hdr);
	data_size = image_get_size(&hdr);

	if (data_size + sizeof(struct image_header) > size)
		return 1;

	if (crc32(0, (const u8 *) data + sizeof(hdr), data_size) !=
	    data_chksum)
		return 1;

	return 0;
}

static int write_bootloader(void *flash, size_t data_addr, uint32_t data_size,
			    int adv)
{
	size_t stock_stage2_off, stock_stage2_off_min;
	size_t tmp, stock_block_size, flash_block_size;
	size_t data_stage1_size, data_stage2_size;
	char *data_stage2_ptr;

	data_stage2_ptr = get_mtk_stage2_image_ptr((char *) data_addr,
						   data_size);
	if (!data_stage2_ptr)
		/* New bootloader is not a two-stage bootloader */
		goto do_write_bl_single;

	/* Verify stage2 block */
	data_stage1_size = (size_t) data_stage2_ptr - data_addr;
	data_stage2_size = data_size - data_stage1_size;
	if (verify_stage2_integrity(data_stage2_ptr, data_stage2_size)) {
		printf(COLOR_ERROR "*** Bootloader stage2 block integrity "
		       "check failed! ***" COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	if (!check_mtk_stock_stage2_info(&stock_stage2_off_min,
					 &stock_stage2_off,
					 &stock_block_size))
		/* Current bootloader is not a two-stage bootloader */
		goto do_write_bl_single;

	flash_block_size = mtk_board_get_flash_erase_size(flash);
	if (!flash_block_size)
		/* Unable to get erase block size */
		goto do_write_bl_single;

	/* Same alignment */
	if (stock_block_size == flash_block_size)
		goto do_write_bl;

	/*
	 * Current bootloader's alignment is not equal to
	 * the real erase block size
	 */
	if (stock_block_size < flash_block_size) {
		/*
		 * Current bootloader's alignment is smaller than
		 * real erase block size
		 */
		if (!stock_block_size)
			/* Current bootloader has no padding */
			goto do_write_bl_single;

		if (flash_block_size % stock_block_size)
			/* And can not divide the real erase block size */
			goto do_write_bl_single;

		if (stock_stage2_off % flash_block_size) {
			/*
			 * Current bootloader's stage2 offset is not on
			 * the erase block boundary
			 */
			if (stock_stage2_off_min % flash_block_size) {
				/*
				 * Start from the minimum offset, find an
				 * offset aligned with the real erase
				 * block size
				 */
				tmp = ALIGN(stock_stage2_off_min,
					    flash_block_size);

				if (tmp > stock_stage2_off)
					/*
					 * The new offset exceeds the
					 * original stage2 offset
					 */
					goto do_write_bl_single;
				else
					/* Use the new offset */
					stock_stage2_off = tmp;
			} else {
				/*
				 * The minimum offset is aligned with the
				 * real erase block size
				 */
				stock_stage2_off = stock_stage2_off_min;
			}
		}
	} else {
		/*
		 * Current bootloader's alignment is larger than real
		 * erase block size
		 */
		if (stock_block_size % flash_block_size)
			/* And is not a multiple of the real block size */
			goto do_write_bl_single;
	}

do_write_bl:
	/* Both stock bootloader and new bootloader are two-stage bootloaders */
	return do_write_bootloader(flash, stock_stage2_off, data_addr,
				   data_size, data_stage1_size, adv);

do_write_bl_single:
	/* Treat the new bootloader as a single-stage bootloader */
	return do_write_bootloader(flash, 0, data_addr, data_size, 0, adv);
}

static int _write_firmware(void *flash, size_t data_addr, uint32_t data_size,
			   int no_prompt)
{
	uint32_t erase_size;
	uint64_t part_off, part_size, tmp;
	int ret;
#ifdef CONFIG_MTK_DUAL_IMAGE_SUPPORT
	struct di_image_info iif;
#endif

	if (get_mtd_part_info("firmware", &part_off, &part_size)) {
		printf(COLOR_ERROR "*** MTD partition 'firmware' does not "
		       "exist! ***" COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	if (!part_off) {
		printf(COLOR_ERROR "*** MTD partition 'firmware' is not "
		       "valid! ***" COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	tmp = part_off;

	if (do_div(tmp, mtk_board_get_flash_erase_size(flash))) {
		printf(COLOR_ERROR "*** MTD partition 'firmware' does not "
		       "start on erase boundary! ***" COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	if (part_size < data_size) {
		printf("\n" COLOR_ERROR "*** Error: new firmware is larger "
		       "than mtd partition 'firmware' ***" COLOR_NORMAL "\n");
		printf(COLOR_ERROR "*** Operation Aborted! ***"
		       COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	printf("\n");

#ifdef CONFIG_MTK_DUAL_IMAGE_SUPPORT
	printf("Verifying image from 0x%zx to 0x%zx, size 0x%x ... ",
	       data_addr, data_addr + data_size - 1, data_size);

	ret = dual_image_check_single_ram((void *)data_addr, data_size,
					  "firmware", &iif);
	if (ret) {
		printf("Fail\n");
		return CMD_RET_FAILURE;
	}

	printf("OK\n");
#endif

	erase_size = ALIGN(data_size, mtk_board_get_flash_erase_size(flash));

	printf("Erasing from 0x%llx to 0x%llx, size 0x%x ... ", part_off,
	       part_off + erase_size - 1, erase_size);

	ret = mtk_board_flash_erase(flash, part_off, erase_size,
				    part_off + part_size);

	if (ret) {
		printf("Fail\n");
		printf(COLOR_ERROR "*** Flash erasure [%llx-%llx] failed! ***"
		       COLOR_NORMAL "\n", part_off, part_off + erase_size - 1);
		return CMD_RET_FAILURE;
	}

	printf("OK\n");

	printf("Writting from 0x%x to 0x%llx, size 0x%x ... ", data_addr,
	       part_off, data_size);

	ret = mtk_board_flash_write(flash, part_off, data_size,
				    part_off + part_size, (void *)data_addr);

	if (ret) {
		printf("Fail\n");
		printf(COLOR_ERROR "*** Flash program [%llx-%llx] failed! ***"
		       COLOR_NORMAL "\n", part_off, part_off + data_size - 1);
		return CMD_RET_FAILURE;
	}

	printf("OK\n");

#ifdef CONFIG_MTK_DUAL_IMAGE_SUPPORT
	printf("Verifying flash from 0x%llx to 0x%llx, size 0x%x ... ",
	       part_off, part_off + data_size - 1, data_size);

	ret = mtk_board_flash_verify(flash, part_off, data_size,
				     part_off + part_size, (void *)data_addr);

	if (ret) {
		printf("Fail\n");
		printf(COLOR_ERROR "*** Verification [%llx-%llx] failed! ***"
		       COLOR_NORMAL "\n", part_off,
		       part_off + data_size - 1);
		printf(COLOR_ERROR "*** Firmware is damaged, please retry! ***"
		       COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	printf("OK\n");

#ifdef CONFIG_MTK_DUAL_IMAGE_UPGRADE_BACKUP
	printf("Updating backup image ... ");

	ret = dual_image_update_backup(&iif);

	if (ret) {
		printf("Fail\n");
		return CMD_RET_FAILURE;
	}

	printf("OK\n");
#endif
#endif

	printf("\n" COLOR_PROMPT "*** Firmware upgrade completed! ***"
	       COLOR_NORMAL "\n");

	if (no_prompt)
		return CMD_RET_SUCCESS;

	if (!prompt_countdown("Hit any key to stop firmware bootup", 3))
		run_command("mtkboardboot", 0);

	return CMD_RET_SUCCESS;
}

int write_firmware_failsafe(size_t data_addr, uint32_t data_size)
{
	void *flash;

	flash = mtk_board_get_flash_dev();

	if (!flash)
		return CMD_RET_FAILURE;

	return _write_firmware(flash, data_addr, data_size, 1);
}

static int write_firmware(void *flash, size_t data_addr, uint32_t data_size)
{
	return _write_firmware(flash, data_addr, data_size, 0);
}

static int write_data(enum file_type ft, size_t addr, uint32_t data_size)
{
	void *flash;

	flash = mtk_board_get_flash_dev();

	if (!flash)
		return CMD_RET_FAILURE;

	switch (ft) {
	case TYPE_BL:
	case TYPE_BL_ADV:
		if (write_bootloader(flash, addr, data_size,
				     (ft == TYPE_BL_ADV)))
			return CMD_RET_FAILURE;

		break;

	case TYPE_FW:
		if (write_firmware(flash, addr, data_size))
			return CMD_RET_FAILURE;

		break;

	default:
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

struct upgrade_part {
	const char *id;
	const char *name;
} upgrade_parts[] = {
	{
		.id = "bl",
		.name = "Bootloader"
	},
	{
		.id = "bladv",
		.name = "Bootloader (Advanced)"
	},
	{
		.id = "fw",
		.name = "Firmware"
	},
};

static const char *select_part(void)
{
	int i;
	char c;

	printf("\n");
	printf(COLOR_PROMPT "Available parts to be upgraded:" COLOR_NORMAL "\n");

	for (i = 0; i < ARRAY_SIZE(upgrade_parts); i++)
		printf("    %d - %s\n", i, upgrade_parts[i].name);

	while (1) {
		printf("\n" COLOR_PROMPT "Select a part:" COLOR_NORMAL " ");

		c = getc();
		if (c == '\r' || c == '\n')
			continue;

		printf("%c\n", c);
		break;
	}

	i = c - '0';
	if (i < 0 || i >= ARRAY_SIZE(upgrade_parts)) {
		printf(COLOR_ERROR "*** Invalid selection! ***"
			COLOR_NORMAL "\n");
		return NULL;
	}

	return upgrade_parts[i].id;
}

static int do_mtkupgrade(cmd_tbl_t *cmdtp, int flag, int argc,
	char *const argv[])
{
	enum file_type ft;
	const char *part, *ft_name, *env_name;

	size_t data_load_addr;
	uint32_t data_size = 0;

	if (argc < 2) {
		part = select_part();
		if (!part)
			return CMD_RET_FAILURE;
	} else {
		part = argv[1];
	}

	if (!strcasecmp(part, "bl")) {
		ft = TYPE_BL;
		ft_name = "Bootloader";
		env_name = "bootfile.bootloader";
	} else if (!strcasecmp(part, "bladv")) {
		ft = TYPE_BL_ADV;
		ft_name = "Bootloader";
		env_name = "bootfile.bootloader";
	} else if (!strcasecmp(part, "fw")) {
		ft = TYPE_FW;
		ft_name = "Firmware";
		env_name = "bootfile.firmware";
	} else {
		printf("Error: invalid type '%s'\n", part);
		return EINVAL;
	}

	printf("\n" COLOR_PROMPT "*** Upgrading %s ***" COLOR_NORMAL
	       "\n\n", ft_name);

	data_load_addr = CONFIG_SYS_LOAD_ADDR;

	/* Load data */
	if (load_data(data_load_addr, &data_size, env_name) != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	printf("\n" COLOR_PROMPT "*** Loaded %d (0x%x) bytes at 0x%08x ***"
	       COLOR_NORMAL "\n\n", data_size, data_size, data_load_addr);

	/* Write data */
	if (write_data(ft, data_load_addr, data_size) != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtkupgrade, 2, 0, do_mtkupgrade,
	"MTK firmware/bootloader upgrading utility",
	"mtkupgrade [<type>]\n"
	"type    - upgrade file type\n"
	"          bl      - Bootloader\n"
	"          bladv   - Bootloader (Advanced)\n"
	"          fw      - Firmware\n"
);

extern void led_time_tick(ulong times);

int decrypt_image(void)
{
    ulong size = 0;
    ulong file_size = 0;
    unsigned char *src_addr = NULL;
    unsigned char *dst_addr = NULL;
    image_head_t *image_head = NULL;
    char *image_tag = NULL;
    char *fenv_hwid = NULL;
    char *fenv_model = NULL;
    //char *fenv_region = NULL;
    ulong image_size = 0;
    ulong encrypted_size = 0;
    ulong block_size = 0;
    ulong checksum = 0;
    ulong curr_checksum = 0;
    unsigned char key_exp[AES256_EXPAND_KEY_LENGTH] = {0};
    unsigned int aes_blocks = 0;

    env_set("decrypt_result", "bad");
    env_set("filesize_result", "bad");

    size = sizeof(image_head_t) + strlen(ENCRPTED_IMG_TAG) + 4 * 2;

    file_size = env_get_hex("filesize", 0);
    if (file_size < size)
    {
        printf("Image head not found!\n");
        return 1;
    }

    src_addr = (unsigned char *)load_addr;
    dst_addr = (unsigned char *)load_addr;

    image_head = (image_head_t *)src_addr;
    src_addr += sizeof(image_head_t);

    image_tag = (char *)src_addr;
    if (strncmp(image_tag, ENCRPTED_IMG_TAG, strlen(ENCRPTED_IMG_TAG)))
    {
        printf("Encrpted tag not found!\n");
        return 1;
    }
    src_addr += strlen(ENCRPTED_IMG_TAG);

    printf("Image is encrypted\n");
    printf("model: %s\n", image_head->model);
    printf("region: %s\n", image_head->region);
    printf("version: %s\n", image_head->version);
    printf("dateTime: %s\n", image_head->dateTime);
    printf("CONFIG_NETGEAR_MODLE_NAME: %s\n", CONFIG_NETGEAR_MODLE_NAME);
    printf("productHwModel: %d\n", image_head->productHwModel);
    printf("modelIndex: %d\n", image_head->modelIndex);
    printf("hwIdNum: %d\n", image_head->hwIdNum);
    printf("modelNum: %d\n", image_head->modelNum);
    printf("modelHwInfo: %s\n", image_head->modelHwInfo);

    //
    if (image_head->modelIndex != 0 && image_head->modelIndex <= image_head->modelNum)
    {
        char loop = 0;
        char delims[] = ";";
        char *result = NULL;
        char achModelHwInfo[512] = { 0 };

        strcpy(achModelHwInfo, image_head->modelHwInfo);
        result = strtok(achModelHwInfo, delims);
        //skip hw id
        while(result != NULL && loop < image_head->hwIdNum)
        {
            printf("hwid is \"%s\"\n", result);
            loop++;
            result = strtok(NULL, delims);
        }
        //model list
        loop = 0;
        while(result != NULL && loop < image_head->modelNum)
        {
            loop++;
            printf("model[%d] is \"%s\"\n", loop, result);

            if (loop == image_head->modelIndex)
            {
                env_set("fenv_model", result);
                break;
            }
            result = strtok(NULL, delims);  
        }
    }

    fenv_hwid = env_get("fenv_hw_id");
    printf("fenv_hwid: %s\n", fenv_hwid);
    if (!fenv_hwid || !strstr(image_head->modelHwInfo, fenv_hwid))
    {
        printf("Image hw id not match!\n");
        return 1;
    }

    fenv_model = env_get("fenv_model");
    printf("fenv_model: %s\n", fenv_model);
    if (!fenv_model || !strstr(image_head->modelHwInfo, fenv_model))
    {
        printf("Image model not match!\n");
        return 1;
    }
#if 0
    fenv_region = env_get("fenv_region");
    if (!fenv_region || strcmp(fenv_region, image_head->region))
    {
        printf("Image region not match!\n");
        return 1;
    }
#endif

    image_size = ntohl(*(ulong *)src_addr);
    src_addr += sizeof(ulong);
    printf("size: 0x%lx\n", image_size);

    encrypted_size = DIV_ROUND_UP(image_size, AES_BLOCK_LENGTH) * AES_BLOCK_LENGTH;

    block_size = ntohl(*(ulong *)src_addr);
    src_addr += sizeof(ulong);
    printf("block size: 0x%lx\n", block_size);
    if (block_size % AES_BLOCK_LENGTH)
    {
        printf("Image block size not times of AES_BLOCK_LENGTH!\n");
        return 1;
    }

    if (file_size < (size + encrypted_size + sizeof(image_tail_t)))
    {
        printf("Image incomplete!\n");
        return 1;
    }

    checksum = ntohl(*(ulong *)(src_addr + encrypted_size));
    printf("checksum: 0x%lx\n", checksum);

    curr_checksum = crc32_no_comp(0, (uchar *)load_addr, size + encrypted_size);
    printf("curr_checksum: 0x%lx\n", curr_checksum);
    if (curr_checksum != checksum)
    {
        printf("Image checksum error!\n");
        return 1;
    }

    printf("Decrypt image...\n");

    aes_expand_key((u8 *)AES_KEY_256, AES256_KEY_LENGTH, key_exp);
    for (size = 0; size < encrypted_size; size += block_size)
    {
        if (size + block_size > encrypted_size)
        {
            block_size = encrypted_size - size;
        }

        aes_blocks = DIV_ROUND_UP(block_size, AES_BLOCK_LENGTH);
        aes_cbc_decrypt_blocks(AES256_KEY_LENGTH, key_exp, (u8 *)AES_NOR_IV, (u8 *)src_addr, (u8 *)dst_addr, aes_blocks);

        src_addr += block_size;
        dst_addr += block_size;
        led_time_tick(get_timer(0));
    }
    printf("Decrypt finish\n");

    env_set_hex("filesize", image_size);

    env_set("filesize_result", "good");

    env_set("decrypt_result", "good");

    return 0;
}

static int do_write_img(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    size_t data_load_addr;
    uint32_t data_size = 0;

    //
    if (0 != decrypt_image())
        return CMD_RET_FAILURE;

    data_load_addr = load_addr;

    data_size = env_get_hex("filesize", 0);

    /* Write data */
    if (write_firmware_failsafe(data_load_addr, data_size) != CMD_RET_SUCCESS)
        return CMD_RET_FAILURE;

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(writeimg, 2, 0, do_write_img,
    "write firmware",
    "write firmware\n"
);

static int run_image(size_t data_addr, uint32_t data_size)
{
	char *uimage_ptr;
	char *argv[2], str[64];
	struct image_header hdr;

	/* Check for SPL bootloader first */
	uimage_ptr = get_mtk_stage2_image_ptr((char *) data_addr,
					      data_size);
	if (uimage_ptr) {
		/* uImage type hack */
		memcpy(&hdr, uimage_ptr, sizeof(hdr));
		hdr.ih_type = IH_TYPE_STANDALONE;
		hdr.ih_hcrc = 0;
		hdr.ih_hcrc = htonl(crc32(0, (u8 *) &hdr, sizeof(hdr)));
		memcpy(uimage_ptr, &hdr, sizeof(hdr));
	} else {
		uimage_ptr = (void *) data_addr;
	}

	sprintf(str, "0x%p", uimage_ptr);
	argv[0] = "bootm";
	argv[1] = str;

	return do_bootm(find_cmd("do_bootm"), 0, 2, argv);
}

static int confirm_run_image(void)
{
	char yn[CONFIG_SYS_CBSIZE + 1];

	yn[0] = 0;

	cli_highlight_input("Run loaded data now? (Y/n):");
	if (cli_readline_into_buffer(NULL, yn, 0) == -1)
		return 0;

	if (!yn[0] || yn[0] == '\r' || yn[0] == '\n')
		return 1;

	return !strcmp(yn, "y") || !strcmp(yn, "Y") ||
		!strcmp(yn, "yes") || !strcmp(yn, "YES");
}

static int do_mtkload(cmd_tbl_t *cmdtp, int flag, int argc,
	char *const argv[])
{
	char s_load_addr[CONFIG_SYS_CBSIZE + 1];
	char *addr_end, *def_load_addr = NULL;
	size_t data_load_addr;
	uint32_t data_size = 0;

	printf("\n" COLOR_PROMPT "*** Loading image ***" COLOR_NORMAL "\n\n");

	/* Set load address */
#if defined(CONFIG_LOADADDR)
	def_load_addr = __stringify(CONFIG_LOADADDR);
#elif defined(CONFIG_SYS_LOAD_ADDR)
	def_load_addr = __stringify(CONFIG_SYS_LOAD_ADDR);
#endif

	if (env_update("loadaddr", def_load_addr, "Input load address:",
		       s_load_addr, sizeof(s_load_addr)))
		return CMD_RET_FAILURE;

	data_load_addr = simple_strtoul(s_load_addr, &addr_end, 0);
	if (*addr_end) {
		printf("\n" COLOR_ERROR "*** Invalid load address! ***"
		       COLOR_NORMAL "\n");
		return CMD_RET_FAILURE;
	}

	printf("\n");

	/* Load data */
	if (load_data(data_load_addr, &data_size, "bootfile") != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	printf("\n" COLOR_PROMPT "*** Loaded %d (0x%x) bytes at 0x%08x ***"
	       COLOR_NORMAL "\n\n", data_size, data_size, data_load_addr);

	/* Whether to run */
	if (confirm_run_image()) {
		/* Run image */
		if (run_image(data_load_addr, data_size) != CMD_RET_SUCCESS)
			return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtkload, 1, 0, do_mtkload,
	"MTK image loading utility",
	NULL
);
