#!/usr/bin/env python
# Minimal working Natron render test

def createInstance(app, group):
    """Minimal node graph - just Ramp to Write (no plugin)"""
    
    print("\n[INFO] Creating minimal Natron graph...")
    
    # Create Ramp
    ramp = app.createNode("net.sf.openfx.Ramp")
    ramp.setScriptName("Ramp1")
    print("[OK] Ramp created")
    
    # Create Write
    writer = app.createNode("fr.inria.openfx.WriteOIIO")
    writer.setScriptName("Writer1")
    print("[OK] Write created")
    
    # Connect
    writer.connectInput(0, ramp)
    print("[OK] Connected Ramp -> Write")
    
    # Set output
    try:
        filename = writer.getParam("filename")
        if filename:
            filename.setValue("natron_benchmark_results/minimal_####.png")
            print("[OK] Output set to minimal_####.png")
    except:
        pass
    
    print("[OK] Graph ready\n")

print("[INFO] Minimal test loaded")
