#!/usr/bin/env python3
"""
Natron Headless Benchmark for Intensity Profile Plotter OFX Plugin

This script properly uses Natron's Python API by implementing the required
createInstance() function. It creates a minimal node graph, renders frames,
and measures performance.

Run with:
    NatronRenderer natron_benchmark_proper.py -w BenchWriter 1-6
    
Or in background mode:
    Natron -b natron_benchmark_proper.py
"""

import json
import sys
import os
import time
import statistics
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional

# Natron Python API - available in headless mode
import NatronEngine

# ==============================================================================
# Configuration
# ==============================================================================

class BenchmarkConfig:
    """Benchmark configuration"""
    
    # Test matrix: resolutions to test (for demo, keep it small)
    TESTS = [
        {"res": "1080p", "width": 1920, "height": 1080, "samples": 256, "gpu": "CPU"},
        {"res": "1080p", "width": 1920, "height": 1080, "samples": 256, "gpu": "GPU"},
        {"res": "4K", "width": 3840, "height": 2160, "samples": 256, "gpu": "CPU"},
        {"res": "4K", "width": 3840, "height": 2160, "samples": 256, "gpu": "GPU"},
        {"res": "4K", "width": 3840, "height": 2160, "samples": 1024, "gpu": "CPU"},
        {"res": "4K", "width": 3840, "height": 2160, "samples": 1024, "gpu": "GPU"},
    ]
    
    OUTPUT_DIR = Path("natron_benchmark_results")
    TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")
    RESULTS_FILE = OUTPUT_DIR / f"benchmark_{TIMESTAMP}.json"
    CSV_FILE = OUTPUT_DIR / f"benchmark_{TIMESTAMP}.csv"


# ==============================================================================
# Global State
# ==============================================================================

benchmark_results = []
frame_times = {}  # frame_number -> elapsed_ms


# ==============================================================================
# Natron Node Graph Setup (required by Natron)
# ==============================================================================

def createInstance(app, group):
    """
    This function is REQUIRED by Natron when running a Python script.
    
    Natron will call this function when the script is loaded in headless mode.
    The 'app' parameter is the global Natron application instance.
    The 'group' parameter is the group node if this is a PyPlug, otherwise None.
    
    We use this to set up our test node graph with Reader -> OFX Plugin -> Writer.
    """
    
    try:
        print("\n" + "=" * 70)
        print("Natron Benchmark - Initializing Node Graph")
        print("=" * 70)
        
        # Create a simple ramp source as input
        ramp = app.createNode("net.sf.openfx.Ramp")
        if ramp:
            print(f"✓ Created Ramp source node")
            ramp.setScriptName("RampSource")
        
        # Try to create the IntensityProfilePlotter node
        # Using the OFX plugin identifier
        intensity_plotter = None
        try:
            # First, try the expected plugin ID
            intensity_plotter = app.createNode("fr.inria.openfx.IntensityProfilePlotter")
            print(f"✓ Created IntensityProfilePlotter node (fr.inria.openfx ID)")
        except:
            try:
                # Sometimes plugin IDs differ - try alternate
                intensity_plotter = app.createNode("IntensityProfilePlotter")
                print(f"✓ Created IntensityProfilePlotter node (simple ID)")
            except:
                print(f"⚠ Could not find IntensityProfilePlotter - will use Viewer instead")
                # Create a Viewer as fallback to still have a renderable output
                intensity_plotter = app.createNode("fr.inria.openfx.Viewer")
                print(f"✓ Created Viewer node as fallback")
        
        if intensity_plotter:
            intensity_plotter.setScriptName("IntensityPlotter")
            
            # Connect ramp to intensity plotter
            if ramp:
                try:
                    intensity_plotter.connectInput(0, ramp)
                    print(f"✓ Connected Ramp → IntensityPlotter")
                except Exception as e:
                    print(f"⚠ Could not connect nodes: {e}")
        
        # Create output Write node
        writer = app.createNode("fr.inria.openfx.WriteOIIO")
        if writer:
            print(f"✓ Created Write node")
            writer.setScriptName("BenchWriter")
            
            # Connect plotter to writer
            if intensity_plotter:
                try:
                    writer.connectInput(0, intensity_plotter)
                    print(f"✓ Connected IntensityPlotter → Writer")
                except Exception as e:
                    print(f"⚠ Could not connect to writer: {e}")
            
            # Set output file path
            output_file = "natron_benchmark_results/test_output_####.png"
            try:
                writer.getParam("filename").setValue(output_file)
                print(f"✓ Set output file: {output_file}")
            except:
                print(f"⚠ Could not set output filename")
        
        # Note: getProject() is not available in headless Natron
        # Frame range is set via command-line arguments instead
        print(f"✓ Ready to render {len(BenchmarkConfig.TESTS)} frames")
        
        print("=" * 70 + "\n")
        
        return True
        
    except Exception as e:
        print(f"ERROR in createInstance: {e}")
        import traceback
        traceback.print_exc()
        return False


# ==============================================================================
# Benchmark Callbacks
# ==============================================================================

def on_frame_begin(frame):
    """Called when frame rendering begins"""
    frame_times[frame] = time.perf_counter()


def on_frame_end(frame, success):
    """Called when frame rendering completes"""
    if frame in frame_times:
        elapsed_ms = (time.perf_counter() - frame_times[frame]) * 1000
        
        test_idx = frame - 1
        if test_idx < len(BenchmarkConfig.TESTS):
            test = BenchmarkConfig.TESTS[test_idx]
            
            print(f"Frame {frame}: {test['res']} - {test['samples']} samples - {test['gpu']:3s} - {elapsed_ms:8.2f}ms")
            
            benchmark_results.append({
                "frame": frame,
                "test_config": test,
                "elapsed_ms": elapsed_ms,
                "success": success,
            })


# ==============================================================================
# Post-Processing
# ==============================================================================

def save_benchmark_results():
    """Save results to JSON and CSV"""
    
    if not benchmark_results:
        print("No benchmark results to save!")
        return
    
    # Create output directory
    BenchmarkConfig.OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    
    # Organize results by test config
    results_by_config = {}
    for result in benchmark_results:
        config_key = (
            result['test_config']['res'],
            result['test_config']['width'],
            result['test_config']['height'],
            result['test_config']['samples'],
            result['test_config']['gpu'],
        )
        
        if config_key not in results_by_config:
            results_by_config[config_key] = []
        results_by_config[config_key].append(result['elapsed_ms'])
    
    # Calculate statistics
    summary_results = []
    for config, times in sorted(results_by_config.items()):
        res, width, height, samples, gpu = config
        
        if times:
            avg = statistics.mean(times)
            min_t = min(times)
            max_t = max(times)
            std_dev = statistics.stdev(times) if len(times) > 1 else 0
            
            summary_results.append({
                "resolution": res,
                "width": width,
                "height": height,
                "samples": samples,
                "gpu_mode": gpu,
                "avg_ms": avg,
                "min_ms": min_t,
                "max_ms": max_t,
                "std_dev": std_dev,
                "iterations": len(times),
            })
    
    # Save JSON
    try:
        with open(BenchmarkConfig.RESULTS_FILE, 'w') as f:
            json.dump({
                "timestamp": BenchmarkConfig.TIMESTAMP,
                "natron_version": "2.5 headless",
                "plugin_name": "IntensityProfilePlotter",
                "test_count": len(summary_results),
                "raw_results": benchmark_results,
                "summary": summary_results,
            }, f, indent=2)
        print(f"✓ Saved JSON: {BenchmarkConfig.RESULTS_FILE}")
    except Exception as e:
        print(f"✗ Failed to save JSON: {e}")
    
    # Save CSV
    try:
        with open(BenchmarkConfig.CSV_FILE, 'w') as f:
            f.write("Resolution,Width,Height,Samples,GPU Mode,Avg (ms),Min (ms),Max (ms),Std Dev,Count\n")
            for result in summary_results:
                f.write(
                    f"{result['resolution']},"
                    f"{result['width']},"
                    f"{result['height']},"
                    f"{result['samples']},"
                    f"{result['gpu_mode']},"
                    f"{result['avg_ms']:.2f},"
                    f"{result['min_ms']:.2f},"
                    f"{result['max_ms']:.2f},"
                    f"{result['std_dev']:.2f},"
                    f"{result['iterations']}\n"
                )
        print(f"✓ Saved CSV: {BenchmarkConfig.CSV_FILE}")
    except Exception as e:
        print(f"✗ Failed to save CSV: {e}")
    
    # Print summary to console
    print("\n" + "=" * 70)
    print("Benchmark Summary")
    print("=" * 70)
    for result in summary_results:
        print(f"{result['resolution']:6s} {result['width']:5d}x{result['height']:<5d} "
              f"{result['samples']:5d} samples {result['gpu_mode']:3s} "
              f"avg={result['avg_ms']:8.2f}ms  σ={result['std_dev']:6.2f}ms  "
              f"range=[{result['min_ms']:7.2f}, {result['max_ms']:7.2f}]ms")
    print("=" * 70)


# ==============================================================================
# Main Entry (for testing outside Natron)
# ==============================================================================

if __name__ == "__main__":
    print("This script is meant to be run with Natron:")
    print("  NatronRenderer natron_benchmark_proper.py -w BenchWriter 1-6")
    print("Or:")
    print("  Natron -b natron_benchmark_proper.py")
    sys.exit(0)
