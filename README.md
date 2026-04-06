# MistolitoRPG

<p align="center">
  <strong>Un simulador de vida fantástica para ESP32-S3</strong>
</p>

<p align="center">
  <em>Combina mecánicas de mascota virtual con sistemas de rol de mesa (SRD 5.1)</em>
</p>

---

## Sobre el proyecto

**MistolitoRPG** es un juego idle open-source que crea una experiencia de juego autónoma y persistente en el ESP32-S3 Waveshare con LCD. Tu personaje vive, combate, gana experiencia y puede morir, iniciando un nuevo ciclo.

### Concepto

En MistolitoRPG, el usuario asume dos roles fundamentales:

- **El Arquitecto:** Antes de que exista la vida, defines las leyes del universo a través del Ritual de Génesis
- **La Guía Divina:** Una vez que el Pet nace, interactúas físicamente con el dispositivo para proveerle energía

Cada Mistolito nace de un **ADN único** que define sus atributos base (Fuerza, Destreza, Constitución, Inteligencia, Sabiduría, Carisma) según el sistema SRD 5.1.

---

## Características

- **Mascota virtual con ciclo de vida completo** - Nace, crece, combate y muere
- **Combate basado en d20** - Sistema SRD 5.1 con tiradas de dados
- **Sistema de niveles y progresión** - Sube de nivel, mejora stats, desbloquea profesiones
- **Generación procedural** - Encuentros y enemigos dinámicos
- **Sistema de energía** - Balance entre exploración, combate y descanso
- **Almacenamiento persistente** - Guardado en tarjeta SD
- **Interfaz LVGL 9** - Animaciones fluidas y HUD responsivo

---

## Hardware compatible

| Componente | Especificación |
|------------|----------------|
| MCU | ESP32-S3 (Dual-Core Xtensa LX7, 240MHz) |
| Memoria | 512KB SRAM + 8MB PSRAM + 16MB Flash |
| Display | ST7789 LCD 240x320 (SPI) |
| Almacenamiento | MicroSD (SPI compartido) |
| Sensores | IMU QMI8658 + Touch CST816D (I2C) |

**Placa recomendada:** Waveshare ESP32-S3-LCD-2

---

## Arquitectura

```
┌─────────────────────────────────────────────┐
│            FreeRTOS Scheduler                │
├─────────────────────────────────────────────┤
│                                             │
│  ┌──────────────┐  Prioridad 5              │
│  │ display_task │  ← LVGL refresh           │
│  └──────────────┘                           │
│                                             │
│  ┌──────────────┐  Prioridad 4              │
│  │ coordinator  │  ← State machine          │
│  └──────────────┘                           │
│                                             │
│  ┌──────────────┐  Prioridad 2              │
│  │ workers      │  ← Search/Rest            │
│  └──────────────┘                           │
│                                             │
│  ┌──────────────┐  Prioridad 1              │
│  │ storage_task │  ← SD operations          │
│  └──────────────┘                           │
│                                             │
└─────────────────────────────────────────────┘
```

---

## Compilación

```bash
# Clonar el repositorio
git clone https://github.com/MistolSoft/MostolitoRpg.git
cd MostolitoRpg/firmware

# Compilar con ESP-IDF v6.0
idf.py build

# Flashear al dispositivo
idf.py -p COM3 flash monitor
```

### Requisitos

- ESP-IDF v6.0
- Python 3.8+
- Plataforma ESP32-S3

---

## Estructura del proyecto

```
firmware/
├── main/
│   └── main.c              # Entry point
│
├── components/
│   └── mistolito_core/
│       ├── game_coordinator.c   # Máquina de estados
│       ├── combat_engine.c      # Motor d20
│       ├── display_task.c       # UI loop
│       ├── storage_task.c       # Persistencia SD
│       ├── animation_loops.c    # Sistema de animaciones
│       ├── screens.c            # LVGL screens
│       └── sprites.c            # Sprites del Pet
│
└── assets/
    └── spirites/            # Sprites en formato C array
```

---

## Roadmap

- [x] Combate idle básico
- [x] Sistema de niveles
- [x] Animaciones LVGL
- [ ] Sistema de profesiones
- [ ] Interacción IMU (movimiento)
- [ ] Interacción táctil
- [ ] Generación procedural de mundos
- [ ] Integración con LLM para narrativas

---

## Contribuir

¡Las contribuciones son bienvenidas! Por favor:

1. Fork el repositorio
2. Creá una rama para tu feature (`git checkout -b feature/nueva-funcionalidad`)
3. Commit tus cambios (`git commit -m 'Agrega nueva funcionalidad'`)
4. Push a la rama (`git push origin feature/nueva-funcionalidad`)
5. Abrí un Pull Request

---

## Licencia

Este proyecto está bajo la **Licencia MIT**. 

Copyright (c) 2026 MistolSoft

Ver [LICENSE](LICENSE) para más detalles.

---

## Autores

**MistolSoft** - *Desarrollo inicial*

---

<p align="center">
  Hecho con ❤️ para la comunidad ESP32
</p>
