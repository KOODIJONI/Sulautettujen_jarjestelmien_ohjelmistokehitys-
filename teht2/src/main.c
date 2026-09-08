#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
//includet aina ekana

/*
Tehtävä 2 Joni Mäyrä
Sulautetun ohjelmoinnin kurssi

Pistevaatimus täydet pisteet koska tein tehtävässä vaadittavat asiat

(vol -) nappi.
        Sekvenssin hallinta
(vol +) nappi.
        Punainen LED päälle/pois
(nappi 2) nappi.
        Keltanen LED päälle/pois
(nappi 3) nappi.
        Vihreä LED päälle/pois
(nappi 4) nappi.
        Keltainen LED vilkkuu 500ms välein

Toteutettu RTOS arkkitehtuuri mallilla

        .--.
       |o_o |
       |:_/ |
      //   \ \
     (|     | )
    /'\_   _/`\
    \___)=(___/

*/
#define RED_LED_NODE DT_ALIAS(led0)
#define GREEN_LED_NODE DT_ALIAS(led1)
#define BLUE_LED_NODE DT_ALIAS(led2)

#define BUTTON_NODE DT_ALIAS(sw0)
#define BUTTON_NODE2 DT_ALIAS(sw1)
#define BUTTON_NODE3 DT_ALIAS(sw2)
#define BUTTON_NODE4 DT_ALIAS(sw3)
#define BUTTON_NODE5 DT_ALIAS(sw4)

static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(RED_LED_NODE, gpios);
static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);
static const struct gpio_dt_spec blue_led = GPIO_DT_SPEC_GET(BLUE_LED_NODE, gpios);
//funktiot
void red_led_thread(void *arg1, void *arg2, void *arg3);
void green_led_thread(void *arg1, void *arg2, void *arg3);
void yellow_led_thread(void *arg1, void *arg2, void *arg3);
void yellow_blink_thread(void *arg1, void *arg2, void *arg3);
void pause_led_thread(void *arg1, void *arg2, void *arg3);

void button_irs(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

void leds_off(void);

//threadit 
struct k_thread red_thread_data;
struct k_thread green_thread_data;
struct k_thread yellow_thread_data;
struct k_thread yellow_blink_thread_data;

struct k_thread pause_thread_data;

//interrupt
static struct gpio_callback button_cb;

static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(BUTTON_NODE2, gpios);
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET(BUTTON_NODE3, gpios);
static const struct gpio_dt_spec button4 = GPIO_DT_SPEC_GET(BUTTON_NODE4, gpios);
static const struct gpio_dt_spec button5 = GPIO_DT_SPEC_GET(BUTTON_NODE5, gpios);

//threadien stackit
K_THREAD_STACK_DEFINE(red_thread_stack, 1024);
K_THREAD_STACK_DEFINE(green_thread_stack, 1024);
K_THREAD_STACK_DEFINE(yellow_thread_stack, 1024);
K_THREAD_STACK_DEFINE(yellow_blink_thread_stack, 1024);
K_THREAD_STACK_DEFINE(pause_thread_stack, 1024);

//globalit muuttujat
int led_state = 0;

volatile bool button_pressed[5] = {false};
volatile int last_led_state = 0;

volatile int64_t last_times[5] = {0};

//inittaa ledit
static int init_leds(void)
{

        if (!device_is_ready(red_led.port)|| !device_is_ready(green_led.port) || !device_is_ready(blue_led.port)) {
                return -ENODEV;
        }


        gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_ACTIVE);
        gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_ACTIVE);
        gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_ACTIVE);

        leds_off(); 
        return 0;
}

//pääohjelma
int main(void)
{
        // Napit valiina?
        const struct gpio_dt_spec *all_buttons[] = {&button1, &button2, &button3, &button4, &button5};
        for (int i = 0; i < 5; i++) {
                if (!device_is_ready(all_buttons[i]->port)) {
                return -1;
                }
        }

        // Init ledits
        if (init_leds() != 0) {
                return -1;
        }

        // Keskeytykset nappuloille
        for (int i = 0; i < 5; i++) {
                gpio_pin_configure_dt(all_buttons[i], GPIO_INPUT | GPIO_PULL_UP);
                gpio_pin_interrupt_configure_dt(all_buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
        }

        // Setuppaa callbackit nappuloille
        gpio_port_pins_t pin_mask = BIT(button1.pin) | BIT(button2.pin) | 
                                        BIT(button3.pin) | BIT(button4.pin) | BIT(button5.pin);
        gpio_init_callback(&button_cb, button_irs, pin_mask);
        
        gpio_add_callback(button1.port, &button_cb);

        //thredit
        k_thread_create(&red_thread_data, red_thread_stack, 
                K_THREAD_STACK_SIZEOF(red_thread_stack),
                red_led_thread, NULL, NULL, NULL, 1, 0, K_NO_WAIT);

        k_thread_create(&green_thread_data, green_thread_stack, 
                K_THREAD_STACK_SIZEOF(green_thread_stack),
                green_led_thread, NULL, NULL, NULL, 1, 0, K_NO_WAIT);

        k_thread_create(&yellow_thread_data, yellow_thread_stack, 
                K_THREAD_STACK_SIZEOF(yellow_thread_stack),
                yellow_led_thread, NULL, NULL, NULL, 1, 0, K_NO_WAIT);

        k_thread_create(&yellow_blink_thread_data, yellow_blink_thread_stack,
                K_THREAD_STACK_SIZEOF(yellow_blink_thread_stack),
                yellow_blink_thread, NULL, NULL, NULL, 1, 0, K_NO_WAIT);

        k_thread_create(&pause_thread_data, pause_thread_stack, 
                K_THREAD_STACK_SIZEOF(pause_thread_stack),
                pause_led_thread, NULL, NULL, NULL, 1, 0, K_NO_WAIT);

        return 0;
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

void yellow_blink_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                if (led_state == 8) {
                        gpio_pin_toggle_dt(&red_led);
                        gpio_pin_toggle_dt(&green_led);
                        k_msleep(500);
                }
                k_msleep(10);
        }
}
//pause thredi
void pause_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                if (button_pressed[0] ) { // jos nappula nii 4 ja jos on jo neljä nii takas
                        if (led_state != 4) {
                                last_led_state = led_state;
                                led_state = 4;
                                leds_off(); //valot pois

                        } else {
                                led_state = last_led_state;
                        }
                        button_pressed[0] = false;
                }
                if (button_pressed[1]) { // jos nappula nii 1
                        if (led_state ==4) {
                                gpio_pin_set_dt(&red_led, 1);
                                led_state = 5;
                        }
                        else if (led_state ==5) {
                                gpio_pin_set_dt(&red_led, 0);
                                led_state = 4;
                        }
                        button_pressed[1] = false;
                }
                if (button_pressed[2]) { // jos nappula nii 2
                        if (led_state ==4) {
                                gpio_pin_set_dt(&green_led, 1);
                                gpio_pin_set_dt(&red_led, 1);
                                led_state = 6;
                        }else if(led_state ==6){
                                gpio_pin_set_dt(&green_led, 0);
                                gpio_pin_set_dt(&red_led, 0);
                                led_state = 4;
                        }
                        button_pressed[2] = false;
                }
                if (button_pressed[3]) { // jos nappula nii 3
                        if (led_state ==4) {
                                gpio_pin_set_dt(&green_led, 1);
                                led_state = 7;
                        }else if(led_state ==7){
                                gpio_pin_set_dt(&green_led, 0);
                                led_state = 4;
                        }
                        button_pressed[3] = false;
                }
                if (button_pressed[4]) { // jos nappula nii 5
                        //blink yellow
                        if (led_state ==4) {
                                led_state = 8;
                        }else  if(led_state ==8){
                                led_state = 4;
                                leds_off();
                        }
                        button_pressed[4] = false;
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
void button_irs(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
        if (pins & BIT(button1.pin)) {
                int64_t current_time = k_uptime_get();
                if (current_time - last_times[0] > 200) {
                        button_pressed[0] = true;
                        last_times[0] = current_time;
                }
        }
        if (pins & BIT(button2.pin)) {
                int64_t current_time = k_uptime_get();
                if (current_time - last_times[1] > 200) {
                        button_pressed[1] = true;
                        last_times[1] = current_time;
                }
        }
        if (pins & BIT(button3.pin)) {
                int64_t current_time = k_uptime_get();
                if (current_time - last_times[2] > 200) {
                        button_pressed[2] = true;
                        last_times[2] = current_time;
                }
        }
        if (pins & BIT(button4.pin)) {
                int64_t current_time = k_uptime_get();
                if (current_time - last_times[3] > 200) {
                        button_pressed[3] = true;
                        last_times[3] = current_time;
                }
        }
        if (pins & BIT(button5.pin)) {
                int64_t current_time = k_uptime_get();
                if (current_time - last_times[4] > 200) {
                        button_pressed[4] = true;
                        last_times[4] = current_time;
                }
        }
                
}