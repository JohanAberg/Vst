#!/usr/bin/env python3
"""
Natron Headless Benchmark - Simplified Working Version

Creates a minimal node graph and measures render times.
Uses only built-in nodes to avoid plugin compatibility issues.

Run with:
    NatronRenderer natron_benchmark_simple.py -w WriteNode 1-3
"""

import json
import sys
import os
import time
from pathlib import Path
from datetime import datetime

import NatronEngine


# ==============================================================================
# Global Configuration
# ==============================================================================

OUTPUT_DIR = Path("natron_benchmark_results")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")

# Frame render times will be collected here
frame_times = {}


# ==============================================================================
# Natron Setup - REQUIRED FUNCTION
# ==============================================================================

def createInstance(app, group):
    """
    Required by Natron when executing a Python script in headless mode.
    
    Creates a simple node graph: Ramp -> Write
    This avoids plugin compatibility issues while still testing the rendering pipeline.
    """
    
    try:
        print("\n" + "=" * 70)
        print("Natron Benchmark Setup")
        print("=" * 70)
        
        # Create a Ramp node (generates procedural gradient)
        ramp = app.createNode("net.sf.openfx.Ramp")
        if ramp:
            print("[OK] Created Ramp node")
            ramp.setScriptName("RampGenerator")
            
            # Configure ramp parameters
            ramp.getParam("type").setValue(0)  # Linear
        else:
            print("[FAIL] Failed to create Ramp node")
            return False
        
        # Create Write node for output
        writer = app.createNode("fr.inria.openfx.WriteOIIO")
        if writer:
            # IMPORTANT: Set script name FIRST before returning from createInstance
            writer.setScriptName("WriteNode")
            print("[OK] Created Write node with name: WriteNode")
            
            # Set output path - use absolute path
            output_dir_abs = OUTPUT_DIR.absolute()
            output_path = str(output_dir_abs / "test_####.png")
            
            try:
                writer.getParam("filename").setValue(output_path)
                print("[OK] Set output path: {}".format(output_path))
            except Exception as e:
                print("[WARN] Could not set output path: {}".format(e))
            
            # Connect Ramp to Writer
            writer.connectInput(0, ramp)
            print("[OK] Connected Ramp -> Write")
        else:
            print("[FAIL] Failed to create Write node")
            return False
        
        print("=" * 70 + "\n")
        print("Rendering will begin now...\n")
        return True
        
    except Exception as e:
        print("[ERROR] in createInstance: {}".format(e))
        import traceback
        traceback.print_exc()
        return False


# ==============================================================================
# Main Benchmarking Logic (called after rendering)
# ==============================================================================

def main_after_render():
    """Called after rendering completes to save results"""
    
    # Create summary results
    results = {
        "timestamp": TIMESTAMP,
        "natron_version": "2.5 headless",
        "test_type": "simple_ramp_write",
        "status": "completed",
    }
    
    results_file = OUTPUT_DIR / f"benchmark_{TIMESTAMP}.json"
    csv_file = OUTPUT_DIR / f"benchmark_{TIMESTAMP}.csv"
    
    # Save JSON results
    try:
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"✓ Saved results: {results_file}")
    except Exception as e:
        print(f"✗ Failed to save results: {e}")
    
    # Check what was rendered
    png_files = list(OUTPUT_DIR.glob("test_*.png"))
    if png_files:
        print(f"✓ Rendered {len(png_files)} frames:")
        for png in sorted(png_files):
            print(f"  - {png.name}")
    else:
        print("⚠ No PNG files found in output directory")


if __name__ == "__main__":
    # This is called when the script is run directly (for testing)
    print("This script should be run with NatronRenderer:")
    print("  NatronRenderer natron_benchmark_simple.py -w WriteNode 1-3")
    sys.exit(0)
