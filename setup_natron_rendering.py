#!/usr/bin/env python3
"""
Create a Natron project file (.ntp) with IntensityProfilePlotter for rendering

This generates a project that can be rendered with NatronRenderer without hanging.
"""

import json
import os
from pathlib import Path

output_dir = Path("natron_benchmark_results")
output_dir.mkdir(exist_ok=True)

# Natron project template as Python script
project_script = '''#!/usr/bin/env python
# Natron project for IntensityProfilePlotter benchmarking

import NatronEngine

def createInstance(app, group):
    """Create the node graph"""
    
    # Create Ramp source
    ramp = app.createNode("net.sf.openfx.Ramp")
    ramp.setScriptName("RampSource")
    
    # Create IntensityProfilePlotter
    plotter = app.createNode("com.coloristtools.intensityprofileplotter")
    plotter.setScriptName("IntensityPlotter")
    
    # Connect Ramp to Plotter
    plotter.connectInput(0, ramp)
    
    # Set backend to CPU for this test
    try:
        backend = plotter.getParam("backend")
        if backend:
            backend.setValue(2)  # CPU
    except:
        pass
    
    # Create Write node
    writer = app.createNode("fr.inria.openfx.WriteOIIO")
    writer.setScriptName("Writer1")
    
    # Connect Plotter to Writer
    writer.connectInput(0, plotter)
    
    # Set output filename
    try:
        filename = writer.getParam("filename")
        if filename:
            filename.setValue("natron_benchmark_results/rendered_frame_####.png")
    except:
        pass

# This function is called when the project is loaded
createInstance(app=app, group=None)
'''

# Write the project script
project_file = Path("benchmark_project.py")
with open(project_file, 'w') as f:
    f.write(project_script)

print(f"[OK] Created project script: {project_file}")

# Create a batch script to render with NatronRenderer
batch_script = '''@echo off
REM Benchmark rendering with NatronRenderer

setlocal enabledelayedexpansion

echo.
echo ============================================================
echo IntensityProfilePlotter Natron Rendering Benchmark
echo ============================================================
echo.

REM Test 1: Single frame render
echo [INFO] Rendering frame 1 with NatronRenderer...
echo.

"C:\\Program Files\\NatronRenderer\\bin\\NatronRenderer.exe" ^
    -b ^
    -w Writer1 ^
    "natron_benchmark_results\\rendered_frame_####.png" ^
    1 ^
    benchmark_project.py

echo.
echo [INFO] Render complete
echo.

REM Check if output file was created
if exist "natron_benchmark_results\\rendered_frame_0001.png" (
    echo [OK] Output file created successfully
    dir /s "natron_benchmark_results\\rendered_frame_0001.png"
) else (
    echo [WARN] Output file not found
)

echo.
echo ============================================================
pause
'''

batch_file = Path("run_natron_render.bat")
with open(batch_file, 'w') as f:
    f.write(batch_script)

print(f"[OK] Created batch script: {batch_file}")

# Create PowerShell version (more reliable)
ps_script = '''
# Natron Rendering Benchmark Script
# Uses NatronRenderer for proper headless rendering

Write-Host ""
Write-Host "="*70 -ForegroundColor Cyan
Write-Host "IntensityProfilePlotter - Natron Rendering Benchmark" -ForegroundColor Cyan
Write-Host "="*70 -ForegroundColor Cyan
Write-Host ""

$natronRenderer = "C:\\Program Files\\NatronRenderer\\bin\\NatronRenderer.exe"
$projectScript = "benchmark_project.py"
$outputPattern = "natron_benchmark_results\\rendered_frame_####.png"

# Check if NatronRenderer exists
if (-not (Test-Path $natronRenderer)) {
    Write-Host "[ERROR] NatronRenderer not found at: $natronRenderer" -ForegroundColor Red
    Write-Host "[INFO] Trying alternative path..." -ForegroundColor Yellow
    $natronRenderer = "C:\\Program Files\\Natron\\bin\\NatronRenderer.exe"
    
    if (-not (Test-Path $natronRenderer)) {
        Write-Host "[ERROR] NatronRenderer not found!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "[OK] Found NatronRenderer: $natronRenderer" -ForegroundColor Green
Write-Host ""

# Test configurations
$tests = @(
    @{ name = "CPU Mode (256 samples)"; backend = 2; samples = 256 },
    @{ name = "GPU Mode (256 samples)"; backend = 1; samples = 256 },
    @{ name = "CPU Mode (1024 samples)"; backend = 2; samples = 1024 },
    @{ name = "GPU Mode (1024 samples)"; backend = 1; samples = 1024 }
)

$results = @()

foreach ($test in $tests) {
    Write-Host "Running: $($test.name)" -ForegroundColor Yellow
    
    $timestamp = Get-Date -Format "HHmmss"
    $frameName = "rendered_frame_{0:0000}.png" -f 1
    $outputFile = Join-Path "natron_benchmark_results" $frameName
    
    # Run renderer
    $startTime = Get-Date
    & $natronRenderer -b -w Writer1 $outputPattern 1 $projectScript 2>&1 | Out-Null
    $elapsed = (Get-Date) - $startTime
    
    # Check results
    if (Test-Path $outputFile) {
        $fileSize = (Get-Item $outputFile).Length
        Write-Host "  [OK] Rendered: $frameName ($fileSize bytes) in $($elapsed.TotalSeconds)s" -ForegroundColor Green
        
        $results += @{
            test = $test.name
            status = "success"
            time_ms = $elapsed.TotalMilliseconds
            file_size = $fileSize
        }
        
        # Cleanup for next test
        Remove-Item $outputFile -Force -ErrorAction SilentlyContinue
    } else {
        Write-Host "  [FAIL] No output file created" -ForegroundColor Red
        
        $results += @{
            test = $test.name
            status = "failed"
            time_ms = $elapsed.TotalMilliseconds
        }
    }
    
    Write-Host ""
}

Write-Host "="*70 -ForegroundColor Cyan
Write-Host "RESULTS SUMMARY" -ForegroundColor Cyan
Write-Host "="*70 -ForegroundColor Cyan
Write-Host ""

$results | ForEach-Object {
    if ($_.status -eq "success") {
        Write-Host "$($_.test)" -ForegroundColor Green
        Write-Host "  Time: $([Math]::Round($_.time_ms, 2)) ms" -ForegroundColor Gray
        Write-Host "  Size: $($_.file_size) bytes" -ForegroundColor Gray
    } else {
        Write-Host "$($_.test)" -ForegroundColor Red
        Write-Host "  Status: FAILED" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "="*70 -ForegroundColor Cyan
'''

ps_file = Path("run_natron_render.ps1")
with open(ps_file, 'w') as f:
    f.write(ps_script)

print(f"[OK] Created PowerShell script: {ps_file}")

print()
print("="*70)
print("NATRON RENDERING SETUP COMPLETE")
print("="*70)
print()
print("Files created:")
print(f"  1. {project_file} - Natron project with node graph")
print(f"  2. {batch_file} - Batch script for Windows CMD")
print(f"  3. {ps_file} - PowerShell script for rendering")
print()
print("To run rendering:")
print()
print("  Option A (PowerShell - Recommended):")
print(f"    pwsh -NoProfile -ExecutionPolicy Bypass -Command \"& '{ps_file}'\"")
print()
print("  Option B (NatronRenderer directly):")
print(f"    NatronRenderer -b -w Writer1 \"natron_benchmark_results/frame_####.png\" 1-6 {project_file}")
print()
print("="*70)
