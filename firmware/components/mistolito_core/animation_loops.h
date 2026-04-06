#ifndef ANIMATION_LOOPS_H
#define ANIMATION_LOOPS_H

#include "events.h"
#include <stdbool.h>
#include <stdint.h>

#define VISUAL_EVENT_QUEUE_SIZE 8

typedef enum {
    VISUAL_IDLE,
    VISUAL_SEARCHING,
    VISUAL_RESTING,
    VISUAL_COMBAT_IDLE,
    VISUAL_COMBAT_PLAYER_ATTACK,
    VISUAL_COMBAT_ENEMY_ATTACK,
    VISUAL_VICTORY,
    VISUAL_DEAD
} visual_state_e;

typedef enum {
    VISUAL_EVT_NONE,
    VISUAL_EVT_STATE_CHANGE,
    VISUAL_EVT_PLAYER_ATTACK,
    VISUAL_EVT_ENEMY_ATTACK,
    VISUAL_EVT_VICTORY,
    VISUAL_EVT_ENEMY_SPAWNED
} visual_event_type_e;

typedef struct {
    visual_event_type_e type;
    union {
        game_state_e new_state;
        struct { int16_t damage; bool hit; uint8_t target_idx; } player_attack;
        struct { uint8_t enemy_idx; int16_t damage; bool hit; } enemy_attack;
        uint32_t exp_amount;
    } data;
} visual_event_t;

void anim_loops_init(void);
void anim_loops_set_state(visual_state_e state);
visual_state_e anim_loops_get_state(void);
void anim_loops_process_event(game_event_t *evt);
void anim_loops_tick(void);
bool anim_loops_is_busy(void);
uint8_t anim_loops_pending_count(void);

#endif
