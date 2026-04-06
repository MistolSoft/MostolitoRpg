# Setup del Entorno de Desarrollo - MistolitoRPG

Este documento describe cómo configurar el entorno de desarrollo en Windows PowerShell usando **EIM CLI** (ESP-IDF Installation Manager) para programar la **Waveshare ESP32-S3-LCD-2**.

---

## Qué es EIM

**EIM** (ESP-IDF Installation Manager) es el instalador oficial de Espressif. Instala automáticamente:

- **ESP-IDF**: Framework con APIs, librerías y código fuente
- **Toolchain**: Compilador Xtensa para ESP32-S3
- **Build tools**: CMake, Ninja
- **Python environment**: Entorno virtual con dependencias
- **Scripts**: `idf.py` para compilar, flashear y monitorear

---

## Instalación

### 1. Instalar EIM CLI

Con PowerShell, ejecutar:

```
winget install Espressif.EIM-CLI
```

Esto instala el comando `eim` en el sistema.

### 2. Instalar ESP-IDF

Ejecutar:

```
eim install -i v5.4
```

La instalación tarda entre 10 y 30 minutos. Descarga e instala ESP-IDF v5.4 y todas las herramientas necesarias.

### 3. Verificar instalación

Cerrar y volver a abrir PowerShell, luego:

```
idf.py --version
```

Debe mostrar: `ESP-IDF v5.4`

---

## Comandos EIM

| Comando | Descripción |
|---------|-------------|
| `eim install -i v5.4` | Instalar ESP-IDF v5.4 |
| `eim wizard` | Instalación interactiva |
| `eim list` | Listar versiones instaladas |
| `eim select v5.4` | Seleccionar versión activa |
| `eim remove v5.4` | Eliminar versión |
| `eim purge` | Eliminar todo |

---

## Crear Proyecto

### Crear estructura

```
New-Item -ItemType Directory -Path "C:\Proyectos\mistolito_rpg" -Force
Set-Location "C:\Proyectos\mistolito_rpg"
idf.py create-project mistolito_rpg
```

### Configurar target

```
idf.py set-target esp32s3
```

---

## Archivos de Configuración

### sdkconfig.defaults

Crear archivo `sdkconfig.defaults` en la raíz del proyecto. El target se configura con `idf.py set-target esp32s3`, no en este archivo.

```
# PSRAM (Octal, 80MHz - Waveshare ESP32-S3-LCD-2 usa ESP32-S3R8)
CONFIG_SPIRAM_SUPPORT=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY=y

# Flash (16MB QIO)
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

# CPU (240 MHz)
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y

# Compilador (optimizar por performance)
CONFIG_COMPILER_OPTIMIZATION_PERF=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
```

Nota: Las opciones de LVGL (color depth, resolución) se configuran en el componente, no en sdkconfig.defaults.

### idf_component.yml

Crear archivo `idf_component.yml` en la raíz del proyecto:

```yaml
dependencies:
  lvgl/lvgl:
    version: "9.*"
  espressif/esp_lcd_touch_cst816s:
    version: "1.1.1"
  waveshare/qmi8658:
    version: "1.0.1"
  espressif/esp_lvgl_port:
    version: "2.7.2"
```

**Nota:** LVGL 9 incluye mejor soporte para sprites con el widget `lv_animimg`.

---

## Primer Build

```
idf.py reconfigure
idf.py build
```

---

## Detectar Puerto COM

```
mode
```

O bien:

```
Get-WMIObject Win32_SerialPort | Select-Object DeviceID, Description
```

Anotar el puerto (ej: COM3, COM4).

---

## Flashear

Reemplazar `COM3` por el puerto detectado:

```
idf.py -p COM3 flash monitor
```

Para salir del monitor: `Ctrl+]`

---

## Comandos idf.py

| Comando | Descripción |
|---------|-------------|
| `idf.py build` | Compilar |
| `idf.py -p COM3 flash` | Flashear |
| `idf.py -p COM3 monitor` | Monitorear |
| `idf.py -p COM3 flash monitor` | Flashear y monitorear |
| `idf.py fullclean` | Limpiar build |
| `idf.py menuconfig` | Configurar |
| `idf.py reconfigure` | Reconfigurar |
| `idf.py set-target esp32s3` | Configurar target |

---

## GPIO de la Placa

| Función | GPIO |
|---------|------|
| LCD MOSI | 38 |
| LCD SCLK | 39 |
| LCD MISO | 40 |
| LCD CS | 45 |
| LCD DC | 42 |
| LCD RST | 0 |
| LCD BL | 1 |
| SD CS | 41 |
| I2C SCL | 47 |
| I2C SDA | 48 |
| Touch INT | 46 |
| IMU INT | 3 |

---

## Problemas Comunes

### "idf.py no se reconoce"

Cerrar y volver a abrir PowerShell.

### "No se encuentra puerto COM"

- Verificar cable USB
- Probar otro puerto USB
- Revisar Administrador de Dispositivos → Puertos (COM y LPT)

### "Flash timeout"

Entrar en modo descarga:

1. Mantener presionado BOOT
2. Presionar y soltar RESET
3. Soltar BOOT
4. Flashear

### "eim no se reconoce"

Reiniciar PowerShell después de instalar con winget.

---

## Debugging

La Waveshare ESP32-S3-LCD-2 tiene **JTAG integrado por USB**, no requiere hardware adicional.

### Opciones de Debugging

| Método | Descripción | Hardware Extra |
|--------|-------------|----------------|
| **JTAG USB nativo** | Debugger completo: breakpoints, stepping, watchpoints | No |
| **idf.py monitor** | Logs en tiempo real, stack trace en crashes | No |
| **GDBStub** | Debugger básico activado al crashear | No |

### Usar JTAG (Recomendado)

Abrir dos terminales PowerShell:

**Terminal 1 - OpenOCD:**
```
openocd -f board/esp32s3-builtin.cfg
```

**Terminal 2 - GDB:**
```
idf.py gdb
```

### Comandos GDB Básicos

| Comando | Descripción |
|---------|-------------|
| `break main` | Poner breakpoint en función |
| `continue` | Continuar ejecución |
| `next` | Step over (siguiente línea) |
| `step` | Step into (entrar en función) |
| `print variable` | Ver valor de variable |
| `backtrace` | Ver call stack |
| `quit` | Salir |

### Ver logs sin JTAG

```
idf.py -p COM3 monitor
```

Muestra logs y, si hay crash, muestra el stack trace con líneas de código.

---

## Unit Testing

ESP-IDF usa **Unity** como framework de testing.

### Estructura de Tests

```
components/
└── combat/
    ├── combat.c
    ├── combat.h
    └── test/
        ├── CMakeLists.txt
        └── test_combat.c
```

### Ejemplo de Test

```c
#include "unity.h"
#include "combat.h"

TEST_CASE("calcular daño correctamente", "[combat]")
{
    int hp = calculate_damage(10, 5);
    TEST_ASSERT_EQUAL_INT(5, hp);
}

TEST_CASE("HP no puede ser negativo", "[combat]")
{
    int hp = calculate_damage(3, 10);
    TEST_ASSERT_TRUE(hp >= 0);
}
```

### CMakeLists.txt del Test

```cmake
idf_component_register(SRC_DIRS "." 
                       INCLUDE_DIRS "." 
                       REQUIRES unity combat)
```

### Ejecutar Tests

```
idf.py build
idf.py -p COM3 flash monitor
```

En el monitor, presionar Enter para ver el menú de tests y seleccionar cuál ejecutar.

### Assertions de Unity

| Macro | Descripción |
|-------|-------------|
| `TEST_ASSERT_TRUE(x)` | Verifica que x es true |
| `TEST_ASSERT_FALSE(x)` | Verifica que x es false |
| `TEST_ASSERT_EQUAL_INT(a, b)` | Verifica a == b |
| `TEST_ASSERT_EQUAL_STRING(a, b)` | Verifica strings iguales |
| `TEST_ASSERT_NOT_NULL(p)` | Verifica puntero no NULL |

---

## Memory Management

**C no tiene garbage collector.** Hay que liberar memoria manualmente.

### Regla de Oro

Quien hace `malloc()`, hace `free()`.

### Detectar Memory Leaks

ESP-IDF incluye herramientas de heap debugging.

#### Ver estadísticas de heap en runtime:

```c
#include "esp_heap_caps.h"

void print_heap_info(void) {
    printf("Free internal: %d bytes\n", 
           heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("Free PSRAM: %d bytes\n", 
           heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
```

#### Habilitar heap tracing en sdkconfig:

```
CONFIG_HEAP_TRACING_STANDALONE=y
```

#### Usar heap tracing en código:

```c
#include "esp_heap_trace.h"

ESP_HEAP_TRACE_START();

// Código sospechoso de leak
void *ptr = malloc(1024);
// ... si falta free(ptr) ...

ESP_HEAP_TRACE_STOP();
```

### Comandos de Heap Debugging

| Comando | Descripción |
|---------|-------------|
| `idf.py heap-stats` | Ver estadísticas de uso de heap |
| `idf.py heap-trace` | Iniciar tracing de allocations |

### Buenas Prácticas

1. Siempre inicializar punteros a `NULL`
2. Verificar que `malloc()` no devolvió `NULL`
3. Liberar memoria en el mismo scope donde se asignó
4. Usar `heap_caps_malloc()` para elegir tipo de memoria

---

## Librerías y Gestión de Memoria

Cada librería del MVP maneja su memoria de forma diferente. Conviene saber cuál gestiona sola y cuál requiere intervención manual.

### Resumen de Librerías

| Librería | Gestiona Memoria | Responsabilidad |
|----------|------------------|-----------------|
| **LVGL** | Sí | Objetos UI, buffer de pantalla |
| **cJSON** | Sí | Strings parseados, objetos JSON |
| **FAT/VFS** | Sí | Handles de archivos, buffers internos |
| **SDMMC** | Sí | Estructuras de tarjeta |
| **FreeRTOS** | Sí | Tasks, colas, semáforos |
| **ESP LCD** | Parcial | Sólo libera lo que vos asignaste |
| **IMU (QMI8658)** | No | Vos gestionás structs del sensor |
| **Tu código** | No | Estructuras del Pet, combate, etc. |

### LVGL - Gestiona Todo

LVGL tiene su propio memory manager interno.

**Creación:**
```c
lv_obj_t* btn = lv_btn_create(lv_scr_act());
lv_obj_t* label = lv_label_create(btn);
```

**Eliminación:**
```c
lv_obj_del(btn);  // Elimina el botón y todos sus hijos
```

**Reglas:**
- LVGL asigna desde un buffer pre-configurado
- Usar `lv_obj_del()` para eliminar, NUNCA `free()`
- Los objetos hijos se eliminan automáticamente al eliminar el padre
- Configurar tamaño del heap en `lv_conf.h`: `LV_MEM_SIZE (32 * 1024)`

**Monitorear memoria LVGL:**
```c
lv_mem_monitor_t mon;
lv_mem_monitor(&mon);
printf("LVGL free: %d bytes\n", mon.free_size);
printf("LVGL used: %d%%\n", mon.used_pct);
```

### cJSON - Gestiona Strings

cJSON asigna memoria para strings y objetos parseados.

**Creación:**
```c
cJSON* root = cJSON_Parse(json_string);  // cJSON asigna internamente
cJSON* name = cJSON_CreateString("Mistolito");
```

**Eliminación:**
```c
cJSON_Delete(root);  // Libera todo el árbol recursivamente
```

**Reglas:**
- `cJSON_Parse()` asigna memoria para todo el árbol
- `cJSON_Delete()` libera el árbol completo
- NUNCA hacer `free()` sobre un puntero cJSON
- Si creás un objeto nuevo, el árbol padre lo libera al hacer `cJSON_Delete()`

**Strings devueltos:**
```c
char* json_str = cJSON_Print(root);  // cJSON asigna un string nuevo
// Usar json_str...
free(json_str);  // VOS debés liberarlo
```

### FAT/VFS - Gestiona Handles

El sistema de archivos maneja sus propios buffers internos.

**Apertura:**
```c
FILE* f = fopen("/sdcard/dna.json", "r");
```

**Cierre:**
```c
fclose(f);  // Libera el handle y buffers internos
```

**Reglas:**
- `fopen()` asigna el handle y buffers
- `fclose()` libera todo
- El contenido leído con `fread()` va a buffers que VOS asignaste
- Siempre cerrar archivos, sino quedan handles colgados

**Buffers del usuario:**
```c
char buffer[1024];  // Vuestro buffer
fread(buffer, 1, sizeof(buffer), f);  // VCS gestionás buffer
```

### SDMMC - Gestiona Estructuras

El driver de SD card inicializa estructuras internas.

**Montaje:**
```c
sdmmc_card_t* card = NULL;
esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
```

**Desmontaje:**
```c
esp_vfs_fat_sdcard_unmount("/sdcard", card);  // Libera estructuras
```

**Reglas:**
- El puntero `card` lo asigna la función de montaje
- No hacer `free(card)`, lo maneja el unmount
- Siempre desmontar antes de apagar

### FreeRTOS - Gestiona Tasks

FreeRTOS tiene su propio allocator para tareas y sincronización.

**Creación:**
```c
TaskHandle_t handle = NULL;
xTaskCreate(task_func, "nombre", 4096, NULL, 5, &handle);
```

**Eliminación:**
```c
vTaskDelete(handle);  // Libera stack y TCB
```

**Reglas:**
- FreeRTOS asigna stack y TCB (Task Control Block)
- `vTaskDelete()` libera todo
- Si la tarea se borra a sí misma: `vTaskDelete(NULL)`
- Colas, semáforos y mutex también los gestiona FreeRTOS

**Colas:**
```c
QueueHandle_t q = xQueueCreate(10, sizeof(int));
// Usar...
vQueueDelete(q);  // Libera la cola
```

### ESP LCD - Gestión Parcial

El driver LCD gestiona algunos recursos, pero el buffer de pantalla es vuestro.

**Vuestro buffer:**
```c
// VOS asignás el buffer (en PSRAM recomendado)
uint8_t* buf = heap_caps_malloc(240 * 320 * 2, MALLOC_CAP_SPIRAM);
```

**Handle LCD:**
```c
esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_new_panel_io_spi(..., &io_handle);  // ESP-IDF asigna
```

**Limpieza:**
```c
free(buf);  // Vuestro buffer, vos lo liberás
esp_lcd_panel_io_del(io_handle);  // ESP-IDF libera el handle
```

### IMU QMI8658 - Sin Gestión

El driver del IMU no asigna memoria, structs son del usuario.

```c
qmi8658_config_t config = { ... };  // En stack o global
qmi8658_init(&config);  // Sólo configura

qmi8658_data_t data;  // Vuestro struct
qmi8658_read(&data);  // Llena vuestro struct
```

No hay nada que liberar.

### Tu Código - Sin Gestión

Para estructuras del Pet, combate, etc.:

**Opción A: Memoria estática (recomendado para MVP):**
```c
static pet_state_t g_pet;  // Siempre existe, nada que liberar
static combat_state_t g_combat;
```

**Opción B: Memoria dinámica:**
```c
pet_state_t* pet = malloc(sizeof(pet_state_t));
// Usar...
free(pet);
```

---

## Reglas Prácticas por Librería

| Librería | Crear | Liberar | ¿Quién libera? |
|----------|-------|---------|----------------|
| **LVGL** | `lv_xxx_create()` | `lv_obj_del()` | Vos llamás delete |
| **cJSON** | `cJSON_Parse()` | `cJSON_Delete()` | Vos llamás delete |
| **cJSON string** | `cJSON_Print()` | `free()` | Vos llamás free |
| **FAT/VFS** | `fopen()` | `fclose()` | Vos cerrás |
| **FreeRTOS task** | `xTaskCreate()` | `vTaskDelete()` | Vos eliminás |
| **FreeRTOS queue** | `xQueueCreate()` | `vQueueDelete()` | Vos eliminás |
| **LCD buffer** | `heap_caps_malloc()` | `free()` | Vos liberás |
| **LCD handle** | `esp_lcd_new_panel_io_spi()` | `esp_lcd_panel_io_del()` | Vos eliminás |
| **IMU** | structs locales | nada | No hay alloc |
| **Tu código** | `malloc()` | `free()` | Vos liberás |

---

## Checklist

- [ ] EIM CLI instalado (`winget install Espressif.EIM-CLI`)
- [ ] ESP-IDF v5.4 instalado (`eim install -i v5.4`)
- [ ] `idf.py --version` funciona
- [ ] Puerto COM detectado
- [ ] Proyecto creado
- [ ] sdkconfig.defaults configurado
- [ ] idf_component.yml configurado
- [ ] Build exitoso
- [ ] Flash exitoso
- [ ] OpenOCD funciona (`openocd -f board/esp32s3-builtin.cfg`)
- [ ] GDB funciona (`idf.py gdb`)
