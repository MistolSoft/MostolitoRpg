# Auditoría - Módulo Pet

## Resumen
El módulo `pet` tiene **dependencias cruzadas** y mezcla responsabilidades.

---

## Archivos del Módulo

| Archivo | Líneas | Propósito |
|---------|--------|-----------|
| `pet.h` | 118 | Header con tipos y funciones |
| `pet.c` | 461 | Implementación de lógica de pet |
| `CMakeLists.txt` | 3 | Configuración de build |

---

## Problema #1: Header incluye `dna_parser.h`

### `pet.h` línea 6:
```c
#include "dna_parser.h"  // ❌ Crea dependencia
```

### Por qué está:
- `pet_generate(pet_t *pet, const dna_t *dna)` recibe `dna_t*`
- `pet_level_up(pet_t *pet, const dna_t *dna)` recibe `dna_t*`

### Problema:
- `pet.h` necesita conocer `dna_t` para declarar funciones
- Pero `dna_t` está definido en `dna_parser.h`
- Crea dependencia `pet → dna`

### Solución:
- **Forward declaration** en `pet.h`:
```c
#ifndef PET_H
#define PET_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Forward declaration - NO incluye el header completo
struct dna_s;
typedef struct dna_s dna_t;

// Ahora las funciones pueden usar dna_t*
esp_err_t pet_generate(pet_t *pet, const dna_t *dna);
```

Y en `pet.c`:
```c
#include "pet.h"
#include "dna_parser.h"  // Include completo solo en .c
```

---

## Problema #2: `pet.c` incluye `data_loader.h`

### Línea 2:
```c
#include "data_loader.h"  // ❌ Dependencia no obvia
```

### Uso:
- `stat_interval_t` (líneas 127, 128)
- `damage_bonus_t` (línea 168)
- `skill_data_t` (líneas 193, etc.)
- `level_table_get_stat_interval()` (línea 128)
- `level_table_get_damage_progression()` (línea 171)
- `skills_get_for_level()` (línea 196)
- `skill_get_data()` (líneas 407, 421, 434, 451)
- `calculate_skill_success_probability()` (línea 219)
- `roll_skill_success()` (línea 223)

### Problema:
- `pet.c` tiene dependencia fuerte de `data`
- El CMakeLists.txt debe declarar `REQUIRES dna data`

---

## Problema #3: `pet_bonuses_t` duplicado

### Estado:
- `pet.h` líneas 25-32: Definición correcta
- `data_loader.h` líneas 73-80: Duplicado (ya marcado para eliminar)

### Solución:
Ya se movió a `pet.h`, pero hay que verificar que `data_loader.h` no lo tenga.

---

## Problema #4: Mezcla de responsabilidades

### `pet.c` hace demasiadas cosas:
1. Gestión de pet básico (generate, reset) ✓ Correcto
2. Sistema de niveles (level_up, add_exp) ✓ Correcto
3. Gestión de HP (take_damage, heal, death) ✓ Correcto
4. Sistema de energía (consume, rest) ✓ Correcto
5. **Sistema de skills** (add, use, restore) ❌ Debería ser componente aparte
6. **Aplicación de bonuses** ❌ Depende de `data_loader`

### Funciones de skills (líneas 363-461):
```c
bool pet_has_skill(pet_t *pet, uint8_t skill_id);
esp_err_t pet_add_skill(pet_t *pet, uint8_t skill_id, uint8_t uses_max);
esp_err_t pet_use_skill(pet_t *pet, uint8_t skill_index);
void pet_restore_combat_skills(pet_t *pet);
void pet_restore_rest_skills(pet_t *pet);
int8_t pet_get_skill_damage_bonus(pet_t *pet, uint8_t skill_index);
uint8_t pet_get_skill_crit_bonus(pet_t *pet, uint8_t skill_index);
```

### Alternativa:
Crear componente `skill_system` con estas funciones.

---

## Problema #5: Uso de funciones externas sin verificar

### Línea 335 en `sd_card.c` (llamado desde storage_load_pet_full):
```c
pet->exp_to_next = pet_get_exp_for_level(pet->level + 1);
```

### Problema:
- `storage` llama a función de `pet`
- Crea dependencia circular

---

## Problema #6: Constantes mágicas

### Líneas 10-21:
```c
static const uint32_t exp_table[] = {
    0, 100, 250, 500, 1000, 2000, 4000, 8000, 16000, 32000
};

static const uint16_t hp_bonus_table[] = {
    0, 5, 5, 10, 10, 15, 15, 20, 20, 25
};

static const char *pet_names[] = {
    "Mistolito", "Nibbler", "Pixel", "Glitch", "Neo",
    "Byte", "Sparky", "Luma", "Vex", "Zyx"
};
```

### Problema:
- Datos hardcodeados que deberían venir de `game_tables.json`
- `exp_table` y `hp_bonus_table` deberían estar en JSON
- `pet_names` podría configurarse externamente

---

## Problema #7: Funciones no usadas declaradas en header

### `pet.h` líneas 113-116:
```c
int8_t pet_get_skill_damage_bonus(pet_t *pet, uint8_t skill_index);
uint8_t pet_get_skill_crit_bonus(pet_t *pet, uint8_t skill_index);
```

### Análisis:
Estas funciones son usadas por `combat_engine.c`, así que deben permanecer públicas.

---

## Problema #8: Falta de documentación de invariantes

### Ejemplo:
```c
void pet_take_damage(pet_t *pet, int16_t damage)
{
    if (!pet) return;  // ¿Qué pasa si damage < 0?
    
    pet->hp -= damage;
    if (pet->hp < 0) {
        pet->hp = 0;
    }
    
    if (pet->hp == 0) {
        pet->is_alive = false;  // ¿Quién llama a pet_death()?
    }
}
```

### Problema:
- No está claro el flujo de muerte del pet
- `pet_death()` (línea 300) incrementa DP pero no se llama automáticamente
- Confusión sobre quién dispara qué

---

## Análisis de Depencencias CMakeLists

### Actual:
```cmake
idf_component_register(SRCS "pet.c"
                       INCLUDE_DIRS "."
                       REQUIRES dna)  # ❌ FALTA: data
```

### Necesario:
```cmake
idf_component_register(SRCS "pet.c"
                       INCLUDE_DIRS "."
                       REQUIRES dna data)
```

Pero si `dna.h` se cambia a forward declaration, solo:
```cmake
idf_component_register(SRCS "pet.c"
                       INCLUDE_DIRS "."
                       REQUIRES data
                       PRIV_REQUIRES dna)
```

---

## Checklist de Correcciones

- [ ] Usar forward declaration para `dna_t` en `pet.h`
- [ ] Agregar `data` a REQUIRES en CMakeLists.txt
- [ ] Verificar que `pet_bonuses_t` no esté en `data_loader.h`
- [ ] Considerar mover sistema de skills a componente separado
- [ ] Mover tablas de EXP/HP a `game_tables.json`
- [ ] Documentar flujo de muerte del pet
- [ ] Documentar invariantes de cada función

---

## Dependencias del Módulo

### Estado Actual:
```
pet → dna
    → data → cJSON
```

### Problema:
`data` no está declarado en CMakeLists.txt.

### Header público expone:
- `pet_t` (struct completo)
- `pet_bonuses_t` (struct completo)
- `pet_skill_slot_t` (struct completo)
- `dna_t*` en funciones (requiere forward declaration)
