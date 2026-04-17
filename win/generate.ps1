#Requires -Version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ─── Logging helpers ────────────────────────────────────────────────────────

function Write-Banner {
    $line = '─' * 54
    Write-Host ""
    Write-Host "  $line" -ForegroundColor DarkCyan
    Write-Host "   TUSK  •  CMake Configure  •  Visual Studio 2022" -ForegroundColor Cyan
    Write-Host "  $line" -ForegroundColor DarkCyan
    Write-Host ""
}

function Write-Step {
    param([string]$Message)
    Write-Host "  ► " -ForegroundColor DarkYellow -NoNewline
    Write-Host $Message -ForegroundColor Yellow
}

function Write-Success {
    param([string]$Message)
    Write-Host "  ✔ " -ForegroundColor Green -NoNewline
    Write-Host $Message -ForegroundColor Green
}

function Write-Failure {
    param([string]$Message)
    Write-Host ""
    Write-Host "  ✘ " -ForegroundColor Red -NoNewline
    Write-Host $Message -ForegroundColor Red
    Write-Host ""
}

# ─── Core steps ─────────────────────────────────────────────────────────────

function Ensure-BuildDirectory {
    param([string]$Path)
    if (-not (Test-Path $Path)) {
        Write-Step "Creating build directory: $Path"
        New-Item -ItemType Directory -Path $Path | Out-Null
        Write-Success "Build directory created."
    }
    else {
        Write-Step "Build directory already exists: $Path"
    }
}

function Invoke-CMakeConfigure {
    param([string]$SourceDir)
    Write-Step "Running: cmake --preset windows-vs2022"
    Push-Location $SourceDir
    try {
        cmake --preset windows-vs2022
        if ($LASTEXITCODE -ne 0) {
            throw "cmake exited with code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

# ─── Entry point ────────────────────────────────────────────────────────────

function Main {
    $scriptDir = Split-Path -Parent $MyInvocation.ScriptName
    $buildDir = Join-Path $scriptDir 'build'

    Write-Banner

    try {
        Ensure-BuildDirectory -Path $buildDir
        Write-Host ""
        Invoke-CMakeConfigure -SourceDir $scriptDir
        Write-Host ""
        Write-Success "Configuration complete."
        Write-Host "  ►  Open " -ForegroundColor DarkGray -NoNewline
        Write-Host "win\build\tusk.sln" -ForegroundColor White -NoNewline
        Write-Host " in Visual Studio 2022 or a newer version." -ForegroundColor DarkGray
        Write-Host " retarget all projects to use latest SDK as required." -ForegroundColor DarkGray
        Write-Host ""
    }
    catch {
        Write-Failure "Configuration failed: $_"
        exit 1
    }
}

Main
