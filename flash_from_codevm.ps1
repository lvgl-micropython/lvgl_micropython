param(
    [string]$BuildCommand = 'python3 make.py esp32 clean BOARD=ESP32_GENERIC_S3 BOARD_VARIANT=SPIRAM_OCT --toml=display_configs/Gen4-ESP32-70CT/Gen4-ESP32-70CT.toml INDEV=ft5x46',
    [switch]$SkipBuild = $true,
    [switch]$SkipDownload = $false,
    [switch]$SkipFlash = $false,
    [switch]$SkipRepl = $false
)

$ErrorActionPreference = 'Stop'

$remoteHost = 'kmk0815@codevm'
$remoteProjectDir = '/home/kmk0815/work/micropython/lvgl_micropython'
$remoteFile = "${remoteHost}:$remoteProjectDir/build/lvgl_micropy_ESP32_GENERIC_S3-SPIRAM_OCT-16.bin"
$localFile = Join-Path $PSScriptRoot 'lvgl_micropy_ESP32_GENERIC_S3-SPIRAM_OCT-16.bin'
$flachPort = 'COM10'
$port = 'COM11'

Write-Host "[1/4] Running remote build on codevm..."
if (-not $SkipBuild) {
    $remoteBuildCmd = @(
        "cd $remoteProjectDir",
        $BuildCommand
    ) -join ' && '
    ssh $remoteHost $remoteBuildCmd
    if ($LASTEXITCODE -ne 0) {
        throw "Remote build failed with exit code $LASTEXITCODE"
    }
} else {
    Write-Host "[1/4] Skipping remote build."
}

Write-Host "[2/4] Downloading firmware via scp..."
if (-not $SkipDownload) {
    scp $remoteFile $localFile
    if ($LASTEXITCODE -ne 0) {
        throw "scp failed with exit code $LASTEXITCODE"
    }
} else {
    Write-Host "[2/4] Skipping firmware download."
}

if ((-not $SkipFlash) -and (-not (Test-Path $localFile))) {
    throw "Firmware file not found after download: $localFile"
}

Write-Host "[3/4] Flashing firmware with esptool..."
if (-not $SkipFlash) {
    esptool --chip esp32s3 -p $flachPort -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m --erase-all 0x0 $localFile
    if ($LASTEXITCODE -ne 0) {
        throw "esptool failed with exit code $LASTEXITCODE"
    }
} else {
    Write-Host "[3/4] Skipping flashing."
}

Write-Host "[4/4] Opening REPL via plink..."
if (-not $SkipRepl) {
    try {
        plink.exe -serial $port -sercfg 115200,8,n,1,N
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "plink exited with code $LASTEXITCODE. Download and flash are already done."
        }
    } catch {
        Write-Warning "plink returned an error. Download and flash are already done."
    }
} else {
    Write-Host "[4/4] Skipping REPL startup."
}
