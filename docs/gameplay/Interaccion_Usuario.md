# **Interacción Usuario-Pet: El Canal Divino**

Este documento define en detalle cómo interactúa el usuario con el Pet a través de los sensores, qué efectos tienen cada tipo de interacción, y los límites del sistema.

---

## **1. Filosofía de Interacción**

La interacción usuario-Pet sigue los principios del **sistema idle**:

* **No es obligatoria:** El Pet vive autónomamente sin interacción
* **Es bonus:** Toda interacción otorga ventajas o mejoras
* **Es narrativa:** Cada interacción tiene impacto en la historia
* **Es física:** No hay menús tradicionales, todo es tangible

---

## **2. Interacción por IMU (Movimiento)**

### **A. Sacudir (Shake)**

**Detección:** Agitar el dispositivo con fuerza suficiente

| Aspecto | Detalle |
|---------|---------|
| **Bonus** | + Energía instantánea |
| **Estado** | Activa "Vigor" por 3-5 Ticks |
| **Efecto narrativo** | El Pet recibe un "impulso divino" |
| **Efecto mecánico** | Mejora tiradas físicas (STR) durante Vigor |

**Implementación:**
* Umbral de aceleración para detectar sacudida
* Cooldown entre sacudidas para evitar spam
* Animación del Pet reacciona al impulso

### **B. Caminar (Movimiento Rítmico)**

**Detección:** Desplazamiento físico continuo con el dispositivo

| Aspecto | Detalle |
|---------|---------|
| **Bonus** | Acelera exploración del mapa |
| **Efecto narrativo** | El Pet avanza más rápido en su mundo |
| **Efecto mecánico** | Multiplica velocidad de avance por Tick |

**Implementación:**
* Contador de pasos basado en IMU
* Cada X pasos = 1 "avance de exploración"
* El Pet puede explorar sin caminar, pero más lento

### **C. Rotación**

**Detección:** Giro brusco o prolongado del dispositivo

| Aspecto | Detalle |
|---------|---------|
| **Efecto** | Estado "Mareo" (penalización temporal) |
| **Duración** | 2-4 Ticks |
| **Efecto narrativo** | El Pet se desorienta |

**Implementación:**
* Girómetro detecta rotación excesiva
* Penalización a DEX durante Mareo
* El usuario puede evitar esto moviendo el dispositivo suavemente

---

## **3. Interacción por Touch (Pantalla Táctil)**

### **A. Tap Directo (Caricia)**

**Detección:** Toque en el área del Pet en pantalla

| Aspecto | Detalle |
|---------|---------|
| **Bonus** | + Social (recarga humor) |
| **Efecto narrativo** | El Pet muestra afecto, mejora ánimo |
| **Estado** | Puede activar "Feliz" si Social estaba bajo |
| **Cooldown** | Mínimo entre caricias para evitar spam |

**Implementación:**
* Zona táctil definida sobre el sprite del Pet
* Animación de reacción del Pet
* Incremento del vector Social

### **B. Respuesta a Peticiones**

**Contexto:** El Pet hace una petición (recursos, atención, decisión)

**Flujo:**
1. Pet solicita algo → UI muestra opciones "Sí/No"
2. Usuario tap en "Sí" → Pet obtiene lo que pidió
3. Usuario tap en "No" → Pet no obtiene nada, puede reaccionar negativamente

| Respuesta | Efecto en Pet |
|-----------|---------------|
| **Sí** | Mejora lealtad, +Social, narrativa positiva |
| **No** | Reduce lealtad, -Social, narrativa de rechazo |
| **Ignorar** | Pet toma decisión autónoma |

### **C. Toma de Decisiones (Dilemas)**

**Contexto:** El Pet solicita "Visión Divina" ante una situación compleja

**Flujo:**
1. Evento de dilema → Pet pide guía
2. UI muestra 2-3 opciones de camino
3. Usuario tap en una opción
4. El curso de la historia se altera según la elección

**Ejemplo:**
```
El Pet encuentra un camino bifurcado:
[║] Ir por el bosque oscuro (arriesgado)
[║] Seguir el río (seguro)
[║] Volver al campamento (conservador)
```

### **D. Bendiciones (Manifestaciones Aleatorias)**

**Contexto:** Aparecen elementos flotantes durante la exploración

**Flujo:**
1. Elemento flotante aparece en pantalla (objeto, anomalía, buff)
2. Usuario tiene tiempo limitado para tap
3. Si tap a tiempo → Bendición aplicada
4. Si no tap → Elemento desaparece sin efecto

| Tipo de Bendición | Efecto |
|-------------------|--------|
| **Buff temporal** | Mejora tiradas por X Ticks |
| **Recurso extra** | + Energía/Social instantáneo |
| **Anomalía** | Evento narrativo especial |
| **Rareza** | Desbloquea algo único |

### **E. Navegación (Swipe)**

**Detección:** Deslizamiento horizontal en pantalla

| Swipe | Pantalla destino |
|-------|------------------|
| **Izquierda** | Pantalla anterior |
| **Derecha** | Pantalla siguiente |

**Pantallas navegables:**
1. TERMINAL_LIFE (Pet + barras)
2. CHRONICLE_LOG (Historia)
3. GENETIC_MATRIX (ADN/Stats)
4. DIVINE_VISION (Cámara)

---

## **4. Interacción por Cámara (Ofrendas)**

### **A. Ofrenda Cromática**

**Contexto:** El Pet solicita un color específico del mundo real

**Flujo:**
1. Pet solicita: "Necesito ver el azul..."
2. Usuario abre DIVINE_VISION (cámara)
3. Apunta la cámara a algo con ese color
4. Sistema detecta color dominante
5. Si coincide → Ofrenda exitosa

| Resultado | Bonus |
|-----------|-------|
| **Color exacto** | +++ Energía, ++ Social, narrativa épica |
| **Color similar** | ++ Energía, + Social |
| **Color incorrecto** | Sin bonus, Pet decepcionado |

### **B. Detección de Presencia**

**Contexto:** El usuario mira a través de la cámara

| Aspecto | Detalle |
|---------|---------|
| **Efecto** | Mitiga soledad del Pet |
| **Bonus** | + Social gradual mientras observa |
| **Narrativa** | El Pet siente la "mirada divina" |

---

## **5. Interacción por Botón Físico**

### **A. Wake-up (Despertar)**

**Contexto:** El dispositivo está en Deep Sleep

| Acción | Efecto |
|--------|--------|
| **Presión corta** | Despierta el sistema, inicia nuevo Tick |
| **Durante uso normal** | Sin efecto (o acción configurada) |

### **B. Hard Reset (Apocalipsis)**

**Contexto:** Durante el boot inicial

| Acción | Efecto |
|--------|--------|
| **Presión prolongada (>5s)** | Borra SD completamente, inicia nuevo Génesis |

**Advertencia:** Esta acción es irreversible. Borra mundo, Pet, todo.

---

## **6. Límites de Interacción**

### **A. Cooldowns**
| Interacción | Cooldown |
|-------------|----------|
| Sacudir | 3-5 segundos |
| Caricia (tap) | 2-3 segundos |
| Ofrenda cromática | 1 por solicitud del Pet |
| Bendición flotante | Tiempo limitado en pantalla |

### **B. No Spam**
El sistema detecta y penaliza:
* Sacudidas demasiado frecuentes → Ignora, no da bonus
* Taps repetitivos → Ignora después del primero
* Movimiento excesivo → No acumula bonus extra

### **C. Sin Microtransacciones**
* No hay forma de "comprar" bonus
* Todo se obtiene por interacción física
* Los DP son la única moneda meta, solo para rituales/ADN

---

## **7. Resumen de Interacciones**

```
┌─────────────────────────────────────────────────────────────┐
│                    INTERACCIONES                            │
├─────────────────────────────────────────────────────────────┤
│  IMU (Movimiento)                                           │
│  ├── Sacudir ──► +Energía, Estado Vigor                    │
│  ├── Caminar ──► Acelera exploración                        │
│  └── Rotación ─► Estado Mareo (penalización)               │
├─────────────────────────────────────────────────────────────┤
│  TOUCH (Pantalla)                                           │
│  ├── Tap (caricia) ──► +Social                             │
│  ├── Petición Sí/No ──► Influye lealtad                    │
│  ├── Dilema (opciones) ──► Altera historia                 │
│  ├── Bendición flotante ──► Bonus/Evento                   │
│  └── Swipe ──► Navegación UI                               │
├─────────────────────────────────────────────────────────────┤
│  CÁMARA (Ofrendas)                                          │
│  ├── Ofrenda cromática ──► +++Energía/Social              │
│  └── Detección presencia ──► +Social gradual              │
├─────────────────────────────────────────────────────────────┤
│  BOTÓN FÍSICO                                               │
│  ├── Wake-up ──► Despertar sistema                         │
│  └── Hard Reset ──► Apocalipsis (borra todo)              │
└─────────────────────────────────────────────────────────────┘

NOTA: Todas las interacciones son BONUS, no obligatorias.
El Pet vive autónomamente en sistema idle.
```
