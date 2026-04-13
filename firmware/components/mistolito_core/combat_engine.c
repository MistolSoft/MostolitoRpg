#include "combat_engine.h"
#include "rules.h"
#include "storage_task.h"
#include "dna_engine.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "COMBAT";

void combat_engine_init(combat_state_t *combat)
{
    memset(combat, 0, sizeof(combat_state_t));
    combat->phase = CS_INIT;
}

void combat_engine_start_encounter(encounter_t *encounter, uint8_t pet_level)
{
    memset(encounter, 0, sizeof(encounter_t));

    encounter->count = 1;
    if (pet_level >= 40) encounter->count = 1 + ((pet_level - 35) / 5);
    if (encounter->count > MAX_ENEMIES_PER_ENCOUNTER) {
        encounter->count = MAX_ENEMIES_PER_ENCOUNTER;
    }

    for (uint8_t i = 0; i < encounter->count; i++) {
        enemy_t *enemy = &encounter->enemies[i];
        
        uint8_t enemy_id = storage_get_random_enemy_id(pet_level);
        
        if (!storage_get_enemy_data(enemy_id, pet_level, enemy)) {
            enemy->hp_max = 20 + pet_level * 10;
            enemy->hp = enemy->hp_max;
            enemy->ac = 8 + (pet_level / 5);
            enemy->damage_dice = 4;
            enemy->damage_bonus = 0;
            enemy->attack_bonus = 2;
            enemy->exp_reward = 15 + pet_level * 5;
            strncpy(enemy->name, "Unknown", ENEMY_NAME_MAX_LEN - 1);
            enemy->level = pet_level;
            enemy->alive = true;
        }
    }

    encounter->active_idx = 0;
}

uint16_t combat_engine_calc_enemy_hp(uint8_t pet_level)
{
    uint16_t base_hp = 30;
    uint16_t hp_per_level = 15;
    return base_hp + (pet_level * hp_per_level);
}

uint8_t combat_engine_calc_enemy_ac(uint8_t pet_level)
{
    uint8_t base_ac = 6;
    uint8_t ac_per_level = 1;
    uint8_t ac = base_ac + (pet_level / 2) * ac_per_level;
    if (ac > 15) ac = 15;
    return ac;
}

void combat_engine_player_attack(pet_t *pet, enemy_t *target, combat_state_t *combat)
{
    int8_t dex_mod = rules_get_modifier(pet->dex);
    int16_t attack_roll = (int16_t)rules_roll_d20() + dex_mod;

    if (attack_roll >= target->ac) {
        uint8_t base_damage = 0;
        uint8_t dice_count = 3 + pet->bonuses.extra_dice;

        switch (pet->profession) {
        case PROF_WARRIOR:
            base_damage = rules_roll_multiple(dice_count, 6);
            break;
        case PROF_MAGE:
            base_damage = rules_roll_multiple(1 + pet->bonuses.extra_dice, 20);
            break;
        case PROF_ROGUE: {
            uint8_t crit_bonus = pet->bonuses.crit;
            for (uint8_t i = 0; i < dice_count; i++) {
                uint8_t d = rules_roll_dice(4);
                if (rules_random_chance(15 + crit_bonus)) {
                    d *= 2;
                }
                base_damage += d;
            }
            if (pet->bonuses.sneak_dice > 0) {
                base_damage += rules_roll_multiple(pet->bonuses.sneak_dice, 6);
            }
            break;
        }
        default:
            base_damage = rules_roll_multiple(pet->combat.dice_count, pet->combat.damage_dice) + pet->combat.damage_bonus;
            break;
        }

        int8_t str_mod = rules_get_modifier(pet->str);
        int16_t damage = base_damage + str_mod + pet->bonuses.min_damage;
        if (damage < 1) damage = 1;

        target->hp -= damage;
        if (target->hp < 0) target->hp = 0;

        combat->last_player_damage = damage;
        combat->player_hit = true;
        combat->turn_count++;

        if (target->hp <= 0) {
            target->alive = false;
        }
    } else {
        combat->last_player_damage = 0;
        combat->player_hit = false;
    }
}

void combat_engine_enemy_attack(enemy_t *enemy, pet_t *pet, combat_state_t *combat)
{
    int8_t enemy_mod = enemy->attack_bonus;
    int8_t pet_ac = pet->combat.base_ac + rules_get_modifier(pet->dex);
    int16_t attack_roll = (int16_t)rules_roll_d20() + enemy_mod;

    if (attack_roll >= pet_ac) {
        uint8_t damage = rules_roll_dice(enemy->damage_dice) + enemy->damage_bonus;
        pet->hp -= damage;
        if (pet->hp < 0) pet->hp = 0;

        combat->last_enemy_damage = damage;
        combat->enemy_hit = true;

        if (pet->hp <= 0) {
            pet->is_alive = false;
        }
    } else {
        combat->last_enemy_damage = 0;
        combat->enemy_hit = false;
    }
}

void combat_engine_log_round(pet_t *pet, enemy_t *enemy, combat_state_t *combat)
{
    if (combat->player_hit) {
        if (enemy->alive) {
            ESP_LOGI(TAG, "[ROUND %d] Pet hit %s for %d dmg (%d/%d HP) | %s hit Pet for %d dmg (%d/%d HP)",
                     combat->turn_count, enemy->name, combat->last_player_damage,
                     enemy->hp, enemy->hp_max,
                     enemy->name, combat->last_enemy_damage,
                     pet->hp, pet->hp_max);
        } else {
            ESP_LOGI(TAG, "[ROUND %d] Pet killed %s with %d dmg! Pet HP: %d/%d",
                     combat->turn_count, enemy->name, combat->last_player_damage,
                     pet->hp, pet->hp_max);
        }
    } else {
        if (combat->enemy_hit) {
            ESP_LOGI(TAG, "[ROUND %d] Pet missed | %s hit Pet for %d dmg (%d/%d HP)",
                     combat->turn_count, enemy->name, combat->last_enemy_damage,
                     pet->hp, pet->hp_max);
        } else {
            ESP_LOGI(TAG, "[ROUND %d] Both missed", combat->turn_count);
        }
    }
}

bool combat_engine_all_enemies_dead(encounter_t *encounter)
{
    for (uint8_t i = 0; i < encounter->count; i++) {
        if (encounter->enemies[i].alive) {
            return false;
        }
    }
    return true;
}



void combat_engine_apply_round_effects(pet_t *pet, encounter_t *encounter)
{
    (void)pet;
    (void)encounter;
}

uint8_t combat_engine_select_target_ai(pet_t *pet, encounter_t *encounter)
{
    (void)pet;
    
    for (uint8_t i = 0; i < encounter->count; i++) {
        if (encounter->enemies[i].alive) {
            return i;
        }
    }
    return 0;
}

uint8_t combat_engine_select_first_alive(encounter_t *encounter)
{
    for (uint8_t i = 0; i < encounter->count; i++) {
        if (encounter->enemies[i].alive) {
            return i;
        }
    }
    return 0;
}

void combat_engine_reset_pet_on_death(pet_t *pet)
{
    char saved_name[PET_NAME_MAX_LEN];
    memcpy(saved_name, pet->name, PET_NAME_MAX_LEN - 1);
    saved_name[PET_NAME_MAX_LEN - 1] = '\0';

    uint32_t saved_dp = pet->dp;

    dna_t saved_dna;
    memcpy(&saved_dna, &pet->dna, sizeof(dna_t));

    memset(pet, 0, sizeof(pet_t));

    memcpy(pet->name, saved_name, PET_NAME_MAX_LEN - 1);
    pet->name[PET_NAME_MAX_LEN - 1] = '\0';

    pet->dp = saved_dp;

    memcpy(&pet->dna, &saved_dna, sizeof(dna_t));

    pet->dna.salt = esp_random();

    dna_generate_hash(&pet->dna);

    uint8_t stats[DNA_STAT_COUNT];
    dna_derive_all_stats(&pet->dna, stats);

    pet->str = stats[STAT_STR];
    pet->dex = stats[STAT_DEX];
    pet->con = stats[STAT_CON];
    pet->intel = stats[STAT_INT];
    pet->wis = stats[STAT_WIS];
    pet->cha = stats[STAT_CHA];

    pet->level = 1;
    pet->hp_max = BASE_PET_HP;
    pet->hp = pet->hp_max;
    pet->energy = MAX_ENERGY;
    pet->energy_max = MAX_ENERGY;
    pet->profession = PROF_NONE;
    pet->is_alive = true;

    ESP_LOGI(TAG, "Pet resurrected: %s (DP=%lu) STR=%d DEX=%d CON=%d INT=%d WIS=%d CHA=%d",
        pet->name, (unsigned long)pet->dp, pet->str, pet->dex, pet->con, pet->intel, pet->wis, pet->cha);
}
