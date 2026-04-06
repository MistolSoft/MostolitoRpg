# Arquitectura del Repositorio: MistolitoRPG

Este repositorio contiene tanto la documentación como el código fuente de MistolitoRPG, organizado para escalar desde el MVP hasta el sistema completo.

---

## 1. Estructura de Directorios

```
MistolitoRPG/
│
├── docs/                          # Documentación técnica
│   ├── vision/                    # Visión y concepto
│   │   └── Vision_General.md
│   │
│   ├── hardware/                  # Especificaciones de hardware
│   │   └── Especificaciones_Tecnicas.md
│   │
│   ├── architecture/              # Arquitectura de software
│   │   ├── Arquitectura_Cerebro.md
│   │   ├── Arquitectura_ESP32.md
│   │   └── Arquitectura_Implementacion.md
│   │
│   ├── systems/                   # Sistemas del juego
│   │   ├── Sistema_Reglas_RPG.md
│   │   ├── Sistema_Vida_Muerte.md
│   │   ├── Sistema_DP_Energia.md
│   │   ├── Sistema_Progresion.md
│   │   ├── Sistema_Memoria_Fractal.md
│   │   └── DNA_System.md
│   │
│   ├── gameplay/                  # Mecánicas de juego
│   │   ├── Estados_Emocionales.md
│   │   ├── Interaccion_Usuario.md
│   │   ├── Matriz_Sensores.md
│   │   └── Generacion_Procedural.md
│   │
│   ├── world/                     # Mundo y narrativa
│   │   ├── Lore_Sistema_Mundo.md
│   │   ├── Ciclo_Vida.md
│   │   ├── Proceso_Genesis.md
│   │   └── Protocolo_Comunicacion.md
│   │
│   ├── ui/                        # Interfaz de usuario
│   │   └── Estilo_Visual_HUD.md
│   │
│   ├── dev/                       # Desarrollo
│   │   ├── Setup_Entorno.md
│   │   ├── Investigacion_Librerias.md
│   │   └── ROADMAP.md
│   │
│   └── milestones/                # Hitos de desarrollo
│       └── MVP_Combat_Basic.md
│
├── firmware/                      # Código ESP-IDF
│   ├── main/                      # Aplicación principal
│   │   ├── CMakeLists.txt
│   │   └── main.c
│   │
│   ├── components/                # Componentes modulares
│   │   ├── brain/                 # Motor cognitivo
│   │   ├── dna/                   # Sistema de ADN
│   │   ├── combat/                # Motor de combate
│   │   ├── hud/                   # Interfaz LVGL
│   │   ├── storage/               # SD y memoria fractal
│   │   ├── sensors/               # IMU, touch, cámara
│   │   └── bridge/                # Comunicación con LLM
│   │
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   └── idf_component.yml
│
├── server/                        # Backend y API
│   ├── api/                       # Endpoints
│   ├── embedding/                 # Generación de embeddings
│   ├── llm/                       # Integración con LLM
│   └── database/                  # Persistencia (si aplica)
│
├── scripts/                       # Herramientas Windows
│   ├── setup/                     # Instalación inicial
│   │   └── install_env.ps1
│   │
│   ├── flash/                     # Flasheo del ESP32
│   │   └── flash_device.ps1
│   │
│   ├── monitor/                   # Monitor serial
│   │   └── serial_monitor.ps1
│   │
│   ├── upload/                    # Subir archivos a SD
│   │   └── upload_dna.ps1
│   │
│   └── tools/                     # Utilidades varias
│       └── generate_dna.ps1
│
├── assets/                        # Recursos gráficos
│   ├── sprites/                   # Sprites del Pet
│   ├── fonts/                     # Fuentes LVGL
│   └── icons/                     # Iconos HUD
│
├── AGENTS.md                      # Guía para agentes IA
└── README.md                      # Descripción del proyecto
```

---

## 2. Propósito de Cada Directorio

### 2.1 `docs/` - Documentación

Organizada por dominio, no por tipo de archivo. Cada subdirectorio agrupa documentación relacionada.

| Directorio | Contenido |
|------------|-----------|
| `vision/` | Concepto general, filosofía |
| `hardware/` | GPIO, periféricos, limitaciones |
| `architecture/` | Diseño de software y sistemas |
| `systems/` | Mecánicas core del juego |
| `gameplay/` | Interacciones y estados |
| `world/` | Lore, narrativa, mundo |
| `ui/` | Diseño visual y HUD |
| `dev/` | Setup, herramientas, ROADMAP |
| `milestones/` | Definición de hitos |

### 2.2 `firmware/` - Código ESP32

Estructura estándar ESP-IDF con componentes modulares.

| Directorio | Contenido |
|------------|-----------|
| `main/` | Punto de entrada, inicialización |
| `components/brain/` | Capas cognitivas, vectores de estado |
| `components/dna/` | Parsing ADN, generación de Pet |
| `components/combat/` | Motor d20, resolución de turnos |
| `components/hud/` | LVGL, pantallas, animaciones |
| `components/storage/` | SD, memoria fractal, navegación vectorial |
| `components/sensors/` | Drivers IMU, touch, cámara |
| `components/bridge/` | HTTP client, JSON, APIs externas |

### 2.3 `server/` - Backend

APIs y servicios externos (fase post-MVP).

| Directorio | Contenido |
|------------|-----------|
| `api/` | Endpoints REST/WebSocket |
| `embedding/` | Servicio de embeddings (Gemini/OpenAI) |
| `llm/` | Generación narrativa |
| `database/` | Persistencia de mundos y Pets |

### 2.4 `scripts/` - Herramientas Windows

Scripts PowerShell para interactuar con el ESP32 desde Windows.

| Directorio | Contenido |
|------------|-----------|
| `setup/` | Instalación de ESP-IDF, dependencias |
| `flash/` | Compilar y flashear firmware |
| `monitor/` | Monitor serial para debug |
| `upload/` | Subir ADN, assets a SD |
| `tools/` | Generación de ADN, utilidades |

### 2.5 `assets/` - Recursos

Archivos binarios que se cargan desde la SD.

| Directorio | Contenido |
|------------|-----------|
| `sprites/` | Imágenes del Pet (PNG → C arrays) |
| `fonts/` | Fuentes LVGL |
| `icons/` | Iconos del HUD |

---

## 3. Flujo de Trabajo

### 3.1 Desarrollo MVP

```
1. docs/dev/Setup_Entorno.md → Instalar ESP-IDF
2. scripts/setup/install_env.ps1 → Automatizar instalación
3. firmware/ → Desarrollar componentes
4. scripts/flash/flash_device.ps1 → Flash al ESP32
5. scripts/monitor/serial_monitor.ps1 → Debug
```

### 3.2 Subir ADN/Assets

```
1. scripts/tools/generate_dna.ps1 → Crear ADN de prueba
2. scripts/upload/upload_dna.ps1 → Copiar a SD
3. ESP32 lee desde SD al boot
```

### 3.3 Iteración de Documentación

```
1. Editar docs/systems/*.md → Definir sistema
2. Implementar en firmware/components/
3. Actualizar docs/milestones/MVP_*.md
```

---

## 4. Convenciones de Nombres

| Tipo | Convención | Ejemplo |
|------|------------|---------|
| Documentos | Snake_Case.md | `Sistema_Vida_Muerte.md` |
| Directorios docs | Minúsculas | `systems/`, `hardware/` |
| Componentes C | Snake_case | `brain_engine.c` |
| Headers C | Snake_case.h | `dna_parser.h` |
| Scripts PS1 | Snake_case.ps1 | `flash_device.ps1` |
| Directorios código | Minúsculas | `firmware/`, `components/` |

---

## 5. Fases de Desarrollo

### Fase 1: MVP (Actual)
- `firmware/main/` → Loop básico
- `firmware/components/dna/` → Parsing ADN
- `firmware/components/combat/` → Combate idle
- `firmware/components/hud/` → UI básica
- `scripts/` → Flasheo y monitor

### Fase 2: Sistemas Core
- `firmware/components/storage/` → SD y memoria
- `firmware/components/brain/` → Capas cognitivas
- `assets/` → Sprites reales

### Fase 3: Interacción
- `firmware/components/sensors/` → IMU, touch
- `docs/gameplay/` → Expandir interacción

### Fase 4: Narrativa
- `server/` → Backend LLM
- `firmware/components/bridge/` → Comunicación
- `docs/world/` → Protocolos completos

---

## 6. Git Ignore (Sugerido)

```gitignore
# ESP-IDF
firmware/build/
firmware/sdkconfig
firmware/.config

# IDE
.vscode/
.idea/

# OS
.DS_Store
Thumbs.db

# Logs
*.log

# Temp
tmp/
temp/
```

---

## 7. Relación entre Directorios

```
docs/architecture/ ──────────► firmware/components/
        │                              │
        ▼                              ▼
docs/systems/ ───────────────► Implementación
        │
        ▼
docs/milestones/ ────────────► Testing
        │
        ▼
scripts/ ────────────────────► Depoy en hardware
```
