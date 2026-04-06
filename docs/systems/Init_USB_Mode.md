# Sistema de Inicialización por USB (Init Mode)

## Concepto

El firmware debe tener dos modos de operación:
1. **Init Mode**: Primera vez que se ejecuta, espera comandos por USB para recibir archivos de configuración
2. **Normal Mode**: Modo de operación normal, el juego corre normalmente

---

## Flujo de Boot

```
Boot
  │
  ├── Verificar si existe flag de inicialización en flash/SD
  │   ├── NO existe → Entrar en INIT MODE
  │   └── Existe → Entrar en NORMAL MODE
  │
  ├── INIT MODE:
  │   ├── Mostrar mensaje en LCD: "INIT MODE - Waiting for USB..."
  │   ├── Esperar comandos por Serial/USB
  │   ├── Recibir archivos:
  │   │   ├── game_tables.json (profesiones, skills, level tables)
  │   │   ├── pet_dna.json (ADN del pet)
  │   │   └── pet_data.json (datos iniciales del pet)
  │   ├── Guardar archivos en SD
  │   ├── Escribir flag de inicialización
  │   └── Reiniciar → Entrar en NORMAL MODE
  │
  └── NORMAL MODE:
      ├── Cargar game_tables.json desde SD
      ├── Cargar/Generar pet según estado
      ├── Inicializar LCD, LVGL, UI
      └── Iniciar tareas del juego
```

---

## Comandos USB (Init Mode)

### Formato de Comandos

Todos los comandos siguen el formato:
```
CMD:<comando>:<datos>
```

### Comandos Disponibles

| Comando | Descripción | Ejemplo |
|---------|-------------|---------|
| `FILE_START:<nombre>:<tamaño>` | Inicia recepción de archivo | `CMD:FILE_START:game_tables.json:4521` |
| `FILE_DATA:<datos_base64>` | Envía chunk de datos | `CMD:FILE_DATA:eyJwcm9mZXNzaW9ucyI6...` |
| `FILE_END` | Finaliza recepción de archivo | `CMD:FILE_END` |
| `INIT_COMPLETE` | Marca inicialización como completa | `CMD:INIT_COMPLETE` |
| `STATUS` | Solicita estado actual | `CMD:STATUS` |
| `LIST_FILES` | Lista archivos en SD | `CMD:LIST_FILES` |
| `WIPE` | Borra todos los datos y reinicia | `CMD:WIPE` |

### Ejemplo de Sesión

```
[ESP32] MistolitoRPG v1.0 - INIT MODE
[ESP32] Waiting for commands...
[PC]    CMD:STATUS
[ESP32] STATUS:WAITING_FOR_FILES
[ESP32] Missing: game_tables.json, pet_dna.json, pet_data.json

[PC]    CMD:FILE_START:game_tables.json:4521
[ESP32] FILE_RECV_START:game_tables.json:4521
[PC]    CMD:FILE_DATA:eyJwcm9mZXNzaW9ucyI6...
[ESP32] FILE_RECV_PROGRESS:1024:4521
[PC]    CMD:FILE_DATA:NobGVkZ2UiOi...
[ESP32] FILE_RECV_PROGRESS:2048:4521
...
[PC]    CMD:FILE_END
[ESP32] FILE_RECV_OK:game_tables.json:4521 bytes saved

[PC]    CMD:FILE_START:pet_dna.json:285
[ESP32] FILE_RECV_START:pet_dna.json:285
[PC]    CMD:FILE_DATA:eyJuYW1lIjoiTWlzdG9sa...
[ESP32] FILE_RECV_PROGRESS:285:285
[PC]    CMD:FILE_END
[ESP32] FILE_RECV_OK:pet_dna.json:285 bytes saved

[PC]    CMD:INIT_COMPLETE
[ESP32] INIT_COMPLETE: All files received
[ESP32] Rebooting into NORMAL MODE...
```

---

## Archivos Requeridos

### game_tables.json
```json
{
  "config": { "cycle_length": 20, "cycle_multiplier": 0.5 },
  "professions": [...],
  "level_tables": { "novice": {...}, "warrior": {...}, ... },
  "skills": [...]
}
```

### pet_dna.json
```json
{
  "name": "Mistolito",
  "str_base": 10,
  "dex_base": 10,
  "con_base": 10,
  "int_base": 10,
  "wis_base": 10,
  "cha_base": 10,
  "str_cap": 18,
  "dex_cap": 18,
  "con_cap": 18,
  "int_cap": 18,
  "wis_cap": 18,
  "cha_cap": 18
}
```

### pet_data.json (Opcional - si no existe se genera)
```json
{
  "name": "Mistolito",
  "level": 1,
  "exp": 0,
  "hp": 25,
  "hp_max": 25,
  "str": 10,
  "dex": 10,
  "con": 10,
  "intel": 10,
  "wis": 10,
  "cha": 10,
  "profession_id": 0,
  "dp": 0,
  "enemies_defeated": 0,
  "lives": 1,
  "energy": 10,
  "bonuses": { "min_damage": 0, "max_damage": 0, ... },
  "skills": []
}
```

---

## Flag de Inicialización

### Ubicación
- **NVS (Non-Volatile Storage)**: `namespace: "mistolito"`, `key: "init_complete"`
- **Alternativa**: Archivo `/BRAIN/.init_flag` en SD

### Valores
- `0` o no existe → Init Mode
- `1` → Normal Mode

---

## Implementación

### Nuevo Componente: `usb_init`

**Archivos:**
```
firmware/components/usb_init/
├── CMakeLists.txt
├── usb_init.h
└── usb_init.c
```

**Funciones principales:**

```c
// Inicializar modo USB
esp_err_t usb_init_mode_start(void);

// Verificar si está en modo init
bool usb_is_init_mode(void);

// Procesar comando recibido
esp_err_t usb_process_command(const char *cmd, char *response, size_t resp_len);

// Verificar archivos requeridos
bool usb_check_required_files(void);

// Escribir flag de inicialización completa
esp_err_t usb_set_init_complete(void);

// Borrar todo y reiniciar
esp_err_t usb_wipe_all(void);
```

### Modificaciones en main.c

```c
void app_main(void)
{
    // Verificar modo de inicialización
    if (usb_is_init_mode()) {
        // INIT MODE
        ESP_LOGI(TAG, "=== INIT MODE ===");
        ESP_LOGI(TAG, "Connect USB and send files");
        
        lcd_init();
        display_init();
        ui_show_init_screen();  // "INIT MODE - Waiting for USB..."
        
        usb_init_mode_start();  // Bloquea hasta recibir INIT_COMPLETE
        
        esp_restart();  // Reiniciar en modo normal
    }
    
    // NORMAL MODE (código actual)
    ESP_LOGI(TAG, "=== NORMAL MODE ===");
    // ... resto del código ...
}
```

---

## Herramienta PC (Python)

**Script:** `scripts/send_init_files.py`

```python
import serial
import base64
import json
import os
import time

class MistolitoUSBInit:
    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(2)  # Esperar a que el ESP32 reinicie
        
    def send_file(self, filepath):
        filename = os.path.basename(filepath)
        filesize = os.path.getsize(filepath)
        
        # Enviar comando de inicio
        self.send_command(f"FILE_START:{filename}:{filesize}")
        
        # Leer y enviar archivo en chunks
        with open(filepath, 'rb') as f:
            while True:
                chunk = f.read(512)
                if not chunk:
                    break
                encoded = base64.b64encode(chunk).decode('utf-8')
                self.send_command(f"FILE_DATA:{encoded}")
        
        # Finalizar archivo
        self.send_command("FILE_END")
    
    def send_command(self, cmd):
        self.ser.write(f"CMD:{cmd}\n".encode())
        response = self.ser.readline().decode().strip()
        return response
    
    def complete_init(self):
        return self.send_command("INIT_COMPLETE")

# Uso:
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', required=True)
    parser.add_argument('--game-tables', default='firmware/data/game_tables.json')
    parser.add_argument('--dna', default='firmware/data/pet_dna.json')
    args = parser.parse_args()
    
    init = MistolitoUSBInit(args.port)
    
    # Enviar archivos
    init.send_file(args.game_tables)
    init.send_file(args.dna)
    
    # Completar inicialización
    init.complete_init()
    print("Init complete! Rebooting...")
```

---

## Secuencia de Uso

### Primera vez (factory fresh)

1. Flashear firmware: `idf.py flash`
2. El ESP32 entra en **INIT MODE** automáticamente
3. Ejecutar script Python:
   ```bash
   python scripts/send_init_files.py --port COM3
   ```
4. El script envía los archivos JSON
5. El ESP32 reinicia en **NORMAL MODE**
6. El juego comienza normalmente

### Reinicio normal

- Si el flag está seteado, entra directo en NORMAL MODE
- El juego carga archivos desde SD y funciona

### Forzar INIT MODE

- Comando: `CMD:WIPE` (borra todo y reinicia en INIT MODE)
- O: Borrar el flag de NVS manualmente

---

## Consideraciones

### Tamaño de chunks
- **Recomendado:** 512 bytes por chunk
- **Máximo:** 4096 bytes (limitado por buffer serial)

### Timeout
- **Conexión:** 5 segundos
- **Por archivo:** 30 segundos
- **Por chunk:** 1 segundo

### Seguridad
- No implementar en producción (solo desarrollo)
- En producción: usar OTA con firmas criptográficas

### Display
- Mostrar progreso en LCD: "Receiving game_tables.json... 45%"
- Animación simple mientras espera

---

## Tareas de Implementación

1. [ ] Crear componente `usb_init`
2. [ ] Implementar parser de comandos
3. [ ] Implementar recepción de archivos con base64
4. [ ] Agregar sistema de flags en NVS
5. [ ] Modificar `main.c` para soportar ambos modos
6. [ ] Crear pantalla de UI para INIT MODE
7. [ ] Crear script Python `send_init_files.py`
8. [ ] Probar flujo completo end-to-end
9. [ ] Documentar uso en README

---

## Alternativa: USB Mass Storage

Una alternativa más simple sería implementar USB Mass Storage (MSC):
- El ESP32 se presenta como un drive USB
- Se copian archivos directamente desde Windows
- Más complejo de implementar pero más fácil de usar

**Decisión:** Por ahora usar Serial/USB init mode por simplicidad.
