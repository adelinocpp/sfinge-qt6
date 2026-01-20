#!/bin/bash

# Script de instalação de dependências do SFINGE-Qt6
# Suporta: Ubuntu/Debian, Fedora, Arch Linux, macOS, Windows (MSYS2)

set -e

echo "=========================================="
echo "SFINGE-Qt6 - Instalação de Dependências"
echo "=========================================="
echo ""

# Detectar sistema operacional
OS_TYPE=$(uname -s)

case "$OS_TYPE" in
    Linux*)
        # Linux - detectar distribuição
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            OS=$ID
        else
            echo "Erro: Não foi possível detectar a distribuição Linux"
            exit 1
        fi
        echo "Sistema detectado: $PRETTY_NAME"
        ;;
    Darwin*)
        # macOS
        OS="macos"
        echo "Sistema detectado: macOS $(sw_vers -productVersion)"
        ;;
    CYGWIN*|MINGW*|MSYS*)
        # Windows com MSYS2/MinGW
        OS="windows"
        echo "Sistema detectado: Windows (MSYS2/MinGW)"
        ;;
    *)
        echo "Sistema operacional não reconhecido: $OS_TYPE"
        exit 1
        ;;
esac

echo ""

case "$OS" in
    ubuntu|debian|linuxmint|pop)
        echo "Instalando dependências para Ubuntu/Debian..."
        sudo apt update
        sudo apt install -y \
            qt6-base-dev \
            qt6-tools-dev \
            qt6-tools-dev-tools \
            cmake \
            build-essential \
            git \
            pkg-config
        
        echo ""
        echo "Verificando versões instaladas:"
        qmake6 --version || qmake --version
        cmake --version | head -n1
        g++ --version | head -n1
        ;;
        
    fedora|rhel|centos)
        echo "Instalando dependências para Fedora/RHEL..."
        sudo dnf install -y \
            qt6-qtbase-devel \
            qt6-qttools-devel \
            cmake \
            gcc-c++ \
            git \
            pkg-config
        
        echo ""
        echo "Verificando versões instaladas:"
        qmake-qt6 --version || qmake --version
        cmake --version | head -n1
        g++ --version | head -n1
        ;;
        
    arch|manjaro)
        echo "Instalando dependências para Arch Linux..."
        sudo pacman -Sy --needed --noconfirm \
            qt6-base \
            qt6-tools \
            cmake \
            gcc \
            git \
            pkg-config
        
        echo ""
        echo "Verificando versões instaladas:"
        qmake6 --version || qmake --version
        cmake --version | head -n1
        g++ --version | head -n1
        ;;
        
    opensuse*|suse)
        echo "Instalando dependências para openSUSE..."
        sudo zypper install -y \
            qt6-base-devel \
            qt6-tools-devel \
            cmake \
            gcc-c++ \
            git \
            pkg-config
        
        echo ""
        echo "Verificando versões instaladas:"
        qmake6 --version || qmake --version
        cmake --version | head -n1
        g++ --version | head -n1
        ;;
    
    macos)
        echo "Instalando dependências para macOS..."
        
        # Verificar se Homebrew está instalado
        if ! command -v brew &> /dev/null; then
            echo "Homebrew não encontrado. Instalando..."
            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        fi
        
        # Instalar dependências
        brew install qt@6 cmake
        
        # Adicionar Qt ao PATH se necessário
        if ! command -v qmake &> /dev/null; then
            echo ""
            echo "Adicionando Qt6 ao PATH..."
            echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.zshrc
            echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.bash_profile
            echo ""
            echo "Execute: source ~/.zshrc"
        fi
        
        echo ""
        echo "Verificando versões instaladas:"
        qmake --version || /opt/homebrew/opt/qt@6/bin/qmake --version
        cmake --version | head -n1
        clang++ --version | head -n1
        ;;
    
    windows)
        echo "Instalando dependências para Windows (MSYS2)..."
        
        # Verificar se pacman está disponível (MSYS2)
        if command -v pacman &> /dev/null; then
            pacman -S --needed --noconfirm \
                mingw-w64-x86_64-qt6-base \
                mingw-w64-x86_64-qt6-tools \
                mingw-w64-x86_64-cmake \
                mingw-w64-x86_64-gcc \
                git
            
            echo ""
            echo "Verificando versões instaladas:"
            qmake --version
            cmake --version | head -n1
            g++ --version | head -n1
        else
            echo ""
            echo "MSYS2 não detectado."
            echo "Por favor, instale Qt6 manualmente:"
            echo "  1. Baixe o Qt Online Installer de: https://www.qt.io/download-qt-installer"
            echo "  2. Instale Qt 6.5+ com MinGW ou MSVC"
            echo "  3. Instale CMake de: https://cmake.org/download/"
            exit 1
        fi
        ;;
        
    *)
        echo "Distribuição/Sistema '$OS' não suportada automaticamente."
        echo ""
        echo "Por favor, instale manualmente:"
        echo "  - Qt 6.5 ou superior (qt6-base-dev, qt6-tools-dev)"
        echo "  - CMake 3.16 ou superior"
        echo "  - Compilador C++17 (GCC 9+ ou Clang 10+)"
        echo "  - Git"
        exit 1
        ;;
esac

echo ""
echo "=========================================="
echo "✓ Dependências instaladas com sucesso!"
echo "=========================================="
echo ""
echo "Próximo passo: Execute um dos scripts de build:"
echo "  ./scripts/build_debug.sh   - Build de desenvolvimento"
echo "  ./scripts/build_release.sh - Build otimizado"
