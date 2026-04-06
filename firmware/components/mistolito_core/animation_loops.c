#include "animation_loops.h"
#include "screens.h"
#include "mistolito.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ANIM_LOOP";

static visual_state_e s_current_state = VISUAL_IDLE;
static visual_state_e s_pending_state = VISUAL_IDLE;
static bool s_animation_busy = false;
static int16_t s_pending_damage = 0;

static visual_event_t s_event_queue[VISUAL_EVENT_QUEUE_SIZE];
static uint8_t s_event_queue_head = 0;
static uint8_t s_event_queue_tail = 0;
static uint8_t s_event_queue_count = 0;

static bool event_queue_push(const visual_event_t *evt)
{
    if (s_event_queue_count >= VISUAL_EVENT_QUEUE_SIZE) {
        ESP_LOGW(TAG, "Visual event queue full, dropping event");
        return false;
    }
    s_event_queue[s_event_queue_head] = *evt;
    s_event_queue_head = (s_event_queue_head + 1) % VISUAL_EVENT_QUEUE_SIZE;
    s_event_queue_count++;
    return true;
}

static bool event_queue_pop(visual_event_t *evt)
{
    if (s_event_queue_count == 0) {
        return false;
    }
    *evt = s_event_queue[s_event_queue_tail];
    s_event_queue_tail = (s_event_queue_tail + 1) % VISUAL_EVENT_QUEUE_SIZE;
    s_event_queue_count--;
    return true;
}

void anim_loops_init(void)
{
    s_current_state = VISUAL_IDLE;
    s_pending_state = VISUAL_IDLE;
    s_animation_busy = false;
    s_pending_damage = 0;
    s_event_queue_head = 0;
    s_event_queue_tail = 0;
    s_event_queue_count = 0;
    memset(s_event_queue, 0, sizeof(s_event_queue));
    ESP_LOGI(TAG, "Animation loops initialized");
}

void anim_loops_set_state(visual_state_e state)
{
    s_pending_state = state;
    ESP_LOGD(TAG, "State change requested: %d -> %d", s_current_state, state);
}

visual_state_e anim_loops_get_state(void)
{
    return s_current_state;
}

void anim_loops_process_event(game_event_t *evt)
{
    if (evt == NULL) return;

    visual_event_t visual_evt;
    memset(&visual_evt, 0, sizeof(visual_evt));

    switch (evt->type) {
        case EVT_STATE_CHANGED:
            visual_evt.type = VISUAL_EVT_STATE_CHANGE;
            visual_evt.data.new_state = evt->data.new_state;
            break;

        case EVT_PLAYER_ATTACK:
            visual_evt.type = VISUAL_EVT_PLAYER_ATTACK;
            visual_evt.data.player_attack.damage = evt->data.attack.damage;
            visual_evt.data.player_attack.hit = evt->data.attack.hit;
            visual_evt.data.player_attack.target_idx = evt->data.attack.target_idx;
            break;

        case EVT_ENEMY_ATTACK:
            visual_evt.type = VISUAL_EVT_ENEMY_ATTACK;
            visual_evt.data.enemy_attack.enemy_idx = evt->data.enemy_attack.enemy_idx;
            visual_evt.data.enemy_attack.damage = evt->data.enemy_attack.damage;
            visual_evt.data.enemy_attack.hit = evt->data.enemy_attack.hit;
            break;

        case EVT_VICTORY:
            visual_evt.type = VISUAL_EVT_VICTORY;
            visual_evt.data.exp_amount = evt->data.exp.amount;
            break;

        case EVT_ENEMY_SPAWNED:
            visual_evt.type = VISUAL_EVT_ENEMY_SPAWNED;
            break;

        default:
            return;
    }

    event_queue_push(&visual_evt);
    if (s_animation_busy) {
        ESP_LOGD(TAG, "Queued event type %d (queue: %d)", visual_evt.type, s_event_queue_count);
    }
}

static void process_visual_event(visual_event_t *evt)
{
    switch (evt->type) {
        case VISUAL_EVT_STATE_CHANGE:
            switch (evt->data.new_state) {
                case GS_SEARCHING:
                    s_pending_state = VISUAL_SEARCHING;
                    break;
                case GS_COMBAT:
                    s_pending_state = VISUAL_COMBAT_IDLE;
                    break;
                case GS_VICTORY:
                    s_pending_state = VISUAL_VICTORY;
                    break;
                case GS_RESTING:
                    s_pending_state = VISUAL_RESTING;
                    break;
                case GS_DEAD:
                    s_pending_state = VISUAL_DEAD;
                    break;
                default:
                    break;
            }
            screens_clear_damage_popup();
            screens_clear_exp_popup();
            break;

        case VISUAL_EVT_PLAYER_ATTACK:
            if (evt->data.player_attack.hit) {
                s_pending_state = VISUAL_COMBAT_PLAYER_ATTACK;
                s_pending_damage = evt->data.player_attack.damage;
                s_animation_busy = true;
            } else {
                screens_show_miss();
            }
            break;

        case VISUAL_EVT_ENEMY_ATTACK:
            if (evt->data.enemy_attack.hit) {
                s_pending_state = VISUAL_COMBAT_ENEMY_ATTACK;
                s_animation_busy = true;
            }
            break;

        case VISUAL_EVT_VICTORY:
            screens_show_exp_popup(evt->data.exp_amount);
            break;

        case VISUAL_EVT_ENEMY_SPAWNED:
            screens_set_enemy_animation(0);
            break;

        default:
            break;
    }
}

void anim_loops_tick(void)
{
    if (s_current_state != s_pending_state) {
        s_current_state = s_pending_state;

        switch (s_current_state) {
            case VISUAL_IDLE:
            case VISUAL_SEARCHING:
                screens_set_pet_animation(0);
                break;
            case VISUAL_RESTING:
                screens_set_pet_animation(0);
                break;
            case VISUAL_COMBAT_IDLE:
                screens_set_pet_animation(0);
                break;
            case VISUAL_COMBAT_PLAYER_ATTACK:
                screens_set_pet_animation(1);
                screens_show_damage_popup(s_pending_damage, false);
                break;
            case VISUAL_COMBAT_ENEMY_ATTACK:
                screens_set_pet_animation(2);
                break;
            case VISUAL_VICTORY:
                screens_set_pet_animation(4);
                break;
            case VISUAL_DEAD:
                screens_set_pet_animation(3);
                break;
        }
        return;
    }

    switch (s_current_state) {
        case VISUAL_COMBAT_PLAYER_ATTACK:
        case VISUAL_COMBAT_ENEMY_ATTACK:
            s_animation_busy = false;
            s_pending_state = VISUAL_COMBAT_IDLE;
            break;
        case VISUAL_VICTORY:
            s_pending_state = VISUAL_SEARCHING;
            break;
        default:
            break;
    }

    if (!s_animation_busy && s_event_queue_count > 0) {
        visual_event_t evt;
        if (event_queue_pop(&evt)) {
            ESP_LOGD(TAG, "Processing queued event type %d", evt.type);
            process_visual_event(&evt);
        }
    }
}

bool anim_loops_is_busy(void)
{
    return s_animation_busy;
}

uint8_t anim_loops_pending_count(void)
{
    return s_event_queue_count;
}
