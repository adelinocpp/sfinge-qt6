#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

DEPLOY_DIR="sfinge-deploy"
BINARY="$PROJECT_DIR/build/sfinge"

echo "=== SFINGE-Qt6 Deploy Script ==="

# Verificar se o binário existe
if [ ! -f "$BINARY" ]; then
    echo "Error: Binary not found. Run 'cmake --build build' first."
    exit 1
fi

# Limpar e criar diretório de deploy
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR/lib"
mkdir -p "$DEPLOY_DIR/plugins/platforms"
mkdir -p "$DEPLOY_DIR/plugins/imageformats"

# Copiar binário
cp "$BINARY" "$DEPLOY_DIR/"

# Encontrar e copiar bibliotecas Qt
echo "Copying Qt libraries..."
for lib in $(ldd "$BINARY" | grep -E "Qt6|libicu|libxcb|libxkb|libGL|libEGL|libOpenGL" | awk '{print $3}' | grep -v "not"); do
    if [ -f "$lib" ]; then
        cp -L "$lib" "$DEPLOY_DIR/lib/" 2>/dev/null || true
    fi
done

# Copiar TODAS as dependências
echo "Copying all dependencies..."
for lib in $(ldd "$BINARY" | grep -v "not found" | awk '{print $3}' | grep -E "^/lib|^/usr"); do
    if [ -f "$lib" ]; then
        cp -L "$lib" "$DEPLOY_DIR/lib/" 2>/dev/null || true
    fi
done

# Encontrar diretório de plugins Qt
QT_PLUGIN_PATH=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo "/usr/lib/x86_64-linux-gnu/qt6/plugins")

# Copiar plugins de plataforma
if [ -d "$QT_PLUGIN_PATH/platforms" ]; then
    cp "$QT_PLUGIN_PATH/platforms/libqxcb.so" "$DEPLOY_DIR/plugins/platforms/" 2>/dev/null || true
    cp "$QT_PLUGIN_PATH/platforms/libqoffscreen.so" "$DEPLOY_DIR/plugins/platforms/" 2>/dev/null || true
fi

# Copiar plugins de imagem
if [ -d "$QT_PLUGIN_PATH/imageformats" ]; then
    cp "$QT_PLUGIN_PATH/imageformats/"*.so "$DEPLOY_DIR/plugins/imageformats/" 2>/dev/null || true
fi

# Criar script de execução
cat > "$DEPLOY_DIR/run.sh" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$SCRIPT_DIR/plugins/platforms"
"$SCRIPT_DIR/sfinge" "$@"
EOF
chmod +x "$DEPLOY_DIR/run.sh"

# Criar tarball
TARBALL="sfinge-qt6-linux-x86_64.tar.gz"
echo "Creating $TARBALL..."
tar -czf "$TARBALL" "$DEPLOY_DIR"

# Mover para deploy/
mkdir -p "$PROJECT_DIR/deploy"
mv "$TARBALL" "$PROJECT_DIR/deploy/"

# Mostrar tamanho
SIZE=$(du -h "$PROJECT_DIR/deploy/$TARBALL" | cut -f1)
echo ""
echo "=== Deploy Complete ==="
echo "Package: deploy/$TARBALL ($SIZE)"
echo ""
echo "To use on another machine:"
echo "  1. tar -xzf $TARBALL"
echo "  2. cd $DEPLOY_DIR"
echo "  3. ./run.sh --batch -n 10 -v 3 -o ./output -j 4"
