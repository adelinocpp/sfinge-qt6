#!/bin/bash

# Script consolidado de execução do SFINGE-Qt6
# Suporta múltiplas opções via linha de comando

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Valores padrão
BUILD_TYPE="Debug"
BUILD_IF_NEEDED=false

# Função de ajuda
show_help() {
    cat << EOF
Uso: $0 [OPÇÕES] [-- ARGUMENTOS_DO_PROGRAMA]

Script consolidado de execução do SFINGE-Qt6

OPÇÕES:
    -d, --debug         Executar versão Debug (padrão)
    -r, --release       Executar versão Release
    -b, --build         Compilar antes de executar (se necessário)
    -h, --help          Mostrar esta ajuda

ARGUMENTOS:
    Após --, você pode passar argumentos para o programa

EXEMPLOS:
    $0                      # Executar debug
    $0 --release            # Executar release
    $0 --debug --build      # Compilar e executar debug
    $0 -r -b                # Compilar e executar release
    $0 -- --help            # Passar --help para o programa

EOF
}

# Parse argumentos
PROGRAM_ARGS=()
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
        -b|--build)
            BUILD_IF_NEEDED=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        --)
            shift
            PROGRAM_ARGS=("$@")
            break
            ;;
        *)
            echo "Erro: Opção desconhecida: $1"
            echo "Use --help para ver opções disponíveis"
            exit 1
            ;;
    esac
done

# Definir diretório de build
if [ "$BUILD_TYPE" = "Debug" ]; then
    BUILD_DIR="$PROJECT_DIR/build-debug"
else
    BUILD_DIR="$PROJECT_DIR/build-release"
fi

# Detectar sistema operacional e nome do executável
OS_TYPE=$(uname -s)
case "$OS_TYPE" in
    Linux*)
        EXEC_NAME="sfinge"
        ;;
    Darwin*)
        # macOS - procurar por .app bundle ou executável direto
        if [ -d "$BUILD_DIR/sfinge-qt6.app" ]; then
            EXECUTABLE="$BUILD_DIR/sfinge-qt6.app/Contents/MacOS/sfinge-qt6"
        else
            EXEC_NAME="sfinge-qt6"
        fi
        ;;
    CYGWIN*|MINGW*|MSYS*|MINGW32*|MINGW64*)
        # Windows
        EXEC_NAME="sfinge-qt6.exe"
        ;;
    *)
        # Fallback - tentar detectar pelo arquivo que existe
        if [ -f "$BUILD_DIR/sfinge" ]; then
            EXEC_NAME="sfinge"
        elif [ -f "$BUILD_DIR/sfinge-qt6" ]; then
            EXEC_NAME="sfinge-qt6"
        elif [ -f "$BUILD_DIR/sfinge-qt6.exe" ]; then
            EXEC_NAME="sfinge-qt6.exe"
        else
            EXEC_NAME="sfinge-qt6"
        fi
        ;;
esac

# Definir caminho completo do executável se não foi definido acima (macOS bundle)
if [ -z "$EXECUTABLE" ]; then
    EXECUTABLE="$BUILD_DIR/$EXEC_NAME"
fi

# Verificar se executável existe
if [ ! -f "$EXECUTABLE" ]; then
    if [ "$BUILD_IF_NEEDED" = true ]; then
        echo "Executável não encontrado. Compilando..."
        echo ""
        
        BUILD_SCRIPT="$PROJECT_DIR/scripts/build.sh"
        if [ "$BUILD_TYPE" = "Debug" ]; then
            "$BUILD_SCRIPT" --debug
        else
            "$BUILD_SCRIPT" --release
        fi
        
        echo ""
    else
        echo "Erro: Executável não encontrado: $EXECUTABLE"
        echo ""
        echo "Execute primeiro:"
        if [ "$BUILD_TYPE" = "Debug" ]; then
            echo "  ./scripts/build.sh --debug"
        else
            echo "  ./scripts/build.sh --release"
        fi
        echo ""
        echo "Ou use a opção --build:"
        echo "  $0 --build"
        exit 1
    fi
fi

echo "Executando SFINGE-Qt6 ($BUILD_TYPE)..."
echo "Sistema: $OS_TYPE"
echo ""

# Para macOS .app bundle, executar diretamente pelo caminho completo
if [[ "$OS_TYPE" == Darwin* ]] && [[ "$EXECUTABLE" == *".app"* ]]; then
    exec "$EXECUTABLE" "${PROGRAM_ARGS[@]}"
else
    cd "$BUILD_DIR"
    EXEC_NAME=$(basename "$EXECUTABLE")
    exec ./"$EXEC_NAME" "${PROGRAM_ARGS[@]}"
fi
