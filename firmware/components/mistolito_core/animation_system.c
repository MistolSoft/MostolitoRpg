#include "animation_system.h"
#include "esp_timer.h"
#include <string.h>

void animation_system_init(animation_system_t *sys)
{
    memset(sys, 0, sizeof(animation_system_t));
}

void animation_start_pet(animation_system_t *sys, uint8_t type, uint32_t duration_ms)
{
    sys->pet_animation.type = type;
    sys->pet_animation.start_ms = (uint32_t)(esp_timer_get_time() / 1000);
    sys->pet_animation.duration_ms = duration_ms;
    sys->pet_animation.active = true;
}

void animation_start_enemy(animation_system_t *sys, uint8_t enemy_idx, uint8_t type, uint32_t duration_ms)
{
    if (enemy_idx >= MAX_ENEMIES_PER_ENCOUNTER) return;

    sys->enemy_animations[enemy_idx].type = type;
    sys->enemy_animations[enemy_idx].start_ms = (uint32_t)(esp_timer_get_time() / 1000);
    sys->enemy_animations[enemy_idx].duration_ms = duration_ms;
    sys->enemy_animations[enemy_idx].target_idx = enemy_idx;
    sys->enemy_animations[enemy_idx].active = true;
}

void animation_start_ui(animation_system_t *sys, uint8_t type, uint32_t duration_ms)
{
    sys->ui_animation.type = type;
    sys->ui_animation.start_ms = (uint32_t)(esp_timer_get_time() / 1000);
    sys->ui_animation.duration_ms = duration_ms;
    sys->ui_animation.active = true;
}

bool animation_is_complete(animation_state_t *anim)
{
    if (!anim->active) return true;

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed = now - anim->start_ms;

    if (elapsed >= anim->duration_ms) {
        anim->active = false;
        return true;
    }

    return false;
}

void animation_update(animation_system_t *sys)
{
    animation_is_complete(&sys->pet_animation);

    for (uint8_t i = 0; i < MAX_ENEMIES_PER_ENCOUNTER; i++) {
        animation_is_complete(&sys->enemy_animations[i]);
    }

    animation_is_complete(&sys->ui_animation);
}

float animation_get_progress(animation_state_t *anim)
{
    if (!anim->active) return 1.0f;

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed = now - anim->start_ms;

    float progress = (float)elapsed / (float)anim->duration_ms;

    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    return progress;
}
