#!/bin/bash
# Natron Headless Benchmark Launcher for macOS/Linux
# This script launches the Natron benchmark in headless mode

set -e

echo ""
echo "================================"
echo "Natron Headless Benchmark Launcher"
echo "================================"
echo ""

# Detect OS
OS_TYPE="$(uname)"

# Find Natron executable
if command -v natron &> /dev/null; then
    NATRON_CMD="natron"
elif [ "$OS_TYPE" = "Darwin" ] && [ -x "/Applications/Natron.app/Contents/MacOS/Natron" ]; then
    NATRON_CMD="/Applications/Natron.app/Contents/MacOS/Natron"
else
    echo "ERROR: Natron not found in PATH"
    echo ""
    echo "Please ensure Natron is installed:"
    echo "  macOS: Download from https://natrongithub.github.io/"
    echo "  Linux: sudo apt install natron (or equivalent for your distro)"
    echo ""
    exit 1
fi

echo "Found Natron: $NATRON_CMD"
echo ""

# Check if benchmark script exists
if [ ! -f "natron_benchmark.py" ]; then
    echo "ERROR: natron_benchmark.py not found in current directory"
    echo ""
    echo "Current directory: $(pwd)"
    echo ""
    exit 1
fi

# Show configuration options
echo "Current Configuration:"
echo "  Resolution tests: 1080p, 1440p, 4K, 5K, 8K"
echo "  Sample counts: 64, 256, 1024, 4096"
echo "  GPU backends: CPU and GPU comparison"
echo "  Iterations: 5 per test"
echo ""

echo "Options:"
echo "  1. Run Full Benchmark (1-2 hours)"
echo "  2. Run Quick Benchmark (5-10 minutes)"
echo "  3. Edit Configuration and Run"
echo "  4. View Previous Results"
echo "  5. Exit"
echo ""

read -p "Enter your choice (1-5): " choice

case "$choice" in
    1)
        echo ""
        echo "Starting Full Benchmark..."
        echo "This may take 1-2 hours depending on your GPU."
        echo ""
        
        # Run benchmark
        "$NATRON_CMD" -b natron_benchmark.py
        
        show_results
        ;;
    
    2)
        echo ""
        echo "To run a quick benchmark, edit natron_benchmark.py:"
        echo "  1. Change ITERATIONS_PER_TEST from 5 to 2-3"
        echo "  2. Reduce RESOLUTIONS to [1920x1080, 3840x2160]"
        echo "  3. Reduce SAMPLE_COUNTS to [256, 1024]"
        echo ""
        read -p "Edit config now? (y/n): " edit
        if [ "$edit" = "y" ] || [ "$edit" = "Y" ]; then
            ${EDITOR:-nano} natron_benchmark.py
        fi
        
        echo ""
        echo "Starting Quick Benchmark..."
        "$NATRON_CMD" -b natron_benchmark.py
        
        show_results
        ;;
    
    3)
        echo ""
        echo "Opening natron_benchmark.py in editor..."
        ${EDITOR:-nano} natron_benchmark.py
        
        echo ""
        echo "Starting Benchmark..."
        "$NATRON_CMD" -b natron_benchmark.py
        
        show_results
        ;;
    
    4)
        echo ""
        if [ -d "natron_benchmark_results" ]; then
            echo "Available results:"
            ls -lh natron_benchmark_results/ | tail -n +2
            echo ""
            read -p "Enter filename to view (or press Enter to skip): " view_file
            if [ -n "$view_file" ]; then
                if [ -f "natron_benchmark_results/$view_file" ]; then
                    # Determine how to open based on file type
                    if [[ "$view_file" == *.json ]]; then
                        # Pretty-print JSON
                        echo ""
                        python3 -m json.tool "natron_benchmark_results/$view_file" | head -n 50
                        echo ""
                        echo "... (truncated, open file for full contents)"
                    elif [[ "$view_file" == *.csv ]]; then
                        # Show CSV with column headers
                        echo ""
                        head -n 10 "natron_benchmark_results/$view_file"
                        echo ""
                        echo "... (showing first 10 rows)"
                    fi
                else
                    echo "File not found: $view_file"
                fi
            fi
        else
            echo "No benchmark results found"
            echo "Run benchmark first (option 1 or 2)"
        fi
        ;;
    
    5)
        echo "Exiting."
        exit 0
        ;;
    
    *)
        echo "Invalid choice. Exiting."
        exit 1
        ;;
esac

echo ""
exit 0

# Function to show results after benchmark
show_results() {
    echo ""
    echo "================================"
    echo "Benchmark Complete!"
    echo "================================"
    echo ""
    
    if [ -d "natron_benchmark_results" ]; then
        echo "Results saved to: natron_benchmark_results/"
        echo ""
        
        # Find latest results
        latest_json=$(ls -t natron_benchmark_results/benchmark_*.json 2>/dev/null | head -n 1)
        
        if [ -n "$latest_json" ]; then
            echo "Latest results:"
            echo "  JSON: $(basename "$latest_json")"
            
            # Find corresponding CSV
            base_name=$(basename "$latest_json" .json)
            latest_csv="natron_benchmark_results/${base_name}.csv"
            
            if [ -f "$latest_csv" ]; then
                echo "  CSV:  $(basename "$latest_csv")"
                echo ""
                
                # Show summary from CSV
                echo "Summary (first 5 results):"
                head -n 6 "$latest_csv"
                echo ""
                
                # Offer to open CSV in default application
                if command -v xdg-open &> /dev/null; then
                    read -p "Open CSV in spreadsheet application? (y/n): " open_csv
                    if [ "$open_csv" = "y" ] || [ "$open_csv" = "Y" ]; then
                        xdg-open "$latest_csv" &
                    fi
                elif [ "$OS_TYPE" = "Darwin" ]; then
                    read -p "Open CSV in default application? (y/n): " open_csv
                    if [ "$open_csv" = "y" ] || [ "$open_csv" = "Y" ]; then
                        open "$latest_csv"
                    fi
                fi
            fi
        fi
    else
        echo "ERROR: Results directory not created"
    fi
    
    echo ""
}
