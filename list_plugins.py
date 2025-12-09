#!/usr/bin/env python3
"""
List all available OFX plugins in Natron
"""
import NatronEngine

def createInstance(app, group):
    """List all plugins"""
    print("\n" + "=" * 70)
    print("Available OFX Plugins in Natron")
    print("=" * 70)
    
    # Try to get plugin list
    try:
        plugins = app.getPluginIDs()
        
        print("\nTotal plugins: {}".format(len(plugins)))
        print("\nSearching for 'Intensity' plugins:")
        print("-" * 70)
        
        intensity_plugins = [p for p in plugins if 'intensity' in p.lower() or 'profile' in p.lower()]
        
        if intensity_plugins:
            for plugin_id in intensity_plugins:
                print("  - {}".format(plugin_id))
        else:
            print("  No intensity/profile plugins found")
        
        print("\nAll plugins containing 'plotter' or 'plot':")
        print("-" * 70)
        plot_plugins = [p for p in plugins if 'plot' in p.lower()]
        for plugin_id in plot_plugins:
            print("  - {}".format(plugin_id))
        
    except Exception as e:
        print("[ERROR] Could not list plugins: {}".format(e))
    
    return False  # Don't actually render anything

if __name__ == "__main__":
    print("Run with: NatronRenderer list_plugins.py")
