#ifndef REST_ENGINE_H
#define REST_ENGINE_H

#include "mistolito.h"
#include <stdint.h>
#include <stdbool.h>

bool rest_should_rest(int16_t hp, int16_t hp_max, uint8_t hp_rest_threshold);
void rest_init(rest_state_t *rest, pet_t *pet);
bool rest_tick_hp(pet_t *pet, rest_state_t *rest);
bool rest_tick_energy(pet_t *pet, rest_state_t *rest);

#endif
