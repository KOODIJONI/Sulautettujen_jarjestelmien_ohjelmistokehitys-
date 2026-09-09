#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>
#include "dispatcher.h"

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

K_FIFO_DEFINE(dispatcher_fifo);

int init_uart_dispatcher(void) {
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return -ENODEV;
    }
    return 0;
}

static void uart_task(void *u1, void *u2, void *u3)
{
    char rc = 0;
    char uart_msg[20];
    memset(uart_msg, 0, sizeof(uart_msg));
    int uart_msg_cnt = 0;

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc != '\r') {
                if (uart_msg_cnt < sizeof(uart_msg) - 1) {
                    uart_msg[uart_msg_cnt++] = rc;
                }
            } else {
                struct data_t *buf = k_malloc(sizeof(struct data_t));
                if (buf != NULL) {
                    snprintf(buf->msg, sizeof(buf->msg), "%s", uart_msg);
                    
                    k_fifo_put(&dispatcher_fifo, buf);
                }

                uart_msg_cnt = 0;
                memset(uart_msg, 0, sizeof(uart_msg));
            }
        }
        k_msleep(10);
    }
}

K_THREAD_DEFINE(uart_thread, 1024, uart_task, NULL, NULL, NULL, 1, 0, 0);
//fifo

