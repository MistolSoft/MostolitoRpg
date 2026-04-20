#include "storage_task.h"
#include "mistolito.h"
#include "dna_engine.h"
#include "esp_log.h"
#include "esp_random.h"
#include "spi_bus.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

static const char *TAG = "STORAGE";

#define SD_MISO 40
#define SD_MOSI 38
#define SD_CLK 39
#define SD_CS 41
#define MOUNT_POINT "/sdcard"

QueueHandle_t g_storage_queue = NULL;
static bool g_mounted = false;

void storage_task_start(void)
{
    if (g_storage_queue == NULL) {
        g_storage_queue = xQueueCreate(STORAGE_QUEUE_SIZE, sizeof(storage_request_t));
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_CS;
    slot_cfg.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 4096
    };

    sdmmc_card_t *card = NULL;
    esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_cfg, &mount_cfg, &card);

    if (ret == ESP_OK) {
        g_mounted = true;
        ESP_LOGI(TAG, "SD card mounted");
    } else {
        ESP_LOGE(TAG, "Mount failed: %s", esp_err_to_name(ret));
    }
}

void storage_task(void *arg)
{
    storage_request_t req;

    ESP_LOGI(TAG, "Storage task started");

    while (1) {
        if (xQueueReceive(g_storage_queue, &req, portMAX_DELAY) == pdTRUE) {
            if (!g_mounted) {
                ESP_LOGW(TAG, "Storage not mounted, skipping operation");
                continue;
            }

            spi_bus_lock();

            switch (req.operation) {
case STORAGE_OP_SAVE_PET_DELTA:
    {
        FILE *f = fopen(MOUNT_POINT "/BRAIN/PET/pet_data.json", "w");
        if (f) {
            fprintf(f, "{\"name\":\"%s\",\"level\":%d,\"profession_level\":%d,\"exp\":%lu,\"hp\":%d,\"hp_max\":%d,\"energy\":%d,\"profession\":%d,\"str\":%d,\"dex\":%d,\"con\":%d,\"int\":%d,\"wis\":%d,\"cha\":%d,\"dp\":%lu,\"enemies_killed\":%lu,\"lives\":%d,\"hp_rest_threshold\":%d,\"recovery_chance\":%d,\"base_ac\":%d,\"damage_dice\":%d,\"damage_bonus\":%d,\"dice_count\":%d,\"dna_salt\":%lu,\"skill_count\":%d,\"perk_count\":%d",
            req.pet->name, req.pet->level, req.pet->profession_level, (unsigned long)req.pet->exp,
            req.pet->hp, req.pet->hp_max, req.pet->energy, req.pet->profession,
            req.pet->str, req.pet->dex, req.pet->con,
            req.pet->intel, req.pet->wis, req.pet->cha,
            (unsigned long)req.pet->dp, (unsigned long)req.pet->enemies_killed, req.pet->lives,
            req.pet->rest.hp_rest_threshold, req.pet->rest.recovery_chance,
            req.pet->combat.base_ac, req.pet->combat.damage_dice, req.pet->combat.damage_bonus, req.pet->combat.dice_count,
            (unsigned long)req.pet->dna.salt, req.pet->skill_count, req.pet->perk_count);

            fprintf(f, ",\"skills\":[");
            for (uint8_t i = 0; i < req.pet->skill_count; i++) {
                if (i > 0) fprintf(f, ",");
                fprintf(f, "{\"id\":%d,\"uses\":%d}", req.pet->skills[i].skill_id, req.pet->skills[i].uses_remaining);
            }
            fprintf(f, "]");

            fprintf(f, ",\"perks\":[");
            for (uint8_t i = 0; i < req.pet->perk_count; i++) {
                if (i > 0) fprintf(f, ",");
                fprintf(f, "{\"id\":%d}", req.pet->perks[i].id);
}
fprintf(f, "]");

fprintf(f, ",\"spell_slots\":[");
for (uint8_t i = 0; i < 9; i++) {
if (i > 0) fprintf(f, ",");
fprintf(f, "%d", req.pet->spell_slots.slots[i]);
}
fprintf(f, "]");

fprintf(f, ",\"spell_slots_max\":[");
for (uint8_t i = 0; i < 9; i++) {
if (i > 0) fprintf(f, ",");
fprintf(f, "%d", req.pet->spell_slots_max[i]);
}
fprintf(f, "]");

fprintf(f, ",\"spells_known\":[");
for (uint8_t i = 0; i < req.pet->spells_known_count; i++) {
if (i > 0) fprintf(f, ",");
fprintf(f, "{\"id\":\"%s\",\"level\":%d}", req.pet->spells_known[i].id, req.pet->spells_known[i].level);
}
fprintf(f, "]");

fprintf(f, ",\"cantrips_known\":%d,\"cantrips_max\":%d", req.pet->cantrips_known, req.pet->cantrips_max);

fprintf(f, ",\"action_surge_uses\":%d,\"action_surge_max\":%d", req.pet->action_surge_uses, req.pet->action_surge_max);
fprintf(f, ",\"indomitable_uses\":%d,\"indomitable_max\":%d", req.pet->indomitable_uses, req.pet->indomitable_max);
fprintf(f, ",\"second_wind_uses\":%d", req.pet->second_wind_uses);
fprintf(f, ",\"superiority_dice\":%d,\"superiority_dice_max\":%d,\"superiority_dice_size\":%d",
req.pet->superiority_dice, req.pet->superiority_dice_max, req.pet->superiority_dice_size);
fprintf(f, ",\"sneak_attack_dice\":%d", req.pet->sneak_attack_dice);
fprintf(f, ",\"arcane_recovery_used\":%d", req.pet->arcane_recovery_used);

fprintf(f, ",\"maneuvers\":[");
        for (uint8_t i = 0; i < req.pet->maneuver_count; i++) {
            if (i > 0) fprintf(f, ",");
            fprintf(f, "{\"id\":%d}", req.pet->maneuvers[i].maneuver_id);
        }
        fprintf(f, "]");

fprintf(f, ",\"ability_points\":%d,\"ability_points_max\":%d", req.pet->ability_points, req.pet->ability_points_max);

fprintf(f, "}");

fclose(f);
ESP_LOGI(TAG, "Pet saved (dirty=0x%04X)", req.dirty_flags);
        }
    }
    break;

            case STORAGE_OP_LOAD_PET:
            {
                FILE *f = fopen(MOUNT_POINT "/BRAIN/PET/pet_data.json", "r");
                if (f) {
                    char buf[512];
                    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
                    buf[len] = '\0';
                    fclose(f);

                    cJSON *root = cJSON_Parse(buf);
                    if (root) {
                        cJSON *item;
if ((item = cJSON_GetObjectItem(root, "name"))) {
    strncpy(req.pet->name, item->valuestring, PET_NAME_MAX_LEN - 1);
}
if ((item = cJSON_GetObjectItem(root, "level"))) req.pet->level = item->valueint;
if ((item = cJSON_GetObjectItem(root, "profession_level"))) req.pet->profession_level = item->valueint;
if ((item = cJSON_GetObjectItem(root, "exp"))) req.pet->exp = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "hp"))) req.pet->hp = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "hp_max"))) req.pet->hp_max = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "energy"))) req.pet->energy = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "str"))) req.pet->str = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "dex"))) req.pet->dex = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "con"))) req.pet->con = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "int"))) req.pet->intel = item->valueint;
                        if ((item = cJSON_GetObjectItem(root, "wis"))) req.pet->wis = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "cha"))) req.pet->cha = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "dp"))) req.pet->dp = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "enemies_killed"))) req.pet->enemies_killed = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "lives"))) req.pet->lives = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "hp_rest_threshold"))) req.pet->rest.hp_rest_threshold = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "recovery_chance"))) req.pet->rest.recovery_chance = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "base_ac"))) req.pet->combat.base_ac = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "damage_dice"))) req.pet->combat.damage_dice = item->valueint;
            if ((item = cJSON_GetObjectItem(root, "damage_bonus"))) req.pet->combat.damage_bonus = item->valueint;
if ((item = cJSON_GetObjectItem(root, "dice_count"))) req.pet->combat.dice_count = item->valueint;

    cJSON *skills_arr = cJSON_GetObjectItem(root, "skills");
    if (skills_arr && cJSON_IsArray(skills_arr)) {
        cJSON *skill_item = NULL;
        cJSON_ArrayForEach(skill_item, skills_arr) {
            if (req.pet->skill_count < MAX_SKILLS) {
                cJSON *id_item = cJSON_GetObjectItem(skill_item, "id");
                cJSON *uses_item = cJSON_GetObjectItem(skill_item, "uses");
                if (id_item) {
                    req.pet->skills[req.pet->skill_count].skill_id = (uint8_t)id_item->valueint;
                    req.pet->skills[req.pet->skill_count].uses_remaining = uses_item ? (uint8_t)uses_item->valueint : 3;
                    req.pet->skills[req.pet->skill_count].uses_max = 3;
                    req.pet->skill_count++;
                }
            }
        }
    }

cJSON *perks_arr = cJSON_GetObjectItem(root, "perks");
if (perks_arr && cJSON_IsArray(perks_arr)) {
cJSON *perk_item = NULL;
cJSON_ArrayForEach(perk_item, perks_arr) {
if (req.pet->perk_count < MAX_PERKS) {
cJSON *id_item = cJSON_GetObjectItem(perk_item, "id");
if (id_item) {
req.pet->perks[req.pet->perk_count].id = (uint8_t)id_item->valueint;
req.pet->perk_count++;
}
}
}
}

cJSON *spell_slots_arr = cJSON_GetObjectItem(root, "spell_slots");
if (spell_slots_arr && cJSON_IsArray(spell_slots_arr)) {
uint8_t idx = 0;
cJSON *slot_item = NULL;
cJSON_ArrayForEach(slot_item, spell_slots_arr) {
if (idx < 9) req.pet->spell_slots.slots[idx++] = (uint8_t)slot_item->valueint;
}
}

cJSON *spell_slots_max_arr = cJSON_GetObjectItem(root, "spell_slots_max");
if (spell_slots_max_arr && cJSON_IsArray(spell_slots_max_arr)) {
uint8_t idx = 0;
cJSON *slot_item = NULL;
cJSON_ArrayForEach(slot_item, spell_slots_max_arr) {
if (idx < 9) req.pet->spell_slots_max[idx++] = (uint8_t)slot_item->valueint;
}
}

cJSON *spells_known_arr = cJSON_GetObjectItem(root, "spells_known");
if (spells_known_arr && cJSON_IsArray(spells_known_arr)) {
cJSON *spell_item = NULL;
cJSON_ArrayForEach(spell_item, spells_known_arr) {
if (req.pet->spells_known_count < MAX_SPELLS_KNOWN) {
cJSON *id_item = cJSON_GetObjectItem(spell_item, "id");
cJSON *level_item = cJSON_GetObjectItem(spell_item, "level");
if (id_item) {
strncpy(req.pet->spells_known[req.pet->spells_known_count].id, id_item->valuestring, sizeof(req.pet->spells_known[0].id) - 1);
req.pet->spells_known[req.pet->spells_known_count].level = level_item ? (uint8_t)level_item->valueint : 0;
req.pet->spells_known_count++;
}
}
}
}

if ((item = cJSON_GetObjectItem(root, "cantrips_known"))) req.pet->cantrips_known = item->valueint;
if ((item = cJSON_GetObjectItem(root, "cantrips_max"))) req.pet->cantrips_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "action_surge_uses"))) req.pet->action_surge_uses = item->valueint;
if ((item = cJSON_GetObjectItem(root, "action_surge_max"))) req.pet->action_surge_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "indomitable_uses"))) req.pet->indomitable_uses = item->valueint;
if ((item = cJSON_GetObjectItem(root, "indomitable_max"))) req.pet->indomitable_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "second_wind_uses"))) req.pet->second_wind_uses = item->valueint;
if ((item = cJSON_GetObjectItem(root, "superiority_dice"))) req.pet->superiority_dice = item->valueint;
if ((item = cJSON_GetObjectItem(root, "superiority_dice_max"))) req.pet->superiority_dice_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "superiority_dice_size"))) req.pet->superiority_dice_size = item->valueint;
if ((item = cJSON_GetObjectItem(root, "sneak_attack_dice"))) req.pet->sneak_attack_dice = item->valueint;
if ((item = cJSON_GetObjectItem(root, "arcane_recovery_used"))) req.pet->arcane_recovery_used = item->valueint;

cJSON *maneuvers_arr = cJSON_GetObjectItem(root, "maneuvers");
                if (maneuvers_arr && cJSON_IsArray(maneuvers_arr)) {
                    cJSON *maneuver_item = NULL;
                    cJSON_ArrayForEach(maneuver_item, maneuvers_arr) {
                        if (req.pet->maneuver_count < MAX_MANEUVERS_KNOWN) {
                            cJSON *id_item = cJSON_GetObjectItem(maneuver_item, "id");
                            if (id_item) {
                                req.pet->maneuvers[req.pet->maneuver_count].maneuver_id = (uint8_t)id_item->valueint;
                                req.pet->maneuver_count++;
                            }
                        }
                    }
                }

if ((item = cJSON_GetObjectItem(root, "ability_points"))) req.pet->ability_points = item->valueint;
                if ((item = cJSON_GetObjectItem(root, "ability_points_max"))) req.pet->ability_points_max = item->valueint;

cJSON_Delete(root);
ESP_LOGI(TAG, "Pet loaded: %s Lv.%d (skills=%d, perks=%d, spells=%d, maneuvers=%d)", 
req.pet->name, req.pet->level, req.pet->skill_count, req.pet->perk_count,
req.pet->spells_known_count, req.pet->maneuver_count);
}
}
}
break;

            case STORAGE_OP_LOAD_TABLES:
                break;
            }

            spi_bus_unlock();
        }
    }
}

void storage_save_pet_delta(uint8_t dirty_flags, pet_t *pet)
{
    if (g_storage_queue == NULL) return;

    storage_request_t req = {
        .operation = STORAGE_OP_SAVE_PET_DELTA,
        .dirty_flags = dirty_flags,
        .pet = pet
    };
    xQueueSend(g_storage_queue, &req, 0);
}

bool storage_load_pet(pet_t *pet)
{
    if (!g_mounted) return false;

    spi_bus_lock();

    FILE *f = fopen(MOUNT_POINT "/BRAIN/PET/pet_data.json", "r");
    if (!f) {
        spi_bus_unlock();
        return false;
    }

    char buf[512];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    buf[len] = '\0';
    fclose(f);

    spi_bus_unlock();

    cJSON *root = cJSON_Parse(buf);
    if (!root) return false;

cJSON *item;
if ((item = cJSON_GetObjectItem(root, "name"))) strncpy(pet->name, item->valuestring, PET_NAME_MAX_LEN - 1);
if ((item = cJSON_GetObjectItem(root, "level"))) pet->level = item->valueint;
if ((item = cJSON_GetObjectItem(root, "profession_level"))) pet->profession_level = item->valueint;
if ((item = cJSON_GetObjectItem(root, "exp"))) pet->exp = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "hp"))) pet->hp = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "hp_max"))) pet->hp_max = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "energy"))) pet->energy = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "str"))) pet->str = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "dex"))) pet->dex = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "con"))) pet->con = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "int"))) pet->intel = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "wis"))) pet->wis = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "cha"))) pet->cha = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "dp"))) pet->dp = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "enemies_killed"))) pet->enemies_killed = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "lives"))) pet->lives = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "hp_rest_threshold"))) pet->rest.hp_rest_threshold = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "recovery_chance"))) pet->rest.recovery_chance = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "base_ac"))) pet->combat.base_ac = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "damage_dice"))) pet->combat.damage_dice = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "damage_bonus"))) pet->combat.damage_bonus = item->valueint;
if ((item = cJSON_GetObjectItem(root, "dice_count"))) pet->combat.dice_count = item->valueint;

cJSON *skills_arr = cJSON_GetObjectItem(root, "skills");
if (skills_arr && cJSON_IsArray(skills_arr)) {
    cJSON *skill_item = NULL;
    cJSON_ArrayForEach(skill_item, skills_arr) {
        if (pet->skill_count < MAX_SKILLS) {
            cJSON *id_item = cJSON_GetObjectItem(skill_item, "id");
            cJSON *uses_item = cJSON_GetObjectItem(skill_item, "uses");
            if (id_item) {
                pet->skills[pet->skill_count].skill_id = (uint8_t)id_item->valueint;
                pet->skills[pet->skill_count].uses_remaining = uses_item ? (uint8_t)uses_item->valueint : 3;
                pet->skills[pet->skill_count].uses_max = 3;
                pet->skill_count++;
            }
        }
    }
}

cJSON *perks_arr = cJSON_GetObjectItem(root, "perks");
if (perks_arr && cJSON_IsArray(perks_arr)) {
cJSON *perk_item = NULL;
cJSON_ArrayForEach(perk_item, perks_arr) {
if (pet->perk_count < MAX_PERKS) {
cJSON *id_item = cJSON_GetObjectItem(perk_item, "id");
if (id_item) {
pet->perks[pet->perk_count].id = (uint8_t)id_item->valueint;
pet->perk_count++;
}
}
}
}

cJSON *spell_slots_arr = cJSON_GetObjectItem(root, "spell_slots");
if (spell_slots_arr && cJSON_IsArray(spell_slots_arr)) {
uint8_t idx = 0;
cJSON *slot_item = NULL;
cJSON_ArrayForEach(slot_item, spell_slots_arr) {
if (idx < 9) {
pet->spell_slots.slots[idx++] = (uint8_t)slot_item->valueint;
}
}
}

cJSON *spell_slots_max_arr = cJSON_GetObjectItem(root, "spell_slots_max");
if (spell_slots_max_arr && cJSON_IsArray(spell_slots_max_arr)) {
uint8_t idx = 0;
cJSON *slot_item = NULL;
cJSON_ArrayForEach(slot_item, spell_slots_max_arr) {
if (idx < 9) {
pet->spell_slots_max[idx++] = (uint8_t)slot_item->valueint;
}
}
}

cJSON *spells_known_arr = cJSON_GetObjectItem(root, "spells_known");
if (spells_known_arr && cJSON_IsArray(spells_known_arr)) {
cJSON *spell_item = NULL;
cJSON_ArrayForEach(spell_item, spells_known_arr) {
if (pet->spells_known_count < MAX_SPELLS_KNOWN) {
cJSON *id_item = cJSON_GetObjectItem(spell_item, "id");
cJSON *level_item = cJSON_GetObjectItem(spell_item, "level");
if (id_item) {
strncpy(pet->spells_known[pet->spells_known_count].id, id_item->valuestring, sizeof(pet->spells_known[0].id) - 1);
pet->spells_known[pet->spells_known_count].id[sizeof(pet->spells_known[0].id) - 1] = '\0';
pet->spells_known[pet->spells_known_count].level = level_item ? (uint8_t)level_item->valueint : 0;
pet->spells_known_count++;
}
}
}
}

if ((item = cJSON_GetObjectItem(root, "cantrips_known"))) pet->cantrips_known = item->valueint;
if ((item = cJSON_GetObjectItem(root, "cantrips_max"))) pet->cantrips_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "action_surge_uses"))) pet->action_surge_uses = item->valueint;
if ((item = cJSON_GetObjectItem(root, "action_surge_max"))) pet->action_surge_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "indomitable_uses"))) pet->indomitable_uses = item->valueint;
if ((item = cJSON_GetObjectItem(root, "indomitable_max"))) pet->indomitable_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "second_wind_uses"))) pet->second_wind_uses = item->valueint;
if ((item = cJSON_GetObjectItem(root, "superiority_dice"))) pet->superiority_dice = item->valueint;
if ((item = cJSON_GetObjectItem(root, "superiority_dice_max"))) pet->superiority_dice_max = item->valueint;
if ((item = cJSON_GetObjectItem(root, "superiority_dice_size"))) pet->superiority_dice_size = item->valueint;
if ((item = cJSON_GetObjectItem(root, "sneak_attack_dice"))) pet->sneak_attack_dice = item->valueint;
if ((item = cJSON_GetObjectItem(root, "arcane_recovery_used"))) pet->arcane_recovery_used = item->valueint;

cJSON *maneuvers_arr = cJSON_GetObjectItem(root, "maneuvers");
    if (maneuvers_arr && cJSON_IsArray(maneuvers_arr)) {
        cJSON *maneuver_item = NULL;
        cJSON_ArrayForEach(maneuver_item, maneuvers_arr) {
            if (pet->maneuver_count < MAX_MANEUVERS_KNOWN) {
                cJSON *id_item = cJSON_GetObjectItem(maneuver_item, "id");
                if (id_item) {
                    pet->maneuvers[pet->maneuver_count].maneuver_id = (uint8_t)id_item->valueint;
                    pet->maneuver_count++;
                }
            }
        }
    }

if ((item = cJSON_GetObjectItem(root, "ability_points"))) pet->ability_points = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "ability_points_max"))) pet->ability_points_max = item->valueint;

uint32_t saved_salt = 0;
    if ((item = cJSON_GetObjectItem(root, "dna_salt"))) saved_salt = (uint32_t)item->valueint;

    pet->energy_max = MAX_ENERGY;
    pet->is_alive = true;

    cJSON_Delete(root);

    storage_load_dna_codes_only(&pet->dna);

    if (saved_salt != 0) {
        pet->dna.salt = saved_salt;
        ESP_LOGI(TAG, "Using saved salt: 0x%08lX", (unsigned long)pet->dna.salt);
    } else {
        pet->dna.salt = esp_random();
        ESP_LOGI(TAG, "Generated new salt: 0x%08lX", (unsigned long)pet->dna.salt);
    }

    storage_derive_dna_stats(&pet->dna);

    ESP_LOGI(TAG, "Pet loaded: %s Lv.%d (salt=0x%08lX, skills=%d, perks=%d)", 
             pet->name, pet->level, (unsigned long)pet->dna.salt, pet->skill_count, pet->perk_count);

    return true;
}

bool storage_load_tables(uint32_t *exp_table, uint16_t *hp_bonus)
{
    for (int i = 0; i < 100; i++) {
        float linear = 50.0f + (50.0f * (float)(i + 1));
        float exp_part = 50.0f * (powf(1.15f, (float)(i + 1)) - 1.0f);
        exp_table[i] = (uint32_t)(linear + exp_part);
        hp_bonus[i] = 5 + 5 * (i / 2);
    }
    return true;
}

static char *g_professions_json = NULL;
static char *g_enemies_json = NULL;
static char *g_config_json = NULL;

static char* load_json_file(const char *path)
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

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        spi_bus_unlock();
        return NULL;
    }

    size_t read_size = fread(buffer, 1, size, f);
    buffer[read_size] = '\0';
    fclose(f);
    spi_bus_unlock();

    return buffer;
}

bool storage_load_game_tables(void)
{
    if (g_professions_json && g_enemies_json && g_config_json) {
        return true;
    }

    if (!g_professions_json) {
        g_professions_json = load_json_file(MOUNT_POINT "/DATA/TABLES/professions.json");
        if (g_professions_json) {
            ESP_LOGI(TAG, "Professions loaded");
        }
    }

    if (!g_enemies_json) {
        g_enemies_json = load_json_file(MOUNT_POINT "/DATA/TABLES/enemies.json");
        if (g_enemies_json) {
            ESP_LOGI(TAG, "Enemies loaded");
        }
    }

    if (!g_config_json) {
        g_config_json = load_json_file(MOUNT_POINT "/DATA/TABLES/config.json");
        if (g_config_json) {
            ESP_LOGI(TAG, "Config loaded");
        }
    }

    return (g_professions_json != NULL);
}

bool storage_apply_profession_data(pet_t *pet, uint8_t profession_id)
{
    if (!storage_load_game_tables() || !g_professions_json) {
        pet->rest.hp_rest_threshold = 60;
        pet->rest.recovery_chance = 75;
        pet->combat.base_ac = 12;
        pet->combat.damage_dice = 15;
        pet->combat.damage_bonus = 1;
        pet->combat.dice_count = 1;
        pet->hp_max = 20;
        pet->hp = 20;
        pet->energy_max = 10;
        pet->energy = 10;
        return false;
    }

    cJSON *root = cJSON_Parse(g_professions_json);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse professions.json");
        return false;
    }

    cJSON *professions = cJSON_GetObjectItem(root, "professions");
    if (!professions) {
        cJSON_Delete(root);
        return false;
    }

    cJSON *prof = NULL;
    cJSON_ArrayForEach(prof, professions) {
        cJSON *id_item = cJSON_GetObjectItem(prof, "id");
        if (id_item && id_item->valueint == profession_id) {
            cJSON *threshold = cJSON_GetObjectItem(prof, "hp_rest_threshold");
            cJSON *chance = cJSON_GetObjectItem(prof, "recovery_chance");

            if (threshold) pet->rest.hp_rest_threshold = threshold->valueint;
            else pet->rest.hp_rest_threshold = 60;

            if (chance) pet->rest.recovery_chance = chance->valueint;
            else pet->rest.recovery_chance = 75;

            cJSON *base_ac = cJSON_GetObjectItem(prof, "base_ac");
            cJSON *damage_dice = cJSON_GetObjectItem(prof, "damage_dice");
            cJSON *damage_bonus = cJSON_GetObjectItem(prof, "damage_bonus");
            cJSON *dice_count = cJSON_GetObjectItem(prof, "dice_count");
            cJSON *base_hp = cJSON_GetObjectItem(prof, "base_hp");
            cJSON *base_energy = cJSON_GetObjectItem(prof, "base_energy");

            if (base_ac) pet->combat.base_ac = base_ac->valueint;
            else pet->combat.base_ac = 12;

            if (damage_dice) pet->combat.damage_dice = damage_dice->valueint;
            else pet->combat.damage_dice = 15;

            if (damage_bonus) pet->combat.damage_bonus = damage_bonus->valueint;
            else pet->combat.damage_bonus = 1;

            if (dice_count) pet->combat.dice_count = dice_count->valueint;
            else pet->combat.dice_count = 1;

            if (base_hp) pet->hp_max = base_hp->valueint;
            else pet->hp_max = 20;
            pet->hp = pet->hp_max;

            if (base_energy) pet->energy_max = base_energy->valueint;
            else pet->energy_max = 10;
            pet->energy = pet->energy_max;

            cJSON_Delete(root);
            ESP_LOGI(TAG, "Applied profession %d: HP=%d, EN=%d, AC=%d, dmg=%dd%d+%d",
                     profession_id, pet->hp_max, pet->energy_max,
                     pet->combat.base_ac, pet->combat.dice_count, pet->combat.damage_dice, pet->combat.damage_bonus);
            return true;
        }
    }

    cJSON_Delete(root);
    pet->rest.hp_rest_threshold = 60;
    pet->rest.recovery_chance = 75;
    pet->combat.base_ac = 12;
    pet->combat.damage_dice = 15;
    pet->combat.damage_bonus = 1;
    pet->combat.dice_count = 1;
    pet->hp_max = 20;
    pet->hp = 20;
    pet->energy_max = 10;
    pet->energy = 10;
    return false;
}

bool storage_get_enemy_data(uint8_t enemy_id, uint8_t pet_level, enemy_t *enemy)
{
    if (enemy == NULL) {
        ESP_LOGE(TAG, "storage_get_enemy_data: enemy is NULL");
        return false;
    }

    if (!storage_load_game_tables() || !g_enemies_json) {
        ESP_LOGW(TAG, "storage_get_enemy_data: enemies.json not loaded");
        return false;
    }

    cJSON *root = cJSON_Parse(g_enemies_json);
    if (!root) {
        ESP_LOGE(TAG, "storage_get_enemy_data: JSON parse failed");
        return false;
    }

    cJSON *enemies = cJSON_GetObjectItem(root, "enemies");
    if (!enemies) {
        ESP_LOGE(TAG, "storage_get_enemy_data: 'enemies' array not found");
        cJSON_Delete(root);
        return false;
    }

    cJSON *enemy_data = NULL;
    cJSON_ArrayForEach(enemy_data, enemies) {
        cJSON *id_item = cJSON_GetObjectItem(enemy_data, "id");
        if (id_item && id_item->valueint == enemy_id) {
            cJSON *name = cJSON_GetObjectItem(enemy_data, "name");
            cJSON *base_hp = cJSON_GetObjectItem(enemy_data, "base_hp");
            cJSON *hp_per_level = cJSON_GetObjectItem(enemy_data, "hp_per_level");
            cJSON *base_ac = cJSON_GetObjectItem(enemy_data, "base_ac");
            cJSON *ac_per_level = cJSON_GetObjectItem(enemy_data, "ac_per_level");
cJSON *damage_dice = cJSON_GetObjectItem(enemy_data, "damage_dice");
cJSON *damage_bonus = cJSON_GetObjectItem(enemy_data, "damage_bonus");
cJSON *damage_per_level = cJSON_GetObjectItem(enemy_data, "damage_per_level");
cJSON *attack_bonus = cJSON_GetObjectItem(enemy_data, "attack_bonus");
cJSON *exp_base = cJSON_GetObjectItem(enemy_data, "exp_base");
cJSON *exp_per_level = cJSON_GetObjectItem(enemy_data, "exp_per_level");

if (name) strncpy(enemy->name, name->valuestring, ENEMY_NAME_MAX_LEN - 1);
else snprintf(enemy->name, ENEMY_NAME_MAX_LEN, "Enemy%d", enemy_id);
enemy->name[ENEMY_NAME_MAX_LEN - 1] = '\0';

uint16_t hp_base = base_hp ? base_hp->valueint : 20;
uint16_t hp_pl = hp_per_level ? hp_per_level->valueint : 10;
enemy->hp_max = hp_base + (pet_level * hp_pl);
enemy->hp = enemy->hp_max;

uint8_t ac_base = base_ac ? base_ac->valueint : 10;
uint8_t ac_pl = ac_per_level ? ac_per_level->valueint : 0;
enemy->ac = ac_base + ((pet_level / 5) * ac_pl);
if (enemy->ac > 20) enemy->ac = 20;

enemy->damage_dice = damage_dice ? damage_dice->valueint : 6;
enemy->damage_bonus = damage_bonus ? damage_bonus->valueint : 0;
uint8_t dmg_pl = damage_per_level ? damage_per_level->valueint : 0;
enemy->damage_bonus += (pet_level / 5) * dmg_pl;

enemy->attack_bonus = attack_bonus ? attack_bonus->valueint : 0;

uint16_t avg_damage = (enemy->damage_dice / 2) + 1 + enemy->damage_bonus;
uint16_t exp_from_stats = (enemy->hp_max * enemy->ac * avg_damage) / 20;
uint16_t exp_b = exp_base ? exp_base->valueint : 20;
uint16_t exp_pl = exp_per_level ? exp_per_level->valueint : 5;
enemy->exp_reward = exp_b + (pet_level * exp_pl) + exp_from_stats;

            enemy->level = pet_level;
            enemy->alive = true;

            cJSON_Delete(root);
            ESP_LOGI(TAG, "Enemy %d loaded: %s HP=%d AC=%d dmg=%dd%d+%d exp=%d",
                     enemy_id, enemy->name, enemy->hp_max, enemy->ac,
                     1, enemy->damage_dice, enemy->damage_bonus, enemy->exp_reward);
            return true;
        }
    }

    cJSON_Delete(root);
    return false;
}

uint8_t storage_get_random_enemy_id(uint8_t pet_level)
{
    if (!storage_load_game_tables() || !g_enemies_json) {
        ESP_LOGW(TAG, "get_random_enemy: enemies.json not loaded");
        return 0;
    }

    cJSON *root = cJSON_Parse(g_enemies_json);
    if (!root) {
        ESP_LOGE(TAG, "get_random_enemy: JSON parse failed");
        return 0;
    }

    cJSON *tiers = cJSON_GetObjectItem(root, "enemy_tiers");
    if (!tiers) {
        ESP_LOGE(TAG, "get_random_enemy: 'enemy_tiers' not found");
        cJSON_Delete(root);
        return 0;
    }

    uint8_t valid_tier = 1;
    cJSON *tier = NULL;
    cJSON_ArrayForEach(tier, tiers) {
        cJSON *min_lvl = cJSON_GetObjectItem(tier, "min_level");
        cJSON *max_lvl = cJSON_GetObjectItem(tier, "max_level");
        if (min_lvl && max_lvl) {
            if (pet_level >= min_lvl->valueint && pet_level <= max_lvl->valueint) {
                cJSON *tier_item = cJSON_GetObjectItem(tier, "tier");
                if (tier_item) {
                    valid_tier = tier_item->valueint;
                }
                break;
            }
        }
    }

    ESP_LOGI(TAG, "Pet level %d -> tier %d", pet_level, valid_tier);

    cJSON *enemies = cJSON_GetObjectItem(root, "enemies");
    if (!enemies) {
        ESP_LOGE(TAG, "get_random_enemy: 'enemies' not found");
        cJSON_Delete(root);
        return 0;
    }

    uint8_t candidates[16];
    uint8_t count = 0;

    cJSON *enemy_data = NULL;
    cJSON_ArrayForEach(enemy_data, enemies) {
        cJSON *tier_item = cJSON_GetObjectItem(enemy_data, "tier");
        if (tier_item && tier_item->valueint == valid_tier) {
            cJSON *id_item = cJSON_GetObjectItem(enemy_data, "id");
            if (id_item && count < 16) {
                candidates[count++] = id_item->valueint;
            }
        }
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Found %d enemies for tier %d", count, valid_tier);

    if (count == 0) {
        ESP_LOGW(TAG, "No enemies found for tier %d", valid_tier);
        return 0;
    }

    uint8_t selected = candidates[esp_random() % count];
    ESP_LOGI(TAG, "Selected enemy id: %d", selected);
    return selected;
}

bool storage_file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

esp_err_t storage_save_file(const char *path, const uint8_t *data, size_t len)
{
    char dir_path[128];
    strncpy(dir_path, path, sizeof(dir_path) - 1);

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(dir_path, 0755);
    }

    spi_bus_lock();

    FILE *f = fopen(path, "w");
    if (!f) {
        spi_bus_unlock();
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    spi_bus_unlock();

    if (written != len) {
        ESP_LOGE(TAG, "Write failed: %zu of %zu bytes", written, len);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Saved %s (%zu bytes)", path, len);
    return ESP_OK;
}

esp_err_t storage_delete_file(const char *path)
{
spi_bus_lock();
int result = remove(path);
spi_bus_unlock();

if (result == 0) {
ESP_LOGI(TAG, "Deleted %s", path);
return ESP_OK;
}
return ESP_FAIL;
}

bool storage_load_dna_codes_only(dna_t *dna)
{
    if (dna == NULL) {
        ESP_LOGE(TAG, "storage_load_dna_codes_only: dna is NULL");
        return false;
    }

    spi_bus_lock();

    FILE *f = fopen(MOUNT_POINT "/DATA/DNA/pet_dna.json", "r");
    if (!f) {
        spi_bus_unlock();
        ESP_LOGW(TAG, "DNA file not found, using default codes");
        const char *default_codes[DNA_STAT_COUNT] = {
            "A3fK9x", "B7mP2q", "C1nL8w", "D5hR4t", "E9kM6y", "F2jS7z"
        };
        for (int i = 0; i < DNA_STAT_COUNT; i++) {
            strncpy(dna->codes[i], default_codes[i], DNA_CODE_LEN - 1);
            dna->codes[i][DNA_CODE_LEN - 1] = '\0';
        }
        return false;
    }

    char buf[256];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    buf[len] = '\0';
    fclose(f);

    spi_bus_unlock();

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGE(TAG, "storage_load_dna_codes_only: JSON parse failed");
        return false;
    }

    cJSON *item;
    const char *stat_keys[] = {"str", "dex", "con", "int", "wis", "cha"};

    for (int i = 0; i < DNA_STAT_COUNT; i++) {
        item = cJSON_GetObjectItem(root, stat_keys[i]);
        if (item && item->valuestring) {
            strncpy(dna->codes[i], item->valuestring, DNA_CODE_LEN - 1);
            dna->codes[i][DNA_CODE_LEN - 1] = '\0';
        }
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "DNA codes loaded: %s %s %s %s %s %s",
             dna->codes[0], dna->codes[1], dna->codes[2],
             dna->codes[3], dna->codes[4], dna->codes[5]);
    return true;
}

void storage_derive_dna_stats(dna_t *dna)
{
    if (dna == NULL) {
        return;
    }

    dna_generate_hash(dna);

    dna_derive_all_stats(dna, dna->base_stats);

    for (uint8_t i = 0; i < DNA_STAT_COUNT; i++) {
        dna->caps[i] = dna->base_stats[i] + DNA_CAP_OFFSET;
    }

    ESP_LOGI(TAG, "DNA stats derived: STR=%d DEX=%d CON=%d INT=%d WIS=%d CHA=%d",
             dna->base_stats[0], dna->base_stats[1], dna->base_stats[2],
             dna->base_stats[3], dna->base_stats[4], dna->base_stats[5]);
}

const char* storage_get_professions_json(void)
{
    return g_professions_json;
}

const char* storage_get_game_tables_json(void)
{
    static char *g_game_tables_json = NULL;
    if (!g_game_tables_json) {
        g_game_tables_json = load_json_file(MOUNT_POINT "/DATA/game_tables.json");
    }
    return g_game_tables_json;
}
