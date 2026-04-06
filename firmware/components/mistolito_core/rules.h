#ifndef RULES_H
#define RULES_H

#include <stdint.h>
#include <stdbool.h>

uint8_t rules_roll_d20(void);
uint8_t rules_roll_dice(uint8_t sides);
uint8_t rules_roll_multiple(uint8_t count, uint8_t sides);
int8_t rules_get_modifier(uint8_t attribute);
bool rules_roll_stat_increase(bool has_advantage, bool has_disadvantage);
float rules_skill_success_probability(uint8_t level);
bool rules_roll_skill_success(float probability);
uint32_t rules_random_range(uint32_t max);
bool rules_random_chance(uint8_t probability_percent);

#endif
