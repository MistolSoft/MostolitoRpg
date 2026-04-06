#include "display_task.h"
#include "screens.h"
#include "events.h"
#include "game_coordinator.h"
#include "animation_loops.h"
#include "usb_init.h"
#include "storage_task.h"
#include "spi_bus.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DISPLAY";

TaskHandle_t g_display_task_handle = NULL;

#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define DISPLAY_TICK_MS 250

static char s_rx_buffer[1024];
static int s_rx_pos = 0;

static void handle_usb_commands(void)
{
    uint8_t byte;
    while (usb_read_byte(&byte, 10)) {
        if (byte == '\n' || byte == '\r') {
            if (s_rx_pos > 0) {
                s_rx_buffer[s_rx_pos] = '\0';
                ESP_LOGI(TAG, "USB RX: %s", s_rx_buffer);

                char response[512];
                usb_process_command(s_rx_buffer, response, sizeof(response));
                usb_write_response(response);

                s_rx_pos = 0;
            }
        } else if (s_rx_pos < (int)sizeof(s_rx_buffer) - 1) {
            s_rx_buffer[s_rx_pos++] = byte;
        }
    }
}

static bool check_boot_button(void)
{
    if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
            ESP_LOGI(TAG, "BOOT button pressed");

            while (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            return true;
        }
    }
    return false;
}

void display_task_start(void)
{
    ESP_LOGI(TAG, "Step 1a: screens_init...");
    screens_init();
    
    ESP_LOGI(TAG, "Step 1b: screens_load...");
    screens_load(SCREEN_INIT);

    ESP_LOGI(TAG, "Step 1c: gpio_config...");
    gpio_config_t boot_btn_cfg = {
        .pin_bit_mask = BIT64(BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&boot_btn_cfg);

    ESP_LOGI(TAG, "Step 1d: usb_init_driver...");
    usb_init_driver();
    ESP_LOGI(TAG, "Step 1e: display_task_start done");
}

void display_task(void *arg)
{
    ESP_LOGI(TAG, "Display task started");

    ESP_LOGI(TAG, "Step 1: calling display_task_start...");
    display_task_start();
    
    ESP_LOGI(TAG, "Step 2: calling anim_loops_init...");
    anim_loops_init();
    
    ESP_LOGI(TAG, "Step 3: checking USB files...");
    game_event_t evt;
    bool game_started = false;
    bool files_ok = usb_check_required_files();

    if (!files_ok) {
        char missing[256];
        usb_get_missing_files(missing, sizeof(missing));
        ESP_LOGI(TAG, "Missing files: %s", missing);
    }

    ESP_LOGI(TAG, "Waiting for START_LOOP command or BOOT button...");

    while (!game_started) {
        handle_usb_commands();

        if (usb_is_complete()) {
            if (usb_start_requested()) {
                ESP_LOGI(TAG, "USB START_LOOP received, starting game...");
                game_started = true;
                break;
            }
        }

        if (check_boot_button()) {
            if (usb_check_required_files()) {
                ESP_LOGI(TAG, "All files present, starting game...");
                game_started = true;
                break;
            } else {
                char missing[256];
                usb_get_missing_files(missing, sizeof(missing));
                ESP_LOGW(TAG, "Cannot start: missing %s", missing);
            }
        }

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    screens_load(SCREEN_GAME);
    usb_reset_state();

    if (g_coordinator_task_handle != NULL) {
        xTaskNotify(g_coordinator_task_handle, 1, eSetValueWithOverwrite);
    }

    ESP_LOGI(TAG, "Game started, entering main loop (4 FPS tick)");

    TickType_t last_tick = xTaskGetTickCount();
    const TickType_t tick_interval = pdMS_TO_TICKS(DISPLAY_TICK_MS);

    while (1) {
        while (events_receive(&evt, 0)) {
            anim_loops_process_event(&evt);
        }

        anim_loops_tick();

        game_snapshot_t *snap = game_coordinator_get_snapshot();
        if (snap) {
            SemaphoreHandle_t mutex = game_coordinator_get_mutex();
            if (mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                screens_update(snap);
                xSemaphoreGive(mutex);
            }
        }

        spi_bus_lock();
        lv_timer_handler();
        spi_bus_unlock();

        vTaskDelayUntil(&last_tick, tick_interval);
    }
}
