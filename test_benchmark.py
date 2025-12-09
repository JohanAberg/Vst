#!/usr/bin/env python3
"""
Test runner for Natron benchmark with timeout and error checking
"""

import subprocess
import sys
import time
from pathlib import Path

def run_benchmark_with_timeout(timeout_seconds=45):
    """Run benchmark with timeout and error capture"""
    
    natron_exe = r"C:\Program Files\Natron\bin\NatronRenderer.exe"
    script = "natron_benchmark_timed.py"
    
    # Verify natron exists
    if not Path(natron_exe).exists():
        print("ERROR: Natron not found at {}".format(natron_exe))
        return False
    
    print("=" * 70)
    print("Starting Natron Benchmark (timeout: {}s)".format(timeout_seconds))
    print("=" * 70)
    print("Command: {} {} -w WriteNode 1-6\n".format(natron_exe, script))
    
    try:
        # Start the process
        process = subprocess.Popen(
            [natron_exe, script, "-w", "WriteNode", "1-6"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=Path(__file__).parent
        )
        
        # Wait for completion with timeout
        stdout, _ = process.communicate(timeout=timeout_seconds)
        
        print("OUTPUT:")
        print("-" * 70)
        print(stdout)
        print("-" * 70)
        
        # Analyze output for errors
        lines = stdout.split('\n')
        errors = [line for line in lines if 'ERROR' in line.upper() or 'FAILED' in line.upper()]
        
        if errors:
            print("\nERRORS FOUND:")
            for error in errors:
                print(f"  - {error}")
            return False
        
        if process.returncode != 0:
            print(f"\nProcess exited with code {process.returncode}")
            return False
        
        # Check for success indicators
        if "timed_" in stdout:
            print("\n[OK] Benchmark succeeded!")
            
            # List generated files
            output_dir = Path("natron_benchmark_results")
            if output_dir.exists():
                files = list(output_dir.glob("timed_*.png"))
                if files:
                    print("[OK] Generated {} output files:".format(len(files)))
                    for f in sorted(files):
                        size_kb = f.stat().st_size / 1024
                        print("  - {} ({:.1f} KB)".format(f.name, size_kb))
            
            return True
        else:
            print("\n[WARN] No output files found - check above for errors")
            return False
            
    except subprocess.TimeoutExpired:
        print("\n[TIMEOUT] Process did not complete within {} seconds".format(timeout_seconds))
        process.kill()
        return False
    except Exception as e:
        print("\n[ERROR] {}".format(e))
        return False

if __name__ == "__main__":
    success = run_benchmark_with_timeout(timeout_seconds=10)
    sys.exit(0 if success else 1)
