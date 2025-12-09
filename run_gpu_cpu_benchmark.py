#!/usr/bin/env python3
"""
GPU vs CPU Benchmark Runner with Timing Analysis

Runs the benchmark and parses Natron's output to extract frame times.
"""

import subprocess
import re
import json
import statistics
from pathlib import Path
from datetime import datetime

def run_gpu_cpu_benchmark():
    """Run benchmark and capture timing data"""
    
    natron_exe = r"C:\Program Files\Natron\bin\NatronRenderer.exe"
    script = "natron_benchmark_gpu_cpu.py"
    
    if not Path(natron_exe).exists():
        print("ERROR: Natron not found at {}".format(natron_exe))
        return False
    
    print("=" * 70)
    print("GPU vs CPU Benchmark - Starting")
    print("=" * 70)
    print("Rendering 10 frames with different configurations...")
    print("Expected time: ~60-90 seconds\n")
    
    try:
        # Start process
        process = subprocess.Popen(
            [natron_exe, script, "-w", "WriteNode", "1-10"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=Path(__file__).parent
        )
        
        frame_times = {}
        current_frame = None
        frame_start_time = None
        
        # Read output line by line in real-time
        print("Progress:")
        print("-" * 70)
        
        for line in iter(process.stdout.readline, ''):
            if not line:
                break
            
            line = line.strip()
            
            # Parse frame progress lines
            # Format: "WriteNode ==> Frame: 1, Progress: 10.0%, 1.2 Fps, Time Remaining: 8 seconds"
            frame_match = re.search(r'Frame:\s*(\d+).*?(\d+\.\d+)\s*Fps', line)
            if frame_match:
                frame_num = int(frame_match.group(1))
                fps = float(frame_match.group(2))
                render_time = 1000.0 / fps if fps > 0 else 0  # Convert to ms
                
                if frame_num not in frame_times:
                    frame_times[frame_num] = render_time
                
                print("  Frame {}: {:.2f} ms/frame ({:.2f} FPS)".format(
                    frame_num, render_time, fps))
            
            # Check for completion
            if "Rendering finished" in line:
                print("\n[OK] Rendering completed!")
                break
        
        # Wait a bit for process to finish
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
        
        # Analyze results
        if frame_times:
            analyze_results(frame_times)
            return True
        else:
            print("\n[WARN] No timing data captured")
            return False
            
    except Exception as e:
        print("[ERROR] {}".format(e))
        return False


def analyze_results(frame_times):
    """Analyze and display benchmark results"""
    
    print("\n" + "=" * 70)
    print("Benchmark Results Analysis")
    print("=" * 70)
    
    # Test configurations (must match the script)
    configs = [
        {"frame": 1, "res": "1080p", "samples": 256, "gpu": False},
        {"frame": 2, "res": "1080p", "samples": 256, "gpu": True},
        {"frame": 3, "res": "1080p", "samples": 1024, "gpu": False},
        {"frame": 4, "res": "1080p", "samples": 1024, "gpu": True},
        {"frame": 5, "res": "4K", "samples": 256, "gpu": False},
        {"frame": 6, "res": "4K", "samples": 256, "gpu": True},
        {"frame": 7, "res": "4K", "samples": 1024, "gpu": False},
        {"frame": 8, "res": "4K", "samples": 1024, "gpu": True},
        {"frame": 9, "res": "4K", "samples": 4096, "gpu": False},
        {"frame": 10, "res": "4K", "samples": 4096, "gpu": True},
    ]
    
    # Group by configuration
    results_by_config = {}
    
    for cfg in configs:
        frame = cfg['frame']
        key = (cfg['res'], cfg['samples'])
        
        if key not in results_by_config:
            results_by_config[key] = {'cpu': None, 'gpu': None}
        
        if frame in frame_times:
            if cfg['gpu']:
                results_by_config[key]['gpu'] = frame_times[frame]
            else:
                results_by_config[key]['cpu'] = frame_times[frame]
    
    # Display comparison
    print("\nGPU vs CPU Performance:")
    print("-" * 70)
    print("{:10s} {:8s} {:>12s} {:>12s} {:>12s}".format(
        "Resolution", "Samples", "CPU (ms)", "GPU (ms)", "Speedup"))
    print("-" * 70)
    
    speedups = []
    
    for (res, samples), times in sorted(results_by_config.items()):
        cpu_time = times['cpu']
        gpu_time = times['gpu']
        
        if cpu_time and gpu_time:
            speedup = cpu_time / gpu_time
            speedups.append(speedup)
            
            print("{:10s} {:8d} {:>12.2f} {:>12.2f} {:>11.2f}x".format(
                res, samples, cpu_time, gpu_time, speedup))
        elif cpu_time:
            print("{:10s} {:8d} {:>12.2f} {:>12s} {:>12s}".format(
                res, samples, cpu_time, "N/A", "N/A"))
        elif gpu_time:
            print("{:10s} {:8d} {:>12s} {:>12.2f} {:>12s}".format(
                res, samples, "N/A", gpu_time, "N/A"))
    
    print("-" * 70)
    
    # Summary statistics
    if speedups:
        avg_speedup = statistics.mean(speedups)
        min_speedup = min(speedups)
        max_speedup = max(speedups)
        
        print("\nGPU Speedup Summary:")
        print("  Average: {:.2f}x faster".format(avg_speedup))
        print("  Range: {:.2f}x - {:.2f}x".format(min_speedup, max_speedup))
        
        if avg_speedup > 1.5:
            print("\n[OK] GPU acceleration is effective!")
        elif avg_speedup > 1.0:
            print("\n[INFO] GPU shows modest improvement")
        else:
            print("\n[WARN] GPU not faster than CPU - check drivers/implementation")
    
    # Save results
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_file = Path("natron_benchmark_results") / "gpu_cpu_results_{}.json".format(timestamp)
    
    try:
        with open(results_file, 'w') as f:
            json.dump({
                "timestamp": timestamp,
                "frame_times": frame_times,
                "configurations": configs,
                "results": results_by_config,
                "speedups": speedups,
                "avg_speedup": statistics.mean(speedups) if speedups else None,
            }, f, indent=2)
        print("\n[OK] Results saved: {}".format(results_file))
    except Exception as e:
        print("\n[WARN] Could not save results: {}".format(e))
    
    print("=" * 70)


if __name__ == "__main__":
    import sys
    success = run_gpu_cpu_benchmark()
    
    # Kill any lingering Natron processes
    import os
    if os.name == 'nt':
        os.system('taskkill /F /IM NatronRenderer.exe >nul 2>&1')
    
    sys.exit(0 if success else 1)
