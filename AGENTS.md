# AGENTS.md - MistolitoRPG

This document provides guidance for agentic coding agents working in this repository.

## Project Overview

MistolitoRPG is a fantasy life simulator for ESP32-S3 that combines virtual pet mechanics with tabletop RPG systems (SRD 5.1) and AI-driven procedural narrative generation. The project is currently in the documentation and design phase.

## Build Commands

This is an embedded systems project targeting ESP32-S3. No build system is currently implemented.

Expected build system: ESP-IDF with CMake

```bash
# Expected commands (once implemented)
idf.py build              # Build the project
idf.py flash              # Flash to device
idf.py monitor            # Serial monitor
idf.py -p PORT flash      # Flash to specific port
idf.py fullclean          # Clean build artifacts
```

## Test Commands

No tests are currently implemented. When tests are added:

```bash
# Expected unit test commands
idf.py test                                    # Run all tests
idf.py test -T "test_component_name"           # Run specific test component
idf.py test -T "test_component_name" -n "test_name"  # Run single test
```

## Code Style Guidelines

### Language and Framework

- **Primary Language:** C (embedded C11)
- **Framework:** ESP-IDF v5.x
- **UI Framework:** LVGL v9.x (con widget lv_animimg para sprites)

### Imports and Includes

```c
// Order of includes:
// 1. ESP-IDF system headers
#include "esp_log.h"
#include "esp_system.h"

// 2. FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 3. Third-party library headers (alphabetically)
#include "cJSON.h"
#include "lvgl.h"

// 4. Project headers (alphabetically)
#include "brain_engine.h"
#include "dna_system.h"
```

### Formatting

- Indentation: 4 spaces (no tabs)
- Max line length: 120 characters
- Braces: Allman style (opening brace on new line)
- Pointer declaration: `int *ptr` (asterisk with variable name)

```c
// Correct function declaration
static esp_err_t init_sensor(void)
{
    esp_err_t ret = ESP_OK;
    sensor_config_t config = {
        .mode = SENSOR_MODE_ACTIVE,
        .sample_rate = 100,
    };
    return ret;
}
```

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Functions | snake_case | `brain_process_tick()` |
| Variables | snake_case | `hunger_level` |
| Constants | UPPER_SNAKE_CASE | `MAX_HP_VALUE` |
| Structs | snake_case with `_t` suffix | `dna_config_t` |
| Enums | UPPER_SNAKE_CASE | `SENSOR_STATE_ACTIVE` |
| Macros | UPPER_SNAKE_CASE | `DNA_HASH_SIZE` |
| Files | snake_case | `brain_engine.c` |

### Types

- Use `uint8_t`, `uint16_t`, `uint32_t`, `int8_t`, `int16_t`, `int32_t` from `<stdint.h>`
- Use `size_t` for sizes and counts
- Use `esp_err_t` for ESP-IDF return values
- Boolean type: `bool` with `true`/`false`

### Error Handling

```c
esp_err_t ret = some_function();
if (ret != ESP_OK)
{
    ESP_LOGE(TAG, "Function failed: %s", esp_err_to_name(ret));
    return ret;
}

// Use GOTO for cleanup in complex functions
esp_err_t ret = ESP_OK;
char *buffer = NULL;

buffer = malloc(BUFFER_SIZE);
if (buffer == NULL)
{
    ret = ESP_ERR_NO_MEM;
    goto cleanup;
}

// ... function logic ...

cleanup:
    free(buffer);
    return ret;
```

### Memory Management

- **Critical:** PSRAM is 8MB, internal SRAM is 512KB
- Large buffers MUST use PSRAM: `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`
- Small frequent allocations: use internal RAM
- Always check allocation results
- Use `ESP_LOGx` for memory debugging

### Logging

```c
static const char *TAG = "module_name";

ESP_LOGV(TAG, "Verbose debug info");  // Verbose
ESP_LOGD(TAG, "Debug info: %d", val); // Debug
ESP_LOGI(TAG, "Info message");        // Info
ESP_LOGW(TAG, "Warning: low memory"); // Warning
ESP_LOGE(TAG, "Error: %s", err);      // Error
```

## Project Architecture

### Core Components

| Component | Purpose | Key Libraries |
|-----------|---------|---------------|
| Brain Engine | Decision vector engine | `esp-dsp` (SIMD) |
| Fractal Memory | Vector-based storage on SD | `sdmmc`, `vfs` |
| Bridge | AI communication | `esp_http_client` |
| HUD | User interface | `LVGL`, `esp_lcd` |
| DNA System | Identity/genetics | `mbedtls` (SHA-256) |

### Memory Hierarchy (SD Card)

```
/BRAIN/
    /SKILLS/[vector_hex]/...    # Abilities and skills
    /WORLD/[vector_hex]/...     # World state and biomes
    /HISTORY/[vector_hex]/...   # Life log entries
    /MEM/[vector_hex]/...       # General memories
```

### Hardware Pin Reference

- I2C Bus: SCL=GPIO47, SDA=GPIO48 (IMU and Touch)
- SPI Bus: MOSI=GPIO38, SCLK=GPIO39 (LCD and SD)
- LCD: CS=GPIO45, DC=GPIO42, BL=GPIO1
- SD Card: CS=GPIO41, MISO=GPIO40
- Camera: DVP interface on GPIOs 2-17

## SRD 5.1 Integration

- Attributes: Strength, Dexterity, Constitution, Intelligence, Wisdom, Charisma
- Resolution: d20 rolls for actions
- DC modifiers based on world theme (Magical, Technological, Epic)

## Communication Protocol

- Embeddings: Generated externally via API (Gemini/OpenAI)
- Narrative: JSON payloads with context layers
- Security: HTTPS with mbedtls SSL verification

## Documentation Standards

All documentation is in Spanish. Code comments should be in English.

## Important Notes

- SPI bus is shared between LCD and SD card - manage CS states carefully
- Backlight PWM on GPIO1 for day/night cycle
- HP system: Pet death at 0 HP triggers database reset
- Offline mode: Accumulates Deity Points (DP), pauses narrative
