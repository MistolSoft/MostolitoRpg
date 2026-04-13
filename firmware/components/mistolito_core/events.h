#ifndef EVENTS_H
#define EVENTS_H

#include "mistolito.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    EVT_STATE_CHANGED,
    EVT_PET_UPDATED,
    EVT_ENEMY_SPAWNED,
    EVT_ENEMY_DIED,
    EVT_DAMAGE_DEALT,
    EVT_VICTORY,
    EVT_LEVEL_UP,
    EVT_ENERGY_CHANGED,
    EVT_GAME_START,

    EVT_COMBAT_PHASE_CHANGED,
    EVT_PLAYER_ATTACK,
    EVT_ENEMY_ATTACK,
    EVT_TARGET_SELECTED,
    EVT_ROUND_END,
    EVT_ANIMATION_START,
    EVT_ANIMATION_COMPLETE,

    EVT_COMBAT_START,
    EVT_TURN_READY,
    EVT_COMBAT_END
} event_type_e;

typedef struct {
    event_type_e type;
    union {
        game_state_e new_state;

        struct { int16_t amount; bool critical; } damage;

        struct { uint32_t amount; } exp;

        struct { uint8_t level; } level_up;

        struct { uint8_t current; uint8_t max; } energy;

        struct {
            combat_phase_e phase;
            uint8_t round;
        } combat_phase;

        struct {
            int16_t damage;
            bool hit;
            uint8_t target_idx;
        } attack;

        struct {
            uint8_t target_idx;
        } target;

        struct {
            uint8_t enemy_idx;
            int16_t damage;
            bool hit;
        } enemy_attack;

        struct {
            uint32_t duration_ms;
        } animation;

        struct {
            bool victory;
            uint32_t exp_gained;
            uint8_t enemies_killed;
        } combat_end;
    } data;
} game_event_t;

#define EVENT_QUEUE_SIZE 16

extern QueueHandle_t g_event_queue;

void events_init(void);
bool events_send(game_event_t *event);
bool events_receive(game_event_t *event, uint32_t timeout_ms);

#endif
