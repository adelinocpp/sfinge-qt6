#!/bin/bash

# Script para limpar builds do SFINGE-Qt6

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "=========================================="
echo "SFINGE-Qt6 - Limpeza de Builds"
echo "=========================================="
echo ""

# Listar diretórios a remover
dirs_to_remove=()

if [ -d "$PROJECT_DIR/build" ]; then
    dirs_to_remove+=("build")
fi

if [ -d "$PROJECT_DIR/build-debug" ]; then
    dirs_to_remove+=("build-debug")
fi

if [ -d "$PROJECT_DIR/build-release" ]; then
    dirs_to_remove+=("build-release")
fi

if [ ${#dirs_to_remove[@]} -eq 0 ]; then
    echo "Nenhum diretório de build encontrado."
    exit 0
fi

echo "Os seguintes diretórios serão removidos:"
for dir in "${dirs_to_remove[@]}"; do
    echo "  - $dir"
done
echo ""

read -p "Confirma a remoção? (s/N): " confirm

if [[ "$confirm" =~ ^[sS]$ ]]; then
    cd "$PROJECT_DIR"
    for dir in "${dirs_to_remove[@]}"; do
        echo "Removendo $dir..."
        rm -rf "$dir"
    done
    echo ""
    echo "✓ Limpeza concluída!"
else
    echo "Operação cancelada."
fi
