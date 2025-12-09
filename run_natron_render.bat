@echo off
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

"C:\Program Files\NatronRenderer\bin\NatronRenderer.exe" ^
    -b ^
    -w Writer1 ^
    "natron_benchmark_results\rendered_frame_####.png" ^
    1 ^
    benchmark_project.py

echo.
echo [INFO] Render complete
echo.

REM Check if output file was created
if exist "natron_benchmark_results\rendered_frame_0001.png" (
    echo [OK] Output file created successfully
    dir /s "natron_benchmark_results\rendered_frame_0001.png"
) else (
    echo [WARN] Output file not found
)

echo.
echo ============================================================
pause
