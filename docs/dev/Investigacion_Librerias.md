# Investigación de Librerías ESP-IDF para MVP MistolitoRPG

**Fecha:** 2026-04-03  
**Objetivo:** Identificar y documentar las librerías de ESP-IDF necesarias para implementar el MVP de combate básico.

---

## Resumen Ejecutivo

ESP-IDF proporciona soporte nativo para todos los módulos requeridos por el MVP. No se necesitan librerías externas adicionales excepto LVGL v8.3, que está disponible en el ESP Component Registry.

**Conclusión principal:** ESP-IDF tiene todo lo necesario. El proyecto puede implementarse usando exclusivamente ESP-IDF y LVGL.

---

## Módulo 1: Display/LCD (ST7789T3)

### Librería Nativa: `esp_lcd`

**Estado:** ✅ Disponible y documentado  
**Componente:** `esp_lcd`  
**Header:** `#include "esp_lcd_panel_io_spi.h"` y `#include "esp_lcd_panel_vendor.h"`

#### Características Clave

- **Driver ST7789 incluido:** ESP-IDF incluye driver nativo para ST7789
- **Arquitectura modular:** Separación entre plano de control y plano de datos
- **Soporte SPI:** Optimizado para LCDs con interfaz SPI
- **DMA integrado:** Transmisiones eficientes con DMA

#### API Principal

```c
// 1. Inicializar bus SPI
spi_bus_config_t buscfg = {
    .sclk_io_num = GPIO_SCLK,
    .mosi_io_num = GPIO_MOSI,
    .miso_io_num = GPIO_MISO,
    .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
};
spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

// 2. Crear handle IO para SPI
esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = GPIO_DC,
    .cs_gpio_num = GPIO_CS,
    .pclk_hz = LCD_PIXEL_CLOCK_HZ,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
};
esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

// 3. Crear panel ST7789
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = GPIO_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
    .bits_per_pixel = 16,
};
esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);

// 4. Inicializar y usar
esp_lcd_panel_reset(panel_handle);
esp_lcd_panel_init(panel_handle);
esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, color_data);
```

#### Pines del Proyecto (según AGENTS.md)

| Función | GPIO |
|---------|------|
| CS (LCD) | GPIO45 |
| DC | GPIO42 |
| BL (Backlight) | GPIO1 |
| MOSI | GPIO38 |
| SCLK | GPIO39 |
| MISO | GPIO40 |

#### Links de Documentación

- [LCD API General](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd/index.html)
- [SPI LCD](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd/spi_lcd.html)
- [SPI Master](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html)

---

## Módulo 2: LVGL UI (v8.3)

### Librería: `lvgl/lvgl` v8.3.0

**Estado:** ✅ Disponible en ESP Component Registry  
**Proveedor:** LVGL (componente oficial)  
**Instalación:** `idf.py add-dependency "lvgl/lvgl^8.3.0"`

#### Características Clave

- **Versión exacta:** v8.3.0 (la especificada en AGENTS.md)
- **Requisitos mínimos:** 64KB Flash, 16KB RAM (mínimo), 1/10 pantalla para buffer
- **Sin dependencias externas:** Portable y autosuficiente
- **Licencia:** MIT

#### Requisitos del Proyecto

- Resolución: 240x320 píxeles
- Color: RGB565 (16-bit)
- Buffer recomendado: 240x320/10 = 7680 píxeles ≈ 15KB

#### Integración con ESP-IDF

```c
#include "lvgl.h"

// Callback de flush para LCD
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

// Tick callback
static void lvgl_tick_cb(void *arg) {
    lv_tick_inc(1);
}

// Inicialización
lv_init();
lv_disp_draw_buf_init(&draw_buf, buf1, NULL, BUFFER_SIZE);
lv_disp_drv_init(&disp_drv);
disp_drv.flush_cb = lvgl_flush_cb;
lv_disp_drv_register(&disp_drv);
```

#### Links

- [ESP Component Registry - LVGL v8.3.0](https://components.espressif.com/components/lvgl/lvgl/versions/8.3.0)
- [Documentación LVGL v8](https://docs.lvgl.io/8.3/)

---

## Módulo 3: SD Card (SDMMC + VFS)

### Librería Nativa: `sdmmc` + `vfs`

**Estado:** ✅ Disponible y documentado  
**Componentes:** `sdmmc`, `vfs`, `fatfs`  
**Headers:** `#include "sdmmc_cmd.h"`, `#include "esp_vfs_fat.h"`

#### Arquitectura

El sistema de SD en ESP-IDF tiene tres capas:

1. **Host Layer:** Driver de hardware (SDMMC o SDSPI)
2. **Protocol Layer:** Manejo del protocolo SD
3. **VFS Layer:** Sistema de archivos virtual

#### API Principal

```c
// 1. Inicializar host SDMMC
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
host.flags = SDMMC_HOST_FLAG_1BIT;
host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
slot_config.width = 1;

sdmmc_host_init();
sdmmc_host_init_slot(host.slot, &slot_config);

// 2. Montar filesystem
esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
};
sdmmc_card_t *card = NULL;
esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

// 3. Usar con fopen/fread/fwrite estándar
FILE *f = fopen("/sdcard/dna.json", "r");
// ...
fclose(f);

// 4. Desmontar
esp_vfs_fat_sdcard_unmount("/sdcard", card);
```

#### Pines del Proyecto (según AGENTS.md)

| Función | GPIO |
|---------|------|
| CS (SD) | GPIO41 |
| MISO | GPIO40 |
| MOSI | GPIO38 |
| SCLK | GPIO39 |

**Nota:** El bus SPI es compartido entre LCD y SD. El manejo de CS debe ser cuidadoso.

#### Links de Documentación

- [SD/SDIO/MMC Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/sdmmc.html)
- [Virtual Filesystem](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/vfs.html)

---

## Módulo 4: JSON Parser (cJSON)

### Librería Nativa: `cJSON`

**Estado:** ✅ Incluido en ESP-IDF  
**Componente:** `json`  
**Header:** `#include "cJSON.h"`

#### Características Clave

- Parser JSON ligero y eficiente
- Incluido en ESP-IDF como componente estándar
- API simple para parsing y creación de JSON

#### API Principal

```c
#include "cJSON.h"

// Parsing
cJSON *root = cJSON_Parse(json_string);
cJSON *name = cJSON_GetObjectItem(root, "name");
cJSON *str_val = cJSON_GetObjectItem(root, "description");
cJSON *num_val = cJSON_GetObjectItem(root, "level");

const char *name_str = name->valuestring;
int level = num_val->valueint;

// Navegación
cJSON *attributes = cJSON_GetObjectItem(root, "attributes");
cJSON *str = cJSON_GetObjectItem(attributes, "STR");

// Crear JSON
cJSON *root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "name", "Mistolito");
cJSON_AddNumberToObject(root, "hp", 100);
char *json_str = cJSON_Print(root);

// Liberar
cJSON_Delete(root);
```

#### Uso en el MVP

El ADN del Pet se cargará desde un archivo JSON en la SD:

```json
{
    "name": "Mistolito",
    "attributes": {
        "STR": 12,
        "DEX": 14,
        "CON": 10,
        "INT": 8,
        "WIS": 13,
        "CHA": 15
    },
    "metabolic_deltas": {
        "hunger": 1.2,
        "energy": 0.8
    }
}
```

#### Links

- cJSON está incluido en ESP-IDF como componente estándar
- Documentación de referencia: [cJSON GitHub](https://github.com/DaveGamble/cJSON)

---

## Módulo 5: RNG (Random Number Generator)

### Librería Nativa: `esp_random`

**Estado:** ✅ Disponible y documentado  
**Componente:** `esp_hw_support`  
**Header:** `#include "esp_random.h"`

#### Características Clave

- **Hardware RNG verdadero:** Basado en ruido térmico y SAR ADC
- **Sin WiFi/Bluetooth necesario:** Secundary entropy source siempre disponible
- **Thread-safe:** Puede usarse desde cualquier contexto
- **Resolución:** 32-bit words

#### API Principal

```c
#include "esp_random.h"

// Obtener número aleatorio de 32 bits
uint32_t random_value = esp_random();

// Llenar buffer con bytes aleatorios
uint8_t buffer[16];
esp_fill_random(buffer, sizeof(buffer));

// Para tiradas d20
uint32_t d20_roll = (esp_random() % 20) + 1;  // 1-20
```

#### Consideraciones

- El RNG produce números verdaderamente aleatorios cuando:
  - WiFi o Bluetooth están activos, O
  - Se habilita la fuente de entropía SAR ADC, O
  - Durante el bootloader
- Sin WiFi/BT: La secondary entropy source (8MHz oscillator) pasa tests Dieharder

#### Links de Documentación

- [Random Number Generation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/random.html)

---

## Módulo 6: Memory Management (PSRAM + heap_caps)

### Librería Nativa: `heap_caps`

**Estado:** ✅ Disponible y documentado  
**Componente:** `heap`  
**Header:** `#include "esp_heap_caps.h"`

#### Características Clave

- **PSRAM 8MB:** Disponible como heap con capabilities
- **Internal SRAM 512KB:** Para datos críticos
- **Allocación por capacidades:** Seleccionar tipo de memoria según uso
- **DMA-capable memory:** Buffers para periféricos

#### API Principal

```c
#include "esp_heap_caps.h"

// Allocar en PSRAM (8MB disponibles)
void *large_buffer = heap_caps_malloc(1024 * 1024, MALLOC_CAP_SPIRAM);

// Allocar en memoria interna (512KB disponibles)
void *internal_buffer = heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

// Allocar memoria DMA para SPI/LCD
void *dma_buffer = heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA);

// Información de heap
size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

// Usar malloc estándar (prefiere memoria interna)
void *ptr = malloc(1024);  // Equivalente a heap_caps_malloc(1024, MALLOC_CAP_8BIT)

// Siempre usar free() estándar para liberar
free(ptr);
```

#### Uso en el MVP

| Tipo de Datos | Memoria | Capability |
|---------------|---------|------------|
| Buffer de pantalla LVGL | PSRAM | `MALLOC_CAP_SPIRAM` |
| Buffer DMA SPI/LCD | Interna | `MALLOC_CAP_DMA` |
| Estructuras del Pet | Interna | `MALLOC_CAP_8BIT` |
| Sprites del Pet | PSRAM | `MALLOC_CAP_SPIRAM` |
| Stack de tareas | Interna | Por defecto |

#### Links de Documentación

- [Heap Memory Allocation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/mem_alloc.html)

---

## Módulo 7: Timers y Tasks (FreeRTOS + ESP Timer)

### Librerías: FreeRTOS (SMP) + `esp_timer`

**Estado:** ✅ Disponible y documentado  
**Componentes:** `freertos`, `esp_timer`  
**Headers:** `#include "freertos/task.h"`, `#include "esp_timer.h"`

#### FreeRTOS SMP (Symmetric Multiprocessing)

ESP32-S3 es dual-core, y ESP-IDF usa una versión modificada de FreeRTOS con soporte SMP.

##### Características

- **Dos cores idénticos:** Core 0 y Core 1
- **Tasks con afinidad:** Pueden anclarse a un core específico
- **Time slicing mejorado:** Best effort round-robin para SMP
- **Stack sizes en bytes:** No en words como Vanilla FreeRTOS

##### API Principal

```c
#include "freertos/task.h"

// Crear task anclada a core específico
xTaskCreatePinnedToCore(
    combat_task,          // Función
    "combat",             // Nombre
    4096,                 // Stack size en BYTES
    NULL,                 // Parámetros
    5,                    // Prioridad
    NULL,                 // Handle
    1                     // Core ID (0 o 1)
);

// Crear task sin afinidad (puede cambiar de core)
xTaskCreate(
    ui_task,
    "ui",
    8192,
    NULL,
    4,
    NULL
);

// Delay
vTaskDelay(pdMS_TO_TICKS(100));  // 100ms

// Delay hasta tiempo absoluto (para loops periódicos)
TickType_t last_wake = xTaskGetTickCount();
for (;;) {
    // Trabajo...
    xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(16));  // ~60 FPS
}
```

#### ESP Timer (High Resolution Timer)

Para timing preciso (resolución de microsegundos).

##### Características

- **Resolución:** 1 microsegundo
- **Bits:** 52 bits
- **One-shot y periódico**
- **Dos modos de dispatch:** Task (default) o ISR

##### API Principal

```c
#include "esp_timer.h"

// Callback
static void combat_tick_cb(void *arg) {
    // Lógica de combate
}

// Crear timer periódico
esp_timer_handle_t combat_timer;
esp_timer_create_args_t timer_args = {
    .callback = &combat_tick_cb,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "combat_tick"
};
esp_timer_create(&timer_args, &combat_timer);

// Iniciar timer periódico (cada 100ms)
esp_timer_start_periodic(combat_timer, 100000);  // microseconds

// Obtener tiempo actual
int64_t now = esp_timer_get_time();  // microseconds since boot

// Detener y eliminar
esp_timer_stop(combat_timer);
esp_timer_delete(combat_timer);
```

#### Uso en el MVP

| Componente | Timer/Task | Core | Frecuencia |
|------------|------------|------|------------|
| Loop de combate | ESP Timer | N/A | 10 Hz |
| UI LVGL | FreeRTOS Task | 1 | ~30 FPS |
| Carga de ADN | FreeRTOS Task | 0 | Una vez |
| Motor de juego | FreeRTOS Task | 0 | Continuo |

#### Links de Documentación

- [FreeRTOS (IDF)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/freertos_idf.html)
- [ESP Timer](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/esp_timer.html)

---

## Matriz de Dependencias

| Módulo MVP | Librería ESP-IDF | Componente | Estado |
|------------|------------------|------------|--------|
| M1: UI Base | `esp_lcd` + LVGL v8.3.0 | `esp_lcd` + `lvgl/lvgl` | ✅ |
| M2: ADN | `sdmmc` + `vfs` + `cJSON` | `sdmmc`, `vfs`, `json` | ✅ |
| M3: Combate | FreeRTOS + `esp_timer` + `esp_random` | `freertos`, `esp_timer` | ✅ |
| M4: Niveles | (Implementación propia) | N/A | ✅ |
| M5: Profesiones | (Implementación propia) | N/A | ✅ |
| M6: Muerte | (Implementación propia) | N/A | ✅ |
| M7: Integración | Todos los anteriores | Todos | ✅ |

---

## Plan de Implementación

### Fase 1: Setup del Proyecto

1. Crear proyecto ESP-IDF básico
2. Configurar `sdkconfig` para:
   - PSRAM enabled (8MB)
   - SPI frequencies
   - LCD driver ST7789
3. Agregar dependencia LVGL v8.3.0

### Fase 2: UI Base (M1)

1. Inicializar SPI bus compartido (LCD + SD)
2. Configurar LCD ST7789
3. Integrar LVGL con callback de flush
4. Crear layout básico (HP bar, EXP bar, nombre)

### Fase 3: SD Card y ADN (M2)

1. Montar SD card con VFS/FAT
2. Definir estructura JSON del ADN
3. Implementar carga y parsing
4. Generar instancia de Pet desde ADN

### Fase 4: Combate (M3)

1. Definir enemigo base
2. Implementar motor de combate tick-based
3. Tiradas d20 con `esp_random()`
4. Detectar victoria/derrota

### Fase 5: Niveles y Profesiones (M4, M5)

1. Implementar curva de EXP
2. Sistema de subida de stats
3. Tablas de profesiones hardcodeadas
4. Desbloqueo de habilidades

### Fase 6: Muerte y Renacimiento (M6)

1. Detectar HP = 0
2. Animación de muerte
3. Reset de Pet
4. Persistencia de DP

### Fase 7: Integración Final (M7)

1. Flujo completo boot → combate → muerte
2. Polishing de UI
3. Testing en hardware real
4. Verificación de memoria

---

## Referencias

### Documentación Oficial ESP-IDF

- [ESP-IDF Programming Guide (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [LCD Peripherals](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd/)
- [SPI Master](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html)
- [SD/SDIO/MMC Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/sdmmc.html)
- [Virtual Filesystem](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/vfs.html)
- [Heap Memory](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/mem_alloc.html)
- [Random Number Generation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/random.html)
- [ESP Timer](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/esp_timer.html)
- [FreeRTOS (IDF)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/freertos_idf.html)

### Componentes Externos

- [LVGL v8.3.0 en ESP Component Registry](https://components.espressif.com/components/lvgl/lvgl/versions/8.3.0)
- [LVGL Documentation v8](https://docs.lvgl.io/8.3/)

### Ejemplos Relevantes

- [peripherals/lcd/spi_lcd_touch](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/lcd/spi_lcd_touch)
- [peripherals/spi_master/lcd](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/spi_master/lcd)
- [storage/sd_card/sdmmc](https://github.com/espressif/esp-idf/tree/master/examples/storage/sd_card/sdmmc)
- [system/esp_timer](https://github.com/espressif/esp-idf/tree/master/examples/system/esp_timer)

---

## Conclusiones

1. **ESP-IDF es suficiente:** No se necesitan librerías adicionales excepto LVGL
2. **Todo está documentado:** APIs estables y bien documentadas
3. **Optimizado para ESP32-S3:** Aprovecha características del chip (PSRAM, DMA, SMP)
4. **Ejemplos disponibles:** Hay ejemplos para casi todos los módulos
5. **Listo para implementar:** El camino está despejado

**Próximo paso:** Comenzar con la Fase 1 del plan de implementación.
