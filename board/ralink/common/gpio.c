#include <asm-generic/gpio.h>
#include <errno.h>
#include <common.h>
#include "gpio.h"

static int gpio_input(unsigned int pin)
{
	int ret = 0;
	unsigned int gpio;
	int value;
	char gpio_number[128];
	sprintf(gpio_number, "%d", pin);
#if defined(CONFIG_DM_GPIO)
	ret = gpio_lookup_name(gpio_number, NULL, NULL, &gpio);
#else
	/* turn the gpio name into a gpio number */
	gpio = name_to_gpio(gpio_number);
#endif
	/* grab the pin before we tweak it */
	gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return -1;
	}

	gpio_direction_input(gpio);
	value = gpio_get_value(gpio);
	if (ret != -EBUSY)
		gpio_free(gpio);

	return value;
}

static void gpio_output(unsigned int pin, int value)
{
	int ret = 0;
	unsigned int gpio;
	char gpio_number[128];
	sprintf(gpio_number, "%d", pin);
#if defined(CONFIG_DM_GPIO)
	ret = gpio_lookup_name(gpio_number, NULL, NULL, &gpio);
#else
	/* turn the gpio name into a gpio number */
	gpio = name_to_gpio(gpio_number);
#endif
	/* grab the pin before we tweak it */
	gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return;
	}

	gpio_direction_output(gpio, value);
	if (ret != -EBUSY)
		gpio_free(gpio);
}

/**
 * system led blink once, 200 ms
 */
static void mt7621_sys_led_blink(void)
{
#ifdef CONFIG_MT7621_SYS_LED
	gpio_output(CONFIG_MT7621_SYS_LED_PIN, !CONFIG_MT7621_SYS_LED_LEVEL);
	mdelay(100);	// wait 100ms
	gpio_output(CONFIG_MT7621_SYS_LED_PIN, CONFIG_MT7621_SYS_LED_LEVEL);
	mdelay(100);	// wait 100ms
#endif // CONFIG_MT7621_SYS_LED
}

/**
 * @return 1 if button is pressed 2 seconds, else 0
 */
static int mt7621_is_button_pressed(int gpio, int trigLevel)
{
	int count = 0;
	for(count = 0; count < 10; count ++) {
		int val = gpio_input(gpio);
		if(val ^ trigLevel)
			return 0;
		mt7621_sys_led_blink();	// blink 10 times, 2 seconds
	}
	return 1;
}

static void mt7621_gpio_init(void)
{
#ifdef CONFIG_MT7621_EXTRA_GPIO1
	gpio_output(CONFIG_MT7621_EXTRA_GPIO1_PIN, CONFIG_MT7621_EXTRA_GPIO1_LEVEL);
#endif // CONFIG_MT7621_EXTRA_GPIO1

#ifdef CONFIG_MT7621_EXTRA_GPIO2
	gpio_output(CONFIG_MT7621_EXTRA_GPIO2_PIN, CONFIG_MT7621_EXTRA_GPIO2_LEVEL);
#endif // CONFIG_MT7621_EXTRA_GPIO2

#ifdef CONFIG_MT7621_EXTRA_GPIO3
	gpio_output(CONFIG_MT7621_EXTRA_GPIO3_PIN, CONFIG_MT7621_EXTRA_GPIO3_LEVEL);
#endif // CONFIG_MT7621_EXTRA_GPIO3

#ifdef CONFIG_MT7621_EXTRA_GPIO4
	gpio_output(CONFIG_MT7621_EXTRA_GPIO4_PIN, CONFIG_MT7621_EXTRA_GPIO4_LEVEL);
#endif // CONFIG_MT7621_EXTRA_GPIO4

#ifdef CONFIG_MT7621_EXTRA_GPIO5
	gpio_output(CONFIG_MT7621_EXTRA_GPIO5_PIN, CONFIG_MT7621_EXTRA_GPIO5_LEVEL);
#endif // CONFIG_MT7621_EXTRA_GPIO5

#ifdef CONFIG_MT7621_PWR_LED
	gpio_output(CONFIG_MT7621_PWR_LED_PIN, CONFIG_MT7621_PWR_LED_LEVEL);
#endif // CONFIG_MT7621_PWR_LED

#ifdef CONFIG_MT7621_SYS_LED
	gpio_output(CONFIG_MT7621_SYS_LED_PIN, CONFIG_MT7621_SYS_LED_LEVEL);
#endif // CONFIG_MT7621_SYS_LED

#ifdef CONFIG_MT7621_RST_BTN
	gpio_input(CONFIG_MT7621_RST_BTN_PIN);
#endif // CONFIG_MT7621_RST_BTN

#ifdef CONFIG_MT7621_WPS_BTN
	gpio_input(CONFIG_MT7621_WPS_BTN_PIN);
#endif // CONFIG_MT7621_WPS_BTN
}

#ifdef CONFIG_MT7621_RST_BTN
/**
 * @return 1 if reset button is pressed 3 seconds, else 0
 */
int mt7621_is_reset_pressed(void)
{
	return mt7621_is_button_pressed(CONFIG_MT7621_RST_BTN_PIN, CONFIG_MT7621_RST_BTN_LEVEL);
}
#endif // CONFIG_MT7621_RST_BTN

#ifdef CONFIG_MT7621_WPS_BTN
/**
 * @return 1 if wps button is pressed 3 seconds, else 0
 */
int mt7621_is_wps_pressed(void)
{
	return mt7621_is_button_pressed(CONFIG_MT7621_WPS_BTN_PIN, CONFIG_MT7621_WPS_BTN_LEVEL);
}
#endif // CONFIG_MT7621_WPS_BTN

#ifdef CONFIG_LAST_STAGE_INIT
int last_stage_init(void)
{
	mt7621_gpio_init();
	return 0;
}
#endif // CONFIG_LAST_STAGE_INIT
