#include "storage_task.h"
#include "mistolito.h"
#include "spi_bus.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

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
                    fprintf(f, "{\"name\":\"%s\",\"level\":%d,\"exp\":%lu,\"hp\":%d,\"hp_max\":%d,\"energy\":%d,\"profession\":%d,\"str\":%d,\"dex\":%d,\"con\":%d,\"int\":%d,\"wis\":%d,\"cha\":%d,\"dp\":%lu,\"enemies_killed\":%lu,\"lives\":%d}\n",
                    req.pet->name, req.pet->level, (unsigned long)req.pet->exp,
                    req.pet->hp, req.pet->hp_max, req.pet->energy, req.pet->profession,
                    req.pet->str, req.pet->dex, req.pet->con,
                    req.pet->intel, req.pet->wis, req.pet->cha,
                    (unsigned long)req.pet->dp, (unsigned long)req.pet->enemies_killed, req.pet->lives);
                    fclose(f);
                    ESP_LOGI(TAG, "Pet saved (dirty=0x%02X)", req.dirty_flags);
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

                        cJSON_Delete(root);
                        ESP_LOGI(TAG, "Pet loaded: %s Lv.%d", req.pet->name, req.pet->level);
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

    pet->energy_max = MAX_ENERGY;
    pet->is_alive = true;

    cJSON_Delete(root);
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
