#ifndef GAME_COORDINATOR_H
#define GAME_COORDINATOR_H

#include "mistolito.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

void game_coordinator_start(void);
game_snapshot_t* game_coordinator_get_snapshot(void);
SemaphoreHandle_t game_coordinator_get_mutex(void);
void game_coordinator_task(void *arg);

extern TaskHandle_t g_coordinator_task_handle;

#endif
