#include "game_coordinator.h"
#include "combat_engine.h"
#include "rest_engine.h"
#include "events.h"
#include "storage_task.h"
#include "display_task.h"
#include "rules.h"
#include "dna_engine.h"
#include "profession_engine.h"
#include "skills_perks_engine.h"
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

static combat_frame_result_t g_combat_result;
static bool g_combat_active = false;

static search_frame_result_t g_search_result;
static uint32_t g_search_start_ms = 0;
static uint32_t g_search_duration_ms = 0;

static rest_frame_result_t g_rest_result;

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

static void emit_level_up_event(uint8_t level)
{
    game_event_t evt = { .type = EVT_LEVEL_UP, .data.level_up.level = level };
    events_send(&evt);
}

static void emit_enemy_spawned_event(void)
{
    game_event_t evt = { .type = EVT_ENEMY_SPAWNED };
    events_send(&evt);
}

void game_coordinator_start(void)
{
    events_init();

    if (g_snapshot_mutex == NULL) {
        g_snapshot_mutex = xSemaphoreCreateMutex();
    }

    memset(&g_snapshot, 0, sizeof(g_snapshot));
    memset(&g_combat_result, 0, sizeof(g_combat_result));
    g_combat_active = false;

    strncpy(g_snapshot.pet.name, "Mistolito", PET_NAME_MAX_LEN);
    g_snapshot.pet.level = 1;
    g_snapshot.pet.profession = PROF_NONE;

    storage_load_dna_codes_only(&g_snapshot.pet.dna);

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

QueueHandle_t game_coordinator_get_turn_done_queue(void)
{
    return NULL;
}

combat_frame_result_t* game_coordinator_get_combat_result(void)
{
    return &g_combat_result;
}

void game_coordinator_clear_combat_result(void)
{
    g_combat_result.pet_damage_this_frame = 0;
    g_combat_result.enemy_damage_this_frame = 0;
    g_combat_result.pet_hit_this_frame = false;
    g_combat_result.enemy_hit_this_frame = false;
    g_combat_result.turns_this_frame = 0;
    g_combat_result.new_data = false;
}

search_frame_result_t* game_coordinator_get_search_result(void)
{
    return &g_search_result;
}

void game_coordinator_clear_search_result(void)
{
    g_search_result.search_ended = false;
    g_search_result.encounter_found = false;
    g_search_result.new_data = false;
}

rest_frame_result_t* game_coordinator_get_rest_result(void)
{
    return &g_rest_result;
}

void game_coordinator_clear_rest_result(void)
{
    g_rest_result.hp_recovered = 0;
    g_rest_result.energy_recovered = 0;
    g_rest_result.rest_ended = false;
    g_rest_result.new_data = false;
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

    int8_t con_mod = dna_get_modifier(g_snapshot.pet.con);
    if (con_mod > 0) {
        g_snapshot.pet.hp_max += con_mod;
    }

    profession_increment_level(&g_snapshot.pet);

    uint8_t current_stats[DNA_STAT_COUNT] = {
        g_snapshot.pet.str, g_snapshot.pet.dex, g_snapshot.pet.con,
        g_snapshot.pet.intel, g_snapshot.pet.wis, g_snapshot.pet.cha
    };

    levelup_queue_t queue = dna_get_levelup_candidates(&g_snapshot.pet.dna, current_stats, new_level);
    dna_apply_levelup(&g_snapshot.pet.dna, current_stats, &queue);

    g_snapshot.pet.str = current_stats[STAT_STR];
    g_snapshot.pet.dex = current_stats[STAT_DEX];
    g_snapshot.pet.con = current_stats[STAT_CON];
    g_snapshot.pet.intel = current_stats[STAT_INT];
    g_snapshot.pet.wis = current_stats[STAT_WIS];
    g_snapshot.pet.cha = current_stats[STAT_CHA];

    if (g_snapshot.pet.profession != PROF_NONE) {
        profession_apply_level_bonuses(&g_snapshot.pet, g_snapshot.pet.profession_level);

        uint8_t bonus_stats[STAT_COUNT];
        uint8_t bonus_count = 0;
        profession_get_bonus_stats(&g_snapshot.pet, bonus_stats, &bonus_count);

        for (uint8_t i = 0; i < bonus_count; i++) {
            uint8_t stat_idx = bonus_stats[i];
            if (profession_try_stat_increase(&g_snapshot.pet, stat_idx, true)) {
                switch (stat_idx) {
                    case STAT_STR: g_snapshot.pet.str++; break;
                    case STAT_DEX: g_snapshot.pet.dex++; break;
                    case STAT_CON: g_snapshot.pet.con++; break;
                    case STAT_INT: g_snapshot.pet.intel++; break;
                    case STAT_WIS: g_snapshot.pet.wis++; break;
                    case STAT_CHA: g_snapshot.pet.cha++; break;
                }
            }
        }
    }

    if (g_snapshot.pet.profession == PROF_NONE) {
        uint8_t available_profs[PROF_COUNT];
        uint8_t prof_count = 0;

        for (uint8_t prof_id = 1; prof_id < PROF_COUNT; prof_id++) {
            if (profession_check_requirements(&g_snapshot.pet, prof_id)) {
                available_profs[prof_count++] = prof_id;
            }
        }

        if (prof_count > 0) {
            uint8_t chosen_idx = (prof_count > 1) ? (esp_random() % prof_count) : 0;
            profession_try_change(&g_snapshot.pet, available_profs[chosen_idx]);
        }
    }

    if (g_snapshot.pet.profession != PROF_NONE) {
        skills_perks_process_level_up(&g_snapshot.pet, g_snapshot.pet.profession_level);
    }

    g_snapshot.pet.dirty_flags |= PET_DIRTY_LEVEL | PET_DIRTY_EXP | PET_DIRTY_HP | PET_DIRTY_STATS | PET_DIRTY_PROF_LEVEL | PET_DIRTY_SKILLS | PET_DIRTY_PERKS;

    emit_level_up_event(new_level);
    storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
    g_snapshot.pet.dirty_flags = 0;

    ESP_LOGI(TAG, "LEVEL UP! Now level %d", new_level);
}

static void simulate_combat_turn(void)
{
    if (!g_snapshot.pet.is_alive || combat_engine_all_enemies_dead(&g_snapshot.encounter)) {
        return;
    }

    uint8_t target_idx = combat_engine_select_first_alive(&g_snapshot.encounter);
    enemy_t *target = &g_snapshot.encounter.enemies[target_idx];

    int8_t dex_mod = rules_get_modifier(g_snapshot.pet.dex);
    int16_t attack_roll = (int16_t)rules_roll_d20() + dex_mod;

    g_combat_result.pet_hit_this_frame = false;
    g_combat_result.enemy_hit_this_frame = false;
    g_combat_result.pet_damage_this_frame = 0;
    g_combat_result.enemy_damage_this_frame = 0;

    if (attack_roll >= target->ac) {
        uint8_t base_damage = 0;
        uint8_t dice_count = 3 + g_snapshot.pet.bonuses.extra_dice;

        switch (g_snapshot.pet.profession) {
            case PROF_WARRIOR:
                base_damage = rules_roll_multiple(dice_count, 6);
                break;
            case PROF_MAGE:
                base_damage = rules_roll_multiple(1 + g_snapshot.pet.bonuses.extra_dice, 20);
                break;
            case PROF_ROGUE: {
                uint8_t crit_bonus = g_snapshot.pet.bonuses.crit;
                for (uint8_t j = 0; j < dice_count; j++) {
                    uint8_t d = rules_roll_dice(4);
                    if (rules_random_chance(15 + crit_bonus)) {
                        d *= 2;
                    }
                    base_damage += d;
                }
                if (g_snapshot.pet.bonuses.sneak_dice > 0) {
                    base_damage += rules_roll_multiple(g_snapshot.pet.bonuses.sneak_dice, 6);
                }
                break;
            }
            default:
                base_damage = rules_roll_multiple(g_snapshot.pet.combat.dice_count, g_snapshot.pet.combat.damage_dice) + g_snapshot.pet.combat.damage_bonus;
                break;
        }

        int8_t str_mod = rules_get_modifier(g_snapshot.pet.str);
        int16_t damage = base_damage + str_mod + g_snapshot.pet.bonuses.min_damage;
        if (damage < 1) damage = 1;

        target->hp -= damage;
        if (target->hp < 0) target->hp = 0;

        g_combat_result.pet_damage_this_frame = damage;
        g_combat_result.pet_hit_this_frame = true;

        if (target->hp <= 0) {
            target->alive = false;
        }
    }

    if (target->alive && g_snapshot.pet.is_alive) {
        int8_t enemy_mod = target->attack_bonus;
        int8_t pet_ac = g_snapshot.pet.combat.base_ac + rules_get_modifier(g_snapshot.pet.dex);
        int16_t enemy_attack_roll = (int16_t)rules_roll_d20() + enemy_mod;

        if (enemy_attack_roll >= pet_ac) {
            int16_t enemy_damage = rules_roll_dice(target->damage_dice) + target->damage_bonus;
            g_snapshot.pet.hp -= enemy_damage;
            if (g_snapshot.pet.hp < 0) g_snapshot.pet.hp = 0;

            g_combat_result.enemy_damage_this_frame = enemy_damage;
            g_combat_result.enemy_hit_this_frame = true;

            if (g_snapshot.pet.hp <= 0) {
                g_snapshot.pet.is_alive = false;
            }
        }
    }

    g_combat_result.turns_this_frame++;
    g_combat_result.new_data = true;
}

static void finish_combat(void)
{
    uint32_t total_exp = 0;
    uint8_t enemies_killed = 0;

    for (uint8_t i = 0; i < g_snapshot.encounter.count; i++) {
        if (!g_snapshot.encounter.enemies[i].alive) {
            total_exp += g_snapshot.encounter.enemies[i].exp_reward;
            enemies_killed++;
        }
    }

    g_snapshot.pet.exp += total_exp;
    g_snapshot.pet.enemies_killed += enemies_killed;

    for (uint8_t i = 0; i < enemies_killed; i++) {
        if (rules_random_chance(DP_GAIN_CHANCE)) {
            g_snapshot.pet.dp++;
            ESP_LOGI(TAG, "Gained 1 DP! (total: %lu)", (unsigned long)g_snapshot.pet.dp);
        }
    }

    g_snapshot.pet.dirty_flags |= PET_DIRTY_EXP | PET_DIRTY_DP;

    g_combat_result.combat_ended = true;
    g_combat_result.victory = combat_engine_all_enemies_dead(&g_snapshot.encounter);
    g_combat_result.new_data = true;

    ESP_LOGI(TAG, "Combat ended: victory=%d, exp=%lu", g_combat_result.victory, (unsigned long)total_exp);

    if (g_combat_result.victory) {
        if (g_snapshot.pet.exp >= g_snapshot.pet.exp_next) {
            transition_to(GS_LEVELUP);
        } else if (rest_should_rest(g_snapshot.pet.hp, g_snapshot.pet.hp_max, g_snapshot.pet.rest.hp_rest_threshold)) {
            rest_init(&g_snapshot.rest, &g_snapshot.pet);
            transition_to(GS_RESTING);
        } else {
            transition_to(GS_VICTORY);
        }
    } else {
        transition_to(GS_DEAD);
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
                        g_snapshot.pet.profession = PROF_NONE;
                        g_snapshot.pet.profession_level = 0;

                        storage_load_dna_codes_only(&g_snapshot.pet.dna);
                        g_snapshot.pet.dna.salt = esp_random();
                        storage_derive_dna_stats(&g_snapshot.pet.dna);

                        g_snapshot.pet.str = g_snapshot.pet.dna.base_stats[STAT_STR];
                        g_snapshot.pet.dex = g_snapshot.pet.dna.base_stats[STAT_DEX];
                        g_snapshot.pet.con = g_snapshot.pet.dna.base_stats[STAT_CON];
                        g_snapshot.pet.intel = g_snapshot.pet.dna.base_stats[STAT_INT];
                        g_snapshot.pet.wis = g_snapshot.pet.dna.base_stats[STAT_WIS];
                        g_snapshot.pet.cha = g_snapshot.pet.dna.base_stats[STAT_CHA];

                        storage_apply_profession_data(&g_snapshot.pet, PROF_NONE);
                        g_snapshot.pet.exp_next = calc_exp_for_level(1);
                        g_snapshot.pet.is_alive = true;
                        g_snapshot.pet.dirty_flags = 0xFF;
                        storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
                        g_snapshot.pet.dirty_flags = 0;

                        ESP_LOGI(TAG, "New pet created with salt 0x%08lX", (unsigned long)g_snapshot.pet.dna.salt);
                    }

        g_snapshot.pet.exp_next = calc_exp_for_level(g_snapshot.pet.level);

        g_search_start_ms = 0;
        g_search_duration_ms = 0;

        if (g_snapshot.pet.energy > 0) {
            transition_to(GS_SEARCHING);
        } else {
            rest_init(&g_snapshot.rest, &g_snapshot.pet);
            transition_to(GS_RESTING);
        }
    }
    break;

case GS_SEARCHING:
    if (g_search_start_ms == 0) {
        g_search_start_ms = (uint32_t)(esp_timer_get_time() / 1000);
        g_search_duration_ms = (esp_random() % 5 + 3) * 1000;
        memset(&g_search_result, 0, sizeof(g_search_result));
        ESP_LOGI(TAG, "Searching started, duration: %lu ms", (unsigned long)g_search_duration_ms);
    }

    uint32_t elapsed_ms = (uint32_t)(esp_timer_get_time() / 1000) - g_search_start_ms;
    g_search_result.new_data = true;

    if (elapsed_ms >= g_search_duration_ms) {
        combat_engine_start_encounter(&g_snapshot.encounter, g_snapshot.pet.level);

        g_snapshot.pet.energy--;
        g_snapshot.pet.dirty_flags |= PET_DIRTY_ENERGY;

        g_search_result.search_ended = true;
        g_search_result.encounter_found = true;
        g_search_result.new_data = true;

        ESP_LOGI(TAG, "Encounter spawned: %d enemy(s)", g_snapshot.encounter.count);

        memset(&g_combat_result, 0, sizeof(g_combat_result));
        g_combat_active = true;

        g_search_start_ms = 0;
        g_search_duration_ms = 0;

        emit_enemy_spawned_event();
        transition_to(GS_COMBAT);
    } else {
        taskYIELD();
    }
    break;

            case GS_COMBAT:
                if (!g_snapshot.pet.is_alive || combat_engine_all_enemies_dead(&g_snapshot.encounter)) {
                    if (g_combat_active) {
                        finish_combat();
                        g_combat_active = false;
                    }
                } else {
                    simulate_combat_turn();
                    taskYIELD();
                }
                break;

case GS_VICTORY:
    storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
    g_snapshot.pet.dirty_flags = 0;
    g_search_start_ms = 0;
    if (rest_should_rest(g_snapshot.pet.hp, g_snapshot.pet.hp_max, g_snapshot.pet.rest.hp_rest_threshold)) {
        rest_init(&g_snapshot.rest, &g_snapshot.pet);
        transition_to(GS_RESTING);
    } else if (g_snapshot.pet.energy > 0) {
        transition_to(GS_SEARCHING);
    } else {
        rest_init(&g_snapshot.rest, &g_snapshot.pet);
        transition_to(GS_RESTING);
    }
    break;

case GS_LEVELUP:
    process_level_up();
    g_search_start_ms = 0;
    if (rest_should_rest(g_snapshot.pet.hp, g_snapshot.pet.hp_max, g_snapshot.pet.rest.hp_rest_threshold)) {
        rest_init(&g_snapshot.rest, &g_snapshot.pet);
        transition_to(GS_RESTING);
    } else if (g_snapshot.pet.energy > 0) {
        transition_to(GS_SEARCHING);
    } else {
        rest_init(&g_snapshot.rest, &g_snapshot.pet);
        transition_to(GS_RESTING);
    }
    break;

case GS_RESTING:
{
    bool hp_recovering = rest_tick_hp(&g_snapshot.pet, &g_snapshot.rest);
    bool energy_recovering = rest_tick_energy(&g_snapshot.pet, &g_snapshot.rest);

    g_rest_result.hp_recovered = hp_recovering ? 1 : 0;
    g_rest_result.energy_recovered = energy_recovering ? 1 : 0;
    g_rest_result.new_data = true;

    g_snapshot.pet.dirty_flags |= PET_DIRTY_HP | PET_DIRTY_ENERGY;

    if (!hp_recovering && !energy_recovering) {
        storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
        g_snapshot.pet.dirty_flags = 0;

        g_rest_result.rest_ended = true;
        g_rest_result.new_data = true;

        transition_to(GS_SEARCHING);
    } else {
        taskYIELD();
    }
}
break;

case GS_DEAD:
    ESP_LOGI(TAG, "Pet died! Resetting...");
    if (g_snapshot_mutex && xSemaphoreTake(g_snapshot_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        combat_engine_reset_pet_on_death(&g_snapshot.pet);
        storage_apply_profession_data(&g_snapshot.pet, PROF_NONE);
        g_snapshot.pet.exp_next = calc_exp_for_level(1);
        g_snapshot.pet.dirty_flags = 0xFF;
        xSemaphoreGive(g_snapshot_mutex);
    }
    storage_save_pet_delta(g_snapshot.pet.dirty_flags, &g_snapshot.pet);
    g_snapshot.pet.dirty_flags = 0;
    g_search_start_ms = 0;
    transition_to(GS_SEARCHING);
    break;

            default:
                transition_to(GS_INIT);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
