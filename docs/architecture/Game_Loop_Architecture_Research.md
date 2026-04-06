# Game Loop Architecture Research Report

## Executive Summary

This report analyzes game loop architecture patterns for embedded systems (ESP32-S3 with FreeRTOS) suitable for a turn-based idle RPG game. Based on the current MistolitoRPG implementation and industry patterns, we provide recommendations for handling combat animations, enemy counter-attacks, user input, and multiple enemies.

---

## 1. Current Architecture Analysis

### 1.1 Existing Implementation

The current architecture uses:

```
Core 0 (Game Logic):
├── game_coordinator_task (Priority 4) - State machine, owns snapshot
├── combat_worker_task (Priority 3) - Suspended until activated
├── search_worker_task (Priority 2) - Suspended until activated
├── rest_worker_task (Priority 1) - Suspended until activated
└── storage_task (Priority 1) - Async SD operations

Core 1 (Display):
└── display_task (Priority 5) - LVGL rendering, 4 FPS target
```

**Communication:**
- Task Notifications: Coordinator ↔ Workers (blocking, synchronous)
- Event Queue: Coordinator → Display (non-blocking, fire-and-forget)
- Direct snapshot pointer: Display reads coordinator's snapshot (lock-free read)

### 1.2 Current State Machine

```
GS_INIT → GS_SEARCHING → GS_COMBAT → GS_VICTORY → GS_LEVELUP
              ↑              │              │
              │              ↓              │
              └──────── GS_RESTING ←────────┘
                         (energy=0)
```

### 1.3 Identified Gaps

1. **No sub-states for combat phases** - GS_COMBAT is monolithic
2. **No enemy counter-attack logic** - Only player attacks implemented
3. **No animation timing system** - Display just reacts to events
4. **No user input during combat** - Combat is fully automatic
5. **Single enemy only** - No multi-enemy encounter support

---

## 2. Game Loop Patterns Analysis

### 2.1 Frame-Based Game Loop (Traditional)

```c
while (running) {
    process_input();
    update(delta_time);
    render();
    sleep(remaining_frame_time);
}
```

**Pros:**
- Simple to understand and implement
- Predictable timing
- Works well for real-time games

**Cons:**
- Not suitable for turn-based games
- Requires careful delta_time handling
- Power consumption concerns on embedded

**Verdict for MistolitoRPG:** ❌ Not recommended. Overkill for turn-based idle game.

### 2.2 Event-Based Game Loop (Current Approach)

```c
while (running) {
    event = wait_for_event(timeout);
    if (event) {
        handle_event(event);
    }
    update_animations();
    render();
}
```

**Pros:**
- Natural fit for turn-based mechanics
- Lower power consumption
- Simpler state management
- Already partially implemented

**Cons:**
- Animations need separate timing
- Can feel "jerky" without proper interpolation
- Complex event routing for multiple systems

**Verdict for MistolitoRPG:** ✅ Recommended. Extend current approach.

### 2.3 Hybrid Loop (Fixed Update, Variable Render)

```c
double lag = 0.0;
while (running) {
    double elapsed = get_elapsed_time();
    lag += elapsed;
    
    process_input();
    
    while (lag >= FIXED_TIMESTEP) {
        update();  // Fixed timestep for game logic
        lag -= FIXED_TIMESTEP;
    }
    
    render(lag / FIXED_TIMESTEP);  // Interpolate for smooth visuals
}
```

**Pros:**
- Deterministic game logic
- Smooth rendering with interpolation
- Handles variable frame rates well
- Industry standard (Gaffer On Games recommendation)

**Cons:**
- More complex implementation
- Requires interpolation logic in render
- May need multiple updates per frame on slow hardware

**Verdict for MistolitoRPG:** ⚠️ Consider for future real-time mode. Overkill for current turn-based.

---

## 3. State Machine Patterns Analysis

### 3.1 Flat State Machine (Current)

```c
switch (state) {
    case GS_COMBAT:
        // All combat logic here
        break;
}
```

**Pros:**
- Simple to understand
- Fast execution (direct jump table)
- Low memory overhead

**Cons:**
- Becomes unwieldy with complex states
- Code duplication for shared behaviors
- Hard to add new features

**Verdict for MistolitoRPG:** ⚠️ Works for simple cases, needs enhancement.

### 3.2 Hierarchical State Machine (HSM)

```
GS_COMBAT (parent)
├── CS_PLAYER_TURN
│   ├── PS_SELECT_ACTION
│   ├── PS_SELECT_TARGET
│   └── PS_EXECUTE_ACTION
├── CS_ENEMY_TURN
│   ├── ES_SELECT_ACTION
│   └── ES_EXECUTE_ACTION
├── CS_ANIMATION_PLAYING
└── CS_VICTORY_CHECK
```

**Pros:**
- Shared behavior at parent level
- Cleaner organization
- Easier to add new sub-states
- Natural fit for combat phases

**Cons:**
- More complex implementation
- Need to handle state stack
- Overhead for simple cases

**Verdict for MistolitoRPG:** ✅ Recommended for combat subsystem.

### 3.3 Pushdown Automata (State History)

```c
// State stack for returning after interrupts
void push_state(state);
void pop_state();  // Returns to previous state
```

**Pros:**
- Natural handling of "temporary" states
- Easy to return after modal dialogs
- Good for nested menus

**Cons:**
- More memory usage (stack)
- More complex to debug

**Verdict for MistolitoRPG:** ⚠️ Useful for UI menus, not needed for core game.

---

## 4. Animation and Game Logic Separation

### 4.1 Current Approach: Event-Driven

```
Coordinator emits EVT_DAMAGE_DEALT
     ↓
Display receives event
     ↓
Display shows damage popup + animation
     ↓
Display waits for animation duration
     ↓
Display clears popup
```

**Problem:** Coordinator doesn't wait for animations.

### 4.2 Option A: Coordinator Blocks on Animations

```c
// Coordinator
emit_animation_event(ANIM_ATTACK);
wait_for_animation_complete(timeout);

// Display
receive_animation_event();
play_animation();
notify_animation_complete();
```

**Pros:**
- Simple synchronization
- Guaranteed animation timing
- Game pace matches visuals

**Cons:**
- Blocks game logic during animations
- Wastes CPU cycles
- Doesn't scale to complex animations

**Verdict:** ❌ Not recommended for idle game.

### 4.3 Option B: Animation State Synchronization

```c
typedef struct {
    animation_id_e current_anim;
    uint32_t start_time_ms;
    uint32_t duration_ms;
    bool complete;
} animation_state_t;

// Snapshot includes animation state
typedef struct {
    game_state_e state;
    combat_phase_e combat_phase;
    animation_state_t animations[MAX_ANIMATIONS];
    // ... rest of snapshot
} game_snapshot_t;
```

**Pros:**
- Display interpolates based on timing
- Non-blocking for coordinator
- Smooth animations possible
- Supports multiple concurrent animations

**Cons:**
- More complex snapshot structure
- Display needs interpolation logic

**Verdict:** ✅ Recommended.

### 4.4 Option C: Separate Animation Task

```
Core 0: Game Logic Task (Coordinator)
Core 1: Display Task (LVGL)
Core 1: Animation Task (Separate FreeRTOS task)
```

**Pros:**
- Dedicated animation timing
- Can run at higher rate than display
- Precise animation control

**Cons:**
- Additional task overhead
- Complex synchronization
- Overkill for 4 FPS target

**Verdict:** ❌ Not recommended.

---

## 5. Cross-Core State Synchronization

### 5.1 Current Approach: Direct Pointer Access

```c
// display_task.c
game_snapshot_t *snap = game_coordinator_get_snapshot();
screens_update(snap);
```

**Problem:** Race condition if coordinator modifies while display reads.

### 5.2 Option A: Double Buffering

```c
typedef struct {
    game_snapshot_t buffers[2];
    volatile uint8_t write_buffer;
    volatile uint8_t read_buffer;
} double_buffer_t;

// Coordinator writes to write_buffer
// Display reads from read_buffer
// Swap buffers on vsync
```

**Pros:**
- No race conditions
- Consistent view for display
- Industry standard pattern

**Cons:**
- Memory overhead (2x snapshot)
- Swap timing complexity
- Display may see stale data

**Verdict:** ✅ Recommended for real-time mode.

### 5.3 Option B: Mutex Protection

```c
static SemaphoreHandle_t snapshot_mutex;

// Coordinator
xSemaphoreTake(snapshot_mutex, portMAX_DELAY);
modify_snapshot();
xSemaphoreGive(snapshot_mutex);

// Display
xSemaphoreTake(snapshot_mutex, pdMS_TO_TICKS(10));
read_snapshot();
xSemaphoreGive(snapshot_mutex);
```

**Pros:**
- Simple to implement
- True synchronization
- No memory overhead

**Cons:**
- Can block display task
- Priority inversion risk
- Performance overhead

**Verdict:** ⚠️ Acceptable for current turn-based pace.

### 5.4 Option C: Atomic Flag + Copy

```c
static volatile bool snapshot_updating = false;
static game_snapshot_t snapshot;

// Coordinator
snapshot_updating = true;
// ... modify ...
snapshot_updating = false;

// Display
if (!snapshot_updating) {
    // Read safely
}
```

**Pros:**
- Very low overhead
- No blocking

**Cons:**
- Not truly atomic on multi-word structs
- Display may skip updates
- Potential for partial reads

**Verdict:** ❌ Not safe for complex structs.

### 5.5 Option D: ESP32 IPC (Inter-Processor Call)

```c
// Execute callback on specific core
esp_ipc_call(1, update_display_callback, &snapshot);
```

**Pros:**
- Official ESP32 mechanism
- Handles cache coherency
- Safe cross-core calls

**Cons:**
- Blocking call
- Overkill for simple data sharing
- Not needed for our use case

**Verdict:** ❌ Not needed for current architecture.

---

## 6. Entity Component System (ECS) Analysis

### 6.1 Traditional ECS

```c
struct PositionComponent { float x, y; };
struct HealthComponent { int16_t hp, max_hp; };
struct CombatComponent { uint8_t attack, defense; };

Entity entities[MAX_ENTITIES];
// Systems iterate over components
```

**Pros:**
- Extremely flexible
- Great for many entities
- Data-oriented design
- Easy to add new components

**Cons:**
- Complex implementation
- Memory fragmentation
- Overhead for few entities
- Not natural for turn-based

**Verdict for MistolitoRPG:** ❌ Overkill for 1 pet + few enemies.

### 6.2 Simplified Component Approach

```c
typedef struct {
    // Components are inline, not pointers
    combat_stats_t combat;
    display_state_t display;
    ai_state_t ai;
} enemy_t;

typedef struct {
    uint8_t count;
    enemy_t enemies[MAX_ENEMIES_PER_ENCOUNTER];
} enemy_group_t;
```

**Pros:**
- Simple memory layout
- Cache-friendly
- Easy to serialize
- Sufficient for our needs

**Cons:**
- Less flexible than full ECS
- Manual component management

**Verdict:** ✅ Recommended for multi-enemy support.

---

## 7. Recommended Architecture

### 7.1 Enhanced State Machine

```c
typedef enum {
    // Parent states
    GS_INIT,
    GS_SEARCHING,
    GS_COMBAT,
    GS_VICTORY,
    GS_LEVELUP,
    GS_RESTING,
    GS_DEAD,
    
    // Combat sub-states (GS_COMBAT children)
    CS_INIT,
    CS_PLAYER_SELECT_ACTION,
    CS_PLAYER_SELECT_TARGET,
    CS_PLAYER_EXECUTE,
    CS_PLAYER_ANIMATION,
    CS_ENEMY_SELECT_ACTION,
    CS_ENEMY_EXECUTE,
    CS_ENEMY_ANIMATION,
    CS_ROUND_END,
    CS_CHECK_VICTORY
} game_state_e;

typedef struct {
    game_state_e state;
    game_state_e parent_state;  // For HSM
    uint32_t state_entered_ms;
    uint8_t combat_round;
    uint8_t current_enemy_idx;
    action_e selected_action;
    uint8_t selected_target_idx;
} combat_phase_t;
```

### 7.2 Combat Flow with Sub-States

```
GS_COMBAT entered
    ↓
CS_INIT
    ├── Setup encounter
    └── → CS_PLAYER_SELECT_ACTION
    
CS_PLAYER_SELECT_ACTION
    ├── Wait for input (or AI auto-select)
    └── → CS_PLAYER_SELECT_TARGET (if needed)
    
CS_PLAYER_SELECT_TARGET
    ├── Wait for target selection
    └── → CS_PLAYER_EXECUTE
    
CS_PLAYER_EXECUTE
    ├── Calculate damage
    ├── Update HP
    └── → CS_PLAYER_ANIMATION
    
CS_PLAYER_ANIMATION
    ├── Emit animation event
    ├── Wait for animation duration
    └── → CS_ENEMY_SELECT_ACTION (or CS_CHECK_VICTORY)
    
CS_ENEMY_SELECT_ACTION
    ├── AI selects action
    └── → CS_ENEMY_EXECUTE
    
CS_ENEMY_EXECUTE
    ├── Calculate damage
    ├── Update HP
    └── → CS_ENEMY_ANIMATION
    
CS_ENEMY_ANIMATION
    ├── Emit animation event
    ├── Wait for animation duration
    └── → CS_ROUND_END
    
CS_ROUND_END
    ├── Apply DOT effects
    ├── Check deaths
    └── → CS_CHECK_VICTORY
    
CS_CHECK_VICTORY
    ├── All enemies dead? → GS_VICTORY
    ├── Pet dead? → GS_DEAD
    └── Else → CS_PLAYER_SELECT_ACTION
```

### 7.3 Multi-Enemy Support

```c
#define MAX_ENEMIES_PER_ENCOUNTER 3

typedef struct {
    enemy_t enemies[MAX_ENEMIES_PER_ENCOUNTER];
    uint8_t count;
    uint8_t active_enemy_idx;  // Current target
} encounter_t;

typedef struct {
    // ... existing fields
    encounter_t encounter;
    combat_phase_t combat;
} game_snapshot_t;

// Enemy generation scales with level
void search_worker_generate_encounter(game_snapshot_t *snap) {
    snap->encounter.count = 1 + (snap->pet.level / 3);
    if (snap->encounter.count > MAX_ENEMIES_PER_ENCOUNTER) {
        snap->encounter.count = MAX_ENEMIES_PER_ENCOUNTER;
    }
    
    for (uint8_t i = 0; i < snap->encounter.count; i++) {
        generate_enemy(&snap->encounter.enemies[i], snap->pet.level, i);
    }
}
```

### 7.4 Animation Timing System

```c
typedef struct {
    uint32_t start_ms;
    uint32_t duration_ms;
    animation_type_e type;
    uint8_t target_idx;
    bool active;
} animation_t;

typedef struct {
    animation_t pet_anim;
    animation_t enemy_anims[MAX_ENEMIES_PER_ENCOUNTER];
    animation_t ui_anim;  // Damage popups, etc.
} animation_state_t;

// In coordinator
void emit_animation(animation_type_e type, uint8_t target, uint32_t duration) {
    game_event_t evt = {
        .type = EVT_ANIMATION_START,
        .data.anim = { .type = type, .target = target, .duration = duration }
    };
    events_send(&evt);
    
    // Wait for animation (non-blocking approach)
    uint32_t wait_until = esp_timer_get_time() / 1000 + duration;
    // Coordinator can do other work or yield
}

// In display
void update_animations(game_snapshot_t *snap) {
    uint32_t now = esp_timer_get_time() / 1000;
    
    // Check animation progress
    if (snap->animations.pet_anim.active) {
        float progress = (float)(now - snap->animations.pet_anim.start_ms) / 
                         snap->animations.pet_anim.duration_ms;
        if (progress >= 1.0f) {
            snap->animations.pet_anim.active = false;
        } else {
            render_animation_frame(&snap->animations.pet_anim, progress);
        }
    }
}
```

### 7.5 Enemy Counter-Attack Implementation

```c
void execute_enemy_turn(game_snapshot_t *snap) {
    for (uint8_t i = 0; i < snap->encounter.count; i++) {
        if (!snap->encounter.enemies[i].alive) continue;
        
        enemy_t *enemy = &snap->encounter.enemies[i];
        
        // Calculate attack
        int8_t enemy_mod = get_modifier(enemy->dex);
        int16_t attack_roll = roll_d20() + enemy_mod;
        int8_t pet_ac = 10 + get_modifier(snap->pet.dex);
        
        if (attack_roll >= pet_ac) {
            // Hit!
            uint8_t damage = roll_dice(enemy->damage_dice) + enemy->damage_bonus;
            snap->pet.hp -= damage;
            
            // Emit event for animation
            game_event_t evt = {
                .type = EVT_ENEMY_ATTACK,
                .data.attack = { 
                    .enemy_idx = i, 
                    .damage = damage,
                    .hit = true 
                }
            };
            events_send(&evt);
        } else {
            // Miss
            game_event_t evt = {
                .type = EVT_ENEMY_ATTACK,
                .data.attack = { .enemy_idx = i, .hit = false }
            };
            events_send(&evt);
        }
        
        // Check pet death
        if (snap->pet.hp <= 0) {
            snap->pet.hp = 0;
            snap->pet.is_alive = false;
            break;
        }
    }
}
```

---

## 8. Memory Considerations

### 8.1 Current Memory Usage

```
Internal RAM (512KB):
├── FreeRTOS tasks: ~24KB (6 tasks × 4KB avg)
├── Stack per task: 4-8KB each
├── LVGL internal: ~32KB
└── System overhead: ~50KB

PSRAM (8MB):
├── LVGL frame buffers: ~300KB (2 × 320×240×2)
├── Game assets: ~2MB (sprites, tiles)
└── Available: ~5.5MB
```

### 8.2 Recommended Allocations

```c
// Snapshot in internal RAM for speed
static game_snapshot_t g_snapshot;  // ~500 bytes

// Animation state in internal RAM
static animation_state_t g_animations;  // ~100 bytes

// Enemy data in internal RAM (small)
static encounter_t g_encounter;  // ~200 bytes

// Large buffers in PSRAM
void *lvgl_buf = heap_caps_malloc(320 * 240 * 2, MALLOC_CAP_SPIRAM);
```

### 8.3 Memory Budget for Multi-Enemy

```c
// Per enemy
sizeof(enemy_t) = ~40 bytes

// Max 3 enemies = 120 bytes
// Negligible compared to LVGL buffers

// Encounter with animations
sizeof(encounter_t) + sizeof(animation_state_t) = ~400 bytes
// Still very manageable
```

---

## 9. Code Structure Recommendations

### 9.1 Proposed File Structure

```
components/mistolito_core/
├── game_coordinator.h/c      # Main state machine (enhanced)
├── combat_engine.h/c         # Combat logic (NEW)
├── enemy_ai.h/c              # Enemy AI (NEW)
├── animation_system.h/c      # Animation timing (NEW)
├── workers.h/c               # Background workers
├── display_task.h/c          # LVGL rendering
├── events.h/c                # Event queue
├── screens.h/c               # UI screens
├── snapshot.h/c              # Snapshot management (NEW)
└── mistolito.h               # Type definitions
```

### 9.2 Enhanced Event Types

```c
typedef enum {
    // Existing
    EVT_STATE_CHANGED,
    EVT_PET_UPDATED,
    EVT_ENEMY_SPAWNED,
    EVT_ENEMY_DIED,
    EVT_DAMAGE_DEALT,
    EVT_VICTORY,
    EVT_LEVEL_UP,
    EVT_ENERGY_CHANGED,
    EVT_GAME_START,
    
    // New events for enhanced combat
    EVT_COMBAT_PHASE_CHANGED,
    EVT_ANIMATION_START,
    EVT_ANIMATION_COMPLETE,
    EVT_ENEMY_ATTACK,
    EVT_TARGET_SELECTED,
    EVT_ACTION_SELECTED,
    EVT_ROUND_END,
    EVT_ENEMY_TARGET_CHANGED
} event_type_e;

typedef struct {
    event_type_e type;
    union {
        game_state_e new_state;
        struct { int16_t amount; bool critical; } damage;
        struct { uint32_t amount; } exp;
        struct { uint8_t level; } level_up;
        struct { uint8_t current; uint8_t max; } energy;
        
        // New data types
        struct { 
            animation_type_e type; 
            uint8_t target; 
            uint32_t duration;
        } anim;
        struct {
            uint8_t enemy_idx;
            int16_t damage;
            bool hit;
        } attack;
        struct {
            game_state_e phase;
            uint8_t round;
        } combat_phase;
    } data;
} game_event_t;
```

---

## 10. Implementation Phases

### Phase 1: Sub-States for Combat (Priority: HIGH)
- Implement combat_phase_e enum
- Modify coordinator to handle sub-states
- Add phase transition events
- Estimated effort: 2-3 days

### Phase 2: Enemy Counter-Attack (Priority: HIGH)
- Add enemy AI logic
- Implement counter-attack in combat flow
- Add enemy attack animations
- Estimated effort: 2-3 days

### Phase 3: Animation System (Priority: MEDIUM)
- Create animation timing structures
- Implement display-side interpolation
- Add animation events
- Estimated effort: 3-4 days

### Phase 4: Multi-Enemy Encounters (Priority: MEDIUM)
- Extend encounter_t structure
- Modify enemy generation
- Add target selection UI/logic
- Implement multi-enemy AI
- Estimated effort: 4-5 days

### Phase 5: User Input During Combat (Priority: LOW)
- Add input handling in display task
- Implement action selection
- Add target selection UI
- Estimated effort: 3-4 days

---

## 11. Summary of Recommendations

| Aspect | Pattern | Justification |
|--------|---------|---------------|
| **Game Loop** | Event-based (extend current) | Natural fit for turn-based, already implemented |
| **State Machine** | Hierarchical for combat | Cleaner organization, supports phases |
| **Animation** | State synchronization | Non-blocking, supports multiple concurrent anims |
| **Cross-Core Sync** | Mutex for current, double-buffer for future | Safe, low complexity |
| **Entity System** | Simplified inline components | Sufficient for 1 pet + few enemies |
| **Memory** | Snapshot in RAM, large buffers in PSRAM | Speed for game state, space for assets |

---

## 12. References

1. Game Programming Patterns - Robert Nystreth
   - State Pattern: https://gameprogrammingpatterns.com/state.html
   - Game Loop: https://gameprogrammingpatterns.com/game-loop.html
   - Update Method: https://gameprogrammingpatterns.com/update-method.html

2. ESP-IDF Documentation
   - FreeRTOS Overview: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
   - IPC: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ipc.html

3. Gaffer On Games - "Fix Your Timestep"
   - https://gafferongames.com/post/fix_your_timestep/

4. FreeRTOS Task Notifications
   - Lightweight alternative to queues for 1:1 signaling
   - Currently used correctly in implementation

---

## Appendix A: Code Example - Enhanced Combat Coordinator

```c
void game_coordinator_task(void *arg) {
    while (1) {
        switch (g_snapshot.state) {
            case GS_COMBAT:
                handle_combat_state();
                break;
            
            // ... other states
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void handle_combat_state(void) {
    switch (g_snapshot.combat.phase) {
        case CS_INIT:
            init_combat_encounter();
            transition_combat_phase(CS_PLAYER_SELECT_ACTION);
            break;
            
        case CS_PLAYER_SELECT_ACTION:
            if (g_auto_combat || g_selected_action != ACTION_NONE) {
                g_snapshot.combat.selected_action = g_selected_action;
                transition_combat_phase(CS_PLAYER_SELECT_TARGET);
            }
            break;
            
        case CS_PLAYER_SELECT_TARGET:
            if (g_selected_target < g_snapshot.encounter.count) {
                g_snapshot.combat.selected_target = g_selected_target;
                transition_combat_phase(CS_PLAYER_EXECUTE);
            }
            break;
            
        case CS_PLAYER_EXECUTE:
            execute_player_action();
            transition_combat_phase(CS_PLAYER_ANIMATION);
            break;
            
        case CS_PLAYER_ANIMATION:
            if (animation_complete()) {
                if (all_enemies_dead()) {
                    transition_combat_phase(CS_CHECK_VICTORY);
                } else {
                    transition_combat_phase(CS_ENEMY_SELECT_ACTION);
                }
            }
            break;
            
        case CS_ENEMY_SELECT_ACTION:
            select_enemy_actions();
            transition_combat_phase(CS_ENEMY_EXECUTE);
            break;
            
        case CS_ENEMY_EXECUTE:
            execute_enemy_turn();
            transition_combat_phase(CS_ENEMY_ANIMATION);
            break;
            
        case CS_ENEMY_ANIMATION:
            if (animation_complete()) {
                transition_combat_phase(CS_ROUND_END);
            }
            break;
            
        case CS_ROUND_END:
            apply_round_effects();
            transition_combat_phase(CS_CHECK_VICTORY);
            break;
            
        case CS_CHECK_VICTORY:
            if (all_enemies_dead()) {
                transition_to(GS_VICTORY);
            } else if (!g_snapshot.pet.is_alive) {
                transition_to(GS_DEAD);
            } else {
                g_snapshot.combat.round++;
                transition_combat_phase(CS_PLAYER_SELECT_ACTION);
            }
            break;
    }
}

static void transition_combat_phase(game_state_e new_phase) {
    g_snapshot.combat.phase = new_phase;
    g_snapshot.combat.phase_entered_ms = esp_timer_get_time() / 1000;
    
    game_event_t evt = {
        .type = EVT_COMBAT_PHASE_CHANGED,
        .data.combat_phase = {
            .phase = new_phase,
            .round = g_snapshot.combat.round
        }
    };
    events_send(&evt);
}
```

---

## Appendix B: Animation Interpolation Example

```c
// In display_task.c
static void render_with_interpolation(game_snapshot_t *snap) {
    uint32_t now = esp_timer_get_time() / 1000;
    
    // Interpolate pet sprite position for smooth movement
    if (snap->animations.pet_anim.active) {
        animation_t *anim = &snap->animations.pet_anim;
        float progress = (float)(now - anim->start_ms) / anim->duration_ms;
        progress = clamp(progress, 0.0f, 1.0f);
        
        // Apply easing function
        float eased = ease_out_quad(progress);
        
        // Render pet at interpolated position
        int16_t pet_x = lerp(snap->pet.base_x, snap->pet.target_x, eased);
        int16_t pet_y = lerp(snap->pet.base_y, snap->pet.target_y, eased);
        
        lv_obj_set_pos(pet_sprite, pet_x, pet_y);
    }
    
    // Render enemies
    for (uint8_t i = 0; i < snap->encounter.count; i++) {
        if (snap->animations.enemy_anims[i].active) {
            render_enemy_animation(&snap->animations.enemy_anims[i], i);
        }
    }
    
    lv_timer_handler();
}

static float ease_out_quad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static int16_t lerp(int16_t a, int16_t b, float t) {
    return a + (int16_t)((b - a) * t);
}
```

---

*Document Version: 1.0*
*Last Updated: 2026-04-06*
*Author: Architecture Analysis*
