#include "resources_engine.h"
#include "storage_task.h"
#include "spi_bus.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "RESOURCES";

static char *g_resources_json = NULL;

void resources_init(void)
{
if (g_resources_json != NULL) {
return;
}

spi_bus_lock();

FILE *f = fopen("/sdcard/DATA/TABLES/resources.json", "r");
if (!f) {
spi_bus_unlock();
ESP_LOGW(TAG, "resources.json not found");
return;
}

fseek(f, 0, SEEK_END);
long size = ftell(f);
fseek(f, 0, SEEK_SET);

g_resources_json = malloc(size + 1);
if (!g_resources_json) {
fclose(f);
spi_bus_unlock();
ESP_LOGE(TAG, "Failed to allocate memory for resources.json");
return;
}

fread(g_resources_json, 1, size, f);
g_resources_json[size] = '\0';
fclose(f);

spi_bus_unlock();

ESP_LOGI(TAG, "resources.json loaded (%ld bytes)", size);
}

static void apply_warrior_resources(pet_t *pet, uint8_t level)
{
    if (pet->profession != PROF_WARRIOR) return;

    cJSON *root = cJSON_Parse(g_resources_json);
    if (!root) return;

    cJSON *resources = cJSON_GetObjectItem(root, "resources");
    if (!resources) {
        cJSON_Delete(root);
        return;
    }

    cJSON *warrior = cJSON_GetObjectItem(resources, "warrior");
    if (!warrior) {
        cJSON_Delete(root);
        return;
    }

    cJSON *action_surge = cJSON_GetObjectItem(warrior, "action_surge");
    if (action_surge) {
        cJSON *count_by_level = cJSON_GetObjectItem(action_surge, "count_by_level");
        if (count_by_level) {
            pet->action_surge_max = 1;
            cJSON *level_entry = NULL;
            cJSON_ArrayForEach(level_entry, count_by_level) {
                cJSON *lvl = cJSON_GetObjectItem(level_entry, "level");
                cJSON *cnt = cJSON_GetObjectItem(level_entry, "count");
                if (lvl && cnt && level >= (uint8_t)lvl->valueint) {
                    pet->action_surge_max = (uint8_t)cnt->valueint;
                }
            }
        }
    }

    cJSON *second_wind = cJSON_GetObjectItem(warrior, "second_wind");
    if (second_wind) {
        pet->second_wind_uses = 1;
    }

    cJSON *indomitable = cJSON_GetObjectItem(warrior, "indomitable");
    if (indomitable) {
        cJSON *count_by_level = cJSON_GetObjectItem(indomitable, "count_by_level");
        if (count_by_level) {
            pet->indomitable_max = 0;
            cJSON *level_entry = NULL;
            cJSON_ArrayForEach(level_entry, count_by_level) {
                cJSON *lvl = cJSON_GetObjectItem(level_entry, "level");
                cJSON *cnt = cJSON_GetObjectItem(level_entry, "count");
                if (lvl && cnt && level >= (uint8_t)lvl->valueint) {
                    pet->indomitable_max = (uint8_t)cnt->valueint;
                }
            }
        }
    }

    cJSON *superiority = cJSON_GetObjectItem(warrior, "superiority_dice");
    if (superiority) {
        cJSON *count_by_level = cJSON_GetObjectItem(superiority, "count_by_level");
        if (count_by_level) {
            pet->superiority_dice_max = 0;
            cJSON *level_entry = NULL;
            cJSON_ArrayForEach(level_entry, count_by_level) {
                cJSON *lvl = cJSON_GetObjectItem(level_entry, "level");
                cJSON *cnt = cJSON_GetObjectItem(level_entry, "count");
                if (lvl && cnt && level >= (uint8_t)lvl->valueint) {
                    pet->superiority_dice_max = (uint8_t)cnt->valueint;
                }
            }
        }

        cJSON *dice_sides = cJSON_GetObjectItem(superiority, "dice_sides_by_level");
        if (dice_sides) {
            pet->superiority_dice_size = 8;
            cJSON *level_entry = NULL;
            cJSON_ArrayForEach(level_entry, dice_sides) {
                cJSON *lvl = cJSON_GetObjectItem(level_entry, "level");
                cJSON *sides = cJSON_GetObjectItem(level_entry, "sides");
                if (lvl && sides && level >= (uint8_t)lvl->valueint) {
                    pet->superiority_dice_size = (uint8_t)sides->valueint;
                }
            }
        }
    }

    cJSON_Delete(root);
}

static void apply_mage_resources(pet_t *pet, uint8_t level)
{
    if (pet->profession != PROF_MAGE) return;

    cJSON *root = cJSON_Parse(g_resources_json);
    if (!root) return;

    cJSON *resources = cJSON_GetObjectItem(root, "resources");
    if (!resources) {
        cJSON_Delete(root);
        return;
    }

    cJSON *mage = cJSON_GetObjectItem(resources, "mage");
    if (!mage) {
        cJSON_Delete(root);
        return;
    }

    cJSON *spell_slots = cJSON_GetObjectItem(mage, "spell_slots");
    if (spell_slots) {
        cJSON *slots_by_level = cJSON_GetObjectItem(spell_slots, "slots_by_level");
        if (slots_by_level) {
            char level_str[4];
            snprintf(level_str, sizeof(level_str), "%d", level);

            cJSON *level_data = cJSON_GetObjectItem(slots_by_level, level_str);
            if (level_data) {
                for (int i = 1; i <= 9; i++) {
                    char slot_key[4];
                    snprintf(slot_key, sizeof(slot_key), "%d", i);
                    cJSON *slot_count = cJSON_GetObjectItem(level_data, slot_key);
                    if (slot_count) {
                        pet->spell_slots_max[i - 1] = (uint8_t)slot_count->valueint;
                        pet->spell_slots.slots[i - 1] = pet->spell_slots_max[i - 1];
                    } else {
                        pet->spell_slots_max[i - 1] = 0;
                        pet->spell_slots.slots[i - 1] = 0;
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
}

static void apply_rogue_resources(pet_t *pet, uint8_t level)
{
    if (pet->profession != PROF_ROGUE) return;

    cJSON *root = cJSON_Parse(g_resources_json);
    if (!root) return;

    cJSON *resources = cJSON_GetObjectItem(root, "resources");
    if (!resources) {
        cJSON_Delete(root);
        return;
    }

    cJSON *rogue = cJSON_GetObjectItem(resources, "rogue");
    if (!rogue) {
        cJSON_Delete(root);
        return;
    }

    cJSON *sneak_attack = cJSON_GetObjectItem(rogue, "sneak_attack");
    if (sneak_attack) {
        cJSON *dice_count = cJSON_GetObjectItem(sneak_attack, "dice_count_by_level");
        if (dice_count) {
            pet->sneak_attack_dice = 1;
            cJSON *level_entry = NULL;
            cJSON_ArrayForEach(level_entry, dice_count) {
                cJSON *lvl = cJSON_GetObjectItem(level_entry, "level");
                cJSON *cnt = cJSON_GetObjectItem(level_entry, "count");
                if (lvl && cnt && level >= (uint8_t)lvl->valueint) {
                    pet->sneak_attack_dice = (uint8_t)cnt->valueint;
                }
            }
        }
    }

    cJSON_Delete(root);
}

void resources_apply_profession(pet_t *pet)
{
if (pet == NULL || pet->profession == PROF_NONE) return;

resources_init();

memset(&pet->spell_slots, 0, sizeof(spell_slots_t));
memset(pet->spell_slots_max, 0, sizeof(pet->spell_slots_max));
pet->action_surge_uses = 0;
pet->action_surge_max = 0;
pet->indomitable_uses = 0;
pet->indomitable_max = 0;
pet->second_wind_uses = 0;
pet->superiority_dice = 0;
pet->superiority_dice_max = 0;
pet->superiority_dice_size = 0;
pet->sneak_attack_dice = 0;
pet->arcane_recovery_used = 0;
pet->ability_points_max = 3;
pet->ability_points = pet->ability_points_max;

resources_apply_level(pet, 1);
}

void resources_apply_level(pet_t *pet, uint8_t profession_level)
{
    if (pet == NULL || pet->profession == PROF_NONE) return;

    resources_init();

    switch (pet->profession) {
        case PROF_WARRIOR:
            apply_warrior_resources(pet, profession_level);
            break;
        case PROF_MAGE:
            apply_mage_resources(pet, profession_level);
            break;
        case PROF_ROGUE:
            apply_rogue_resources(pet, profession_level);
            break;
    }

    pet->action_surge_uses = pet->action_surge_max;
    pet->indomitable_uses = pet->indomitable_max;
    pet->second_wind_uses = 1;
    pet->superiority_dice = pet->superiority_dice_max;

    ESP_LOGI(TAG, "Applied resources for %s level %d", 
             pet->profession == PROF_WARRIOR ? "Warrior" :
             pet->profession == PROF_MAGE ? "Mage" : "Rogue",
             profession_level);
}

void resources_recover_short_rest(pet_t *pet)
{
if (pet == NULL) return;

if (pet->profession == PROF_WARRIOR) {
pet->action_surge_uses = pet->action_surge_max;
pet->second_wind_uses = 1;
pet->superiority_dice = pet->superiority_dice_max;
}

pet->ability_points = pet->ability_points_max;

ESP_LOGI(TAG, "Short rest resources recovered (ability_points: %d)", pet->ability_points);
}

void resources_recover_long_rest(pet_t *pet)
{
if (pet == NULL) return;

resources_recover_short_rest(pet);

if (pet->profession == PROF_WARRIOR) {
pet->indomitable_uses = pet->indomitable_max;
}

    if (pet->profession == PROF_MAGE) {
        for (int i = 0; i < 9; i++) {
            pet->spell_slots.slots[i] = pet->spell_slots_max[i];
        }
        pet->arcane_recovery_used = 0;
    }

    ESP_LOGI(TAG, "Long rest resources recovered");
}
