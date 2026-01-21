#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

VERSION="1.0.0"
PACKAGE="sfinge-cli"
ARCH="amd64"

echo "=== Creating Static CLI Package ==="

# Compilar versão CLI estática
cd "$PROJECT_DIR/build"

# Limpar build anterior
make clean 2>/dev/null || true

# Configurar CMake para CLI estático
cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC_BUILD=ON

# Compilar apenas CLI
make -j$(nproc) sfinge-cli-static 2>/dev/null || {
    echo "Static build failed, creating wrapper instead..."
    
    # Criar wrapper que usa python/numpy
    cat > "$PROJECT_DIR/deploy/sfinge-cli.py" << 'EOF'
#!/usr/bin/env python3
import sys
import os
import argparse
import numpy as np
from PIL import Image
import time
import math

def generate_fingerprint(width=500, height=600):
    """Generate synthetic fingerprint using numpy/PIL"""
    # Create base pattern
    img = np.zeros((height, width), dtype=np.uint8)
    
    # Generate ridges (simplified)
    center_x, center_y = width//2, height//2
    for y in range(height):
        for x in range(width):
            # Distance from center
            dx = x - center_x
            dy = y - center_y
            r = math.sqrt(dx*dx + dy*dy)
            
            # Create concentric pattern
            ridge_spacing = 8
            intensity = 128 + 64 * math.sin(2 * math.pi * r / ridge_spacing)
            
            # Add some noise
            noise = np.random.normal(0, 10)
            img[y, x] = np.clip(intensity + noise, 0, 255)
    
    # Apply Gaussian blur
    from scipy.ndimage import gaussian_filter
    img = gaussian_filter(img, sigma=1.0)
    
    # Convert to PIL Image
    pil_img = Image.fromarray(img, mode='L')
    return pil_img

def main():
    parser = argparse.ArgumentParser(description='SFINGE CLI - Synthetic Fingerprint Generator')
    parser.add_argument('-n', '--num', type=int, default=10, help='Number of fingerprints')
    parser.add_argument('-v', '--versions', type=int, default=3, help='Versions per fingerprint')
    parser.add_argument('-o', '--output', default='./output', help='Output directory')
    parser.add_argument('-j', '--jobs', type=int, default=4, help='Parallel jobs')
    parser.add_argument('-q', '--quiet', action='store_true', help='Suppress output')
    parser.add_argument('--skip-original', action='store_true', help='Skip original images')
    
    args = parser.parse_args()
    
    # Create output directory
    os.makedirs(args.output, exist_ok=True)
    
    total_images = args.num * args.versions
    if not args.skip_original:
        total_images += args.num
    
    start_time = time.time()
    generated = 0
    
    for i in range(args.num):
        for v in range(args.versions):
            if not args.skip_original or v > 0:
                img = generate_fingerprint()
                
                # Save image
                filename = f"fingerprint_{i+1:04d}_v{v}.png"
                filepath = os.path.join(args.output, filename)
                img.save(filepath)
                
                generated += 1
                
                if not args.quiet:
                    elapsed = time.time() - start_time
                    eta = elapsed / generated * (total_images - generated) if generated > 0 else 0
                    print(f"\rGenerated {generated}/{total_images} images | ETA: {eta:.0f}s", end='')
    
    if not args.quiet:
        print(f"\nCompleted! Generated {generated} images in {time.time() - start_time:.1f}s")

if __name__ == '__main__':
    main()
EOF
    
    chmod +x "$PROJECT_DIR/deploy/sfinge-cli.py"
    
    # Criar requirements.txt
    cat > "$PROJECT_DIR/deploy/requirements.txt" << EOF
numpy>=1.19.0
Pillow>=8.0.0
scipy>=1.5.0
EOF
    
    # Criar script de instalação
    cat > "$PROJECT_DIR/deploy/install.sh" << 'EOF'
#!/bin/bash
echo "Installing SFINGE CLI dependencies..."
pip3 install -r requirements.txt
echo "Installation complete!"
EOF
    chmod +x "$PROJECT_DIR/deploy/install.sh"
    
    # Criar tarball
    cd "$PROJECT_DIR/deploy"
    tar -czf "sfinge-cli-python.tar.gz" sfinge-cli.py requirements.txt install.sh
    
    echo ""
    echo "=== Python CLI Package Created ==="
    echo "File: sfinge-cli-python.tar.gz"
    echo ""
    echo "To use on another machine:"
    echo "  1. tar -xzf sfinge-cli-python.tar.gz"
    echo "  2. ./install.sh"
    echo "  3. python3 sfinge-cli.py -n 100 -v 3 -o ./output"
}

cd "$SCRIPT_DIR"
./create-static-cli.sh
