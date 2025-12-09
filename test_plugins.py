#!/usr/bin/env python3
"""
Debug test - check what plugins actually exist and work

Run with:
    Natron -b test_plugins.py
"""

import NatronEngine
import json
from pathlib import Path
from datetime import datetime

output_dir = Path("natron_benchmark_results")
output_dir.mkdir(exist_ok=True)

def createInstance(app, group):
    """Natron entry point"""
    
    results = {
        "timestamp": datetime.now().isoformat(),
        "tests": []
    }
    
    # Test 1: Try standard Ramp
    print("[TEST] Creating Ramp...")
    try:
        ramp = app.createNode("net.sf.openfx.Ramp")
        if ramp:
            print(f"[OK] Ramp created: {ramp}")
            results["tests"].append({"node": "Ramp", "status": "success", "id": str(ramp)})
        else:
            print("[FAIL] Ramp returned None")
            results["tests"].append({"node": "Ramp", "status": "failed", "reason": "returned_none"})
    except Exception as e:
        print(f"[FAIL] Ramp exception: {e}")
        results["tests"].append({"node": "Ramp", "status": "failed", "reason": str(e)})
    
    # Test 2: Try to list available nodes
    print("\n[TEST] Listing available OFX plugins...")
    try:
        # List some known plugins to see what works
        known_plugins = [
            "net.sf.openfx.Ramp",
            "fr.inria.openfx.Viewer",
            "fr.inria.openfx.WriteOIIO",
            "fr.inria.openfx.Read",
            "fr.inria.openfx.IntensityProfilePlotter",
            "IntensityProfilePlotter",
        ]
        
        for plugin_id in known_plugins:
            print(f"  Testing: {plugin_id}...", end=" ")
            try:
                node = app.createNode(plugin_id)
                if node:
                    print(f"OK (got {node})")
                    results["tests"].append({"node": plugin_id, "status": "created", "value": str(type(node))})
                else:
                    print("returned None")
                    results["tests"].append({"node": plugin_id, "status": "none"})
            except Exception as e:
                print(f"exception: {str(e)[:50]}")
                results["tests"].append({"node": plugin_id, "status": "exception", "error": str(e)[:100]})
    except Exception as e:
        print(f"[ERROR] Plugin enumeration failed: {e}")
        results["tests"].append({"test": "enumeration", "status": "failed", "error": str(e)})
    
    # Save results
    results_file = output_dir / "test_plugins_results.json"
    with open(results_file, 'w') as f:
        json.dump(results, f, indent=2)
    print(f"\n[OK] Results saved to {results_file}")

print("[INFO] Script loaded...")
