#include "usb_init.h"
#include "storage_task.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "USB_INIT";

static const char *REQUIRED_FILES[] = {
    "/DATA/game_tables.json",
    NULL
};

static usb_file_transfer_t s_transfer = {0};
static uint8_t *s_file_buffer = NULL;
static usb_init_state_e s_state = USB_STATE_IDLE;
static bool s_start_loop_requested = false;

static int base64_decode_char(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static size_t base64_decode(const char *input, size_t input_len, uint8_t *output)
{
    if (input_len % 4 != 0) return 0;

    size_t output_len = (input_len / 4) * 3;
    if (input_len >= 1 && input[input_len - 1] == '=') output_len--;
    if (input_len >= 2 && input[input_len - 2] == '=') output_len--;

    size_t j = 0;
    for (size_t i = 0; i < input_len; i += 4) {
        int a = base64_decode_char(input[i]);
        int b = base64_decode_char(input[i + 1]);
        int c = (input[i + 2] == '=') ? 0 : base64_decode_char(input[i + 2]);
        int d = (input[i + 3] == '=') ? 0 : base64_decode_char(input[i + 3]);

        if (a < 0 || b < 0 || c < 0 || d < 0) return 0;

        output[j++] = (a << 2) | (b >> 4);
        if (input[i + 2] != '=') {
            output[j++] = ((b & 0x0F) << 4) | (c >> 2);
        }
        if (input[i + 3] != '=') {
            output[j++] = ((c & 0x03) << 6) | d;
        }
    }

    return j;
}

static esp_err_t handle_file_start(const char *params, char *response, size_t resp_len)
{
    char *saveptr;
    char params_copy[128];
    strncpy(params_copy, params, sizeof(params_copy) - 1);
    params_copy[sizeof(params_copy) - 1] = '\0';

    char *filename = strtok_r(params_copy, ":", &saveptr);
    char *size_str = strtok_r(NULL, ":", &saveptr);

    if (!filename || !size_str) {
        snprintf(response, resp_len, "ERROR:INVALID_PARAMS");
        return ESP_FAIL;
    }

    size_t file_size = (size_t)strtoul(size_str, NULL, 10);

    if (file_size == 0 || file_size > USB_FILE_BUFFER_SIZE) {
        snprintf(response, resp_len, "ERROR:INVALID_SIZE:%zu", file_size);
        return ESP_FAIL;
    }

    strncpy(s_transfer.filename, filename, sizeof(s_transfer.filename) - 1);
    s_transfer.filename[sizeof(s_transfer.filename) - 1] = '\0';
    s_transfer.total_size = file_size;
    s_transfer.received_size = 0;
    s_transfer.in_progress = true;
    s_state = USB_STATE_RECEIVING_FILE;

    if (s_file_buffer) {
        free(s_file_buffer);
        s_file_buffer = NULL;
    }

    s_file_buffer = malloc(file_size + 1);
    if (!s_file_buffer) {
        snprintf(response, resp_len, "ERROR:NO_MEMORY");
        return ESP_FAIL;
    }
    memset(s_file_buffer, 0, file_size + 1);

    snprintf(response, resp_len, "FILE_RECV_START:%s:%zu", filename, file_size);
    return ESP_OK;
}

static esp_err_t handle_file_data(const char *params, char *response, size_t resp_len)
{
    if (!s_transfer.in_progress) {
        snprintf(response, resp_len, "ERROR:NO_FILE_IN_PROGRESS");
        return ESP_FAIL;
    }

    size_t params_len = strlen(params);
    size_t decoded_len = base64_decode(params, params_len, s_file_buffer + s_transfer.received_size);

    if (decoded_len == 0) {
        snprintf(response, resp_len, "ERROR:BASE64_DECODE_FAILED");
        return ESP_FAIL;
    }

    s_transfer.received_size += decoded_len;

    if (s_transfer.received_size > s_transfer.total_size) {
        snprintf(response, resp_len, "ERROR:SIZE_OVERFLOW");
        s_transfer.in_progress = false;
        s_state = USB_STATE_ERROR;
        return ESP_FAIL;
    }

    int percent = (int)((s_transfer.received_size * 100) / s_transfer.total_size);
    snprintf(response, resp_len, "FILE_RECV_PROGRESS:%zu:%zu:%d%%", s_transfer.received_size, s_transfer.total_size, percent);
    return ESP_OK;
}

static esp_err_t handle_file_end(const char *params, char *response, size_t resp_len)
{
    (void)params;
    
    if (!s_transfer.in_progress) {
        snprintf(response, resp_len, "ERROR:NO_FILE_IN_PROGRESS");
        return ESP_FAIL;
    }

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, s_transfer.filename);

    esp_err_t ret = storage_save_file(full_path, s_file_buffer, s_transfer.received_size);
    if (ret != ESP_OK) {
        snprintf(response, resp_len, "ERROR:SAVE_FAILED:%s", s_transfer.filename);
        s_transfer.in_progress = false;
        s_state = USB_STATE_ERROR;
        return ESP_FAIL;
    }

    snprintf(response, resp_len, "FILE_RECV_OK:%s:%zu bytes saved", s_transfer.filename, s_transfer.received_size);

    s_transfer.in_progress = false;
    s_state = USB_STATE_IDLE;

    if (s_file_buffer) {
        free(s_file_buffer);
        s_file_buffer = NULL;
    }

    return ESP_OK;
}

static esp_err_t handle_status(const char *params, char *response, size_t resp_len)
{
    (void)params;
    char missing[256] = {0};
    usb_get_missing_files(missing, sizeof(missing));

    if (strlen(missing) == 0) {
        snprintf(response, resp_len, "STATUS:READY_FOR_INIT");
    } else {
        snprintf(response, resp_len, "STATUS:WAITING_FOR_FILES|Missing: %s", missing);
    }
    return ESP_OK;
}

static esp_err_t handle_init_complete(const char *params, char *response, size_t resp_len)
{
    (void)params;
    char missing[256] = {0};
    usb_get_missing_files(missing, sizeof(missing));

    if (strlen(missing) > 0) {
        snprintf(response, resp_len, "ERROR:MISSING_FILES:%s", missing);
        return ESP_FAIL;
    }

    s_state = USB_STATE_COMPLETE;
    snprintf(response, resp_len, "INIT_COMPLETE:All files received. Starting...");
    return ESP_OK;
}

static esp_err_t handle_list_files(const char *params, char *response, size_t resp_len)
{
    (void)params;
    snprintf(response, resp_len, "FILES:game_tables.json");
    return ESP_OK;
}

static esp_err_t handle_wipe(const char *params, char *response, size_t resp_len)
{
    (void)params;
    ESP_LOGI(TAG, "Wiping all data...");

    char path[128];
    snprintf(path, sizeof(path), "%s/DATA/game_tables.json", MOUNT_POINT);
    storage_delete_file(path);
    snprintf(path, sizeof(path), "%s/BRAIN/PET/pet_data.json", MOUNT_POINT);
    storage_delete_file(path);

    snprintf(response, resp_len, "WIPE_COMPLETE");
    return ESP_OK;
}

static esp_err_t handle_start_loop(const char *params, char *response, size_t resp_len)
{
    (void)params;
    ESP_LOGI(TAG, "Start loop requested");
    s_start_loop_requested = true;
    s_state = USB_STATE_COMPLETE;
    snprintf(response, resp_len, "START_LOOP:Starting game loop...");
    return ESP_OK;
}

esp_err_t usb_process_command(const char *cmd_line, char *response, size_t resp_len)
{
    if (strncmp(cmd_line, "CMD:", 4) != 0) {
        snprintf(response, resp_len, "ERROR:INVALID_FORMAT");
        return ESP_FAIL;
    }

    const char *cmd = cmd_line + 4;

    char *saveptr;
    char cmd_copy[256];
    strncpy(cmd_copy, cmd, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    char *command = strtok_r(cmd_copy, ":", &saveptr);
    const char *params = saveptr;

    if (!command) {
        snprintf(response, resp_len, "ERROR:NO_COMMAND");
        return ESP_FAIL;
    }

    if (strcmp(command, "FILE_START") == 0) {
        return handle_file_start(params, response, resp_len);
    } else if (strcmp(command, "FILE_DATA") == 0) {
        return handle_file_data(params, response, resp_len);
    } else if (strcmp(command, "FILE_END") == 0) {
        return handle_file_end(params, response, resp_len);
    } else if (strcmp(command, "STATUS") == 0) {
        return handle_status(params, response, resp_len);
    } else if (strcmp(command, "INIT_COMPLETE") == 0) {
        return handle_init_complete(params, response, resp_len);
    } else if (strcmp(command, "LIST_FILES") == 0) {
        return handle_list_files(params, response, resp_len);
    } else if (strcmp(command, "WIPE") == 0) {
        return handle_wipe(params, response, resp_len);
    } else if (strcmp(command, "START_LOOP") == 0) {
        return handle_start_loop(params, response, resp_len);
    } else {
        snprintf(response, resp_len, "ERROR:UNKNOWN_COMMAND:%s", command);
        return ESP_FAIL;
    }
}

void usb_init_driver(void)
{
    usb_serial_jtag_driver_config_t usb_config = {
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024,
    };
    usb_serial_jtag_driver_install(&usb_config);
}

bool usb_check_required_files(void)
{
    for (int i = 0; REQUIRED_FILES[i] != NULL; i++) {
        char full_path[128];
        snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, REQUIRED_FILES[i]);

        if (!storage_file_exists(full_path)) {
            return false;
        }
    }
    return true;
}

void usb_get_missing_files(char *buf, size_t buf_len)
{
    buf[0] = '\0';
    size_t pos = 0;

    for (int i = 0; REQUIRED_FILES[i] != NULL; i++) {
        char full_path[128];
        snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, REQUIRED_FILES[i]);

        if (!storage_file_exists(full_path)) {
            const char *filename = REQUIRED_FILES[i];
            if (strncmp(filename, "/", 1) == 0) {
                filename++;
            }

            if (pos > 0 && pos < buf_len - 2) {
                buf[pos++] = ',';
                buf[pos++] = ' ';
            }

            if (pos < buf_len - strlen(filename) - 1) {
                strcpy(buf + pos, filename);
                pos += strlen(filename);
            }
        }
    }

    buf[pos] = '\0';
}

bool usb_read_byte(uint8_t *byte, uint32_t timeout_ms)
{
    return usb_serial_jtag_read_bytes(byte, 1, pdMS_TO_TICKS(timeout_ms)) > 0;
}

void usb_write_response(const char *response)
{
    usb_serial_jtag_write_bytes((const uint8_t *)response, strlen(response), pdMS_TO_TICKS(100));
    usb_serial_jtag_write_bytes((const uint8_t *)"\n", 1, pdMS_TO_TICKS(100));
}

bool usb_is_complete(void)
{
    return s_state == USB_STATE_COMPLETE;
}

bool usb_start_requested(void)
{
    return s_start_loop_requested;
}

void usb_reset_state(void)
{
    s_state = USB_STATE_IDLE;
    s_start_loop_requested = false;
    s_transfer.in_progress = false;
    if (s_file_buffer) {
        free(s_file_buffer);
        s_file_buffer = NULL;
    }
}
