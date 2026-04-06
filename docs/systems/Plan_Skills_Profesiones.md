# Plan: Sistema de Profesiones, Skills y Escalado por Nivel

## Resumen del Sistema

Las **tablas JSON** son la única fuente de verdad. El pet consulta las tablas en cada level up para:
1. Subir stats según intervalos y modificadores
2. Aplicar bonificaciones de daño
3. Aprender skills con probabilidad de fallo

---

## Conceptos Clave

1. **Tablas = Funciones constantes**
   - Cada nivel tiene modificadores específicos
   - Cada profesión tiene sus propias reglas
   - El pet CONSULTA las tablas, no guarda bonuses

2. **Stats suben por intervalos regulares**
   - Cada profesión tiene: stats con ventaja/desventaja
   - Roll random con modificadores según tabla

3. **Skills con probabilidad de éxito inversa al nivel**
   - A mayor nivel, MENOS probabilidad de éxito (skills más difíciles de dominar)
   - Random choice si hay DP suficientes

---

## Archivo JSON: `/DATA/game_tables.json`

```json
{
  "config": {
    "cycle_length": 20,
    "cycle_multiplier": 0.5
  },
  "professions": [
    {"id": 0, "name": "Novice"},
    {"id": 1, "name": "Warrior", "req": {"attr": "str", "value": 14}, "dp_cost": 10},
    {"id": 2, "name": "Mage", "req": {"attr": "int", "value": 14}, "dp_cost": 10},
    {"id": 3, "name": "Rogue", "req": {"attr": "dex", "value": 14}, "dp_cost": 10}
  ],
  "level_tables": {
    "novice": {
      "stat_intervals": [
        {"level": 4, "stats": ["str", "con"], "advantage": [], "disadvantage": []},
        {"level": 8, "stats": ["dex", "wis"], "advantage": [], "disadvantage": []},
        {"level": 12, "stats": ["str", "con"], "advantage": [], "disadvantage": []},
        {"level": 16, "stats": ["dex", "int"], "advantage": [], "disadvantage": []}
      ],
      "damage_progression": [
        {"level": 4, "min_damage": 1},
        {"level": 8, "max_damage": 1},
        {"level": 12, "min_damage": 1},
        {"level": 16, "max_damage": 1}
      ]
    },
    "warrior": {
      "stat_intervals": [
        {"level": 4, "stats": ["str", "con"], "advantage": ["str"], "disadvantage": ["int"]},
        {"level": 6, "stats": ["str"], "advantage": ["str"], "disadvantage": []},
        {"level": 8, "stats": ["con"], "advantage": [], "disadvantage": ["dex"]},
        {"level": 12, "stats": ["str", "con"], "advantage": ["str"], "disadvantage": ["int"]},
        {"level": 14, "stats": ["str"], "advantage": ["str"], "disadvantage": []},
        {"level": 16, "stats": ["con"], "advantage": [], "disadvantage": ["dex"]}
      ],
      "damage_progression": [
        {"level": 4, "min_damage": 1},
        {"level": 5, "skill_uses": 1},
        {"level": 8, "max_damage": 1},
        {"level": 10, "extra_dice": 1},
        {"level": 12, "min_damage": 1},
        {"level": 15, "skill_uses": 1},
        {"level": 16, "max_damage": 1},
        {"level": 20, "extra_dice": 1}
      ]
    },
    "mage": {
      "stat_intervals": [
        {"level": 4, "stats": ["int", "wis"], "advantage": ["int"], "disadvantage": ["str"]},
        {"level": 6, "stats": ["int"], "advantage": ["int"], "disadvantage": []},
        {"level": 8, "stats": ["wis"], "advantage": [], "disadvantage": ["dex"]},
        {"level": 12, "stats": ["int", "wis"], "advantage": ["int"], "disadvantage": ["str"]},
        {"level": 14, "stats": ["int"], "advantage": ["int"], "disadvantage": []},
        {"level": 16, "stats": ["wis"], "advantage": [], "disadvantage": ["dex"]}
      ],
      "damage_progression": [
        {"level": 4, "max_damage": 1},
        {"level": 5, "skill_uses": 1},
        {"level": 8, "extra_dice": 1},
        {"level": 10, "skill_uses": 2},
        {"level": 12, "max_damage": 1},
        {"level": 15, "skill_uses": 1},
        {"level": 16, "extra_dice": 1},
        {"level": 20, "skill_uses": 2}
      ]
    },
    "rogue": {
      "stat_intervals": [
        {"level": 4, "stats": ["dex", "int"], "advantage": ["dex"], "disadvantage": ["str"]},
        {"level": 6, "stats": ["dex"], "advantage": ["dex"], "disadvantage": []},
        {"level": 8, "stats": ["int"], "advantage": [], "disadvantage": ["con"]},
        {"level": 12, "stats": ["dex", "int"], "advantage": ["dex"], "disadvantage": ["str"]},
        {"level": 14, "stats": ["dex"], "advantage": ["dex"], "disadvantage": []},
        {"level": 16, "stats": ["int"], "advantage": [], "disadvantage": ["con"]}
      ],
      "damage_progression": [
        {"level": 4, "crit": 5},
        {"level": 5, "skill_uses": 1},
        {"level": 8, "sneak_dice": 1},
        {"level": 10, "crit": 10},
        {"level": 12, "crit": 5},
        {"level": 15, "skill_uses": 1},
        {"level": 16, "sneak_dice": 1},
        {"level": 20, "crit": 15}
      ]
    }
  },
  "skills": [
    {"id": 0, "name": "Tackle", "level": 3, "profession": 1, "dp_cost": 5, "uses_per": "combat", "uses_max": 3, "effect": {"damage_base": 3, "damage_per_level": 1}},
    {"id": 1, "name": "Missile", "level": 3, "profession": 2, "dp_cost": 5, "uses_per": "combat", "uses_max": 2, "effect": {"damage_base": 4, "damage_per_level": 2}},
    {"id": 2, "name": "Sneak Attack", "level": 3, "profession": 3, "dp_cost": 5, "uses_per": "combat", "uses_max": 2, "effect": {"crit_base": 15, "crit_per_level": 3}},
    {"id": 3, "name": "Power Strike", "level": 5, "profession": 1, "dp_cost": 8, "uses_per": "combat", "uses_max": 2, "effect": {"damage_base": 5, "damage_per_level": 2}},
    {"id": 4, "name": "Fireball", "level": 5, "profession": 2, "dp_cost": 8, "uses_per": "combat", "uses_max": 1, "effect": {"damage_base": 8, "damage_per_level": 3}},
    {"id": 5, "name": "Backstab", "level": 5, "profession": 3, "dp_cost": 8, "uses_per": "combat", "uses_max": 1, "effect": {"crit_base": 30, "crit_per_level": 5}}
  ]
}
```

---

## Sistema de Stats con Advantage/Disadvantage

### Stat Intervals por Nivel

| Campo | Descripción |
|-------|-------------|
| `level` | A qué nivel se puede subir stats |
| `stats` | Qué stats pueden subir en ese nivel |
| `advantage` | Stats con ventaja (roll 2d20, elegir mayor) |
| `disadvantage` | Stats con desventaja (roll 2d20, elegir menor) |

### Ejemplo Warrior Nivel 4

```json
{"level": 4, "stats": ["str", "con"], "advantage": ["str"], "disadvantage": ["int"]}
```

**Proceso:**
1. Pet llega a nivel 4
2. Puede subir STR o CON
3. STR tiene ventaja (más probabilidad de subir)
4. Si quisiera subir INT (no está en stats), tendría desventaja (pero en este caso no puede subir INT en nivel 4)

### Fórmula para subir stat

```c
bool try_stat_increase(pet_t *pet, uint8_t stat_index, bool has_advantage, bool has_disadvantage)
{
    uint8_t roll1 = roll_d20();
    uint8_t roll2 = roll_d20();
    uint8_t final_roll;

    if (has_advantage) {
        final_roll = max(roll1, roll2);
    } else if (has_disadvantage) {
        final_roll = min(roll1, roll2);
    } else {
        final_roll = roll1;
    }

    // Probabilidad base: stat aumenta si roll >= 10
    // Con ventaja: más probable
    // Con desventaja: menos probable
    return final_roll >= 10;
}
```

---

## Sistema de Aprendizaje de Skills con Probabilidad de Éxito Inversa

### Fórmula

```
probabilidad_exito = 100 - (nivel_pet × 3)
```

**Ejemplos:**
- Nivel 3: 100 - 9 = 91% éxito
- Nivel 5: 100 - 15 = 85% éxito
- Nivel 10: 100 - 30 = 70% éxito
- Nivel 20: 100 - 60 = 40% éxito
- Nivel 30: 100 - 90 = 10% éxito

**Lógica:** A niveles altos, las skills son más poderosas y difíciles de dominar.

### Proceso de Aprendizaje

```c
void try_learn_skill(pet_t *pet, uint8_t *available_skills, uint8_t count)
{
    // Filtrar skills por DP disponible
    skill_data_t affordable[16];
    uint8_t affordable_count = 0;

    for (int i = 0; i < count; i++) {
        skill_data_t skill;
        skill_get_data(available_skills[i], &skill);
        if (pet->dp >= skill.dp_cost) {
            affordable[affordable_count++] = skill;
        }
    }

    if (affordable_count == 0) return;

    // Elegir skill random
    uint8_t idx = esp_random() % affordable_count;
    skill_data_t chosen = affordable[idx];

    // Calcular probabilidad de éxito (inversa al nivel)
    uint8_t success_rate = 100 - (pet->level * 3);
    if (success_rate < 5) success_rate = 5; // Mínimo 5% siempre

    // Roll
    uint8_t roll = esp_random() % 100;

    if (roll < success_rate) {
        // Éxito: aprender skill
        pet->dp -= chosen.dp_cost;
        add_skill_to_pet(pet, chosen.id);
        ESP_LOGI(TAG, "Skill aprendida: %s", chosen.name);
    } else {
        // Fallo
        ESP_LOGI(TAG, "Fallo al aprender skill: %s", chosen.name);
    }
}
```

---

## Flujo Completo de Level Up

```
pet_level_up(pet)
    │
    ├── 1. Aumentar nivel
    │
    ├── 2. Consultar tabla de stat_intervals
    │   ├── Para cada stat disponible en este nivel
    │   ├── Roll con advantage/disadvantage según tabla
    │   └── Si éxito → stat++
    │
    ├── 3. Consultar tabla de damage_progression
    │   ├── Aplicar bonus según nivel (min_damage, max_damage, extra_dice, etc.)
    │   └── Guardar en pet->bonuses
    │
    ├── 4. Consultar skills disponibles
    │   ├── Filtrar por nivel y profesión
    │   ├── Filtrar por DP disponible
    │   ├── Elegir skill random
    │   ├── Calcular probabilidad de éxito
    │   ├── Roll → ¿éxito?
    │   │   ├── Sí → Descontar DP, aprender skill
    │   │   └── No → No aprender, DP se conserva
    │
    └── 5. Guardar pet_data.json
```

---

## Fórmula de Repetición con Multiplicador (Ciclos Infinitos)

```
ciclo = floor((level - 1) / 20)
nivel_en_ciclo = ((level - 1) % 20) + 1
multiplicador = 1 + (ciclo × 0.5)
bonus_final = floor(bonus_base × multiplicador)
```

### Ejemplo Guerrero nivel 44

```
ciclo = floor(43 / 20) = 2
nivel_en_ciclo = (43 % 20) + 1 = 4
multiplicador = 1 + (2 × 0.5) = 2.0

Bonus nivel 4: min_damage: 1
Bonus aplicado: floor(1 × 2.0) = +2 min_damage
```

### Ejemplo Guerrero nivel 64

```
ciclo = floor(63 / 20) = 3
nivel_en_ciclo = (63 % 20) + 1 = 4
multiplicador = 1 + (3 × 0.5) = 2.5

Bonus nivel 4: min_damage: 1
Bonus aplicado: floor(1 × 2.5) = +2 min_damage
```

---

## Estructura del Pet (`pet_data.json`)

```json
{
  "name": "Mistolito",
  "level": 4,
  "exp": 0,
  "hp": 45,
  "hp_max": 45,
  "str": 15,
  "dex": 12,
  "con": 14,
  "int": 10,
  "wis": 8,
  "cha": 11,
  "profession_id": 1,
  "dp": 8,
  "enemies_defeated": 5,
  "lives": 1,
  "energy": 10,
  "bonuses": {
    "min_damage": 1,
    "max_damage": 0,
    "extra_dice": 0,
    "crit": 0,
    "sneak_dice": 0,
    "skill_uses": 1
  },
  "skills": [
    {"id": 0, "uses_remaining": 3}
  ]
}
```

---

## Resumen de Profesiones y Skills

### Profesiones

| ID | Nombre | Requisito | DP Costo |
|----|--------|-----------|----------|
| 0 | Novice | - | 0 |
| 1 | Warrior | STR ≥ 14 | 10 DP |
| 2 | Mage | INT ≥ 14 | 10 DP |
| 3 | Rogue | DEX ≥ 14 | 10 DP |

### Skills

| ID | Nombre | Profesión | Nivel | DP Costo | Usos | Efecto |
|----|--------|-----------|-------|----------|------|--------|
| 0 | Tackle | Warrior | 3 | 5 DP | 3/combate | +3 daño (+1/lvl) |
| 1 | Missile | Mage | 3 | 5 DP | 2/combate | +4 daño (+2/lvl) |
| 2 | Sneak Attack | Rogue | 3 | 5 DP | 2/combate | +15% crit (+3%/lvl) |
| 3 | Power Strike | Warrior | 5 | 8 DP | 2/combate | +5 daño (+2/lvl) |
| 4 | Fireball | Mage | 5 | 8 DP | 1/combate | +8 daño (+3/lvl) |
| 5 | Backstab | Rogue | 5 | 8 DP | 1/combate | +30% crit (+5%/lvl) |

---

## Plan de Implementación

### FASE 1: Crear archivos de datos
1. Crear `firmware/data/game_tables.json`

### FASE 2: Nuevo componente `data_loader`
2. Crear `firmware/components/data/CMakeLists.txt`
3. Crear `firmware/components/data/data_loader.h`
4. Crear `firmware/components/data/data_loader.c`

**Funciones principales:**
```c
esp_err_t data_loader_init(void);
esp_err_t level_table_get_stat_interval(uint8_t level, uint8_t profession, stat_interval_t *out);
esp_err_t level_table_get_damage_progression(uint8_t level, uint8_t profession, damage_bonus_t *out);
bool skill_check_available(uint8_t level, uint8_t profession, uint8_t *skill_ids, uint8_t *count);
esp_err_t skill_get_data(uint8_t skill_id, skill_data_t *out);
esp_err_t profession_get_req(uint8_t profession_id, profession_req_t *out);
```

### FASE 3: Modificar `pet.h` y `pet.c`
5. Agregar campos `bonuses` y `skills` dinámicos a `pet_t`
6. Modificar `pet_level_up()` para:
   - Consultar stat_intervals y hacer rolls con advantage/disadvantage
   - Consultar damage_progression y aplicar bonus
   - Consultar skills disponibles y intentar aprender con probabilidad de fallo
7. Crear función `pet_apply_level_bonus()` que calcula bonus según tabla
8. Eliminar tablas hardcodeadas

### FASE 4: Modificar `combat_engine.c`
9. Usar bonuses del pet para calcular daño con extra_dice, min_damage, max_damage
10. Aplicar efectos de skills activas

### FASE 5: Modificar `storage/sd_card.c`
11. Guardar/cargar campo `bonuses` en `pet_data.json`
12. Guardar/cargar array `skills` dinámico

### FASE 6: Integrar en `main.c`
13. Agregar `#include "data_loader.h"`
14. Llamar `data_loader_init()` después de montar SD

### FASE 7: Modificar `main/CMakeLists.txt`
15. Agregar dependencia `REQUIRES data`

### FASE 8: Probar flujo completo
16. Copiar archivo JSON a SD manualmente
17. Flashear y verificar:
   - Level up consulta stat_intervals
   - Stats suben con advantage/disadvantage
   - Bonuses se aplican correctamente
   - Skills se aprenden con DP y probabilidad de fallo
   - Daño escala correctamente

---

## Preguntas Pendientes

### 1. Probabilidad de éxito de skills

**Fórmula actual:** `probabilidad_exito = 100 - (nivel_pet × 3)`

- Nivel 3: 91% éxito
- Nivel 10: 70% éxito
- Nivel 30: 10% éxito (mínimo 5%)

**¿Está bien esta dificultad progresiva?** ¿O prefieres otros valores?

### 2. Stat intervals con advantage/disadvantage

¿Están bien definidos los intervalos y qué stats tienen ventaja/desventaja?

### 3. Proceso de level up completo

¿Falta algo en el flujo de level up?

### 4. Múltiples skills disponibles y DP limitado

¿Qué pasa si el pet tiene múltiples skills disponibles y DP para solo una?

- **Opción A:** Elige random entre las que puede pagar
- **Opción B:** Elige la más barata primero
- **Opción C:** Elige la más cara (mejor skill)

### 5. Múltiples skills por level up

¿Puede aprender múltiples skills en un mismo level up?

- **Opción A:** Solo 1 skill por level up
- **Opción B:** Todas las que pueda pagar y pasar el roll

---

## Decisión sobre Cambio de Profesión

Cuando el pet cambia de profesión:
- **Sigue la tabla de la nueva profesión** desde el nivel actual
- **Mantiene las bonificaciones acumuladas** de la profesión anterior
- **Las nuevas bonificaciones se suman** a las existentes

Ejemplo:
- Guerrero lvl 10: +2 min_damage, +1 max_damage, +1 extra_dice
- Cambia a Mage en lvl 10
- Mage lvl 12: +1 max_damage (se suma al existente)
- Total: +2 min_damage, +2 max_damage, +1 extra_dice
