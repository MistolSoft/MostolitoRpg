# Research de Librerías ESP-IDF para MVP

Este documento rastrea la búsqueda de librerías para cada módulo del MVP.

---

## Módulos del MVP

1. **Display/LVGL** - Renderizado de UI en LCD
2. **SPI LCD** - Driver para ST7789T3
3. **SD Card** - Almacenamiento de ADN y datos
4. **JSON Parsing** - Parsear ADN y configuraciones
5. **Random/d20** - Generación de números aleatorios
6. **Sprites/Graphics** - Cargar y renderizar sprites

---

## TODO: Búsqueda por Módulo

### [x] Módulo 1: Display/LVGL ✓
- [x] Buscar integración LVGL con ESP-IDF
- [x] Buscar driver oficial ESP-IDF para LCD
- [x] Documentar configuración de memoria (PSRAM)

**HALLAZGOS:**
- **LVGL 9.6** tiene integración oficial con ESP-IDF
- Documentación: `https://docs.lvgl.io/master/integration/chip_vendors/espressif/add_lvgl_to_esp32_idf_project.html`
- ESP32-S3 tiene soporte para **PPA (Pixel Processing Accelerator)** para aceleración gráfica
- Driver oficial: `lv_st7789.h` disponible en LVGL
- Soporte para DMA2D en ESP32-S3

### [x] Módulo 2: SPI LCD (ST7789T3) ✓
- [x] Buscar driver específico ST7789 o compatible
- [x] Buscar ejemplos ESP-IDF SPI LCD
- [x] Documentar pines y configuración

**HALLAZGOS:**
- **Driver oficial LVGL para ST7789:** `lv_st7789.h`
- Documentación: `https://docs.lvgl.io/master/integration/external_display_controllers/st7789.html`
- El ST7789T3 es compatible con ST7789 estándar
- Pines ya definidos en documentación del proyecto:
  - MOSI=GPIO38, SCLK=GPIO39
  - CS=GPIO45, DC=GPIO42, BL=GPIO1

### [x] Módulo 3: SD Card (SPI mode) ✓
- [x] Buscar soporte FatFS en ESP-IDF
- [x] Buscar ejemplos de SD card SPI
- [x] Documentar mounting y lectura

**HALLAZGOS:**
- **FatFS incluido en ESP-IDF** con soporte completo
- Documentación: `https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/fatfs.html`
- **SDMMC Driver** para SD cards: `esp_vfs_fat_sdspi_mount()` para SPI mode
- Documentación: `https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/sdmmc.html`
- Pines ya definidos: MOSI=GPIO38, SCLK=GPIO39, MISO=GPIO40, CS=GPIO41
- Ejemplo oficial: `examples/storage/sd_card/sdspi`

### [x] Módulo 4: JSON Parsing ✓
- [x] Buscar cJSON en ESP-IDF
- [x] Documentar API de parsing
- [x] Buscar alternativas si cJSON no es suficiente

**HALLAZGOS:**
- **cJSON incluido en ESP-IDF** como componente oficial
- Header: `#include "cJSON.h"`
- Componente: `cJSON` (REQUIRES cJSON en CMakeLists.txt)
- API estándar: `cJSON_Parse()`, `cJSON_GetObjectItem()`, `cJSON_GetArrayItem()`
- Documentación: incluida en ESP-IDF bajo `components/json/cJSON`

### [x] Módulo 5: Random Number Generator ✓
- [x] Buscar esp_random() documentación
- [x] Verificar entropía hardware del ESP32-S3
- [x] Documentar uso para d20

**HALLAZGOS:**
- **Hardware RNG en ESP32-S3** con entropía real
- Documentación: `https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/random.html`
- API principal: `esp_random()` devuelve uint32_t (0 a UINT32_MAX)
- API buffer: `esp_fill_random(buf, len)`
- Entropía basada en ruido térmico del ADC y clocks asíncronos
- Para d20: `(esp_random() % 20) + 1`

### [ ] Módulo 6: Sprites/Graphics
- [ ] Buscar formato de sprites recomendado
- [ ] Buscar librerías de decodificación de imágenes
- [ ] Documentar integración con LVGL

---
