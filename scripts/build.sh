#!/bin/bash

# SFINGE-Qt6 - Build Script
# Compilação simplificada da aplicação Qt6

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

echo -e "${BLUE}=========================================="
echo "SFINGE-Qt6 - Build"
echo "==========================================${NC}"
echo "Projeto: $PROJECT_DIR"
echo "Build: $BUILD_DIR"
echo ""

# Criar diretório de build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configurar CMake
echo -e "${YELLOW}Configurando com CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compilar
echo -e "${YELLOW}Compilando...${NC}"
make -j$(nproc)

echo -e "${GREEN}✅ Build concluído!${NC}"
echo ""
echo "Para executar:"
echo "  cd $BUILD_DIR && ./sfinge"
echo ""
echo "Para criar pacote .deb:"
echo "  cd $PROJECT_DIR && ./scripts/create-deb.sh"
