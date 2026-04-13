# Sistema de ADN Criptográfico - DNA Engine

Este documento describe el sistema de generación determinista de estadísticas y tiradas de dados mediante hashing criptográfico.

---

## 1. Visión General

El DNA Engine es el subsistema responsable de:

1. **Cargar códigos ADN** desde almacenamiento externo (SD)
2. **Generar hashes criptográficos** deterministas
3. **Derivar estadísticas** del Pet mediante método 4d6 drop lowest
4. **Calcular tiradas d20** para combate y progresión
5. **Gestionar límites genéticos** (CAPs) y level-ups

### 1.1 Principios de Diseño

| Principio | Descripción |
|-----------|-------------|
| **Determinismo controlado** | Mismo ADN + misma sal = mismos resultados |
| **Variabilidad entre vidas** | Cada renacimiento genera stats diferentes |
| **Complejidad progresiva** | Hash permite agregar capas de complejidad futura |
| **Reproducibilidad** | Con sal conocida, todo es reproducible |

### 1.2 Dependencias

| Librería | Uso | Justificación |
|----------|-----|---------------|
| `mbedtls/sha256.h` | Generación de hash | Hardware acelerado en ESP32-S3 (GDMA) |
| `esp_random.h` | Generación de sal | TRNG de hardware para entropía |

---

## 2. Flujo de Vida del ADN

### 2.1 Diagrama General

```
┌─────────────────────────────────────────────────────────────────────┐
│                        CICLO DE VIDA COMPLETO                       │
└─────────────────────────────────────────────────────────────────────┘

     ┌──────────────┐
     │  SD CARD     │
     │  pet_dna.json│
     └──────┬───────┘
            │
            ▼
┌───────────────────────────────────────────────────────────────────────┐
│                        NACIMIENTO (Boot)                              │
│                                                                       │
│  1. Cargar códigos ADN ───────────────────────────────────────┐      │
│     [STR][DEX][CON][INT][WIS][CHA]                             │      │
│     "A3fK9x" "B7mP2q" "C1nL8w" "D5hR4t" "E9kM6y" "F2jS7z"    │      │
│                                                                 ▼      │
│  2. Generar sal ──────────────────────────────────────> sal = rand()  │
│                                                                       │
│  3. Concatenar y hashear ─────────────────────────────────────────►   │
│     input = códigos + sal                                            │
│     hash = SHA256(input) ──────────────────────────────► 256 bits    │
│                                                                       │
│  4. Derivar estadísticas                                             │
│     ┌─────────────────────────────────────────────────────────┐      │
│     │  hash[0-39]  ──► STR (4d6 drop lowest) ──► 15           │      │
│     │  hash[40-79] ──► DEX (4d6 drop lowest) ──► 11           │      │
│     │  hash[80-119]──► CON (4d6 drop lowest) ──► 16           │      │
│     │  hash[120-159]► INT (4d6 drop lowest) ──► 12            │      │
│     │  hash[160-199]► WIS (4d6 drop lowest) ──► 9             │      │
│     │  hash[200-239]► CHA (4d6 drop lowest) ──► 13            │      │
│     └─────────────────────────────────────────────────────────┘      │
│                                                                       │
│  5. Calcular CAPs genéticos                                          │
│     CAP[stat] = base_stat + 5                                        │
│     STR_CAP = 15 + 5 = 20                                            │
│                                                                       │
│  6. Guardar sal en memoria (solo durante esta vida)                  │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
            │
            ▼
┌───────────────────────────────────────────────────────────────────────┐
│                        VIDA DEL PET                                   │
│                                                                       │
│  • Combate idle con d20 derivados del hash                           │
│  • Level-ups con candidatos limitados                                │
│  • Stats nunca superan CAP genético                                  │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
            │
            ▼
┌───────────────────────────────────────────────────────────────────────┐
│                        LEVEL UP                                       │
│                                                                       │
│  Para cada stat (6 stats):                                           │
│  ┌─────────────────────────────────────────────────────────────┐     │
│  │  1. Calcular probabilidad                                   │     │
│  │     DC = 10 + (stat / 2)                                    │     │
│  │     roll = d20(stat_hash + level)                           │     │
│  │                                                             │     │
│  │  2. Evaluar                                                 │     │
│  │     Si roll ≥ DC ──► stat puede subir ──► agregar a cola   │     │
│  │                                                             │     │
│  │  3. Limitar                                                 │     │
│  │     Si cola tiene más de 2 candidatos:                      │     │
│  │       - Seleccionar 2 aleatoriamente                        │     │
│  │       - Descartar el resto                                  │     │
│  │                                                             │     │
│  │  4. Aplicar                                                 │     │
│  │     Cada candidato seleccionado: stat += 1                  │     │
│  │     Verificar: stat < CAP (si no, cancelar)                 │     │
│  └─────────────────────────────────────────────────────────────┘     │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
            │
            ▼
┌───────────────────────────────────────────────────────────────────────┐
│                        MUERTE                                         │
│                                                                       │
│  1. HP llega a 0                                                     │
│  2. Animación de muerte                                              │
│  3. Guardar DP persistidos (SD)                                      │
│  4. Descartar sal actual                                             │
│  5. Descartar hash actual                                            │
│  6. Preservar códigos ADN                                            │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
            │
            ▼
┌───────────────────────────────────────────────────────────────────────┐
│                        RENACIMIENTO                                   │
│                                                                       │
│  1. Mismos códigos ADN (persisten)                                   │
│  2. Nueva sal = esp_random()                                         │
│  3. Nuevo hash = SHA256(códigos + nueva_sal)                         │
│  4. Recalcular todos los stats (4d6 drop lowest)                     │
│  5. Recalcular todos los CAPs                                        │
│                                                                       │
│  Resultado: Pet con mismos genes pero diferentes atributos           │
│                                                                       │
│  Ejemplo:                                                            │
│  ┌─────────────────────────────────────────────────────────────┐     │
│  │  Vida 1 (sal=0xA3F...): STR=15 DEX=11 CON=16 INT=12 ...    │     │
│  │  Vida 2 (sal=0x7B2...): STR=11 DEX=16 CON=9  INT=14 ...    │     │
│  │  Vida 3 (sal=0x1E9...): STR=13 DEX=12 CON=14 INT=10 ...    │     │
│  └─────────────────────────────────────────────────────────────┘     │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
            │
            └──────────────► [VOLVER A NACIMIENTO]
```

---

## 3. Modelo de Datos

### 3.1 Estructura `dna_t`

```
┌─────────────────────────────────────────────────────────────────────┐
│                          dna_t                                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  codes: char[6][7]                                                   │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ [0] "A3fK9x" ──► Código para STR                              │   │
│  │ [1] "B7mP2q" ──► Código para DEX                              │   │
│  │ [2] "C1nL8w" ──► Código para CON                              │   │
│  │ [3] "D5hR4t" ──► Código para INT                              │   │
│  │ [4] "E9kM6y" ──► Código para WIS                              │   │
│  │ [5] "F2jS7z" ──► Código para CHA                              │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  hash: uint8_t[32]                                                   │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ SHA-256 de (códigos concatenados + sal)                      │   │
│  │ 256 bits = 32 bytes                                          │   │
│  │                                                              │   │
│  │ [0-4]   ──► Bits para STR (40 bits)                          │   │
│  │ [5-9]   ──► Bits para DEX (40 bits)                          │   │
│  │ [10-14] ──► Bits para CON (40 bits)                          │   │
│  │ [15-19] ──► Bits para INT (40 bits)                          │   │
│  │ [20-24] ──► Bits para WIS (40 bits)                          │   │
│  │ [25-29] ──► Bits para CHA (40 bits)                          │   │
│  │ [30-31] ──► Reservados                                       │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  salt: uint32_t                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ Valor aleatorio de 32 bits                                   │   │
│  │ Generado por esp_random() al nacer                           │   │
│  │ Único por vida del Pet                                       │   │
│  │ Se descarta al morir                                         │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  base_stats: uint8_t[6]                                              │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ [0] STR base = 15                                            │   │
│  │ [1] DEX base = 11                                            │   │
│  │ [2] CON base = 16                                            │   │
│  │ [3] INT base = 12                                            │   │
│  │ [4] WIS base = 9                                             │   │
│  │ [5] CHA base = 13                                            │   │
│  │                                                              │   │
│  │ Valores calculados al nacer mediante 4d6 drop lowest         │   │
│  │ Se mantienen fijos para calcular CAPs                        │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  caps: uint8_t[6]                                                    │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ [0] STR_CAP = 15 + 5 = 20                                    │   │
│  │ [1] DEX_CAP = 11 + 5 = 16                                    │   │
│  │ [2] CON_CAP = 16 + 5 = 21                                    │   │
│  │ [3] INT_CAP = 12 + 5 = 17                                    │   │
│  │ [4] WIS_CAP = 9 + 5 = 14                                     │   │
│  │ [5] CHA_CAP = 13 + 5 = 18                                    │   │
│  │                                                              │   │
│  │ Fórmula: CAP = base_stat + 5                                 │   │
│  │ Un stat nunca puede superar su CAP                           │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 Estructura `levelup_queue_t`

```
┌─────────────────────────────────────────────────────────────────────┐
│                     levelup_queue_t                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  candidates: uint8_t[6]                                              │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ Índices de stats que pasaron la tirada de level-up           │   │
│  │                                                              │   │
│  │ Ejemplo: [0, 2, 4] significa STR, CON, WIS pueden subir      │   │
│  │                                                              │   │
│  │ Valores posibles:                                           │   │
│  │   0 = STAT_STR                                              │   │
│  │   1 = STAT_DEX                                              │   │
│  │   2 = STAT_CON                                              │   │
│  │   3 = STAT_INT                                              │   │
│  │   4 = STAT_WIS                                              │   │
│  │   5 = STAT_CHA                                              │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  count: uint8_t                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ Número de candidatos válidos en el array                    │   │
│  │                                                              │   │
│  │ Si count > 2: se seleccionan 2 aleatoriamente               │   │
│  │ Si count ≤ 2: todos suben                                   │   │
│  │ Si count = 0: ningún stat sube                              │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.3 Estructura `roll_context_t`

```
┌─────────────────────────────────────────────────────────────────────┐
│                     roll_context_t                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  dna: dna_t*                                                         │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ Puntero al ADN del Pet                                       │   │
│  │ Contiene hash y códigos necesarios para la tirada           │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  stat_idx: uint8_t                                                   │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ Índice del stat usado para esta tirada                      │   │
│  │ 0-5 (STR, DEX, CON, INT, WIS, CHA)                          │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  enemy_id: uint8_t                                                   │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ ID del enemigo actual                                        │   │
│  │ Se incluye en el sal para variación por enemigo             │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  round: uint8_t                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ Número de ronda del combate                                  │   │
│  │ Se incluye en el sal para variación por ronda               │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  action: const char*                                                 │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │ Tipo de acción: "attack", "defend", "skill", etc.           │   │
│  │ Se incluye en el sal para variación por tipo de acción      │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 4. Algoritmos

### 4.1 Algoritmo de Derivación de Stats (4d6 Drop Lowest)

```
┌─────────────────────────────────────────────────────────────────────┐
│                DERIVACIÓN DE STAT DESDE HASH                        │
└─────────────────────────────────────────────────────────────────────┘

ENTRADA:
  - hash: array de 32 bytes (SHA-256)
  - stat_idx: índice del stat (0-5)

SALIDA:
  - valor del stat (3-18)

ALGORITMO:

  1. Calcular offset
     ┌───────────────────────────────────────────────────────────┐
     │  offset = stat_idx × 5                                    │
     │                                                           │
     │  Cada stat usa 5 bytes del hash (40 bits)                │
     │  STR: offset = 0  (bytes 0-4)                             │
     │  DEX: offset = 5  (bytes 5-9)                             │
     │  CON: offset = 10 (bytes 10-14)                           │
     │  INT: offset = 15 (bytes 15-19)                           │
     │  WIS: offset = 20 (bytes 20-24)                           │
     │  CHA: offset = 25 (bytes 25-29)                           │
     └───────────────────────────────────────────────────────────┘

  2. Extraer 4 valores de "dado" (simular 1d6 cada uno)
     ┌───────────────────────────────────────────────────────────┐
     │  d1 = (hash[offset + 0] mod 6) + 1  → rango 1-6           │
     │  d2 = (hash[offset + 1] mod 6) + 1  → rango 1-6           │
     │  d3 = (hash[offset + 2] mod 6) + 1  → rango 1-6           │
     │  d4 = (hash[offset + 3] mod 6) + 1  → rango 1-6           │
     └───────────────────────────────────────────────────────────┘

  3. Ordenar los dados de menor a mayor
     ┌───────────────────────────────────────────────────────────┐
     │  dados = [d1, d2, d3, d4]                                 │
     │  sort_ascending(dados)                                    │
     │                                                           │
     │  Ejemplo: [3, 5, 4, 6] → ordenado → [3, 4, 5, 6]          │
     └───────────────────────────────────────────────────────────┘

  4. Descartar el dado más bajo y sumar los otros 3
     ┌───────────────────────────────────────────────────────────┐
     │  stat = dados[1] + dados[2] + dados[3]                    │
     │                                                           │
     │  Ejemplo: [3, 4, 5, 6] → descartar 3 → 4+5+6 = 15         │
     │                                                           │
     │  Rango posible: 3 (mínimo: 1+1+1) a 18 (máximo: 6+6+6)   │
     └───────────────────────────────────────────────────────────┘

  5. Retornar stat
     ┌───────────────────────────────────────────────────────────┐
     │  return stat  (3-18)                                      │
     └───────────────────────────────────────────────────────────┘


DISTRIBUCIÓN RESULTANTE:
  ┌─────────────────────────────────────────────────────────────┐
  │  Esta distribución es idéntica al método D&D 5e:           │
  │                                                             │
  │  Frecuencia de valores:                                    │
  │  3:  ████ (0.08%)        11: ██████████████████ (9.7%)    │
  │  4:  ██████ (0.19%)      12: ████████████████████ (10.1%) │
  │  5:  ████████ (0.35%)    13: ██████████████████ (9.7%)    │
  │  6:  ██████████ (0.62%)  14: ████████████ (7.0%)          │
  │  7:  ████████████ (0.97%) 15: ████████ (4.6%)             │
  │  8:  ██████████████ (1.4%) 16: ██████ (2.3%)              │
  │  9:  ██████████████ (2.0%) 17: ████ (0.97%)               │
  │  10: ████████████████ (2.8%) 18: ██ (0.46%)               │
  │                                                             │
  │  Promedio: ~12.24                                          │
  │  Mediana: 12                                               │
  │  Moda: 13                                                  │
  └─────────────────────────────────────────────────────────────┘
```

### 4.2 Algoritmo de Tirada d20 desde Hash

```
┌─────────────────────────────────────────────────────────────────────┐
│                   TIRADA d20 DETERMINISTA                           │
└─────────────────────────────────────────────────────────────────────┘

ENTRADA:
  - dna: estructura con hash y códigos
  - stat_idx: índice del stat (0-5)
  - action_salt: sal dinámica (enemy_id + round + action_type)

SALIDA:
  - valor d20 (1-20)

ALGORITMO:

  1. Construir input para hash
     ┌───────────────────────────────────────────────────────────┐
     │  input = dna.hash                    (32 bytes)           │
     │         + dna.codes[stat_idx]        (6 bytes)            │
     │         + action_salt                (variable)           │
     │                                                           │
     │  action_salt típicamente incluye:                         │
     │    - enemy_id (1 byte)                                    │
     │    - round_number (1 byte)                                │
     │    - action_type string ("attack", "defend", etc.)        │
     └───────────────────────────────────────────────────────────┘

  2. Generar hash SHA-256
     ┌───────────────────────────────────────────────────────────┐
     │  output_hash = SHA256(input)                              │
     │                                                           │
     │  Usar mbedtls_sha256 con aceleración de hardware          │
     │  (GDMA en ESP32-S3)                                       │
     └───────────────────────────────────────────────────────────┘

  3. Extraer valor numérico del hash
     ┌───────────────────────────────────────────────────────────┐
     │  value = (output_hash[0] << 24)                           │
     │        | (output_hash[1] << 16)                           │
     │        | (output_hash[2] << 8)                            │
     │        | output_hash[3]                                   │
     │                                                           │
     │  Esto produce un entero de 32 bits                        │
     │  Rango: 0 a 4,294,967,295                                 │
     └───────────────────────────────────────────────────────────┘

  4. Mapear a rango 1-20
     ┌───────────────────────────────────────────────────────────┐
     │  d20 = (value mod 20) + 1                                 │
     │                                                           │
     │  Rango final: 1-20                                        │
     │  Distribución: uniforme (cada valor tiene 5% prob.)       │
     └───────────────────────────────────────────────────────────┘

  5. Retornar d20
     ┌───────────────────────────────────────────────────────────┐
     │  return d20                                               │
     └───────────────────────────────────────────────────────────┘


EJEMPLO:
  ┌─────────────────────────────────────────────────────────────┐
  │  Input:                                                    │
  │    dna.hash = 0x8E4F... (32 bytes)                         │
  │    stat_idx = 0 (STR)                                      │
  │    codes[0] = "A3fK9x"                                     │
  │    action_salt = enemy_id:5 + round:3 + "attack"           │
  │                                                             │
  │  SHA256 concatenado → output_hash                          │
  │  output_hash[0-3] = 0x7F3A2B1C                             │
  │  value = 2,133,916,252                                     │
  │  d20 = (2,133,916,252 mod 20) + 1 = 13                     │
  │                                                             │
  │  Resultado: Tirada de 13                                   │
  └─────────────────────────────────────────────────────────────┘
```

### 4.3 Algoritmo de Level-Up

```
┌─────────────────────────────────────────────────────────────────────┐
│                      PROCESO DE LEVEL-UP                            │
└─────────────────────────────────────────────────────────────────────┘

ENTRADA:
  - dna: estructura con hash y stats actuales
  - level: nivel actual del Pet
  - current_stats: array de 6 valores actuales

SALIDA:
  - stats_actualizados: array con máximo 2 stats incrementados

ALGORITMO:

  1. FASE DE EVALUACIÓN
     ┌───────────────────────────────────────────────────────────┐
     │  Para cada stat (i = 0 a 5):                              │
     │                                                           │
     │    a) Calcular DC (Dificultad)                            │
     │       DC = 10 + (current_stats[i] / 2)                    │
     │                                                           │
     │       Ejemplo:                                            │
     │         STR = 15 → DC = 10 + 7 = 17                       │
     │         DEX = 11 → DC = 10 + 5 = 15                       │
     │         CON = 9  → DC = 10 + 4 = 14                       │
     │                                                           │
     │    b) Realizar tirada                                     │
     │       sal = level + i                                     │
     │       roll = d20(dna, i, sal)                             │
     │                                                           │
     │    c) Evaluar resultado                                   │
     │       Si roll ≥ DC:                                       │
     │         └─► Agregar i a cola de candidatos                │
     │       Si roll < DC:                                       │
     │         └─► Descartar                                     │
     │                                                           │
     │    d) Verificar CAP                                       │
     │       Si current_stats[i] ≥ caps[i]:                      │
     │         └─► No puede subir (remover de candidatos)        │
     └───────────────────────────────────────────────────────────┘

  2. FASE DE SELECCIÓN
     ┌───────────────────────────────────────────────────────────┐
     │  candidatos = cola de stats que pasaron la tirada         │
     │                                                           │
     │  Si len(candidatos) > 2:                                  │
     │    ┌─────────────────────────────────────────────────┐    │
     │    │  Seleccionar 2 candidatos aleatoriamente        │    │
     │    │  usando d20 para desempate                      │    │
     │    │                                                  │    │
     │    │  Ejemplo: [STR, CON, WIS] → seleccionar 2       │    │
     │    │  Tirada: STR=15, CON=11, WIS=9                  │    │
     │    │  Ordenar por tirada descendente                 │    │
     │    │  Seleccionados: STR y CON                       │    │
     │    └─────────────────────────────────────────────────┘    │
     │                                                           │
     │  Si len(candidatos) ≤ 2:                                  │
     │    └─► Todos los candidatos suben                        │
     └───────────────────────────────────────────────────────────┘

  3. FASE DE APLICACIÓN
     ┌───────────────────────────────────────────────────────────┐
     │  Para cada candidato seleccionado:                        │
     │                                                           │
     │    Si current_stats[i] < caps[i]:                         │
     │      current_stats[i] += 1                                │
     │      └─► Log: "STR subió a 16"                            │
     │                                                           │
     │    Si current_stats[i] ≥ caps[i]:                         │
     │      └─► Cancelar incremento (alcanzó CAP)                │
     └───────────────────────────────────────────────────────────┘

  4. RETORNAR
     ┌───────────────────────────────────────────────────────────┐
     │  return current_stats                                     │
     └───────────────────────────────────────────────────────────┘


EJEMPLO COMPLETO:
  ┌─────────────────────────────────────────────────────────────┐
  │  Pet nivel 2 → nivel 3                                     │
    │                                                             │
  │  Stats actuales:  STR=15 DEX=11 CON=16 INT=12 WIS=9 CHA=13 │
  │  CAPs:            STR=20 DEX=16 CON=21 INT=17 WIS=14 CHA=18│
  │                                                             │
  │  FASE 1 - Evaluación:                                      │
  │    STR: DC=17, roll=12 → ❌ No puede subir                  │
  │    DEX: DC=15, roll=18 → ✅ Puede subir                     │
  │    CON: DC=18, roll=20 → ✅ Puede subir                     │
  │    INT: DC=16, roll=14 → ❌ No puede subir                  │
  │    WIS: DC=14, roll=15 → ✅ Puede subir                     │
  │    CHA: DC=16, roll=10 → ❌ No puede subir                  │
  │                                                             │
  │  Cola de candidatos: [DEX, CON, WIS]                       │
  │                                                             │
  │  FASE 2 - Selección:                                       │
  │    3 candidatos > 2, seleccionar 2                         │
  │    Tiradas desempate: DEX=18, CON=20, WIS=15               │
  │    Ordenar: CON(20), DEX(18), WIS(15)                      │
  │    Seleccionados: CON y DEX                                │
  │                                                             │
  │  FASE 3 - Aplicación:                                      │
  │    CON: 16 < 21 (CAP) → CON = 17 ✅                         │
  │    DEX: 11 < 16 (CAP) → DEX = 12 ✅                         │
  │                                                             │
  │  Resultado: STR=15 DEX=12 CON=17 INT=12 WIS=9 CHA=13       │
  └─────────────────────────────────────────────────────────────┘
```

---

## 5. Formato de Archivo JSON

### 5.1 Estructura del Archivo

```
┌─────────────────────────────────────────────────────────────────────┐
│                    pet_dna.json                                     │
└─────────────────────────────────────────────────────────────────────┘

{
    "version": "1.0",
    "str": "A3fK9x",
    "dex": "B7mP2q",
    "con": "C1nL8w",
    "int": "D5hR4t",
    "wis": "E9kM6y",
    "cha": "F2jS7z"
}

┌─────────────────────────────────────────────────────────────────────┐
│  CAMPO       TIPO      DESCRIPCIÓN                                  │
├─────────────────────────────────────────────────────────────────────┤
│  version    string    Versión del formato (para futuras migraciones) │
│  str        string    Código ADN para Fuerza (6 caracteres)         │
│  dex        string    Código ADN para Destreza (6 caracteres)       │
│  con        string    Código ADN para Constitución (6 caracteres)   │
│  int        string    Código ADN para Inteligencia (6 caracteres)   │
│  wis        string    Código ADN para Sabiduría (6 caracteres)      │
│  cha        string    Código ADN para Carisma (6 caracteres)        │
└─────────────────────────────────────────────────────────────────────┘


CARACTERES VÁLIDOS:
  ┌─────────────────────────────────────────────────────────────────┐
  │  Base62: [0-9] [A-Z] [a-z]                                      │
  │                                                                 │
  │  62 caracteres posibles por posición                            │
  │  6 posiciones = 62^6 = 56,800,235,584 combinaciones posibles   │
  │                                                                 │
  │  Ejemplos válidos:                                              │
  │    "A3fK9x"                                                     │
  │    "zZ0zZz"                                                     │
  │    "123456"                                                     │
  │    "ABCDEF"                                                     │
    │    "abcdef"                                                     │
  └─────────────────────────────────────────────────────────────────┘
```

### 5.2 Ubicación en SD Card

```
┌─────────────────────────────────────────────────────────────────────┐
│                    ESTRUCTURA DE ARCHIVOS EN SD                     │
└─────────────────────────────────────────────────────────────────────┘

/SD_ROOT/
├── DNA/
│   └── pet_dna.json          ← Archivo de ADN (obligatorio)
│
├── CONFIG/
│   └── state.json            ← Estado persistido (DP, lives)
│
└── TABLES/
    ├── professions.json
    ├── enemies.json
    └── config.json
```

---

## 6. Integración con el Sistema de Combate

### 6.1 Flujo de Tiradas en Combate

```
┌─────────────────────────────────────────────────────────────────────┐
│               COMBATE: TIRADAS DETERMINISTAS                        │
└─────────────────────────────────────────────────────────────────────┘

PET ATACA:
  ┌───────────────────────────────────────────────────────────────┐
  │  1. Obtener stat relevante                                    │
  │     - Melee: STR                                              │
  │     - Ranged: DEX                                             │
  │     - Magic: INT                                              │
  │                                                               │
  │  2. Construir contexto de tirada                             │
  │     roll_context = {                                          │
  │       dna: &pet.dna,                                          │
  │       stat_idx: STAT_STR,                                     │
  │       enemy_id: current_enemy.id,                             │
  │       round: combat.round,                                    │
  │       action: "attack"                                        │
  │     }                                                         │
  │                                                               │
  │  3. Realizar tirada d20                                       │
  │     roll = dna_roll_d20_context(&roll_context)                │
  │                                                               │
  │  4. Agregar modificador                                       │
  │     attack = roll + get_modifier(stat)                        │
  │                                                               │
  │  5. Comparar con AC del enemigo                              │
  │     Si attack >= enemy.AC → hit                              │
  │     Si attack < enemy.AC → miss                              │
  └───────────────────────────────────────────────────────────────┘

ENEMIGO ATACA:
  ┌───────────────────────────────────────────────────────────────┐
  │  El enemigo usa el mismo sistema pero con sus propios stats   │
  │                                                               │
  │  1. Construir contexto                                        │
  │     roll_context = {                                          │
  │       dna: &enemy_dna,                                        │
  │       stat_idx: STAT_STR,                                     │
  │       enemy_id: pet.id,      ← Pet como "enemigo"             │
  │       round: combat.round,                                    │
  │       action: "attack"                                        │
  │     }                                                         │
  │                                                               │
  │  2. Tirada d20                                                │
  │     roll = dna_roll_d20_context(&roll_context)                │
  │                                                               │
  │  3. Comparar con AC del Pet                                   │
  │     pet_ac = base_ac + get_modifier(pet.dex)                  │
  │     Si roll >= pet_ac → hit                                  │
  └───────────────────────────────────────────────────────────────┘


VARIACIÓN POR CONTEXTO:
  ┌───────────────────────────────────────────────────────────────┐
  │  El mismo ADN produce diferentes resultados según:            │
  │                                                               │
  │  • enemy_id: Cada enemigo es una experiencia única           │
  │  • round: Cada ronda es diferente                            │
  │  • action: Ataque vs Defensa vs Habilidad                    │
  │                                                               │
  │  Esto garantiza que:                                          │
  │  - No hay patrones predecibles                               │
  │  - Cada combate se siente único                              │
  │  - El Pet no puede "aprender" las tiradas                    │
  │  - Pero todo es determinista y reproducible                  │
  └───────────────────────────────────────────────────────────────┘
```

### 6.2 Ejemplo de Secuencia de Combate

```
┌─────────────────────────────────────────────────────────────────────┐
│              EJEMPLO: COMBATE COMPLETO                              │
└─────────────────────────────────────────────────────────────────────┘

ADN: STR="A3fK9x", DEX="B7mP2q", ...
Sal actual: 0x7F3A2B1C
Stats: STR=15(+2), DEX=11(+0), CON=16(+3)

Enemigo: Goblin (AC=12, id=5)

RONDA 1:
  ┌─────────────────────────────────────────────────────────────┐
  │  Pet ataca (STR):                                           │
  │    context = {stat:STR, enemy:5, round:1, action:"attack"}  │
  │    hash = SHA256(dna.hash + "A3fK9x" + 5 + 1 + "attack")    │
  │    roll = 14                                                │
  │    attack = 14 + 2(mod) = 16                                │
  │    16 >= 12 (AC) → HIT                                      │
  │    damage = 1d8 + 2 = 7                                     │
  │                                                             │
  │  Goblin ataca:                                              │
  │    context = {stat:STR, enemy:0, round:1, action:"attack"}  │
  │    roll = 8                                                 │
  │    attack = 8 + 1 = 9                                       │
  │    9 < 14 (Pet AC) → MISS                                   │
  └─────────────────────────────────────────────────────────────┘

RONDA 2:
  ┌─────────────────────────────────────────────────────────────┐
  │  Pet ataca (STR):                                           │
  │    context = {stat:STR, enemy:5, round:2, action:"attack"}  │
  │    ← Contexto diferente (round=2)                           │
  │    hash = SHA256(dna.hash + "A3fK9x" + 5 + 2 + "attack")    │
  │    roll = 9                                                 │
  │    attack = 9 + 2 = 11                                      │
  │    11 < 12 (AC) → MISS                                      │
  │                                                             │
  │  Goblin ataca:                                              │
  │    context = {stat:STR, enemy:0, round:2, action:"attack"}  │
  │    roll = 18                                                │
  │    attack = 18 + 1 = 19                                     │
  │    19 >= 14 (Pet AC) → HIT                                  │
  │    damage = 1d6 + 1 = 4                                     │
  └─────────────────────────────────────────────────────────────┘
```

---

## 7. Consideraciones de Hardware

### 7.1 Uso de Memoria

```
┌─────────────────────────────────────────────────────────────────────┐
│                    FOOTPRINT DE MEMORIA                             │
└─────────────────────────────────────────────────────────────────────┘

ESTRUCTURA dna_t:
  ┌───────────────────────────────────────────────────────────────┐
  │  codes[6][7]:      42 bytes                                   │
  │  hash[32]:         32 bytes                                   │
  │  salt:             4 bytes                                    │
  │  base_stats[6]:    6 bytes                                    │
  │  caps[6]:          6 bytes                                    │
  │  ─────────────────────────────────                           │
  │  TOTAL:            90 bytes                                   │
  └───────────────────────────────────────────────────────────────┘

ESTRUCTURA roll_context_t:
  ┌───────────────────────────────────────────────────────────────┐
  │  dna*:             4 bytes (puntero)                          │
  │  stat_idx:         1 byte                                     │
  │  enemy_id:         1 byte                                     │
  │  round:            1 byte                                     │
  │  action*:          4 bytes (puntero)                          │
  │  ─────────────────────────────────                           │
  │  TOTAL:            11 bytes                                   │
  └───────────────────────────────────────────────────────────────┘

STACK PARA SHA-256:
  ┌───────────────────────────────────────────────────────────────┐
  │  mbedtls_sha256_context: ~300 bytes                           │
  │  Buffer de salida: 32 bytes                                   │
  │  Buffer de entrada: ~100 bytes (máximo)                       │
  │  ─────────────────────────────────                           │
  │  TOTAL:            ~432 bytes en stack                        │
  │                                                               │
  │  Recomendación: Asignar en PSRAM si es posible               │
  └───────────────────────────────────────────────────────────────┘
```

### 7.2 Performance

```
┌─────────────────────────────────────────────────────────────────────┐
│                    TIEMPOS DE EJECUCIÓN                             │
│                    (ESP32-S3 @ 240MHz)                              │
└─────────────────────────────────────────────────────────────────────┘

OPERACIÓN                          TIEMPO      NOTAS
───────────────────────────────────────────────────────────────────────
SHA-256 (hardware, 64 bytes)       ~5 μs       GDMA acelerado
SHA-256 (software, 64 bytes)       ~30 μs      Fallback si HW no disponible
Extracción de 4d6                  ~1 μs       Operaciones simples
Level-up (6 stats)                 ~35 μs      6 evaluaciones + selección

IMPACTO EN COMBATE:
  - Tirada d20: ~6 μs por tirada
  - Ronda completa: ~20 μs (2 tiradas + daño)
  - Frame de 33ms (30 FPS): despreciable

CONCLUSÍON:
  El overhead es imperceptible para el gameplay.
  No afecta la fluidez del juego.
```

---

## 8. Extensiones Futuras

### 8.1 Complejidad Adicional

```
┌─────────────────────────────────────────────────────────────────────┐
│                    CAPAS DE COMPLEJIDAD FUTURAS                     │
└─────────────────────────────────────────────────────────────────────┘

CAPA 1: ACTUAL
  ┌───────────────────────────────────────────────────────────────┐
  │  • Hash simple para stats                                     │
  │  • Sal por vida                                               │
  │  • CAPs genéticos                                             │
  └───────────────────────────────────────────────────────────────┘

CAPA 2: AFINIDAD ELEMENTAL (Futuro)
  ┌───────────────────────────────────────────────────────────────┐
  │  • Derivar elemento del Pet desde hash                        │
  │  • Fuego, Agua, Tierra, Aire, etc.                            │
  │  • Bonificaciones contra enemigos de elemento opuesto         │
  │  • Penalizaciones contra elemento igual                       │
  └───────────────────────────────────────────────────────────────┘

CAPA 3: MUTACIONES (Futuro)
  ┌───────────────────────────────────────────────────────────────┐
  │  • Eventos raros que modifican el ADN                         │
  │  • Ejemplo: "Exposición a radiación" → +1 a CAP de CON        │
  │  • Guardar mutaciones en state.json                           │
  └───────────────────────────────────────────────────────────────┘

CAPA 4: HERENCIA (Futuro)
  ┌───────────────────────────────────────────────────────────────┐
  │  • Cruce de dos ADNs para crear nuevo Pet                     │
  │  • Combinar códigos de padres                                 │
  │  • Posibilidad de mutaciones heredadas                        │
  └───────────────────────────────────────────────────────────────┘

CAPA 5: MEMORIA FRACTAL (Futuro)
  ┌───────────────────────────────────────────────────────────────┐
  │  • Usar hash del ADN como dirección en memoria fractal        │
  │  • /MEM/[hash_hex]/experiences.json                           │
  │  • Experiencias pasadas influyen en tiradas                   │
  └───────────────────────────────────────────────────────────────┘
```

### 8.2 Posibles Mejoras

```
┌─────────────────────────────────────────────────────────────────────┐
│                    IDEAS PARA FUTURO                                │
└─────────────────────────────────────────────────────────────────────┘

1. VALIDACIÓN DE ADN
   - Agregar checksum en JSON
   - Verificar integridad al cargar
   - Prevenir ADNs corruptos

2. ADN RARO / LEGENDARIO
   - Códigos especiales con patrones únicos
   - Ejemplo: "AAAAAA" → Pet legendario
   - Distribución limitada

3. DEBUG MODE
   - Modo desarrollador que muestra sal actual
   - Permitir reproducir combate exacto
   - Útil para testing

4. SIMULADOR EXTERNO
   - Herramienta web para previsualizar stats
   - Input: códigos ADN
   - Output: stats probables, CAPs, distribución

5. ESTADÍSTICAS DE VIDA
   - Guardar historia de stats por vida
   - Mostrar en UI: "Vida anterior: STR=18"
   - Comparar progreso entre vidas
```

---

## 9. Resumen

El Sistema de ADN Criptográfico permite:

| Característica | Beneficio |
|----------------|-----------|
| **Stats deterministas** | Mismo ADN + misma sal = mismos stats |
| **Variabilidad** | Cada renacimiento es único |
| **CAPs genéticos** | Limita el progreso, crea identidad |
| **Level-ups estratégicos** | Máximo 2 stats por nivel |
| **Combate determinista** | Reproducible, justo, predecible |
| **Extensible** | Fácil agregar complejidad futura |

El sistema equilibra **determinismo** (para debugging y justicia) con **variabilidad** (para replay value), usando hashing criptográfico como base matemática.
