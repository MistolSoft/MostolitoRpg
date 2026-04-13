# Plan: Sistema de Profesiones, Skills y Perks

## Resumen del Sistema

El sistema usa **dos niveles separados**:

| Nivel | Qué controla | Rango |
|-------|--------------|-------|
| `pet.level` | Enemigos tier, HP base, EXP next, progreso general | Infinito |
| `pet.profession_level` | Bonus de profesión (stats, dados, skills, perks) | 1-20 (cicla) |

### Flujo de Niveles

```
Pet sube de nivel (EXP >= EXP_NEXT)
        │
        ├── pet.level++ (siempre sube)
        │
        ├── pet.profession_level++ (si tiene profesión)
        │   │
        │   └── Si profession_level > 20 → profession_level = 1 (reinicia ciclo)
        │
        └── Aplicar bonus según profession_level actual
```

**Ejemplo:**
- Pet lvl 45, Warrior profesión lvl 8 → aplica bonus nivel 8 de Warrior
- Pet lvl 45, Warrior profesión lvl 20 → aplica bonus máximo (nivel 20)
- Pet lvl 46, Warrior profesión lvl 1 → reinició ciclo, bonus base

---

## 1. Sistema de Stats por Nivel

### Concepto Base

Todos los niveles permiten subir stats, pero la cantidad y tipo depende de la profesión:

| Profesión | Stats Base (Novice) | Stats Extra (Profesión) | Total máximo |
|-----------|---------------------|-------------------------|--------------|
| Novice    | 2 stats cualquiera  | -                       | 2            |
| Warrior   | 2 stats cualquiera  | +1 STR o CON            | 3            |
| Mage      | 2 stats cualquiera  | +1 INT o WIS            | 3            |
| Rogue     | 2 stats cualquiera  | +1 DEX o INT            | 3            |

### Ejemplo: Guerrero profesión nivel 4

1. **Tirada base (Novice)**: Puede elegir 2 stats cualquiera
2. **Tirada profesión (Warrior)**: Puede hacer +1 tirada extra para STR o CON

Si el guerrero tiene candidatos: STR, DEX, CON:
- Novice: elige 2 (ej: STR y DEX suben)
- Warrior: puede intentar subir STR o CON adicional
- Resultado posible: STR+1, DEX+1, STR+1 (otra vez) = STR subió 2, DEX subió 1

### Fórmula de Tirada de Stats

```c
bool try_stat_increase(pet_t *pet, uint8_t stat_idx, bool has_class_bonus)
{
    // Tirada base: d20 >= 10 para subir
    uint8_t roll = roll_d20();
    
    // Con bonus de clase: ventaja (elegir mayor de 2d20)
    if (has_class_bonus) {
        uint8_t roll2 = roll_d20();
        roll = (roll > roll2) ? roll : roll2;
    }
    
    return roll >= 10;
}
```

---

## 2. Nivel de Profesión y Bonus

### Estructura

El `profession_level` determina qué bonus de la tabla de profesión se aplican:

```
profession_level 1-20: bonus normales
profession_level = 21: reinicia a 1 (nuevo ciclo)
```

### Tabla de Bonus por Profesión

**Warrior (niveles de profesión):**

| Nivel Prof. | Stats Extra | Bonus |
|-------------|-------------|-------|
| 4 | STR/CON advantage | +1 min_damage |
| 6 | +1 STR roll | - |
| 8 | CON disadvantage DEX | +1 max_damage |
| 10 | - | +1 extra_dice |
| 12 | STR/CON advantage | +1 min_damage |
| 14 | +1 STR roll | - |
| 16 | CON disadvantage DEX | +1 max_damage |
| 20 | - | +1 extra_dice |

**Mage (niveles de profesión):**

| Nivel Prof. | Stats Extra | Bonus |
|-------------|-------------|-------|
| 4 | INT/WIS advantage | +1 max_damage |
| 5 | - | +1 skill_uses |
| 8 | +1 INT roll | +1 extra_dice |
| 10 | - | +2 skill_uses |
| 12 | INT/WIS advantage | +1 max_damage |
| 14 | +1 INT roll | - |
| 16 | WIS disadvantage DEX | +1 extra_dice |
| 20 | - | +2 skill_uses |

**Rogue (niveles de profesión):**

| Nivel Prof. | Stats Extra | Bonus |
|-------------|-------------|-------|
| 4 | DEX/INT advantage | +5% crit |
| 5 | - | +1 skill_uses |
| 8 | +1 DEX roll | +1 sneak_dice |
| 10 | - | +10% crit |
| 12 | DEX/INT advantage | +5% crit |
| 14 | +1 DEX roll | - |
| 16 | INT disadvantage CON | +1 sneak_dice |
| 20 | - | +15% crit |

---

## 3. Sistema de Profesiones

### Estructura de Profesiones

```json
{
  "professions": [
    {
      "id": 0,
      "name": "Novice",
      "description": "Clase base sin especialización",
      "bonus_stats": [],
      "base_hp": 20,
      "base_energy": 10,
      "base_ac": 12
    },
    {
      "id": 1,
      "name": "Warrior",
      "description": "Especialista en combate cuerpo a cuerpo",
      "req": {"str": 14, "con": 12},
      "dp_cost": 10,
      "success_dc": 10,
      "bonus_stats": ["str", "con"],
      "base_hp": 25,
      "base_energy": 10,
      "base_ac": 14
    },
    {
      "id": 2,
      "name": "Mage",
      "description": "Maestro de las artes arcanas",
      "req": {"int": 14, "wis": 12},
      "dp_cost": 10,
      "success_dc": 10,
      "bonus_stats": ["int", "wis"],
      "base_hp": 16,
      "base_energy": 12,
      "base_ac": 10
    },
    {
      "id": 3,
      "name": "Rogue",
      "description": "Especialista en sigilo y ataques precisos",
      "req": {"dex": 14, "int": 10},
      "dp_cost": 10,
      "success_dc": 10,
      "bonus_stats": ["dex", "int"],
      "base_hp": 18,
      "base_energy": 10,
      "base_ac": 12
    }
  ]
}
```

### Flujo de Cambio de Profesión

```
LEVEL UP completado
        │
        ▼
┌─────────────────────────────┐
│ ¿Cumple requisitos de       │
│ alguna profesión nueva?     │
└─────────────────────────────┘
        │
        ├─ NO ──► Fin del level up
        │
        ▼ SI
        │
┌─────────────────────────────┐
│ ¿Cumple MÁS de una?         │
│                             │
│ SI ──► Random choice        │
│ NO  ──► Única opción        │
└─────────────────────────────┘
        │
        ▼
┌─────────────────────────────┐
│ ¿Tiene DP suficientes?      │
│                             │
│ NO ──► Fin (no puede cambiar)│
│ SI ──► Continuar            │
└─────────────────────────────┘
        │
        ▼
┌─────────────────────────────┐
│ TIRADA DE SUERTE            │
│                             │
│ d20 >= success_dc (10)      │
│                             │
│ ÉXITO ──► Cambia profesión  │
│ FALLO  ──► Consume DP       │
└─────────────────────────────┘
        │
        ▼ (si cambió)
┌─────────────────────────────┐
│ Aplicar nueva profesión:    │
│ - Leer tabla de profesión   │
│ - Aplicar modificadores     │
│ - Revisar skills disponibles│
└─────────────────────────────┘
```

### Código de Cambio de Profesión

```c
void try_profession_change(pet_t *pet, uint8_t new_level)
{
    // Solo puede ocurrir en level up
    if (pet->profession_id != 0) {
        return; // Ya tiene profesión (por ahora no hay multiclase)
    }
    
    // Buscar profesiones disponibles
    uint8_t available[4];
    uint8_t count = 0;
    
    for (uint8_t i = 1; i < profession_count; i++) {
        profession_t *prof = &professions[i];
        if (check_profession_requirements(pet, prof)) {
            available[count++] = i;
        }
    }
    
    if (count == 0) return;
    
    // Elegir profesión (random si hay varias)
    uint8_t chosen_idx = (count > 1) ? (esp_random() % count) : 0;
    uint8_t chosen_prof = available[chosen_idx];
    profession_t *prof = &professions[chosen_prof];
    
    // Verificar DP
    if (pet->dp < prof->dp_cost) {
        ESP_LOGI(TAG, "No tiene DP para %s (necesita %d)", 
                 prof->name, prof->dp_cost);
        return;
    }
    
    // Tirada de suerte
    uint8_t roll = roll_d20();
    if (roll >= prof->success_dc) {
        // ÉXITO: Cambiar profesión
        pet->profession_id = chosen_prof;
        pet->dp -= prof->dp_cost;
        apply_profession_base(pet, prof);
        ESP_LOGI(TAG, "¡Profesión adquirida: %s! (roll=%d)", 
                 prof->name, roll);
        
        // Revisar skills disponibles
        try_learn_skills(pet, new_level);
    } else {
        // FALLO: Consumir DP igualmente
        pet->dp -= prof->dp_cost;
        ESP_LOGI(TAG, "Fallo al adquirir %s (roll=%d, DC=%d)", 
                 prof->name, roll, prof->success_dc);
    }
}
```

---

## 4. Sistema de Skills y Perks

### Concepto

Las skills y perks se desbloquean según el **profession_level** y los **stats del pet**:
- **Máximo 2 por level up** (combinados)
- **Tirada de éxito** para cada una
- Requisitos: profession_level + stats base (STR, DEX, CON, INT, WIS, CHA)

### Requisitos Simplificados

Skills y perks solo usan:
- `profession_level_req`: Nivel de profesión necesario (1-20)
- `stat_req`: Stats base mínimos (ej: STR 12)

**NO usan** `level_req` de pet ni `damage_base` como requisito.

### Estructura de Skills

```json
{
  "skills": [
    {
      "id": 0,
      "name": "Tackle",
      "profession_level_req": 3,
      "profession_req": [0, 1],
      "stat_req": {"str": 12},
      "dp_cost": 5,
      "success_dc": 8,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 3,
      "effect": {
        "damage_base": 3,
        "damage_per_level": 1
      }
    },
    {
      "id": 1,
      "name": "Power Strike",
      "profession_level_req": 5,
      "profession_req": [1],
      "stat_req": {"str": 14},
      "dp_cost": 8,
      "success_dc": 12,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 2,
      "effect": {
        "damage_base": 5,
        "damage_per_level": 2
      }
    }
  ]
}
```

### Estructura de Perks (Pasivos)

```json
{
  "perks": [
    {
      "id": 0,
      "name": "Tough Skin",
      "profession_level_req": 4,
      "profession_req": [1],
      "stat_req": {"con": 14},
      "dp_cost": 6,
      "success_dc": 10,
      "type": "passive",
      "effect": {
        "ac_bonus": 1
      }
    },
    {
      "id": 1,
      "name": "Arcane Mind",
      "profession_level_req": 5,
      "profession_req": [2],
      "stat_req": {"int": 14},
      "dp_cost": 7,
      "success_dc": 11,
      "type": "passive",
      "effect": {
        "energy_max_bonus": 2
      }
    },
    {
      "id": 2,
      "name": "Quick Reflexes",
      "profession_level_req": 4,
      "profession_req": [3],
      "stat_req": {"dex": 14},
      "dp_cost": 6,
      "success_dc": 10,
      "type": "passive",
      "effect": {
        "crit_bonus": 5
      }
    }
  ]
}
```

### Flujo de Aprendizaje de Skills/Perks

```
Después de cambio de profesión (o level up con profesión)
        │
        ▼
┌─────────────────────────────┐
│ Filtrar skills/perks        │
│ disponibles por:            │
│ - Nivel actual              │
│ - Profesión                 │
│ - Stats                     │
└─────────────────────────────┘
        │
        ▼
┌─────────────────────────────┐
│ ¿Tiene DP para alguna?      │
│                             │
│ NO ──► Fin                  │
│ SI ──► Crear lista          │
└─────────────────────────────┘
        │
        ▼
┌─────────────────────────────┐
│ Seleccionar máximo 2        │
│ (random de la lista)        │
└─────────────────────────────┘
        │
        ▼
┌─────────────────────────────┐
│ Por cada una:               │
│                             │
│ d20 >= success_dc?          │
│                             │
│ ÉXITO ──► Aprende           │
│ FALLO  ──► DP consumido     │
└─────────────────────────────┘
```

### Código de Aprendizaje

```c
void try_learn_skills_and_perks(pet_t *pet, uint8_t new_level)
{
    // Filtrar skills disponibles
    skill_t *available_skills[16];
    uint8_t skill_count = 0;
    
    for (uint8_t i = 0; i < skill_count_total; i++) {
        skill_t *s = &skills[i];
        if (skill_check_requirements(pet, s, new_level)) {
            available_skills[skill_count++] = s;
        }
    }
    
    // Filtrar perks disponibles
    perk_t *available_perks[16];
    uint8_t perk_count = 0;
    
    for (uint8_t i = 0; i < perk_count_total; i++) {
        perk_t *p = &perks[i];
        if (perk_check_requirements(pet, p, new_level)) {
            available_perks[perk_count++] = p;
        }
    }
    
    // Combinar y seleccionar máximo 2
    uint8_t total_available = skill_count + perk_count;
    if (total_available == 0) return;
    
    // Crear lista combinada con prioridad random
    learn_candidate_t candidates[32];
    uint8_t candidate_count = 0;
    
    for (uint8_t i = 0; i < skill_count; i++) {
        candidates[candidate_count].type = LEARN_TYPE_SKILL;
        candidates[candidate_count].ptr = available_skills[i];
        candidate_count++;
    }
    for (uint8_t i = 0; i < perk_count; i++) {
        candidates[candidate_count].type = LEARN_TYPE_PERK;
        candidates[candidate_count].ptr = available_perks[i];
        candidate_count++;
    }
    
    // Seleccionar hasta 2 (random)
    uint8_t to_learn = (candidate_count > 2) ? 2 : candidate_count;
    
    // Shuffle y tomar los primeros
    shuffle_array(candidates, candidate_count);
    
    for (uint8_t i = 0; i < to_learn; i++) {
        learn_candidate_t *c = &candidates[i];
        
        // Verificar DP
        uint8_t dp_cost = get_dp_cost(c);
        if (pet->dp < dp_cost) continue;
        
        // Tirada de éxito
        uint8_t dc = get_success_dc(c);
        uint8_t roll = roll_d20();
        
        if (roll >= dc) {
            // ÉXITO
            pet->dp -= dp_cost;
            apply_learnable(pet, c);
            ESP_LOGI(TAG, "¡Aprendido: %s! (roll=%d, DC=%d)", 
                     get_name(c), roll, dc);
        } else {
            // FALLO
            pet->dp -= dp_cost;
            ESP_LOGI(TAG, "Fallo al aprender: %s (roll=%d, DC=%d)", 
                     get_name(c), roll, dc);
        }
    }
}
```

---

## 5. Flujo Completo de Level Up

```
EXP >= EXP_NEXT
        │
        ▼
┌─────────────────────────────────────┐
│ 1. INCREMENTAR NIVEL                │
│    pet->level++                     │
│    pet->exp = 0                     │
│    pet->exp_next = calc_exp(level)  │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 2. INCREMENTAR NIVEL PROFESIÓN      │
│    if (pet->profession_id > 0)      │
│      pet->profession_level++        │
│      if (profession_level > 20)     │
│        profession_level = 1         │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 3. SUBIR STATS BASE (NOVICE)        │
│    - Obtener candidatos del ADN     │
│    - Seleccionar 2 stats            │
│    - Tirada d20 >= 10 para subir    │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 4. SUBIR STATS PROFESIÓN            │
│    (si tiene profesión)             │
│    - Verificar stat_intervals       │
│    - Si profession_level coincide   │
│    - Tirada con ventaja             │
│    - Se suma al total               │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 5. APLICAR BONUS PROFESIÓN          │
│    - Verificar damage_progression   │
│    - Si profession_level coincide   │
│    - Aplicar bonus correspondiente  │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 6. VERIFICAR CAMBIO PROFESIÓN       │
│    - ¿Cumple requisitos?            │
│    - ¿Tiene DP?                     │
│    - Tirada de suerte               │
│    - Si éxito: cambiar y aplicar    │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 7. APRENDER SKILLS/PERKS            │
│    - Filtrar por profession_level   │
│    - Filtrar por stats              │
│    - Seleccionar máx 2              │
│    - Tirada de éxito para cada una  │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 8. ACTUALIZAR HP/ENERGY             │
│    - Aplicar bonos de CON           │
│    - Aplicar perks pasivos          │
└─────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ 9. GUARDAR                          │
│    storage_save_pet_delta()         │
└─────────────────────────────────────┘
```

---

## 6. Estructura JSON Completa

### game_tables.json

```json
{
  "config": {
    "cycle_length": 20,
    "max_stats_per_level": 2,
    "max_skills_per_level": 2,
    "base_stat_dc": 10
  },
  
  "professions": [
    {
      "id": 0,
      "name": "Novice",
      "bonus_stats": [],
      "success_dc": 0,
      "dp_cost": 0,
      "base_hp": 20,
      "base_energy": 10,
      "base_ac": 12
    },
    {
      "id": 1,
      "name": "Warrior",
      "req": {"str": 14, "con": 12},
      "bonus_stats": ["str", "con"],
      "success_dc": 10,
      "dp_cost": 10,
      "base_hp": 25,
      "base_energy": 10,
      "base_ac": 14,
      "damage_dice": 6,
      "dice_count": 3
    },
    {
      "id": 2,
      "name": "Mage",
      "req": {"int": 14, "wis": 12},
      "bonus_stats": ["int", "wis"],
      "success_dc": 10,
      "dp_cost": 10,
      "base_hp": 16,
      "base_energy": 12,
      "base_ac": 10,
      "damage_dice": 20,
      "dice_count": 1
    },
    {
      "id": 3,
      "name": "Rogue",
      "req": {"dex": 14, "int": 10},
      "bonus_stats": ["dex", "int"],
      "success_dc": 10,
      "dp_cost": 10,
      "base_hp": 18,
      "base_energy": 10,
      "base_ac": 12,
      "damage_dice": 4,
      "dice_count": 3
    }
  ],
  
  "skills": [
    {
      "id": 0,
      "name": "Tackle",
      "profession_level_req": 3,
      "profession_req": [0, 1],
      "stat_req": {"str": 12},
      "dp_cost": 5,
      "success_dc": 8,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 3,
      "effect": {"damage_base": 3, "damage_per_level": 1}
    },
    {
      "id": 1,
      "name": "Power Strike",
      "profession_level_req": 5,
      "profession_req": [1],
      "stat_req": {"str": 14},
      "dp_cost": 8,
      "success_dc": 12,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 2,
      "effect": {"damage_base": 5, "damage_per_level": 2}
    },
    {
      "id": 2,
      "name": "Missile",
      "profession_level_req": 3,
      "profession_req": [0, 2],
      "stat_req": {"int": 12},
      "dp_cost": 5,
      "success_dc": 8,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 2,
      "effect": {"damage_base": 4, "damage_per_level": 2}
    },
    {
      "id": 3,
      "name": "Fireball",
      "profession_level_req": 5,
      "profession_req": [2],
      "stat_req": {"int": 14},
      "dp_cost": 8,
      "success_dc": 12,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 1,
      "effect": {"damage_base": 8, "damage_per_level": 3}
    },
    {
      "id": 4,
      "name": "Sneak Attack",
      "profession_level_req": 3,
      "profession_req": [0, 3],
      "stat_req": {"dex": 12},
      "dp_cost": 5,
      "success_dc": 8,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 2,
      "effect": {"crit_base": 15, "crit_per_level": 3}
    },
    {
      "id": 5,
      "name": "Backstab",
      "profession_level_req": 5,
      "profession_req": [3],
      "stat_req": {"dex": 14},
      "dp_cost": 8,
      "success_dc": 12,
      "type": "active",
      "uses_per": "combat",
      "uses_max": 1,
      "effect": {"crit_base": 30, "crit_per_level": 5}
    }
  ],
  
  "perks": [
    {
      "id": 0,
      "name": "Tough Skin",
      "profession_level_req": 4,
      "profession_req": [1],
      "stat_req": {"con": 14},
      "dp_cost": 6,
      "success_dc": 10,
      "type": "passive",
      "effect": {"ac_bonus": 1}
    },
    {
      "id": 1,
      "name": "Arcane Mind",
      "profession_level_req": 5,
      "profession_req": [2],
      "stat_req": {"int": 14},
      "dp_cost": 7,
      "success_dc": 11,
      "type": "passive",
      "effect": {"energy_max_bonus": 2}
    },
    {
      "id": 2,
      "name": "Quick Reflexes",
      "profession_level_req": 4,
      "profession_req": [3],
      "stat_req": {"dex": 14},
      "dp_cost": 6,
      "success_dc": 10,
      "type": "passive",
      "effect": {"crit_bonus": 5}
    },
    {
      "id": 3,
      "name": "Battle Hardened",
      "profession_level_req": 6,
      "profession_req": [1],
      "stat_req": {"con": 15},
      "dp_cost": 10,
      "success_dc": 13,
      "type": "passive",
      "effect": {"hp_max_bonus": 5}
    },
    {
      "id": 4,
      "name": "Spell Mastery",
      "profession_level_req": 6,
      "profession_req": [2],
      "stat_req": {"int": 15},
      "dp_cost": 10,
      "success_dc": 13,
      "type": "passive",
      "effect": {"skill_uses_bonus": 1}
    },
    {
      "id": 5,
      "name": "Shadow Steps",
      "profession_level_req": 6,
      "profession_req": [3],
      "stat_req": {"dex": 15},
      "dp_cost": 10,
      "success_dc": 13,
      "type": "passive",
      "effect": {"dodge_bonus": 2}
    }
  ],
  
  "enemies": [...],
  "enemy_tiers": [...]
}
```

---

## 7. Resumen de Reglas

### Niveles
| Campo | Uso | Rango |
|-------|-----|-------|
| `pet.level` | Tier de enemigos, HP base, EXP | Infinito |
| `pet.profession_level` | Bonus de profesión | 1-20 (cicla) |

### Stats
| Regla | Valor |
|-------|-------|
| Stats base por level (Novice) | 2 |
| Stats extra por profesión | +1 (stats específicos) |
| DC para subir stat | 10 |
| Bonus de profesión | Ventaja (2d20, elegir mayor) |

### Profesiones
| Regla | Valor |
|-------|-------|
| Requisitos | Stats mínimos |
| DP cost | 10 |
| DC de éxito | 10 |
| Consume DP en fallo | Sí |

### Skills/Perks
| Regla | Valor |
|-------|-------|
| Máximo por level up | 2 |
| Requisito nivel | `profession_level` (1-20) |
| Requisito stats | Stats base mínimos |
| DC de éxito | Variable (8-13) |
| Consume DP en fallo | Sí |

---

## 8. Preguntas Pendientes

### 1. ¿Puede cambiar de profesión múltiples veces?
**Propuesta actual:** No, una vez obtiene profesión permanece. (Podría implementarse multiclase en el futuro)

### 2. ¿Los skills se pierden al morir?
**Propuesta actual:** Sí, al morir el pet pierde todo y renace como Novice nivel 1 con nuevo ADN derivado.

### 3. ¿Puede aprender skills de otra profesión?
**Propuesta actual:** Solo si `profession_req` incluye Novice (id=0). Skills avanzadas son exclusivas.

### 4. ¿Qué pasa al reiniciar profession_level?
**Propuesta actual:** Al llegar a profession_level 21, se reinicia a 1. El pet puede reaprender skills/perks del nuevo ciclo, pero los ya aprendidos se mantienen.
