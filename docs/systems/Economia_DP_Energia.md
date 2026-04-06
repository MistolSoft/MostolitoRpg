# **Economía de Energía y Puntos de Deidad**

Este documento define los dos sistemas de recursos del juego: la Energía (recurso vital del Pet) y los Puntos de Deidad (recurso acumulativo del usuario).

---

## **1. El Sistema Idle: Autonomía del Pet**

MistolitoRPG es un **sistema idle**. El Pet vive de forma autónoma sin requerir interacción constante del usuario.

### **A. Vida Autónoma**
* El Pet consume y recupera Energía de forma natural
* El Vector de Estado `[Hambre, Energía, Social]` se gestiona automáticamente
* El Pet toma decisiones, explora y narra su historia de forma independiente
* Sin intervención, el Pet continúa su vida normal

### **B. El Vector de Estado**
Los tres componentes del vector se comportan de forma idle:

| Componente | Comportamiento Idle |
|------------|---------------------|
| **Hambre** | Se consume por Tick, se recupera al comer (automático) |
| **Energía** | Se consume por acción, se recupera al descansar (automático) |
| **Social** | Se consume por Tick, se recupera con interacciones (automático o por usuario) |

---

## **2. La Energía: Ciclo Natural**

### **A. ¿Qué es la Energía?**
* Representa la vitalidad física del Pet
* Se consume con acciones y se recupera al descansar
* Es **independiente del HP** (HP es vida, Energía es capacidad de actuar)

### **B. Diferencia con HP**
| Concepto | Energía | HP |
|----------|---------|-----|
| **Representa** | Capacidad de actuar | Vida total |
| **Al llegar a 0** | Pet descansa automáticamente | Muerte |
| **Recuperación** | Descanso automático | Alimentos, descanso |
| **Efecto narrativo** | Pet se cansa | Pet muere |

### **C. Ciclo de Energía Idle**
```
ACCIONAR → CONSUMO → ENERGÍA BAJA → DESCANSO AUTOMÁTICO → RECUPERACIÓN → ACCIONAR
```

El Pet decide cuándo descansar basándose en su Vector de Estado y el motor de decisiones.

---

## **3. La Intervención del Usuario (Bonus)**

La interacción NO es obligatoria, pero otorga **ventajas y bonificaciones**:

### **A. Bonus por IMU (Movimiento)**

| Interacción | Bonus | Efecto |
|-------------|-------|--------|
| **Sacudir** | + Energía instantánea | Estado "Vigor" temporal, mejora tiradas físicas |
| **Caminar** | Acelera exploración | El Pet avanza más rápido en el mapa |

### **B. Bonus por Touch**

| Interacción | Bonus | Efecto |
|-------------|-------|--------|
| **Tap (caricia)** | + Social | Mejora ánimo, efecto narrativo positivo |
| **Swipe** | Navegación UI | Sin efecto en gameplay |
| **Responder peticiones** | Variable | Influye en lealtad y decisiones del Pet |

### **C. Bonus por Cámara (Ofrendas)**

| Interacción | Bonus | Efecto |
|-------------|-------|--------|
| **Ofrenda cromática exitosa** | ++ Energía + Social | Gran bonificación, mejora narrativa |

### **D. Sin Bonus Obligatorio**
* El Pet puede vivir completamente sin interacción
* Sin bonus, el Pet sigue su curso natural idle
* La interacción enriquece la experiencia pero no es requerida

---

## **4. Puntos de Deidad (DP): Recurso del Usuario**

Los Puntos de Deidad son un recurso meta-juego que **persiste entre vidas** y pertenece al usuario, no al Pet.

### **A. ¿Qué son los DP?**
* Puntos acumulados por interacción física
* Representan la "gracia divina" del usuario
* **Se conservan cuando el Pet muere**
* Solo se usan para **Rituales de Ascensión** y **modificación de ADN al renacer**

### **B. Adquisición de DP**
| Fuente | DP Ganados |
|--------|------------|
| Interacción física (IMU/Touch) | Acumulación gradual |
| Modo Limbo (offline) | Mayor acumulación (sistema compensatorio) |
| Ofrendas Cromáticas | Bonus adicional |
| Supervivencia prolongada | Bonus por longevidad del Pet |

### **C. Uso de DP**
Los DP tienen **dos usos exclusivos**:

#### **1. Ritual de Ascensión**
* Costo variable según el nivel del Pet
* Solo uso durante la vida del Pet
* No se recupera si el ritual falla

#### **2. Modificación de ADN al Renacer**
* Se usa ANTES de incubar un nuevo Pet
* Permite mejorar stats base del nuevo Pet
* Véase "Sistema de Vida y Muerte" para detalles

---

## **5. Diferencia Clave: Energía vs DP**

| Aspecto | Energía | DP |
|---------|---------|-----|
| **Pertenece a** | El Pet | El usuario |
| **Persistencia** | Se reinicia al morir | Se conserva entre vidas |
| **Función** | Permitir acciones del Pet | Ascensión y mejoras de ADN |
| **Recarga** | Automática (idle) + bonus por interacción | Acumulación por interacción |
| **Gasto** | Automático (consumo) | Voluntario (rituales/modificaciones) |

---

## **6. Economía en Modo Limbo (Offline)**

Cuando el dispositivo está sin conexión a internet:

### **Energía y Vida Idle**
* Funciona normalmente (capas reactiva e instintiva operativas)
* El Pet continúa viviendo y consumiendo recursos
* La narrativa se pausa, pero el ciclo idle continúa

### **DP**
* Se acumulan con mayor tasa (sistema compensatorio)
* La interacción física sigue contando
* Al reconectar, los DP acumulados están disponibles para rituales

---

## **7. Balance y Consideraciones**

### **Reglas de Diseño**
1. **Idle primero:** El Pet vive autónomamente sin requerir interacción
2. **Bonus, no obligación:** La interacción mejora la experiencia, no la hace posible
3. **DP como legado:** Los DP crean continuidad entre vidas
4. **Consecuencias reales:** El abandono prolongado puede causar muerte por negligencia

### **Variables por ADN**
El ADN puede modificar:
* Delta de consumo de Energía (metabolismo)
* Eficiencia de recuperación al descansar
* Capacidad máxima de Energía
* Bonificaciones específicas

---

## **8. Resumen Visual**

```
┌─────────────────────────────────────────────────────────────┐
│                     SISTEMA IDLE                            │
│                                                             │
│   ┌─────────────┐      ┌─────────────┐                     │
│   │   ACCIÓN    │ ───► │  CONSUMO    │                     │
│   └─────────────┘      └─────────────┘                     │
│                              │                              │
│                              ▼                              │
│   ┌─────────────┐      ┌─────────────┐                     │
│   │  DESCANSO   │ ◄─── │ ENERGÍA BAJA│                     │
│   └─────────────┘      └─────────────┘                     │
│        │                                                    │
│        ▼                                                    │
│   ┌─────────────┐                                          │
│   │ RECUPERACIÓN│ ───────────────────────────────► ACCIÓN  │
│   └─────────────┘                                          │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│                   INTERVENCIÓN (BONUS)                      │
│                                                             │
│   SACUDIR ──► + Energía instantánea + Estado Vigor          │
│   CAMINAR ──► Acelera exploración                           │
│   TAP ──────► + Social, mejora ánimo                        │
│   OFRENDA ──► ++ Energía + Social + Mejora narrativa        │
│                                                             │
│   (Todo es BONUS, no obligatorio)                          │
├─────────────────────────────────────────────────────────────┤
│                      DP (persisten)                         │
│                                                             │
│   Acumulación por interacción ──► Ascensión / Modificar ADN │
└─────────────────────────────────────────────────────────────┘
```
