#include "combat_engine.h"
#include "rules.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "COMBAT";

static const char* ENEMY_NAMES[] = {
    "Slime", "Goblin", "Skeleton", "Bat", "Spider",
    "Wolf", "Orc", "Zombie", "Harpy", "Troll",
    "Golem", "Demon", "Dragon", "Wraith", "Basilisk"
};
#define ENEMY_COUNT (sizeof(ENEMY_NAMES) / sizeof(ENEMY_NAMES[0]))

void combat_engine_init(combat_state_t *combat)
{
    memset(combat, 0, sizeof(combat_state_t));
    combat->phase = CS_INIT;
}

void combat_engine_start_encounter(encounter_t *encounter, uint8_t pet_level)
{
    memset(encounter, 0, sizeof(encounter_t));

    encounter->count = 1;
    if (pet_level >= 5) encounter->count = 1 + (pet_level / 5);
    if (encounter->count > MAX_ENEMIES_PER_ENCOUNTER) {
        encounter->count = MAX_ENEMIES_PER_ENCOUNTER;
    }

    for (uint8_t i = 0; i < encounter->count; i++) {
        enemy_t *enemy = &encounter->enemies[i];

        uint8_t enemy_idx = (uint8_t)(esp_random() % ENEMY_COUNT);
        strncpy(enemy->name, ENEMY_NAMES[enemy_idx], ENEMY_NAME_MAX_LEN - 1);
        enemy->name[ENEMY_NAME_MAX_LEN - 1] = '\0';

        enemy->hp_max = combat_engine_calc_enemy_hp(pet_level);
        enemy->hp = enemy->hp_max;
        enemy->ac = combat_engine_calc_enemy_ac(pet_level);
        enemy->attack_bonus = 0;
        enemy->damage_dice = 4;
        enemy->damage_bonus = 1;
        enemy->level = pet_level;
        enemy->alive = true;

        uint16_t base_exp = (uint16_t)(enemy->hp_max / 4);
        enemy->exp_reward = base_exp + (uint8_t)(esp_random() % 20) + 1;
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
    uint8_t base_ac = 8;
    uint8_t ac_per_level = 1;
    uint8_t ac = base_ac + (pet_level / 2) * ac_per_level;
    if (ac > 15) ac = 15;
    return ac;
}

void combat_engine_player_attack(pet_t *pet, enemy_t *target, combat_state_t *combat)
{
    int8_t dex_mod = rules_get_modifier(pet->dex);
    int16_t attack_roll = (int16_t)rules_roll_d20() + dex_mod;

    ESP_LOGI(TAG, "Player attack: roll=%d (d20+%d) vs AC=%d", attack_roll, dex_mod, target->ac);

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
            base_damage = rules_roll_dice(15) + 1;
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

        ESP_LOGI(TAG, "HIT! Damage=%d, Enemy HP: %d/%d", damage, target->hp, target->hp_max);

        if (target->hp <= 0) {
            target->alive = false;
            ESP_LOGI(TAG, "Enemy died!");
        }
    } else {
        combat->last_player_damage = 0;
        combat->player_hit = false;
        ESP_LOGI(TAG, "MISS!");
    }
}

void combat_engine_enemy_attack(enemy_t *enemy, pet_t *pet, combat_state_t *combat)
{
    int8_t enemy_mod = rules_get_modifier(enemy->ac);
    int8_t pet_ac = 10 + rules_get_modifier(pet->dex);
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

    memset(pet, 0, sizeof(pet_t));

    memcpy(pet->name, saved_name, PET_NAME_MAX_LEN - 1);
    pet->name[PET_NAME_MAX_LEN - 1] = '\0';
    pet->level = 1;
    pet->hp_max = BASE_PET_HP;
    pet->hp = BASE_PET_HP;
    pet->energy = MAX_ENERGY;
    pet->energy_max = MAX_ENERGY;
    pet->str = 10;
    pet->dex = 10;
    pet->con = 10;
    pet->intel = 10;
    pet->wis = 10;
    pet->cha = 10;
    pet->is_alive = true;

    ESP_LOGI(TAG, "Pet resurrected: %s", pet->name);
}
