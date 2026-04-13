# Changelog

Todos los cambios notables de este proyecto se documentarán en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y el versionado sigue [SemVer](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2026-04-13

### Agregado
- Sistema de profesiones (Novice, Warrior, Mage, Rogue)
- Sistema de skills y perks con requisitos por profession_level
- Motor de profesiones con bonus por nivel
- Motor de skills/perks con tiradas de éxito
- DNA engine criptográfico para identidad única del pet
- Documentación de DNA Engine
- Archivo de arquitectura del firmware

### Cambiado
- Optimización del sistema de combate para ejecución en tiempo real
- UI ahora hace pull del estado de combate en lugar de pre-cálculo
- Eliminado pre-cálculo de combate que causaba stack overflow
- Sistema de niveles con profession_level separado (1-20, cicla)
- Tablas JSON actualizadas con estructura de profesiones, skills y perks

### Corregido
- Heap corruption por no ceder CPU con taskYIELD()
- Stack overflow en coordinator por array de grabación de combate
- Animaciones que se congelaban durante combate

## [0.1.0] - 2026-04-06

### Agregado
- Arquitectura multi-task con FreeRTOS
- State machine central (coordinator)
- Motor de combate d20 basado en SRD 5.1
- Sistema de niveles y progresión
- Sistema de animaciones con cola de eventos visuales
- UI con LVGL 9
- Mutex para SPI bus compartido (LCD + SD)
- Soporte para ESP32-S3 Waveshare LCD
- README.md y LICENSE (MIT)
- Sistema de versionado manual
