#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

DEPLOY_DIR="sfinge-deploy-system"
BINARY="$PROJECT_DIR/build/sfinge"

echo "=== SFINGE-Qt6 System Deploy Script ==="

# Verificar se o binário existe
if [ ! -f "$BINARY" ]; then
    echo "Error: Binary not found. Run 'cmake --build build' first."
    exit 1
fi

# Limpar e criar diretório de deploy
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR/plugins/platforms"
mkdir -p "$DEPLOY_DIR/plugins/imageformats"

# Copiar binário
cp "$BINARY" "$DEPLOY_DIR/"

# Copiar apenas plugins Qt (não bibliotecas)
echo "Copying Qt plugins..."
QT_PLUGIN_PATH=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo "/usr/lib/x86_64-linux-gnu/qt6/plugins")

if [ -d "$QT_PLUGIN_PATH/platforms" ]; then
    cp "$QT_PLUGIN_PATH/platforms/libqoffscreen.so" "$DEPLOY_DIR/plugins/platforms/" 2>/dev/null || true
fi

if [ -d "$QT_PLUGIN_PATH/imageformats" ]; then
    cp "$QT_PLUGIN_PATH/imageformats/"*.so "$DEPLOY_DIR/plugins/imageformats/" 2>/dev/null || true
fi

# Criar script de execução que usa bibliotecas do sistema
cat > "$DEPLOY_DIR/run.sh" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$SCRIPT_DIR/plugins/platforms"
export QT_QPA_PLATFORM=offscreen
"$SCRIPT_DIR/sfinge" "$@"
EOF
chmod +x "$DEPLOY_DIR/run.sh"

# Criar tarball
TARBALL="sfinge-qt6-system-x86_64.tar.gz"
echo "Creating $TARBALL..."
tar -czf "$TARBALL" "$DEPLOY_DIR"

# Mostrar tamanho
SIZE=$(du -h "$TARBALL" | cut -f1)
echo ""
echo "=== Deploy Complete ==="
echo "Package: $TARBALL ($SIZE)"
echo ""
echo "REQUIRES Qt6 libraries installed on target system:"
echo "  sudo apt-get install libqt6core6 libqt6gui6 libqt6widgets6"
echo ""
echo "To use on another machine:"
echo "  1. Install Qt6 libraries first"
echo "  2. tar -xzf $TARBALL"
echo "  3. cd $DEPLOY_DIR"
echo "  4. ./run.sh --batch -n 100 -v 3 -o ./output -j 4"
