#include "rules.h"
#include "esp_random.h"
#include <math.h>

uint8_t rules_roll_d20(void)
{
    return (uint8_t)((esp_random() % 20) + 1);
}

uint8_t rules_roll_dice(uint8_t sides)
{
    if (sides == 0) return 0;
    return (uint8_t)((esp_random() % sides) + 1);
}

uint8_t rules_roll_multiple(uint8_t count, uint8_t sides)
{
    if (sides == 0 || count == 0) return 0;

    uint8_t total = 0;
    for (uint8_t i = 0; i < count; i++) {
        total += rules_roll_dice(sides);
    }
    return total;
}

int8_t rules_get_modifier(uint8_t attribute)
{
    if (attribute <= 1) return -5;
    if (attribute >= 20) return 5;
    return (int8_t)((attribute / 2) - 5);
}

bool rules_roll_stat_increase(bool has_advantage, bool has_disadvantage)
{
    uint8_t roll1 = rules_roll_d20();
    uint8_t roll2 = rules_roll_d20();
    uint8_t final_roll;

    if (has_advantage && !has_disadvantage) {
        final_roll = (roll1 > roll2) ? roll1 : roll2;
    } else if (has_disadvantage && !has_advantage) {
        final_roll = (roll1 < roll2) ? roll1 : roll2;
    } else {
        final_roll = roll1;
    }

    return final_roll >= 10;
}

float rules_skill_success_probability(uint8_t level)
{
    const float k = 0.015f;
    float prob = 100.0f * expf(-k * (float)level);

    if (prob > 100.0f) prob = 100.0f;
    if (prob < 0.001f) prob = 0.001f;

    return prob;
}

bool rules_roll_skill_success(float probability)
{
    uint32_t roll = esp_random() % 100000;
    uint32_t threshold = (uint32_t)(probability * 1000.0f);

    return roll < threshold;
}

uint32_t rules_random_range(uint32_t max)
{
    if (max == 0) return 0;
    return esp_random() % max;
}

bool rules_random_chance(uint8_t probability_percent)
{
    if (probability_percent == 0) return false;
    if (probability_percent >= 100) return true;
    return (uint8_t)(esp_random() % 100) < probability_percent;
}
