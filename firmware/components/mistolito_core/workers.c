#include "workers.h"
#include "game_coordinator.h"
#include "combat_engine.h"
#include "rules.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "WORKERS";

TaskHandle_t g_combat_task_handle = NULL;
TaskHandle_t g_search_task_handle = NULL;
TaskHandle_t g_rest_task_handle = NULL;

void combat_worker_task(void *arg)
{
    ESP_LOGI(TAG, "Combat worker task started (deprecated - logic moved to combat_engine)");
    vTaskSuspend(NULL);
}

void search_worker_task(void *arg)
{
    ESP_LOGI(TAG, "Search worker task started");

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint32_t search_time = (esp_random() % 5 + 3) * 1000;
        vTaskDelay(pdMS_TO_TICKS(search_time));

        game_snapshot_t *snap = game_coordinator_get_snapshot();

        combat_engine_start_encounter(&snap->encounter, snap->pet.level);

        snap->pet.energy--;
        snap->pet.dirty_flags |= PET_DIRTY_ENERGY;

        ESP_LOGI(TAG, "Encounter spawned: %d enemy(s)", snap->encounter.count);
        for (uint8_t i = 0; i < snap->encounter.count; i++) {
            ESP_LOGI(TAG, "  [%d] %s (HP:%d, AC:%d, EXP:%d)",
                i, snap->encounter.enemies[i].name,
                snap->encounter.enemies[i].hp_max,
                snap->encounter.enemies[i].ac,
                snap->encounter.enemies[i].exp_reward);
        }

        xTaskNotify(g_coordinator_task_handle, 1, eSetValueWithOverwrite);

        vTaskSuspend(NULL);
    }
}

void rest_worker_task(void *arg)
{
    ESP_LOGI(TAG, "Rest worker task started");

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(1000));

        xTaskNotify(g_coordinator_task_handle, 1, eSetValueWithOverwrite);
    }
}

void combat_worker_execute_turn(game_snapshot_t *snapshot)
{
    (void)snapshot;
}

uint8_t search_worker_generate_enemy(game_snapshot_t *snapshot)
{
    (void)snapshot;
    return 0;
}
