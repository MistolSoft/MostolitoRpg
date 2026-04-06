# Sprites y Animaciones en LVGL 9

Este documento describe cómo crear, convertir y usar sprites en MistolitoRPG con LVGL 9.

---

## 1. Widget lv_animimg

LVGL 9 incluye `lv_animimg` (Animation Image), un widget diseñado específicamente para animaciones de sprites.

### 1.1 Ventajas sobre LVGL 8

| Característica | LVGL 8 | LVGL 9 (lv_animimg) |
|----------------|--------|---------------------|
| Animación de frames | Manual con timers | Built-in |
| Configuración | Compleja | Simple (set_src, set_duration) |
| Loop infinito | Manual | LV_ANIM_REPEAT_INFINITE |
| Memoria | Requiere gestión manual | Optimizado |

### 1.2 API Básica

```c
lv_obj_t *animimg = lv_animimg_create(parent);

lv_animimg_set_src(animimg, image_dsc_array, array_size);
lv_animimg_set_duration(animimg, duration_ms);
lv_animimg_set_repeat_count(animimg, count);
lv_animimg_start(animimg);
```

---

## 2. Flujo de Trabajo para Sprites

### 2.1 Diseño

1. Crear sprites en cualquier software (Aseprite, Photoshop, GIMP)
2. Resolución recomendada para el Pet: **64x64** o **80x80** píxeles
3. Exportar cada frame como PNG separado

### 2.2 Conversión

Usar el conversor online: https://lvgl.io/tools/imageconverter

**Configuración recomendada:**

| Parámetro | Valor | Razón |
|-----------|-------|-------|
| Color format | RGB565 | 16-bit, balance tamaño/calidad |
| Output format | C array | Compilado en firmware |
| Stride alignment | 1 byte | Sin padding extra |

**Para sprites con transparencia:**
- Color format: ARGB8565 (o ARGB1555 si se necesita ahorrar memoria)

### 2.3 Integración

```c
#include "sprites/pet_idle_0.c"
#include "sprites/pet_idle_1.c"
#include "sprites/pet_idle_2.c"

LV_IMG_DECLARE(pet_idle_0);
LV_IMG_DECLARE(pet_idle_1);
LV_IMG_DECLARE(pet_idle_2);

static lv_image_dsc_t *idle_frames[] = {
    &pet_idle_0,
    &pet_idle_1,
    &pet_idle_2,
};
```

---

## 3. Sprites MVP

### 3.1 Sprites Necesarios

| Nombre | Frames | Tamaño estimado | Uso |
|--------|--------|-----------------|-----|
| `pet_idle` | 4 | ~20 KB | Pet en espera |
| `pet_attack` | 3 | ~15 KB | Animación de ataque |
| `pet_hit` | 2 | ~10 KB | Recibe daño |
| `pet_death` | 3 | ~15 KB | Muerte del Pet |
| `pet_levelup` | 3 | ~15 KB | Celebración |
| **Total** | **15** | **~75 KB** | |

### 3.2 Resolución

Para pantalla 240x320:
- Pet sprite: **80x80** píxeles (centrado, con espacio para UI)
- Enemy sprite: **64x64** píxeles
- Icons: **16x16** o **24x24** píxeles

---

## 4. Ejemplo Completo: Pet Sprite

### 4.1 Archivo de Sprite

Después de convertir con el conversor online, guardar como `pet_idle_0.c`:

```c
// pet_idle_0.c
#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

static const LV_ATTRIBUTE_MEM_ALIGN uint8_t pet_idle_0_map[] = {
    // Datos del sprite convertidos...
    0x00, 0x00, 0x1F, 0x00, ...
};

const lv_image_dsc_t pet_idle_0 = {
    .header.always_zero = 0,
    .header.w = 80,
    .header.h = 80,
    .header.cf = LV_COLOR_FORMAT_RGB565,
    .data_size = 80 * 80 * 2,
    .data = pet_idle_0_map,
};
```

### 4.2 Header

```c
// sprites.h
#ifndef SPRITES_H
#define SPRITES_H

#include "lvgl.h"

// Pet Idle
LV_IMG_DECLARE(pet_idle_0);
LV_IMG_DECLARE(pet_idle_1);
LV_IMG_DECLARE(pet_idle_2);
LV_IMG_DECLARE(pet_idle_3);

// Pet Attack
LV_IMG_DECLARE(pet_attack_0);
LV_IMG_DECLARE(pet_attack_1);
LV_IMG_DECLARE(pet_attack_2);

// Pet Hit
LV_IMG_DECLARE(pet_hit_0);
LV_IMG_DECLARE(pet_hit_1);

// Pet Death
LV_IMG_DECLARE(pet_death_0);
LV_IMG_DECLARE(pet_death_1);
LV_IMG_DECLARE(pet_death_2);

// Pet Level Up
LV_IMG_DECLARE(pet_levelup_0);
LV_IMG_DECLARE(pet_levelup_1);
LV_IMG_DECLARE(pet_levelup_2);

#endif // SPRITES_H
```

### 4.3 Módulo de Animación

```c
// pet_sprite.c
#include "sprites.h"
#include "pet_sprite.h"

typedef enum {
    ANIM_IDLE,
    ANIM_ATTACK,
    ANIM_HIT,
    ANIM_DEATH,
    ANIM_LEVELUP,
} pet_animation_t;

static lv_obj_t *pet_sprite = NULL;
static lv_animimg_anim_t current_anim = ANIM_IDLE;

static lv_image_dsc_t *idle_frames[] = {&pet_idle_0, &pet_idle_1, &pet_idle_2, &pet_idle_3};
static lv_image_dsc_t *attack_frames[] = {&pet_attack_0, &pet_attack_1, &pet_attack_2};
static lv_image_dsc_t *hit_frames[] = {&pet_hit_0, &pet_hit_1};
static lv_image_dsc_t *death_frames[] = {&pet_death_0, &pet_death_1, &pet_death_2};
static lv_image_dsc_t *levelup_frames[] = {&pet_levelup_0, &pet_levelup_1, &pet_levelup_2};

void pet_sprite_init(lv_obj_t *parent) {
    pet_sprite = lv_animimg_create(parent);
    lv_obj_align(pet_sprite, LV_ALIGN_CENTER, 0, 0);
    
    pet_sprite_set_animation(ANIM_IDLE);
    lv_animimg_start(pet_sprite);
}

void pet_sprite_set_animation(pet_animation_t anim) {
    current_anim = anim;
    
    switch (anim) {
        case ANIM_IDLE:
            lv_animimg_set_src(pet_sprite, idle_frames, 4);
            lv_animimg_set_duration(pet_sprite, 800);
            lv_animimg_set_repeat_count(pet_sprite, LV_ANIM_REPEAT_INFINITE);
            break;
            
        case ANIM_ATTACK:
            lv_animimg_set_src(pet_sprite, attack_frames, 3);
            lv_animimg_set_duration(pet_sprite, 300);
            lv_animimg_set_repeat_count(pet_sprite, 1);
            break;
            
        case ANIM_HIT:
            lv_animimg_set_src(pet_sprite, hit_frames, 2);
            lv_animimg_set_duration(pet_sprite, 200);
            lv_animimg_set_repeat_count(pet_sprite, 2);
            break;
            
        case ANIM_DEATH:
            lv_animimg_set_src(pet_sprite, death_frames, 3);
            lv_animimg_set_duration(pet_sprite, 1000);
            lv_animimg_set_repeat_count(pet_sprite, 1);
            break;
            
        case ANIM_LEVELUP:
            lv_animimg_set_src(pet_sprite, levelup_frames, 3);
            lv_animimg_set_duration(pet_sprite, 500);
            lv_animimg_set_repeat_count(pet_sprite, 2);
            break;
    }
    
    lv_animimg_start(pet_sprite);
}
```

---

## 5. Gestión de Memoria

### 5.1 Ubicación de Sprites

Los sprites compilados como C arrays se almacenan en **Flash** (no en RAM):

```
Flash (16 MB)
├── firmware (~2 MB)
├── sprites compilados (~100 KB)
└── particiones restantes
```

### 5.2 Carga en RAM

LVGL 9 carga los sprites en RAM solo cuando se renderizan:

```
Render cycle:
1. LVGL lee sprite desde Flash
2. Decodifica a RAM temporal
3. Flush al display
4. Libera RAM temporal
```

### 5.3 Cache de Sprites

LVGL 9 tiene cache automático de imágenes:

```c
// Configurar tamaño de cache (en lv_conf.h)
#define LV_CACHE_DEF_SIZE_PX_ALIGN  (80 * 80)  // Cache para sprites 80x80
```

---

## 6. Alternativa: Sprites en SD

Para proyectos con muchos sprites, cargar desde SD:

### 6.1 Estructura

```
/SD_ROOT/ASSETS/SPRITES/
├── pet/
│   ├── idle_0.png
│   ├── idle_1.png
│   └── ...
└── enemy/
    ├── slime_0.png
    └── ...
```

### 6.2 Código

```c
lv_obj_t *img = lv_image_create(lv_scr_act());
lv_image_set_src(img, "A:/ASSETS/SPRITES/pet/idle_0.png");
```

**Nota:** Requiere montar el filesystem con `lv_fs_if`.

---

## 7. Conversión con Script Python (Recomendado)

LVGL incluye un script Python oficial para convertir PNG a archivos .c: `LVGLImage.py`

### 7.1 Instalación de Dependencias

```powershell
pip install pypng lz4
```

### 7.2 Descargar el Script

```powershell
# Desde la raíz del proyecto
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/lvgl/lvgl/master/scripts/LVGLImage.py" -OutFile "scripts/tools/LVGLImage.py"
```

### 7.3 Uso del Script

```powershell
# Convertir un PNG a archivo .c
python scripts/tools/LVGLImage.py --ofmt C --cf RGB565 -o firmware/components/hud/sprites assets/sprites/pet_idle_0.png

# Convertir todos los PNG de una carpeta
python scripts/tools/LVGLImage.py --ofmt C --cf RGB565 -o firmware/components/hud/sprites assets/sprites/

# Con transparencia (ARGB8565)
python scripts/tools/LVGLImage.py --ofmt C --cf ARGB8565 -o firmware/components/hud/sprites assets/sprites/pet_idle_0.png

# Con compresión LZ4
python scripts/tools/LVGLImage.py --ofmt C --cf RGB565 --compress LZ4 -o output assets/sprites/pet_idle_0.png
```

### 7.4 Opciones Disponibles

| Opción | Valores | Descripción |
|--------|---------|-------------|
| `--ofmt` | C, BIN, PNG | Formato de salida |
| `--cf` | RGB565, ARGB8565, etc. | Formato de color |
| `--compress` | NONE, RLE, LZ4 | Compresión |
| `--align` | 1, 2, 4, etc. | Alineación de stride |
| `--premultiply` | flag | Pre-multiplicar alpha |
| `-o` | path | Directorio de salida |
| `--name` | string | Nombre personalizado |

### 7.5 Formatos de Color Recomendados

| Uso | Formato | Descripción |
|-----|---------|-------------|
| Sprites sin transparencia | `RGB565` | 16-bit, más pequeño |
| Sprites con transparencia | `ARGB8565` | 24-bit, alpha de 8-bit |
| Iconos simples | `I1`, `I2`, `I4` | Indexado, muy compacto |
| Máscaras alpha | `A1`, `A2`, `A8` | Solo alpha |

### 7.6 Script de Conversión MVP

Crear `scripts/tools/convert_sprites.ps1`:

```powershell
# convert_sprites.ps1
# Convierte todos los sprites PNG a archivos .c para el MVP

$inputPath = "assets/sprites"
$outputPath = "firmware/components/hud/sprites"
$scriptPath = "scripts/tools/LVGLImage.py"

# Crear directorio de salida si no existe
New-Item -ItemType Directory -Path $outputPath -Force

# Convertir sprites del Pet (sin transparencia)
$petSprites = @(
    "pet_idle_0.png",
    "pet_idle_1.png",
    "pet_idle_2.png",
    "pet_idle_3.png",
    "pet_attack_0.png",
    "pet_attack_1.png",
    "pet_attack_2.png",
    "pet_hit_0.png",
    "pet_hit_1.png",
    "pet_death_0.png",
    "pet_death_1.png",
    "pet_death_2.png",
    "pet_levelup_0.png",
    "pet_levelup_1.png",
    "pet_levelup_2.png"
)

foreach ($sprite in $petSprites) {
    $input = Join-Path $inputPath $sprite
    if (Test-Path $input) {
        python $scriptPath --ofmt C --cf RGB565 -o $outputPath $input
        Write-Host "Converted: $sprite"
    }
}

Write-Host "Done! Sprites converted to $outputPath"
```

---

## 8. Alternativa: Conversión Online

Si no quieres usar Python, el conversor online sigue siendo válido:

**URL:** https://lvgl.io/tools/imageconverter

**Configuración:**
- Color format: RGB565
- Output format: C array

---

## 9. Herramientas de Diseño Recomendadas

### 9.1 Diseño de Sprites

| Herramienta | Uso | Precio |
|-------------|-----|--------|
| Aseprite | Pixel art, animaciones | $20 |
| GIMP | Edición general | Gratis |
| Piskel | Pixel art online | Gratis |
| Photoshop | Edición profesional | Suscripción |

### 9.2 Conversión

| Herramienta | Uso |
|-------------|-----|
| [LVGL Image Converter](https://lvgl.io/tools/imageconverter) | Online, oficial |
| [LVGL Image Converter CLI](https://github.com/lvgl/lv_image_converter) | Línea de comandos |
| [LVGL Online Simulator](https://sim.lvgl.io) | Testing |

---

## 10. Ejemplo de Estilo de Sprite

Para MistolitoRPG, estilo recomendado:

- **Paleta:** Limitada (16-32 colores)
- **Estilo:** Pixel art, algo cartoon
- **Silueta:** Clara y reconocible
- **Animaciones:** Fluidas pero simples (3-4 frames)
- **Tamaño:** 80x80 píxeles máximo

### Referencia Visual

```
┌────────────────────────┐
│   ♥ ♥     (ojos)       │
│    ▼      (boca)       │
│  ╔═══╗    (cuerpo)     │
│  ║   ║                 │
│  ╚═══╝                 │
│   ║ ║     (piernas)    │
└────────────────────────┘
     80x80 pixels
```
