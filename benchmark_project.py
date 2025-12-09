#!/usr/bin/env python
# Natron project for IntensityProfilePlotter benchmarking

def createInstance(app, group):
    """Create the node graph - REQUIRED by Natron"""
    
    print("[INFO] Creating node graph...")
    
    # Create Ramp source
    ramp = app.createNode("net.sf.openfx.Ramp")
    ramp.setScriptName("RampSource")
    print("[OK] Ramp created")
    
    # Create IntensityProfilePlotter
    plotter = app.createNode("com.coloristtools.intensityprofileplotter")
    plotter.setScriptName("IntensityPlotter")
    print("[OK] IntensityProfilePlotter created")
    
    # Connect Ramp to Plotter
    plotter.connectInput(0, ramp)
    print("[OK] Connected Ramp -> Plotter")
    
    # Set backend to CPU for this test
    try:
        backend = plotter.getParam("backend")
        if backend:
            backend.setValue(2)  # CPU
            print("[OK] Backend set to CPU")
    except Exception as e:
        print(f"[WARN] Could not set backend: {e}")
    
    # Create Write node
    writer = app.createNode("fr.inria.openfx.WriteOIIO")
    writer.setScriptName("Writer1")
    print("[OK] WriteOIIO created")
    
    # Connect Plotter to Writer
    writer.connectInput(0, plotter)
    print("[OK] Connected Plotter -> Writer")
    
    # Set output filename
    try:
        filename = writer.getParam("filename")
        if filename:
            filename.setValue("natron_benchmark_results/rendered_frame_####.png")
            print("[OK] Output filename set")
    except Exception as e:
        print(f"[WARN] Could not set filename: {e}")
    
    print("[OK] Node graph setup complete")
