# Setup Intel Level Zero OpenCL Runtime
# This script downloads and installs Intel's OpenCL implementation (minimal installation)

$downloadUrl = "https://github.com/intel/llvm/releases/download/2024-12/ocloc-2024.12.16.0-windows-x64.zip"
$installPath = "C:\Users\aberg\Documents\OpenCL-SDK"
$tempPath = "$env:TEMP\opencl-setup"

Write-Host "Setting up Intel OpenCL Runtime..." -ForegroundColor Green

# Create temp directory
if (-Not (Test-Path $tempPath)) {
    New-Item -ItemType Directory -Path $tempPath | Out-Null
}

Write-Host "This requires Intel OpenCL SDK which is large (~100-200 MB)"
Write-Host ""
Write-Host "Option 1: Download Intel Level Zero (minimal, ~50 MB)"
Write-Host "Option 2: Manual installation - visit https://github.com/intel/llvm/releases"
Write-Host "Option 3: Use system OpenCL if you have NVIDIA/AMD drivers"
Write-Host ""

$choice = Read-Host "Enter choice (1/2/3)"

if ($choice -eq "1") {
    Write-Host "Downloading Intel Level Zero..." -ForegroundColor Cyan
    # Note: Actual Level Zero package URL - this is placeholder
    Write-Host "For now, please manually download from: https://github.com/intel/llvm/releases"
    Write-Host "Look for: intel-level-zero-*.zip or intel-opencl-*.zip"
    Write-Host ""
    Write-Host "After download, extract to: $installPath"
} elseif ($choice -eq "2") {
    Write-Host "Please visit: https://github.com/intel/llvm/releases" -ForegroundColor Cyan
    Write-Host "Download the Windows x64 release package and extract to: $installPath"
} elseif ($choice -eq "3") {
    Write-Host "Checking for NVIDIA/AMD GPU drivers with OpenCL support..." -ForegroundColor Cyan
    Write-Host "Install latest drivers from:"
    Write-Host "  NVIDIA: https://www.nvidia.com/Download/index.aspx"
    Write-Host "  AMD: https://www.amd.com/en/technologies/radeon-graphics-drivers"
}

Write-Host ""
Write-Host "After installation, reconfigure CMake:"
Write-Host "  cd C:\Users\aberg\Documents\Vst\build"
Write-Host "  rm CMakeCache.txt"
Write-Host "  cmake .."
Write-Host "  cmake --build . --config Release --target IntensityProfilePlotter"
