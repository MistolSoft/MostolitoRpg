# Setup SD Card for MistolitoRPG
# This script prepares the SD card with the required directory structure and data files

param(
    [string]$SDDrive
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "MistolitoRPG - SD Card Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Auto-detect SD drive if not specified
if (-not $SDDrive) {
    Write-Host ""
    Write-Host "Auto-detecting SD card drive..." -ForegroundColor Yellow
    
    # Look for drives with common SD card labels or removable drives
    $removableDrives = Get-WmiObject -Class Win32_LogicalDisk | 
        Where-Object { $_.DriveType -eq 2 } | 
        ForEach-Object { $_.DeviceID }
    
    if ($removableDrives.Count -eq 0) {
        Write-Error "No removable drives found. Please insert SD card and try again, or specify drive with -SDDrive"
        Write-Host "Usage: .\setup_sd_card.ps1 -SDDrive E:" -ForegroundColor Yellow
        exit 1
    } elseif ($removableDrives.Count -eq 1) {
        $SDDrive = $removableDrives[0]
        Write-Host "  Found removable drive: $SDDrive" -ForegroundColor Green
    } else {
        Write-Host "  Multiple removable drives found:" -ForegroundColor Yellow
        for ($i = 0; $i -lt $removableDrives.Count; $i++) {
            $volName = (Get-Volume -DriveLetter $removableDrives[$i].Replace(':','') -ErrorAction SilentlyContinue).FileSystemLabel
            Write-Host "    [$i] $($removableDrives[$i]) $volName" -ForegroundColor Gray
        }
        Write-Host ""
        $selection = Read-Host "Select SD card drive (0-$($removableDrives.Count - 1))"
        if ($selection -match '^\d+$' -and [int]$selection -lt $removableDrives.Count) {
            $SDDrive = $removableDrives[[int]$selection]
        } else {
            Write-Error "Invalid selection"
            exit 1
        }
    }
}

# Validate SD drive exists
if (-not (Test-Path $SDDrive)) {
    Write-Error "SD drive $SDDrive not found. Please insert SD card and try again."
    exit 1
}

Write-Host ""
Write-Host "Using SD drive: $SDDrive" -ForegroundColor Green

# Define paths
$projectRoot = Split-Path -Parent $PSScriptRoot
$dataSource = Join-Path $projectRoot "firmware\data"
$sdRoot = $SDDrive
$sdData = Join-Path $sdRoot "DATA"
$sdBrain = Join-Path $sdRoot "BRAIN"
$sdBrainPet = Join-Path $sdBrain "PET"

# Create directory structure
Write-Host ""
Write-Host "Creating directory structure..." -ForegroundColor Yellow

$directories = @(
    $sdData,
    $sdBrain,
    $sdBrainPet
)

foreach ($dir in $directories) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host "  Created: $dir" -ForegroundColor Green
    } else {
        Write-Host "  Exists: $dir" -ForegroundColor Gray
    }
}

# Copy game tables
Write-Host ""
Write-Host "Copying game data files..." -ForegroundColor Yellow

$gameTables = Join-Path $dataSource "game_tables.json"
$sdGameTables = Join-Path $sdData "game_tables.json"

if (Test-Path $gameTables) {
    Copy-Item -Path $gameTables -Destination $sdGameTables -Force
    Write-Host "  Copied: game_tables.json" -ForegroundColor Green
} else {
    Write-Error "game_tables.json not found at $gameTables"
    Write-Host "  Run this script from the project root directory" -ForegroundColor Red
    exit 1
}

# Create default pet_data.json if not exists
$petDataPath = Join-Path $sdBrainPet "pet_data.json"
if (-not (Test-Path $petDataPath)) {
    Write-Host ""
    Write-Host "Creating default pet_data.json..." -ForegroundColor Yellow
    
    $defaultPet = @{
        name = "Mistolito"
        level = 1
        exp = 0
        hp = 25
        hp_max = 25
        str = 10
        dex = 10
        con = 10
        intel = 10
        wis = 10
        cha = 10
        profession_id = 0
        dp = 0
        enemies_defeated = 0
        lives = 1
        energy = 10
        bonuses = @{
            min_damage = 0
            max_damage = 0
            extra_dice = 0
            crit = 0
            sneak_dice = 0
            skill_uses = 0
        }
        skills = @()
    }
    
    $defaultPet | ConvertTo-Json -Depth 3 | Out-File -FilePath $petDataPath -Encoding UTF8
    Write-Host "  Created: pet_data.json" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "Pet data already exists, skipping..." -ForegroundColor Gray
}

# Copy DNA template if exists
$dnaSource = Join-Path $dataSource "pet_dna.json"
$dnaDest = Join-Path $sdBrain "pet_dna.json"

if (Test-Path $dnaSource) {
    Copy-Item -Path $dnaSource -Destination $dnaDest -Force
    Write-Host "  Copied: pet_dna.json" -ForegroundColor Green
}

# Summary
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "SD Card Setup Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Directory structure:" -ForegroundColor Yellow
Write-Host "  $sdRoot\" -ForegroundColor Gray
Write-Host "  |-- DATA\" -ForegroundColor Gray
Write-Host "  |   |-- game_tables.json" -ForegroundColor Gray
Write-Host "  |-- BRAIN\" -ForegroundColor Gray
Write-Host "      |-- pet_dna.json" -ForegroundColor Gray
Write-Host "      |-- PET\" -ForegroundColor Gray
Write-Host "          |-- pet_data.json" -ForegroundColor Gray
Write-Host ""
Write-Host "You can now safely eject the SD card and insert it into the device." -ForegroundColor Green
