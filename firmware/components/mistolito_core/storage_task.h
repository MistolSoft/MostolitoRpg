#ifndef STORAGE_TASK_H
#define STORAGE_TASK_H

#include "mistolito.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define MOUNT_POINT "/sdcard"

#define STORAGE_OP_SAVE_PET_DELTA 1
#define STORAGE_OP_LOAD_PET 2
#define STORAGE_OP_LOAD_TABLES 3

typedef struct {
    uint8_t operation;
    uint8_t dirty_flags;
    pet_t *pet;
} storage_request_t;

#define STORAGE_QUEUE_SIZE 4

extern QueueHandle_t g_storage_queue;

void storage_task_start(void);
void storage_task(void *arg);
void storage_save_pet_delta(uint8_t dirty_flags, pet_t *pet);
bool storage_load_pet(pet_t *pet);
bool storage_load_tables(uint32_t *exp_table, uint16_t *hp_bonus);
bool storage_file_exists(const char *path);
esp_err_t storage_save_file(const char *path, const uint8_t *data, size_t len);
esp_err_t storage_delete_file(const char *path);

#endif
