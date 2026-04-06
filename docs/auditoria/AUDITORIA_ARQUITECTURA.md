# Auditoría de Arquitectura - MistolitoRPG

## Resumen Ejecutivo

El proyecto tiene **problemas arquitectónicos graves** que impiden la compilación y requieren refactoring significativo. Los problemas principales son:

1. **Dependencias circulares entre componentes**
2. **Inclusiones de headers incorrectas**
3. **CMakeLists.txt incompletos o incorrectos**
4. **Código duplicado y muerto**
5. **Falta de separación de responsabilidades**

---

## Problema Crítico #1: Dependencias Circulares

### Descripción
Existe una cadena de dependencias circulares que hace imposible compilar el proyecto:

```
storage → pet → data → (cJSON)
   ↑         |
   └─────────┘
```

### Detalle por archivo:

#### `sd_card.h` (línea 7)
```c
#include "pet.h"  // ❌ ERROR: storage depende de pet
```

**Problema:** `sd_card.h` incluye `pet.h` en el header público, obligando a cualquier componente que incluya `sd_card.h` a tener acceso a `pet.h`.

**Impacto:**
- `usb_init` incluye `sd_card.h` → necesita `pet.h`
- Crea dependencia circular: storage → pet → data → storage

#### `pet.h` (línea 6)
```c
#include "data_loader.h"  // ❌ REMOVIDO pero pet.c aún lo necesita
```

#### `combat_engine.c` (línea 2)
```c
#include "data_loader.h"  // ❌ Usando skill_data_t y funciones de data
```

**Problema:** `combat` depende de `data` pero su CMakeLists solo declara `REQUIRES pet`.

---

## Problema Crítico #2: CMakeLists.txt Incompletos

### `combat/CMakeLists.txt`
```cmake
idf_component_register(SRCS "combat_engine.c" "enemy.c"
                       INCLUDE_DIRS "."
                       REQUIRES pet)  # ❌ FALTA: data
```

**Falta declarar dependencia de `data` porque `combat_engine.c` usa:**
- `skill_data_t` (línea 141, 154)
- `skill_get_data()` (línea 141, 154)
- `profession_get_name()` (implícitamente vía pet.h)

### `data/CMakeLists.txt`
```cmake
idf_component_register(SRCS "data_loader.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_timer fatfs vfs spiffs)  # ❌ FALTA: cJSON
```

**Problema:** cJSON se agregó via `idf_component.yml` pero no en CMakeLists.txt.

### `hud/CMakeLists.txt`
```cmake
REQUIRES lvgl pet combat  # ❌ FALTA declarar dependencias correctamente
```

---

## Problema Crítico #3: Código Duplicado

### `sd_card.c` tiene función duplicada (líneas 97-185)

El archivo contiene DOS versiones de `storage_save_pet_full()`:
1. Líneas 97-152: Versión completa con skills y bonuses
2. Líneas 154-185: Versión anterior sin skills (usando variables no declaradas)

**La segunda versión usa variables inexistentes:**
```c
// Línea 161: 'name' no está definido
fprintf(f, " \"name\": \"%s\",\n", name);
```

### `enemy.c` tiene función vacía (líneas 11-14)
```c
void enemy_generate(enemy_t *enemy, uint8_t pet_level)
{
    if (!enemy) return;
    // ❌ Función vacía - la implementación real está en combat_engine.c
}
```

Pero `combat_engine.c` línea 238 también tiene `enemy_generate()` implementada.

---

## Problema Crítico #4: Definiciones de Tipos en Headers Incorrectos

### `pet_bonuses_t` estaba definido en dos lugares:
- `pet.h` (líneas 25-32) ✓ Correcto
- `data_loader.h` (líneas 73-80) ❌ Duplicado

**Estado:** Se movió a `pet.h` pero aún hay referencias cruzadas.

### `enemy_t` definido en `combat_engine.h` (líneas 35-45)
```c
typedef struct {
    char name[ENEMY_NAME_MAX_LEN];
    // ...
} enemy_t;
```

**Problema:** `enemy_t` está en `combat_engine.h` pero `enemy.h` también existe. Confusión sobre quién posee el tipo.

---

## Problema Crítico #5: Funciones de Persistencia en Lugar Incorrecto

### `storage_save_pet_full()` y `storage_load_pet_full()`

Estas funciones están en `storage/sd_card.c` pero:
1. Requieren conocer `pet_t` (crean dependencia circular)
2. Usan `data_loader.h` (otra dependencia)
3. Serializan JSON manualmente (no usan cJSON)

**Alternativas:**
- Mover a `pet.c` y que pet incluya storage
- Crear componente `persistence` que dependa de ambos
- Usar cJSON para serialización

---

## Análisis de Dependencias Correcto

### Grafo de dependencias ACTUAL (roto):
```
main → hud → pet → data → cJSON
        ↓        ↓
      combat    storage → pet (CICLO)
                 ↓
              usb_init
```

### Grafo de dependencias DESEADO:
```
main → hud ─────────┬─→ pet → data → cJSON
      │             │      ↓
      │             └─→ storage (genérico)
      │                    ↓
      └→ combat ──────────┘
      
usb_init → storage (genérico, sin pet)
```

---

## Recomendaciones de Refactoring

### 1. Separar responsabilidades de `storage`
- `sd_card.c` solo maneja SD genérica (mount, save file, load file)
- Crear `pet_persistence.c` en componente `pet` para guardar/cargar pet
- O mover lógica a `main.c`

### 2. Eliminar `data_loader.h` de headers públicos
- `pet.h` NO debe incluir `data_loader.h`
- Solo `pet.c` lo necesita (include privado)

### 3. Arreglar `enemy.h` / `combat_engine.h`
- Mover `enemy_t` a `enemy.h`
- `combat_engine.h` incluye `enemy.h`

### 4. Declarar todas las dependencias en CMakeLists
- `combat`: REQUIRES pet data
- `pet`: REQUIRES dna data
- `data`: PRIV_REQUIRES cjson
- `storage`: REQUIRES fatfs sdmmc driver (SIN pet)

### 5. Eliminar código muerto/duplicado
- Borrar función duplicada en `sd_card.c` (líneas 154-185)
- Implementar o borrar `enemy_generate()` vacío en `enemy.c`

---

## Archivos Afectados

| Archivo | Problemas | Prioridad |
|---------|-----------|-----------|
| `storage/sd_card.h` | Incluye pet.h | CRÍTICA |
| `storage/sd_card.c` | Código duplicado, depende de pet | CRÍTICA |
| `pet/pet.h` | Incluye data_loader.h | CRÍTICA |
| `combat/combat_engine.c` | Usa data sin declarar dependencia | ALTA |
| `combat/enemy.c` | Función vacía | MEDIA |
| `*/CMakeLists.txt` | Dependencias no declaradas | ALTA |
