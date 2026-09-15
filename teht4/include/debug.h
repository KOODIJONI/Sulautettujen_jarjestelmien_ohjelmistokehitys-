#ifndef DEBUG_H_
#define DEBUG_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

enum timing_data_type {
    RED_TIMING_DATA = 0,
    YELLOW_TIMING_DATA,
    GREEN_TIMING_DATA,
    SERIAL_TIMING_DATA,
    SEQUENCE_TIMING_DATA,
    CMD_TIMING_DATA,
    TIMING_TASK_COUNT
};

struct timing_data {
    void *fifo_reserved;        
    enum timing_data_type type; 
    const char *name;
    int64_t start_time;         
    int64_t end_time;           
    int64_t duration;           
};

extern bool debug_enabled;
extern struct k_fifo debug_info;
extern struct timing_data times[TIMING_TASK_COUNT];

void timing_task_start(enum timing_data_type task_type, const char *task_name);
void timing_task_end(enum timing_data_type task_type);
void toggle_debug();
bool get_debug();
void debug_fifo_thread_entry(void *p1, void *p2, void *p3);

#endif 