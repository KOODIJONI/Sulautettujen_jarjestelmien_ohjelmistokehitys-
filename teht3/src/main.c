#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>
#include "main.h"
#include "leds.h"
#include "dispatcher.h"
//includet aina ekana

/*
Tehtävä 2 Joni Mäyrä
Sulautetun ohjelmoinnin kurssi

Pistevaatimus täydet pisteet koska tein tehtävässä vaadittavat asiat

1. Sekvenssin vastaanotto ja tallennus

2. dispatcher-taski

3. condition variablet käytössä

4. release-signaali valotaskissa

5. sekvenssin ajastus

6. refaktorointi

7. toisto

        .--.
       |o_o |
       |:_/ |
      //   \ \
     (|     | )
    /'\_   _/`\
    \___)=(___/

*/


//threadit 
#define STACK_SIZE 512
#define THREAD_PRIORITY 1

K_THREAD_DEFINE(red_tid, STACK_SIZE, red_led_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(green_tid, STACK_SIZE, green_led_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_tid, STACK_SIZE, yellow_led_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(fifo_consumer_tid, STACK_SIZE, fifo_consumer_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(sequence_tid, STACK_SIZE, sequence_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);
//interrupt

//Mutexi ja conditional varit
K_MUTEX_DEFINE(sync_mutex);
K_CONDVAR_DEFINE(red_cond);
K_CONDVAR_DEFINE(green_cond);
K_CONDVAR_DEFINE(yellow_cond);

//semiforet
K_SEM_DEFINE(seq_sem, 0, 1);
K_SEM_DEFINE(release_sem,0,1);

//syötetyn valon delay
static volatile int active_delay;

static volatile bool sequence_active = false;

//T moodi puskuri
static command_t cmd_buffer[MAX_COMMANDS];
static size_t cmd_count = 0;

//pääohjelma
int main(void)
{

        // Init ledits
        if (leds_init() != 0) {
                return -1;
        }

        if (init_uart_dispatcher() != 0) {
                return -1;
        }

}

//punanen thredi
void red_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                k_mutex_lock(&sync_mutex, K_FOREVER);

                k_condvar_wait(&red_cond, &sync_mutex, K_FOREVER);
                int delay = active_delay;
                k_mutex_unlock(&sync_mutex);

                led_red_set(1);
                k_msleep(delay);
                led_red_set(0);

                k_sem_give(&release_sem);
        }
}
// vihreä thredi
void green_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                k_mutex_lock(&sync_mutex, K_FOREVER);

                k_condvar_wait(&green_cond, &sync_mutex, K_FOREVER);
                int delay = active_delay;
                k_mutex_unlock(&sync_mutex);

                led_green_set(1);
                k_msleep(delay);
                led_green_set(0);

                k_sem_give(&release_sem);
        }
}

//keltanen thredi
void yellow_led_thread(void *arg1, void *arg2, void *arg3)
{
        while (1) {
                k_mutex_lock(&sync_mutex, K_FOREVER);

                k_condvar_wait(&yellow_cond, &sync_mutex, K_FOREVER);
                int delay = active_delay;
                k_mutex_unlock(&sync_mutex);

                led_yellow_set(1);
                k_msleep(delay);
                led_yellow_set(0);

                k_sem_give(&release_sem);
        }
}

static void execute_led_cmd(char color, int delay)
{
        k_mutex_lock(&sync_mutex, K_FOREVER);
        active_delay = delay;
        printk("%c, %d\n", color, delay);
        if (color == 'R' || color == 'r') {
                k_condvar_signal(&red_cond);
        } else if (color == 'G' || color == 'g') {
                k_condvar_signal(&green_cond);
        } else if (color == 'Y' || color == 'y') {
                k_condvar_signal(&yellow_cond);
        } else {
                k_mutex_unlock(&sync_mutex);
                return;
        }

        

        k_mutex_unlock(&sync_mutex);


        k_sem_take(&release_sem, K_FOREVER);
}

void sequence_thread(void *arg1, void *arg2, void *arg3)
{
    while (1) {
        k_sem_take(&seq_sem, K_FOREVER);
        //pyöritä sekvenssiä
        while (sequence_active && cmd_count > 0) {
            for (size_t i = 0; i < cmd_count; i++) {
                if (!sequence_active) {
                    break; 
                }
                execute_led_cmd(cmd_buffer[i].color, cmd_buffer[i].delay);
            }
        }
    }
}

void fifo_consumer_thread(void *arg1, void *arg2, void *arg3)
{
    while (1) {
        struct data_t *buf = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        if (buf != NULL) {
            char color = buf->msg[0];
            
            //parserointi
            char *comma = strchr(buf->msg, ',');
            int delay = (comma != NULL) ? atoi(comma + 1) : atoi(&buf->msg[2]);
            if (delay <= 0) {
                delay = 1000;
            }

            //Jos väri kirjai
            if (color == 'R' || color == 'r' || 
                color == 'G' || color == 'g' || 
                color == 'Y' || color == 'y') {

                // tallenna kometopuskurii
                k_mutex_lock(&sync_mutex, K_FOREVER);
                if (cmd_count < MAX_COMMANDS) {
                    cmd_buffer[cmd_count].color = color;
                    cmd_buffer[cmd_count].delay = delay;
                    cmd_count++;
                }
                k_mutex_unlock(&sync_mutex);

                //aja ledi 
                execute_led_cmd(color, delay);

                // Toisto
                } else if (color == 'T' || color == 't') {
                        if (!sequence_active && cmd_count > 0) {
                                sequence_active = true;
                                k_sem_give(&seq_sem);
                        }

                // Pysäytä skenvenssi
                } else if (color == 'C' || color == 'c') {
                        sequence_active = false;

                        k_mutex_lock(&sync_mutex, K_FOREVER);
                        cmd_count = 0;
                        k_mutex_unlock(&sync_mutex);
                }

                k_free(buf);
        }
    }
}
