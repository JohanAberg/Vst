# CMake integration for Natron Benchmark

## Overview

This document describes how to integrate the Natron headless benchmark with your CMake build system.

## Installation

The benchmark is included in the root CMakeLists.txt and installed automatically.

### Automatic Installation

```bash
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --install .  # Installs benchmark to /usr/local/bin/natron_benchmark
```

### Manual Installation

```bash
# Copy benchmark script
cp natron_benchmark.py /usr/local/bin/
chmod +x /usr/local/bin/natron_benchmark

# Create wrapper script
sudo cp run_natron_benchmark.sh /usr/local/bin/natron_benchmark
chmod +x /usr/local/bin/natron_benchmark
```

## CMake Options

Add to root CMakeLists.txt:

```cmake
# Natron Benchmark Configuration
option(BUILD_NATRON_BENCHMARK "Build Natron headless benchmark" ON)
option(INSTALL_NATRON_BENCHMARK "Install benchmark scripts" ON)

if(BUILD_NATRON_BENCHMARK)
    # Install Python script
    if(INSTALL_NATRON_BENCHMARK)
        install(
            FILES natron_benchmark.py
            DESTINATION bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                       GROUP_READ GROUP_EXECUTE
                       WORLD_READ WORLD_EXECUTE
        )
        
        # Install launcher script for the platform
        if(WIN32)
            install(
                FILES run_natron_benchmark.bat
                DESTINATION bin
            )
        else()
            install(
                FILES run_natron_benchmark.sh
                DESTINATION bin
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                           GROUP_READ GROUP_EXECUTE
                           WORLD_READ WORLD_EXECUTE
            )
        endif()
    endif()
    
    # Install documentation
    install(
        FILES NATRON_BENCHMARK_README.md
        DESTINATION share/doc/natron_benchmark
    )
endif()
```

## Usage

### From Build Directory

```bash
# After building
cd build
../run_natron_benchmark.sh  # macOS/Linux
start ../run_natron_benchmark.bat  # Windows
```

### After Installation

```bash
# macOS/Linux
natron_benchmark
run_natron_benchmark.sh

# Windows
run_natron_benchmark.bat
```

### Command Line

```bash
# Direct execution
natron -b natron_benchmark.py

# With custom output directory
cd /tmp && natron -b /path/to/natron_benchmark.py
```

## Configuration via CMake

### Pre-build Configuration

Create `natron_benchmark_config.cmake` to customize before build:

```cmake
set(NATRON_BENCHMARK_RESOLUTIONS 
    "1920,1080,1080p"
    "3840,2160,4K"
)
set(NATRON_BENCHMARK_SAMPLES "256;1024")
set(NATRON_BENCHMARK_ITERATIONS 3)
```

## Integration with CI/CD

### GitHub Actions

```yaml
name: Benchmark

on:
  schedule:
    - cron: '0 2 * * 0'

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install dependencies
        run: |
          sudo apt install natron python3 python3-pip
          pip install psutil
      
      - name: Build
        run: |
          mkdir build && cd build
          cmake .. -DBUILD_NATRON_BENCHMARK=ON
          cmake --build .
      
      - name: Run benchmark
        run: |
          cd build
          natron -b ../natron_benchmark.py
      
      - name: Upload results
        uses: actions/upload-artifact@v3
        with:
          name: benchmark-results
          path: natron_benchmark_results/
```

## Troubleshooting

### "natron command not found"

```bash
# Add to PATH
export PATH="/Applications/Natron.app/Contents/MacOS:$PATH"  # macOS
export PATH="/opt/natron/bin:$PATH"  # Linux

# Or create symlink
sudo ln -s /Applications/Natron.app/Contents/MacOS/Natron /usr/local/bin/natron
```

### Python API errors

```bash
# Ensure Python 3 is available
python3 --version

# Install required packages
pip3 install psutil
```

### Permission denied

```bash
# Make scripts executable
chmod +x natron_benchmark.py run_natron_benchmark.sh
```

## Next Steps

1. **Build and install**: `cmake --install .`
2. **Run benchmark**: `natron_benchmark` (if installed)
3. **Analyze results**: Open CSV in spreadsheet application
4. **Integrate with CI**: Add to GitHub Actions workflow
