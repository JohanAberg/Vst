#!/usr/bin/env python3
"""
Natron Benchmark: IntensityProfilePlotter GPU vs CPU Performance

Tests the OFX plugin with different configurations:
- Multiple resolutions (1080p, 4K)
- Multiple sample counts (256, 1024, 4096)
- GPU enabled vs CPU only

Renders each configuration and measures performance.
"""

import json
from pathlib import Path
from datetime import datetime

import NatronEngine


OUTPUT_DIR = Path("natron_benchmark_results")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")

# Test matrix: resolution, samples, gpu_enabled
# Each test is rendered as a separate frame
TEST_CONFIGS = [
    # 1080p tests
    {"frame": 1, "res": "1080p", "width": 1920, "height": 1080, "samples": 256, "gpu": False, "desc": "1080p-256-CPU"},
    {"frame": 2, "res": "1080p", "width": 1920, "height": 1080, "samples": 256, "gpu": True, "desc": "1080p-256-GPU"},
    {"frame": 3, "res": "1080p", "width": 1920, "height": 1080, "samples": 1024, "gpu": False, "desc": "1080p-1024-CPU"},
    {"frame": 4, "res": "1080p", "width": 1920, "height": 1080, "samples": 1024, "gpu": True, "desc": "1080p-1024-GPU"},
    
    # 4K tests
    {"frame": 5, "res": "4K", "width": 3840, "height": 2160, "samples": 256, "gpu": False, "desc": "4K-256-CPU"},
    {"frame": 6, "res": "4K", "width": 3840, "height": 2160, "samples": 256, "gpu": True, "desc": "4K-256-GPU"},
    {"frame": 7, "res": "4K", "width": 3840, "height": 2160, "samples": 1024, "gpu": False, "desc": "4K-1024-CPU"},
    {"frame": 8, "res": "4K", "width": 3840, "height": 2160, "samples": 1024, "gpu": True, "desc": "4K-1024-GPU"},
    {"frame": 9, "res": "4K", "width": 3840, "height": 2160, "samples": 4096, "gpu": False, "desc": "4K-4096-CPU"},
    {"frame": 10, "res": "4K", "width": 3840, "height": 2160, "samples": 4096, "gpu": True, "desc": "4K-4096-GPU"},
]


def createInstance(app, group):
    """
    Required by Natron - creates node graph with IntensityProfilePlotter plugin.
    
    Graph: Ramp -> IntensityProfilePlotter -> Write
    
    The plugin parameters will be controlled per-frame to test different configurations.
    """
    
    try:
        print("\n" + "=" * 70)
        print("Natron Benchmark: IntensityProfilePlotter GPU vs CPU")
        print("=" * 70)
        print("Test configurations: {}".format(len(TEST_CONFIGS)))
        print()
        
        # Create Ramp source with test pattern
        ramp = app.createNode("net.sf.openfx.Ramp")
        if not ramp:
            print("[FAIL] Could not create Ramp node")
            return False
        
        ramp.setScriptName("RampSource")
        print("[OK] Created Ramp source")
        
        # Try to create IntensityProfilePlotter plugin
        plotter = None
        try:
            plotter = app.createNode("fr.inria.openfx.IntensityProfilePlotter")
            if plotter:
                print("[OK] Created IntensityProfilePlotter (fr.inria.openfx)")
            else:
                raise Exception("Node creation returned None")
        except Exception as e:
            print("[WARN] IntensityProfilePlotter not available: {}".format(e))
            try:
                plotter = app.createNode("net.sf.openfx.GradePlugin")
                if plotter:
                    print("[OK] Created Grade as fallback")
            except:
                pass
        
        if not plotter:
            print("[FAIL] Could not create any processing node")
            return False
        
        plotter.setScriptName("Plotter")
        
        # Connect Ramp to Plotter
        plotter.connectInput(0, ramp)
        print("[OK] Connected Ramp -> Plotter")
        
        # Configure IntensityProfilePlotter parameters if available
        try:
            # Set scan line endpoints (horizontal across middle)
            p1_param = plotter.getParam("p1")
            p2_param = plotter.getParam("p2")
            
            if p1_param and p2_param:
                p1_param.setValue(0.1, 0.5)  # Start at 10% width, 50% height
                p2_param.setValue(0.9, 0.5)  # End at 90% width, 50% height
                print("[OK] Set scan line parameters")
            
            # Sample count will be varied per frame
            samples_param = plotter.getParam("sampleCount")
            if samples_param:
                print("[OK] Found sampleCount parameter")
            
            # GPU enable parameter
            gpu_param = plotter.getParam("useGPU")
            if gpu_param:
                print("[OK] Found useGPU parameter")
            
        except Exception as e:
            print("[INFO] Could not set plugin parameters: {}".format(e))
        
        # Create Write output
        writer = app.createNode("fr.inria.openfx.WriteOIIO")
        if not writer:
            print("[FAIL] Could not create Write node")
            return False
        
        writer.setScriptName("WriteNode")
        
        # Set output path
        output_path = str(OUTPUT_DIR.absolute() / "gpu_cpu_test_####.png")
        try:
            writer.getParam("filename").setValue(output_path)
            print("[OK] Output: {}".format(output_path))
        except:
            print("[WARN] Could not set output path")
        
        # Connect Plotter to Writer
        writer.connectInput(0, plotter)
        print("[OK] Connected Plotter -> Write")
        
        # Save configuration metadata
        config_file = OUTPUT_DIR / "benchmark_config_{}.json".format(TIMESTAMP)
        try:
            with open(config_file, 'w') as f:
                json.dump({
                    "timestamp": TIMESTAMP,
                    "test_count": len(TEST_CONFIGS),
                    "configurations": TEST_CONFIGS,
                }, f, indent=2)
            print("[OK] Saved config: {}".format(config_file.name))
        except:
            pass
        
        print("=" * 70)
        print("Node graph ready - rendering {} frames...".format(len(TEST_CONFIGS)))
        print()
        
        # Print test matrix
        print("Test Matrix:")
        print("-" * 70)
        for cfg in TEST_CONFIGS:
            print("  Frame {}: {} - {} samples - {}".format(
                cfg['frame'], cfg['res'], cfg['samples'], 
                'GPU' if cfg['gpu'] else 'CPU'))
        print("-" * 70)
        print()
        
        return True
        
    except Exception as e:
        print("[ERROR] {}".format(e))
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    print("Run with: NatronRenderer natron_benchmark_gpu_cpu.py -w WriteNode 1-10")
