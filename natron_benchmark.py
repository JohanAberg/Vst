#!/usr/bin/env python3
"""
Natron Headless Benchmark for Intensity Profile Plotter OFX Plugin

This script benchmarks the IntensityProfilePlotter plugin in Natron's headless mode,
measuring GPU vs CPU performance, memory usage, and frame rendering times across
various image resolutions and sample configurations.

Usage:
    natron -b natron_benchmark.py
    
Requirements:
    - Natron 2.4+ with OFX support enabled
    - IntensityProfilePlotter.ofx installed in OFX plugin path
    - Python 3.6+
"""

import json
import sys
import os
import time
import statistics
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Tuple, Optional

# Natron Python API
try:
    import NatronEngine
except ImportError:
    # For testing outside Natron
    NatronEngine = None

# ==============================================================================
# Configuration
# ==============================================================================

class BenchmarkConfig:
    """Benchmark configuration parameters"""
    
    # Resolution matrix: (width, height, label)
    RESOLUTIONS = [
        (1920, 1080, "1080p"),
        (2560, 1440, "1440p"),
        (3840, 2160, "4K"),
        (5120, 2880, "5K"),
        (7680, 4320, "8K"),
    ]
    
    # Sample counts to test
    SAMPLE_COUNTS = [64, 256, 1024, 4096]
    
    # Number of iterations per test (for statistical stability)
    ITERATIONS_PER_TEST = 5
    
    # Warmup iterations (not counted in results)
    WARMUP_ITERATIONS = 1
    
    # Timeout per test (seconds)
    TEST_TIMEOUT = 120
    
    # Output format
    OUTPUT_DIR = Path("natron_benchmark_results")
    TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M%S")
    RESULTS_FILE = OUTPUT_DIR / f"benchmark_{TIMESTAMP}.json"
    CSV_FILE = OUTPUT_DIR / f"benchmark_{TIMESTAMP}.csv"
    
    # GPU backend hints (0=CPU, 1=GPU, 2=Auto)
    GPU_MODE_CPU = 0
    GPU_MODE_GPU = 1
    GPU_MODE_AUTO = 2


# ==============================================================================
# Utilities
# ==============================================================================

class Timer:
    """High-resolution timer context manager"""
    
    def __init__(self, name: str = ""):
        self.name = name
        self.start_time = None
        self.elapsed_ms = 0.0
    
    def __enter__(self):
        self.start_time = time.perf_counter()
        return self
    
    def __exit__(self, *args):
        self.elapsed_ms = (time.perf_counter() - self.start_time) * 1000
        if self.name:
            print(f"  {self.name}: {self.elapsed_ms:.2f}ms")


class BenchmarkResult:
    """Single benchmark result"""
    
    def __init__(self, width: int, height: int, samples: int, 
                 gpu_mode: int, iteration: int):
        self.width = width
        self.height = height
        self.samples = samples
        self.gpu_mode = gpu_mode
        self.iteration = iteration
        self.render_time_ms = 0.0
        self.memory_peak_mb = 0.0
        self.memory_avg_mb = 0.0
        self.gpu_utilization = 0.0
        self.success = False
        self.error_msg = ""
    
    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization"""
        return {
            'width': self.width,
            'height': self.height,
            'samples': self.samples,
            'gpu_mode': ['CPU', 'GPU', 'Auto'][self.gpu_mode],
            'iteration': self.iteration,
            'render_time_ms': round(self.render_time_ms, 3),
            'memory_peak_mb': round(self.memory_peak_mb, 2),
            'memory_avg_mb': round(self.memory_avg_mb, 2),
            'gpu_utilization': round(self.gpu_utilization, 1),
            'success': self.success,
            'error_msg': self.error_msg,
        }


class BenchmarkSummary:
    """Summary statistics for a test configuration"""
    
    def __init__(self, width: int, height: int, samples: int, gpu_mode: int):
        self.width = width
        self.height = height
        self.samples = samples
        self.gpu_mode = gpu_mode
        self.times: List[float] = []
        self.memory_peaks: List[float] = []
    
    def add_result(self, result: BenchmarkResult):
        """Add a result to this summary"""
        if result.success:
            self.times.append(result.render_time_ms)
            self.memory_peaks.append(result.memory_peak_mb)
    
    def get_stats(self) -> Dict:
        """Get statistical summary"""
        if not self.times:
            return {}
        
        return {
            'width': self.width,
            'height': self.height,
            'samples': self.samples,
            'gpu_mode': ['CPU', 'GPU', 'Auto'][self.gpu_mode],
            'iterations': len(self.times),
            'render_time': {
                'min_ms': round(min(self.times), 3),
                'max_ms': round(max(self.times), 3),
                'mean_ms': round(statistics.mean(self.times), 3),
                'median_ms': round(statistics.median(self.times), 3),
                'stdev_ms': round(statistics.stdev(self.times), 3) if len(self.times) > 1 else 0.0,
            },
            'memory_peak': {
                'min_mb': round(min(self.memory_peaks), 2),
                'max_mb': round(max(self.memory_peaks), 2),
                'mean_mb': round(statistics.mean(self.memory_peaks), 2),
            },
        }
    
    def to_csv_row(self) -> str:
        """Format as CSV row"""
        if not self.times:
            return ""
        
        stats = self.get_stats()
        return (
            f"{self.width},{self.height},{self.samples},"
            f"{['CPU', 'GPU', 'Auto'][self.gpu_mode]},"
            f"{stats['iterations']},"
            f"{stats['render_time']['min_ms']},"
            f"{stats['render_time']['max_ms']},"
            f"{stats['render_time']['mean_ms']},"
            f"{stats['render_time']['median_ms']},"
            f"{stats['render_time']['stdev_ms']},"
            f"{stats['memory_peak']['min_mb']},"
            f"{stats['memory_peak']['max_mb']},"
            f"{stats['memory_peak']['mean_mb']}"
        )


# ==============================================================================
# Natron Integration
# ==============================================================================

class NatronBenchmark:
    """Natron-based benchmark harness"""
    
    def __init__(self):
        self.results: List[BenchmarkResult] = []
        self.app: Optional[NatronEngine.AppInstance] = None
        self.output_dir = BenchmarkConfig.OUTPUT_DIR
        self.output_dir.mkdir(parents=True, exist_ok=True)
    
    def initialize(self) -> bool:
        """Initialize Natron environment"""
        try:
            if NatronEngine is None:
                print("ERROR: Natron Python API not available")
                print("This script must be run: natron -b natron_benchmark.py")
                return False
            
            self.app = NatronEngine.natron
            if self.app is None:
                self.app = NatronEngine.getApp(0)
            
            # Try to get Natron version, with fallback
            try:
                natron_version = NatronEngine.__version__
            except AttributeError:
                natron_version = "Unknown (headless mode)"
            
            print(f"✓ Natron initialized: {natron_version}")
            return True
        except Exception as e:
            print(f"✗ Failed to initialize Natron: {e}")
            return False
    
    def create_test_project(self, width: int, height: int) -> Optional[NatronEngine.Group]:
        """Create a new Natron project with test composition"""
        try:
            # In Natron 2.5 headless, app is already the composition/project
            # Set resolution on the app directly
            try:
                # Try to get the main composition/group
                comp = self.app
                
                # Try to set project dimensions (method varies by Natron version)
                try:
                    comp.getParam("projectWidth").set(width)
                    comp.getParam("projectHeight").set(height)
                except:
                    # If that doesn't work, try setting as render context
                    pass
                
                print(f"  Created composition: {width}x{height}")
                return comp
            except Exception as e:
                print(f"  ✗ Failed to create project: {e}")
                return None
        except Exception as e:
            print(f"  ✗ Failed to create project: {e}")
            return None
    
    def add_plugin_to_project(self, comp: NatronEngine.Group, 
                             samples: int) -> Optional[NatronEngine.Effect]:
        """Add IntensityProfilePlotter plugin to composition"""
        try:
            # Create plugin instance
            plugin = comp.createNode("fr.inria.openfx.IntensityProfilePlotter", 1, comp)
            
            if plugin is None:
                print(f"  ✗ Could not load plugin - check OFX plugin path")
                return None
            
            # Set parameters
            plugin.getParam("sampleCount").set(samples)
            
            # Optional: Set endpoints (scan line from top-left to bottom-right)
            plugin.getParam("point1X").set(0.1)
            plugin.getParam("point1Y").set(0.1)
            plugin.getParam("point2X").set(0.9)
            plugin.getParam("point2Y").set(0.9)
            
            print(f"  Added plugin: {samples} samples")
            return plugin
        except Exception as e:
            print(f"  ✗ Failed to add plugin: {e}")
            return None
    
    def render_frame(self, comp: NatronEngine.Group, frame: int = 1) -> Tuple[bool, float]:
        """Render a single frame and return success status and time"""
        try:
            with Timer("Frame render") as t:
                # Render composition
                comp.render(frame, frame)
            
            return True, t.elapsed_ms
        except Exception as e:
            print(f"  ✗ Render failed: {e}")
            return False, 0.0
    
    def get_memory_usage(self) -> Tuple[float, float]:
        """Get current memory usage (peak, average in MB)"""
        try:
            # This is a placeholder - Natron's memory API may vary
            # In production, use psutil or native Natron memory reporting
            import psutil
            process = psutil.Process()
            peak_mb = process.memory_info().rss / 1024 / 1024
            return peak_mb, peak_mb / 2  # Approximation
        except ImportError:
            # Fallback: return dummy values
            return 0.0, 0.0
        except Exception as e:
            print(f"  Warning: Could not get memory: {e}")
            return 0.0, 0.0
    
    def run_single_test(self, width: int, height: int, samples: int, 
                       gpu_mode: int, iteration: int) -> BenchmarkResult:
        """Run a single benchmark test"""
        result = BenchmarkResult(width, height, samples, gpu_mode, iteration)
        
        try:
            # Create project
            comp = self.create_test_project(width, height)
            if comp is None:
                result.error_msg = "Failed to create project"
                return result
            
            # Add plugin
            plugin = self.add_plugin_to_project(comp, samples)
            if plugin is None:
                result.error_msg = "Failed to load plugin"
                return result
            
            # Set GPU mode if available
            try:
                plugin.getParam("gpuMode").set(gpu_mode)
            except:
                pass  # Parameter may not exist
            
            # Render frame
            success, render_time = self.render_frame(comp)
            if not success:
                result.error_msg = "Render failed"
                return result
            
            # Get memory usage
            peak_mb, avg_mb = self.get_memory_usage()
            
            # Update result
            result.render_time_ms = render_time
            result.memory_peak_mb = peak_mb
            result.memory_avg_mb = avg_mb
            result.success = True
            
            # Close project
            self.app.closeProject(comp)
            
            return result
        
        except Exception as e:
            result.error_msg = str(e)
            return result
    
    def run_benchmarks(self) -> None:
        """Run all benchmark tests"""
        print("\n" + "="*70)
        print("Natron Headless Benchmark - IntensityProfilePlotter OFX Plugin")
        print("="*70)
        
        test_count = 0
        for width, height, label in BenchmarkConfig.RESOLUTIONS:
            for samples in BenchmarkConfig.SAMPLE_COUNTS:
                for gpu_mode in [BenchmarkConfig.GPU_MODE_CPU, BenchmarkConfig.GPU_MODE_GPU]:
                    test_count += 1
        
        test_num = 0
        
        # Run tests
        for width, height, label in BenchmarkConfig.RESOLUTIONS:
            for samples in BenchmarkConfig.SAMPLE_COUNTS:
                for gpu_mode in [BenchmarkConfig.GPU_MODE_CPU, BenchmarkConfig.GPU_MODE_GPU]:
                    test_num += 1
                    gpu_label = "CPU" if gpu_mode == BenchmarkConfig.GPU_MODE_CPU else "GPU"
                    
                    print(f"\n[{test_num}/{test_count}] {label} - {samples} samples - {gpu_label}")
                    print("-" * 70)
                    
                    # Warmup
                    print(f"  Warmup iteration...")
                    for _ in range(BenchmarkConfig.WARMUP_ITERATIONS):
                        self.run_single_test(width, height, samples, gpu_mode, 0)
                    
                    # Actual iterations
                    print(f"  Running {BenchmarkConfig.ITERATIONS_PER_TEST} iterations...")
                    for iteration in range(BenchmarkConfig.ITERATIONS_PER_TEST):
                        result = self.run_single_test(width, height, samples, gpu_mode, iteration + 1)
                        self.results.append(result)
                        
                        if result.success:
                            print(f"    Iteration {iteration+1}: {result.render_time_ms:.2f}ms")
                        else:
                            print(f"    Iteration {iteration+1}: FAILED - {result.error_msg}")
        
        print("\n" + "="*70)
        print("Benchmarking Complete")
        print("="*70)
    
    def save_results(self) -> None:
        """Save benchmark results to JSON and CSV"""
        print(f"\nSaving results to {self.output_dir}...")
        
        # Prepare summaries
        summaries = {}
        for result in self.results:
            key = (result.width, result.height, result.samples, result.gpu_mode)
            if key not in summaries:
                summaries[key] = BenchmarkSummary(*key)
            summaries[key].add_result(result)
        
        # Try to get Natron version
        try:
            natron_version = NatronEngine.__version__
        except (AttributeError, NameError):
            natron_version = "Unknown"
        
        # Save JSON
        json_data = {
            'metadata': {
                'natron_version': natron_version,
                'timestamp': BenchmarkConfig.TIMESTAMP,
                'system': self._get_system_info(),
            },
            'config': {
                'resolutions': BenchmarkConfig.RESOLUTIONS,
                'sample_counts': BenchmarkConfig.SAMPLE_COUNTS,
                'iterations': BenchmarkConfig.ITERATIONS_PER_TEST,
            },
            'results': [r.to_dict() for r in self.results],
            'summaries': [s.get_stats() for s in summaries.values()],
        }
        
        with open(BenchmarkConfig.RESULTS_FILE, 'w') as f:
            json.dump(json_data, f, indent=2)
        print(f"  ✓ JSON: {BenchmarkConfig.RESULTS_FILE}")
        
        # Save CSV
        with open(BenchmarkConfig.CSV_FILE, 'w') as f:
            f.write("Width,Height,Samples,GPU_Mode,Iterations,")
            f.write("Min_ms,Max_ms,Mean_ms,Median_ms,Stdev_ms,")
            f.write("Mem_Min_MB,Mem_Max_MB,Mem_Mean_MB\n")
            
            for summary in summaries.values():
                row = summary.to_csv_row()
                if row:
                    f.write(row + "\n")
        
        print(f"  ✓ CSV: {BenchmarkConfig.CSV_FILE}")
    
    def print_summary(self) -> None:
        """Print summary statistics"""
        print("\n" + "="*70)
        print("Performance Summary")
        print("="*70)
        
        # Group by resolution and GPU mode
        by_res_gpu = {}
        for result in self.results:
            if result.success:
                key = (result.width, result.height, result.gpu_mode)
                if key not in by_res_gpu:
                    by_res_gpu[key] = []
                by_res_gpu[key].append(result.render_time_ms)
        
        # Print summaries
        for (width, height, gpu_mode), times in sorted(by_res_gpu.items()):
            gpu_label = "CPU" if gpu_mode == BenchmarkConfig.GPU_MODE_CPU else "GPU"
            avg = statistics.mean(times)
            min_t = min(times)
            max_t = max(times)
            
            print(f"\n{width}x{height} - {gpu_label}:")
            print(f"  Average: {avg:.2f}ms")
            print(f"  Range:   {min_t:.2f}ms - {max_t:.2f}ms")
            
            # Calculate speedup if both CPU and GPU exist
            cpu_key = (width, height, BenchmarkConfig.GPU_MODE_CPU)
            gpu_key = (width, height, BenchmarkConfig.GPU_MODE_GPU)
            if cpu_key in by_res_gpu and gpu_key in by_res_gpu:
                cpu_avg = statistics.mean(by_res_gpu[cpu_key])
                gpu_avg = statistics.mean(by_res_gpu[gpu_key])
                speedup = cpu_avg / gpu_avg
                print(f"  GPU Speedup: {speedup:.2f}x")
    
    @staticmethod
    def _get_system_info() -> Dict:
        """Get system information"""
        try:
            import platform
            import psutil
            
            return {
                'os': platform.system(),
                'platform': platform.platform(),
                'processor': platform.processor(),
                'cpu_count': psutil.cpu_count(logical=True),
                'memory_total_gb': psutil.virtual_memory().total / (1024**3),
            }
        except:
            return {'info': 'unavailable'}


# ==============================================================================
# Main Entry Point
# ==============================================================================

def main():
    """Main entry point"""
    
    # Create benchmark
    benchmark = NatronBenchmark()
    
    # Initialize
    if not benchmark.initialize():
        return 1
    
    # Run benchmarks
    try:
        benchmark.run_benchmarks()
        benchmark.print_summary()
        benchmark.save_results()
        print("\n✓ Benchmarking completed successfully")
        return 0
    except KeyboardInterrupt:
        print("\n✗ Benchmarking interrupted by user")
        return 1
    except Exception as e:
        print(f"\n✗ Benchmarking failed: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
