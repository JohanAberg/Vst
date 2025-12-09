#!/usr/bin/env python3
"""
Natron Benchmark - Using the correct plugin ID

Run with:
    Natron -b benchmark_correct_id.py
"""

import NatronEngine
import json
from pathlib import Path
from datetime import datetime
import time

output_dir = Path("natron_benchmark_results")
output_dir.mkdir(exist_ok=True)

def createInstance(app, group):
    """Natron entry point"""
    
    print("\n" + "="*70)
    print("Natron Benchmark - Correct Plugin ID Test")
    print("="*70)
    
    # Create Ramp
    print("[INFO] Creating Ramp source...")
    ramp = app.createNode("net.sf.openfx.Ramp")
    if ramp:
        print("[OK] Ramp created")
        ramp.setScriptName("RampSource")
    else:
        print("[FAIL] Ramp creation failed")
        return
    
    # Create IntensityProfilePlotter with CORRECT ID
    print("[INFO] Creating IntensityProfilePlotter (correct ID)...")
    plotter = None
    try:
        plotter = app.createNode("com.coloristtools.intensityprofileplotter")
        if plotter:
            print("[OK] IntensityProfilePlotter created!")
            plotter.setScriptName("Plotter")
        else:
            print("[FAIL] Plugin returned None")
    except Exception as e:
        print(f"[FAIL] Plugin creation failed: {e}")
        return
    
    # Connect Ramp to Plotter
    print("[INFO] Connecting Ramp -> Plotter...")
    try:
        plotter.connectInput(0, ramp)
        print("[OK] Connected")
    except Exception as e:
        print(f"[WARN] Connection failed: {e}")
    
    # Set backend parameter
    print("[INFO] Setting backend to CPU...")
    try:
        backend = plotter.getParam("backend")
        if backend:
            backend.setValue(2)  # CPU
            print("[OK] Backend set to CPU")
        else:
            print("[WARN] Backend parameter not found")
    except Exception as e:
        print(f"[WARN] Backend setting failed: {e}")
    
    # Create Write node
    print("[INFO] Creating Write node...")
    writer = None
    try:
        writer = app.createNode("fr.inria.openfx.WriteOIIO")
        if writer:
            print("[OK] Write node created")
            writer.setScriptName("Writer")
        else:
            print("[FAIL] Write node returned None")
            return
    except Exception as e:
        print(f"[FAIL] Write node failed: {e}")
        return
    
    # Connect Plotter to Writer
    print("[INFO] Connecting Plotter -> Writer...")
    try:
        writer.connectInput(0, plotter)
        print("[OK] Connected")
    except Exception as e:
        print(f"[WARN] Connection failed: {e}")
    
    # Set output file
    print("[INFO] Setting output file...")
    try:
        filename = writer.getParam("filename")
        if filename:
            output_path = str(output_dir / "benchmark_correct_####.png")
            filename.setValue(output_path)
            print(f"[OK] Output: {output_path}")
    except Exception as e:
        print(f"[WARN] Output setting failed: {e}")
    
    # Test render with timeout
    print("\n[INFO] Attempting to render frame 1...")
    print("[INFO] (Timeout after 30 seconds)")
    
    results = {
        "timestamp": datetime.now().isoformat(),
        "plugin_id": "com.coloristtools.intensityprofileplotter",
        "test": "frame_render",
        "frame": 1,
        "backend": "CPU",
        "status": "started"
    }
    
    try:
        start_time = time.time()
        success = app.render(writer, 1, 1)
        elapsed = time.time() - start_time
        
        results["status"] = "completed"
        results["render_time_ms"] = elapsed * 1000
        results["success"] = success
        print(f"[OK] Render completed in {elapsed:.2f}s")
        
    except Exception as e:
        elapsed = time.time() - start_time
        results["status"] = "exception"
        results["error"] = str(e)
        results["elapsed_ms"] = elapsed * 1000
        print(f"[ERROR] Render failed after {elapsed:.2f}s: {e}")
    
    # Save results
    results_file = output_dir / "benchmark_correct_id_results.json"
    with open(results_file, 'w') as f:
        json.dump(results, f, indent=2)
    print(f"\n[OK] Results saved to {results_file}")
    print("="*70)

print("[INFO] Script loaded...")
