# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2015 Google, Inc
# Written by Simon Glass <sjg@chromium.org>

ifdef CONFIG_NETGEAR_UPGRADE_MODEL_SET
obj-$(CONFIG_NETGEAR_UPGRADE_MODEL_SET_WAX202) += led_ctrl.o
else
obj-y += led-uclass.o
obj-$(CONFIG_LED_BCM6328) += led_bcm6328.o
obj-$(CONFIG_LED_BCM6358) += led_bcm6358.o
obj-$(CONFIG_$(SPL_)LED_GPIO) += led_gpio.o
endif
