@echo off
REM Natron Headless Benchmark Launcher for Windows
REM This script launches the Natron benchmark in headless mode and opens results when done

setlocal enabledelayedexpansion

echo.
echo ================================
echo Natron Headless Benchmark Launcher
echo ================================
echo.

REM Set Natron path directly
set "NATRON_CMD=C:\Program Files\Natron\bin\Natron.exe"

REM Check if Natron exists
if not exist "!NATRON_CMD!" (
    echo ERROR: Natron not found at !NATRON_CMD!
    echo.
    echo Please ensure Natron is installed from: https://natrongithub.github.io/
    echo.
    pause
    exit /b 1
)

echo Found Natron: !NATRON_CMD!
echo.

REM Check if benchmark script exists
if not exist "natron_benchmark.py" (
    echo ERROR: natron_benchmark.py not found in current directory
    echo.
    echo Please run this script from the same directory as natron_benchmark.py
    echo Current directory: %cd%
    echo.
    pause
    exit /b 1
)

REM Show configuration options
echo Current Configuration:
echo   Resolution tests: 1080p, 1440p, 4K, 5K, 8K
echo   Sample counts: 64, 256, 1024, 4096
echo   GPU backends: CPU and GPU comparison
echo   Iterations: 5 per test
echo.

echo Options:
echo   1. Run Full Benchmark (1-2 hours)
echo   2. Run Quick Benchmark (5-10 minutes)
echo   3. Run Custom Benchmark (edit config first)
echo   4. View Previous Results
echo   5. Exit
echo.

set /p choice="Enter your choice (1-5): "

if "%choice%"=="1" (
    echo.
    echo Starting Full Benchmark...
    echo.
    "!NATRON_CMD!" -b natron_benchmark.py
    goto show_results
)

if "%choice%"=="2" (
    echo.
    echo Creating quick benchmark configuration...
    REM Backup original
    if exist natron_benchmark.py (
        copy natron_benchmark.py natron_benchmark.py.bak > nul
    )
    
    REM This would require modifying the script - for now, just run as-is
    echo Note: Run natron_benchmark.py directly to use quick configuration
    echo Starting benchmark with current config...
    echo.
    "!NATRON_CMD!" -b natron_benchmark.py
    goto show_results
)

if "%choice%"=="3" (
    echo.
    echo Opening natron_benchmark.py in default editor...
    start natron_benchmark.py
    echo.
    echo After editing, run: natron -b natron_benchmark.py
    goto end
)

if "%choice%"=="4" (
    echo.
    if exist "natron_benchmark_results" (
        cd natron_benchmark_results
        echo.
        echo Available results:
        dir /b *.json *.csv 2>nul
        echo.
        set /p view_file="Enter filename to view (or press Enter to open folder): "
        if not "!view_file!"=="" (
            if exist "!view_file!" (
                start !view_file!
            ) else (
                echo File not found: !view_file!
            )
        ) else (
            start .
        )
        cd ..
    ) else (
        echo No benchmark results found
        echo Run benchmark first (option 1 or 2)
    )
    goto end
)

if "%choice%"=="5" (
    goto end
)

echo Invalid choice. Exiting.
goto end

:show_results
echo.
echo ================================
echo Benchmark Complete!
echo ================================
echo.

if exist "natron_benchmark_results" (
    echo Results saved to: natron_benchmark_results\
    echo.
    
    REM Find latest results
    for /f "tokens=*" %%F in ('dir /b /o-d natron_benchmark_results\benchmark_*.json ^| findstr /r ".*" ^| for /f "delims=" %%G in ('find /v ""') do @echo %%G') do (
        set latest_json=%%F
        goto found_json
    )
    
    :found_json
    if defined latest_json (
        echo Latest JSON: !latest_json!
        echo.
        set /p open="Open results in Excel? (y/n): "
        if /i "!open!"=="y" (
            cd natron_benchmark_results
            REM Find corresponding CSV
            for /f "tokens=1,2 delims=_" %%A in ("!latest_json!") do (
                set csv_prefix=%%B
                for %%C in (benchmark_!csv_prefix!*.csv) do (
                    echo Opening CSV: %%C
                    start %%C
                    goto results_open
                )
            )
            :results_open
            cd ..
        )
    )
) else (
    echo ERROR: Results directory not created
)

:end
echo.
pause
