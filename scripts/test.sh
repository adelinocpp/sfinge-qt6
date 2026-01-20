#!/bin/bash

# Script para executar testes do SFINGE-Qt6

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-debug"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Erro: Diretório de build debug não encontrado: $BUILD_DIR"
    echo ""
    echo "Execute primeiro:"
    echo "  ./scripts/build_debug.sh"
    exit 1
fi

echo "=========================================="
echo "SFINGE-Qt6 - Executando Testes"
echo "=========================================="
echo ""

cd "$BUILD_DIR"

# Verificar se testes foram compilados
if [ ! -d "tests" ]; then
    echo "Aviso: Diretório de testes não encontrado."
    echo "Os testes podem não ter sido compilados."
    echo ""
fi

# Executar testes com CTest
echo "Executando testes com CTest..."
echo ""

ctest --output-on-failure --verbose

echo ""
echo "=========================================="
echo "✓ Testes concluídos!"
echo "=========================================="
