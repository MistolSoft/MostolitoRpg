# Arquitectura FreeRTOS: Coordinator + Workers

Este documento describe la arquitectura de tareas FreeRTOS para MistolitoRPG v2.0, utilizando un coordinator como orquestador central y workers especializados.

---

## 1. Visión General

### 1.1 Diagrama de Arquitectura

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ CORE 0                                                                        │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │ game_coordinator_task (Priority 4)                                      │ │
│  │                                                                         │ │
│  │  ┌─────────────────────────────────────────────────────────────────┐   │ │
│  │  │                    STATE MACHINE                                │   │ │
│  │  │                                                                 │   │ │
│  │  │   GS_INIT ──► GS_SEARCHING ──► GS_COMBAT ──► GS_VICTORY        │   │ │
│  │  │        ▲                           │              │             │   │ │
│  │  │        │                           ▼              ▼             │   │ │
│  │  │        └──────────────── GS_RESTING ◄──────── GS_LEVELUP       │   │ │
│  │  │                       (energy=0)        (exp>=exp_next)        │   │ │
│  │  └─────────────────────────────────────────────────────────────────┘   │ │
│  │                                                                         │ │
│  │  - POSEE el snapshot (única fuente de verdad)                          │ │
│  │  - Activa workers vía Task Notifications                               │ │
│  │  - Recibe resultados vía Task Notifications                            │ │
│  │  - Emite eventos a Display vía Queue                                   │ │
│  │  - Envía deltas a Storage vía Queue                                    │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                                      │                                        │
│              ┌───────────────────────┼───────────────────────┐               │
│              │                       │                       │               │
│              ▼                       ▼                       ▼               │
│  ┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐        │
│  │ combat_worker     │  │ search_worker     │  │ rest_worker       │        │
│  │ (Priority 3)      │  │ (Priority 2)      │  │ (Priority 1)      │        │
│  │                   │  │                   │  │                   │        │
│  │ - SUSPENDED       │  │ - SUSPENDED       │  │ - SUSPENDED       │        │
│  │ - Ejecuta 1 turno │  │ - Delay 3-8 seg   │  │ - Delay 1 seg     │        │
│  │ - Notifica daño   │  │ - Genera enemigo  │  │ - +1 energía      │        │
│  └───────────────────┘  └───────────────────┘  └───────────────────┘        │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │ storage_task (Priority 1)                                               │ │
│  │                                                                         │ │
│  │  - Recibe requests vía Queue                                            │ │
│  │  - Guarda/carga JSON en SD                                              │ │
│  │  - 100% asíncrono, nunca bloquea                                        │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │ Event Queue
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ CORE 1                                                                        │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │ display_task (Priority 5)                                               │ │
│  │                                                                         │ │
│  │  - Recibe eventos (non-blocking, 10ms timeout)                          │ │
│  │  - Renderiza UI con LVGL (2-4 FPS)                                      │ │
│  │  - SOLO LEE snapshot, nunca escribe                                     │ │
│  │  - Mantiene sprites y animaciones                                       │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Resumen de Tareas

| Tarea | Prioridad | Core | Stack | Función |
|-------|-----------|------|-------|---------|
| `game_coordinator_task` | 4 | 0 | 8 KB | Máquina de estados, posee snapshot |
| `combat_worker_task` | 3 | 0 | 4 KB | Ejecuta turnos de combate |
| `search_worker_task` | 2 | 0 | 4 KB | Busca y genera enemigos |
| `rest_worker_task` | 1 | 0 | 4 KB | Recupera energía |
| `storage_task` | 1 | 0 | 4 KB | Persistencia en SD |
| `display_task` | 5 | 1 | 8 KB | Renderizado LVGL |

---

## 2. Snapshot (Fuente de Verdad)

El coordinator es el **ÚNICO** que posee y modifica el snapshot.

### 2.1 Estructura

```c
typedef struct {
    game_state_e state;
    uint32_t state_entered_ms;
    
    pet_t pet;
    enemy_t enemy;
    combat_state_t combat;
} game_snapshot_t;
```

### 2.2 pet_t (del legacy)

```c
typedef struct {
    char name[16];
    uint8_t level;
    uint32_t exp;
    uint32_t exp_to_next;
    
    int16_t hp;
    int16_t hp_max;
    
    uint8_t str;       // Fuerza
    uint8_t dex;       // Destreza
    uint8_t con;       // Constitución
    uint8_t intel;     // Inteligencia
    uint8_t wis;       // Sabiduría
    uint8_t cha;       // Carisma
    
    uint8_t profession_id;
    uint32_t dp;       // Deity Points
    uint32_t enemies_killed;
    uint8_t lives;
    
    uint8_t energy;
    uint8_t energy_max;
    
    pet_bonuses_t bonuses;    // Bonus de daño por profesión
    pet_skill_t skills[32];   // Habilidades aprendidas
    uint8_t skill_count;
    
    uint8_t dirty_flags;      // Para storage delta
    bool is_alive;
} pet_t;
```

### 2.3 enemy_t (del legacy)

```c
typedef struct {
    char name[16];
    int16_t hp;
    int16_t hp_max;
    uint8_t ac;              // Armor Class
    uint8_t attack_bonus;
    uint8_t damage_dice;     // d6, d8, etc.
    uint8_t damage_bonus;
    uint8_t level;
    uint16_t exp_reward;
    bool alive;
} enemy_t;
```

### 2.4 pet_bonuses_t (del legacy)

```c
typedef struct {
    int8_t min_damage;
    int8_t max_damage;
    int8_t extra_dice;
    int8_t crit;
    int8_t sneak_dice;
    int8_t skill_uses;
} pet_bonuses_t;
```

---

## 3. Estados del Coordinator

### 3.1 Diagrama de Estados

```
                    ┌─────────────────────────────────────────────────┐
                    │                                                 │
                    ▼                                                 │
               ┌─────────┐                                           │
               │ GS_INIT │ ◄─────────────────────────────────────┐   │
               └────┬────┘                                       │   │
                    │ (notificación de display_task)             │   │
                    ▼                                             │   │
            ┌──────────────┐                                     │   │
            │ GS_SEARCHING │ ◄──────────────────────────────┐    │   │
            └──────┬───────┘                                │    │   │
                   │ (enemy spawned)                        │    │   │
                   ▼                                        │    │   │
             ┌───────────┐                                  │    │   │
             │ GS_COMBAT │ ─────────────────────────────┐   │    │   │
             └─────┬─────┘                               │   │    │   │
                   │ (enemy dead)                        │   │    │   │
                   ▼                                     │   │    │   │
             ┌───────────┐                               │   │    │   │
             │ GS_VICTORY│                               │   │    │   │
             └─────┬─────┘                               │   │    │   │
                   │ (exp >= exp_next?)                  │   │    │   │
                   ├──── NO ────┐                        │   │    │   │
                   │            ▼                        │   │    │   │
                   │    ┌───────────────┐                │   │    │   │
                   │    │ energy > 0?   │                │   │    │   │
                   │    └───────┬───────┘                │   │    │   │
                   │            │                        │   │    │   │
                   │      ┌──── NO ────┐                 │   │    │   │
                   │      │           ▼                 │   │    │   │
                   │      │    ┌────────────┐           │   │    │   │
                   │      │    │ GS_RESTING │ ──────────┼───┼────┘   │
                   │      │    └────────────┘           │   │        │
                   │      │           │ (energy=max)   │   │        │
                   │      │           └────────────────┼───┼────────┘
                   │      │                            │   │
                   │      └────────────────────────────┼───┼───► GS_SEARCHING
                   │                                   │   │
                   └──── YES ──► GS_LEVELUP ───────────┘   │
                                      │                    │
                                      └────────────────────┘
```

### 3.2 Descripción de Estados

| Estado | Descripción | Transición |
|--------|-------------|------------|
| `GS_INIT` | Espera señal de display_task (USB init o BOOT button) | → GS_SEARCHING o GS_RESTING |
| `GS_SEARCHING` | Activa search_worker para generar enemigo | → GS_COMBAT |
| `GS_COMBAT` | Activa combat_worker por cada turno | → GS_VICTORY (enemy dead) |
| `GS_VICTORY` | Procesa recompensas, guarda estado | → GS_LEVELUP o GS_SEARCHING/GS_RESTING |
| `GS_LEVELUP` | Sube de nivel, actualiza stats | → GS_SEARCHING/GS_RESTING |
| `GS_RESTING` | Activa rest_worker hasta energy=max | → GS_SEARCHING |

---

## 4. Reglas del Juego (del legacy rules.c)

### 4.1 Dados

```c
uint8_t rules_roll_d20(void) {
    return (uint8_t)((esp_random() % 20) + 1);
}

uint8_t rules_roll_dice(uint8_t sides) {
    if (sides == 0) return 0;
    return (uint8_t)((esp_random() % sides) + 1);
}

uint8_t rules_roll_multiple(uint8_t count, uint8_t sides) {
    uint8_t total = 0;
    for (uint8_t i = 0; i < count; i++) {
        total += rules_roll_dice(sides);
    }
    return total;
}
```

### 4.2 Modificadores de Atributo

```c
int8_t rules_get_modifier(uint8_t attribute) {
    if (attribute <= 1) return -5;
    if (attribute >= 20) return 5;
    return (int8_t)((attribute / 2) - 5);
}
```

Tabla de ejemplo:
| Atributo | Modificador |
|----------|-------------|
| 1 | -5 |
| 2-3 | -4 |
| 4-5 | -3 |
| 6-7 | -2 |
| 8-9 | -1 |
| 10-11 | 0 |
| 12-13 | +1 |
| 14-15 | +2 |
| 16-17 | +3 |
| 18-19 | +4 |
| 20+ | +5 |

---

## 5. Sistema de Combate (del legacy combat.c)

### 5.1 Flujo de Turno

```c
combat_result_t combat_execute_turn(pet_t *pet, enemy_t *enemy) {
    combat_result_t result = {COMBAT_EVENT_NONE, 0, 0};
    
    // 1. Calcular attack roll
    int8_t dex_mod = pet_get_modifier(pet, STAT_DEX);
    int16_t attack_roll = rules_roll_d20() + dex_mod;
    
    // 2. Verificar hit
    if (attack_roll >= enemy->ac) {
        // 3. Calcular daño base según profesión
        uint8_t base_damage = 0;
        uint8_t dice_count = 3 + pet->bonuses.extra_dice;
        
        switch (pet->profession_id) {
            case PROFESSION_WARRIOR:
                base_damage = rules_roll_multiple(dice_count, 6);
                break;
            case PROFESSION_MAGE:
                base_damage = rules_roll_multiple(1 + pet->bonuses.extra_dice, 20);
                break;
            case PROFESSION_ROGUE:
                // Rogue tiene chance de crítico
                for (uint8_t i = 0; i < dice_count; i++) {
                    uint8_t d = rules_roll_dice(4);
                    if (rules_random_chance(15 + pet->bonuses.crit)) {
                        d *= 2;  // Crítico!
                    }
                    base_damage += d;
                }
                break;
            default:
                base_damage = rules_roll_dice(15) + 1;
                break;
        }
        
        // 4. Aplicar modificadores
        int8_t str_mod = pet_get_modifier(pet, STAT_STR);
        int16_t damage = base_damage + str_mod + pet->bonuses.min_damage;
        if (damage < 1) damage = 1;
        
        // 5. Aplicar daño
        enemy->hp -= damage;
        if (enemy->hp < 0) enemy->hp = 0;
        
        result.type = COMBAT_EVENT_PET_HIT;
        result.damage = damage;
        result.remaining_hp = enemy->hp;
        
        // 6. Verificar muerte del enemigo
        if (enemy->hp <= 0) {
            result.type = COMBAT_EVENT_ENEMY_DIED;
            
            // Posibilidad de ganar DP
            if (rules_random_chance(DP_GAIN_CHANCE)) {
                pet->dp++;
            }
        }
    } else {
        result.type = COMBAT_EVENT_PET_ATTACK;  // Miss
    }
    
    return result;
}
```

### 5.2 Constantes de Combate

```c
#define BASE_ENEMY_HP 50
#define ENEMY_HP_PER_LEVEL 20
#define BASE_ENEMY_AC 8
#define BASE_PET_HP 20
#define DP_GAIN_CHANCE 30
#define PROFESSION_UNLOCK_DP 10
```

---

## 6. Sistema de Enemigos (del legacy enemy.c)

### 6.1 Generación de Enemigo

```c
void enemy_generate(enemy_t *enemy, uint8_t pet_level, const char *names[], uint8_t names_count) {
    // Seleccionar tipo aleatorio
    uint8_t enemy_type = rules_random_range(names_count);
    strncpy(enemy->name, names[enemy_type], 15);
    enemy->name[15] = '\0';
    
    // Escalar según nivel del pet
    enemy->hp_max = BASE_ENEMY_HP + (pet_level * ENEMY_HP_PER_LEVEL);
    enemy->hp = enemy->hp_max;
    enemy->ac = BASE_ENEMY_AC + pet_level;
    enemy->attack_bonus = pet_level / 2;
    enemy->damage_dice = 6;
    enemy->damage_bonus = pet_level / 2;
    enemy->level = pet_level;
    
    // Recompensa de EXP
    uint16_t base_exp = enemy->hp_max / 4;
    enemy->exp_reward = base_exp + rules_random_range(20) + 1;
}
```

### 6.2 Nombres de Enemigos

```c
static const char* ENEMY_NAMES[] = {
    "Slime", "Goblin", "Skeleton", "Bat", "Spider",
    "Wolf", "Orc", "Zombie", "Harpy", "Troll",
    "Golem", "Demon", "Dragon", "Wraith", "Basilisk"
};
#define ENEMY_COUNT 15
```

---

## 7. Sistema de Niveles (del legacy pet.c + game_data.c)

### 7.1 Fórmula de EXP

```c
// Config por defecto
float exp_base = 50.0f;
float exp_linear = 50.0f;
float exp_multiplier = 1.15f;

uint32_t game_data_calc_exp_for_level(uint8_t level) {
    if (level == 0 || level >= MAX_LEVEL) return 0;
    
    // Fórmula: exp_base + (exp_linear * level) + exp_base * (exp_multiplier^level - 1)
    float linear_part = exp_base + (exp_linear * (float)level);
    float exp_part = exp_base * (powf(exp_multiplier, (float)level) - 1.0f);
    
    return (uint32_t)(linear_part + exp_part);
}
```

### 7.2 Tabla de EXP

| Nivel | EXP Requerida |
|-------|---------------|
| 1 | 100 |
| 2 | 150 |
| 3 | 220 |
| 4 | 320 |
| 5 | 460 |
| 6 | 670 |
| 7 | 980 |
| 8 | 1450 |
| 9 | 2200 |
| 10 | 3400 |

### 7.3 Bonus de HP por Nivel

```c
uint16_t game_data_calc_hp_bonus(uint8_t level) {
    if (level == 0 || level >= MAX_LEVEL) return 0;
    return 5 + 5 * ((level - 1) / 2);
}
```

| Nivel | HP Bonus |
|-------|----------|
| 1 | 5 |
| 2 | 5 |
| 3 | 10 |
| 4 | 10 |
| 5 | 15 |
| 6 | 15 |
| 7 | 20 |
| 8 | 20 |
| 9 | 25 |
| 10 | 25 |

---

## 8. Sistema de Profesiones

### 8.1 Definición

```c
typedef enum {
    PROFESSION_NONE = 0,
    PROFESSION_WARRIOR,
    PROFESSION_MAGE,
    PROFESSION_ROGUE,
    PROFESSION_COUNT
} profession_e;
```

### 8.2 Características por Profesión

| Profesión | Daño Base | Crítico | Especial |
|-----------|-----------|---------|----------|
| Novice | 1d15+1 | - | Básico |
| Warrior | 3d6 | - | +min_damage por nivel |
| Mage | 1d20 | - | Daño mágico alto |
| Rogue | 3d4 | 15% x2 | Sneak dice + crit bonus |

### 8.3 Desbloqueo de Profesión

```c
// Al ganar DP, si tiene suficientes y no tiene profesión:
if (pet->dp >= PROFESSION_UNLOCK_DP && pet->profession_id == PROFESSION_NONE) {
    pet->dp = 0;
    pet->profession_id = rules_random_range(3) + 1;  // Warrior, Mage o Rogue
}
```

---

## 9. Comunicación Entre Tareas

### 9.1 Task Notifications (Coordinator ↔ Workers)

**Coordinator → Worker (activar):**
```c
vTaskResume(worker_handle);
xTaskNotify(worker_handle, 0, eNoAction);
```

**Worker → Coordinator (resultado):**
```c
xTaskNotify(coordinator_handle, result_value, eSetValueWithOverwrite);
```

**Coordinator espera resultado:**
```c
uint32_t result = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
```

### 9.2 Event Queue (Coordinator → Display)

```c
#define EVENT_QUEUE_SIZE 16

typedef enum {
    EVT_STATE_CHANGED,
    EVT_PET_UPDATED,
    EVT_ENEMY_SPAWNED,
    EVT_ENEMY_DIED,
    EVT_DAMAGE_DEALT,
    EVT_VICTORY,
    EVT_LEVEL_UP,
    EVT_ENERGY_CHANGED,
    EVT_GAME_START
} event_type_e;

typedef struct {
    event_type_e type;
    union {
        game_state_e new_state;
        struct { int16_t amount; } damage;
        struct { uint32_t amount; } exp;
        struct { uint8_t level; } level_up;
        struct { uint8_t current; uint8_t max; } energy;
    } data;
} game_event_t;
```

### 9.3 Storage Queue (Coordinator → Storage)

```c
#define STORAGE_QUEUE_SIZE 4

typedef enum {
    STORAGE_OP_SAVE_PET_DELTA,
    STORAGE_OP_LOAD_PET,
    STORAGE_OP_LOAD_TABLES
} storage_op_e;

typedef struct {
    storage_op_e operation;
    uint8_t dirty_flags;
    pet_t *pet;  // Puntero al pet (solo lectura)
} storage_request_t;
```

---

## 10. Dirty Flags (para Storage Optimizado)

```c
#define PET_DIRTY_NAME     (1 << 0)
#define PET_DIRTY_LEVEL    (1 << 1)
#define PET_DIRTY_EXP      (1 << 2)
#define PET_DIRTY_HP       (1 << 3)
#define PET_DIRTY_ENERGY   (1 << 4)
#define PET_DIRTY_STATS    (1 << 5)
#define PET_DIRTY_PROFESSION (1 << 6)
#define PET_DIRTY_DP       (1 << 7)
```

**Uso:**
```c
// Al modificar algo:
snapshot.pet.exp += exp_gained;
snapshot.pet.dirty_flags |= PET_DIRTY_EXP;

// Al guardar:
storage_save_pet_delta(snapshot.pet.dirty_flags, &snapshot.pet);
snapshot.pet.dirty_flags = 0;  // Clear después de enviar
```

---

## 11. Flujo de Inicio (USB Init)

### 11.1 Secuencia de Boot

```
app_main()
    │
    ├── lcd_hardware_init()     // SPI bus + LCD ST7789
    │
    ├── lvgl_display_init()     // LVGL buffers en PSRAM
    │
    ├── storage_task_start()    // Mount SD (no reinicializa SPI)
    │
    ├── game_coordinator_start() // State = GS_INIT
    │
    └── Crear tasks...
        │
        ├── game_coordinator_task (Core 0, Priority 4)
        ├── combat_worker_task (Core 0, Priority 3, SUSPENDED)
        ├── search_worker_task (Core 0, Priority 2, SUSPENDED)
        ├── rest_worker_task (Core 0, Priority 1, SUSPENDED)
        ├── storage_task (Core 0, Priority 1)
        └── display_task (Core 1, Priority 5)
```

### 11.2 display_task (Modo USB Init)

```c
void display_task(void *arg) {
    // 1. Crear screen INIT
    screens_init();
    screens_load(SCREEN_INIT);
    
    // 2. Esperar USB commands o BOOT button
    while (!game_started) {
        handle_usb_commands();
        
        if (usb_start_requested() || check_boot_button()) {
            if (usb_check_required_files()) {
                game_started = true;
            }
        }
        
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // 3. Cambiar a screen GAME
    screens_load(SCREEN_GAME);
    
    // 4. Notificar coordinator para salir de GS_INIT
    xTaskNotify(g_coordinator_task_handle, 1, eSetValueWithOverwrite);
    
    // 5. Main loop
    while (1) {
        while (events_receive(&evt, 10)) {
            // Procesar eventos
        }
        
        game_snapshot_t *snap = game_coordinator_get_snapshot();
        screens_update(snap);
        
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(250));  // 4 FPS
    }
}
```

### 11.3 game_coordinator_task (GS_INIT)

```c
case GS_INIT:
    // Esperar notificación de display_task
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100))) {
        // Cargar pet desde SD
        if (!storage_load_pet(&g_snapshot.pet)) {
            // Generar pet nuevo con stats por defecto
            uint8_t default_stats[6] = {10, 10, 10, 10, 10, 10};
            pet_generate(&g_snapshot.pet, "Mistolito", default_stats);
        }
        
        // Cargar tablas de juego
        storage_load_tables(g_game_tables_exp, g_game_tables_hp_bonus);
        
        // Transición inicial
        if (g_snapshot.pet.energy > 0) {
            transition_to(GS_SEARCHING);
        } else {
            transition_to(GS_RESTING);
        }
    }
    break;
```

---

## 12. Archivos del Componente

### 12.1 Estructura

```
components/mistolito_core/
├── mistolito.h           # Tipos base (pet_t, enemy_t, game_state_e)
├── events.h/c            # Event queue (coordinator → display)
├── game_coordinator.h/c  # State machine + snapshot owner
├── workers.h/c           # combat/search/rest workers
├── storage_task.h/c      # SD card operations (async)
├── display_task.h/c      # LVGL loop + USB init handling
├── screens.h/c           # Screen manager (INIT, GAME)
├── usb_init.h/c          # USB serial commands
├── lcd_init.h/c          # LCD ST7789 + LVGL init
├── CMakeLists.txt
└── idf_component.yml     # Dependencies: lvgl, cjson
```

### 12.2 Dependencias

```
main
└── mistolito_core
    ├── lvgl (managed component)
    ├── cjson (managed component)
    └── ESP-IDF: driver, fatfs, sdmmc, usb_serial_jtag
```

---

## 13. JSON de game_tables.json

```json
{
  "config": {
    "exp_base": 50,
    "exp_linear": 50,
    "exp_multiplier": 1.15,
    "hp_bonus_base": 5,
    "hp_bonus_step": 5
  },
  "level_tables": {
    "novice": {
      "stat_intervals": [
        {"level": 1, "stats": ["str"], "advantage": [], "disadvantage": []}
      ],
      "damage_progression": [
        {"level": 1, "min_damage": 0, "max_damage": 0}
      ]
    },
    "warrior": {
      "stat_intervals": [
        {"level": 1, "stats": ["str", "con"], "advantage": ["str"], "disadvantage": []}
      ],
      "damage_progression": [
        {"level": 1, "min_damage": 1, "extra_dice": 0}
      ]
    }
  },
  "skills": [
    {
      "id": 1,
      "name": "Power Strike",
      "level": 3,
      "profession": 1,
      "dp_cost": 5,
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

---

## 14. Constantes del Sistema

```c
#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define MAX_LEVEL 10
#define MAX_ENERGY 10
#define MAX_NAME_LEN 16

// Combate
#define BASE_ENEMY_HP 50
#define ENEMY_HP_PER_LEVEL 20
#define BASE_ENEMY_AC 8
#define BASE_PET_HP 20
#define DP_GAIN_CHANCE 30
#define PROFESSION_UNLOCK_DP 10

// Professions
#define PROF_NONE 0
#define PROF_WARRIOR 1
#define PROF_MAGE 2
#define PROF_ROGUE 3

// Stats
#define STAT_STR 0
#define STAT_DEX 1
#define STAT_CON 2
#define STAT_INT 3
#define STAT_WIS 4
#define STAT_CHA 5
#define STAT_COUNT 6
```

---

## 15. Próximos Pasos

1. **Implementar workers con lógica del legacy**:
   - combat_worker usa `combat_execute_turn()` de combat.c
   - search_worker usa `enemy_generate()` de enemy.c
   - rest_worker solo incrementa energía

2. **Implementar level up**:
   - Usar `pet_level_up()` de pet.c
   - Cargar stat_intervals y damage_progression de game_data.c

3. **Implementar UI**:
   - Usar sprites de sprites.c
   - Usar layout de ui.c

4. **Probar flujo completo**:
   - USB init → GS_INIT → GS_SEARCHING → GS_COMBAT → GS_VICTORY → GS_LEVELUP
