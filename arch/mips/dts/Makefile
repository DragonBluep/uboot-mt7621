# SPDX-License-Identifier: GPL-2.0+

dtb-$(CONFIG_TARGET_AP121) += ap121.dtb
dtb-$(CONFIG_TARGET_AP143) += ap143.dtb
dtb-$(CONFIG_TARGET_BOSTON) += img,boston.dtb
dtb-$(CONFIG_TARGET_MALTA) += mti,malta.dtb
dtb-$(CONFIG_TARGET_PIC32MZDASK) += pic32mzda_sk.dtb
dtb-$(CONFIG_TARGET_XILFPGA) += nexys4ddr.dtb
dtb-$(CONFIG_BOARD_COMTREND_AR5315U) += comtrend,ar-5315u.dtb
dtb-$(CONFIG_BOARD_COMTREND_AR5387UN) += comtrend,ar-5387un.dtb
dtb-$(CONFIG_BOARD_COMTREND_CT5361) += comtrend,ct-5361.dtb
dtb-$(CONFIG_BOARD_COMTREND_VR3032U) += comtrend,vr-3032u.dtb
dtb-$(CONFIG_BOARD_COMTREND_WAP5813N) += comtrend,wap-5813n.dtb
dtb-$(CONFIG_BOARD_HUAWEI_HG556A) += huawei,hg556a.dtb
dtb-$(CONFIG_BOARD_NETGEAR_CG3100D) += netgear,cg3100d.dtb
dtb-$(CONFIG_BOARD_NETGEAR_DGND3700V2) += netgear,dgnd3700v2.dtb
dtb-$(CONFIG_BOARD_SAGEM_FAST1704) += sagem,f@st1704.dtb
dtb-$(CONFIG_BOARD_TPLINK_WDR4300) += tplink_wdr4300.dtb
dtb-$(CONFIG_BOARD_MT7621_RFB) += mt7621_nor_rfb.dtb
dtb-$(CONFIG_BOARD_MT7621_NAND_RFB) += mt7621_nand_rfb.dtb
dtb-$(CONFIG_BOARD_MT7621_NOR_TEMPLATE) += mt7621_nor_template.dtb
dtb-$(CONFIG_BOARD_MT7621_NAND_TEMPLATE) += mt7621_nand_template.dtb
dtb-$(CONFIG_BOARD_H3C_TX1801_PLUS) += h3c,tx1801-plus.dtb
dtb-$(CONFIG_BOARD_RAISECOM_MSG1500X00) += raisecom,msg1500-x00.dtb
dtb-$(CONFIG_BOARD_SIM_SIMAX1800T) += sim,simax1800t.dtb
dtb-$(CONFIG_BOARD_ASUS_RTAC1200GU) += asus,rt-ac1200gu.dtb
dtb-$(CONFIG_BOARD_DLINK_DIR878A1) += dlink,dir-878-a1.dtb
dtb-$(CONFIG_BOARD_LINKSURE_SG5) += linksure,sg5.dtb

targets += $(dtb-y)

# Add any required device tree compiler flags here
DTC_FLAGS +=

PHONY += dtbs
dtbs: $(addprefix $(obj)/, $(dtb-y))
	@:

clean-files := *.dtb
