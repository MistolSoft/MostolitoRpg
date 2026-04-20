#include "spell_engine.h"
#include "storage_task.h"
#include "spi_bus.h"
#include "esp_log.h"
#include "esp_random.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "SPELLS";

static char *g_spells_cantrips = NULL;
static char *g_spells_level_1 = NULL;
static char *g_spells_level_2 = NULL;
static char *g_spells_level_3 = NULL;

static uint8_t roll_d20(void)
{
return (esp_random() % 20) + 1;
}

static char *load_spell_file(const char *path)
{
spi_bus_lock();

FILE *f = fopen(path, "r");
if (!f) {
spi_bus_unlock();
return NULL;
}

fseek(f, 0, SEEK_END);
long size = ftell(f);
fseek(f, 0, SEEK_SET);

char *content = malloc(size + 1);
if (!content) {
fclose(f);
spi_bus_unlock();
return NULL;
}

fread(content, 1, size, f);
content[size] = '\0';
fclose(f);

spi_bus_unlock();
return content;
}

void spells_init(void)
{
if (g_spells_cantrips == NULL) {
g_spells_cantrips = load_spell_file("/sdcard/DATA/TABLES/spells_cantrips.json");
if (g_spells_cantrips) {
ESP_LOGI(TAG, "spells_cantrips.json loaded");
}
}

if (g_spells_level_1 == NULL) {
g_spells_level_1 = load_spell_file("/sdcard/DATA/TABLES/spells_level_1.json");
if (g_spells_level_1) {
ESP_LOGI(TAG, "spells_level_1.json loaded");
}
}

if (g_spells_level_2 == NULL) {
g_spells_level_2 = load_spell_file("/sdcard/DATA/TABLES/spells_level_2.json");
        if (g_spells_level_2) {
            ESP_LOGI(TAG, "spells_level_2.json loaded");
        }
    }

    if (g_spells_level_3 == NULL) {
        g_spells_level_3 = load_spell_file("/sdcard/DATA/TABLES/spells_level_3.json");
        if (g_spells_level_3) {
            ESP_LOGI(TAG, "spells_level_3.json loaded");
        }
    }
}

static bool spell_is_known(pet_t *pet, const char *spell_id)
{
    for (uint8_t i = 0; i < pet->spells_known_count; i++) {
        if (strcmp(pet->spells_known[i].id, spell_id) == 0) {
            return true;
        }
    }
    return false;
}

static uint8_t get_max_spell_level(pet_t *pet)
{
    for (int i = 8; i >= 0; i--) {
        if (pet->spell_slots_max[i] > 0) {
            return (uint8_t)(i + 1);
        }
    }
    return 0;
}

static void parse_spells_from_json(char *json, pet_t *pet, uint8_t max_level, spell_candidate_t *candidates, uint8_t *count, uint8_t max_count)
{
    if (!json) return;

    cJSON *root = cJSON_Parse(json);
    if (!root) return;

    cJSON *spells = cJSON_GetObjectItem(root, "spells");
    if (!spells || !cJSON_IsArray(spells)) {
        cJSON_Delete(root);
        return;
    }

    cJSON *spell = NULL;
    cJSON_ArrayForEach(spell, spells) {
        if (*count >= max_count) break;

        cJSON *id = cJSON_GetObjectItem(spell, "id");
        cJSON *name = cJSON_GetObjectItem(spell, "name");
        cJSON *level = cJSON_GetObjectItem(spell, "level");
        cJSON *dp_cost = cJSON_GetObjectItem(spell, "dp_cost");
        cJSON *success_dc = cJSON_GetObjectItem(spell, "success_dc");

        if (!id || !name || !level || !dp_cost || !success_dc) continue;

        uint8_t spell_level = (uint8_t)level->valueint;

        if (spell_level > max_level) continue;

        if (spell_is_known(pet, id->valuestring)) continue;

        if (pet->dp < (uint32_t)dp_cost->valueint) continue;

        strncpy(candidates[*count].id, id->valuestring, sizeof(candidates[*count].id) - 1);
        candidates[*count].id[sizeof(candidates[*count].id) - 1] = '\0';

        strncpy(candidates[*count].name, name->valuestring, sizeof(candidates[*count].name) - 1);
        candidates[*count].name[sizeof(candidates[*count].name) - 1] = '\0';

        candidates[*count].level = spell_level;
        candidates[*count].dp_cost = (uint8_t)dp_cost->valueint;
        candidates[*count].success_dc = (uint8_t)success_dc->valueint;

        (*count)++;
    }

    cJSON_Delete(root);
}

uint8_t spells_get_available(pet_t *pet, spell_candidate_t *candidates, uint8_t max_count)
{
    if (pet == NULL || candidates == NULL) {
        return 0;
    }

    if (pet->profession != PROF_MAGE) {
        return 0;
    }

    spells_init();

    uint8_t count = 0;
    uint8_t max_level = get_max_spell_level(pet);

    parse_spells_from_json(g_spells_cantrips, pet, max_level, candidates, &count, max_count);
    parse_spells_from_json(g_spells_level_1, pet, max_level, candidates, &count, max_count);

    if (max_level >= 2) {
        parse_spells_from_json(g_spells_level_2, pet, max_level, candidates, &count, max_count);
    }

    if (max_level >= 3) {
        parse_spells_from_json(g_spells_level_3, pet, max_level, candidates, &count, max_count);
    }

    return count;
}

bool spells_try_learn(pet_t *pet, spell_candidate_t *candidate)
{
    if (pet == NULL || candidate == NULL) {
        return false;
    }

    if (pet->spells_known_count >= MAX_SPELLS_KNOWN) {
        ESP_LOGI(TAG, "Cannot learn more spells (max %d)", MAX_SPELLS_KNOWN);
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
snprintf(pet->spells_known[pet->spells_known_count].id, sizeof(pet->spells_known[0].id), "%s", candidate->id);
pet->spells_known[pet->spells_known_count].level = candidate->level;
        pet->spells_known_count++;

        ESP_LOGI(TAG, "Spell %s learned! (roll=%d, DC=%d)",
                 candidate->name, roll, candidate->success_dc);
        return true;
    } else {
        ESP_LOGI(TAG, "Failed to learn %s (roll=%d, DC=%d), DP consumed",
                 candidate->name, roll, candidate->success_dc);
        return false;
    }
}
