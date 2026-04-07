# install_env.ps1
# Script para instalar el entorno de desarrollo de MistolitoRPG
# Ejecutar en PowerShell normal (NO requiere administrador)

param(
    [string]$Action = "install"
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host $Message -ForegroundColor Yellow
    Write-Host "========================================`n" -ForegroundColor Cyan
}

function Test-Command {
    param([string]$Command)
    $result = Get-Command $Command -ErrorAction SilentlyContinue
    return $null -ne $result
}

function Install-EIM {
    Write-Step "Instalando EIM CLI (ESP-IDF Installation Manager)"
    
    if (Test-Command "eim") {
        Write-Host "EIM ya está instalado" -ForegroundColor Green
        eim list
        return
    }
    
    Write-Host "Instalando EIM via winget..." -ForegroundColor White
    winget install Espressif.EIM-CLI --accept-source agreements --accept-package-agreements
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error instalando EIM. Intenta manualmente: winget install Espressif.EIM-CLI" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "EIM instalado correctamente" -ForegroundColor Green
}

function Install-ESP_IDF {
    Write-Step "Instalando ESP-IDF v5.4"
    
    if (Test-Command "idf.py") {
        Write-Host "ESP-IDF ya está instalado:" -ForegroundColor Green
        idf.py --version
        return
    }
    
    Write-Host "Esto puede tardar 10-30 minutos..." -ForegroundColor Yellow
    Write-Host "Descargando e instalando ESP-IDF v5.4..." -ForegroundColor White
    
    eim install -i v5.4
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error instalando ESP-IDF. Revisa los logs arriba." -ForegroundColor Red
        exit 1
    }
    
    Write-Host "ESP-IDF instalado correctamente" -ForegroundColor Green
}

function Install-PythonDeps {
    Write-Step "Instalando dependencias Python para conversión de sprites"
    
    Write-Host "Instalando pypng..." -ForegroundColor White
    pip install pypng --quiet
    
    Write-Host "Instalando lz4..." -ForegroundColor White
    pip install lz4 --quiet
    
    Write-Host "Dependencias Python instaladas" -ForegroundColor Green
}

function New-ProjectStructure {
    Write-Step "Creando estructura del proyecto"
    
    $basePath = "C:\Users\blanc\Documents\Proyectos\MistolitoRPG"
    
    if (Test-Path $basePath) {
        Write-Host "El proyecto ya existe en: $basePath" -ForegroundColor Yellow
        Write-Host "¿Deseas recrearlo? (S/N): " -NoNewline
        $response = Read-Host
        if ($response -ne "S") {
            Write-Host "Manteniendo estructura existente" -ForegroundColor White
            return
        }
        Remove-Item $basePath -Recurse -Force
    }
    
    # Crear proyecto con ESP-IDF
    Write-Host "Creando proyecto con idf.py..." -ForegroundColor White
    New-Item -ItemType Directory -Path "C:\Users\blanc\Documents\Proyectos" -Force | Out-Null
    Set-Location "C:\Users\blanc\Documents\Proyectos"
    
    idf.py create-project MistolitoRPG
    Set-Location $basePath
    idf.py set-target esp32s3
    
    # Crear estructura de carpetas adicionales
    $directories = @(
        "components\hud",
        "components\combat",
        "components\pet",
        "components\dna",
        "components\storage",
        "main\sprites",
        "assets\sprites",
        "assets\dna"
    )
    
    foreach ($dir in $directories) {
        New-Item -ItemType Directory -Path "$basePath\$dir" -Force | Out-Null
    }
    
    Write-Host "Estructura creada en: $basePath" -ForegroundColor Green
}

function Copy-Documentation {
    Write-Step "Enlazando documentación"
    
    $docsPath = $PSScriptRoot
    $projectPath = "C:\Users\blanc\Documents\Proyectos\MistolitoRPG"
    
    # Crear acceso directo simbólico a documentación (si no existe)
    if (-not (Test-Path "$projectPath\docs")) {
        # En Windows, usar junction en lugar de symlink
        cmd /c mklink /J "$projectPath\docs" $docsPath 2>$null
        Write-Host "Documentación enlazada" -ForegroundColor Green
    }
}

function New-ConfigFiles {
    Write-Step "Creando archivos de configuración"
    
    $projectPath = "C:\Users\blanc\Documents\Proyectos\MistolitoRPG"
    
    # sdkconfig.defaults
    $sdkconfig = @"
# ESP32-S3 Target (configurado con idf.py set-target)

# PSRAM (Octal, 80MHz - Waveshare ESP32-S3-LCD-2)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY=y

# Flash (16MB QIO)
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

# CPU (240 MHz)
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y

# Compilador (optimizar por performance)
CONFIG_COMPILER_OPTIMIZATION_PERF=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
"@
    
    Set-Content -Path "$projectPath\sdkconfig.defaults" -Value $sdkconfig
    
    # idf_component.yml
    $componentYml = @'
dependencies:
  lvgl/lvgl:
    version: "9.*"
  espressif/esp_lcd_st7789:
    version: "*"
'@
    
    Set-Content -Path "$projectPath\idf_component.yml" -Value $componentYml
    
    # .gitignore
    $gitignore = @"
# ESP-IDF
build/
sdkconfig
.sdkconfig
dependencies.lock

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

# Python
__pycache__/
*.pyc
.env
venv/
"@
    
    Set-Content -Path "$projectPath\.gitignore" -Value $gitignore
    
    Write-Host "Archivos de configuración creados" -ForegroundColor Green
}

function Invoke-FirstBuild {
    Write-Step "Primer build del proyecto"
    
    $projectPath = "C:\Users\blanc\Documents\Proyectos\MistolitoRPG"
    Set-Location $projectPath
    
    Write-Host "Ejecutando primer build..." -ForegroundColor White
    idf.py reconfigure
    idf.py build
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build exitoso!" -ForegroundColor Green
    } else {
        Write-Host "Build falló. Revisa los errores arriba." -ForegroundColor Red
    }
}

function Show-NextSteps {
    Write-Step "Próximos pasos"
    
    Write-Host @"
    
El entorno está listo. Próximos pasos:

1. CONECTAR LA PLACA:
   - Conecta el ESP32-S3-LCD-2 via USB-C
   - Espera a que Windows instale los drivers (automático)
   - Verifica el puerto COM con: mode

2. DETECTAR PUERTO:
   - Ejecuta: mode
   - O: Get-WMIObject Win32_SerialPort | Select-Object DeviceID, Description
   - Anota el puerto (ej: COM3, COM4)

3. FLASHEAR PROYECTO BASE:
   - idf.py -p COM3 flash monitor
   - (Reemplaza COM3 por tu puerto)

4. PARA SALIR DEL MONITOR:
   - Presiona: Ctrl+]

Documentación en: $PSScriptRoot

"@
}

# Ejecutar instalación
if ($Action -eq "install") {
    Install-EIM
    Install-ESP_IDF
    Install-PythonDeps
    New-ProjectStructure
    Copy-Documentation
    New-ConfigFiles
    
    Write-Step "Instalación completada"
    Show-NextSteps
    
} elseif ($Action -eq "build") {
    Invoke-FirstBuild
    
} elseif ($Action -eq "status") {
    Write-Step "Verificando estado"
    
    Write-Host "Python: " -NoNewline
    python --version
    
    Write-Host "Git: " -NoNewline
    git --version
    
    Write-Host "ESP-IDF: " -NoNewline
    if (Test-Command "idf.py") {
        idf.py --version
    } else {
        Write-Host "NO INSTALADO" -ForegroundColor Red
    }
    
    Write-Host "EIM: " -NoNewline
    if (Test-Command "eim") {
        Write-Host "INSTALADO" -ForegroundColor Green
    } else {
        Write-Host "NO INSTALADO" -ForegroundColor Red
    }
    
} else {
    Write-Host "Uso: .\install_env.ps1 -Action [install|build|status]" -ForegroundColor Yellow
    Write-Host "  install - Instala todo el entorno" -ForegroundColor White
    Write-Host "  build   - Ejecuta primer build" -ForegroundColor White
    Write-Host "  status  - Muestra estado de herramientas" -ForegroundColor White
}
