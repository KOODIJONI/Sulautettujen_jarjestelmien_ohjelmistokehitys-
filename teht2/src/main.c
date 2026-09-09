#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include "main.h"
#include "leds.h"
#include "buttons.h"
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
void button_irs(const struct device *dev, struct gpio_callback *cb, uint32_t pins);


//threadit 
struct k_thread red_thread_data;
struct k_thread green_thread_data;
struct k_thread yellow_thread_data;
struct k_thread yellow_blink_thread_data;

struct k_thread pause_thread_data;

//interrupt
static struct gpio_callback button_cb[5];

// nappula speksaus
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON1_NODE, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(BUTTON2_NODE, gpios);
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET(BUTTON3_NODE, gpios);
static const struct gpio_dt_spec button4 = GPIO_DT_SPEC_GET(BUTTON4_NODE, gpios);
static const struct gpio_dt_spec button5 = GPIO_DT_SPEC_GET(BUTTON5_NODE, gpios);

// array nappuloista
static const struct gpio_dt_spec *all_buttons[5] = {
    &button1, &button2, &button3, &button4, &button5
};

//threadien stackit
K_THREAD_STACK_DEFINE(red_thread_stack, 1024);
K_THREAD_STACK_DEFINE(green_thread_stack, 1024);
K_THREAD_STACK_DEFINE(yellow_thread_stack, 1024);
K_THREAD_STACK_DEFINE(yellow_blink_thread_stack, 1024);
K_THREAD_STACK_DEFINE(pause_thread_stack, 1024);

//globalit muuttujat
volatile int led_state = RED;
volatile int last_led_state = RED;

volatile bool button_pressed[5] = {false};
volatile int64_t last_times[5] = {0};


//pääohjelma
int main(void)
{
        // Napit valiina?
       if (buttons_init() != 0) {
                return -1;
        }

        // Init ledits
        if (leds_init() != 0) {
                return -1;
        }

        // Keskeytykset nappuloille
        for (int i = 0; i < 5; i++) {
                gpio_pin_configure_dt(all_buttons[i], GPIO_INPUT | GPIO_PULL_UP);
                gpio_pin_interrupt_configure_dt(all_buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
        }

        for (int i = 0; i < 5; i++) {
                gpio_init_callback(&button_cb[i], button_irs, BIT(all_buttons[i]->pin));
                gpio_add_callback(all_buttons[i]->port, &button_cb[i]);
        }

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
                if (led_state == RED) {

                        led_red_set(1);
                        k_msleep(1000);
                        led_red_set(0);
                        if (led_state == RED) { // jos tila ei kuulu threadille nii älä vaiha
                                led_state = GREEN;
                        }
                }
                k_msleep(10);
        }
}
// vihreä thredi
void green_led_thread(void *arg1, void *arg2, void *arg3)
{
     
        while (1) {

                if (led_state == GREEN) {
                        led_green_set(1);
                        k_msleep(1000);
                        led_green_set(0);
                        if (led_state == GREEN) {
                                led_state = YELLOW;
                        }

                }
                k_msleep(10);
        }
}

//keltanen thredi
void yellow_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                if (led_state == YELLOW) {
                        led_red_set(1);
                        led_green_set(1);
                        k_msleep(1000);
                        led_red_set(0);
                        led_green_set(0);
                        if (led_state == YELLOW) {
                                led_state = RED;
                        }
                }
                k_msleep(10);
        }
}

void yellow_blink_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                if (led_state == YELLOW_BLINK) {
                                
                        led_yellow_toggle();
                        k_msleep(500);
                }
                k_msleep(10);
        }
}
//pause thredi
void pause_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                if (button_was_pressed(0) ) { // jos nappula nii 4 ja jos on jo neljä nii takas
                        
                        if (led_state < PAUSE) {

                                last_led_state = led_state;
                                led_state = PAUSE;
                                leds_off(); //valot pois

                        } 
                        else {
                                led_state = last_led_state;
                        }
                }
                if (button_was_pressed(1)) { // jos nappula nii 1

                        if (led_state ==PAUSE) {
                                led_red_set(1);
                                led_state = RED_ON;
                        }
                        else if (led_state ==RED_ON) {
                                led_red_set(0);
                                led_state = PAUSE;
                        }
                }
                if (button_was_pressed(2)) { // jos nappula nii 2
                        if (led_state ==PAUSE) {
                                led_yellow_set(1);
                                led_state = RED_GREEN_ON;
                        }else if(led_state ==RED_GREEN_ON){
                                led_yellow_set(0);
                                led_state = PAUSE;
                        }
                }
                if (button_was_pressed(3)) { // jos nappula nii 3
                        if (led_state ==PAUSE) {
                                led_green_set(1);
                                led_state = GREEN_ON;
                        }else if(led_state ==GREEN_ON){
                                led_green_set(0);
                                led_state = PAUSE;
                        }
                }
                if (button_was_pressed(4)) { // jos nappula nii 5
                        //blink yellow
                        if (led_state ==PAUSE) {
                                led_state = YELLOW_BLINK;
                        }else  if(led_state ==YELLOW_BLINK){
                                led_state = PAUSE;
                                leds_off();
                        }
                }

    
                k_msleep(10);
        }
}
//nappula keskeytys
void button_irs(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
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