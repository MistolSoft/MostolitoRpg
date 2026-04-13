#ifndef PROFESSION_ENGINE_H
#define PROFESSION_ENGINE_H

#include "mistolito.h"
#include "esp_err.h"

#define PROF_LEVEL_MAX 20
#define PROF_LEVEL_CYCLE_RESET 1

#define STAT_DC_BASE 10
#define PROF_CHANGE_DC 10

bool profession_check_requirements(pet_t *pet, uint8_t profession_id);
bool profession_try_change(pet_t *pet, uint8_t new_profession_id);
void profession_increment_level(pet_t *pet);
void profession_apply_level_bonuses(pet_t *pet, uint8_t profession_level);
bool profession_try_stat_increase(pet_t *pet, uint8_t stat_idx, bool has_class_bonus);
void profession_get_bonus_stats(pet_t *pet, uint8_t stats[STAT_COUNT], uint8_t *count);

#endif
