#include "rest_engine.h"
#include "rules.h"
#include "resources_engine.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "REST";

bool rest_should_rest(int16_t hp, int16_t hp_max, uint8_t hp_rest_threshold)
{
    if (hp_max <= 0) return false;
    
    uint32_t hp_percent = ((uint32_t)hp * 100) / (uint32_t)hp_max;
    
    return hp_percent < hp_rest_threshold;
}

void rest_init(rest_state_t *rest, pet_t *pet)
{
    int8_t con_mod = rules_get_modifier(pet->con);
    int8_t int_mod = rules_get_modifier(pet->intel);
    
    uint8_t hp_dice = rules_roll_dice(REST_DICE_SIDES);
    uint8_t energy_dice = rules_roll_dice(REST_DICE_SIDES);
    
    rest->hp_ticks_total = REST_BASE_TICKS + hp_dice;
    if (con_mod > 0) {
        rest->hp_ticks_total += (uint8_t)con_mod;
    }
    
    rest->energy_ticks_total = REST_BASE_TICKS + energy_dice;
    if (int_mod > 0) {
        rest->energy_ticks_total += (uint8_t)int_mod;
    }
    
    rest->hp_ticks_remaining = rest->hp_ticks_total;
    rest->energy_ticks_remaining = rest->energy_ticks_total;
    
    ESP_LOGI(TAG, "Rest init: HP ticks=%d, Energy ticks=%d", 
             rest->hp_ticks_total, rest->energy_ticks_total);
}

bool rest_tick_hp(pet_t *pet, rest_state_t *rest)
{
    if (rest->hp_ticks_remaining == 0) {
        return false;
    }
    
    rest->hp_ticks_remaining--;
    
    if (rules_random_chance(pet->rest.recovery_chance)) {
        uint16_t recovery = (uint16_t)(((uint32_t)pet->hp_max * REST_RECOVERY_PERCENT) / 100);
        if (recovery < 1) recovery = 1;
        
        pet->hp += (int16_t)recovery;
        if (pet->hp > pet->hp_max) {
            pet->hp = pet->hp_max;
        }
        
        ESP_LOGI(TAG, "HP recovered: +%d (now %d/%d)", recovery, pet->hp, pet->hp_max);
    }
    
    return rest->hp_ticks_remaining > 0;
}

bool rest_tick_energy(pet_t *pet, rest_state_t *rest)
{
    if (rest->energy_ticks_remaining == 0) {
        return false;
    }
    
    rest->energy_ticks_remaining--;
    
    if (rules_random_chance(pet->rest.recovery_chance)) {
        uint8_t recovery = (uint8_t)(((uint32_t)pet->energy_max * REST_RECOVERY_PERCENT) / 100);
        if (recovery < 1) recovery = 1;
        
        pet->energy += recovery;
        if (pet->energy > pet->energy_max) {
            pet->energy = pet->energy_max;
        }
        
        ESP_LOGI(TAG, "Energy recovered: +%d (now %d/%d)", recovery, pet->energy, pet->energy_max);
    }
    
    return rest->energy_ticks_remaining > 0;
}

void rest_finish(pet_t *pet)
{
resources_recover_long_rest(pet);
ESP_LOGI(TAG, "Long rest completed - all resources recovered");
}
