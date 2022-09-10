/*
 * Control BLE Advertising
 *
 */

#include <dm.h>
#include <serial.h>
#include <asm/addrspace.h>
#include <stdio.h>
#include <errno.h>
#include <asm/gpio.h>

#define MT7621_UART3_BASE	0x1e000e00
#define CMD_RET_SUCCESS 1
#define CMD_RET_FAIL 0
#define TRUE 1
#define FALSE 0
#define BLE_FORMAT_HEADER_LEN 4
#define BLE_PW_ON_IDX 0
#define BLE_HCI_RESET_IDX 1
#define BLE_HCI_SET_ADV_PARAM_IDX 2
#define BLE_HCI_SET_ADV_DATA_IDX 3
#define BLE_HCI_SET_SCAN_RESP_DATA_IDX 4
#define BLE_HCI_SET_ADV_ENABLE_IDX 5
#define GPIO_CLEAR 0
#define GPIO_SET 1
// BLE EXPECTED RX Messgae
const u8  pw_on_correct_code[] = {0x04, 0xe4, 0x05, 0x02, 0x06, 0x01, 0x00, 0x00};
const u8  hic_reset_correct_code[] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00};
const u8  hic_le_set_adv_param_correct_code[] = {0x04, 0x0E, 0x04, 0x01, 0x06, 0x20, 0x00};
const u8  hic_le_set_adv_data_correct_code[] = {0x04, 0x0E, 0x04, 0x01, 0x08, 0x20, 0x00};
const u8  hic_le_set_scan_resp_data_correct_code[] = {0x04, 0x0E, 0x04, 0x01, 0x09, 0x20, 0x00};
const u8  hic_le_set_adv_en_correct_code[] = {0x04, 0x0E, 0x04, 0x01, 0x0A, 0x20, 0x00};

//BLE TX Command
const char bt_power_on_tx_cmd[] = {0x01, 0x6f, 0xfc, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01};

const char hci_reset_tx_cmd[] = {0x01, 0x03, 0x0C, 0x00};

const char hci_le_set_adv_param_tx_cmd[] =
{
        0x01, 0x06, 0x20, 0x0f,
        0xa0, 0x00, //Advertising Interval Min (x0.625ms)
        0xa0, 0x00, //Advertising Interval Max (x0.625ms)
        0x02, //Advertising Type
        0x00, //Own Address Type
        0x00, //Peer Address Type
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //Peer Address
        0x07, //Advertising Channel Map
        0x00 //Advertising Filter Policy
};
//Sample1: MT7915_BLE_ADV_TEST
const char hci_le_set_adv_data_tx_cmd[] =
{
        0x01, 0x08, 0x20, 0x20,
        0x18, //Advertising Data Length, Total 31 bytes
        0x02, 0x01, 0x1A, 0x14, 0x09, 0x4d, // Advertising Data Byte0~Byte6
        0x54, 0x37, 0x39, 0x31, 0x35, 0x5f, // Advertising Data Byte7~Byte12
        0x42, 0x4c, 0x45, 0x5f, 0x41, 0x44, // Advertising Data Byte13~Byte18
        0x56, 0x5f, 0x54, 0x45, 0x53, 0x54, // Advertising Data Byte19~Byte24
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Advertising Data Byte25~Byte30
        0x00 // Advertising Data Byte31
};

const char hci_le_set_scan_resp_data_tx_cmd[] =
{
        0x01, 0x09, 0x20, 0x20,
        0x18, //Scan Response Data Length, Total 31 bytes
        0x02, 0x01, 0x1A, 0x14, 0x09, 0x4d, // Scan Response Data Byte0~Byte6
        0x54, 0x37, 0x39, 0x31, 0x35, 0x5f, // Scan Response Data Byte7~Byte12
        0x42, 0x4c, 0x45, 0x5f, 0x41, 0x44, // Scan Response Data Byte13~Byte18
        0x56, 0x5f, 0x54, 0x45, 0x53, 0x54, // Scan Response Data Byte19~Byte24
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Scan Response Data Byte25~Byte30
        0x00 // Scan Response Data Byte31
};

const char hci_le_set_adv_enable_tx_cmd[] = {	0x01, 0x0A, 0x20, 0x01, 0x01}; //Advertising Enable/Disable


static void _serial_putc(struct udevice *dev, char ch)
{
	struct dm_serial_ops *ops = serial_get_ops(dev);
	int err;

	do {
		err = ops->putc(dev, ch);
	} while (err == -EAGAIN);
}

static int _serial_getc(struct udevice *dev)
{
	struct dm_serial_ops *ops = serial_get_ops(dev);
	int err;

	do {
		err = ops->getc(dev);
	} while (err == -EAGAIN);

	return err >= 0 ? err : 0;
}

static int _serial_tstc(struct udevice *dev)
{
	struct dm_serial_ops *ops = serial_get_ops(dev);

	if (ops->pending)
		return ops->pending(dev, true);

	return 1;
}

static void _serial_setbrg(struct udevice *dev, int baudrate)
{
	struct dm_serial_ops *ops = serial_get_ops(dev);

	if (ops->setbrg)
		ops->setbrg(dev, baudrate);
}

static int get_ble_tx_cmd(u8 idx, u8 tx_cmd_buf[])
{
	u8 len = 0;

	memset(tx_cmd_buf, 0x0, sizeof(tx_cmd_buf));
	switch(idx)
	{
		case BLE_PW_ON_IDX:
			len = sizeof(bt_power_on_tx_cmd) / sizeof(u8);
			memcpy(tx_cmd_buf, bt_power_on_tx_cmd, sizeof(bt_power_on_tx_cmd));
			break;
		case BLE_HCI_RESET_IDX:
			len = sizeof(hci_reset_tx_cmd) / sizeof(u8);
			memcpy(tx_cmd_buf, hci_reset_tx_cmd, sizeof(hci_reset_tx_cmd));
			break;
		case BLE_HCI_SET_ADV_PARAM_IDX:
			len = sizeof(hci_le_set_adv_param_tx_cmd) / sizeof(u8);
			memcpy(tx_cmd_buf, hci_le_set_adv_param_tx_cmd, sizeof(hci_le_set_adv_param_tx_cmd));
			break;
		case BLE_HCI_SET_ADV_DATA_IDX:
			len = sizeof(hci_le_set_adv_data_tx_cmd) / sizeof(u8);
			memcpy(tx_cmd_buf, hci_le_set_adv_data_tx_cmd, sizeof(hci_le_set_adv_data_tx_cmd));
			break;
		case BLE_HCI_SET_SCAN_RESP_DATA_IDX:
			len = sizeof(hci_le_set_scan_resp_data_tx_cmd) / sizeof(u8);
			memcpy(tx_cmd_buf, hci_le_set_scan_resp_data_tx_cmd, sizeof(hci_le_set_scan_resp_data_tx_cmd));
			break;
		case BLE_HCI_SET_ADV_ENABLE_IDX:
			len = sizeof(hci_le_set_adv_enable_tx_cmd) / sizeof(u8);
			memcpy(tx_cmd_buf, hci_le_set_adv_enable_tx_cmd, sizeof(hci_le_set_adv_enable_tx_cmd));
			break;
		default:
			break;
	}
	return len;
}
static int judge_rx_msg(u8 msg_idx, u8 rx_msg_buf[])
{
	u8 ret = 0;
	u8 num = 0;

	switch(msg_idx)
	{
		case BLE_PW_ON_IDX:
			num = sizeof(pw_on_correct_code);
			ret = memcmp( rx_msg_buf, pw_on_correct_code, num);
			//printf("[judge_rx_msg]: num=%d, ret=%d\n", num, ret);
			break;
		case BLE_HCI_RESET_IDX:
			num = sizeof(hic_reset_correct_code);
			ret = memcmp( rx_msg_buf, hic_reset_correct_code, num);
			//printf("[judge_rx_msg]: num=%d, ret=%d\n", num, ret);
			break;
		case BLE_HCI_SET_ADV_PARAM_IDX:
			num = sizeof(hic_le_set_adv_param_correct_code);
			ret = memcmp( rx_msg_buf, hic_le_set_adv_param_correct_code, num);
			//printf("[judge_rx_msg]: num=%d, ret=%d\n", num, ret);
			break;
		case BLE_HCI_SET_ADV_DATA_IDX:
			num = sizeof(hic_le_set_adv_data_correct_code);
			ret = memcmp( rx_msg_buf, hic_le_set_adv_data_correct_code, num);
			//printf("[judge_rx_msg]: num=%d, ret=%d\n", num, ret);
			break;
		case BLE_HCI_SET_SCAN_RESP_DATA_IDX:
			num = sizeof(hic_le_set_scan_resp_data_correct_code);
			ret = memcmp( rx_msg_buf, hic_le_set_scan_resp_data_correct_code, num);
			//printf("[judge_rx_msg]: num=%d, ret=%d\n", num, ret);
			break;
		case BLE_HCI_SET_ADV_ENABLE_IDX:
			num = sizeof(hic_le_set_adv_en_correct_code);
			ret = memcmp( rx_msg_buf, hic_le_set_adv_en_correct_code, num);
			//printf("[judge_rx_msg]: num=%d, ret=%d\n", num, ret);
			break;
		default:
			break;

	}
	if(ret == 0 )
		return CMD_RET_SUCCESS;
	else
		return CMD_RET_FAIL;
}
static int do_mtk_bleadv(cmd_tbl_t *cmdtp, int flag, int argc,	char *const argv[])
{
	// START BT PROCESS
	struct udevice *uart;
    	u8 bt_tx_cmd[40]={0x0};
	u8 bt_rx_msg[40]={0x0};
	u8 num = 0;
	u8 len = 0;
    	u8 i = 0;
    	u8 ch = 0;
	u8 curr_step_pass = 0;
	u32 reg;
	int ret;
	int value;
	u8 gpio;
	u8 count = 4;
	const char *str_gpio = NULL;
	const char *str_gpio_action = NULL;
	u32 start_timep;
	u32 curr_timep;

	printf("=========================GPIO INIT=====================\n");

	while(count)
	{
		if(count == 4)
		{
			value = GPIO_CLEAR;
			str_gpio = "16";
			str_gpio_action = "output low";
		}
		else if(count == 3)
		{
			value = GPIO_SET;
			str_gpio = "16";
			str_gpio_action = "output high";
		}
		else if(count == 2)
		{
			value = GPIO_CLEAR;
			str_gpio = "19";
			str_gpio_action = "output low";

		}
		else if(count == 1)
		{
			value = GPIO_SET;
			str_gpio = "19";
			str_gpio_action = "output high";
		}else
			return -1;

		ret = gpio_lookup_name(str_gpio, NULL, NULL, &gpio);
		if (ret)
		{
			printf("GPIO: '%s' not found\n", str_gpio);
			return -1;
		}

		ret = gpio_request(gpio, "cmd_gpio");
		if (ret && ret != -EBUSY)
		{
			printf("gpio: requesting pin %u failed\n", gpio);
			return -1;
		}
		printf("GPIO_%s, action is %s\n", str_gpio, str_gpio_action);
		gpio_direction_output(gpio, value);
		count--;
		mdelay(10);
	}

	mdelay(100);

	printf("=========================UART_3 INIT=====================\n");
	ret = uclass_first_device_check(UCLASS_SERIAL, &uart);
	if (!uart) {
		printf("No valid serial device found\n");
		return CMD_RET_FAILURE;
	}

	while (uart)
	{
		if (ret >= 0) {
			reg = CPHYSADDR(ofnode_get_addr(uart->node));
			debug("%s, %08x\n", uart->name, reg);

			if (reg == MT7621_UART3_BASE)
			{
				printf("%s bring up, %08x\n", uart->name, reg);
				break;
			}
			else
				printf("%s, %08x\n", uart->name, reg);
		}

		ret = uclass_next_device_check(&uart);
	}

	if (!uart) {
		printf("UART3 device not found\n");
		return CMD_RET_FAILURE;
	}

	_serial_setbrg(uart, 115200);

	//FLOW 1: BT Power On
	printf("=========================FLOW 1=====================\n");
	memset(bt_tx_cmd, 0x0, sizeof(bt_tx_cmd));
	len = sizeof(pw_on_correct_code)/sizeof(u8);
	ret = get_ble_tx_cmd(BLE_PW_ON_IDX, bt_tx_cmd);
	if(ret > 0)
	{
		num = ret;
		printf("[BT Power On]: Tx Cmd=");
		for (i = 0; i < num; i++)
		{
			//putc(bt_tx_cmd[i]);
			_serial_putc(uart, bt_tx_cmd[i]);
			printf("%02x ",bt_tx_cmd[i]);
		}
		printf("\n");
	}
	printf("len= %d, num= %d\n", len, num);

	i = 0;
	memset(bt_rx_msg, 0x0, sizeof(bt_rx_msg));
	printf("[BT Power On]: Rx Msg=");
	start_timep = get_timer(0);
	while (i < len)
	{
		if(_serial_tstc(uart))
		{
			ch = _serial_getc(uart);
			//putc(ch);
			memcpy(&bt_rx_msg[i], &ch, sizeof(ch));
			printf("%02x ",bt_rx_msg[i]);
			i++;
		}

		curr_timep = get_timer(0);
		if(curr_timep - start_timep > 1500)
			break;
	}
	printf("\n");
	ret = judge_rx_msg(BLE_PW_ON_IDX, bt_rx_msg);
	if(ret == CMD_RET_SUCCESS)
	{
		curr_step_pass = TRUE;
		printf("[BT Power On Result] Success\n");
	}else
		printf("[BT Power On Result] Fail\n");

	//FLOW 2: HCI_RESET
	printf("\n=========================FLOW 2=====================\n");
	if(curr_step_pass == TRUE)
	{
		memset(bt_tx_cmd, 0x0, sizeof(bt_tx_cmd));
		len = sizeof(hic_reset_correct_code)/sizeof(u8);
		ret = get_ble_tx_cmd(BLE_HCI_RESET_IDX, bt_tx_cmd);
		if(ret > 0)
		{
			num = ret;
			printf("[HCI RESET]: Tx Cmd=");
			for (i = 0; i < num; i++)
			{
				//putc(bt_tx_cmd[i]);
				_serial_putc(uart, bt_tx_cmd[i]);
				printf("%02x ",bt_tx_cmd[i]);
			}
			printf("\n");
		}
		printf("len= %d, num= %d\n", len, num);

		i = 0;
		memset(bt_rx_msg, 0x0, sizeof(bt_rx_msg));
		printf("[HCI RESET]: Rx Msg=");
		start_timep = get_timer(0);
		while (i < len)
		{
			if (_serial_tstc(uart))
			{
				ch = _serial_getc(uart);
				//putc(ch);
				memcpy(&bt_rx_msg[i], &ch, sizeof(ch));
				printf("%02x ",bt_rx_msg[i]);
				i++;
			}

			curr_timep = get_timer(0);
			if(curr_timep - start_timep > 1500)
				break;
		}
		printf("\n");

		ret = judge_rx_msg(BLE_HCI_RESET_IDX, bt_rx_msg);
		if(ret == CMD_RET_SUCCESS)
		{
			curr_step_pass = TRUE;
			printf("[HIC RESET Result] Success\n");
		}
		else
		{
			curr_step_pass = FALSE;
			printf("[HIC RESET Result] Fail\n");
		}
	}

	//FLOW 3: HCI_LE_Set_Advertising_Parameters
	printf("\n=========================FLOW 3=====================\n");
	if(curr_step_pass == TRUE)
	{
		memset(bt_tx_cmd, 0x0, sizeof(bt_tx_cmd));
		len = sizeof(hic_le_set_adv_param_correct_code)/sizeof(u8);
		ret = get_ble_tx_cmd(BLE_HCI_SET_ADV_PARAM_IDX, bt_tx_cmd);
		if(ret > 0)
		{
			num = ret;
			printf("[HCI LE_Set Advertising Parameters]: Tx Cmd=");
			for (i = 0; i < num; i++)
			{
				//putc(bt_tx_cmd[i]);
				_serial_putc(uart, bt_tx_cmd[i]);
				printf("%02x ",bt_tx_cmd[i]);
			}
			printf("\n");
		}
		printf("len= %d, num= %d\n", len, num);

		i = 0;
		memset(bt_rx_msg, 0x0, sizeof(bt_rx_msg));
		printf("[HCI LE_Set Advertising Parameters]: Rx Msg=");
		start_timep = get_timer(0);
		while (i < len)
		{
			if (_serial_tstc(uart))
			{
				ch = _serial_getc(uart);
				//putc(ch);
				memcpy(&bt_rx_msg[i], &ch, sizeof(ch));
				printf("%02x ",bt_rx_msg[i]);
				i++;
			}

			curr_timep = get_timer(0);
			if(curr_timep - start_timep > 1500)
				break;
		}
		printf("\n");

		ret = judge_rx_msg(BLE_HCI_SET_ADV_PARAM_IDX, bt_rx_msg);
		if(ret == CMD_RET_SUCCESS)
		{
			curr_step_pass = TRUE;
			printf("[HIC LE  SET ADVERTISING PARAMETER Result] Success\n");
		}
		else
		{
			curr_step_pass = FALSE;
			printf("[HIC LE  SET ADVERTISING PARAMETER Result]  Fail\n");
		}
	}

	//FLOW 4: HCI_LE_Set_Advertising_Data
	printf("\n=========================FLOW 4=====================\n");
	if(curr_step_pass == TRUE)
	{
		memset(bt_tx_cmd, 0x0, sizeof(bt_tx_cmd));
		len = sizeof(hic_le_set_adv_data_correct_code)/sizeof(u8);
		ret = get_ble_tx_cmd(BLE_HCI_SET_ADV_DATA_IDX, bt_tx_cmd);
		if(ret > 0)
		{
			num = ret;
			printf("[HCI LE_Set Advertising Data]: Tx Cmd=");
			for (i = 0; i < num; i++)
			{
				//putc(bt_tx_cmd[i]);
				_serial_putc(uart, bt_tx_cmd[i]);
				printf("%02x ",bt_tx_cmd[i]);
			}
			printf("\n");
		}
		printf("len= %d, num= %d\n", len, num);

		i = 0;
		memset(bt_rx_msg, 0x0, sizeof(bt_rx_msg));
		printf("[HCI LE_Set Advertising Data]: Rx Msg=");
		start_timep = get_timer(0);
		while (i < len)
		{
			if(_serial_tstc(uart))
			{
				ch = _serial_getc(uart);
				//putc(ch);
				memcpy(&bt_rx_msg[i], &ch, sizeof(ch));
				printf("%02x ",bt_rx_msg[i]);
				i++;
			}

			curr_timep = get_timer(0);
			if(curr_timep - start_timep > 1500)
				break;
		}
		printf("\n");

		ret = judge_rx_msg(BLE_HCI_SET_ADV_DATA_IDX, bt_rx_msg);
		if(ret == CMD_RET_SUCCESS)
		{
			curr_step_pass = TRUE;
			printf("[HIC LE  SET ADVERTISING DATA Result] Success\n");
		}
		else
		{
			curr_step_pass = FALSE;
			printf("[HIC LE  SET ADVERTISING DATA Result] Fail\n");
		}
	}

	//FLOW 5: HCI_LE_Set_Scan_Response_Data
	printf("\n=========================FLOW 5=====================\n");
	if(curr_step_pass == TRUE)
	{
		memset(bt_tx_cmd, 0x0, sizeof(bt_tx_cmd));
		len = sizeof(hic_le_set_scan_resp_data_correct_code)/sizeof(u8);
		ret = get_ble_tx_cmd(BLE_HCI_SET_SCAN_RESP_DATA_IDX, bt_tx_cmd);
		if(ret > 0)
		{
			num = ret;
			printf("[HCI LE_Set Scan Response Data]: Tx Cmd=");
			for (i = 0; i < num; i++)
			{
				//putc(bt_tx_cmd[i]);
				_serial_putc(uart, bt_tx_cmd[i]);
				printf("%02x ",bt_tx_cmd[i]);
			}
			printf("\n");
		}
		printf("len= %d, num= %d\n", len, num);

		i = 0;
		memset(bt_rx_msg, 0x0, sizeof(bt_rx_msg));
		printf("[HCI LE_Set Scan Response Data ]: Rx Msg=");
		start_timep = get_timer(0);
		while (i < len)
		{
			if(_serial_tstc(uart))
			{
				ch = _serial_getc(uart);
				//putc(ch);
				memcpy(&bt_rx_msg[i], &ch, sizeof(ch));
				printf("%02x ",bt_rx_msg[i]);
				i++;
			}

			curr_timep = get_timer(0);
			if(curr_timep - start_timep > 1500)
				break;
		}
		printf("\n");

		ret = judge_rx_msg(BLE_HCI_SET_SCAN_RESP_DATA_IDX, bt_rx_msg);
		if(ret == CMD_RET_SUCCESS)
		{
			curr_step_pass = TRUE;
			printf("[HIC LE  SET SCAN RESPONSE Result] Success\n");
		}
		else
		{
			curr_step_pass = FALSE;
			printf("[HIC LE  SET SCAN RESPONSE Result] Fail\n");
		}
	}

	//FLOW 6: HCI_LE_Set_Advertising_Enable
	printf("\n=========================FLOW 6=====================\n");
	if(curr_step_pass == TRUE)
	{
		memset(bt_tx_cmd, 0x0, sizeof(bt_tx_cmd));
		len = sizeof(hic_le_set_adv_en_correct_code)/sizeof(u8);
		ret = get_ble_tx_cmd(BLE_HCI_SET_ADV_ENABLE_IDX, bt_tx_cmd);
		if(ret > 0)
		{
			num = ret;
			printf("[HCI LE_Set Advertising Enable]: Tx Cmd=");
			for (i = 0; i < num; i++)
			{
				//putc(bt_tx_cmd[i]);
				_serial_putc(uart, bt_tx_cmd[i]);
				printf("%02x ",bt_tx_cmd[i]);
			}
			printf("\n");
		}
		printf("len= %d, num= %d\n", len, num);

		i = 0;
		memset(bt_rx_msg, 0x0, sizeof(bt_rx_msg));
		printf("[HCI LE_Set Advertising Enable]: Rx Msg=");
		start_timep = get_timer(0);
		while (i < len)
		{
			if(_serial_tstc(uart))
			{
				ch = _serial_getc(uart);
				//putc(ch);
				memcpy(&bt_rx_msg[i], &ch, sizeof(ch));
				printf("%02x ",bt_rx_msg[i]);
				i++;
			}

			curr_timep = get_timer(0);
			if(curr_timep - start_timep > 1500)
				break;
		}
		printf("\n");

		ret = judge_rx_msg(BLE_HCI_SET_ADV_ENABLE_IDX, bt_rx_msg);
		if(ret == CMD_RET_SUCCESS)
		{
			curr_step_pass = TRUE;
			printf("[HIC LE  SET ADVERISTING ENABLE Result] Success\n");
		}
		else
		{
			curr_step_pass = FALSE;
			printf("[HIC LE  SET ADVERISTING ENABLE Result] Fail\n");
		}
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtk_bleadv, 1, 0, do_mtk_bleadv,	"Test MTK BLE Advertising", "this feature is based on uart3 function enable");
