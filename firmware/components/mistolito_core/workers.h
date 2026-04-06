#ifndef WORKERS_H
#define WORKERS_H

#include "mistolito.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define COMBAT_RESULT_HIT 0
#define COMBAT_RESULT_MISS 1
#define COMBAT_RESULT_ENEMY_DEAD 2

void combat_worker_task(void *arg);
void search_worker_task(void *arg);
void rest_worker_task(void *arg);

extern TaskHandle_t g_combat_task_handle;
extern TaskHandle_t g_search_task_handle;
extern TaskHandle_t g_rest_task_handle;

void combat_worker_execute_turn(game_snapshot_t *snapshot);
uint8_t search_worker_generate_enemy(game_snapshot_t *snapshot);

#endif
