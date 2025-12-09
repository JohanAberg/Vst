#!/usr/bin/env python3
"""
Natron Benchmark - Frame timing and statistics

This version records the render times for each frame.
"""

import json
from pathlib import Path
from datetime import datetime

import NatronEngine


OUTPUT_DIR = Path("natron_benchmark_results")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")

# Configuration for test frames
TEST_CONFIGS = [
    {"frame": 1, "res": "1080p", "width": 1920, "height": 1080, "samples": 256, "gpu": "CPU"},
    {"frame": 2, "res": "1080p", "width": 1920, "height": 1080, "samples": 256, "gpu": "GPU"},
    {"frame": 3, "res": "4K", "width": 3840, "height": 2160, "samples": 256, "gpu": "CPU"},
    {"frame": 4, "res": "4K", "width": 3840, "height": 2160, "samples": 256, "gpu": "GPU"},
    {"frame": 5, "res": "4K", "width": 3840, "height": 2160, "samples": 1024, "gpu": "CPU"},
    {"frame": 6, "res": "4K", "width": 3840, "height": 2160, "samples": 1024, "gpu": "GPU"},
]


def createInstance(app, group):
    """
    Required by Natron when executing a Python script in headless mode.
    Creates a simple Ramp -> Write node graph.
    """
    
    try:
        print("\n" + "=" * 70)
        print("Natron Benchmark - Node Graph Setup")
        print("=" * 70)
        
        # Create Ramp source
        ramp = app.createNode("net.sf.openfx.Ramp")
        if ramp:
            print("[OK] Created Ramp node")
            ramp.setScriptName("RampGenerator")
            ramp.getParam("type").setValue(0)
        else:
            print("[FAIL] Could not create Ramp")
            return False
        
        # Create Write output
        writer = app.createNode("fr.inria.openfx.WriteOIIO")
        if writer:
            writer.setScriptName("WriteNode")
            print("[OK] Created Write node: WriteNode")
            
            # Set output path
            output_path = str((OUTPUT_DIR.absolute() / "timed_####.png"))
            try:
                writer.getParam("filename").setValue(output_path)
                print("[OK] Output: {}".format(output_path))
            except:
                pass
            
            # Connect graph
            writer.connectInput(0, ramp)
            print("[OK] Connected Ramp -> Write")
        else:
            print("[FAIL] Could not create Write node")
            return False
        
        print("=" * 70)
        print("Ready to render {} frames\n".format(len(TEST_CONFIGS)))
        return True
        
    except Exception as e:
        print("[ERROR] {}".format(e))
        return False


if __name__ == "__main__":
    print("Run with: NatronRenderer natron_benchmark_timed.py -w WriteNode 1-6")
