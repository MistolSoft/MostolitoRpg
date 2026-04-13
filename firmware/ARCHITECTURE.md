# MistolitoRPG v2.0 - Documento de Arquitectura

## Visión General

MistolitoRPG es un idle RPG para ESP32-S3 con pantalla LCD. El sistema utiliza FreeRTOS con múltiples tasks coordinadas por una máquina de estados central.

---

## Arquitectura de Tasks

### Diagrama General

```
┌──────────────────────────────────────────────────────────────────────┐
│ CORE 0                                                                │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │ game_coordinator_task (Priority 4)                             │  │
│  │                                                                 │  │
│  │  STATE MACHINE:                                                │  │
│  │  [IDLE] → [SEARCHING] → [COMBAT] → [VICTORY] → [LEVELUP]     │  │
│  │      ↓                                              ↓          │  │
│  │  [RESTING] ◄───────────────────────────────────────────┘      │  │
│  │                                                                 │  │
│  │  - Posee el SNAPSHOT (única fuente de verdad)                  │  │
│  │  - Activa worker tasks vía Task Notifications                  │  │
│  │  - Recibe resultados de workers vía Task Notifications         │  │
│  │  - Emite eventos a UI vía Queue                                │  │
│  │  - Envía delta changes a Storage vía Queue                     │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                                    │                                  │
│           ┌────────────────────────┼────────────────────────┐        │
│           │                        │                        │        │
│           ▼                        ▼                        ▼        │
│  ┌─────────────┐          ┌─────────────┐          ┌─────────────┐  │
│  │ combat_task │          │ search_task │          │ rest_task   │  │
│  │ (Priority 3)│          │ (Priority 2)│          │ (Priority 1)│  │
│  │             │          │             │          │             │  │
│  │ SUSPENDED   │          │ SUSPENDED   │          │ SUSPENDED   │  │
│  │ por defecto │          │ por defecto │          │ por defecto │  │
│  │             │          │             │          │             │  │
│  │ - Espera    │          │ - Delay     │          │ - Delay     │  │
│  │   señal     │          │   3-8 seg   │          │   1 seg     │  │
│  │ - Ejecuta   │          │ - Genera    │          │ - Incrementa│  │
│  │   1 turno   │          │   enemigo   │          │   energía   │  │
│  │ - Notifica  │          │ - Notifica  │          │ - Notifica  │  │
│  │   resultado │          │   completion│          │   completion│  │
│  └─────────────┘          └─────────────┘          └─────────────┘  │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │ storage_task (Priority 1)                                      │  │
│  │                                                                 │  │
│  │  - Recibe requests vía Queue                                    │  │
│  │  - Guarda solo campos modificados (delta)                       │  │
│  │  - Operaciones 100% asíncronas                                  │  │
│  │  - No bloquea nunca al coordinator                              │  │
│  └────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Event Queue
                                    ▼
┌──────────────────────────────────────────────────────────────────────┐
│ CORE 1                                                                │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │ display_task (Priority 5)                                      │  │
│  │                                                                 │  │
│  │  - Recibe eventos del coordinator (non-blocking, 10ms timeout) │  │
│  │  - Renderiza UI a 2-4 FPS                                       │  │
│  │  - Solo LEE el snapshot, nunca escribe                          │  │
│  │  - No mantiene estado propio                                    │  │
│  └────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Tasks Detalladas

### 1. game_coordinator_task (Priority 4, Core 0)

**Responsabilidad**: Máquina de estados central del juego.

**Comportamiento**:
- Mantiene el snapshot (única fuente de verdad del estado del juego)
- Ejecuta la máquina de estados principal
- Activa worker tasks según el estado actual
- Recibe notificaciones de workers
- Emite eventos a display_task
- Envía delta changes a storage_task

**Estados**:
```
[IDLE] → [SEARCHING] → [COMBAT] → [VICTORY] → [LEVELUP] → [IDLE]
    ↓
[RESTING] → [IDLE]
```

---

### 2. combat_task (Priority 3, Core 0)

**Responsabilidad**: Ejecutar un turno de combate.

**Comportamiento**:
- Permanece SUSPENDED por defecto
- Coordinator lo activa con `xTaskNotify()` cuando entra en estado COMBAT
- Por cada notificación del coordinator, ejecuta UN turno:
  - Calcula attack roll (d20 + DEX mod)
  - Si hit: calcula daño según profesión
  - Actualiza variables locales (enemy_hp, damage_dealt)
  - Notifica resultado al coordinator con valor (damage) o código especial (enemy_dead)
- Permanece en loop esperando siguiente notificación
- Coordinator lo suspende al salir de COMBAT

**Nota**: El pet no muere, solo mata. No hay manejo de muerte del pet en MVP.

---

### 3. search_task (Priority 2, Core 0)

**Responsabilidad**: Búsqueda de enemigos.

**Comportamiento**:
- Permanece SUSPENDED por defecto
- Coordinator lo activa con `xTaskNotify()` cuando entra en estado SEARCHING
- Realiza delay aleatorio (3-8 segundos)
- Genera enemigo aleatorio basado en nivel del pet
- Notifica al coordinator con el enemy_id generado
- Se suspende automáticamente al completar

---

### 4. rest_task (Priority 1, Core 0)

**Responsabilidad**: Recuperación de energía.

**Comportamiento**:
- Permanece SUSPENDED por defecto
- Coordinator lo activa con `xTaskNotify()` cuando entra en estado RESTING
- Cada 1 segundo:
  - Incrementa energía +1
  - Notifica al coordinator con el nuevo valor de energía
- Se suspende cuando coordinator lo notifica (energy == max)

---

### 5. storage_task (Priority 1, Core 0)

**Responsabilidad**: Persistencia asíncrona en SD card.

**Comportamiento**:
- Espera en cola (blocking, sin timeout)
- Recibe requests con:
  - Tipo de operación (SAVE_PET_DELTA, LOAD_PET, LOAD_TABLES)
  - Dirty flags (qué campos cambiaron)
  - Puntero a datos
- Abre archivo JSON
- Lee JSON existente (o crea nuevo)
- Actualiza SOLO campos con dirty_flag activo
- Escribe JSON compacto
- (Opcional) Notifica completado

**Operaciones**:
- `SAVE_PET_DELTA`: Guarda solo campos modificados del pet
- `LOAD_PET`: Carga pet completo al inicio
- `LOAD_TABLES`: Carga tablas de juego (exp, hp_bonus, etc.)

---

### 6. display_task (Priority 5, Core 1)

**Responsabilidad**: Renderizado de UI.

**Comportamiento**:
- Loop principal:
  1. Intenta recibir evento de cola (non-blocking, 10ms timeout)
  2. Si hay evento: actualiza estado interno
  3. Lee snapshot completo (sin mutex, coordinator no modifica durante render)
  4. Renderiza UI con `lv_timer_handler()`
  5. `vTaskDelay(250ms)` para 4 FPS, o `vTaskDelay(500ms)` para 2 FPS

**Nota**: No mantiene estado propio, todo lo obtiene del snapshot.

---

## Mecanismos de Sincronización

### 1. Task Notifications (Coordinator ↔ Workers)

**Coordinator → Worker**:
```c
xTaskNotify(worker_task_handle, 0, eNoAction);  // Solo señalizar
xTaskNotify(worker_task_handle, value, eSetValueWithOverwrite);  // Con valor
```

**Worker → Coordinator**:
```c
xTaskNotify(coordinator_task_handle, result_value, eSetValueWithOverwrite);
```

**Coordinator esperando**:
```c
uint32_t result = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
```

### 2. Event Queue (Coordinator → Display)

```c
QueueHandle_t g_event_queue;  // 16 slots, sizeof(game_event_t)
```

**Coordinator envía**:
```c
game_event_t evt = { .type = EVENT_DAMAGE_DEALT, .data.damage = 15 };
xQueueSend(g_event_queue, &evt, 0);  // Non-blocking
```

**Display recibe**:
```c
game_event_t evt;
if (xQueueReceive(g_event_queue, &evt, pdMS_TO_TICKS(10)) == pdTRUE) {
    // Procesar evento
}
```

### 3. Storage Queue (Coordinator → Storage)

```c
QueueHandle_t g_storage_queue;  // 4 slots, sizeof(storage_request_t)
```

**Coordinator envía**:
```c
storage_request_t req = {
    .operation = SAVE_PET_DELTA,
    .dirty_flags = PET_DIRTY_EXP | PET_DIRTY_LEVEL,
    .pet = &snapshot.pet
};
xQueueSend(g_storage_queue, &req, 0);  // Non-blocking
```

---

## Snapshot

El snapshot es la **única representación del estado del juego**.

**Propietario**: Solo `game_coordinator_task` puede modificarlo.

**Lectura**: `display_task` lee el snapshot completo cada frame.

**Estructura**:
```
game_snapshot_t:
├── state: game_state_e
├── pet:
│   ├── name[16]
│   ├── level
│   ├── exp, exp_next
│   ├── hp, hp_max
│   ├── energy, energy_max
│   ├── stats[6]  (STR, DEX, CON, INT, WIS, CHA)
│   ├── profession
│   ├── dp (deity points)
│   ├── enemies_killed
│   └── dirty_flags
├── enemy:
│   ├── name[16]
│   ├── hp, hp_max
│   ├── ac
│   ├── level
│   ├── exp_reward
│   └── alive (bool)
├── combat:
│   ├── last_damage
│   ├── turn_count
│   └── last_action
└── timing:
    ├── state_entered_at (ms)
    └── state_duration (ms)
```

### Dirty Flags (para Storage Optimizado)

```c
#define PET_DIRTY_NAME       (1 << 0)
#define PET_DIRTY_LEVEL      (1 << 1)
#define PET_DIRTY_EXP        (1 << 2)
#define PET_DIRTY_HP         (1 << 3)
#define PET_DIRTY_ENERGY     (1 << 4)
#define PET_DIRTY_STATS      (1 << 5)
#define PET_DIRTY_PROFESSION (1 << 6)
#define PET_DIRTY_DP         (1 << 7)
```

**Uso**:
```c
// Cuando coordinator modifica algo:
snapshot.pet.exp += exp_gained;
snapshot.pet.dirty_flags |= PET_DIRTY_EXP;

// Cuando envía a storage:
storage_request_t req = {
    .operation = SAVE_PET_DELTA,
    .dirty_flags = snapshot.pet.dirty_flags,
    .pet = &snapshot.pet
};
xQueueSend(g_storage_queue, &req, 0);
snapshot.pet.dirty_flags = 0;  // Clear después de enviar
```

---

## Eventos UI (Game → Display)

```c
typedef enum {
    EVENT_STATE_CHANGED,      // Nuevo estado del juego
    EVENT_PET_UPDATED,        // Stats del pet cambiaron
    EVENT_ENEMY_SPAWNED,      // Nuevo enemigo visible
    EVENT_ENEMY_DIED,         // Enemigo derrotado
    EVENT_DAMAGE_DEALT,       // Daño al enemigo
    EVENT_DAMAGE_TAKEN,       // Daño al pet (futuro)
    EVENT_VICTORY,            // Ganó combate
    EVENT_LEVEL_UP,           // Subió de nivel
    EVENT_ENERGY_CHANGED,     // Energía cambió
    EVENT_RESTING_STARTED,    // Entró en descanso
    EVENT_RESTING_ENDED       // Energía full
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

---

## Flujo por Estado

### IDLE

**Coordinator**:
- Espera notificación o timeout
- Si `energy > 0` → transición a SEARCHING
- Si `energy == 0` → transición a RESTING

**Snapshot updates**:
- Ninguno

---

### SEARCHING

**Coordinator**:
1. Actualiza `snapshot.state = STATE_SEARCHING`
2. Emite evento `STATE_CHANGED`
3. Notifica a `search_task` (activate)
4. Espera notificación de `search_task`

**search_task**:
1. Recibe señal de activate
2. Delay aleatorio (3-8 segundos)
3. Genera enemigo aleatorio
4. Notifica al coordinator con `enemy_id`

**Coordinator (al recibir notificación)**:
1. Llena `snapshot.enemy` con datos del enemigo generado
2. Emite evento `ENEMY_SPAWNED`
3. Transición a COMBAT

---

### COMBAT

**Coordinator**:
1. Actualiza `snapshot.state = STATE_COMBAT`
2. Emite evento `STATE_CHANGED`
3. Notifica a `combat_task` (activate)
4. Loop:
   - Notifica a `combat_task` para ejecutar turno
   - Espera notificación con resultado
   - Procesa resultado:
     - Si `DAMAGE`: actualiza `snapshot.enemy.hp`, emite `DAMAGE_DEALT`
     - Si `ENEMY_DIED`: transición a VICTORY
   - Delay 1 segundo entre turnos (opcional, para UI)

**combat_task**:
1. Recibe señal de activate inicial
2. Loop:
   - Espera notificación del coordinator (turno)
   - Ejecuta cálculos de combate:
     - Attack roll: d20 + DEX_mod
     - Si hit: damage = dice_roll + STR_mod + profession_bonus
   - Notifica resultado:
     - `damage_dealt` (valor positivo)
     - `enemy_dead` (código especial, ej: 0xFFFF)

---

### VICTORY

**Coordinator**:
1. Actualiza `snapshot.state = STATE_VICTORY`
2. Calcula EXP ganada: `snapshot.pet.exp += enemy.exp_reward`
3. `snapshot.pet.dirty_flags |= PET_DIRTY_EXP`
4. Emite evento `VICTORY` con exp_ganada
5. Verifica level up:
   - Si `snapshot.pet.exp >= snapshot.pet.exp_next`:
     - Transición a LEVELUP
6. Envía `SAVE_PET_DELTA` a storage (async)
7. Delay 2 segundos
8. Transición a IDLE

---

### LEVELUP

**Coordinator**:
1. Actualiza `snapshot.state = STATE_LEVELUP`
2. **BLOQUEANTE**: Consulta tablas de juego:
   - Calcula nuevo `hp_max`
   - Calcula stat increases
   - Obtiene nuevo `exp_next`
3. Actualiza `snapshot.pet`:
   - `level++`
   - `exp = 0`
   - `exp_next` = valor de tabla
   - `hp_max` = calculado
   - `hp = hp_max` (full heal)
   - `stats` = actualizados
   - `dirty_flags |= PET_DIRTY_LEVEL | PET_DIRTY_EXP | PET_DIRTY_HP | PET_DIRTY_STATS`
4. Emite evento `LEVEL_UP`
5. Emite evento `PET_UPDATED`
6. Envía `SAVE_PET_DELTA` a storage (async)
7. Delay 2 segundos
8. Transición a IDLE

**Nota**: Level up es bloqueante porque necesita acceder a tablas y hacer cálculos. Es aceptable porque el juego está pausado durante la animación de level up.

---

### RESTING

**Coordinator**:
1. Actualiza `snapshot.state = STATE_RESTING`
2. Emite evento `STATE_CHANGED`
3. Notifica a `rest_task` (activate)
4. Loop:
   - Espera notificación de `rest_task`
   - `snapshot.pet.energy++`
   - `snapshot.pet.dirty_flags |= PET_DIRTY_ENERGY`
   - Emite evento `ENERGY_CHANGED`
   - Si `snapshot.pet.energy >= MAX_ENERGY`:
     - Notifica a `rest_task` (suspend)
     - Transición a IDLE

**rest_task**:
1. Recibe señal de activate
2. Loop:
   - Delay 1 segundo
   - Notifica al coordinator (energy tick)
   - Si recibe señal de suspend: break

---

## Storage: Operaciones Delta

### Request Structure

```c
typedef struct {
    uint8_t operation;
    uint8_t dirty_flags;
    const pet_t *pet;
} storage_request_t;

#define STORAGE_OP_SAVE_PET_DELTA  1
#define STORAGE_OP_LOAD_PET        2
#define STORAGE_OP_LOAD_TABLES     3
```

### Storage Task Flow

```
storage_task loop:
│
├─ Espera en cola (blocking)
│
├─ Recibe request
│
├─ Switch operation:
│   │
│   ├─ SAVE_PET_DELTA:
│   │   ├─ Abre /sdcard/BRAIN/PET/pet_data.json
│   │   ├─ Lee JSON existente (o crea nuevo)
│   │   ├─ Para cada dirty_flag:
│   │   │   └─ Actualiza campo correspondiente
│   │   ├─ Escribe JSON compacto
│   │   └─ Cierra archivo
│   │
│   ├─ LOAD_PET:
│   │   ├─ Abre archivo
│   │   ├─ Parsea JSON
│   │   └─ Llena estructura pet
│   │
│   └─ LOAD_TABLES:
│       ├─ Abre /sdcard/DATA/game_tables.json
│       ├─ Parsea JSON
│       └─ Llena estructura tables
│
└─ (Opcional) Notifica completado
```

---

## Archivos JSON

### pet_data.json (Compacto)

```json
{"name":"Mistolito","level":1,"exp":0,"hp":20,"hp_max":20,"energy":10,"profession":0,"dp":0,"stats":[10,10,10,10,10,10]}
```

### game_tables.json (Ejemplo)

```json
{
  "exp_table": [100, 250, 500, 1000, 2000, 4000, 8000, 15000, 30000, 60000],
  "hp_bonus": [0, 5, 5, 10, 10, 15, 15, 20, 20, 25],
  "stat_intervals": [
    [0,0,0,0,0,0],
    [1,0,0,0,0,0],
    [0,1,0,0,0,0],
    [0,0,1,0,0,0],
    [0,0,0,1,0,0],
    [0,0,0,0,1,0],
    [0,0,0,0,0,1],
    [1,1,0,0,0,0],
    [0,0,1,1,0,0],
    [1,1,1,1,1,1]
  ]
}
```

---

## Hardware Configuration

### GPIO Pins

| Función | GPIO |
|---------|------|
| LCD_CS  | 45   |
| LCD_DC  | 42   |
| LCD_BL  | 1    |
| LCD_MOSI| 38   |
| LCD_CLK | 39   |
| SD_MISO | 40   |
| SD_MOSI | 38   (shared with LCD) |
| SD_CLK  | 39   (shared with LCD) |
| SD_CS   | 41   |

### Display

- ST7789, 320x240, landscape
- SPI bus shared with SD card
- LVGL 9.x
- 2-4 FPS target

---

## Prioridad de Implementación

### Fase 1: Core System
1. Implementar `game_coordinator_task` con máquina de estados
2. Implementar snapshot structure
3. Implementar event queue
4. Implementar `display_task` básico (solo renderiza snapshot)

### Fase 2: Worker Tasks
1. Implementar `search_task`
2. Implementar `combat_task`
3. Implementar `rest_task`

### Fase 3: Storage
1. Implementar `storage_task`
2. Implementar dirty flags
3. Implementar load/save delta

### Fase 4: Polish
1. Animaciones UI
2. Logs y debugging
3. Optimización de memoria

---

## Notas Importantes

1. **El pet no muere en MVP**: Solo mata enemigos. La muerte del pet se implementará después.

2. **Level up es bloqueante**: Es aceptable porque pausa el juego para mostrar animación.

3. **Storage siempre async**: Nunca bloquea al coordinator. Usa dirty flags para optimizar.

4. **Snapshot sin mutex**: Coordinator es el único escritor. Display lee en su propio contexto temporal.

5. **Combat task por turnos**: Coordinator notifica cada vez que quiere ejecutar un turno.
