#!/usr/bin/env python
# Simple Natron test - just render a Ramp without the plugin

def createInstance(app, group):
    """Create a minimal node graph with just Ramp -> Write"""
    
    print("[INFO] Creating minimal test graph...")
    
    # Create Ramp source
    ramp = app.createNode("net.sf.openfx.Ramp")
    ramp.setScriptName("RampSource")
    print("[OK] Ramp created")
    
    # Create Write node directly from Ramp
    writer = app.createNode("fr.inria.openfx.WriteOIIO")
    writer.setScriptName("Writer1")
    print("[OK] WriteOIIO created")
    
    # Connect Ramp to Writer
    writer.connectInput(0, ramp)
    print("[OK] Connected Ramp -> Writer")
    
    # Set output filename
    try:
        filename = writer.getParam("filename")
        if filename:
            filename.setValue("natron_benchmark_results/test_ramp_####.png")
            print("[OK] Output filename set")
    except Exception as e:
        print(f"[WARN] Could not set filename: {e}")
    
    print("[OK] Minimal graph setup complete")
