# Arquitectura MVP: Combate Idle

Este documento describe la arquitectura mínima necesaria para el MVP de MistolitoRPG: combate idle sin interacción de usuario.

---

## 1. Alcance del MVP

### 1.1 Qué incluye
- UI básica con **LVGL 9** (Pet, HP, EXP, nombre)
- Sistema de ADN (carga desde SD)
- Combate idle 1v1 (sin input del usuario)
- Sistema de niveles y progresión
- Profesiones y habilidades por nivel
- Muerte y renacimiento
- DP que persisten entre vidas

### 1.2 Qué NO incluye
- IMU, touch, cámara (sin interacción)
- Mundo procedural
- Memoria fractal
- Bridge/LLM
- Múltiples enemigos
- Biomas

---

## 2. LVGL 9 para Sprites

### 2.1 Widget lv_animimg

LVGL 9 incluye el widget `lv_animimg` (Animation Image) ideal para sprites del Pet:

```c
LV_IMG_DECLARE(sprite_idle_0);
LV_IMG_DECLARE(sprite_idle_1);
LV_IMG_DECLARE(sprite_idle_2);

static lv_image_dsc_t *idle_frames[] = {
    &sprite_idle_0,
    &sprite_idle_1,
    &sprite_idle_2,
};

lv_obj_t *pet_sprite = lv_animimg_create(lv_scr_act());
lv_animimg_set_src(pet_sprite, idle_frames, 3);
lv_animimg_set_duration(pet_sprite, 500);
lv_animimg_set_repeat_count(pet_sprite, LV_ANIM_REPEAT_INFINITE);
lv_animimg_start(pet_sprite);
```

### 2.2 Conversión de Sprites

Usar el conversor online de LVGL: https://lvgl.io/tools/imageconverter

Configuración recomendada:
- **Color format:** RGB565 (16-bit, buen balance tamaño/calidad)
- **Output format:** C array (compilado en firmware)
- Para sprites con transparencia: ARGB8565

### 2.3 Sprites MVP Necesarios

| Sprite | Frames | Uso |
|--------|--------|-----|
| `pet_idle` | 3-4 frames | Animación cuando el Pet está esperando |
| `pet_attack` | 2-3 frames | Animación de ataque |
| `pet_hit` | 1-2 frames | Cuando recibe daño |
| `pet_death` | 2-3 frames | Animación de muerte |
| `pet_levelup` | 2-3 frames | Celebración de level up |

---

## 3. Mapa de Memoria MVP

### 3.1 Uso Simplificado

```
SRAM Interna (512 KB)
├── FreeRTOS Tasks Stacks (~30 KB total)
├── Variables globales del Pet
├── Cola de eventos combate
└── Buffers mínimos

PSRAM (8 MB)
├── LVGL 9 Framebuffers (~150 KB)
├── Sprites del Pet (C arrays compilados)
└── JSON buffer para ADN
```

### 3.2 Asignaciones MVP

| Buffer | Tamaño | Ubicación |
|--------|--------|-----------|
| LVGL buf1 | 240×10×2 = 4.8 KB | PSRAM |
| LVGL buf2 | 240×10×2 = 4.8 KB | PSRAM |
| Sprite Pet | ~20 KB | PSRAM |
| JSON ADN | ~2 KB | PSRAM |
| Stack tareas | 4 KB c/u | SRAM |

---

## 3. Tareas FreeRTOS MVP

### 3.1 Solo 3 Tareas

```
┌─────────────────────────────────────────────┐
│           FreeRTOS Scheduler                │
├─────────────────────────────────────────────┤
│                                             │
│  ┌──────────────┐  Prioridad 5              │
│  │ display_task │  ← LVGL refresh (30 FPS) │
│  └──────────────┘                           │
│                                             │
│  ┌──────────────┐  Prioridad 3              │
│  │ combat_task  │  ← Loop combate          │
│  └──────────────┘                           │
│                                             │
│  ┌──────────────┐  Prioridad 1              │
│  │ main_task    │  ← Init y supervisión    │
│  └──────────────┘                           │
│                                             │
└─────────────────────────────────────────────┘
```

### 3.2 Especificación

| Tarea | Prioridad | Stack | Periodo | Función |
|-------|-----------|-------|---------|---------|
| `display_task` | 5 | 4 KB | 33 ms | LVGL timer handler |
| `combat_task` | 3 | 4 KB | 500 ms | Resolver turno combate |
| `main_task` | 1 | 8 KB | - | Boot, init, monitoreo |

---

## 4. Flujo de Boot MVP

```
┌─────────────┐
│   Power On  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Init GPIO  │
│  Init SPI   │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Mount SD   │────► FAIL ──► Error screen
└──────┬──────┘
       │ OK
       ▼
┌─────────────┐
│  Load DNA   │────► FAIL ──► Default DNA (hardcoded)
└──────┬──────┘
       │ OK
       ▼
┌─────────────┐
│  Generate   │
│  Pet from   │
│  DNA        │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Init LCD   │
│  Init LVGL  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Create UI  │
│  (HUD)      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Start      │
│  Tasks      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Spawn      │
│  Enemy      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Combat     │
│  Loop       │◄─────────────────────┐
└──────┬──────┘                      │
       │                             │
       ▼                             │
┌─────────────────┐                  │
│ ¿Pet HP = 0?    │── Sí ──► Death   │
└──────┬──────────┘                  │
       │ No                          │
       ▼                             │
┌─────────────────┐                  │
│ ¿Enemy HP = 0?  │── Sí ──► Victory │
└──────┬──────────┘         │        │
       │ No                 │        │
       ▼                    ▼        │
┌─────────────────┐  ┌────────────┐  │
│ Resolve Turn    │  │ Level Up   │  │
│ (d20 + stats)   │  │ New Enemy  │  │
└──────┬──────────┘  └─────┬──────┘  │
       │                   │         │
       └───────────────────┴─────────┘
```

---

## 5. Flujo de Combate MVP

### 5.1 Turno Simple

```
combat_task (cada 500ms)
│
├──► 1. Tirada d20 Pet
│    └── random(1, 20) + modificador_ataque
│
├──► 2. ¿Golpea?
│    └── Si tirada >= AC enemigo → hit
│
├──► 3. Calcular daño
│    └── daño = base_damage + modificador_fuerza
│
├──► 4. Aplicar daño a enemigo
│    └── enemy_hp -= daño
│
├──► 5. ¿Enemigo muere?
│    ├── Sí → Victoria, EXP, ¿Level up?
│    └── No → Continuar
│
├──► 6. Tirada d20 Enemigo
│    └── (mismo proceso inverso)
│
├──► 7. ¿Pet muere?
│    ├── Sí → Death sequence
│    └── No → Continuar
│
└──► 8. Actualizar UI
     └── Enviar evento a display_task
```

### 5.2 Resolución d20

```c
typedef struct {
    int8_t str_mod;    // Modificador de Fuerza
    int8_t dex_mod;    // Modificador de Destreza
    int8_t con_mod;    // Modificador de Constitución
    int8_t int_mod;    // Modificador de Inteligencia
    int8_t wis_mod;    // Modificador de Sabiduría
    int8_t cha_mod;    // Modificador de Carisma
} attribute_mods_t;

int roll_attack(attribute_mods_t *mods, bool is_ranged) {
    int roll = esp_random() % 20 + 1;  // d20
    
    if (is_ranged) {
        roll += mods->dex_mod;
    } else {
        roll += mods->str_mod;
    }
    
    return roll;
}

int calculate_damage(int base_damage, attribute_mods_t *mods, bool is_ranged) {
    int dmg = base_damage;
    
    if (is_ranged) {
        dmg += mods->dex_mod;
    } else {
        dmg += mods->str_mod;
    }
    
    return dmg > 0 ? dmg : 1;  // Mínimo 1 de daño
}
```

---

## 6. Sistema de Datos MVP

### 6.1 Estructuras Principales

```c
// Pet
typedef struct {
    char name[16];
    uint8_t level;
    uint32_t exp;
    uint32_t exp_to_next;
    
    int16_t hp;
    int16_t hp_max;
    
    uint8_t str;
    uint8_t dex;
    uint8_t con;
    uint8_t intel;
    uint8_t wis;
    uint8_t cha;
    
    uint8_t profession_id;
    uint8_t skill_id;
    
    uint32_t dp;  // Deity Points
} pet_t;

// Enemigo
typedef struct {
    char name[16];
    int16_t hp;
    int16_t hp_max;
    uint8_t ac;
    uint8_t attack_bonus;
    uint8_t damage_dice;  // d6, d8, etc.
    uint8_t damage_bonus;
} enemy_t;

// ADN
typedef struct {
    char id[33];  // Hash hex
    
    uint8_t str_base;
    uint8_t dex_base;
    uint8_t con_base;
    uint8_t int_base;
    uint8_t wis_base;
    uint8_t cha_base;
    
    float hunger_delta;
    float energy_delta;
    
    uint8_t str_cap;
    uint8_t dex_cap;
    uint8_t con_cap;
    uint8_t int_cap;
    uint8_t wis_cap;
    uint8_t cha_cap;
} dna_t;
```

### 6.2 JSON de ADN

```json
{
    "id": "1a2b3c4d5e6f",
    "attributes": {
        "str": 14,
        "dex": 12,
        "con": 16,
        "int": 10,
        "wis": 8,
        "cha": 11
    },
    "caps": {
        "str": 18,
        "dex": 16,
        "con": 20,
        "int": 14,
        "wis": 12,
        "cha": 15
    }
}
```

---

## 7. Sistema de Archivos MVP

### 7.1 Estructura en SD

```
/SD_ROOT/
├── DNA/
│   └── pet_dna.json      # ADN del Pet
│
├── CONFIG/
│   └── state.json        # Estado persistido (DP, lives)
│
└── ASSETS/
    └── sprites/          # Sprites (opcional, puede ser hardcoded)
```

### 7.2 Estado Persistido

```json
{
    "dp": 150,
    "lives": 3,
    "enemies_defeated": 12
}
```

---

## 8. UI MVP

### 8.1 Layout (240x320)

```
┌────────────────────────────┐
│         [Nombre Pet]       │  Línea 1
│         Nivel: 5           │  Línea 2
├────────────────────────────┤
│                            │
│      ┌──────────┐          │
│      │          │          │
│      │  SPRITE  │          │  Área Pet (160x160)
│      │   PET    │          │
│      │          │          │
│      └──────────┘          │
│                            │
├────────────────────────────┤
│  HP: ████████░░  80/100   │  Barra HP
│  EXP: ████░░░░░░  45/100   │  Barra EXP
├────────────────────────────┤
│  [Nombre Enemigo]          │
│  HP: ██████░░░░  60/100    │  Barra HP Enemigo
├────────────────────────────┤
│  DP: 150    Profesión: War │  Status bar
└────────────────────────────┘
```

### 8.2 Eventos UI

```c
typedef enum {
    UI_EVENT_HP_CHANGED,
    UI_EVENT_EXP_CHANGED,
    UI_EVENT_LEVEL_UP,
    UI_EVENT_ENEMY_SPAWNED,
    UI_EVENT_ENEMY_DIED,
    UI_EVENT_PET_HIT,
    UI_EVENT_PET_DIED,
} ui_event_type_t;

typedef struct {
    ui_event_type_t type;
    int16_t value;
} ui_event_t;
```

---

## 9. Profesiones y Habilidades MVP

### 9.1 Profesiones

| ID | Nombre | Requisito | Bonus |
|----|--------|-----------|-------|
| 0 | Novato | Nivel 1 | Ninguno |
| 1 | Guerrero | STR ≥ 14 | +2 daño melee |
| 2 | Mago | INT ≥ 14 | +2 daño mágico |
| 3 | Pícaro | DEX ≥ 14 | +2 daño ranged |
| 4 | Clérigo | WIS ≥ 14 | +1 HP/regen |

### 9.2 Habilidades por Nivel

| Nivel | Profesión | Habilidad | Efecto |
|-------|-----------|-----------|--------|
| 1 | Todas | Básico | Ataque normal |
| 3 | Guerrero | Tacleo | +3 daño, -2 AC |
| 3 | Mago | Misil | +4 daño mágico |
| 3 | Pícaro | Crítico | 15% x2 daño |
| 3 | Clérigo | Curar | +10 HP |
| 5 | Guerrero | Defensa | +3 AC |
| 5 | Mago | Escudo | +2 AC |
| 5 | Pícaro | Evasión | +3 AC |
| 5 | Clérigo | Resurrect | Revive 1/combate |

---

## 10. Niveles y Progresión MVP

### 10.1 Curva de EXP

| Nivel | EXP Requerida | HP Bonus |
|-------|---------------|----------|
| 1 | 0 | +0 |
| 2 | 100 | +5 |
| 3 | 250 | +5 |
| 4 | 500 | +10 |
| 5 | 1000 | +10 |
| 6 | 2000 | +15 |
| 7 | 4000 | +15 |
| 8 | 8000 | +20 |
| 9 | 16000 | +20 |
| 10 | 32000 | +25 |

### 10.2 Subida de Stats

```c
void level_up(pet_t *pet, dna_t *dna) {
    pet->level++;
    pet->exp = 0;
    pet->exp_to_next = get_exp_for_level(pet->level + 1);
    
    // HP máximo aumenta
    pet->hp_max += get_hp_bonus(pet->level);
    pet->hp = pet->hp_max;  // Full heal on level up
    
    // Stat increase (probabilidad basada en caps)
    if (pet->str < dna->str_cap && (esp_random() % 100) < 30) {
        pet->str++;
    }
    // ... similar para otros stats
    
    // Desbloquear profesión si cumple requisitos
    check_profession_unlock(pet);
    
    // Desbloquear habilidad si aplica
    check_skill_unlock(pet);
}
```

---

## 11. Muerte y Renacimiento MVP

### 11.1 Secuencia de Muerte

```c
void pet_death(pet_t *pet) {
    // 1. Guardar DP
    uint32_t dp = pet->dp;
    storage_save_dp(dp);
    
    // 2. Mostrar pantalla de muerte
    ui_show_death_screen(pet);
    
    // 3. Esperar "aceptar" (timeout 5s)
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // 4. Reiniciar Pet
    pet_reset(pet);
    
    // 5. Restaurar DP
    pet->dp = dp;
    
    // 6. Volver a combate
    spawn_enemy();
}
```

### 11.2 Generación de DP

| Acción | DP Ganados |
|--------|------------|
| Victoria combate | +5 |
| Level up | +10 |
| Muerte (bonus) | +2 × nivel |

---

## 12. Componentes del Firmware MVP

### 12.1 Estructura Mínima

```
firmware/
├── main/
│   ├── CMakeLists.txt
│   ├── main.c              # Entry point
│   └── app_main.c          # Init tasks
│
├── components/
│   ├── dna/
│   │   ├── dna_parser.c    # Parse JSON ADN
│   │   └── dna_parser.h
│   │
│   ├── combat/
│   │   ├── combat_engine.c # Motor d20
│   │   ├── combat_engine.h
│   │   ├── enemy.c         # Generación enemigos
│   │   └── enemy.h
│   │
│   ├── pet/
│   │   ├── pet.c           # Lógica del Pet
│   │   ├── pet.h
│   │   ├── level.c         # Progresión
│   │   └── profession.c    # Profesiones
│   │
│   ├── hud/
│   │   ├── ui.c            # LVGL screens
│   │   ├── ui.h
│   │   └── sprites.c       # Sprites hardcoded
│   │
│   └── storage/
│       ├── sd_card.c       # Montar SD
│       └── sd_card.h
│
├── CMakeLists.txt
├── sdkconfig.defaults
└── idf_component.yml
```

### 12.2 Dependencias MVP

```
main
 ├──► hud ──────► lvgl
 │
 ├──► combat ───► pet
 │
 ├──► pet ──────► dna
 │
 └──► storage ──► sdmmc
                 ► fatfs
```

---

## 13. Flujo de Datos MVP

```
┌─────────────────────────────────────────────────────────────┐
│                         MAIN TASK                           │
│  - Init hardware                                            │
│  - Mount SD                                                 │
│  - Load DNA                                                 │
│  - Create Pet                                               │
│  - Start other tasks                                        │
└────────────────────────────┬────────────────────────────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  DISPLAY TASK   │  │  COMBAT TASK    │  │     SD CARD     │
│                 │  │                 │  │                 │
│ - LVGL handler  │◄─│ - Turn logic    │  │ - DNA load      │
│ - Update bars   │  │ - d20 rolls     │  │ - State save    │
│ - Animations    │  │ - Damage calc   │  │                 │
│                 │  │ - Level up      │  │                 │
└─────────────────┘  └────────┬────────┘  └─────────────────┘
                             │
                             ▼
                     ┌───────────────┐
                     │  UI EVENTS    │
                     │  (Queue)      │
                     └───────────────┘
```

---

## 14. Archivos de Configuración MVP

### 14.1 sdkconfig.defaults

```
# ESP32-S3
CONFIG_IDF_TARGET="esp32s3"

# PSRAM
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_40M=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000

# LVGL (via Kconfig)
CONFIG_LV_MEM_CUSTOM=y
CONFIG_LV_MEM_CUSTOM_ALLOC="heap_caps_malloc"
CONFIG_LV_MEM_CUSTOM_FREE="heap_caps_free"
```

### 14.2 idf_component.yml

```yaml
dependencies:
  lvgl/lvgl:
    version: "8.3.*"
  espressif/esp_lcd_st7789:
    version: "*"
```

---

## 15. Testing MVP

### 15.1 Test Unitarios

```c
// test_combat.c
void test_d20_roll_range(void) {
    for (int i = 0; i < 1000; i++) {
        int roll = roll_d20();
        TEST_ASSERT_TRUE(roll >= 1 && roll <= 20);
    }
}

void test_damage_calculation(void) {
    attribute_mods_t mods = {.str_mod = 3};
    int dmg = calculate_damage(5, &mods, false);
    TEST_ASSERT_EQUAL_INT(8, dmg);
}

void test_level_up(void) {
    pet_t pet = {.level = 1, .exp = 100};
    level_up(&pet, &default_dna);
    TEST_ASSERT_EQUAL_INT(2, pet.level);
    TEST_ASSERT_EQUAL_INT(0, pet.exp);
}
```

### 15.2 Test en Hardware

1. Compilar y flashear
2. Verificar LCD enciende
3. Verificar Pet aparece en pantalla
4. Verificar combate inicia automáticamente
5. Verificar HP bars actualizan
6. Verificar level up funciona
7. Verificar muerte y renacimiento

---

## 16. Scripts MVP

### 16.1 scripts/flash/flash_device.ps1

```powershell
# Compilar y flashear MVP
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

### 16.2 scripts/upload/upload_dna.ps1

```powershell
# Copiar ADN a SD
$sdDrive = "E:\"  # Ajustar según sistema
Copy-Item "assets\dna\test_dna.json" -Destination "$sdDrive\DNA\pet_dna.json"
```

---

## 17. Resumen MVP

| Componente | Archivos | Líneas aprox. |
|------------|----------|---------------|
| Main | 2 | 150 |
| DNA Parser | 2 | 100 |
| Combat Engine | 4 | 300 |
| Pet Logic | 4 | 250 |
| HUD/UI | 3 | 400 |
| Storage | 2 | 100 |
| **Total** | **17** | **~1300** |

Tiempo estimado: 2-3 semanas de desarrollo.
