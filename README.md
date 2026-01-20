# SFINGE-Qt6 - Synthetic Fingerprint Generation

Gerador de impressões digitais sintéticas multiplataforma usando C++ e Qt6.

## Características (pretendidas)

- 🖥️ **Multiplataforma**: Linux, Windows 11, macOS
- 🎨 **Interface Moderna**: Qt6 Widgets
- ⚡ **Performance**: Algoritmos otimizados em C++17
- 🔬 **Precisão**: Mantém compatibilidade com resultados do SFINGE original

## Algoritmos Implementados

1. **Shape Generation**: Geração da forma do dedo usando elipses
2. **Density Map**: Mapa de frequência das cristas usando ruído Perlin
3. **Orientation Field**: Campo de orientação baseado em pontos singulares
4. **Ridge Pattern**: Geração de cristas usando filtros Gabor 2D

## Requisitos

### Compilação
- CMake 3.16+
- Qt 6.5+
- Compilador C++17:
  - GCC 9+ (Linux)
  - Clang 10+ (macOS)
  - MSVC 2019+ (Windows)

### Runtime
- Qt6 Core, Gui, Widgets

## Compilação Rápida com Scripts

### Setup completo (um comando)
```bash
cd sfinge-qt6
./scripts/sfinge.sh all
```

### Build e execução (método recomendado)
```bash
# Build Debug e executar
./scripts/sfinge.sh build --debug --run

# Build Release e executar
./scripts/sfinge.sh build --release --run

# Ou usar scripts consolidados
./scripts/build.sh --debug --run
./scripts/build.sh --release --run
```

### Método tradicional (passo a passo)
```bash
# Instalar dependências (primeira vez)
./scripts/sfinge.sh install

# Build e executar
./scripts/sfinge.sh build --debug
./scripts/sfinge.sh run --debug
```

### Compilação Manual

#### Linux
```bash
cd sfinge-qt6
mkdir build && cd build
cmake ..
make -j$(nproc)
./sfinge-qt6
```

#### Windows
```bash
cd sfinge-qt6
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

#### macOS
```bash
cd sfinge-qt6
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
open sfinge-qt6.app
```

## Uso

1. Configure parâmetros de forma (Left, Right, Top, Bottom, Middle)
2. Ajuste densidade e zoom
3. Adicione pontos singulares (cores e deltas)
4. Gere a impressão digital
5. Exporte para PNG/BMP/TIFF

## Estrutura do Projeto

```
sfinge-qt6/
├── src/
│   ├── core/           # Algoritmos principais
│   ├── models/         # Modelos de dados
│   ├── ui/             # Interface Qt6
│   └── utils/          # Utilitários
├── resources/          # Ícones e recursos
├── tests/              # Testes unitários
├── docs/               # Documentação
└── legacy/             # Código original (referência)
```

## Desenvolvimento

### Executar Testes
```bash
cd build
ctest --output-on-failure
```

### Gerar Documentação
```bash
doxygen docs/Doxyfile
```

## Migração do Código Legado

Este projeto é uma reimplementação do SFINGE original. O código legado está preservado em `legacy/` para referência e validação.

## Licença

MIT License - Ver arquivo LICENSE

## Autores

- Projeto Original: SFINGE
- Migração Qt6: Adelino Pinheiro Silva

## Referências

Para detalhes sobre os algoritmos, consulte:
- `docs/algorithms.md`
- Código legado em `legacy/DemoOCV/`
