#include "orientation_generator.h"
#include "fomfe_orientation_generator.h"
#include "orientation_smoother.h"
#include <QDebug>
#include <QPainter>
#include <cmath>
#include <algorithm>
#include <random>
#include <limits>
#include "../models/fingerprint_parameters.h"

namespace SFinGe {

OrientationGenerator::OrientationGenerator() 
    : m_width(0), m_height(0), m_fpClass(FingerprintClass::RightLoop) {
}

void OrientationGenerator::setSingularPoints(const SingularPoints& points) {
    m_points = points;
}

void OrientationGenerator::setShapeMap(const std::vector<float>& shapeMap, int width, int height) {
    m_shapeMap = shapeMap;
    m_width = width;
    m_height = height;
}

void OrientationGenerator::setParameters(const OrientationParameters& params) {
    m_params = params;
    
    // DEBUG: Verificar parâmetros recebidos
    qDebug() << "[OrientationGenerator] DEBUG: setParameters chamado com:";
    qDebug() << "  coreConvergenceStrength:" << m_params.coreConvergenceStrength;
    qDebug() << "  coreConvergenceRadius:" << m_params.coreConvergenceRadius;
    qDebug() << "  coreConvergenceProbability:" << m_params.coreConvergenceProbability;
}

void OrientationGenerator::setFingerprintClass(FingerprintClass fpClass) {
    m_fpClass = fpClass;
}

void OrientationGenerator::generateOrientationMap() {
    qDebug() << "[OrientationGenerator] Método selecionado:" << static_cast<int>(m_params.method);
    
    switch(m_params.method) {
        case OrientationMethod::FOMFE:
            generateFOMFEMap();
            break;
        case OrientationMethod::PoincareSmoothed:
            generatePoincareMap();
            applyLegendreSmoothing();
            break;
        case OrientationMethod::Poincare:
        default:
            generatePoincareMap();
            break;
    }
}

void OrientationGenerator::generatePoincareMap() {
    m_orientationMap.resize(m_width * m_height);
    
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    qDebug() << "[OrientationGenerator] Gerando mapa Poincaré";
    qDebug() << "[OrientationGenerator] Dimensões:" << m_width << "x" << m_height;
    qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    
    if (cores.empty() && deltas.empty()) {
        std::fill(m_orientationMap.begin(), m_orientationMap.end(), 0.0);
        return;
    }
    
    bool isArch = (m_fpClass == FingerprintClass::Arch || m_fpClass == FingerprintClass::TentedArch);
    bool isWhorl = (m_fpClass == FingerprintClass::Whorl);
    bool isRightLoop = (m_fpClass == FingerprintClass::RightLoop);
    bool isLeftLoop = (m_fpClass == FingerprintClass::LeftLoop);
    bool isTwinLoop = (m_fpClass == FingerprintClass::TwinLoop);
    
    // DEBUG: Parâmetros de convergência para Twin Loop
    if (isTwinLoop && cores.size() == 2) {
        qDebug() << "[OrientationGenerator] DEBUG Twin Loop:";
        qDebug() << "  coreConvergenceStrength:" << m_params.coreConvergenceStrength;
        qDebug() << "  coreConvergenceRadius:" << m_params.coreConvergenceRadius;
        qDebug() << "  coreConvergenceProbability:" << m_params.coreConvergenceProbability;
    }
    const double archRotation = -M_PI / 4.0;
    
    // Gerador aleatório para convergência
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double theta = 0.0;
            
            // Processamento normal para todos os tipos
            for (const auto& core : cores) {
                double dx = core.x - i;
                double dy = core.y - j;
                theta += std::atan2(dy, dx);
            }
            
            for (const auto& delta : deltas) {
                double dx = delta.x - i;
                double dy = delta.y - j;
                theta -= std::atan2(dy, dx);
            }
            
            theta *= 0.5;
            
            if (isArch) {
                theta += archRotation;
            }
            
            // Convergência modesta e aleatória no core point
            if (m_params.coreConvergenceStrength > 0 && !cores.empty()) {
                for (size_t coreIdx = 0; coreIdx < cores.size(); ++coreIdx) {
                    const auto& core = cores[coreIdx];
                    double dx = i - core.x;
                    double dy = j - core.y;
                    double dist = std::sqrt(dx*dx + dy*dy);
                    
                    bool applyConvergence = false;
                    
                    if (isWhorl) {
                        // Verticílio: aplicar em toda região do core (modestamente)
                        applyConvergence = true;
                    } else if (isRightLoop) {
                        // Right Loop: porção LESTE (x > core.x)
                        applyConvergence = (dx > 0);
                    } else if (isLeftLoop) {
                        // Left Loop: porção OESTE (x < core.x) 
                        applyConvergence = (dx < 0);
                    } else if (isTwinLoop) {
                        // Twin Loop: aplicar em região próxima dos cores
                        applyConvergence = true;
                    }
                    
                    if (applyConvergence && dist < m_params.coreConvergenceRadius) {
                        // DEBUG: Verificar se está aplicando convergência
                        if (isTwinLoop && cores.size() == 2) {
                            static bool twinConvergenceDebug = false;
                            if (!twinConvergenceDebug) {
                                qDebug() << "[OrientationGenerator] DEBUG: Twin Loop - applyConvergence=true, dist=" << dist << "radius=" << m_params.coreConvergenceRadius;
                                qDebug() << "[OrientationGenerator] DEBUG: coreConvergenceProbability=" << m_params.coreConvergenceProbability;
                                twinConvergenceDebug = true;
                            }
                        }
                        
                        // Aplicar sempre para Twin Loop (forçar para debug)
                        if (isTwinLoop || probDist(gen) < m_params.coreConvergenceProbability) {
                            // Tender para convergir no core (ângulo radial)
                            double targetAngle = std::atan2(-dy, -dx);
                            
                            // Twin Loop: aplicar rotação transversal apenas na convergência
                            if (isTwinLoop && cores.size() == 2) {
                                // Determinar qual é esquerda (menor x) e direita (maior x)
                                bool isLeftCore = (coreIdx == 0 && cores[0].x < cores[1].x) || 
                                                 (coreIdx == 1 && cores[1].x < cores[0].x);
                                
                                // Core da direita: inverter direção na convergência
                                if (!isLeftCore) {
                                    targetAngle += M_PI;  // Inverter 180 graus
                                }
                            }
                            
                            double angleDiff = targetAngle - theta;
                            
                            // Normalizar para [-π, π]
                            while (angleDiff > M_PI) angleDiff -= 2.0 * M_PI;
                            while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;
                            
                            // Peso decai com distância
                            double weight = std::exp(-dist / m_params.coreConvergenceRadius);
                            
                            // Para whorl e twin loop, limitar convergência a 0.25 max
                            double effectiveStrength = m_params.coreConvergenceStrength;
                            if (isWhorl || isTwinLoop) {
                                effectiveStrength = std::min(effectiveStrength, 0.25);
                            }
                            
                            theta += angleDiff * effectiveStrength * weight;
                        }
                    }
                }
            }
            
            // Bias vertical para loops: SUL ao NORDESTE (right) ou SUL ao NOROESTE (left)
            // Raio aumenta gradualmente do sul (1x) até nordeste/noroeste (2x)
            if ((isRightLoop || isLeftLoop) && !cores.empty() && m_params.verticalBiasStrength > 0) {
                for (const auto& core : cores) {
                    double dx = i - core.x;
                    double dy = j - core.y;
                    
                    bool inTargetRegion = false;
                    double radiusMultiplier = 1.0;
                    
                    // Calcular ângulo relativo ao core (0 = leste, π/2 = sul, π = oeste, -π/2 = norte)
                    double angleFromCore = std::atan2(dy, dx);
                    
                    if (isRightLoop) {
                        // Right Loop: do SUL ao NORDESTE (não ultrapassar para norte)
                        // Região: sul + leste + nordeste limitado
                        // Condição 1: dy > -dx (abaixo da diagonal NE-SW)
                        // Condição 2: dy > -raio/2 (não ultrapassar muito ao norte)
                        double northLimit = -m_params.verticalBiasRadius * 0.5;
                        inTargetRegion = (dy > -dx) && (dy > northLimit);
                        
                        if (inTargetRegion) {
                            // Raio aumenta de 1x (sul puro π/2) até 2x conforme se afasta em direção nordeste
                            double deviationFromSouth = std::abs(angleFromCore - M_PI_2);
                            double maxDeviation = M_PI; // Até 180 graus
                            radiusMultiplier = 1.0 + (deviationFromSouth / maxDeviation);
                            radiusMultiplier = std::min(radiusMultiplier, 2.0);
                        }
                    } else if (isLeftLoop) {
                        // Left Loop: do SUL ao NOROESTE (não ultrapassar para norte)
                        // Região: sul + oeste + noroeste limitado
                        // Condição 1: dy > dx (abaixo da diagonal NW-SE)
                        // Condição 2: dy > -raio/2 (não ultrapassar muito ao norte)
                        double northLimit = -m_params.verticalBiasRadius * 0.5;
                        inTargetRegion = (dy > dx) && (dy > northLimit);
                        
                        if (inTargetRegion) {
                            // Raio aumenta de 1x (sul puro π/2) até 2x conforme se afasta em direção noroeste
                            double deviationFromSouth = std::abs(angleFromCore - M_PI_2);
                            // Para ângulos no quadrante oeste/noroeste, ajustar cálculo
                            if (std::abs(angleFromCore) > M_PI_2) {
                                // Oeste ou noroeste: calcular desvio passando por π
                                double angleFromWest = std::abs(angleFromCore) - M_PI;
                                deviationFromSouth = M_PI_2 + std::abs(angleFromWest);
                            }
                            double maxDeviation = M_PI; // Até 180 graus
                            radiusMultiplier = 1.0 + (deviationFromSouth / maxDeviation);
                            radiusMultiplier = std::min(radiusMultiplier, 2.0);
                        }
                    }
                    
                    if (inTargetRegion) {
                        double dist = std::sqrt(dx*dx + dy*dy);
                        double effectiveRadius = m_params.verticalBiasRadius * radiusMultiplier;
                        
                        if (dist < effectiveRadius) {
                            // Peso decai exponencialmente
                            double weight = std::exp(-dist / effectiveRadius);
                            
                            // Tender para vertical (π/2)
                            // Como orientações estão em [0, π), precisamos mapear π/2 corretamente
                            double currentTheta = theta;
                            while (currentTheta < 0) currentTheta += M_PI;
                            while (currentTheta >= M_PI) currentTheta -= M_PI;
                            
                            // Calcular menor diferença angular para π/2
                            double targetAngle = M_PI_2;
                            double angleDiff = targetAngle - currentTheta;
                            
                            // Como estamos em [0, π), a diferença máxima é π/2
                            // Não precisa normalizar para [-π, π] pois já está em range correto
                            
                            theta += angleDiff * m_params.verticalBiasStrength * weight;
                        }
                    }
                }
            }
            
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateFOMFEMap() {
    qDebug() << "[OrientationGenerator] Gerando mapa FOMFE";
    
    generatePoincareMap();
    
    FOMFEOrientationGenerator fomfe;
    fomfe.setSize(m_width, m_height);
    fomfe.setObservedOrientation(m_orientationMap);
    fomfe.setExpansionOrder(m_params.fomfeOrderM, m_params.fomfeOrderN);
    fomfe.fitCoefficients();
    
    m_orientationMap = fomfe.getOrientationMap();
}

void OrientationGenerator::applyLegendreSmoothing() {
    qDebug() << "[OrientationGenerator] Aplicando Legendre smoothing";
    
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    std::vector<std::pair<int,int>> singularPoints;
    for (const auto& c : cores) {
        singularPoints.push_back({static_cast<int>(c.x), static_cast<int>(c.y)});
    }
    for (const auto& d : deltas) {
        singularPoints.push_back({static_cast<int>(d.x), static_cast<int>(d.y)});
    }
    
    OrientationSmoother smoother;
    smoother.setOrientationMap(m_orientationMap, m_width, m_height);
    smoother.setLegendreOrder(m_params.legendreOrder);
    smoother.setSingularPoints(singularPoints);
    
    m_orientationMap = smoother.smoothAdaptiveLegendre();
}

QImage OrientationGenerator::generate() {
    generateOrientationMap();
    
    QImage image(m_width, m_height, QImage::Format_Grayscale8);
    image.fill(Qt::white);
    
    return image;
}

QImage OrientationGenerator::generateVisualization() {
    generateOrientationMap();
    
    qDebug() << "[OrientationGenerator] Recalculated orientation map with" << m_points.getCoreCount() << "cores," << m_points.getDeltaCount() << "deltas";
    
    QImage image(m_width, m_height, QImage::Format_RGB32);
    image.fill(Qt::white);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPen pen(Qt::blue);
    pen.setWidth(1);
    painter.setPen(pen);
    
    int spacing = 12;
    int lineLength = 10;
    int linesDrawn = 0;
    
    for (int j = spacing/2; j < m_height; j += spacing) {
        for (int i = spacing/2; i < m_width; i += spacing) {
            if (m_shapeMap.empty() || m_shapeMap[j * m_width + i] > 0.5) {
                double theta = m_orientationMap[j * m_width + i];
                
                double x1 = i - lineLength * std::cos(theta) / 2.0;
                double y1 = j - lineLength * std::sin(theta) / 2.0;
                double x2 = i + lineLength * std::cos(theta) / 2.0;
                double y2 = j + lineLength * std::sin(theta) / 2.0;
                
                painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
                linesDrawn++;
            }
        }
    }
    
    qDebug() << "[OrientationGenerator] Drew" << linesDrawn << "lines," << m_points.getCoreCount() << "cores," << m_points.getDeltaCount() << "deltas";
    
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    pen.setColor(Qt::red);
    pen.setWidth(3);
    painter.setPen(pen);
    painter.setBrush(Qt::red);
    for (const auto& core : cores) {
        painter.drawEllipse(QPointF(core.x, core.y), 5, 5);
    }
    
    pen.setColor(Qt::green);
    painter.setPen(pen);
    painter.setBrush(Qt::green);
    for (const auto& delta : deltas) {
        painter.drawEllipse(QPointF(delta.x, delta.y), 5, 5);
    }
    
    painter.end();
    return image;
}

}
