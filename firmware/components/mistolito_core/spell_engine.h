#ifndef SPELL_ENGINE_H
#define SPELL_ENGINE_H

#include "mistolito.h"
#include "esp_err.h"

#define MAX_SPELL_CANDIDATES 16

typedef struct {
    char id[32];
    char name[32];
    uint8_t level;
    uint8_t dp_cost;
    uint8_t success_dc;
} spell_candidate_t;

void spells_init(void);
uint8_t spells_get_available(pet_t *pet, spell_candidate_t *candidates, uint8_t max_count);
bool spells_try_learn(pet_t *pet, spell_candidate_t *candidate);

#endif
