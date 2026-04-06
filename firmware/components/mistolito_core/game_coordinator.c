#include "game_coordinator.h"
#include "combat_engine.h"
#include "events.h"
#include "workers.h"
#include "storage_task.h"
#include "display_task.h"
#include "rules.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/semphr.h"
#include <string.h>
#include <math.h>

static const char *TAG = "COORD";

static game_snapshot_t g_snapshot;
static bool g_initialized = false;
static SemaphoreHandle_t g_snapshot_mutex = NULL;

TaskHandle_t g_coordinator_task_handle = NULL;

static float exp_base = 50.0f;
static float exp_linear = 50.0f;
static float exp_multiplier = 1.15f;

static uint32_t calc_exp_for_level(uint8_t level)
{
    float linear_part = exp_base + (exp_linear * (float)level);
    float exp_part = exp_base * (powf(exp_multiplier, (float)level) - 1.0f);
    return (uint32_t)(linear_part + exp_part);
}

static uint16_t calc_hp_bonus(uint8_t level)
{
    return 5 + 5 * ((level - 1) / 2);
}

static void transition_to(game_state_e new_state)
{
    g_snapshot.state = new_state;
    g_snapshot.state_entered_ms = (uint32_t)(esp_timer_get_time() / 1000);

    game_event_t evt = { .type = EVT_STATE_CHANGED, .data.new_state = new_state };
    events_send(&evt);

    ESP_LOGI(TAG, "State transition: %d", new_state);
}

static void emit_player_attack_event(int16_t damage, bool hit, uint8_t target_idx)
{
    game_event_t evt = {
        .type = EVT_PLAYER_ATTACK,
        .data.attack = { .damage = damage, .hit = hit, .target_idx = target_idx }
    };
    events_send(&evt);
}

static void emit_enemy_attack_event(uint8_t enemy_idx, int16_t damage, bool hit)
{
    game_event_t evt = {
        .type = EVT_ENEMY_ATTACK,
        .data.enemy_attack = { .enemy_idx = enemy_idx, .damage = damage, .hit = hit }
    };
    events_send(&evt);
}

static void emit_victory_event(uint32_t exp)
{
    game_event_t evt = { .type = EVT_VICTORY, .data.exp.amount = exp };
    events_send(&evt);
}

static void emit_level_up_event(uint8_t level)
{
    game_event_t evt = { .type = EVT_LEVEL_UP, .data.level_up.level = level };
    events_send(&evt);
}

static void emit_energy_event(uint8_t current)
{
    game_event_t evt = { .type = EVT_ENERGY_CHANGED, .data.energy.current = current, .data.energy.max = MAX_ENERGY };
    events_send(&evt);
}

static void emit_enemy_spawned_event(void)
{
    game_event_t evt = { .type = EVT_ENEMY_SPAWNED };
    events_send(&evt);
}

static void resume_and_notify(TaskHandle_t task)
{
    if (task != NULL) {
        vTaskResume(task);
        xTaskNotify(task, 0, eNoAction);
    }
}

void game_coordinator_start(void)
{
    events_init();

    if (g_snapshot_mutex == NULL) {
        g_snapshot_mutex = xSemaphoreCreateMutex();
    }

    memset(&g_snapshot, 0, sizeof(g_snapshot));

    strncpy(g_snapshot.pet.name, "Mistolito", PET_NAME_MAX_LEN);
    g_snapshot.pet.level = 1;
    g_snapshot.pet.hp_max = BASE_PET_HP;
    g_snapshot.pet.hp = BASE_PET_HP;
    g_snapshot.pet.energy = MAX_ENERGY;
    g_snapshot.pet.energy_max = MAX_ENERGY;
    g_snapshot.pet.exp_next = calc_exp_for_level(1);
    g_snapshot.pet.str = 10;
    g_snapshot.pet.dex = 10;
    g_snapshot.pet.con = 10;
    g_snapshot.pet.intel = 10;
    g_snapshot.pet.wis = 10;
    g_snapshot.pet.cha = 10;
    g_snapshot.pet.is_alive = true;

    combat_engine_init(&g_snapshot.combat);

    g_snapshot.state = GS_INIT;
    g_snapshot.state_entered_ms = (uint32_t)(esp_timer_get_time() / 1000);
    g_initialized = true;

    ESP_LOGI(TAG, "Coordinator started (INIT mode)");
}

game_snapshot_t* game_coordinator_get_snapshot(void)
{
    return &g_snapshot;
}

SemaphoreHandle_t game_coordinator_get_mutex(void)
{
    return g_snapshot_mutex;
}

static void process_level_up(void)
{
    uint8_t new_level = g_snapshot.pet.level + 1;
    g_snapshot.pet.level = new_level;
    g_snapshot.pet.exp = 0;
    g_snapshot.pet.exp_next = calc_exp_for_level(new_level);

    uint16_t hp_bonus = calc_hp_bonus(new_level);
    g_snapshot.pet.hp_max += hp_bonus;
    g_snapshot.pet.hp = g_snapshot.pet.hp_max;
    g_snapshot.pet.energy = g_snapshot.pet.energy_max;

    int8_t con_mod = rules_get_modifier(g_snapshot.pet.con);
    if (con_mod > 0) {
        g_snapshot.pet.hp_max += con_mod;
    }

    if (rules_roll_stat_increase(false, false)) {
        uint8_t stat_idx = (uint8_t)(esp_random() % STAT_COUNT);
        switch (stat_idx) {
            case STAT_STR: g_snapshot.pet.str++; break;
            case STAT_DEX: g_snapshot.pet.dex++; break;
            case STAT_CON: g_snapshot.pet.con++; break;
            case STAT_INT: g_snapshot.pet.intel++; break;
            case STAT_WIS: g_snapshot.pet.wis++; break;
            case STAT_CHA: g_snapshot.pet.cha++; break;
        }
    }

    g_snapshot.pet.dirty_flags |= PET_DIRTY_LEVEL | PET_DIRTY_EXP | PET_DIRTY_HP | PET_DIRTY_STATS;

    emit_level_up_event(new_level);
    storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
    g_snapshot.pet.dirty_flags = 0;

    ESP_LOGI(TAG, "LEVEL UP! Now level %d, HP: %d", new_level, g_snapshot.pet.hp_max);
}

static void handle_combat_state(void)
{
    switch (g_snapshot.combat.phase) {
        case CS_INIT:
            combat_engine_init(&g_snapshot.combat);
            g_snapshot.combat.round = 1;
            g_snapshot.combat.phase = CS_PLAYER_SELECT_ACTION;
            break;

        case CS_PLAYER_SELECT_ACTION:
            g_snapshot.combat.selected_action = ACTION_ATTACK;
            g_snapshot.combat.selected_target_idx = combat_engine_select_first_alive(&g_snapshot.encounter);
            g_snapshot.combat.phase = CS_PLAYER_SELECT_TARGET;
            break;

        case CS_PLAYER_SELECT_TARGET:
            g_snapshot.combat.selected_target_idx = combat_engine_select_first_alive(&g_snapshot.encounter);
            g_snapshot.combat.phase = CS_PLAYER_EXECUTE;
            break;

        case CS_PLAYER_EXECUTE:
            combat_engine_player_attack(
                &g_snapshot.pet,
                &g_snapshot.encounter.enemies[g_snapshot.combat.selected_target_idx],
                &g_snapshot.combat
            );
            emit_player_attack_event(
                g_snapshot.combat.last_player_damage,
                g_snapshot.combat.player_hit,
                g_snapshot.combat.selected_target_idx
            );

            if (combat_engine_all_enemies_dead(&g_snapshot.encounter)) {
                g_snapshot.combat.phase = CS_CHECK_VICTORY;
            } else {
                g_snapshot.combat.phase = CS_ENEMY_SELECT_ACTION;
            }
            break;

        case CS_ENEMY_SELECT_ACTION:
            g_snapshot.combat.current_enemy_idx = 0;
            g_snapshot.combat.phase = CS_ENEMY_EXECUTE;
            break;

        case CS_ENEMY_EXECUTE:
            for (uint8_t i = 0; i < g_snapshot.encounter.count; i++) {
                if (g_snapshot.encounter.enemies[i].alive) {
                    combat_engine_enemy_attack(
                        &g_snapshot.encounter.enemies[i],
                        &g_snapshot.pet,
                        &g_snapshot.combat
                    );
                    emit_enemy_attack_event(i, g_snapshot.combat.last_enemy_damage, g_snapshot.combat.enemy_hit);

                    if (!g_snapshot.pet.is_alive) {
                        g_snapshot.combat.phase = CS_CHECK_VICTORY;
                        return;
                    }
                }
            }
            g_snapshot.combat.phase = CS_ROUND_END;
            break;

        case CS_ROUND_END:
            combat_engine_apply_round_effects(&g_snapshot.pet, &g_snapshot.encounter);
            g_snapshot.combat.round++;
            g_snapshot.combat.phase = CS_CHECK_VICTORY;
            break;

        case CS_CHECK_VICTORY:
            if (combat_engine_all_enemies_dead(&g_snapshot.encounter)) {
                uint32_t total_exp = 0;
                for (uint8_t i = 0; i < g_snapshot.encounter.count; i++) {
                    total_exp += g_snapshot.encounter.enemies[i].exp_reward;
                }
                g_snapshot.pet.exp += total_exp;
                g_snapshot.pet.enemies_killed += g_snapshot.encounter.count;
                g_snapshot.pet.dirty_flags |= PET_DIRTY_EXP;
                emit_victory_event(total_exp);

                if (g_snapshot.pet.exp >= g_snapshot.pet.exp_next) {
                    transition_to(GS_LEVELUP);
                } else {
                    transition_to(GS_VICTORY);
                }
            } else if (!g_snapshot.pet.is_alive) {
                transition_to(GS_DEAD);
            } else {
                g_snapshot.combat.phase = CS_PLAYER_SELECT_ACTION;
            }
            break;
    }
}

void game_coordinator_task(void *arg)
{
    ESP_LOGI(TAG, "Coordinator task started");

    while (!g_initialized) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    while (1) {
        switch (g_snapshot.state) {
            case GS_INIT:
                if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100))) {
                    if (!storage_load_pet(&g_snapshot.pet)) {
                        strncpy(g_snapshot.pet.name, "Mistolito", PET_NAME_MAX_LEN);
                        g_snapshot.pet.level = 1;
                        g_snapshot.pet.hp_max = BASE_PET_HP;
                        g_snapshot.pet.hp = BASE_PET_HP;
                        g_snapshot.pet.energy = MAX_ENERGY;
                        g_snapshot.pet.energy_max = MAX_ENERGY;
                        g_snapshot.pet.exp_next = calc_exp_for_level(1);
                        g_snapshot.pet.str = 10;
                        g_snapshot.pet.dex = 10;
                        g_snapshot.pet.con = 10;
                        g_snapshot.pet.intel = 10;
                        g_snapshot.pet.wis = 10;
                        g_snapshot.pet.cha = 10;
                        g_snapshot.pet.dirty_flags = 0xFF;
                        storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
                    }

                    g_snapshot.pet.exp_next = calc_exp_for_level(g_snapshot.pet.level);

                    if (g_snapshot.pet.energy > 0) {
                        resume_and_notify(g_search_task_handle);
                        transition_to(GS_SEARCHING);
                    } else {
                        resume_and_notify(g_rest_task_handle);
                        transition_to(GS_RESTING);
                    }
                }
                break;

            case GS_SEARCHING:
                if (g_search_task_handle != NULL) {
                    vTaskResume(g_search_task_handle);
                    xTaskNotify(g_search_task_handle, 0, eNoAction);

                    uint32_t result = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                    if (result > 0) {
                        emit_enemy_spawned_event();
                        transition_to(GS_COMBAT);
                    }
                }
                break;

            case GS_COMBAT:
                handle_combat_state();
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case GS_VICTORY:
                storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
                g_snapshot.pet.dirty_flags = 0;
                vTaskDelay(pdMS_TO_TICKS(2000));
                if (g_snapshot.pet.energy > 0) {
                    resume_and_notify(g_search_task_handle);
                    transition_to(GS_SEARCHING);
                } else {
                    resume_and_notify(g_rest_task_handle);
                    transition_to(GS_RESTING);
                }
                break;

            case GS_LEVELUP:
                process_level_up();
                vTaskDelay(pdMS_TO_TICKS(2000));
                if (g_snapshot.pet.energy > 0) {
                    resume_and_notify(g_search_task_handle);
                    transition_to(GS_SEARCHING);
                } else {
                    resume_and_notify(g_rest_task_handle);
                    transition_to(GS_RESTING);
                }
                break;

            case GS_RESTING:
                if (g_rest_task_handle != NULL) {
                    vTaskResume(g_rest_task_handle);
                    xTaskNotify(g_rest_task_handle, 0, eNoAction);

                    uint32_t result = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
                    if (result > 0) {
                        g_snapshot.pet.energy++;
                        g_snapshot.pet.dirty_flags |= PET_DIRTY_ENERGY;
                        emit_energy_event(g_snapshot.pet.energy);

                        if (g_snapshot.pet.energy >= MAX_ENERGY) {
                            vTaskSuspend(g_rest_task_handle);
                            storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
                            g_snapshot.pet.dirty_flags = 0;
                            resume_and_notify(g_search_task_handle);
                            transition_to(GS_SEARCHING);
                        }
                    }
                }
                break;

            case GS_DEAD:
                ESP_LOGI(TAG, "Pet died! Resetting...");
                if (g_snapshot_mutex && xSemaphoreTake(g_snapshot_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    combat_engine_reset_pet_on_death(&g_snapshot.pet);
                    g_snapshot.pet.exp_next = calc_exp_for_level(1);
                    g_snapshot.pet.dirty_flags = 0xFF;
                    xSemaphoreGive(g_snapshot_mutex);
                }
                storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
                g_snapshot.pet.dirty_flags = 0;
                vTaskDelay(pdMS_TO_TICKS(2000));
                resume_and_notify(g_search_task_handle);
                transition_to(GS_SEARCHING);
                break;

            default:
                transition_to(GS_INIT);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
