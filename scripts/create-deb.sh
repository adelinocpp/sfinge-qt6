#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

VERSION="1.0.0"
PACKAGE="sfinge-qt6"
ARCH="amd64"
BINARY="$PROJECT_DIR/build/sfinge"

echo "=== Creating Debian Package ==="

if [ ! -f "$BINARY" ]; then
    echo "Error: Binary not found. Run 'cmake --build build' first."
    exit 1
fi

# Criar estrutura do pacote
PKG_DIR="${PACKAGE}_${VERSION}_${ARCH}"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/share/applications"
mkdir -p "$PKG_DIR/usr/share/doc/$PACKAGE"

# Copiar binário
cp "$BINARY" "$PKG_DIR/usr/bin/sfinge"
chmod 755 "$PKG_DIR/usr/bin/sfinge"

# Criar arquivo de controle
cat > "$PKG_DIR/DEBIAN/control" << EOF
Package: $PACKAGE
Version: $VERSION
Section: graphics
Priority: optional
Architecture: $ARCH
Depends: libc6 (>= 2.34), libstdc++6 (>= 11)
Recommends: libqt6widgets6 (>= 6.2), libqt6gui6 (>= 6.2), libqt6core6 (>= 6.2)
Maintainer: SFinGe Team
Description: Synthetic Fingerprint Generator
 SFINGE-Qt6 generates realistic synthetic fingerprint images
 for research and testing purposes. Supports batch generation
 with parallel processing.
 .
 Note: Requires Qt6 libraries to run. Install qt6-base-dev or
 equivalent package if not already available.
EOF

# Criar .desktop
cat > "$PKG_DIR/usr/share/applications/sfinge.desktop" << EOF
[Desktop Entry]
Name=SFinGe-Qt6
Comment=Synthetic Fingerprint Generator
Exec=sfinge
Terminal=false
Type=Application
Categories=Graphics;Science;
EOF

# Criar README
cat > "$PKG_DIR/usr/share/doc/$PACKAGE/README" << EOF
SFINGE-Qt6 - Synthetic Fingerprint Generator

REQUIREMENTS:
  - Qt6 libraries (libqt6core6, libqt6gui6, libqt6widgets6)
  - Standard C++ library (libstdc++6)

INSTALL QT6 ON DEBIAN/UBUNTU:
  sudo apt-get update
  sudo apt-get install qt6-base-dev qt6-base-dev-tools

USAGE:
  sfinge                    # Launch GUI
  sfinge --batch [options]  # Batch generation (CLI)

Batch Options:
  -n, --num <count>       Number of fingerprints (default: 10)
  -v, --versions <count>  Versions per fingerprint (default: 3)
  -o, --output <dir>      Output directory (default: ./output)
  -j, --jobs <count>      Parallel jobs (default: CPU cores)
  -q, --quiet             Suppress debug output
  --skip-original         Skip v0 (original) images

Example:
  sfinge --batch -n 100 -v 3 -o ./output -j 8 -q
EOF

# Criar pacote com compressão xz (compatível com Debian mais antigo)
dpkg-deb --build -Z xz "$PKG_DIR"

# Mover para deploy/
mkdir -p "$PROJECT_DIR/deploy"
mv "${PKG_DIR}.deb" "$PROJECT_DIR/deploy/"

# Limpar
rm -rf "$PKG_DIR"

echo ""
echo "=== Package Created ==="
echo "File: deploy/${PKG_DIR}.deb"
echo ""
echo "Install on Debian/Ubuntu:"
echo "  1. sudo dpkg -i deploy/${PKG_DIR}.deb"
echo "  2. sudo apt-get install qt6-base-dev qt6-tools-dev"
echo ""
echo "Or install Qt6 first, then:"
echo "  sudo dpkg -i deploy/${PKG_DIR}.deb"
