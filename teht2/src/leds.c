#include <zephyr/drivers/gpio.h>
#include "leds.h"
#include "main.h"   // for RED_LED_NODE etc if still needed

static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(RED_LED_NODE, gpios);
static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);
static const struct gpio_dt_spec blue_led = GPIO_DT_SPEC_GET(BLUE_LED_NODE, gpios);

int leds_init(void) {
        if (!device_is_ready(red_led.port) || !device_is_ready(green_led.port) || !device_is_ready(blue_led.port)) {
                return -ENODEV;
        }
        gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_ACTIVE);
        gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_ACTIVE);
        gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_ACTIVE);
        leds_off();
        return 0;
}

void leds_off(void) {
        gpio_pin_set_dt(&red_led, 0);
        gpio_pin_set_dt(&green_led, 0);
        gpio_pin_set_dt(&blue_led, 0);
}

void led_red_set(int val)   { gpio_pin_set_dt(&red_led, val); }
void led_green_set(int val) { gpio_pin_set_dt(&green_led, val); }
void led_blue_set(int val)  { gpio_pin_set_dt(&blue_led, val); }
void led_yellow_set(int val) {
    gpio_pin_set_dt(&red_led, val);
    gpio_pin_set_dt(&green_led, val);
}

void led_red_toggle(void)   { gpio_pin_toggle_dt(&red_led); }
void led_green_toggle(void) { gpio_pin_toggle_dt(&green_led); }
void led_blue_toggle(void)  { gpio_pin_toggle_dt(&blue_led); }
void led_yellow_toggle(void) { 
    gpio_pin_toggle_dt(&red_led); 
    gpio_pin_toggle_dt(&green_led); 
}