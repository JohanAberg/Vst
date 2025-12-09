#!/usr/bin/env python
# Diagnostic - check if plugin is actually loaded and usable

import sys

def createInstance(app, group):
    """Diagnostic test"""
    
    print("\n" + "="*70)
    print("DIAGNOSTIC TEST - Plugin Loading and Connectivity")
    print("="*70 + "\n")
    
    # Test 1: Create Ramp
    print("[TEST 1] Creating Ramp source...")
    try:
        ramp = app.createNode("net.sf.openfx.Ramp")
        if ramp:
            print(f"  [OK] Ramp created: {ramp}")
            print(f"       Type: {type(ramp)}")
            ramp.setScriptName("RampSource")
        else:
            print("  [FAIL] Ramp creation returned None")
            sys.exit(1)
    except Exception as e:
        print(f"  [FAIL] Exception: {e}")
        sys.exit(1)
    
    # Test 2: Check Ramp outputs
    print("\n[TEST 2] Checking Ramp output clip...")
    try:
        # Try to get output info
        output_clip = ramp.getOutputByIndex(0)
        if output_clip:
            print(f"  [OK] Output clip exists")
        else:
            print("  [WARN] No output clip found")
    except Exception as e:
        print(f"  [WARN] Could not query output: {e}")
    
    # Test 3: Create IntensityProfilePlotter
    print("\n[TEST 3] Creating IntensityProfilePlotter...")
    try:
        plotter = app.createNode("com.coloristtools.intensityprofileplotter")
        if plotter:
            print(f"  [OK] Plugin created: {plotter}")
            print(f"       Type: {type(plotter)}")
            plotter.setScriptName("Plotter")
        else:
            print("  [FAIL] Plugin creation returned None")
            print("  [INFO] Trying alternate ID...")
            plotter = app.createNode("IntensityProfilePlotter")
            if plotter:
                print(f"  [OK] Alternate ID worked: {plotter}")
            else:
                print("  [FAIL] No alternate ID found")
                sys.exit(1)
    except Exception as e:
        print(f"  [FAIL] Exception: {e}")
        sys.exit(1)
    
    # Test 4: Connection
    print("\n[TEST 4] Connecting Ramp -> Plotter...")
    try:
        # Check plotter has inputs
        input_count = plotter.getInputsCount()
        print(f"  [INFO] Plotter has {input_count} input(s)")
        
        plotter.connectInput(0, ramp)
        print("  [OK] Connected successfully")
    except Exception as e:
        print(f"  [FAIL] Connection failed: {e}")
        sys.exit(1)
    
    # Test 5: Parameters
    print("\n[TEST 5] Checking plugin parameters...")
    try:
        # List available parameters
        params = plotter.getParams()
        print(f"  [INFO] Plugin has {len(list(params))} parameters")
        
        # Try to access backend param
        backend = plotter.getParam("backend")
        if backend:
            print(f"  [OK] Backend parameter found")
            backend.setValue(2)  # CPU
            print(f"  [OK] Set backend to CPU (2)")
        else:
            print("  [WARN] Backend parameter not found")
    except Exception as e:
        print(f"  [WARN] Parameter access failed: {e}")
    
    # Test 6: Write node
    print("\n[TEST 6] Creating Write node...")
    try:
        writer = app.createNode("fr.inria.openfx.WriteOIIO")
        if writer:
            print(f"  [OK] Write node created: {writer}")
            writer.setScriptName("Writer1")
        else:
            print("  [FAIL] Write node returned None")
            sys.exit(1)
    except Exception as e:
        print(f"  [FAIL] Exception: {e}")
        sys.exit(1)
    
    # Test 7: Connect to Write
    print("\n[TEST 7] Connecting Plotter -> Write...")
    try:
        writer.connectInput(0, plotter)
        print("  [OK] Connected successfully")
    except Exception as e:
        print(f"  [FAIL] Connection failed: {e}")
        sys.exit(1)
    
    # Test 8: Set output
    print("\n[TEST 8] Setting output filename...")
    try:
        filename = writer.getParam("filename")
        if filename:
            filename.setValue("natron_benchmark_results/diag_frame_####.png")
            print("  [OK] Output filename set")
        else:
            print("  [WARN] Filename parameter not found")
    except Exception as e:
        print(f"  [WARN] Could not set filename: {e}")
    
    # Test 9: Pre-render status
    print("\n[TEST 9] Node graph ready for render")
    print("  Graph structure:")
    print("    Ramp -> Plotter -> Write")
    print("  Backend: CPU")
    print("  Output: natron_benchmark_results/diag_frame_####.png")
    
    print("\n" + "="*70)
    print("DIAGNOSTIC COMPLETE - Graph ready to render")
    print("="*70 + "\n")
    
    # Return success
    return True

# Entry point
print("[INFO] Starting diagnostic...")
