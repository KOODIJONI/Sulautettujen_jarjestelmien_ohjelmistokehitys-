#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "dispatcher.h"

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

K_FIFO_DEFINE(dispatcher_fifo);


/*
 * Alustetaan UART.
 */
int init_uart_dispatcher(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return -ENODEV;
    }

    return 0;
}


/*
 * UART receiver task.
 *
 * Vastaanottaa esimerkiksi:
 *
 * R,1000\r
 * Y,500\r
 * G,1000\r
 *
 * ja laittaa jokaisen komennon dispatcher_fifoon
 * omana data_t-rakenteenaan.
 *
 * Myös pelkkä:
 *
 * RYGYRYG\r
 *
 * toimii.
 */
static void uart_task(void *u1, void *u2, void *u3)
{
    unsigned char rc;

    char seq_buf[128];
    size_t seq_len = 0;

    if (init_uart_dispatcher() != 0) {
        return;
    }

    printk("UART receiver started\n");

    while (true) {

        /*
         * Luetaan kaikki tällä hetkellä saatavilla olevat merkit.
         */
        while (uart_poll_in(uart_dev, &rc) == 0) {

            /*
             * CR ja LF tarkoittavat komennon loppua.
             */
            if (rc == '\r' || rc == '\n') {

                /*
                 * Jos puskuri sisältää komennon,
                 * tehdään siitä FIFO-alkio.
                 */
                if (seq_len > 0) {

                    seq_buf[seq_len] = '\0';

                    struct data_t *buf =
                        k_malloc(sizeof(struct data_t));

                    if (buf == NULL) {
                        seq_len = 0;
                        continue;
                    }

                    /*
                     * Kopioidaan komento FIFO-dataan.
                     */
                    snprintf(buf->msg,
                             sizeof(buf->msg),
                             "%s",
                             seq_buf);


                    k_fifo_put(&dispatcher_fifo, buf);

                    /*
                     * Aloitetaan seuraavan komennon vastaanotto.
                     */
                    seq_len = 0;
                    memset(seq_buf, 0, sizeof(seq_buf));
                }

                continue;
            }


            /*
             * Normaali merkki.
             */
            if (seq_len < sizeof(seq_buf) - 1) {

                seq_buf[seq_len++] = (char)rc;

            } else {

                /*
                 * Puskuri täynnä.
                 */

                seq_len = 0;
                memset(seq_buf, 0, sizeof(seq_buf));
            }
        }

        /*
         * Annetaan muille taskeille CPU-aikaa.
         */
        k_msleep(1);
    }
}


K_THREAD_DEFINE(
    uart_thread,
    1024,
    uart_task,
    NULL,
    NULL,
    NULL,
    5,
    0,
    0
);
