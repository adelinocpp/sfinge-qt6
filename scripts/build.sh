#!/bin/bash

# Script consolidado de build do SFINGE-Qt6
# Suporta múltiplas opções via linha de comando

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Valores padrão
BUILD_TYPE="Debug"
CLEAN_FIRST=false
RUN_AFTER=false
RUN_TESTS=false
VERBOSE=false
JOBS=""

# Função de ajuda
show_help() {
    cat << EOF
Uso: $0 [OPÇÕES]

Script consolidado de build do SFINGE-Qt6

OPÇÕES:
    -d, --debug         Build em modo Debug (padrão)
    -r, --release       Build em modo Release (otimizado)
    -c, --clean         Limpar build anterior
    -R, --run           Executar após build
    -t, --test          Executar testes após build
    -v, --verbose       Build verboso
    -j, --jobs N        Número de jobs paralelos (padrão: auto)
    -h, --help          Mostrar esta ajuda

EXEMPLOS:
    $0                          # Build debug
    $0 --release                # Build release
    $0 --debug --run            # Build debug e executar
    $0 --release --clean --run  # Limpar, build release e executar
    $0 --debug --test           # Build debug e executar testes
    $0 -r -c -R -j 8            # Release, limpar, executar, 8 jobs

EOF
}

# Parse argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -c|--clean)
            CLEAN_FIRST=true
            shift
            ;;
        -R|--run)
            RUN_AFTER=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Erro: Opção desconhecida: $1"
            echo "Use --help para ver opções disponíveis"
            exit 1
            ;;
    esac
done

# Definir diretório de build baseado no tipo
if [ "$BUILD_TYPE" = "Debug" ]; then
    BUILD_DIR="$PROJECT_DIR/build-debug"
else
    BUILD_DIR="$PROJECT_DIR/build-release"
fi

echo "=========================================="
echo "SFINGE-Qt6 - Build $BUILD_TYPE"
echo "=========================================="
echo ""
echo "Diretório do projeto: $PROJECT_DIR"
echo "Diretório de build: $BUILD_DIR"
echo ""

# Limpar se solicitado
if [ "$CLEAN_FIRST" = true ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo "Limpando build anterior..."
        rm -rf "$BUILD_DIR"
        echo ""
    fi
fi

# Criar diretório de build
if [ ! -d "$BUILD_DIR" ]; then
    echo "Criando diretório de build..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configurar com CMake
echo "Configurando projeto com CMake ($BUILD_TYPE)..."
CMAKE_ARGS=(
    "$PROJECT_DIR"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
)

if [ "$BUILD_TYPE" = "Debug" ]; then
    CMAKE_ARGS+=(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
fi

cmake "${CMAKE_ARGS[@]}"

echo ""
echo "Compilando..."
echo ""

# Detectar número de cores se não especificado
if [ -z "$JOBS" ]; then
    if command -v nproc &> /dev/null; then
        JOBS=$(nproc)
    else
        JOBS=4
    fi
fi

# Build
BUILD_ARGS=(
    --build .
    --parallel "$JOBS"
)

if [ "$VERBOSE" = true ]; then
    BUILD_ARGS+=(--verbose)
fi

cmake "${BUILD_ARGS[@]}"

# Detectar sistema operacional e nome do executável
OS_TYPE=$(uname -s)
case "$OS_TYPE" in
    Linux*)
        EXEC_NAME="sfinge"
        EXEC_PATH="$BUILD_DIR/$EXEC_NAME"
        ;;
    Darwin*)
        # macOS - pode ser .app bundle ou executável direto
        if [ -d "$BUILD_DIR/sfinge-qt6.app" ]; then
            EXEC_NAME="sfinge-qt6.app"
            EXEC_PATH="$BUILD_DIR/sfinge-qt6.app/Contents/MacOS/sfinge-qt6"
        else
            EXEC_NAME="sfinge-qt6"
            EXEC_PATH="$BUILD_DIR/$EXEC_NAME"
        fi
        ;;
    CYGWIN*|MINGW*|MSYS*|MINGW32*|MINGW64*)
        # Windows
        EXEC_NAME="sfinge-qt6.exe"
        EXEC_PATH="$BUILD_DIR/$EXEC_NAME"
        ;;
    *)
        # Fallback - tentar detectar
        if [ -f "$BUILD_DIR/sfinge" ]; then
            EXEC_NAME="sfinge"
        elif [ -f "$BUILD_DIR/sfinge-qt6" ]; then
            EXEC_NAME="sfinge-qt6"
        elif [ -f "$BUILD_DIR/sfinge-qt6.exe" ]; then
            EXEC_NAME="sfinge-qt6.exe"
        else
            EXEC_NAME="sfinge"
        fi
        EXEC_PATH="$BUILD_DIR/$EXEC_NAME"
        ;;
esac

# Strip se release (não para macOS .app bundle)
if [ "$BUILD_TYPE" = "Release" ] && [ -f "$EXEC_PATH" ]; then
    echo ""
    echo "Removendo símbolos de debug (strip)..."
    strip "$EXEC_PATH" 2>/dev/null || true
fi

echo ""
echo "=========================================="
echo "✓ Build $BUILD_TYPE concluído!"
echo "=========================================="
echo ""
echo "Sistema: $OS_TYPE"
echo "Executável: $BUILD_DIR/$EXEC_NAME"
if [ "$BUILD_TYPE" = "Release" ] && [ -f "$EXEC_PATH" ]; then
    echo "Tamanho: $(du -h $EXEC_PATH 2>/dev/null | cut -f1 || echo 'N/A')"
fi
echo ""

# Executar testes se solicitado
if [ "$RUN_TESTS" = true ]; then
    echo "Executando testes..."
    echo ""
    ctest --output-on-failure
    echo ""
fi

# Executar se solicitado
if [ "$RUN_AFTER" = true ]; then
    echo "Executando SFINGE-Qt6..."
    echo ""
    # Para macOS .app bundle, executar pelo caminho completo
    if [[ "$OS_TYPE" == Darwin* ]] && [[ "$EXEC_PATH" == *".app"* ]]; then
        exec "$EXEC_PATH"
    else
        cd "$BUILD_DIR"
        exec ./"$EXEC_NAME"
    fi
fi
