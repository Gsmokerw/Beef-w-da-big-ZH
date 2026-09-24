$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$baseDir = $scriptDir
$raylibZip = Join-Path $baseDir "raylib.zip"
$devkitZip = Join-Path $baseDir "w64devkit.zip"

Write-Host "Setting up build environment for Windows..."
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

if (-not (Test-Path (Join-Path $baseDir "raylib-5.0_win64_mingw-w64"))) {
    Write-Host "Downloading Raylib 5.0 (MinGW-w64)..."
    (New-Object System.Net.WebClient).DownloadFile("https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_mingw-w64.zip", $raylibZip)
    Write-Host "Extracting Raylib..."
    Expand-Archive -LiteralPath $raylibZip -DestinationPath $baseDir -Force
    Remove-Item -Force $raylibZip
} else {
    Write-Host "Raylib 5.0 already present."
}

if (-not (Test-Path (Join-Path $baseDir "w64devkit"))) {
    Write-Host "Downloading w64devkit..."
    (New-Object System.Net.WebClient).DownloadFile("https://github.com/skeeto/w64devkit/releases/download/v1.20.0/w64devkit-1.20.0.zip", $devkitZip)
    Write-Host "Extracting w64devkit..."
    Expand-Archive -LiteralPath $devkitZip -DestinationPath $baseDir -Force
    Remove-Item -Force $devkitZip
} else {
    Write-Host "w64devkit already present."
}

Write-Host "Windows build tools setup finished successfully!"
