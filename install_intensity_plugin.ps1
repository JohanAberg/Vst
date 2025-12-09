# Install OFX Intensity Profile Plotter (Support API)
# Builds (optional) and installs the IntensityProfilePlotter plugin into Program Files
# Requires Administrator privileges

$ErrorActionPreference = "Stop"

# Check admin
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: This script requires Administrator privileges!" -ForegroundColor Red
    Write-Host "Please run PowerShell as Administrator and try again." -ForegroundColor Yellow
    exit 1
}

Write-Host "=== Installing OFX Intensity Profile Plotter (Support API) ===" -ForegroundColor Green
$BuildDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
Write-Host "Installer Run Time: $BuildDate" -ForegroundColor Magenta

# Config
$PluginName = "IntensityProfilePlotter"
$BuildDir   = "build\ofx\Release"
$PluginBinary = Join-Path $BuildDir "$PluginName.ofx"
$InstallBaseDir = "C:\Program Files\Common Files\OFX\Plugins"
$InstallDir = Join-Path $InstallBaseDir "$PluginName.ofx.bundle"
$Win64Dir = Join-Path $InstallDir "Contents\Win64"
$ResourcesDir = Join-Path $InstallDir "Contents\Resources"

# Optional: set to $true to force a rebuild before install
$Rebuild = $true
$CMakeExe = "cmake.exe"  # assumes in PATH; adjust if needed

if ($Rebuild) {
    Write-Host "[1/4] Building plugin (Release)..." -ForegroundColor Yellow
    & $CMakeExe --build "build" --config Release --target $PluginName
    if ($LASTEXITCODE -ne 0) { Write-Host "ERROR: Build failed." -ForegroundColor Red; exit 1 }
} else {
    Write-Host "[1/4] Skipping build (Rebuild=$Rebuild)." -ForegroundColor Yellow
}

Write-Host "[2/4] Verifying plugin binary..." -ForegroundColor Yellow
if (-not (Test-Path $PluginBinary)) {
    Write-Host "ERROR: Plugin binary not found at '$PluginBinary'." -ForegroundColor Red
    exit 1
}
Write-Host "Plugin binary found: $PluginBinary" -ForegroundColor Green
Write-Host "File size: $((Get-Item $PluginBinary).Length) bytes" -ForegroundColor Green

Write-Host "[3/4] Creating bundle directories..." -ForegroundColor Cyan
if (-not (Test-Path $InstallBaseDir)) { New-Item -ItemType Directory -Force -Path $InstallBaseDir | Out-Null }
New-Item -ItemType Directory -Force -Path $Win64Dir | Out-Null
New-Item -ItemType Directory -Force -Path $ResourcesDir | Out-Null
Write-Host "Bundle path: $InstallDir" -ForegroundColor Green

Write-Host "[4/4] Copying plugin and Info.plist..." -ForegroundColor Cyan
Copy-Item $PluginBinary -Destination (Join-Path $Win64Dir "$PluginName.ofx") -Force

# Minimal Info.plist for Resolve
$InfoPlist = @"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$PluginName</string>
    <key>CFBundlePackageType</key>
    <string>BNDL</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleVersion</key>
    <string>2.0.0.14</string>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
</dict>
</plist>
"@
Set-Content -Path (Join-Path $InstallDir "Contents\Info.plist") -Value $InfoPlist -Encoding UTF8
Write-Host "Info.plist written." -ForegroundColor Green

Write-Host "=== Installation Complete ===" -ForegroundColor Green
Write-Host "Installed to: $InstallDir" -ForegroundColor Cyan
Write-Host "Tip: If Resolve still shows the old version, delete OFXPluginCacheV2.xml and restart Resolve." -ForegroundColor Yellow
