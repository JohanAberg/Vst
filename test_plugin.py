#!/usr/bin/env python3
"""
Test IntensityProfilePlotter plugin loading
"""
import NatronEngine

def createInstance(app, group):
    """Test plugin creation with different IDs"""
    print("\n" + "=" * 70)
    print("Testing IntensityProfilePlotter Plugin IDs")
    print("=" * 70)
    
    test_ids = [
        "fr.inria.openfx.IntensityProfilePlotter",
        "IntensityProfilePlotter",
        "net.sf.openfx.IntensityProfilePlotter",
        "com.github.IntensityProfilePlotter",
    ]
    
    for plugin_id in test_ids:
        print("\nTrying: {}".format(plugin_id))
        try:
            node = app.createNode(plugin_id)
            if node:
                print("  [SUCCESS] Node created!")
                print("  Node type: {}".format(type(node)))
                print("  Script name: {}".format(node.getScriptName()))
                
                # Try to list parameters
                try:
                    params = node.getParams()
                    print("  Parameters: {} found".format(len(params)))
                    for param in params[:5]:  # First 5
                        print("    - {}".format(param.getScriptName()))
                except:
                    print("  Could not list parameters")
                
                return True
            else:
                print("  [FAIL] createNode returned None")
        except Exception as e:
            print("  [FAIL] {}".format(str(e)[:80]))
    
    print("\n" + "=" * 70)
    print("None of the plugin IDs worked")
    print("=" * 70)
    return False

if __name__ == "__main__":
    print("Run with: NatronRenderer test_plugin.py")
