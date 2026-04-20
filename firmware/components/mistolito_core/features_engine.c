#include "features_engine.h"
#include "storage_task.h"
#include "spi_bus.h"
#include "esp_log.h"
#include "esp_random.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "FEATURES";

static char *g_features_json = NULL;

static uint8_t roll_d20(void)
{
return (esp_random() % 20) + 1;
}

void features_init(void)
{
if (g_features_json != NULL) {
return;
}

spi_bus_lock();

FILE *f = fopen("/sdcard/DATA/TABLES/features_by_level.json", "r");
if (!f) {
spi_bus_unlock();
ESP_LOGW(TAG, "features_by_level.json not found");
return;
}

fseek(f, 0, SEEK_END);
long size = ftell(f);
fseek(f, 0, SEEK_SET);

g_features_json = malloc(size + 1);
if (!g_features_json) {
fclose(f);
spi_bus_unlock();
ESP_LOGE(TAG, "Failed to allocate memory for features_by_level.json");
return;
}

fread(g_features_json, 1, size, f);
g_features_json[size] = '\0';
fclose(f);

spi_bus_unlock();

ESP_LOGI(TAG, "features_by_level.json loaded (%ld bytes)", size);
}

static const char *get_profession_name(uint8_t profession)
{
    switch (profession) {
        case PROF_WARRIOR: return "warrior";
        case PROF_MAGE: return "mage";
        case PROF_ROGUE: return "rogue";
        default: return NULL;
    }
}

static bool feature_is_known(pet_t *pet, const char *name)
{
    for (uint8_t i = 0; i < pet->perk_count; i++) {
        cJSON *root = cJSON_Parse(storage_get_game_tables_json());
        if (root) {
            cJSON *perks = cJSON_GetObjectItem(root, "perks");
            if (perks) {
                cJSON *perk = NULL;
                cJSON_ArrayForEach(perk, perks) {
                    cJSON *id_item = cJSON_GetObjectItem(perk, "id");
                    if (id_item && id_item->valueint == pet->perks[i].id) {
                        cJSON *name_item = cJSON_GetObjectItem(perk, "name");
                        if (name_item && strcmp(name_item->valuestring, name) == 0) {
                            cJSON_Delete(root);
                            return true;
                        }
                    }
                }
            }
            cJSON_Delete(root);
        }
    }

    for (uint8_t i = 0; i < pet->skill_count; i++) {
        cJSON *root = cJSON_Parse(storage_get_game_tables_json());
        if (root) {
            cJSON *skills = cJSON_GetObjectItem(root, "skills");
            if (skills) {
                cJSON *skill = NULL;
                cJSON_ArrayForEach(skill, skills) {
                    cJSON *id_item = cJSON_GetObjectItem(skill, "id");
                    if (id_item && id_item->valueint == pet->skills[i].skill_id) {
                        cJSON *name_item = cJSON_GetObjectItem(skill, "name");
                        if (name_item && strcmp(name_item->valuestring, name) == 0) {
                            cJSON_Delete(root);
                            return true;
                        }
                    }
                }
            }
            cJSON_Delete(root);
        }
    }

    return false;
}

static void parse_feature_object(cJSON *feature_obj, feature_candidate_t *candidate)
{
    if (!feature_obj || !candidate) return;

    cJSON *name = cJSON_GetObjectItem(feature_obj, "name");
    cJSON *dp_cost = cJSON_GetObjectItem(feature_obj, "dp_cost");
    cJSON *success_dc = cJSON_GetObjectItem(feature_obj, "success_dc");

    if (name && dp_cost && success_dc) {
        strncpy(candidate->name, name->valuestring, sizeof(candidate->name) - 1);
        candidate->name[sizeof(candidate->name) - 1] = '\0';
        candidate->dp_cost = (uint8_t)dp_cost->valueint;
        candidate->success_dc = (uint8_t)success_dc->valueint;
        memset(candidate->archetype_req, 0, sizeof(candidate->archetype_req));
    }
}

uint8_t features_get_available(pet_t *pet, uint8_t profession_level, feature_candidate_t *candidates, uint8_t max_count)
{
    if (pet == NULL || candidates == NULL || pet->profession == PROF_NONE) {
        return 0;
    }

    features_init();

    if (g_features_json == NULL) {
        return 0;
    }

    cJSON *root = cJSON_Parse(g_features_json);
    if (!root) {
        return 0;
    }

    cJSON *features_by_level = cJSON_GetObjectItem(root, "features_by_level");
    if (!features_by_level) {
        cJSON_Delete(root);
        return 0;
    }

    const char *prof_name = get_profession_name(pet->profession);
    if (!prof_name) {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *prof_features = cJSON_GetObjectItem(features_by_level, prof_name);
    if (!prof_features) {
        cJSON_Delete(root);
        return 0;
    }

    char level_str[4];
    snprintf(level_str, sizeof(level_str), "%d", profession_level);

    cJSON *level_data = cJSON_GetObjectItem(prof_features, level_str);
    if (!level_data) {
        cJSON_Delete(root);
        return 0;
    }

    uint8_t count = 0;

    cJSON *features_available = cJSON_GetObjectItem(level_data, "features_available");
    if (features_available && cJSON_IsArray(features_available)) {
        cJSON *feature_obj = NULL;
        cJSON_ArrayForEach(feature_obj, features_available) {
            if (count >= max_count) break;

            feature_candidate_t candidate;
            parse_feature_object(feature_obj, &candidate);

            if (candidate.name[0] != '\0' && !feature_is_known(pet, candidate.name)) {
                if (pet->dp >= candidate.dp_cost) {
                    candidates[count++] = candidate;
                }
            }
        }
    }

    cJSON *features_by_archetype = cJSON_GetObjectItem(level_data, "features_available_by_archetype");
    if (features_by_archetype) {
        cJSON *archetype_features = NULL;
        cJSON_ArrayForEach(archetype_features, features_by_archetype) {
            if (count >= max_count) break;

            if (cJSON_IsArray(archetype_features)) {
                cJSON *feature_obj = NULL;
                cJSON_ArrayForEach(feature_obj, archetype_features) {
                    if (count >= max_count) break;

                    feature_candidate_t candidate;
                    parse_feature_object(feature_obj, &candidate);

                    if (candidate.name[0] != '\0' && !feature_is_known(pet, candidate.name)) {
                        strncpy(candidate.archetype_req, archetype_features->string, sizeof(candidate.archetype_req) - 1);

                        if (pet->dp >= candidate.dp_cost) {
                            candidates[count++] = candidate;
                        }
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    return count;
}

bool features_try_learn(pet_t *pet, feature_candidate_t *candidate)
{
    if (pet == NULL || candidate == NULL) {
        return false;
    }

    if (pet->dp < candidate->dp_cost) {
        ESP_LOGI(TAG, "Not enough DP for %s: have %lu, need %d", 
                 candidate->name, pet->dp, candidate->dp_cost);
        return false;
    }

    uint8_t roll = roll_d20();
    bool success = (roll >= candidate->success_dc);

    pet->dp -= candidate->dp_cost;

    if (success) {
        cJSON *root = cJSON_Parse(storage_get_game_tables_json());
        if (root) {
            cJSON *skills = cJSON_GetObjectItem(root, "skills");
            if (skills) {
                cJSON *skill = NULL;
                cJSON_ArrayForEach(skill, skills) {
                    cJSON *name_item = cJSON_GetObjectItem(skill, "name");
                    if (name_item && strcmp(name_item->valuestring, candidate->name) == 0) {
                        cJSON *id_item = cJSON_GetObjectItem(skill, "id");
                        if (id_item && pet->skill_count < MAX_SKILLS) {
                            pet->skills[pet->skill_count].skill_id = (uint8_t)id_item->valueint;
                            pet->skills[pet->skill_count].uses_remaining = 3;
                            pet->skills[pet->skill_count].uses_max = 3;
                            pet->skill_count++;
                            ESP_LOGI(TAG, "Feature %s learned! (roll=%d, DC=%d)", 
                                     candidate->name, roll, candidate->success_dc);
                        }
                        break;
                    }
                }
            }

            cJSON *perks = cJSON_GetObjectItem(root, "perks");
            if (perks) {
                cJSON *perk = NULL;
                cJSON_ArrayForEach(perk, perks) {
                    cJSON *name_item = cJSON_GetObjectItem(perk, "name");
                    if (name_item && strcmp(name_item->valuestring, candidate->name) == 0) {
                        cJSON *id_item = cJSON_GetObjectItem(perk, "id");
                        if (id_item && pet->perk_count < MAX_PERKS) {
                            pet->perks[pet->perk_count].id = (uint8_t)id_item->valueint;
                            pet->perk_count++;
                            ESP_LOGI(TAG, "Perk %s learned! (roll=%d, DC=%d)", 
                                     candidate->name, roll, candidate->success_dc);
                        }
                        break;
                    }
                }
            }

            cJSON_Delete(root);
        }
        return true;
    } else {
        ESP_LOGI(TAG, "Failed to learn %s (roll=%d, DC=%d), DP consumed", 
                 candidate->name, roll, candidate->success_dc);
        return false;
    }
}
