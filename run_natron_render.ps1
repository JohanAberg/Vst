
# Natron Rendering Benchmark Script
# Uses NatronRenderer for proper headless rendering

Write-Host ""
Write-Host "="*70 -ForegroundColor Cyan
Write-Host "IntensityProfilePlotter - Natron Rendering Benchmark" -ForegroundColor Cyan
Write-Host "="*70 -ForegroundColor Cyan
Write-Host ""

$natronRenderer = "C:\Program Files\NatronRenderer\bin\NatronRenderer.exe"
$projectScript = "benchmark_project.py"
$outputPattern = "natron_benchmark_results\rendered_frame_####.png"

# Check if NatronRenderer exists
if (-not (Test-Path $natronRenderer)) {
    Write-Host "[ERROR] NatronRenderer not found at: $natronRenderer" -ForegroundColor Red
    Write-Host "[INFO] Trying alternative path..." -ForegroundColor Yellow
    $natronRenderer = "C:\Program Files\Natron\bin\NatronRenderer.exe"
    
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
