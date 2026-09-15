#include "debug.h"
#include <zephyr/logging/log.h>
#include <assert.h>
LOG_MODULE_REGISTER(debug_timing, LOG_LEVEL_INF);

bool debug_enabled = true;
K_FIFO_DEFINE(debug_info);

struct timing_data times[TIMING_TASK_COUNT];

void timing_task_start(enum timing_data_type task_type, const char *task_name)
{
    if (task_type >= TIMING_TASK_COUNT) {
        return;
    }

    assert(task_type <=TIMING_TASK_COUNT);
    
    times[task_type].type = task_type;
    times[task_type].name = task_name;
    times[task_type].start_time = k_cycle_get_32();
}
void toggle_debug(){
    debug_enabled=!debug_enabled;
    if(debug_enabled) {
            printk("Debug on\n");
    }
    else{
            printk("Debug off\n");

    }
    
}
bool get_debug(){
    return debug_enabled;
}
void timing_task_end(enum timing_data_type task_type)
{
    if (task_type >= TIMING_TASK_COUNT) {
        return;
    }

    assert(task_type <=TIMING_TASK_COUNT);

    struct timing_data *data = &times[task_type];
    data->end_time  = k_cycle_get_32();
    data->duration =  k_cyc_to_us_floor32(data->end_time - data->start_time);

    if (debug_enabled) {
        k_fifo_put(&debug_info, data);
    }
}


void debug_fifo_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    printk("Debug timing FIFO processing thread started\n");

    while (1) {
        struct timing_data *item = k_fifo_get(&debug_info, K_FOREVER);

        if (item != NULL) {
            printk("Task [%s | ID:%d] executed in %lld us (Start: %lld cycle, End: %lld cycle)\n",
                item->name != NULL ? item->name : "UNKNOWN",
                item->type,
                item->duration,
                item->start_time,
                item->end_time);
        }
    }
}

K_THREAD_DEFINE(debug_fifo_tid, 1024,
                debug_fifo_thread_entry, NULL, NULL, NULL,
                5, 0, 0);