#!/bin/bash
# =============================================================================
# SFINGE-Qt6 CLI - Build script for headless servers (no GUI)
# Tested on: Debian 11 (bullseye), Ubuntu 20.04+
# =============================================================================

set -e

echo "=== SFINGE-Qt6 CLI Headless Build Script ==="
echo ""

# Check if running as root for apt commands
check_deps() {
    echo "[1/4] Checking dependencies..."
    
    # Check for Qt6
    if ! pkg-config --exists Qt6Core 2>/dev/null; then
        echo "Qt6 not found. Please install Qt6 development packages:"
        echo ""
        echo "  For Debian/Ubuntu:"
        echo "    sudo apt update"
        echo "    sudo apt install qt6-base-dev"
        echo ""
        echo "  For Fedora/RHEL:"
        echo "    sudo dnf install qt6-qtbase-devel"
        echo ""
        exit 1
    fi
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        echo "CMake not found. Please install cmake:"
        echo "  sudo apt install cmake"
        exit 1
    fi
    
    # Check for g++
    if ! command -v g++ &> /dev/null; then
        echo "g++ not found. Please install build-essential:"
        echo "  sudo apt install build-essential"
        exit 1
    fi
    
    echo "All dependencies found!"
}

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Build
build_cli() {
    echo ""
    echo "[2/4] Configuring build..."
    
    cd "$PROJECT_DIR"
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure with CMake
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    echo ""
    echo "[3/4] Building sfinge-cli..."
    
    # Detect number of cores
    NPROC=$(nproc 2>/dev/null || echo 4)
    echo "Using $NPROC parallel jobs"
    
    # Build only CLI target
    make sfinge-cli -j$NPROC
    
    echo ""
    echo "[4/4] Build complete!"
}

# Print usage
print_usage() {
    echo ""
    echo "=== Usage ==="
    echo ""
    echo "Run with offscreen platform (required for headless servers):"
    echo ""
    echo "  QT_QPA_PLATFORM=offscreen ./build/sfinge-cli [options]"
    echo ""
    echo "Example:"
    echo "  QT_QPA_PLATFORM=offscreen ./build/sfinge-cli -n 1000 -v 3 -o ./output -q -j 36"
    echo ""
    echo "Options:"
    echo "  -n, --num <count>       Number of fingerprints (default: 10)"
    echo "  -v, --versions <count>  Versions per fingerprint (default: 3)"
    echo "  -o, --output <dir>      Output directory (default: ./output)"
    echo "  -p, --prefix <name>     Filename prefix (default: fingerprint)"
    echo "  -s, --start <index>     Start index (default: 0)"
    echo "  -j, --jobs <count>      Parallel jobs (default: CPU cores)"
    echo "  --skip-original         Skip v0 (original) images"
    echo "  --no-mask               Disable elliptical mask"
    echo "  --save-params           Save parameters JSON"
    echo "  -q, --quiet             Suppress debug output"
    echo "  -h, --help              Show help"
    echo ""
    echo "=== Performance Tips ==="
    echo ""
    echo "For maximum performance on this server:"
    echo "  - Use -j with number of physical cores (not hyperthreads)"
    echo "  - Use -q to suppress debug output"
    echo "  - Ensure output directory is on fast storage"
    echo ""
}

# Main
check_deps
build_cli
print_usage

echo "=== Done! ==="
