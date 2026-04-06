#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "mistolito.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void display_task_start(void);
void display_task(void *arg);

extern TaskHandle_t g_display_task_handle;

#endif
