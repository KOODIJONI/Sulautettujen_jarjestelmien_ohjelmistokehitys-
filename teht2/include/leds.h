#ifndef LEDS_H
#define LEDS_H

int leds_init(void);
void leds_off(void);
void led_red_set(int val);
void led_green_set(int val);
void led_red_toggle(void);
void led_green_toggle(void);
void led_blue_set(int val);
void led_blue_toggle(void);
void led_yellow_toggle(void);
void led_yellow_set(int val);

#define RED_LED_NODE DT_ALIAS(led0)
#define GREEN_LED_NODE DT_ALIAS(led1)
#define BLUE_LED_NODE DT_ALIAS(led2)


#endif