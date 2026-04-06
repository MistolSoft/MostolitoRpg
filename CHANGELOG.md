# Changelog

Todos los cambios notables de este proyecto se documentarán en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y el versionado sigue [SemVer](https://semver.org/spec/v2.0.0.html).

## [2.0.1] - 2026-04-06

### Agregado
- Sistema de animaciones con cola de eventos visuales (`animation_loops`)
- Mutex para SPI bus compartido entre LCD y SD card
- Sistema de versionado manual con archivo VERSION
- README.md y LICENSE (MIT)

### Cambiado
- Display task ahora usa tick constante (250ms / 4 FPS)
- Eliminadas fases de animación del combat engine
- Refactorizada la arquitectura de coordinación FreeRTOS

### Corregido
- Race conditions en SPI bus compartido
- Animaciones que se congelaban durante combate

## [2.0.0] - 2026-04-05

### Agregado
- Arquitectura multi-task con FreeRTOS
- State machine central (coordinator)
- Motor de combate d20 basado en SRD 5.1
- Sistema de niveles y progresión
- UI con LVGL 9
- Soporte para ESP32-S3 Waveshare LCD

### Notas
- Reescritura completa del firmware
- Arquitectura orientada a idle game
