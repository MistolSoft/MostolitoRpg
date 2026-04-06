# Auditoría - Módulo Combat

## Resumen
El módulo `combat` tiene **3 archivos** con **problemas de organización y dependencias**.

---

## Archivos del Módulo

| Archivo | Líneas | Propósito |
|---------|--------|-----------|
| `combat_engine.h` | 69 | Header con tipos y declaraciones |
| `combat_engine.c` | 260 | Lógica de combate |
| `enemy.h` | 13 | Header de enemigos |
| `enemy.c` | 46 | Generación de enemigos (vacío) |
| `CMakeLists.txt` | 3 | Configuración de build |

---

## Problema #1: `enemy_t` en archivo incorrecto

### Ubicación actual
`combat_engine.h` líneas 35-45:
```c
typedef struct {
    char name[ENEMY_NAME_MAX_LEN];
    int16_t hp;
    int16_t hp_max;
    uint8_t ac;
    uint8_t attack_bonus;
    uint8_t damage_dice;
    uint8_t damage_bonus;
    uint8_t level;
    uint16_t exp_reward;
} enemy_t;
```

### Problema
- `enemy_t` está definido en `combat_engine.h`
- Pero existe `enemy.h` que NO tiene la definición
- `enemy.c` tiene función vacía que se implementa en `combat_engine.c`

### Recomendación
Mover `enemy_t` a `enemy.h`:
```c
// enemy.h
#ifndef ENEMY_H
#define ENEMY_H

#include <stdint.h>

#define ENEMY_NAME_MAX_LEN 16

typedef struct {
    char name[ENEMY_NAME_MAX_LEN];
    int16_t hp;
    int16_t hp_max;
    uint8_t ac;
    uint8_t attack_bonus;
    uint8_t damage_dice;
    uint8_t damage_bonus;
    uint8_t level;
    uint16_t exp_reward;
} enemy_t;

void enemy_generate(enemy_t *enemy, uint8_t pet_level);
const char *enemy_get_name(uint8_t enemy_type);
uint32_t enemy_get_exp_reward(const enemy_t *enemy);

#endif
```

---

## Problema #2: Dependencia de `data_loader.h` no declarada

### Código afectado
`combat_engine.c` línea 2:
```c
#include "data_loader.h"
```

### Uso de `data_loader` en `combat_engine.c`:
- Línea 141: `skill_get_data()` - obtiene datos de skill
- Línea 154: `skill_get_data()` - obtiene datos de skill
- `skill_data_t` tipo usado en líneas 140, 153

### CMakeLists.txt actual:
```cmake
idf_component_register(SRCS "combat_engine.c" "enemy.c"
                       INCLUDE_DIRS "."
                       REQUIRES pet)  # ❌ FALTA: data
```

### CMakeLists.txt correcto:
```cmake
idf_component_register(SRCS "combat_engine.c" "enemy.c"
                       INCLUDE_DIRS "."
                       REQUIRES pet data)
```

---

## Problema #3: Función duplicada `enemy_generate()`

### `enemy.c` líneas 11-14:
```c
void enemy_generate(enemy_t *enemy, uint8_t pet_level)
{
    if (!enemy) return;
    // ❌ Vacío - no hace nada
}
```

### `combat_engine.c` líneas 238-260:
```c
void enemy_generate(enemy_t *enemy, uint8_t pet_level)
{
    if (!enemy) return;

    uint8_t enemy_type = (uint8_t)(esp_random() % 15);
    // ... implementación completa ...
}
```

### Análisis
- Dos funciones con el mismo nombre
- La de `enemy.c` está vacía (solo hace return)
- La de `combat_engine.c` tiene la implementación real
- El linker probablemente usa una u otra arbitrariamente

### Recomendación
Eliminar la función vacía de `enemy.c` y mantener solo la de `combat_engine.c`, O mover la implementación completa a `enemy.c`.

---

## Problema #4: `TAG` sin usar en `enemy.c`

### Código:
```c
static const char *TAG = "ENEMY";  // ❌ Nunca usado
```

No hay llamadas a `ESP_LOGx` que usen este TAG.

---

## Problema #5: Lógica de combate acoplada a skills

### `combat_engine.c` líneas 134-158:
```c
int8_t skill_idx = find_best_damage_skill(pet);
if (skill_idx >= 0) {
    int8_t skill_bonus = pet_get_skill_damage_bonus(pet, skill_idx);
    if (skill_bonus > 0) {
        damage += skill_bonus;
        pet_use_skill(pet, skill_idx);
        skill_data_t skill;
        if (skill_get_data(pet->skills[skill_idx].skill_id, &skill) == ESP_OK) {
            ESP_LOGI(TAG, "Used skill %s: +%d damage", skill.name, skill_bonus);
        }
    }
}
```

### Problema
- El combate conoce detalles internos de cómo funcionan los skills
- Crea dependencia con `data_loader.h`
- Viola principio de responsabilidad única

### Alternativa
El sistema de combate debería llamar a una función abstracta como:
```c
int16_t damage = pet_calculate_damage(pet, enemy);
```

Y que `pet.c` maneje internamente los skills y bonuses.

---

## Problema #6: Profesiones hardcodeadas

### Líneas 98-126:
```c
switch (pet->profession_id) {
case PROFESSION_WARRIOR:
    base_damage = roll_multiple_dice(dice_count, dice_sides);
    break;
case PROFESSION_MAGE:
    dice_count = 1 + pet->bonuses.extra_dice;
    base_damage = roll_multiple_dice(dice_count, 20);
    break;
case PROFESSION_ROGUE:
    // ... lógica compleja ...
    break;
default:
    base_damage = (uint8_t)((esp_random() % 15) + 1);
    break;
}
```

### Problema
- Lógica de daño por profesión hardcodeada
- Debería venir de `game_tables.json`
- Difícil de mantener y extender

### Recomendación
Mover esta lógica a `data_loader` y obtener fórmulas de daño del JSON.

---

## Dependencias del Módulo

### Dependencias actuales (reales):
```
combat → pet → dna
        ↓
       data (NO DECLARADA)
```

### Dependencias declaradas:
```cmake
REQUIRES pet  # ❌ Falta: data
```

---

## Checklist de Correcciones

- [ ] Mover `enemy_t` a `enemy.h`
- [ ] Eliminar función duplicada `enemy_generate()`
- [ ] Agregar `data` a REQUIRES en CMakeLists.txt
- [ ] Eliminar `TAG` sin usar en `enemy.c`
- [ ] Considerar mover lógica de profesiones a `data_loader`
- [ ] Considerar refactor para desacoplar skills del combate
