# Install Raw OFX API Intensity Profile Plotter Plugin (V3)
# This script builds and installs the IntensityProfilePlotterRaw plugin
# NOTE: This script requires Administrator privileges to install to Program Files

$ErrorActionPreference = "Stop"

# Check for administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script requires Administrator privileges!" -ForegroundColor Red
    Write-Host "Please run PowerShell as Administrator and try again." -ForegroundColor Yellow
    Write-Host "`nRight-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    exit 1
}

Write-Host "=== Building and Installing Raw OFX API Intensity Profile Plotter Plugin (V3) ===" -ForegroundColor Green

# Print Build Version
$BuildDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
Write-Host "Installer Run Time: $BuildDate" -ForegroundColor Magenta

# Configuration
$PluginName = "IntensityProfilePlotterRaw"
# CORRECT PATH: pointing to the subdirectory where CMake puts the binary
$BuildDir = "build_raw_api\ofx_raw_api\Release"
$InstallBaseDir = "C:\Program Files\Common Files\OFX\Plugins"
$InstallDir = "$InstallBaseDir\$PluginName.ofx.bundle"
$Win64Dir = "$InstallDir\Contents\Win64"
$ResourcesDir = "$InstallDir\Contents\Resources"
$PluginBinary = "$BuildDir\$PluginName.ofx"

# Path to cmake.exe
$cmakeExe = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if (-not (Test-Path $cmakeExe)) {
    Write-Host "Error: cmake.exe not found at '$cmakeExe'." -ForegroundColor Red
    Write-Host "Please ensure Visual Studio Build Tools are installed or update the \$cmakeExe path." -ForegroundColor Yellow
    exit 1
}

# Step 1: Build the plugin in Release mode
Write-Host "`n[1/4] Building plugin in Release mode (CLEAN BUILD)..." -ForegroundColor Yellow
& $cmakeExe --build build_raw_api --config Release --target $PluginName --clean-first
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed." -ForegroundColor Red
    exit 1
}
Write-Host "Plugin built successfully." -ForegroundColor Green

# Step 2: Verify plugin binary exists
Write-Host "`n[2/4] Verifying plugin binary..." -ForegroundColor Yellow
if (-not (Test-Path $PluginBinary)) {
    Write-Host "ERROR: Plugin binary not found at '$PluginBinary'." -ForegroundColor Red
    Write-Host "Current directory: $(Get-Location)" -ForegroundColor Gray
    Write-Host "Expected location: $PluginBinary" -ForegroundColor Gray
    exit 1
}
Write-Host "Plugin binary found: $PluginBinary" -ForegroundColor Green
Write-Host "File size: $( (Get-Item $PluginBinary).Length ) bytes" -ForegroundColor Green

# Step 3: Create bundle directory structure
Write-Host "`n[3/4] Creating bundle directory structure..." -ForegroundColor Cyan

# Create parent directory if needed
if (-not (Test-Path $InstallBaseDir)) {
    Write-Host "Creating base installation directory: $InstallBaseDir" -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $InstallBaseDir | Out-Null
}

New-Item -ItemType Directory -Force -Path $Win64Dir | Out-Null
New-Item -ItemType Directory -Force -Path $ResourcesDir | Out-Null
Write-Host "Bundle directories created: $InstallDir" -ForegroundColor Green

# Step 4: Copy plugin and create Info.plist
Write-Host "`n[4/4] Copying plugin and creating Info.plist..." -ForegroundColor Cyan
Copy-Item $PluginBinary -Destination "$Win64Dir\$PluginName.ofx" -Force
Write-Host "Plugin copied to: $Win64Dir\$PluginName.ofx" -ForegroundColor Green

# Create Info.plist
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
    <string>1.0.0</string>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
</dict>
</plist>
"@
Set-Content -Path "$InstallDir\Contents\Info.plist" -Value $InfoPlist -Encoding UTF8
Write-Host "Info.plist created at: $InstallDir\Contents\Info.plist" -ForegroundColor Green

Write-Host "`n=== Installation Complete! ===" -ForegroundColor Green
Write-Host "Plugin installed to: $InstallDir" -ForegroundColor Cyan
Write-Host "Remember to restart DaVinci Resolve." -ForegroundColor Yellow
Write-Host "`nNOTE: This is the V4 version with CLEAN BUILD and ON-SCREEN TIME." -ForegroundColor Yellow
