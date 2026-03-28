Param(
    [string]$Version = "0.5.0"
)

$ErrorActionPreference = "Stop"

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " SkySweep32 Release Builder (v$Version)      " -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

$ProjectRoot = Split-Path -Path $PSScriptRoot -Parent
$ReleaseDir = Join-Path -Path $ProjectRoot -ChildPath "releases\v$Version"

If (!(Test-Path $ReleaseDir)) {
    New-Item -ItemType Directory -Force -Path $ReleaseDir | Out-Null
}

$PioExec = ""
$PioPrefixArgs = ""

# Strategy 1: Global 'pio'
try {
    $null = Get-Command "pio" -ErrorAction Stop
    $PioExec = "pio"
} catch {
    # Strategy 2: Local PlatformIO installation
    $PioFallback = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
    if (Test-Path $PioFallback) {
        $PioExec = $PioFallback
        Write-Host "[INFO] Automatically found PlatformIO at: $PioExec" -ForegroundColor Gray
    } else {
        # Strategy 3: Python (pip)
        try {
            $null = Get-Command "python" -ErrorAction Stop
            Write-Host "[INFO] 'pio' not found. Detected Python, attempting to use Python module..." -ForegroundColor Yellow
            $PioExec = "python"
            $PioPrefixArgs = "-m platformio"
            
            # Quick check if module exists, if not, install it
            $testPio = Start-Process -FilePath $PioExec -ArgumentList "$PioPrefixArgs --version" -Wait -NoNewWindow -PassThru
            if ($testPio.ExitCode -ne 0) {
                Write-Host "[INFO] Installing PlatformIO Core via pip..." -ForegroundColor Green
                Start-Process -FilePath $PioExec -ArgumentList "-m pip install -U platformio" -Wait -NoNewWindow
            }
        } catch {
            Write-Host "[ERROR] PlatformIO is missing, and Python is not installed." -ForegroundColor Red
            Write-Host "Please run this script from the VSCode PlatformIO Terminal (Alien Head icon at bottom)." -ForegroundColor Red
            exit 1
        }
    }
}

$Environments = @(
    @{ Name = "Starter"; EnvString = "esp32dev_base" },
    @{ Name = "Hunter"; EnvString = "esp32dev_standard" },
    @{ Name = "Sentinel"; EnvString = "esp32dev_pro" }
)

foreach ($env in $Environments) {
    Write-Host "`n[+] Building Tier: "$env.Name" ...`n" -ForegroundColor Yellow
    
    $PioArgs = ""
    if ($PioPrefixArgs -ne "") {
        $PioArgs += "$PioPrefixArgs "
    }
    $PioArgs += "run -e $($env.EnvString)"
    
    $process = Start-Process -FilePath $PioExec -ArgumentList $PioArgs -Wait -NoNewWindow -PassThru
    
    if ($process.ExitCode -eq 0) {
        $BinPath = Join-Path -Path $ProjectRoot -ChildPath ".pio\build\$($env.EnvString)\firmware.bin"
        if (Test-Path $BinPath) {
            $TargetFile = Join-Path -Path $ReleaseDir -ChildPath "SkySweep32_$($env.Name)_v$Version.bin"
            Copy-Item $BinPath -Destination $TargetFile -Force
            Write-Host "[OK] Saved: $TargetFile" -ForegroundColor Green
        } else {
            Write-Host "[ERROR] Could not find firmware.bin at: $BinPath" -ForegroundColor Red
        }
    } else {
        Write-Host "[ERROR] Build failed for $($env.Name). Please check compilation errors." -ForegroundColor Red
        exit 1
    }
}

Write-Host "`n=============================================" -ForegroundColor Cyan
Write-Host " All builds completed successfully!          " -ForegroundColor Cyan
Write-Host " Binaries are in the 'releases' folder.      " -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan
