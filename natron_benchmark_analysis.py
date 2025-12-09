#!/usr/bin/env python3
"""
Natron Benchmark Analysis Utilities

Post-processing and analysis tools for Natron benchmark results.
Generates charts, statistical summaries, and performance reports.

Usage:
    python3 natron_benchmark_analysis.py <results_file.json>
"""

import json
import sys
import statistics
from pathlib import Path
from typing import Dict, List, Tuple
from dataclasses import dataclass
from collections import defaultdict


@dataclass
class PerformanceMetric:
    """Single performance metric"""
    name: str
    value: float
    unit: str
    threshold: float = 0.0
    
    def is_good(self) -> bool:
        """Check if metric exceeds threshold"""
        if self.threshold <= 0:
            return True
        return self.value <= self.threshold  # Lower is better for time


class BenchmarkAnalyzer:
    """Analyze benchmark results and generate reports"""
    
    def __init__(self, results_file: Path):
        self.results_file = Path(results_file)
        self.data = None
        self.load_results()
    
    def load_results(self) -> None:
        """Load benchmark results from JSON"""
        with open(self.results_file, 'r') as f:
            self.data = json.load(f)
        print(f"✓ Loaded {len(self.data.get('results', []))} benchmark results")
    
    def analyze_cpu_vs_gpu(self) -> Dict:
        """Analyze CPU vs GPU performance differences"""
        analysis = {}
        
        # Group by resolution and sample count
        by_res_samples = defaultdict(lambda: {'cpu': [], 'gpu': []})
        
        for result in self.data.get('results', []):
            if not result['success']:
                continue
            
            key = (result['width'], result['height'], result['samples'])
            mode = 'cpu' if result['gpu_mode'] == 'CPU' else 'gpu'
            by_res_samples[key][mode].append(result['render_time_ms'])
        
        # Calculate speedup for each configuration
        speedups = []
        for (width, height, samples), times in by_res_samples.items():
            if times['cpu'] and times['gpu']:
                cpu_avg = statistics.mean(times['cpu'])
                gpu_avg = statistics.mean(times['gpu'])
                speedup = cpu_avg / gpu_avg
                speedups.append({
                    'resolution': f"{width}x{height}",
                    'samples': samples,
                    'cpu_avg_ms': round(cpu_avg, 2),
                    'gpu_avg_ms': round(gpu_avg, 2),
                    'speedup': round(speedup, 2),
                })
        
        # Sort by speedup
        speedups.sort(key=lambda x: x['speedup'], reverse=True)
        
        return {
            'configurations': speedups,
            'average_speedup': statistics.mean([s['speedup'] for s in speedups]) if speedups else 0,
            'best_speedup': speedups[0]['speedup'] if speedups else 0,
            'worst_speedup': speedups[-1]['speedup'] if speedups else 0,
        }
    
    def analyze_memory_usage(self) -> Dict:
        """Analyze memory usage patterns"""
        cpu_mem = []
        gpu_mem = []
        
        for result in self.data.get('results', []):
            if not result['success']:
                continue
            
            if result['gpu_mode'] == 'CPU':
                cpu_mem.append(result['memory_peak_mb'])
            else:
                gpu_mem.append(result['memory_peak_mb'])
        
        return {
            'cpu_memory': {
                'min_mb': round(min(cpu_mem), 2) if cpu_mem else 0,
                'max_mb': round(max(cpu_mem), 2) if cpu_mem else 0,
                'avg_mb': round(statistics.mean(cpu_mem), 2) if cpu_mem else 0,
            },
            'gpu_memory': {
                'min_mb': round(min(gpu_mem), 2) if gpu_mem else 0,
                'max_mb': round(max(gpu_mem), 2) if gpu_mem else 0,
                'avg_mb': round(statistics.mean(gpu_mem), 2) if gpu_mem else 0,
            },
        }
    
    def analyze_stability(self) -> Dict:
        """Analyze result stability (standard deviation)"""
        by_config = defaultdict(list)
        
        for result in self.data.get('results', []):
            if not result['success']:
                continue
            
            key = (result['width'], result['height'], result['samples'], result['gpu_mode'])
            by_config[key].append(result['render_time_ms'])
        
        instability = []
        for (width, height, samples, gpu_mode), times in by_config.items():
            if len(times) > 1:
                stdev = statistics.stdev(times)
                mean = statistics.mean(times)
                cv = (stdev / mean) * 100  # Coefficient of variation
                
                instability.append({
                    'config': f"{width}x{height} {samples}s {gpu_mode}",
                    'iterations': len(times),
                    'stdev_ms': round(stdev, 3),
                    'cv_percent': round(cv, 2),
                    'stable': cv < 10,  # < 10% CV is stable
                })
        
        instability.sort(key=lambda x: x['cv_percent'], reverse=True)
        
        return {
            'configurations': instability,
            'unstable_count': sum(1 for i in instability if not i['stable']),
            'total_count': len(instability),
        }
    
    def analyze_scaling(self) -> Dict:
        """Analyze how performance scales with resolution"""
        by_samples_gpu = defaultdict(list)
        
        for result in self.data.get('results', []):
            if result['gpu_mode'] != 'GPU' or not result['success']:
                continue
            
            pixels = result['width'] * result['height']
            by_samples_gpu[result['samples']].append({
                'pixels': pixels,
                'time_ms': result['render_time_ms'],
                'resolution': f"{result['width']}x{result['height']}",
            })
        
        scaling = {}
        for samples, data in by_samples_gpu.items():
            # Sort by pixel count
            data.sort(key=lambda x: x['pixels'])
            
            # Check if linear or sublinear scaling
            if len(data) >= 2:
                # Estimate scaling factor from first to last
                pixels_ratio = data[-1]['pixels'] / data[0]['pixels']
                time_ratio = data[-1]['time_ms'] / data[0]['time_ms']
                
                scaling[samples] = {
                    'results': data,
                    'pixels_ratio': round(pixels_ratio, 2),
                    'time_ratio': round(time_ratio, 2),
                    'scaling_factor': round(time_ratio / (pixels_ratio ** 0.5), 2),  # Expect sqrt scaling
                }
        
        return scaling
    
    def generate_report(self) -> str:
        """Generate comprehensive analysis report"""
        lines = []
        
        lines.append("=" * 80)
        lines.append("NATRON BENCHMARK ANALYSIS REPORT")
        lines.append("=" * 80)
        lines.append("")
        
        # Metadata
        metadata = self.data.get('metadata', {})
        lines.append("SYSTEM INFORMATION")
        lines.append("-" * 80)
        lines.append(f"  Natron Version: {metadata.get('natron_version', 'Unknown')}")
        lines.append(f"  Timestamp: {metadata.get('timestamp', 'Unknown')}")
        if 'system' in metadata:
            sys_info = metadata['system']
            lines.append(f"  OS: {sys_info.get('os', 'Unknown')}")
            lines.append(f"  CPU Count: {sys_info.get('cpu_count', 'Unknown')}")
            lines.append(f"  Memory: {sys_info.get('memory_total_gb', 'Unknown')} GB")
        lines.append("")
        
        # Configuration
        config = self.data.get('config', {})
        lines.append("TEST CONFIGURATION")
        lines.append("-" * 80)
        lines.append(f"  Resolutions: {len(config.get('resolutions', []))} tested")
        lines.append(f"  Sample Counts: {config.get('sample_counts', [])}")
        lines.append(f"  Iterations per Test: {config.get('iterations', 0)}")
        lines.append("")
        
        # Results overview
        results = self.data.get('results', [])
        successes = sum(1 for r in results if r['success'])
        lines.append("RESULTS OVERVIEW")
        lines.append("-" * 80)
        lines.append(f"  Total Tests: {len(results)}")
        lines.append(f"  Successful: {successes}")
        lines.append(f"  Failed: {len(results) - successes}")
        lines.append(f"  Success Rate: {(successes/len(results)*100):.1f}%")
        lines.append("")
        
        # CPU vs GPU Analysis
        lines.append("GPU ACCELERATION ANALYSIS")
        lines.append("-" * 80)
        cpu_gpu = self.analyze_cpu_vs_gpu()
        lines.append(f"  Average GPU Speedup: {cpu_gpu.get('average_speedup', 0):.2f}x")
        lines.append(f"  Best Speedup: {cpu_gpu.get('best_speedup', 0):.2f}x")
        lines.append(f"  Worst Speedup: {cpu_gpu.get('worst_speedup', 0):.2f}x")
        lines.append("")
        lines.append("  Top 5 Speedups:")
        for config in cpu_gpu['configurations'][:5]:
            lines.append(f"    {config['resolution']} @ {config['samples']} samples: "
                        f"{config['speedup']:.2f}x "
                        f"(CPU: {config['cpu_avg_ms']:.1f}ms, GPU: {config['gpu_avg_ms']:.1f}ms)")
        lines.append("")
        
        # Memory Analysis
        lines.append("MEMORY USAGE ANALYSIS")
        lines.append("-" * 80)
        memory = self.analyze_memory_usage()
        cpu_mem = memory['cpu_memory']
        gpu_mem = memory['gpu_memory']
        lines.append(f"  CPU Mode:")
        lines.append(f"    Min: {cpu_mem['min_mb']:.1f} MB")
        lines.append(f"    Max: {cpu_mem['max_mb']:.1f} MB")
        lines.append(f"    Avg: {cpu_mem['avg_mb']:.1f} MB")
        lines.append(f"  GPU Mode:")
        lines.append(f"    Min: {gpu_mem['min_mb']:.1f} MB")
        lines.append(f"    Max: {gpu_mem['max_mb']:.1f} MB")
        lines.append(f"    Avg: {gpu_mem['avg_mb']:.1f} MB")
        lines.append("")
        
        # Stability Analysis
        lines.append("RESULT STABILITY ANALYSIS")
        lines.append("-" * 80)
        stability = self.analyze_stability()
        lines.append(f"  Stable Configurations: {stability['total_count'] - stability['unstable_count']}"
                    f"/{stability['total_count']}")
        if stability['unstable_count'] > 0:
            lines.append("  Unstable (CV > 10%):")
            for config in stability['configurations']:
                if not config['stable']:
                    lines.append(f"    {config['config']}: CV = {config['cv_percent']:.1f}%")
        lines.append("")
        
        # Scaling Analysis
        lines.append("RESOLUTION SCALING ANALYSIS")
        lines.append("-" * 80)
        scaling = self.analyze_scaling()
        for samples, analysis in scaling.items():
            lines.append(f"  {samples} Samples:")
            lines.append(f"    Pixel Count Ratio: {analysis['pixels_ratio']:.2f}x")
            lines.append(f"    Time Ratio: {analysis['time_ratio']:.2f}x")
            lines.append(f"    Scaling Factor: {analysis['scaling_factor']:.2f}")
            if analysis['scaling_factor'] < 1.1:
                lines.append("    → Sub-linear scaling (GPU bound)")
            else:
                lines.append("    → Linear or super-linear scaling (CPU/memory bound)")
        lines.append("")
        
        # Recommendations
        lines.append("RECOMMENDATIONS")
        lines.append("-" * 80)
        
        if cpu_gpu.get('average_speedup', 0) < 1.5:
            lines.append("  ⚠ GPU acceleration provides minimal benefit (<1.5x)")
            lines.append("    → Consider CPU-only optimization or smaller sample counts")
        elif cpu_gpu.get('average_speedup', 0) > 5:
            lines.append("  ✓ GPU acceleration is highly beneficial (>5x)")
            lines.append("    → Consider always using GPU mode for performance-critical use cases")
        
        if stability['unstable_count'] > 0:
            lines.append("  ⚠ Some configurations show high variance")
            lines.append("    → Increase ITERATIONS_PER_TEST or check for system load")
        else:
            lines.append("  ✓ All configurations show stable results (<10% CV)")
        
        if gpu_mem['max_mb'] > 2000:
            lines.append("  ⚠ High GPU memory usage detected")
            lines.append("    → Consider reducing resolution or sample count for memory-constrained systems")
        
        lines.append("")
        lines.append("=" * 80)
        
        return "\n".join(lines)


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: python3 natron_benchmark_analysis.py <results_file.json>")
        print("")
        print("Example:")
        print("  python3 natron_benchmark_analysis.py natron_benchmark_results/benchmark_*.json")
        sys.exit(1)
    
    results_file = sys.argv[1]
    
    if not Path(results_file).exists():
        print(f"ERROR: File not found: {results_file}")
        sys.exit(1)
    
    try:
        analyzer = BenchmarkAnalyzer(results_file)
        report = analyzer.generate_report()
        print(report)
        
        # Save report
        report_file = Path(results_file).parent / "analysis_report.txt"
        with open(report_file, 'w') as f:
            f.write(report)
        print(f"\n✓ Report saved to: {report_file}")
        
        return 0
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
