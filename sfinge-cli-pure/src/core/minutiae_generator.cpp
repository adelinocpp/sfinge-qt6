#include "minutiae_generator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

MinutiaeGenerator::MinutiaeGenerator() 
    : m_width(0), m_height(0), m_coreX(0), m_coreY(0), m_rng(std::random_device{}()) {
}

void MinutiaeGenerator::reseed() {
    std::random_device rd;
    m_rng.seed(rd());
}

void MinutiaeGenerator::setParameters(const MinutiaeParameters& params) {
    m_params = params;
}

void MinutiaeGenerator::setOrientationMap(const std::vector<double>& orientationMap, int width, int height) {
    m_orientationMap = orientationMap;
    m_width = width;
    m_height = height;
}

void MinutiaeGenerator::setShapeMap(const std::vector<float>& shapeMap) {
    m_shapeMap = shapeMap;
}

void MinutiaeGenerator::setRidgeMap(const std::vector<float>& ridgeMap) {
    m_ridgeMap = ridgeMap;
}

void MinutiaeGenerator::setCorePosition(double coreX, double coreY) {
    m_coreX = coreX;
    m_coreY = coreY;
}

int MinutiaeGenerator::calculateTargetCount() {
    if (m_params.targetMinutiae > 0) {
        return m_params.targetMinutiae;
    }
    
    // Distribuição normal centrada no valor típico
    std::normal_distribution<double> dist(
        m_params.stats.typicalMinutiae,
        (m_params.stats.maxMinutiae - m_params.stats.minMinutiae) / 4.0
    );
    
    int count = static_cast<int>(dist(m_rng));
    return std::clamp(count, m_params.stats.minMinutiae, m_params.stats.maxMinutiae);
}

bool MinutiaeGenerator::isValidPosition(double x, double y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return false;
    }
    
    int idx = static_cast<int>(y) * m_width + static_cast<int>(x);
    if (idx < 0 || idx >= static_cast<int>(m_shapeMap.size())) {
        return false;
    }
    
    // Deve estar dentro da forma da impressão digital
    return m_shapeMap[idx] > 0.3f;
}

bool MinutiaeGenerator::hasMinimumSpacing(double x, double y) const {
    double minSpacing = m_params.stats.minSpacing;
    
    for (const auto& m : m_minutiae) {
        double dx = x - m.x;
        double dy = y - m.y;
        double dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist < minSpacing) {
            return false;
        }
    }
    
    return true;
}

double MinutiaeGenerator::getLocalOrientation(double x, double y) const {
    int ix = static_cast<int>(x);
    int iy = static_cast<int>(y);
    
    if (ix < 0 || ix >= m_width || iy < 0 || iy >= m_height) {
        return 0.0;
    }
    
    return m_orientationMap[iy * m_width + ix];
}

bool MinutiaeGenerator::isOnRidge(double x, double y) const {
    int ix = static_cast<int>(x);
    int iy = static_cast<int>(y);
    
    if (ix < 0 || ix >= m_width || iy < 0 || iy >= m_height) {
        return false;
    }
    
    // Crista = valor alto no ridgeMap (após binarização)
    return m_ridgeMap[iy * m_width + ix] > 0.5f;
}

std::pair<double, double> MinutiaeGenerator::generatePosition() {
    std::uniform_real_distribution<double> distX(0, m_width);
    std::uniform_real_distribution<double> distY(0, m_height);
    std::uniform_real_distribution<double> dist01(0, 1);
    
    // Concentração perto do core
    double coreRadius = std::min(m_width, m_height) * m_params.stats.coreRadiusFactor;
    
    for (int attempt = 0; attempt < 100; ++attempt) {
        double x, y;
        
        // Decidir se gerar perto do core ou aleatoriamente
        if (dist01(m_rng) < m_params.stats.coreConcentration) {
            // Gerar perto do core (distribuição gaussiana)
            std::normal_distribution<double> gaussX(m_coreX, coreRadius);
            std::normal_distribution<double> gaussY(m_coreY, coreRadius);
            x = gaussX(m_rng);
            y = gaussY(m_rng);
        } else {
            // Gerar aleatoriamente
            x = distX(m_rng);
            y = distY(m_rng);
        }
        
        if (isValidPosition(x, y) && hasMinimumSpacing(x, y)) {
            return {x, y};
        }
    }
    
    // Fallback: posição aleatória válida
    for (int attempt = 0; attempt < 1000; ++attempt) {
        double x = distX(m_rng);
        double y = distY(m_rng);
        
        if (isValidPosition(x, y) && hasMinimumSpacing(x, y)) {
            return {x, y};
        }
    }
    
    return {-1, -1}; // Falha
}

std::vector<Minutia> MinutiaeGenerator::generateMinutiae() {
    m_minutiae.clear();
    
    if (!m_params.enableExplicitMinutiae) {
        return m_minutiae;
    }
    
    // Se não temos core definido, usar centro da imagem
    if (m_coreX == 0 && m_coreY == 0) {
        m_coreX = m_width / 2.0;
        m_coreY = m_height * 0.4; // Core tipicamente no terço superior
    }
    
    int targetCount = calculateTargetCount();
    
    std::uniform_real_distribution<double> dist01(0, 1);
    std::uniform_real_distribution<double> qualityDist(
        m_params.stats.minQuality, m_params.stats.maxQuality
    );
    
    for (int i = 0; i < targetCount; ++i) {
        auto [x, y] = generatePosition();
        
        if (x < 0 || y < 0) {
            continue; // Não conseguiu encontrar posição válida
        }
        
        Minutia m;
        m.x = x;
        m.y = y;
        m.angle = getLocalOrientation(x, y);
        m.quality = qualityDist(m_rng);
        
        // Decidir tipo baseado na proporção estatística
        if (dist01(m_rng) < m_params.stats.bifurcationRatio) {
            m.type = MinutiaeType::Bifurcation;
        } else {
            m.type = MinutiaeType::RidgeEnding;
        }
        
        m_minutiae.push_back(m);
    }
    
    return m_minutiae;
}

void MinutiaeGenerator::insertRidgeEnding(std::vector<float>& ridgeMap, const Minutia& m) {
    int ix = static_cast<int>(m.x);
    int iy = static_cast<int>(m.y);
    
    // Criar terminação: interromper a crista na direção perpendicular
    double perpAngle = m.angle + M_PI / 2.0;
    
    // Apagar pixels na direção da crista (criar gap)
    for (int d = 0; d < 8; ++d) {
        int px = ix + static_cast<int>(d * std::cos(m.angle));
        int py = iy + static_cast<int>(d * std::sin(m.angle));
        
        if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
            ridgeMap[py * m_width + px] = 0.0f;
        }
    }
}

void MinutiaeGenerator::insertBifurcation(std::vector<float>& ridgeMap, const Minutia& m) {
    int ix = static_cast<int>(m.x);
    int iy = static_cast<int>(m.y);
    
    // Criar bifurcação: adicionar uma ramificação
    double branchAngle1 = m.angle + M_PI / 6.0;  // +30 graus
    double branchAngle2 = m.angle - M_PI / 6.0;  // -30 graus
    
    // Desenhar ramificações
    for (int d = 0; d < 6; ++d) {
        // Ramificação 1
        int px1 = ix + static_cast<int>(d * std::cos(branchAngle1));
        int py1 = iy + static_cast<int>(d * std::sin(branchAngle1));
        
        if (px1 >= 0 && px1 < m_width && py1 >= 0 && py1 < m_height) {
            ridgeMap[py1 * m_width + px1] = 1.0f;
        }
        
        // Ramificação 2
        int px2 = ix + static_cast<int>(d * std::cos(branchAngle2));
        int py2 = iy + static_cast<int>(d * std::sin(branchAngle2));
        
        if (px2 >= 0 && px2 < m_width && py2 >= 0 && py2 < m_height) {
            ridgeMap[py2 * m_width + px2] = 1.0f;
        }
    }
}

void MinutiaeGenerator::applyMinutiae(std::vector<float>& ridgeMap) {
    if (!m_params.enableExplicitMinutiae || m_minutiae.empty()) {
        return;
    }
    
    for (const auto& m : m_minutiae) {
        if (m.type == MinutiaeType::RidgeEnding) {
            insertRidgeEnding(ridgeMap, m);
        } else {
            insertBifurcation(ridgeMap, m);
        }
    }
}

int MinutiaeGenerator::getBifurcationCount() const {
    int count = 0;
    for (const auto& m : m_minutiae) {
        if (m.type == MinutiaeType::Bifurcation) {
            count++;
        }
    }
    return count;
}

int MinutiaeGenerator::getEndingCount() const {
    int count = 0;
    for (const auto& m : m_minutiae) {
        if (m.type == MinutiaeType::RidgeEnding) {
            count++;
        }
    }
    return count;
}

} // namespace SFinGe
