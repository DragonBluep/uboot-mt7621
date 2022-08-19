#include <serial.h>
#include <common.h>
#include <config.h>

#include <command.h>
#include <asm/gpio.h>
#include <asm/led.h>
#include <console.h>

typedef struct {
    int led_id;
    int mask;
    int state;
    int active;
    int delay_on; //ms
    int delay_off; //ms
    ulong cnt; //ms
} led_dev_t;

struct led_control_t;
struct led_control_inst_t;
static void ledctl_init_led_dev(struct led_control_inst_t *self);
static void ledctl_time_tick(struct led_control_inst_t *self, ulong timestamp);
static void ledctl_set_led_state(struct led_control_inst_t *self, int mask, int state, int delayon, int delayoff);

typedef struct led_control_inst_t{
    struct led_control_t *class;
    int last_tick;
    led_dev_t led_dev[];
}led_control_inst;

typedef struct led_control_t{
    void (*init_led_dev)(led_control_inst *self);
    void (*time_tick)(led_control_inst *self, ulong timestamp);
    void (*set_led_state)(led_control_inst *self, int mask, int state, int delayon, int delayoff);
} led_control;

static led_control led_control_class = {
    .init_led_dev = ledctl_init_led_dev,
    .time_tick = ledctl_time_tick,
    .set_led_state = ledctl_set_led_state
};


static void ledctl_init_led_dev(led_control_inst *self)
{
    int ret;
    int idx;
    for(idx = 0; self->led_dev[idx].led_id != -1; idx++)
    {
        led_dev_t *led = &self->led_dev[idx];
        ret = gpio_requestf(led->led_id, "gpio%d", led->led_id);
        if (ret && ret != -EBUSY)
        {
            printf("gpio: requesting pin %u failed\n", led->led_id);
            return;
        }
        if (led->state == STATUS_LED_ON)
        {
            gpio_direction_output(led->led_id, led->active);
        }
        else if (led->state == STATUS_LED_OFF)
        {
            gpio_direction_output(led->led_id, !led->active);
        }
    }
}

static void ledctl_time_tick(led_control_inst *self, ulong timestamp)
{
    int idx;
    ulong interval = timestamp - self->last_tick;
    for(idx = 0; self->led_dev[idx].led_id != -1; idx++)
    {
        led_dev_t *led = &self->led_dev[idx];
        if (led->state != STATUS_LED_BLINKING)
        {
            continue;
        }
        led->cnt += interval;
        if (led->cnt < led->delay_on)
        {
            gpio_direction_output(led->led_id, led->active);
        }
        else if(led->cnt < led->delay_on + led->delay_off)
        {
            gpio_direction_output(led->led_id, !led->active);
        }
        else
        {
            gpio_direction_output(led->led_id, led->active);
            led->cnt = 0;
        }
    } 
    self->last_tick = timestamp;
}

static void ledctl_set_led_state(led_control_inst *self, int mask, int state, int delayon, int delayoff)
{
    int idx;
    for(idx = 0; self->led_dev[idx].led_id != -1; idx++)
    {
        led_dev_t *led = &self->led_dev[idx];
        if(led->mask & mask)
        {
            if (led->state != state)
            {
                led->state = state;
                if (state == STATUS_LED_BLINKING)
                {
                    gpio_direction_output(led->led_id, led->active);
                    led->delay_on = delayon;
                    led->delay_off = delayoff;
                }
                else if (state == STATUS_LED_ON)
                {
                    gpio_direction_output(led->led_id, led->active);
                }
                else if (state == STATUS_LED_OFF)
                {
                    gpio_direction_output(led->led_id, !led->active);
                }
            }
            break;
        }
    }
};

led_control_inst led_dev_control = {
    .class = &led_control_class,
    .last_tick = 0,
    .led_dev = {
#ifdef CONFIG_NETGEAR_UPGRADE_MODEL_SET_WAX202
    {POWER_GREEN, POWER_GREEN_SELECTOR, STATUS_LED_OFF, 0, 0, 0},
    {POWER_RED, POWER_RED_SELECTOR, STATUS_LED_ON, 0, 0, 0},

    {NET_GREEN, NET_GREEN_SELECTOR, STATUS_LED_OFF, 0, 0, 0},//pull up active
    {NET_RED, NET_RED_SELECTOR, STATUS_LED_OFF, 0, 0, 0},
    {WIFIN_GREEN, WIFIN_GREEN_SELECTOR, STATUS_LED_OFF, 0, 0, 0},
    {WIFIN_RED, WIFIN_RED_SELECTOR, STATUS_LED_OFF, 0, 0, 0},
    {WIFIA_GREEN, WIFIA_GREEN_SELECTOR, STATUS_LED_OFF, 0, 0, 0},//pull up active
    {WIFIA_RED, WIFIA_RED_SELECTOR, STATUS_LED_OFF, 0, 0, 0},

#else
        { POWER_GREEN, POWER_GREEN_SELECTOR, STATUS_LED_OFF,  LED_ACTIVE_LOW,  0, 0, 0 },
        { POWER_RED,   POWER_RED_SELECTOR,   STATUS_LED_ON,  LED_ACTIVE_HIGH,  0, 0, 0 },
        //{ WPS_GREEN,   WPS_GREEN_SELECTOR,   STATUS_LED_OFF, LED_ACTIVE_HIGH, 0, 0, 0 },
        { CLINK_RED,   CLINK_GREEN_SELECTOR, STATUS_LED_OFF, LED_ACTIVE_LOW,  0, 0, 0 },
        { CLINK_GREEN, CLINK_GREEN_SELECTOR, STATUS_LED_OFF, LED_ACTIVE_LOW,  0, 0, 0 },
        { RLINK_GREEN, RLINK_GREEN_SELECTOR, STATUS_LED_OFF, LED_ACTIVE_HIGH, 0, 0, 0 },
        { RLINK_RED,   RLINK_GREEN_SELECTOR, STATUS_LED_OFF, LED_ACTIVE_HIGH, 0, 0, 0 },
#endif
        { -1,          0,                    0,              0,               0, 0, 0 }
    },
};

int led_dev_init(void)
{
    led_dev_control.class->init_led_dev(&led_dev_control);

    return 0;
}

void led_set_state(int mask, int state, int delayon, int delayoff)
{
    led_dev_control.class->set_led_state(&led_dev_control, mask, state, delayon, delayoff);
}

void led_time_tick(ulong times)
{
    led_dev_control.class->time_tick(&led_dev_control, times);
}

int  get_led_cmd(char *var)
{
	if (strcmp(var, "clear") == 0) {
		return STATUS_LED_OFF;
	}
	if (strcmp(var, "set") == 0) {
		return STATUS_LED_ON;
	}
	if (strcmp(var, "blink") == 0)
		return STATUS_LED_BLINKING;
	return -1;
}

int do_led (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int cmd;

	/* Validate arguments */
	if ((argc != 4)) {
		return CMD_RET_USAGE;
	}

	cmd = get_led_cmd(argv[1]);
	if (cmd < 0) {
		return CMD_RET_USAGE;
	}

	if (strcmp("power", argv[2]) == 0)
	{
		if (strcmp("green", argv[3]) == 0)
		{
			led_set_state(POWER_GREEN_SELECTOR, cmd, 500, 500);
		}

		if (strcmp("red", argv[3]) == 0)
		{
			led_set_state(POWER_RED_SELECTOR, cmd, 500, 500);
		}
	}
	if (strcmp("net", argv[2]) == 0)
	{
		if (strcmp("green", argv[3]) == 0)
		{
			led_set_state(NET_GREEN_SELECTOR, cmd, 500, 500);
		}

		if (strcmp("red", argv[3]) == 0)
		{
			led_set_state(NET_RED_SELECTOR, cmd, 500, 500);
		}
	}
	if (strcmp("wifin", argv[2]) == 0)
	{
		if (strcmp("green", argv[3]) == 0)
		{
			led_set_state(WIFIN_GREEN_SELECTOR, cmd, 500, 500);
		}

		if (strcmp("red", argv[3]) == 0)
		{
			led_set_state(WIFIN_RED_SELECTOR, cmd, 500, 500);
		}
	}
	if (strcmp("wifia", argv[2]) == 0)
	{
		if (strcmp("green", argv[3]) == 0)
		{
			led_set_state(WIFIA_GREEN_SELECTOR, cmd, 500, 500);
		}

		if (strcmp("red", argv[3]) == 0)
		{
			led_set_state(WIFIA_RED_SELECTOR, cmd, 500, 500);
		}
	}

	if(cmd == STATUS_LED_BLINKING)
	{
		while(1)
		{
			if (ctrlc()) break;
			led_time_tick(get_timer(0));
		}
	}

	return 0;
}

U_BOOT_CMD(
	led, 4, 1, do_led,
	"<set|clear|blink>  <power|net|wifin|wifia>  <green|red>",
	"<set|clear|blink>  <power|net|wifin|wifia>  <green|red>"
);
