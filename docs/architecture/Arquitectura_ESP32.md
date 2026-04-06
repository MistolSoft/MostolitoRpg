# Arquitectura del ESP32: MistolitoRPG

Este documento describe la arquitectura de software del firmware que ejecuta en el ESP32-S3, incluyendo memoria, flujo de datos, tareas FreeRTOS y organización de componentes.

---

## 1. Mapa de Memoria

### 1.1 Memoria Física

| Tipo | Tamaño | Uso Principal |
|------|--------|---------------|
| SRAM Interna | 512 KB | Stack, heap pequeño, datos críticos |
| PSRAM | 8 MB | Buffers LVGL, vectores, assets |
| Flash | 16 MB | Firmware, particiones |
| SD Card | Variable | Memoria fractal, assets, ADN |

### 1.2 Asignación de Memoria

```
SRAM Interna (512 KB)
├── FreeRTOS Tasks Stacks
├── Heap pequeño (< 4KB por allocation)
├── Variables globales críticas
├── ISRs y callbacks
└── Buffers de comunicación UART/SPI

PSRAM (8 MB)
├── LVGL Framebuffers (~150 KB para 240x320)
├── Buffers de sprites
├── Vectores de estado (brain)
├── Índices de navegación fractal
├── Buffers JSON grandes
└── Cache de assets
```

### 1.3 Reglas de Asignación

```c
// SRAM - Objetos pequeños y frecuentes
widget_t *btn = malloc(sizeof(widget_t));  // OK (< 1KB)

// PSRAM - Objetos grandes
sprite_t *sprite = heap_caps_malloc(
    sizeof(sprite_t) + pixel_data_size,
    MALLOC_CAP_SPIRAM
);

// SIEMPRE verificar
if (sprite == NULL) {
    ESP_LOGE(TAG, "PSRAM allocation failed");
    return ESP_ERR_NO_MEM;
}
```

---

## 2. Sistema de Tareas FreeRTOS

### 2.1 Tareas Principales

```
┌─────────────────────────────────────────────────────────────┐
│                    FreeRTOS Scheduler                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐  Prioridad 5 (Alta)                       │
│  │  display_task│  ← LVGL refresh (30 FPS)                  │
│  └──────────────┘                                            │
│                                                              │
│  ┌──────────────┐  Prioridad 4                              │
│  │ combat_task  │  ← Loop de combate (tick-based)           │
│  └──────────────┘                                            │
│                                                              │
│  ┌──────────────┐  Prioridad 3                              │
│  │ brain_task   │  ← Decisión vectorial                     │
│  └──────────────┘                                            │
│                                                              │
│  ┌──────────────┐  Prioridad 2                              │
│  │ storage_task │  ← SD operations, navigation              │
│  └──────────────┘                                            │
│                                                              │
│  ┌──────────────┐  Prioridad 1 (Baja)                       │
│  │ bridge_task  │  ← HTTP requests to LLM                   │
│  └──────────────┘                                            │
│                                                              │
│  ┌──────────────┐  Prioridad 1                              │
│  │ sensors_task │  ← IMU/Touch polling                      │
│  └──────────────┘                                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Especificación de Tareas

| Tarea | Prioridad | Stack | Periodo | Función |
|-------|-----------|-------|---------|---------|
| `display_task` | 5 | 8 KB | 33 ms | Render LVGL, flush LCD |
| `combat_task` | 4 | 4 KB | 100 ms | Resolver turno, actualizar HP |
| `brain_task` | 3 | 6 KB | 500 ms | Calcular vector de estado, decisión |
| `storage_task` | 2 | 8 KB | Event-driven | Leer/escribir SD, navegar fractal |
| `bridge_task` | 1 | 12 KB | Event-driven | Requests HTTP, parse JSON |
| `sensors_task` | 1 | 4 KB | 50 ms | Poll IMU/Touch, generar eventos |

---

## 3. Flujo de Datos

### 3.1 Flujo de Boot

```
┌─────────────┐
│    Power    │
│     On      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  ESP-IDF    │
│  Init       │
└──────┬──────┘
       │
       ▼
┌─────────────┐     ┌──────────┐
│  Mount SD   │────►│ No SD?   │──► Error Screen
└──────┬──────┘     └──────────┘
       │
       ▼
┌─────────────┐     ┌──────────┐
│  Load DNA   │────►│ No DNA?  │──► Waiting Screen
└──────┬──────┘     └──────────┘
       │
       ▼
┌─────────────┐
│  Generate   │
│    Pet      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Init UI    │
│  (LVGL)     │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Start      │
│  Tasks      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Main      │
│   Loop      │
└─────────────┘
```

### 3.2 Flujo de Combate (MVP)

```
┌─────────────┐
│ combat_task │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────┐
│  ¿Pet tiene HP > 0?         │─── No ──► Death Sequence
└──────┬──────────────────────┘
       │ Sí
       ▼
┌─────────────────────────────┐
│  Calcular turno (tick)      │
│  - Tirada d20               │
│  - Modificador de atributo  │
│  - Aplicar daño             │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│  Actualizar estado          │
│  - HP actual                │
│  - EXP si victoria          │
│  - Level up check           │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│  Enviar evento a UI         │
│  (Queue: combat_events)     │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────────────────────┐
│  Wait (tick_duration)       │
└─────────────────────────────┘
```

### 3.3 Flujo de Decisión (Brain)

```
┌──────────────┐
│  brain_task  │
└──────┬───────┘
       │
       ▼
┌──────────────────────────────┐
│  Leer Vector de Estado       │
│  [Hambre, Energía, Social]   │
└──────┬───────────────────────┘
       │
       ▼
┌──────────────────────────────┐
│  Calcular Acción de Máximo   │
│  Deseo (argmax del vector)   │
└──────┬───────────────────────┘
       │
       ▼
┌──────────────────────────────┐
│  Generar Intención           │
│  - "Quiero comer"            │
│  - "Quiero dormir"           │
│  - "Quiero socializar"       │
└──────┬───────────────────────┘
       │
       ▼
┌──────────────────────────────┐
│  Enviar intención a storage  │
│  (Navegación fractal)        │
└──────────────────────────────┘
```

---

## 4. Comunicación Entre Tareas

### 4.1 Colas (Queues)

```c
// Cola de eventos de combate → UI
QueueHandle_t combat_events;

// Cola de eventos de sensores → Brain
QueueHandle_t sensor_events;

// Cola de requests → Bridge
QueueHandle_t bridge_requests;

// Cola de respuestas → Brain/UI
QueueHandle_t bridge_responses;
```

### 4.2 Tipos de Eventos

```c
typedef enum {
    EVENT_COMBAT_HIT,
    EVENT_COMBAT_MISS,
    EVENT_COMBAT_VICTORY,
    EVENT_COMBAT_DEATH,
    EVENT_LEVEL_UP,
    EVENT_SKILL_UNLOCK,
} combat_event_type_t;

typedef struct {
    combat_event_type_t type;
    int32_t value;           // Daño, EXP ganada, etc.
    uint32_t timestamp;
} combat_event_t;
```

### 4.3 Ejemplo de Uso

```c
// combat_task envía evento
combat_event_t evt = {
    .type = EVENT_COMBAT_HIT,
    .value = damage_dealt,
    .timestamp = esp_timer_get_time()
};
xQueueSend(combat_events, &evt, 0);

// display_task recibe evento
combat_event_t received;
if (xQueueReceive(combat_events, &received, pdMS_TO_TICKS(10))) {
    // Actualizar UI
    update_hp_bar(received.value);
}
```

---

## 5. Sistema de Archivos en SD

### 5.1 Estructura de Directorios

```
/SD_ROOT/
│
├── DNA/
│   └── pet_dna.json          # ADN del Pet actual
│
├── BRAIN/
│   ├── MEM/                  # Memoria fractal
│   │   └── [vector_hex]/
│   │       └── [vector_hex]/
│   │           └── memory.json
│   │
│   ├── SKILLS/               # Habilidades
│   │   └── [vector_hex]/
│   │       └── skill.json
│   │
│   ├── WORLD/                # Estado del mundo
│   │   └── [vector_hex]/
│   │       └── biome.json
│   │
│   └── HISTORY/              # Life log
│       └── [vector_hex]/
│           └── event.json
│
├── ASSETS/
│   ├── sprites/              # Sprites del Pet
│   ├── fonts/                # Fuentes
│   └── icons/                # Iconos HUD
│
└── CONFIG/
    └── settings.json         # Configuración del usuario
```

### 5.2 Formato de Archivos

**DNA (`pet_dna.json`):**
```json
{
    "id": "0x1a2b3c...",
    "attributes": {
        "str": 14,
        "dex": 12,
        "con": 16,
        "int": 10,
        "wis": 8,
        "cha": 11
    },
    "metabolism": {
        "hunger_delta": 0.02,
        "energy_delta": 0.015
    },
    "soft_caps": {
        "str_max": 18,
        "dex_max": 16
    }
}
```

**Memory (`memory.json`):**
```json
{
    "vector": [0.12, -0.34, 0.56, ...],
    "content": "Recuerdo de la primera caza",
    "timestamp": 1234567890,
    "importance": 0.85
}
```

---

## 6. Gestión del SPI Compartido

### 6.1 Problema

LCD y SD comparten el mismo bus SPI:
- MOSI = GPIO38
- SCLK = GPIO39
- LCD CS = GPIO45
- SD CS = GPIO41

### 6.2 Solución

```c
// Mutex para el bus SPI
SemaphoreHandle_t spi_bus_mutex;

// Antes de usar SPI
xSemaphoreTake(spi_bus_mutex, pdMS_TO_TICKS(100));
// Operación SPI
sd_read_file(...);
xSemaphoreGive(spi_bus_mutex);

// Configuración del bus SPI
spi_bus_config_t bus_cfg = {
    .mosi_io_num = 38,
    .miso_io_num = 40,  // Solo SD
    .sclk_io_num = 39,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4096,
};
spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
```

---

## 7. Componentes del Firmware

### 7.1 Estructura de Componente

```
components/
└── brain/
    ├── CMakeLists.txt
    ├── Kconfig
    ├── include/
    │   └── brain_engine.h
    ├── src/
    │   ├── brain_engine.c
    │   ├── brain_vectors.c
    │   └── brain_decision.c
    └── test/
        └── test_brain.c
```

### 7.2 Dependencias entre Componentes

```
┌─────────────┐
│    main     │
└──────┬──────┘
       │
       ├──► hud ──────► lvgl
       │
       ├──► combat ───► dna
       │         └────► brain
       │
       ├──► brain ────► storage
       │         └────► esp-dsp
       │
       ├──► storage ──► sdmmc
       │         └────► vfs
       │
       ├──► sensors ─► driver/imu
       │         └────► driver/touch
       │
       └──► bridge ──► esp_http_client
                 └────► cJSON
                 └────► mbedtls
```

---

## 8. Ciclo de Vida del Pet

### 8.1 Estados del Sistema

```
┌────────────────┐
│     INIT       │  Boot, carga ADN
└───────┬────────┘
        │
        ▼
┌────────────────┐
│     IDLE       │  Esperando combate, acumulando DP
└───────┬────────┘
        │
        ▼
┌────────────────┐
│    COMBAT      │  Combate activo
└───────┬────────┘
        │
        ├────► VICTORY ──► Level Up ──► IDLE
        │
        └────► DEATH ─────► RESET ──► IDLE
```

### 8.2 Reinicio tras Muerte

```c
void pet_death_sequence(void) {
    // 1. Animación de muerte
    hud_show_death_animation();
    
    // 2. Guardar DP acumulados
    uint32_t dp = get_accumulated_dp();
    storage_save_dp(dp);
    
    // 3. Reiniciar Pet
    pet_reset();
    
    // 4. Restaurar DP
    dp = storage_load_dp();
    set_deity_points(dp);
    
    // 5. Volver a idle
    system_state = STATE_IDLE;
}
```

---

## 9. Configuración de LVGL

### 9.1 Inicialización

```c
// Framebuffer en PSRAM
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

buf1 = heap_caps_malloc(
    LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t),
    MALLOC_CAP_SPIRAM
);

buf2 = heap_caps_malloc(
    LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t),
    MALLOC_CAP_SPIRAM
);

lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LV_HOR_RES_MAX * 10);
```

### 9.2 Tarea de Display

```c
void display_task(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(33); // ~30 FPS
    
    while (1) {
        lv_timer_handler();
        vTaskDelay(xDelay);
    }
}
```

---

## 10. Logs y Debug

### 10.1 Niveles de Log

```c
// Componentes con TAG separado
static const char *TAG_BRAIN = "BRAIN";
static const char *TAG_COMBAT = "COMBAT";
static const char *TAG_STORAGE = "STORAGE";

// Uso
ESP_LOGV(TAG_BRAIN, "Vector state: [%.2f, %.2f, %.2f]", v[0], v[1], v[2]);
ESP_LOGD(TAG_COMBAT, "Turn %d: Pet HP=%d, Enemy HP=%d", turn, pet_hp, enemy_hp);
ESP_LOGI(TAG_STORAGE, "Loaded DNA: %s", dna_id);
ESP_LOGW(TAG_BRAIN, "Low energy, entering rest mode");
ESP_LOGE(TAG_STORAGE, "Failed to mount SD: %s", esp_err_to_name(ret));
```

### 10.2 Debug por JTAG

```bash
# Iniciar OpenOCD
openocd -f board/esp32s3-builtin.cfg

# Conectar GDB
idf.py gdb

# Breakpoints
(gdb) break combat_task
(gdb) continue
```

---

## 11. Métricas de Memoria

### 11.1 Monitoreo

```c
void print_memory_stats(void) {
    ESP_LOGI("MEM", "Free internal: %lu bytes", 
             esp_get_free_heap_size());
    ESP_LOGI("MEM", "Free PSRAM: %lu bytes", 
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI("MEM", "Min free ever: %lu bytes", 
             esp_get_minimum_free_heap_size());
}
```

### 11.2 Límites Seguros

| Métrica | Límite | Acción |
|---------|--------|--------|
| Free SRAM < 50 KB | Crítico | Reiniciar tareas |
| Free PSRAM < 500 KB | Advertencia | Liberar caches |
| Stack watermark < 512 bytes | Crítico | Aumentar stack |

---

## 12. Resumen de Flujos

```
                    ┌─────────────┐
                    │    BOOT     │
                    └──────┬──────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ Load DNA/SD  │  │ Init LVGL    │  │ Start Tasks  │
└──────────────┘  └──────────────┘  └──────────────┘

                    ┌─────────────┐
                    │    RUN      │
                    └──────┬──────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ sensors_task │  │ brain_task   │  │ combat_task  │
│ (Poll IMU)   │  │ (Decisión)   │  │ (Turnos)     │
└──────┬───────┘  └──────┬───────┘  └──────┬───────┘
       │                 │                 │
       └────────────────┬┴─────────────────┘
                        │
                        ▼
                ┌──────────────┐
                │ display_task │
                │ (LVGL 30fps) │
                └──────────────┘
```
