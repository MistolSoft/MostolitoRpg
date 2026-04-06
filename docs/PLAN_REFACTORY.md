# Plan de Refactory Arquitectural - MistolitoRPG

## Objetivo

Crear `components_v2/` con arquitectura limpia sin ciclos de dependencias. El código actual en `components/` se mantiene intacto hasta que el nuevo funcione.

---

## Arquitectura Final

```
components_v2/
├── config/          # Constantes y configuración
├── core/            # Tipos base y reglas (dice rolls, modifiers)
├── storage/         # Operaciones de archivo genéricas
├── dna/             # Parser de DNA
├── data/            # Carga de game_tables.json
├── pet/             # Entidad pet (sin dependencias a data)
├── enemy/           # Entidad enemy (posee enemy_t)
├── combat/          # Motor de combate
└── ui/              # Interfaz LVGL
```

### Grafo de Dependencias

```
main
├── ui ──────────────────┬──> pet
│                        └──> combat
├── combat ──────────────┼──> pet
│                        └──> enemy
├── pet ─────────────────┴──> core
├── enemy ──────────────────> core
├── data ───────────────────┼──> storage
│                           └──> cJSON
├── dna ────────────────────> storage
└── config (base, sin dependencias internas)

core ──> config
```

**Sin ciclos. Cada capa solo depende de capas inferiores.**

---

## Fase 1: Setup Inicial

**Duración**: 15 minutos

### Tareas

| # | Tarea | Comando/Acción |
|---|-------|----------------|
| 1.1 | Crear directorio components_v2 | `mkdir firmware/components_v2` |
| 1.2 | Crear subdirectorios | `mkdir config core pet enemy combat data storage dna ui` dentro de components_v2 |
| 1.3 | Backup del CMakeLists actual | Copiar `firmware/CMakeLists.txt` a `firmware/CMakeLists.txt.bak` |

### Estructura de Directorios

```
firmware/
├── components/          # Código antiguo (NO TOCAR)
├── components_v2/       # Código nuevo
│   ├── config/
│   ├── core/
│   ├── storage/
│   ├── dna/
│   ├── data/
│   ├── pet/
│   ├── enemy/
│   ├── combat/
│   └── ui/
└── main/
```

---

## Fase 2: Config

**Duración**: 30 minutos

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `config/config.h` | Constantes de rutas, límites, GPIOs |
| `config/config.c` | Vacío (para futuro: carga desde Kconfig) |
| `config/CMakeLists.txt` | `idf_component_register()` |

### config.h - Constantes

```
RUTAS:
- STORAGE_MOUNT_POINT = "/sdcard"
- PET_DATA_PATH = "/sdcard/BRAIN/PET/pet_data.json"
- GAME_TABLES_PATH = "/sdcard/DATA/game_tables.json"
- DNA_FILE_PATH = "/sdcard/DATA/dna.json"

LIMITES:
- PET_MAX_LEVEL = 10
- PET_MAX_ENERGY = 10
- PET_MAX_SKILLS = 32
- ENEMY_NAME_MAX_LEN = 16
- PET_NAME_MAX_LEN = 16

GPIOs (SD Card):
- SD_MISO_GPIO = 40
- SD_MOSI_GPIO = 38
- SD_CLK_GPIO = 39
- SD_CS_GPIO = 41
```

### Dependencias

Ninguna. `config` es la base.

---

## Fase 3: Core (Types + Rules)

**Duración**: 45 minutos

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `core/types.h` | Enums base: `stat_e`, `profession_e`, `pet_state_e` |
| `core/rules.h` | Declaraciones de funciones de dados y modifiers |
| `core/rules.c` | Implementación de rolls, modifiers, probabilidades |
| `core/CMakeLists.txt` | `idf_component_register(REQUIRES config)` |

### types.h - Enums

```
stat_e:
- STAT_STR = 0
- STAT_DEX
- STAT_CON
- STAT_INT
- STAT_WIS
- STAT_CHA
- STAT_COUNT

profession_e:
- PROFESSION_NONE = 0
- PROFESSION_WARRIOR
- PROFESSION_MAGE
- PROFESSION_ROGUE
- PROFESSION_COUNT

pet_state_e:
- PET_STATE_IDLE
- PET_STATE_COMBAT
- PET_STATE_RESTING
```

### rules.h - Funciones

```
Dice rolls:
- uint8_t rules_roll_d20(void)
- uint8_t rules_roll_dice(uint8_t sides)
- uint8_t rules_roll_multiple(uint8_t count, uint8_t sides)

Modifiers:
- int8_t rules_get_modifier(uint8_t attribute)

Stat increases:
- bool rules_roll_stat_increase(bool advantage, bool disadvantage)

Skill learning:
- float rules_skill_success_probability(uint8_t level)
- bool rules_roll_skill_success(float probability)
```

### De dónde sacar la lógica

- `rules_get_modifier()` → `pet.c:318-323`
- `rules_roll_stat_increase()` → `pet.c:95-110`
- `rules_skill_success_probability()` → `data_loader.c:372-381`
- `rules_roll_skill_success()` → `data_loader.c:383-389`

### Dependencias

- `config` (solo para constantes de límites)

---

## Fase 4: Storage

**Duración**: 45 minutos

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `storage/storage.h` | Funciones genéricas de archivo |
| `storage/storage.c` | Implementación basada en sd_card.c |
| `storage/CMakeLists.txt` | `idf_component_register(REQUIRES config fatfs sdmmc driver)` |

### storage.h - Funciones

```
Mount/Unmount:
- esp_err_t storage_init(void)
- esp_err_t storage_mount(void)
- void storage_unmount(void)
- bool storage_is_mounted(void)

File operations:
- bool storage_file_exists(const char *path)
- esp_err_t storage_ensure_dir(const char *path)
- esp_err_t storage_save_file(const char *path, const uint8_t *data, size_t len)
- esp_err_t storage_load_file(const char *path, uint8_t **data, size_t *len)

JSON convenience (usa cJSON internamente):
- esp_err_t storage_save_json(const char *path, cJSON *json)
- cJSON* storage_load_json(const char *path)
```

### Qué eliminar de sd_card.c original

- Eliminar `storage_pet_exists()` → Se reemplaza por `storage_file_exists(PET_DATA_PATH)`
- Eliminar `storage_wipe_all()` → Mover a aplicación
- Eliminar `storage_ensure_data_dir()` → Se reemplaza por `storage_ensure_dir()`
- Eliminar constantes de ruta específicas de dominio

### Dependencias

- `config` (GPIOs, mount point)
- ESP-IDF: `fatfs`, `sdmmc`, `driver`

---

## Fase 5: DNA

**Duración**: 30 minutos

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `dna/dna.h` | Struct `dna_t` y funciones |
| `dna/dna.c` | Copiar de dna_parser.c |
| `dna/CMakeLists.txt` | `idf_component_register(REQUIRES config storage)` |

### dna.h - Struct y Funciones

```
dna_t:
- char id[33]
- uint8_t str_base, dex_base, con_base, int_base, wis_base, cha_base
- uint8_t str_cap, dex_cap, con_cap, int_cap, wis_cap, cha_cap
- float hunger_delta, energy_delta

Funciones:
- esp_err_t dna_init(void)
- esp_err_t dna_load_from_file(const char *path, dna_t *dna)
- esp_err_t dna_load_default(dna_t *dna)
- void dna_get_attribute_caps(const dna_t *dna, uint8_t caps[6])
```

### Dependencias

- `config` (rutas)
- `storage` (carga de archivo)

---

## Fase 6: Data (Game Tables)

**Duración**: 1.5 horas

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `data/game_data.h` | Structs y declaraciones de acceso a datos |
| `data/game_data.c` | Carga de JSON y acceso |
| `data/CMakeLists.txt` | `idf_component_register(REQUIRES config storage PRIV_REQUIRES cjson)` |

### game_data.h - Structs

```
skill_data_t:
- uint8_t id
- char name[16]
- uint8_t level
- uint8_t profession
- uint8_t dp_cost
- uint8_t uses_max
- struct effect: damage_base, damage_per_level, crit_base, crit_per_level

damage_bonus_t:
- int8_t min_damage, max_damage, extra_dice, crit, sneak_dice, skill_uses

stat_interval_t:
- uint8_t level
- stat_e stats[3], advantage[3], disadvantage[3]
- uint8_t stats_count, advantage_count, disadvantage_count

damage_progression_t:
- uint8_t level
- damage_bonus_t bonus
```

### game_data.h - Funciones

```
Init:
- esp_err_t game_data_init(void)

Tablas base:
- const uint32_t* game_data_get_exp_table(void)
- const uint16_t* game_data_get_hp_bonus_table(void)
- const char* game_data_get_pet_name(uint8_t index)
- const char* game_data_get_enemy_name(uint8_t index)

Profesiones:
- const char* game_data_get_profession_name(uint8_t id)

Level tables:
- esp_err_t game_data_get_stat_interval(uint8_t level, uint8_t profession, stat_interval_t *out)
- esp_err_t game_data_get_damage_progression(uint8_t level, uint8_t profession, damage_bonus_t *out)

Skills:
- esp_err_t game_data_get_skill(uint8_t skill_id, skill_data_t *out)
- bool game_data_get_skills_for_level(uint8_t level, uint8_t profession, skill_data_t *out, uint8_t *count)
```

### game_tables.json - Agregar

```
Agregar al JSON existente:
{
  "exp_table": [0, 100, 250, 500, 1000, 2000, 4000, 8000, 16000, 32000],
  "hp_bonus_table": [0, 5, 5, 10, 10, 15, 15, 20, 20, 25],
  "pet_names": ["Mistolito", "Nibbler", "Pixel", "Glitch", "Neo", "Byte", "Sparky", "Luma", "Vex", "Zyx"],
  "enemy_names": ["Slime", "Goblin", "Skeleton", "Bat", "Spider", "Wolf", "Orc", "Zombie", "Harpy", "Troll", "Golem", "Demon", "Dragon", "Wraith", "Basilisk"]
}
```

### De dónde sacar la lógica

- Copiar estructura general de `data_loader.c`
- Mover tablas hardcodeadas de `pet.c:10-21` y `combat_engine.c:11-15` al JSON

### Dependencias

- `config` (rutas)
- `storage` (carga de JSON)
- cJSON (via `PRIV_REQUIRES`)

---

## Fase 7: Pet

**Duración**: 1 hora

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `pet/pet.h` | Struct `pet_t` y funciones |
| `pet/pet.c` | Implementación sin llamadas a data_loader |
| `pet/CMakeLists.txt` | `idf_component_register(REQUIRES config core)` |

### pet.h - Struct

```
pet_bonuses_t:
- int8_t min_damage, max_damage, extra_dice, crit, sneak_dice, skill_uses

pet_skill_t:
- uint8_t skill_id
- uint8_t uses_remaining
- uint8_t uses_max

pet_t:
- char name[16]
- uint8_t level
- uint32_t exp, exp_to_next
- int16_t hp, hp_max
- uint8_t str, dex, con, intel, wis, cha
- uint8_t profession_id
- uint32_t dp, enemies_defeated
- uint8_t lives, energy, energy_max
- pet_state_e state
- bool is_alive
- pet_bonuses_t bonuses
- pet_skill_t skills[32]
- uint8_t skill_count
```

### pet.h - Funciones

```
Lifecycle:
- void pet_init(pet_t *pet)
- void pet_generate(pet_t *pet, const char *name, const uint8_t stats[6])

State changes:
- void pet_take_damage(pet_t *pet, int16_t damage)
- void pet_heal(pet_t *pet, int16_t amount)
- void pet_consume_energy(pet_t *pet)
- void pet_rest(pet_t *pet)

Queries:
- int8_t pet_get_modifier(const pet_t *pet, stat_e stat)
- bool pet_has_energy(const pet_t *pet)
- bool pet_has_skill(const pet_t *pet, uint8_t skill_id)

Skills:
- esp_err_t pet_add_skill(pet_t *pet, uint8_t skill_id, uint8_t uses_max)
- void pet_use_skill(pet_t *pet, uint8_t skill_index)
- void pet_restore_combat_skills(pet_t *pet)
- void pet_restore_rest_skills(pet_t *pet, const skill_data_t *skills, uint8_t count)

Level up (recibe valores pre-calculados):
- void pet_level_up(pet_t *pet, uint16_t hp_bonus, const uint8_t stat_increases[6])
- bool pet_add_exp(pet_t *pet, uint32_t amount, uint32_t exp_table[])

Death:
- void pet_death(pet_t *pet)
```

### Cambios vs código original

| Función Original | Cambio |
|------------------|--------|
| `pet_level_up(pet, dna)` | Ahora `pet_level_up(pet, hp_bonus, stat_increases)` |
| `pet_add_exp(pet, amount, dna)` | Ahora `pet_add_exp(pet, amount, exp_table)` |
| `pet_generate(pet, dna)` | Ahora `pet_generate(pet, name, stats)` |
| Llamadas internas a `data_loader` | Eliminadas, valores pasados por parámetro |

### De dónde sacar la lógica

- Copiar de `pet/pet.c` original
- Eliminar includes y llamadas a `data_loader.h`
- Modificar funciones que recibían `dna_t*` para recibir valores directos

### Dependencias

- `config` (constantes)
- `core` (types, rules)

---

## Fase 8: Enemy

**Duración**: 30 minutos

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `enemy/enemy.h` | Struct `enemy_t` y funciones |
| `enemy/enemy.c` | Implementación completa |
| `enemy/CMakeLists.txt` | `idf_component_register(REQUIRES config core)` |

### enemy.h - Struct

```
enemy_t:
- char name[16]
- int16_t hp, hp_max
- uint8_t ac, attack_bonus, damage_dice, damage_bonus, level
- uint16_t exp_reward
```

### enemy.h - Funciones

```
Generation:
- void enemy_generate(enemy_t *enemy, uint8_t pet_level, const char *names[], uint8_t names_count)

Queries:
- const char* enemy_get_name_by_type(uint8_t type, const char *names[], uint8_t names_count)
- uint16_t enemy_get_exp_reward(const enemy_t *enemy)
```

### De dónde sacar la lógica

- Copiar implementación completa de `combat_engine.c:238-260`
- Eliminar función vacía de `enemy.c` original

### Dependencias

- `config` (constantes)
- `core` (types, rules para random)

---

## Fase 9: Combat

**Duración**: 1 hora

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `combat/combat.h` | Enums de estado, funciones |
| `combat/combat.c` | Implementación sin data_loader |
| `combat/CMakeLists.txt` | `idf_component_register(REQUIRES pet enemy core)` |

### combat.h - Enums

```
combat_state_e:
- COMBAT_STATE_IDLE
- COMBAT_STATE_IN_PROGRESS
- COMBAT_STATE_VICTORY
- COMBAT_STATE_DEFEAT

combat_event_e:
- COMBAT_EVENT_NONE
- COMBAT_EVENT_PET_ATTACK
- COMBAT_EVENT_PET_HIT
- COMBAT_EVENT_ENEMY_ATTACK
- COMBAT_EVENT_ENEMY_HIT
- COMBAT_EVENT_ENEMY_DIED
- COMBAT_EVENT_PET_DIED

combat_result_t:
- combat_event_e type
- int16_t damage
- int16_t remaining_hp
```

### combat.h - Funciones

```
Lifecycle:
- void combat_init(void)
- void combat_start(pet_t *pet, enemy_t *enemy)
- void combat_end(void)

Turn execution:
- combat_result_t combat_execute_turn(pet_t *pet, enemy_t *enemy)

State queries:
- bool combat_is_active(void)
- combat_state_e combat_get_state(void)

Dice ( wrappers a rules):
- uint8_t combat_roll_d20(void)
- bool combat_check_hit(int16_t roll, uint8_t ac)
```

### Cambios vs código original

| Aspecto | Cambio |
|---------|--------|
| `enemy_names[]` hardcodeado | Eliminado, se pasa por parámetro o usa enemy_get_name |
| Llamadas a `skill_get_data()` | Eliminadas, pet ya tiene toda la info necesaria |
| `find_best_damage_skill()` | Se queda, pero no consulta data_loader |

### De dónde sacar la lógica

- Copiar de `combat/combat_engine.c` original
- Eliminar includes a `data_loader.h`
- Eliminar array `enemy_names[]`

### Dependencias

- `pet`
- `enemy`
- `core`

---

## Fase 10: UI

**Duración**: 1 hora

### Archivos a Crear

| Archivo | Contenido |
|---------|-----------|
| `ui/ui.h` | Funciones de renderizado |
| `ui/ui.c` | Copiar de hud/ui.c |
| `ui/sprites.h` | Declaraciones de sprites |
| `ui/sprites.c` | Copiar de hud/sprites.c |
| `ui/CMakeLists.txt` | `idf_component_register(REQUIRES lvgl pet combat)` |

### ui.h - Funciones

```
Init:
- esp_err_t ui_init(void)

Render:
- void ui_render_pet_stats(const pet_t *pet)
- void ui_render_enemy_stats(const enemy_t *enemy)
- void ui_render_combat_log(const char *message)
- void ui_clear(void)

Sprites:
- void ui_draw_arena(void)
- void ui_draw_pet_sprite(void)
- void ui_draw_enemy_sprite(void)
```

### Dependencias

- `pet`
- `combat`
- LVGL (externo)

---

## Fase 11: Main (Aplicación)

**Duración**: 1 hora

### Archivos a Modificar

| Archivo | Cambio |
|---------|--------|
| `main/main.c` | Reescribir con nueva arquitectura |
| `main/CMakeLists.txt` | Actualizar dependencias |

### main.c - Flujo

```
app_main():
1. Inicializar componentes base
   - config_init()
   - storage_init()
   - game_data_init()

2. Cargar o crear pet
   - Si existe PET_DATA_PATH: cargar
   - Si no: cargar DNA default, generar pet

3. Inicializar UI
   - ui_init()

4. Game loop
   - Generar enemigo
   - Combatir
   - Si victoria: procesar level up, guardar
   - Si derrota: procesar muerte, reiniciar si aplica

5. Manejar level up (AQUÍ VA LA LÓGICA)
   - game_data_get_stat_interval()
   - rules_roll_stat_increase()
   - pet_level_up(pet, hp_bonus, stat_increases)
```

### main/CMakeLists.txt

```
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES config core storage dna data pet enemy combat ui
)
```

---

## Fase 12: Testing y Limpieza

**Duración**: 1 hora

### Tareas

| # | Tarea |
|---|-------|
| 12.1 | Compilar: `idf.py build` |
| 12.2 | Corregir errores de compilación |
| 12.3 | Verificar warnings |
| 12.4 | Flashear a hardware |
| 12.5 | Probar combate básico |
| 12.6 | Probar level up |
| 12.7 | Probar persistencia |
| 12.8 | Si funciona: `rm -rf components` |
| 12.9 | Renombrar: `mv components_v2 components` |

---

## Resumen de Dependencias por Componente

| Componente | REQUIRES | PRIV_REQUIRES |
|------------|----------|---------------|
| config | - | - |
| core | config | - |
| storage | config | fatfs, sdmmc, driver |
| dna | config, storage | - |
| data | config, storage | cjson |
| pet | config, core | - |
| enemy | config, core | - |
| combat | pet, enemy, core | - |
| ui | pet, combat | lvgl |
| main | config, core, storage, dna, data, pet, enemy, combat, ui | - |

---

## Resumen de Tiempos

| Fase | Duración |
|------|----------|
| 1. Setup | 15 min |
| 2. Config | 30 min |
| 3. Core | 45 min |
| 4. Storage | 45 min |
| 5. DNA | 30 min |
| 6. Data | 1.5 h |
| 7. Pet | 1 h |
| 8. Enemy | 30 min |
| 9. Combat | 1 h |
| 10. UI | 1 h |
| 11. Main | 1 h |
| 12. Testing | 1 h |
| **TOTAL** | **~10 h** |

---

## Checklist de Verificación por Fase

### Fase 1
- [ ] Directorio `components_v2` creado
- [ ] Subdirectorios creados
- [ ] Backup de CMakeLists.txt

### Fase 2
- [ ] config.h creado con constantes
- [ ] config.c creado (vacío)
- [ ] CMakeLists.txt creado
- [ ] Compila sin errores

### Fase 3
- [ ] types.h con enums
- [ ] rules.h con declaraciones
- [ ] rules.c con implementación
- [ ] CMakeLists.txt con dependencia config
- [ ] Compila sin errores

### Fase 4
- [ ] storage.h con funciones genéricas
- [ ] storage.c implementado
- [ ] Sin referencias a pet/dominio
- [ ] CMakeLists.txt con ESP-IDF deps
- [ ] Compila sin errores

### Fase 5
- [ ] dna.h con struct
- [ ] dna.c copiado
- [ ] CMakeLists.txt con storage dep
- [ ] Compila sin errores

### Fase 6
- [ ] game_data.h con structs
- [ ] game_data.c implementado
- [ ] JSON actualizado con tablas base
- [ ] CMakeLists.txt con cJSON
- [ ] Compila sin errores

### Fase 7
- [ ] pet.h con struct y funciones
- [ ] pet.c sin includes a data_loader
- [ ] CMakeLists.txt con core dep
- [ ] Compila sin errores

### Fase 8
- [ ] enemy.h con enemy_t
- [ ] enemy.c con generate completo
- [ ] CMakeLists.txt con core dep
- [ ] Compila sin errores

### Fase 9
- [ ] combat.h con enums y funciones
- [ ] combat.c sin data_loader
- [ ] CMakeLists.txt con pet, enemy deps
- [ ] Compila sin errores

### Fase 10
- [ ] ui.h/c copiados
- [ ] sprites.h/c copiados
- [ ] CMakeLists.txt con lvgl
- [ ] Compila sin errores

### Fase 11
- [ ] main.c reescrito
- [ ] CMakeLists.txt actualizado
- [ ] Compila sin errores

### Fase 12
- [ ] Build exitoso sin warnings
- [ ] Flash exitoso
- [ ] Combate funciona
- [ ] Level up funciona
- [ ] Persistencia funciona
- [ ] Código antiguo eliminado

---

## Notas Adicionales

### Sobre la migración de lógica

1. **NO copiar includes**: Cada archivo nuevo solo incluye lo que necesita según la arquitectura
2. **NO copiar código muerto**: Si algo no se usa, no se copia
3. **Cambiar firmas**: Las funciones que recibían `dna_t*` ahora reciben valores directos
4. **Mover constantes**: Todo lo hardcodeado va a config.h o game_tables.json

### Sobre el game loop en main

La responsabilidad de main es:
- Inicializar todo
- Coordinar el flujo
- **Calcular valores** usando rules + game_data
- **Pasar valores** a pet/combat

Ejemplo de level up:
```c
// En main.c, NO en pet.c
stat_interval_t interval;
game_data_get_stat_interval(pet.level, pet.profession_id, &interval);

uint8_t stat_increases[6] = {0};
for (int i = 0; i < interval.stats_count; i++) {
    bool adv = contains(interval.advantage, interval.stats[i]);
    bool dis = contains(interval.disadvantage, interval.stats[i]);
    if (rules_roll_stat_increase(adv, dis)) {
        stat_increases[interval.stats[i]] = 1;
    }
}

uint16_t hp_bonus = game_data_get_hp_bonus_table()[pet.level];
pet_level_up(&pet, hp_bonus, stat_increases);
```

### Sobre testing

Compilar frecuentemente. No esperar a terminar todo para compilar.
