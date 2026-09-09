#include <zephyr/drivers/gpio.h>
#include "buttons.h"
#include "main.h"

static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON1_NODE, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(BUTTON2_NODE, gpios);
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET(BUTTON3_NODE, gpios);
static const struct gpio_dt_spec button4 = GPIO_DT_SPEC_GET(BUTTON4_NODE, gpios);
static const struct gpio_dt_spec button5 = GPIO_DT_SPEC_GET(BUTTON5_NODE, gpios);

static const struct gpio_dt_spec *all_buttons[5] = { &button1, &button2, &button3, &button4, &button5 };
static struct gpio_callback button_cb[5];
static volatile bool button_pressed[5] = {false};
static volatile int64_t last_times[5] = {0};

static void button_irs(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
        for (int i = 0; i < 5; i++) {
                if (dev == all_buttons[i]->port && (pins & BIT(all_buttons[i]->pin))) {
                        int64_t now = k_uptime_get();
                        if ((now - last_times[i]) > 200) {
                                button_pressed[i] = true;
                                last_times[i] = now;
                        }
                }
        }
}

int buttons_init(void) {
        for (int i = 0; i < 5; i++) {
                if (!device_is_ready(all_buttons[i]->port)) return -1;
        }
        for (int i = 0; i < 5; i++) {
                gpio_pin_configure_dt(all_buttons[i], GPIO_INPUT | GPIO_PULL_UP);
        }
        for (int i = 0; i < 5; i++) {
                gpio_init_callback(&button_cb[i], button_irs, BIT(all_buttons[i]->pin));
                gpio_add_callback(all_buttons[i]->port, &button_cb[i]);
        }
        for (int i = 0; i < 5; i++) {
                gpio_pin_interrupt_configure_dt(all_buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
        }
        return 0;
}

bool button_was_pressed(int i) {
        bool val = button_pressed[i];
        button_pressed[i] = false;
        return val;
}