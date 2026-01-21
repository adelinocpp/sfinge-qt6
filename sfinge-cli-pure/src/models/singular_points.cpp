#include "singular_points.h"
#include <cmath>
#include <iostream>

namespace SFinGe {

SingularPoints::SingularPoints() : m_rng(std::random_device{}()) {}

void SingularPoints::addCore(double x, double y) {
    m_cores.push_back({x, y});
}

void SingularPoints::addDelta(double x, double y) {
    m_deltas.push_back({x, y});
}

void SingularPoints::updateCore(int index, double x, double y) {
    if (index >= 0 && index < static_cast<int>(m_cores.size())) {
        m_cores[index].x = x;
        m_cores[index].y = y;
    }
}

void SingularPoints::updateDelta(int index, double x, double y) {
    if (index >= 0 && index < static_cast<int>(m_deltas.size())) {
        m_deltas[index].x = x;
        m_deltas[index].y = y;
    }
}

void SingularPoints::clear() {
    m_cores.clear();
    m_deltas.clear();
}

void SingularPoints::clearCores() {
    m_cores.clear();
}

void SingularPoints::clearDeltas() {
    m_deltas.clear();
}

void SingularPoints::generateRandomPoints(FingerprintClass fpClass, int width, int height) {
    clear();
    
    // Estatísticas forenses reais para posições de pontos singulares:
    // - Core: tipicamente no terço superior da impressão (30-45% da altura)
    // - Delta: tipicamente no terço inferior (55-75% da altura)
    // - Distância core-delta em loops: 40-60 ridge counts (~4-6mm)
    // - Em whorls: 2 cores próximos ao centro, 2 deltas nas laterais inferiores
    
    std::uniform_real_distribution<double> smallVar(-0.03, 0.03);
    std::uniform_real_distribution<double> coreVarX(-0.05, 0.05);
    std::uniform_real_distribution<double> coreVarY(-0.03, 0.03);
    
    switch (fpClass) {
        case FingerprintClass::Arch:
            // Arcos não têm pontos singulares verdadeiros
            break;
            
        case FingerprintClass::TentedArch: {
            // Core no centro-superior, delta logo abaixo
            double cx = width * (0.50 + coreVarX(m_rng));
            double cy = height * (0.35 + coreVarY(m_rng));
            addCore(cx, cy);
            // Delta diretamente abaixo do core (distância típica: 15-25% da altura)
            addDelta(cx + width * smallVar(m_rng), cy + height * 0.20);
            break;
        }
        
        case FingerprintClass::LeftLoop: {
            // Core deslocado para a esquerda (30-40% da largura)
            // Delta na direita inferior
            double cx = width * (0.38 + coreVarX(m_rng));
            double cy = height * (0.38 + coreVarY(m_rng));
            addCore(cx, cy);
            // Delta na lateral oposta, mais abaixo
            double dx = width * (0.62 + smallVar(m_rng));
            double dy = height * (0.62 + smallVar(m_rng));
            addDelta(dx, dy);
            break;
        }
        
        case FingerprintClass::RightLoop: {
            // Core deslocado para a direita (60-70% da largura)
            // Delta na esquerda inferior
            double cx = width * (0.62 + coreVarX(m_rng));
            double cy = height * (0.38 + coreVarY(m_rng));
            addCore(cx, cy);
            // Delta na lateral oposta, mais abaixo
            double dx = width * (0.38 + smallVar(m_rng));
            double dy = height * (0.62 + smallVar(m_rng));
            addDelta(dx, dy);
            break;
        }
        
        case FingerprintClass::Whorl: {
            // 2 cores próximos ao centro (separação típica: 10-15% da largura)
            // 2 deltas nas laterais inferiores
            double cx = width * 0.50;
            double cy = height * (0.38 + coreVarY(m_rng));
            double coreSep = width * (0.06 + std::abs(smallVar(m_rng)));
            addCore(cx - coreSep, cy);
            addCore(cx + coreSep, cy);
            // Deltas nas laterais inferiores (típico: 25-30% e 70-75% da largura)
            addDelta(width * (0.28 + smallVar(m_rng)), height * (0.68 + smallVar(m_rng)));
            addDelta(width * (0.72 + smallVar(m_rng)), height * (0.68 + smallVar(m_rng)));
            break;
        }
        
        case FingerprintClass::TwinLoop: {
            // 2 cores com maior separação que whorl
            double cx = width * 0.50;
            double cy = height * (0.36 + coreVarY(m_rng));
            double coreSep = width * (0.12 + std::abs(smallVar(m_rng)));
            addCore(cx - coreSep, cy);
            addCore(cx + coreSep, cy);
            // Deltas nas laterais inferiores
            addDelta(width * (0.25 + smallVar(m_rng)), height * (0.70 + smallVar(m_rng)));
            addDelta(width * (0.75 + smallVar(m_rng)), height * (0.70 + smallVar(m_rng)));
            break;
        }
        
        case FingerprintClass::CentralPocket: {
            // 1 core central, 2 deltas laterais
            addCore(width * (0.50 + coreVarX(m_rng)), height * (0.38 + coreVarY(m_rng)));
            addDelta(width * (0.28 + smallVar(m_rng)), height * (0.68 + smallVar(m_rng)));
            addDelta(width * (0.72 + smallVar(m_rng)), height * (0.68 + smallVar(m_rng)));
            break;
        }
        
        case FingerprintClass::Accidental: {
            // Padrão irregular com 2 cores e 2 deltas
            addCore(width * (0.42 + coreVarX(m_rng)), height * (0.34 + coreVarY(m_rng)));
            addCore(width * (0.58 + coreVarX(m_rng)), height * (0.40 + coreVarY(m_rng)));
            addDelta(width * (0.30 + smallVar(m_rng)), height * (0.68 + smallVar(m_rng)));
            addDelta(width * (0.70 + smallVar(m_rng)), height * (0.68 + smallVar(m_rng)));
            break;
        }
        
        default:
            break;
    }
}

void SingularPoints::suggestPoints(FingerprintClass fpClass, int width, int height) {
    generateRandomPoints(fpClass, width, height);
}

void SingularPoints::reseed() {
    std::random_device rd;
    m_rng.seed(rd());
}

} // namespace SFinGe
