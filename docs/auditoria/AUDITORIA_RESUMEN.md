# Auditoría Completa - MistolitoRPG Firmware

## Resumen Ejecutivo

**Estado actual:** El proyecto NO COMPILA debido a problemas arquitectónicos críticos.

**Problemas encontrados:** 27 problemas distribuidos en 5 módulos.

**Tiempo estimado de corrección:** 8-16 horas de desarrollo.

---

## Contador de Problemas por Severidad

| Severidad | Cantidad | Módulos afectados |
|-----------|----------|-------------------|
| CRÍTICA | 7 | storage, pet, combat |
| ALTA | 9 | Todos los módulos |
| MEDIA | 6 | combat, pet, data |
| BAJA | 5 | Varios |

---

## Problemas Críticos (Impiden Compilación)

### 1. Dependencia Circular storage ↔ pet
**Archivo:** `storage/sd_card.h:7`
```c
#include "pet.h"  // Causa ciclo
```
**Solución:** Mover funciones de persistencia de pet a otro componente.

### 2. Código Duplicado en sd_card.c
**Archivo:** `storage/sd_card.c:154-185`
- Función `storage_save_pet_full()` duplicada
- Código muerto con variables inexistentes
**Solución:** Eliminar código duplicado.

### 3. CMakeLists.txt sin dependencias declaradas
**Archivos:** Varios CMakeLists.txt
- `combat` falta `data`
- `pet` falta `data`
- `data` no usa correctamente cJSON

### 4. enemy_generate() duplicada
**Archivos:** `combat/enemy.c` y `combat/combat_engine.c`
**Solución:** Eliminar implementación vacía.

### 5. cJSON no encontrado
**Archivo:** `data/data_loader.c:4`
```c
#include "cJSON.h"  // No se encuentra
```
**Solución:** Agregar dependencia correctamente.

### 6. enemy_t en archivo incorrecto
**Archivo:** `combat/combat_engine.h:35-45`
**Solución:** Mover a `enemy.h`.

### 7. pet_bonuses_t duplicado
**Archivos:** `pet.h` y `data_loader.h`
**Solución:** Mantener solo en `pet.h`.

---

## Dependencias Reales vs Declaradas

| Componente | REQUIRES declarado | REQUIRES real | Problema |
|------------|-------------------|---------------|----------|
| combat | pet | pet, data | Falta `data` |
| pet | dna | dna, data | Falta `data` |
| data | (none) | cJSON | Falta en CMakeLists |
| storage | fatfs, sdmmc, driver | + pet | No debería depender de pet |
| hud | lvgl, pet, combat | ✓ Correcto | - |
| usb_init | storage | ✓ Correcto | - |

---

## Grafo de Dependencias Actual (Roto)

```
                    ┌─────────┐
                    │  main   │
                    └────┬────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
    ┌────▼────┐    ┌────▼────┐    ┌────▼────┐
    │   hud   │    │ combat  │    │ usb_init│
    └────┬────┘    └────┬────┘    └────┬────┘
         │               │               │
    ┌────▼────┐    ┌────▼────┐    ┌────▼────┐
    │pet,combat│   │ pet,data│    │ storage │
    └────┬────┘    └────┬────┘    └────┬────┘
         │               │               │
    ┌────▼────┐    ┌────▼────┐          │
    │   pet   │    │   data  │◄─────────┘ (CICLO)
    └────┬────┘    └────┬────┘
         │               │
    ┌────▼────┐    ┌────▼────┐
    │dna, data│    │  cJSON  │
    └─────────┘    └─────────┘
```

---

## Grafo de Dependencias Propuesto

```
Layer 3 (Aplicación):
    ┌─────────┐
    │  main   │
    └────┬────┘
         │
         ├───────────────┬───────────────┐
         │               │               │
    ┌────▼────┐    ┌────▼────┐    ┌────▼────┐
    │   hud   │    │ combat  │    │ usb_init│
    └────┬────┘    └────┬────┘    └────┬────┘
         │               │               │

Layer 2 (Lógica de Negocio):
    ┌────▼────┐    ┌────▼────┐    ┌────▼────┐
    │   pet   │    │   data  │    │  enemy  │
    └────┬────┘    └────┬────┘    └─────────┘
         │               │
         │          ┌────▼────┐
         │          │  cJSON  │
         │          └─────────┘
         
Layer 1 (Fundamentos):
    ┌────▼────┐    ┌────▼────┐
    │   dna   │    │ storage │
    └─────────┘    └─────────┘
```

---

## Plan de Refactoring

### Fase 1: Resolver Dependencias Circulares (4-6 horas)
1. Mover `enemy_t` a `enemy.h`
2. Eliminar `#include "pet.h"` de `sd_card.h`
3. Crear `pet_persistence.c` o mover funciones a `pet.c`
4. Eliminar código duplicado en `sd_card.c`

### Fase 2: Declarar Dependencias Correctamente (2-3 horas)
1. Actualizar todos los CMakeLists.txt
2. Agregar cJSON como dependencia correctamente
3. Verificar que `idf_component.yml` esté correcto

### Fase 3: Limpieza de Código (2-4 horas)
1. Eliminar funciones vacías/duplicadas
2. Eliminar `TAG` sin usar
3. Mover constantes hardcodeadas a JSON o Kconfig
4. Usar cJSON para serialización en lugar de fprintf manual

### Fase 4: Testing (1-2 horas)
1. Verificar compilación limpia
2. Probar en hardware
3. Verificar persistencia

---

## Archivos de Auditoría Detallados

- [AUDITORIA_ARQUITECTURA.md](./AUDITORIA_ARQUITECTURA.md) - Visión general
- [AUDITORIA_COMBAT.md](./AUDITORIA_COMBAT.md) - Módulo combat
- [AUDITORIA_STORAGE.md](./AUDITORIA_STORAGE.md) - Módulo storage
- [AUDITORIA_PET.md](./AUDITORIA_PET.md) - Módulo pet

---

## Próximos Pasos

1. **Decidir enfoque:** ¿Refactorizar gradualmente o reescribir componentes problemáticos?
2. **Priorizar:** Resolver dependencias circulares primero
3. **Documentar decisiones:** Actualizar AGENTS.md con arquitectura correcta
4. **Implementar:** Seguir el plan de refactoring
5. **Verificar:** Compilar y probar en hardware
