# **Estados Emocionales del Pet**

Este documento define los estados emocionales y físicos que puede experimentar el Pet, qué los triggea, cómo afectan su comportamiento y cómo se visualizan en el HUD.

---

## **1. Concepto de Estados**

Los estados son **condiciones temporales** que modifican el comportamiento del Pet. No son permanentes y pueden cambiar según:
* Interacción del usuario
* Eventos del mundo
* Niveles del Vector de Estado
* Resultados de tiradas d20

---

## **2. Clasificación de Estados**

### **A. Estados Reactivos (Capa Reactiva)**
Generados por sensores físicos, de respuesta inmediata.

| Estado | Trigger | Duración | Efecto |
|--------|---------|----------|--------|
| **Vigor** | Sacudir dispositivo (IMU) | 3-5 Ticks | + Energía, animación activa |
| **Susto** | Giro brusco o sacudida fuerte | 1-2 Ticks | Pet se inmoviliza, animación de sobresalto |
| **Mareo** | Rotación prolongada | 2-4 Ticks | Penalización a DEX, animación tambaleante |
| **Despertar** | Tap en pantalla (estaba dormido) | Instantáneo | Sale de estado de sueño |

### **B. Estados de Necesidad (Capa de Instintos)**
Generados por los niveles del Vector de Estado.

| Estado | Condición | Efecto en Comportamiento |
|--------|-----------|-------------------------|
| **Hambriento** | Hambre < 25% | Prioriza buscar comida sobre otras acciones |
| **Agotado** | Energía < 25% | Reduce actividad, entra en Letargo si persiste |
| **Solitario** | Social < 25% | Busca interacción, puede hacer peticiones al usuario |
| **Letargo** | Energía casi agotada | Inactivo, narrativa pausada, espera interacción |
| **Crítico** | HP < 30% | Comportamiento de supervivencia, puede huir |

### **C. Estados de Ánimo (Capa Consciente)**
Generados por la narrativa y eventos del mundo.

| Estado | Trigger | Efecto Narrativo |
|--------|---------|------------------|
| **Feliz** | Éxito en eventos, caricias | Tono narrativo positivo, más social |
| **Triste** | Pérdidas, fallos repetidos | Tono melancólico, menos proactivo |
| **Asustado** | Eventos peligrosos | Evita riesgos, busca refugio |
| **Curioso** | Nuevo bioma, descubrimiento | Exploración activa, más decisiones |
| **Determinado** | Level Up, ascensión | Aumenta bravura, toma más riesgos |

### **D. Estados Especiales**
Generados por eventos únicos o rituales.

| Estado | Trigger | Efecto |
|--------|---------|--------|
| **Bendecido** | Ofrenda cromática exitosa | Bonificación temporal a todas las tiradas |
| **Ascendido** | Post-Ritual exitoso | Nuevas habilidades disponibles |
| **Enfermo** | Evento narrativo negativo | Penalización a CON,consume más Hambre |
| **Enamorado** | Evento narrativo especial | Busca a una entidad específica |

---

## **3. Sistema de Prioridad de Estados**

El Pet puede tener múltiples estados activos simultáneamente, pero con prioridad:

```
1. Crítico (HP bajo) ← Máxima prioridad
2. Letargo (Sin Energía)
3. Estados de Necesidad (Hambre, Solitario)
4. Estados Reactivos (Vigor, Susto)
5. Estados de Ánimo (Feliz, Triste)
```

### **Ejemplo de Resolución**
Si el Pet está **Hambriento** + **Curioso** + **Vigor**:
1. Vigor da +Energía
2. Hambriento prioriza buscar comida
3. Curioso hace que explore mientras busca
4. Resultado: "Explora activamente buscando alimento"

---

## **4. Visualización en HUD**

### **A. Indicadores Visuales**
Cada estado tiene representación visual:

| Estado | Animación | Color | Efecto |
|--------|-----------|-------|--------|
| Vigor | Saltos, brillo | Cian brillante | Parpadeo rápido |
| Hambriento | Mirando al vacío | Naranja | Temblor leve |
| Agotado | Ojos cerrados, lento | Gris apagado | Movimiento reducido |
| Letargo | Inmóvil, respiración lenta | Gris oscuro | Sin movimiento |
| Feliz | Saltos, sonrisa | Verde neón | Movimiento fluido |
| Triste | Cabeza baja, lento | Azul oscuro | Sin saltos |
| Asustado | Ojos grandes, retrocede | Rojo parpadeante | Movimiento errático |
| Crítico | Parpadeo de alerta | Rojo neón fuerte | Glitch visual |

### **B. Barra de Estado**
En la cabecera del HUD se muestra el estado dominante actual con un ícono ASCII:

```
[VIGOR]  DP: 150  ████████░░  BATT: 85%
```

### **C. Efectos de Glitch**
Estados críticos generan efectos visuales de "glitch":
* Caracteres aleatorios en la UI
* Parpadeo del frame del Pet
* Colores que cambian bruscamente

---

## **5. Transiciones entre Estados**

### **A. Transiciones Automáticas**
| Desde | Hasta | Condición |
|-------|-------|-----------|
| Vigor | Normal | Pasan 3-5 Ticks |
| Hambriento | Normal | Come algo |
| Agotado | Letargo | Energía llega a mínimo |
| Letargo | Normal | Usuario recarga Energía |
| Crítico | Normal | HP sube de 30% |

### **B. Transiciones por Evento**
| Desde | Hasta | Evento |
|-------|-------|--------|
| Cualquiera | Susto | Giro brusco |
| Normal | Vigor | Sacudida |
| Cualquiera | Feliz | Caricia del usuario |
| Triste | Determinado | Level Up |
| Curioso | Asustado | Encuentro peligroso |

---

## **6. Impacto en la Narrativa**

La IA recibe el estado actual del Pet como contexto:

### **Ejemplo de Prompt Enriquecido**
```
Pet actual:
- Nombre: Aldric
- Estado dominante: Hambriento + Curioso
- Vector de Estado: Hambre 15%, Energía 60%, Social 40%

Contexto para narrativa:
Aldric está hambriento y curioso. Sus decisiones estarán 
influenciadas por la necesidad de encontrar alimento, 
pero su curiosidad lo llevará a explorar mientras busca.
```

### **Tono Narrativo por Estado**
| Estado | Tono Ejemplo |
|--------|--------------|
| Feliz | "Con paso ligero, [Nombre] avanza..." |
| Triste | "Con la mirada baja, [Nombre]..." |
| Asustado | "Los ojos de [Nombre] se abren..." |
| Vigor | "Un estallido de energía recorre a [Nombre]..." |
| Crítico | "[Nombre] siente cómo la vida se escapa..." |

---

## **7. Peticiones del Pet según Estado**

El Pet puede hacer peticiones al usuario según su estado:

| Estado | Petición Posible |
|--------|-----------------|
| Hambriento | "Necesito alimento..." |
| Solitario | "¿Jugamos?" |
| Agotado | "Estoy cansado..." |
| Curioso | "¡Mira eso! ¿Vamos?" |
| Crítico | "No me siento bien..." |

El usuario puede responder:
* **Otorgar:** Tap en "Sí" → Pet mejora su estado
* **Denegar:** Tap en "No" → Pet mantiene estado, puede empeorar
* **Ignorar:** No responde → Pet toma decisión autónoma

---

## **8. Resumen de Estados**

```
┌─────────────────────────────────────────────────────────────┐
│                    ESTADOS DEL PET                          │
├─────────────────────────────────────────────────────────────┤
│  REACTIVOS        │  NECESIDAD      │  ÁNIMO      │ ESP.   │
│  (Sensores)       │  (Vector)       │  (Narrativa)│        │
├───────────────────┼─────────────────┼─────────────┼────────┤
│  • Vigor          │  • Hambriento   │  • Feliz    │ • Ben- │
│  • Susto          │  • Agotado      │  • Triste   │   decido│
│  • Mareo          │  • Solitario    │  • Asustado │ • As-  │
│  • Despertar      │  • Letargo      │  • Curioso  │   cendido│
│                   │  • Crítico      │  • Determ.  │        │
└───────────────────┴─────────────────┴─────────────┴────────┘
                            │
                            ▼
                    VISUALIZACIÓN HUD
                    (Animación + Color)
```
