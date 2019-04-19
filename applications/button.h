#ifndef __BUTTON_H__
#define __BUTTON_H__
#include <rtthread.h> 
#include <rtdevice.h> 
#include "multi_button.h"
#include <board.h>

#define BTN_EVENT(n)    (0x01 << n)
#define BUTTON_PIN      48 //GET_PIN(A, 0)

extern int multi_button_init(void);
extern rt_event_t button_event_handler_get(void);

#endif
