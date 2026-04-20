#ifndef RESOURCES_ENGINE_H
#define RESOURCES_ENGINE_H

#include "mistolito.h"
#include "esp_err.h"

void resources_init(void);
void resources_apply_profession(pet_t *pet);
void resources_apply_level(pet_t *pet, uint8_t profession_level);
void resources_recover_short_rest(pet_t *pet);
void resources_recover_long_rest(pet_t *pet);

#endif
