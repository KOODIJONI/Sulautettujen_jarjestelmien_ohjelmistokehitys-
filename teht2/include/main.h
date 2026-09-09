#ifndef MAIN_H
#define MAIN_H

void red_led_thread(void *arg1, void *arg2, void *arg3);
void green_led_thread(void *arg1, void *arg2, void *arg3);
void yellow_led_thread(void *arg1, void *arg2, void *arg3);
void yellow_blink_thread(void *arg1, void *arg2, void *arg3);
void pause_led_thread(void *arg1, void *arg2, void *arg3);

enum led_states {
    RED = 0,
    GREEN = 1,
    YELLOW = 2,
    PAUSE = 4,
    RED_ON = 5,
    RED_GREEN_ON = 6,
    GREEN_ON = 7,
    YELLOW_BLINK = 8
};

#endif /* MAIN_H */