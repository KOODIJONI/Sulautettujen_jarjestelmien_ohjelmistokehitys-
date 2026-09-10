#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

#define BUTTON1_NODE DT_ALIAS(sw0)
#define BUTTON2_NODE DT_ALIAS(sw1)
#define BUTTON3_NODE DT_ALIAS(sw2)
#define BUTTON4_NODE DT_ALIAS(sw3)
#define BUTTON5_NODE DT_ALIAS(sw4)

int buttons_init(void);        
bool button_was_pressed(int i);  

#endif