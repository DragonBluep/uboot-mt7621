#ifndef __LEOPARD_EVB_API_LED_H__
#define __LEOPARD_EVB_API_LED_H__

#define STATUS_LED_OFF      0
#define STATUS_LED_BLINKING 1
#define STATUS_LED_ON       2

#define LED_ACTIVE_LOW      0
#define LED_ACTIVE_HIGH     1

#define POWER_GREEN (16)
#define POWER_RED (7)

#define WPS_GREEN (17)
#define RLINK_GREEN (5)
#define RLINK_RED (11)
#define CLINK_GREEN (13)
#define CLINK_RED (14)

#define POWER_GREEN_SELECTOR 0x1
#define POWER_RED_SELECTOR 0x2
#define POWER_AMBER_SELECTOR 0x4
#define WPS_GREEN_SELECTOR 0x8

#define CLINK_GREEN_SELECTOR 0x10
#define CLINK_RED_SELECTOR 0x20
#define CLINK_AMBER_SELECTOR 0x40
#define RLINK_GREEN_SELECTOR 0x80
#define RLINK_RED_SELECTOR 0x100
#define RLINK_AMBER_SELECTOR 0x200


void led_set_state(int mask, int state, int delayon, int delayoff);
void led_time_tick(ulong times);
int  led_dev_init(void);


/* demo
 led_set_state(POWER_AMBER_SELECTOR, STATUS_LED_BLINKING, 250, 750);

 //poll call
 led_time_tick(current_time); 
*/

#endif
