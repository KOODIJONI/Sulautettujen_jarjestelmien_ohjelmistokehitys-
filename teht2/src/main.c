#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define RED_LED_NODE DT_ALIAS(led0)
#define GREEN_LED_NODE DT_ALIAS(led1)
#define BLUE_LED_NODE DT_ALIAS(led2)

#define BUTTON_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(RED_LED_NODE, gpios);
static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);
static const struct gpio_dt_spec blue_led = GPIO_DT_SPEC_GET(BLUE_LED_NODE, gpios);
//funktiot
void red_led_thread(void *arg1, void *arg2, void *arg3);
void green_led_thread(void *arg1, void *arg2, void *arg3);
void yellow_led_thread(void *arg1, void *arg2, void *arg3);
void pause_led_thread(void *arg1, void *arg2, void *arg3);
void button_interupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

void leds_off(void);

//threadit 
struct k_thread red_thread_data;
struct k_thread green_thread_data;
struct k_thread yellow_thread_data;
struct k_thread pause_thread_data;

//interrupt
static struct gpio_callback button_cb;
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

//threadien stackit
K_THREAD_STACK_DEFINE(red_thread_stack, 1024);
K_THREAD_STACK_DEFINE(green_thread_stack, 1024);
K_THREAD_STACK_DEFINE(yellow_thread_stack, 1024);
K_THREAD_STACK_DEFINE(pause_thread_stack, 1024);

//globalit muuttujat
int led_state = 0;

volatile bool button_pressed = false;
volatile int last_led_state = 0;
volatile int64_t last_time = 0;

//inittaa ledit
static void init_leds(void)
{
    gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_ACTIVE);

    leds_off(); //pois päältä
}

//pääohjelma
int main(void)
{
        // onko pinnit valamiit
    if (!device_is_ready(red_led.port)|| !device_is_ready(green_led.port) || !device_is_ready(blue_led.port)) {
        return 0;
    }

        // onko nappi valamis
    if (!device_is_ready(button.port)) {
        return 0;
    }

    init_leds(); // init ledit
    
    //threadit päälle
    k_thread_create(&red_thread_data, red_thread_stack, 
        K_THREAD_STACK_SIZEOF(red_thread_stack),
                    red_led_thread, NULL, NULL, NULL,
                    1, 0, K_NO_WAIT);

    k_thread_create(&green_thread_data, green_thread_stack, 
        K_THREAD_STACK_SIZEOF(green_thread_stack),
                    green_led_thread, NULL, NULL, NULL,
                    1, 0, K_NO_WAIT);

    k_thread_create(&yellow_thread_data, yellow_thread_stack, 
        K_THREAD_STACK_SIZEOF(yellow_thread_stack),
                    yellow_led_thread, NULL, NULL, NULL,
                    1, 0, K_NO_WAIT);

    k_thread_create(&pause_thread_data, pause_thread_stack, 
        K_THREAD_STACK_SIZEOF(pause_thread_stack),
                    pause_led_thread, NULL, NULL, NULL,
                    1, 0, K_NO_WAIT);
        // nappi inittaa
        gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
        
        gpio_init_callback(&button_cb, button_interupt_handler, BIT(button.pin));
        gpio_add_callback(button.port, &button_cb);

        // nappi keskeytys juttu
        gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

}

//punanen thredi
void red_led_thread(void *arg1, void *arg2, void *arg3)
{

        while (1) {
                if (led_state == 0) {

                        gpio_pin_set_dt(&red_led, 1);
                        k_msleep(1000);
                        gpio_pin_set_dt(&red_led, 0);
                        if (led_state == 0) { // jos tila ei kuulu threadille nii älä vaiha
                                led_state = 1;
                        }
                }
                k_msleep(10);
        }
}
// vihreä thredi
void green_led_thread(void *arg1, void *arg2, void *arg3)
{
     
        while (1) {

                if (led_state == 1) {
                        gpio_pin_set_dt(&green_led, 1);
                        k_msleep(1000);
                        gpio_pin_set_dt(&green_led, 0);
                        if (led_state == 1) {
                                led_state = 2;
                        }

                }
                k_msleep(10);
        }
}

//keltanen thredi
void yellow_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                if (led_state == 2) {
                        gpio_pin_set_dt(&red_led, 1);
                        gpio_pin_set_dt(&green_led, 1);
                        k_msleep(1000);
                        gpio_pin_set_dt(&red_led, 0);
                        gpio_pin_set_dt(&green_led, 0);
                        if (led_state == 2) {
                                led_state = 0;
                        }
                }
                k_msleep(10);
        }
}

//pause thredi
void pause_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                if (button_pressed) { // jos nappula nii 4 ja jos on jo neljä nii takas
                        if (led_state != 4) {
                                last_led_state = led_state;
                                led_state = 4;
                        } else {
                                led_state = last_led_state;
                        }
                        button_pressed = false;
                }

                if (led_state == 4) { 
                        leds_off(); //valot pois
                }

                k_msleep(10);
        }
}

void leds_off(void)
{
        //valot pois 
    gpio_pin_set_dt(&red_led, 0);// valo 1 pois
    gpio_pin_set_dt(&green_led, 0);// valo 2 pois
    gpio_pin_set_dt(&blue_led, 0);// valo 3 pois
}

//nappula keskeytys
void button_interupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    int64_t now = k_uptime_get(); //debounci
    if (now - last_time > 150) { // 150ms debounccia
        button_pressed = true;
        last_time = now;
    }
}