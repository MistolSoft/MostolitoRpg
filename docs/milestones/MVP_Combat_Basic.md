# MVP: Combate Básico

**Objetivo:** Crear el MVP más simple posible que valide el ciclo core del juego.

**Alcance:**
- UI de pantalla del Pet
- Sistema de ADN (upload, parsing, generación de Pet)
- Combate idle contra 1 enemigo
- Sistema de niveles y stats
- Profesiones y habilidades según nivel
- Muerte y renacimiento

**Fuera de alcance:**
- Interacción del usuario (IMU, touch, cámara)
- Mundo procedural
- Memoria fractal
- Bridge/LLM
- Múltiples enemigos o biomas

---

## M1: UI Base

### M1.1: Display LVGL funcional
- [ ] Configurar LVGL para ESP32-S3
- [ ] Inicializar LCD ST7789T3
- [ ] Renderizar un "Hello World" en pantalla

### M1.2: Layout del Pet
- [ ] Diseñar pantalla principal (240x320)
- [ ] Área del sprite del Pet
- [ ] Barra de HP
- [ ] Barra de EXP/Nivel
- [ ] Nombre del Pet

### M1.3: Sprites básicos
- [ ] Sprite idle del Pet (1 frame)
- [ ] Sprite de daño
- [ ] Sprite de muerte
- [ ] Sprite de victoria/level up

---

## M2: Sistema de ADN

### M2.1: Formato de ADN
- [ ] Definir estructura JSON del ADN
- [ ] Atributos base (STR, DEX, CON, INT, WIS, CHA)
- [ ] Deltas metabólicos
- [ ] Soft-caps genéticos

### M2.2: Upload de ADN
- [ ] Cargar ADN desde archivo en SD
- [ ] Parsear JSON
- [ ] Validar estructura

### M2.3: Generación del Pet
- [ ] Crear instancia de Pet desde ADN
- [ ] Calcular HP base
- [ ] Calcular modificadores de atributos
- [ ] Asignar nombre (hardcodeado o aleatorio)

---

## M3: Combate Idle

### M3.1: Enemigo único
- [ ] Definir stats del enemigo base
- [ ] HP, atributos, daño
- [ ] Nombre y sprite placeholder

### M3.2: Motor de combate
- [ ] Turnos automáticos (tick-based)
- [ ] Tirada d20 para ataque
- [ ] Cálculo de daño (atributo + modificador)
- [ ] Aplicación de daño a HP

### M3.3: Loop de combate
- [ ] Pet ataca → Enemigo ataca → Repetir
- [ ] Detectar victoria (HP enemigo = 0)
- [ ] Detectar derrota (HP Pet = 0)
- [ ] Mostrar resultado en UI

### M3.4: Recompensas
- [ ] Ganancia de EXP por victoria
- [ ] Distribución de EXP (sin loot)

---

## M4: Sistema de Niveles

### M4.1: Experiencia y niveles
- [ ] Curva de EXP por nivel
- [ ] Acumulación de EXP
- [ ] Detección de level up

### M4.2: Subida de stats
- [ ] Incremento de stats al subir nivel
- [ ] Probabilidad basada en ADN (afinidad genética)
- [ ] Actualizar modificadores

### M4.3: HP máximo
- [ ] Aumentar HP máximo por nivel
- [ ] Recalcular HP actual
- [ ] Actualizar UI

---

## M5: Profesiones y Habilidades

### M5.1: Tabla de profesiones
- [ ] Definir profesiones base (Guerrero, Mago, Pícaro, etc.)
- [ ] Requisitos de atributos
- [ ] Bonus de profesión

### M5.2: Desbloqueo por nivel
- [ ] Profesiones disponibles desde nivel X
- [ ] Asignación automática según stats
- [ ] Mostrar profesión en UI

### M5.3: Habilidades por nivel
- [ ] Definir habilidades base por profesión
- [ ] Desbloqueo automático al alcanzar nivel
- [ ] Efecto de habilidad en combate (+bonus)
- [ ] Mostrar habilidad activa en UI

---

## M6: Muerte y Renacimiento

### M6.1: Muerte del Pet
- [ ] Detectar HP = 0
- [ ] Animación de muerte
- [ ] Pantalla de "Game Over"

### M6.2: Renacimiento
- [ ] Opción de renacer con mismo ADN
- [ ] Reset de nivel, stats, HP
- [ ] Mantener contador de vidas
- [ ] Volver a M2 (generación de Pet)

### M6.3: Persistencia de DP (simplificado)
- [ ] Acumular DP por combate
- [ ] Mostrar DP en UI
- [ ] DP persisten entre vidas

---

## M7: Integración Final MVP

### M7.1: Flujo completo
- [ ] Boot → Cargar ADN → Generar Pet
- [ ] Pet idle combate → Level up → Habilidades
- [ ] Muerte → Renacimiento → Repetir

### M7.2: UI Polish
- [ ] Refinar visualización de stats
- [ ] Animaciones básicas
- [ ] Feedback visual de combate

### M7.3: Testing en hardware
- [ ] Probar en ESP32-S3 real
- [ ] Verificar memoria (PSRAM)
- [ ] Validar performance

---

## Dependencias entre Milestones

```
M1 (UI Base)
 │
 ├─► M2 (ADN) ──► M3 (Combate) ──► M4 (Niveles)
 │                                      │
 │                                      ▼
 │                               M5 (Profesiones)
 │                                      │
 │                                      ▼
 └──────────────────────────────► M6 (Muerte)
                                        │
                                        ▼
                                   M7 (Integración)
```

---

## Notas de Implementación

- **Sin interacción:** El usuario solo observa, no puede influir
- **Sin LLM:** Todo hardcodeado o procedural simple
- **Sin mundo:** Solo combate 1v1 infinito
- **Objetivo:** Validar que el ciclo core funciona en hardware
