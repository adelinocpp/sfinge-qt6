#!/bin/bash

# SFINGE-Qt6 - Run Script
# Execução da aplicação Qt6

set -e

# Cores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuração
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
EXECUTABLE="$BUILD_DIR/sfinge"

echo -e "${BLUE}=========================================="
echo "SFINGE-Qt6 - Run"
echo "==========================================${NC}"

# Verificar se executável existe
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Executável não encontrado. Compilando...${NC}"
    "$PROJECT_DIR/scripts/build.sh"
fi

# Executar
echo -e "${GREEN}Executando SFINGE-Qt6...${NC}"
cd "$BUILD_DIR"
./sfinge "$@"
