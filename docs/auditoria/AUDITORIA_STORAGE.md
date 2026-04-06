# Auditoría - Módulo Storage

## Resumen
El módulo `storage` tiene **problemas arquitectónicos críticos** que causan dependencias circulares.

---

## Archivos del Módulo

| Archivo | Líneas | Propósito |
|---------|--------|-----------|
| `sd_card.h` | 38 | Header público |
| `sd_card.c` | 434 | Implementación (con código duplicado) |
| `CMakeLists.txt` | 3 | Configuración de build |

---

## Problema #1: Header incluye `pet.h` (CRÍTICO)

### `sd_card.h` línea 7:
```c
#include "pet.h"  // ❌ ERROR CÍCLICO
```

### Problema
- `storage` está en el nivel más bajo (acceso a hardware SD)
- `pet` está en el nivel de lógica de negocio
- Storage NO debería conocer `pet_t`

### Impacto
```
storage → pet → data → storage (CICLO)
```

### Funciones que usan `pet_t`:
```c
esp_err_t storage_save_pet_full(const pet_t *pet);  // Línea 24
esp_err_t storage_load_pet_full(pet_t *pet);        // Línea 26
```

### Solución
1. **Opción A:** Mover estas funciones a `pet.c`
2. **Opción B:** Crear componente `persistence` intermedio
3. **Opción C:** Pasar callbacks o punteros genéricos

---

## Problema #2: Código Duplicado (CRÍTICO)

### `sd_card.c` tiene DOS versiones de guardar pet:

#### Versión 1 (líneas 97-152): Completa
```c
esp_err_t storage_save_pet_full(const pet_t *pet)
{
    // ... implementación con bonuses y skills ...
    fprintf(f, " \"bonuses\": {\n");
    fprintf(f, " \"skills\": [\n");
    // ...
}
```

#### Versión 2 (líneas 154-185): Incompleta/antigua
```c
// ❌ Código muerto - usa variables inexistentes
fprintf(f, " \"name\": \"%s\",\n", name);  // 'name' no existe
fprintf(f, " \"level\": %u,\n", level);    // 'level' no existe
```

**Este código está huérfano y usa variables no declaradas.**

---

## Problema #3: `sd_card.c` incluye `data_loader.h`

### Línea 3:
```c
#include "data_loader.h"  // ❌ Otra dependencia incorrecta
```

### Razón:
Se usa `pet_get_exp_for_level()` en línea 335:
```c
pet->exp_to_next = pet_get_exp_for_level(pet->level + 1);
```

Pero esa función está en `pet.h`, no en `data_loader.h`.

---

## Problema #4: Serialización JSON Manual

### Código actual (líneas 110-144):
```c
fprintf(f, "{\n");
fprintf(f, " \"name\": \"%s\",\n", pet->name);
fprintf(f, " \"level\": %u,\n", pet->level);
// ... 20 líneas más de fprintf ...
```

### Problemas:
- No usa cJSON (ya disponible en el proyecto)
- Propenso a errores de formato
- Difícil de mantener
- Sin validación

### Alternativa con cJSON:
```c
cJSON *json = cJSON_CreateObject();
cJSON_AddStringToObject(json, "name", pet->name);
cJSON_AddNumberToObject(json, "level", pet->level);
char *str = cJSON_Print(json);
// escribir str a archivo
cJSON_Delete(json);
```

---

## Problema #5: Parser JSON Manual

### Líneas 187-227: Función `find_json_value()`
```c
static char* find_json_value(const char* json, const char* key)
{
    static char value[256];
    char search_key[64];
    // ... parser manual ...
}
```

### Problemas:
- Reimplementa parsing JSON (ya existe cJSON)
- `static char value[256]` - no es thread-safe
- Frágil ante formatos inesperados
- Código redundante

---

## Problema #6: Constantes Hardcodeadas

### Líneas 18-21:
```c
#define SD_MISO_GPIO 40
#define SD_MOSI_GPIO 38
#define SD_CLK_GPIO 39
#define SD_CS_GPIO 41
```

### Problema:
- Configuración de hardware hardcodeada
- Debería estar en Kconfig o sdkconfig
- Difícil cambiar entre revisiones de hardware

---

## Problema #7: Sin Manejo de Errores Robusto

### Ejemplo (líneas 104-108):
```c
FILE *f = fopen(PET_FILE_PATH, "w");
if (!f) {
    ESP_LOGE(TAG, "Failed to open pet file for writing");
    return ESP_FAIL;  // ❌ No limpia, no informa qué falló
}
```

### Alternativa:
```c
FILE *f = fopen(PET_FILE_PATH, "w");
if (!f) {
    ESP_LOGE(TAG, "Failed to open %s: %s", PET_FILE_PATH, strerror(errno));
    return ESP_FAIL;
}
```

---

## Arquitectura Propuesta

### Estado Actual:
```
┌─────────────┐
│   usb_init  │
│      ↓      │
│   storage   │──→ pet (CICLO)
│      ↓      │
│   (SD HW)   │
└─────────────┘
```

### Estado Propuesto:
```
┌──────────┐     ┌──────────────┐
│ usb_init │     │     pet      │
│     ↓    │     │      ↓       │
│ storage  │     │ pet_persist  │
│ (SD HW)  │←────┴──────────────┘
└──────────┘
```

### Funciones de storage genéricas:
```c
// sd_card.h - SOLO funciones genéricas
esp_err_t storage_init(void);
esp_err_t storage_mount_sd(void);
bool storage_is_mounted(void);
bool storage_file_exists(const char *path);
esp_err_t storage_save_file(const char *path, const uint8_t *data, size_t len);
esp_err_t storage_load_file(const char *path, uint8_t **data, size_t *len);
esp_err_t storage_delete_file(const char *path);
```

### Funciones de pet_persistence:
```c
// pet_persistence.h
esp_err_t pet_save(pet_t *pet);
esp_err_t pet_load(pet_t *pet);
bool pet_data_exists(void);
```

---

## Checklist de Correcciones

- [ ] Eliminar `#include "pet.h"` de `sd_card.h`
- [ ] Eliminar `#include "data_loader.h"` de `sd_card.c`
- [ ] Borrar código duplicado (líneas 154-185)
- [ ] Mover `storage_save_pet_full()` y `storage_load_pet_full()` a otro lado
- [ ] Reescribir serialización con cJSON
- [ ] Mover constantes GPIO a Kconfig
- [ ] Agregar manejo de errores con errno
