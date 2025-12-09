#!/usr/bin/env python3
"""
Simple Natron Benchmark - Direct rendering test
This script tests if the IntensityProfilePlotter plugin works in Natron.

Run with:
    Natron -b benchmark_simple.py
"""

import NatronEngine
import json
from pathlib import Path
from datetime import datetime

output_dir = Path("natron_benchmark_results")
output_dir.mkdir(exist_ok=True)

def createInstance(app, group):
    """Natron entry point"""
    print("\n[INFO] Natron Benchmark - Starting simple test")
    
    # Create a simple Ramp input
    print("[INFO] Creating Ramp node...")
    try:
        ramp = app.createNode("net.sf.openfx.Ramp")
        print("[OK] Ramp created")
    except Exception as e:
        print(f"[ERROR] Ramp failed: {e}")
        return
    
    # Create IntensityProfilePlotter
    print("[INFO] Creating IntensityProfilePlotter node...")
    try:
        plotter = app.createNode("fr.inria.openfx.IntensityProfilePlotter")
        print("[OK] IntensityProfilePlotter created (full ID)")
    except:
        try:
            plotter = app.createNode("IntensityProfilePlotter")
            print("[OK] IntensityProfilePlotter created (simple ID)")
        except Exception as e:
            print(f"[ERROR] IntensityProfilePlotter failed: {e}")
            return
    
    # Connect Ramp to Plotter
    print("[INFO] Connecting nodes...")
    try:
        plotter.connectInput(0, ramp)
        print("[OK] Nodes connected")
    except Exception as e:
        print(f"[WARN] Connection failed: {e}")
    
    # Set the backend parameter
    print("[INFO] Setting backend parameter...")
    try:
        backend = plotter.getParam("backend")
        if backend:
            backend.setValue(2)  # CPU mode
            print("[OK] Backend set to CPU (2)")
        else:
            print("[WARN] Backend parameter not found")
    except Exception as e:
        print(f"[WARN] Backend set failed: {e}")
    
    # Create output node
    print("[INFO] Creating output Write node...")
    try:
        writer = app.createNode("fr.inria.openfx.WriteOIIO")
        print("[OK] Write node created")
    except Exception as e:
        print(f"[ERROR] Write node failed: {e}")
        return
    
    # Connect plotter to writer
    print("[INFO] Connecting plotter to writer...")
    try:
        writer.connectInput(0, plotter)
        print("[OK] Plotter connected to writer")
    except Exception as e:
        print(f"[WARN] Writer connection failed: {e}")
    
    # Set output filename
    print("[INFO] Setting output filename...")
    try:
        output_file = writer.getParam("filename")
        if output_file:
            output_file.setValue(str(output_dir / "simple_test_####.png"))
            print(f"[OK] Output set to: natron_benchmark_results/simple_test_####.png")
    except Exception as e:
        print(f"[WARN] Output filename set failed: {e}")
    
    # Try to render frame 1
    print("[INFO] Attempting to render frame 1...")
    print("[INFO] (This may hang if the plugin render is blocking)")
    
    try:
        # Just try to render the writer node on frame 1
        success = app.render(writer, 1, 1)
        print(f"[OK] Render completed: {success}")
        
        # If we got here, write success to JSON
        results = {
            "status": "success",
            "timestamp": datetime.now().isoformat(),
            "test": "simple_render",
            "backend": "CPU"
        }
        
        results_file = output_dir / "benchmark_simple_results.json"
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"[OK] Results saved to {results_file}")
        
    except Exception as e:
        print(f"[ERROR] Render failed: {e}")
        
        # Write error results
        results = {
            "status": "failed",
            "timestamp": datetime.now().isoformat(),
            "test": "simple_render",
            "error": str(e)
        }
        
        results_file = output_dir / "benchmark_simple_results.json"
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"[OK] Error results saved to {results_file}")
    
    print("[INFO] Benchmark complete")

print("[INFO] Script loaded - waiting for Natron to call createInstance()...")
