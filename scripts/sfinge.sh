#!/bin/bash

# Script principal consolidado do SFINGE-Qt6
# Gerencia instalação, build, execução e testes

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPT_DIR="$PROJECT_DIR/scripts"

# Função de ajuda
show_help() {
    cat << EOF
Uso: $0 COMANDO [OPÇÕES]

Script principal do SFINGE-Qt6

COMANDOS:
    install             Instalar dependências
    build               Compilar projeto
    run                 Executar aplicação
    test                Executar testes
    clean               Limpar builds
    all                 Instalar deps + build + executar
    help                Mostrar esta ajuda

OPÇÕES DE BUILD:
    -d, --debug         Modo Debug (padrão)
    -r, --release       Modo Release
    -c, --clean         Limpar antes de compilar
    -j N                Número de jobs paralelos

OPÇÕES DE RUN:
    -b, --build         Compilar antes de executar

EXEMPLOS:
    # Instalação e primeira execução
    $0 install
    $0 build --debug
    $0 run

    # Workflow completo
    $0 all

    # Build e executar release
    $0 build --release --clean
    $0 run --release

    # Build debug com 8 jobs e executar
    $0 build -d -j 8
    $0 run -d

    # Executar testes
    $0 test

    # Limpar tudo
    $0 clean

AJUDA POR COMANDO:
    $0 build --help
    $0 run --help

EOF
}

# Verificar se comando foi fornecido
if [ $# -eq 0 ]; then
    show_help
    exit 1
fi

COMMAND=$1
shift

# Processar comando
case $COMMAND in
    install)
        exec "$SCRIPT_DIR/install_dependencies.sh" "$@"
        ;;
        
    build)
        exec "$SCRIPT_DIR/build.sh" "$@"
        ;;
        
    run)
        exec "$SCRIPT_DIR/run.sh" "$@"
        ;;
        
    test)
        exec "$SCRIPT_DIR/test.sh" "$@"
        ;;
        
    clean)
        exec "$SCRIPT_DIR/clean.sh" "$@"
        ;;
        
    all)
        echo "=========================================="
        echo "SFINGE-Qt6 - Setup Completo"
        echo "=========================================="
        echo ""
        
        # Instalar dependências
        if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
            echo "Instalando dependências..."
            "$SCRIPT_DIR/install_dependencies.sh"
            echo ""
        else
            echo "Dependências já instaladas. Pulando..."
            echo ""
        fi
        
        # Build debug por padrão, ou usar argumentos fornecidos
        echo "Compilando..."
        "$SCRIPT_DIR/build.sh" --debug "$@"
        echo ""
        
        # Executar
        echo "Executando..."
        exec "$SCRIPT_DIR/run.sh" --debug
        ;;
        
    help|--help|-h)
        show_help
        exit 0
        ;;
        
    *)
        echo "Erro: Comando desconhecido: $COMMAND"
        echo ""
        echo "Comandos disponíveis:"
        echo "  install, build, run, test, clean, all, help"
        echo ""
        echo "Use '$0 help' para mais informações"
        exit 1
        ;;
esac
