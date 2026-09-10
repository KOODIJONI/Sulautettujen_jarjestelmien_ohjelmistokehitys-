#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <zephyr/kernel.h>

struct data_t {
    void *fifo_reserved;
    char msg[128];
};

extern struct k_fifo dispatcher_fifo;

int init_uart_dispatcher(void);

#endif