#ifndef FEATURES_ENGINE_H
#define FEATURES_ENGINE_H

#include "mistolito.h"
#include "esp_err.h"

#define MAX_FEATURE_CANDIDATES 16

typedef struct {
    char name[32];
    uint8_t dp_cost;
    uint8_t success_dc;
    char archetype_req[32];
} feature_candidate_t;

void features_init(void);
uint8_t features_get_available(pet_t *pet, uint8_t profession_level, feature_candidate_t *candidates, uint8_t max_count);
bool features_try_learn(pet_t *pet, feature_candidate_t *candidate);

#endif
