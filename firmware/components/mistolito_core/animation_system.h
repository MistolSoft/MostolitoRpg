#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include "mistolito.h"
#include <stdint.h>
#include <stdbool.h>

#define ANIMATION_IDLE 0
#define ANIMATION_ATTACK 1
#define ANIMATION_HIT 2
#define ANIMATION_DEATH 3
#define ANIMATION_LEVELUP 4

#define ANIMATION_DEFAULT_DURATION_MS 500

typedef struct {
    uint8_t type;
    uint32_t start_ms;
    uint32_t duration_ms;
    uint8_t target_idx;
    bool active;
} animation_state_t;

typedef struct {
    animation_state_t pet_animation;
    animation_state_t enemy_animations[MAX_ENEMIES_PER_ENCOUNTER];
    animation_state_t ui_animation;
} animation_system_t;

void animation_system_init(animation_system_t *sys);
void animation_start_pet(animation_system_t *sys, uint8_t type, uint32_t duration_ms);
void animation_start_enemy(animation_system_t *sys, uint8_t enemy_idx, uint8_t type, uint32_t duration_ms);
void animation_start_ui(animation_system_t *sys, uint8_t type, uint32_t duration_ms);
bool animation_is_complete(animation_state_t *anim);
void animation_update(animation_system_t *sys);
float animation_get_progress(animation_state_t *anim);

#endif
