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
    
    // DEBUG: Verificar parâmetros recebidos (apenas se não estiver em modo quiet)
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] DEBUG: setParameters chamado com:";
        qDebug() << "  archAmplitude:" << m_params.archAmplitude;
        qDebug() << "  tentedArchPeakInfluenceDecay:" << m_params.tentedArchPeakInfluenceDecay;
        qDebug() << "  loopEdgeBlendFactor:" << m_params.loopEdgeBlendFactor;
        qDebug() << "  whorlSpiralFactor:" << m_params.whorlSpiralFactor;
        qDebug() << "  whorlEdgeDecayFactor:" << m_params.whorlEdgeDecayFactor;
    }
}

void OrientationGenerator::setFingerprintClass(FingerprintClass fpClass) {
    m_fpClass = fpClass;
}

void OrientationGenerator::generateOrientationMap() {
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Método selecionado:" << static_cast<int>(m_params.method);
    
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

void OrientationGenerator::generateVariedAlphas() {
    // Gerar alphas variados para Sherlock-Monro
    // Cores: média +1, desvio padrão 0.025
    // Deltas: média -1, desvio padrão 0.025
    static std::mt19937 gen(std::random_device{}());
    std::normal_distribution<double> coreAlphaDist(1.0, 0.025);
    std::normal_distribution<double> deltaAlphaDist(-1.0, 0.025);
    
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    m_coreAlphas.resize(cores.size());
    m_deltaAlphas.resize(deltas.size());
    
    for (size_t i = 0; i < cores.size(); ++i) {
        m_coreAlphas[i] = coreAlphaDist(gen);
    }
    
    for (size_t i = 0; i < deltas.size(); ++i) {
        m_deltaAlphas[i] = deltaAlphaDist(gen);
    }
}

void OrientationGenerator::generatePoincareMap() {
    m_orientationMap.resize(m_width * m_height);
    
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    // Gerar alphas variados para esta impressão
    generateVariedAlphas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando mapa Poincaré (refatorado v2.0)";
        qDebug() << "[OrientationGenerator] Dimensões:" << m_width << "x" << m_height;
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
        qDebug() << "[OrientationGenerator] Classe:" << static_cast<int>(m_fpClass);
    }
    
    // Módulo 1: Estrutura refatorada por tipo de impressão digital (v2.0)
    switch (m_fpClass) {
        case FingerprintClass::Arch:
            generateArchOrientation();
            break;
        case FingerprintClass::TentedArch:
            generateTentedArchOrientation();
            break;
        case FingerprintClass::LeftLoop:
        case FingerprintClass::RightLoop:
            generateLoopOrientation();
            break;
        case FingerprintClass::Whorl:
            generateWhorlOrientation();
            break;
        case FingerprintClass::TwinLoop:
            generateTwinLoopOrientation();
            break;
        case FingerprintClass::CentralPocket:
            generateCentralPocketOrientation();
            break;
        case FingerprintClass::Accidental:
            generateAccidentalOrientation();
            break;
        default:
            generateDefaultPoincare();
            break;
    }
    
    // Aplicar suavização gaussiana se habilitada
    if (m_params.enableSmoothing) {
        double sigma = m_params.smoothingSigma;
        // Twin Loop usa seu próprio valor de smoothing
        if (m_fpClass == FingerprintClass::TwinLoop && m_params.twinLoopSmoothing > 0) {
            sigma = m_params.twinLoopSmoothing;
        }
        if (sigma > 0) {
            smoothOrientationMap(sigma);
        }
    }
}

void OrientationGenerator::generateArchOrientation() {
    // ALGORITMO v5.0: Baseado em synthetic_fingerprint_v4.py
    // Convenção: theta = direção PERPENDICULAR às cristas (para Gabor)
    // Para cristas horizontais: theta = π/2
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação para Plain Arch (v5.0)";
        qDebug() << "[OrientationGenerator] archAmplitude:" << m_params.archAmplitude;
    }
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            // Coordenadas normalizadas de -1 a 1
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            // Para cristas horizontais, theta = π/2 (perpendicular)
            // Ondulação senoidal que cria o arco no centro
            double ondulacao = m_params.archAmplitude * std::sin(M_PI * x) * (1.0 - 0.3 * std::abs(y));
            double theta = M_PI / 2.0 + ondulacao;
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateTentedArchOrientation() {
    // ALGORITMO v5.2: Tented Arch usando Sherlock-Monro puro (como Loop/Twin Loop)
    // Convenção: theta = direção PERPENDICULAR às cristas
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação para Tented Arch (v5.3 - Generalizado)";
        qDebug() << "[OrientationGenerator] loopEdgeBlendFactor:" << m_params.loopEdgeBlendFactor;
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    }
    
    // Converter todos os pontos para coordenadas normalizadas
    std::vector<std::pair<double, double>> norm_cores;
    std::vector<std::pair<double, double>> norm_deltas;
    
    for (const auto& c : cores) {
        double cx = (2.0 * c.x / m_width) - 1.0;
        double cy = (2.0 * c.y / m_height) - 1.0;
        norm_cores.push_back({cx, cy});
    }
    
    for (const auto& d : deltas) {
        double dx = (2.0 * d.x / m_width) - 1.0;
        double dy = (2.0 * d.y / m_height) - 1.0;
        norm_deltas.push_back({dx, dy});
    }
    
    const double eps = 0.015;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            // Se não houver singularidades, campo horizontal
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
            // Sherlock-Monro generalizado com índices fracionários
            double total_angle = 0;
            
            // Sherlock-Monro com alphas variados (cores ~+1, deltas ~-1)
            for (size_t k = 0; k < cores.size(); ++k) {
                double dc_x = x - norm_cores[k].first;
                double dc_y = y - norm_cores[k].second;
                if (std::sqrt(dc_x*dc_x + dc_y*dc_y) < eps) { dc_x = eps; dc_y = 0; }
                total_angle += m_coreAlphas[k] * std::atan2(dc_y, dc_x);
            }
            
            for (size_t k = 0; k < deltas.size(); ++k) {
                double dd_x = x - norm_deltas[k].first;
                double dd_y = y - norm_deltas[k].second;
                if (std::sqrt(dd_x*dd_x + dd_y*dd_y) < eps) { dd_x = eps; dd_y = 0; }
                total_angle += m_deltaAlphas[k] * std::atan2(dd_y, dd_x);
            }
            
            double theta_ridge = 0.5 * total_angle;
            
            // Converter para Gabor: adicionar π/2
            double theta = theta_ridge + M_PI / 2.0;
            
            // Edge blend opcional
            if (m_params.loopEdgeBlendFactor > 0) {
                double edge_y = std::clamp((std::abs(y) - 0.85) * 6.67, 0.0, 1.0);
                double edge_x = std::clamp((std::abs(x) - 0.85) * 6.67, 0.0, 1.0);
                double edge_factor = std::max(edge_y, edge_x) * m_params.loopEdgeBlendFactor;
                theta = theta * (1.0 - edge_factor) + (M_PI / 2.0) * edge_factor;
            }
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateLoopOrientation() {
    // ALGORITMO v5.1: Loop - Sherlock-Monro GENERALIZADO
    // Usa TODOS os cores e deltas com índices fracionários
    // Se não houver singularidades, campo é horizontal (θ = π/2)
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação para Loop (v5.1 - Generalizado)";
        qDebug() << "[OrientationGenerator] loopEdgeBlendFactor:" << m_params.loopEdgeBlendFactor;
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    }
    
    // Converter todos os pontos para coordenadas normalizadas
    std::vector<std::pair<double, double>> norm_cores;
    std::vector<std::pair<double, double>> norm_deltas;
    
    for (const auto& c : cores) {
        double cx = (2.0 * c.x / m_width) - 1.0;
        double cy = (2.0 * c.y / m_height) - 1.0;
        norm_cores.push_back({cx, cy});
    }
    
    for (const auto& d : deltas) {
        double dx = (2.0 * d.x / m_width) - 1.0;
        double dy = (2.0 * d.y / m_height) - 1.0;
        norm_deltas.push_back({dx, dy});
    }
    
    const double eps = 0.015;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            // Se não houver singularidades, campo horizontal
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
            // Sherlock-Monro generalizado com índices fracionários
            double total_angle = 0;
            
            // Sherlock-Monro com alphas variados (cores ~+1, deltas ~-1)
            for (size_t k = 0; k < cores.size(); ++k) {
                double dc_x = x - norm_cores[k].first;
                double dc_y = y - norm_cores[k].second;
                if (std::sqrt(dc_x*dc_x + dc_y*dc_y) < eps) { dc_x = eps; dc_y = 0; }
                total_angle += m_coreAlphas[k] * std::atan2(dc_y, dc_x);
            }
            
            for (size_t k = 0; k < deltas.size(); ++k) {
                double dd_x = x - norm_deltas[k].first;
                double dd_y = y - norm_deltas[k].second;
                if (std::sqrt(dd_x*dd_x + dd_y*dd_y) < eps) { dd_x = eps; dd_y = 0; }
                total_angle += m_deltaAlphas[k] * std::atan2(dd_y, dd_x);
            }
            
            double theta_ridge = 0.5 * total_angle;
            
            // Converter para Gabor: adicionar π/2
            double theta = theta_ridge + M_PI / 2.0;
            
            // Edge blend opcional
            if (m_params.loopEdgeBlendFactor > 0) {
                double edge_y = std::clamp((std::abs(y) - 0.85) * 6.67, 0.0, 1.0);
                double edge_x = std::clamp((std::abs(x) - 0.85) * 6.67, 0.0, 1.0);
                double edge_factor = std::max(edge_y, edge_x) * m_params.loopEdgeBlendFactor;
                theta = theta * (1.0 - edge_factor) + (M_PI / 2.0) * edge_factor;
            }
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateWhorlOrientation() {
    // ALGORITMO v5.4: Whorl usando Sherlock-Monro GENERALIZADO
    // Usa TODOS os cores e deltas adicionados
    // Convenção: theta = direção PERPENDICULAR às cristas
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação para Whorl (v5.4 - Sherlock-Monro Generalizado)";
        qDebug() << "[OrientationGenerator] whorlSpiralFactor:" << m_params.whorlSpiralFactor;
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    }
    
    // Converter todos os pontos para coordenadas normalizadas
    std::vector<std::pair<double, double>> norm_cores;
    std::vector<std::pair<double, double>> norm_deltas;
    
    for (const auto& c : cores) {
        double cx = (2.0 * c.x / m_width) - 1.0;
        double cy = (2.0 * c.y / m_height) - 1.0;
        norm_cores.push_back({cx, cy});
    }
    
    for (const auto& d : deltas) {
        double dx = (2.0 * d.x / m_width) - 1.0;
        double dy = (2.0 * d.y / m_height) - 1.0;
        norm_deltas.push_back({dx, dy});
    }
    
    // Calcular centro dos cores para espiral (se houver)
    double center_x = 0, center_y = 0;
    if (!norm_cores.empty()) {
        for (const auto& c : norm_cores) {
            center_x += c.first;
            center_y += c.second;
        }
        center_x /= norm_cores.size();
        center_y /= norm_cores.size();
    }
    
    const double eps = 0.02;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            // Se não houver singularidades, campo horizontal
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
            // Sherlock-Monro generalizado com índices fracionários
            double total_angle = 0;
            
            // Sherlock-Monro com alphas variados (cores ~+1, deltas ~-1)
            for (size_t k = 0; k < cores.size(); ++k) {
                double dc_x = x - norm_cores[k].first;
                double dc_y = y - norm_cores[k].second;
                if (std::sqrt(dc_x*dc_x + dc_y*dc_y) < eps) { dc_x = eps; dc_y = 0; }
                total_angle += m_coreAlphas[k] * std::atan2(dc_y, dc_x);
            }
            
            for (size_t k = 0; k < deltas.size(); ++k) {
                double dd_x = x - norm_deltas[k].first;
                double dd_y = y - norm_deltas[k].second;
                if (std::sqrt(dd_x*dd_x + dd_y*dd_y) < eps) { dd_x = eps; dd_y = 0; }
                total_angle += m_deltaAlphas[k] * std::atan2(dd_y, dd_x);
            }
            
            double theta_ridge = 0.5 * total_angle;
            
            // Converter para Gabor: adicionar π/2
            double theta = theta_ridge + M_PI / 2.0;
            
            // Espiral sutil em torno do centro dos cores
            double dx = x - center_x;
            double dy = y - center_y;
            double r = std::sqrt(dx * dx + dy * dy);
            theta += m_params.whorlSpiralFactor * r * 0.5;
            
            // Edge blend opcional (usar whorlEdgeDecayFactor)
            if (m_params.whorlEdgeDecayFactor > 0) {
                double edge_y = std::clamp((std::abs(y) - 0.85) * 6.67, 0.0, 1.0);
                double edge_x = std::clamp((std::abs(x) - 0.85) * 6.67, 0.0, 1.0);
                double edge_factor = std::max(edge_y, edge_x) * m_params.whorlEdgeDecayFactor;
                theta = theta * (1.0 - edge_factor) + (M_PI / 2.0) * edge_factor;
            }
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateTwinLoopOrientation() {
    // ALGORITMO v5.6: Twin Loop - Sherlock-Monro GENERALIZADO
    // Usa TODOS os cores e deltas com índices fracionários
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação para Twin Loop (v5.6 - Generalizado)";
        qDebug() << "[OrientationGenerator] whorlSpiralFactor:" << m_params.whorlSpiralFactor;
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    }
    
    // Converter todos os pontos para coordenadas normalizadas
    std::vector<std::pair<double, double>> norm_cores;
    std::vector<std::pair<double, double>> norm_deltas;
    
    for (const auto& c : cores) {
        double cx = (2.0 * c.x / m_width) - 1.0;
        double cy = (2.0 * c.y / m_height) - 1.0;
        norm_cores.push_back({cx, cy});
    }
    
    for (const auto& d : deltas) {
        double dx = (2.0 * d.x / m_width) - 1.0;
        double dy = (2.0 * d.y / m_height) - 1.0;
        norm_deltas.push_back({dx, dy});
    }
    
    // Calcular centro dos cores para espiral (se houver)
    double center_x = 0, center_y = 0;
    if (!norm_cores.empty()) {
        for (const auto& c : norm_cores) {
            center_x += c.first;
            center_y += c.second;
        }
        center_x /= norm_cores.size();
        center_y /= norm_cores.size();
    }
    
    const double eps = 0.02;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            // Se não houver singularidades, campo horizontal
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
            // Sherlock-Monro generalizado com índices fracionários
            double total_angle = 0;
            
            // Sherlock-Monro com alphas variados (cores ~+1, deltas ~-1)
            for (size_t k = 0; k < cores.size(); ++k) {
                double dc_x = x - norm_cores[k].first;
                double dc_y = y - norm_cores[k].second;
                if (std::sqrt(dc_x*dc_x + dc_y*dc_y) < eps) { dc_x = eps; dc_y = 0; }
                total_angle += m_coreAlphas[k] * std::atan2(dc_y, dc_x);
            }
            
            for (size_t k = 0; k < deltas.size(); ++k) {
                double dd_x = x - norm_deltas[k].first;
                double dd_y = y - norm_deltas[k].second;
                if (std::sqrt(dd_x*dd_x + dd_y*dd_y) < eps) { dd_x = eps; dd_y = 0; }
                total_angle += m_deltaAlphas[k] * std::atan2(dd_y, dd_x);
            }
            
            double theta_ridge = 0.5 * total_angle;
            
            // Converter para Gabor: adicionar π/2
            double theta = theta_ridge + M_PI / 2.0;
            
            // Espiral sutil em torno do centro
            double dx = x - center_x;
            double dy = y - center_y;
            double r = std::sqrt(dx * dx + dy * dy);
            theta += m_params.whorlSpiralFactor * r * 0.5;
            
            // Edge blend opcional
            if (m_params.whorlEdgeDecayFactor > 0) {
                double edge_y = std::clamp((std::abs(y) - 0.85) * 6.67, 0.0, 1.0);
                double edge_x = std::clamp((std::abs(x) - 0.85) * 6.67, 0.0, 1.0);
                double edge_factor = std::max(edge_y, edge_x) * m_params.whorlEdgeDecayFactor;
                theta = theta * (1.0 - edge_factor) + (M_PI / 2.0) * edge_factor;
            }
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateCentralPocketOrientation() {
    // ALGORITMO v5.4: Central Pocket Loop - Sherlock-Monro GENERALIZADO
    // Usa TODOS os cores e deltas + bolsa circular no centro
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação para Central Pocket (v5.4 - Sherlock-Monro Generalizado)";
        qDebug() << "[OrientationGenerator] centralPocketConcentration:" << m_params.centralPocketConcentration;
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    }
    
    // Converter todos os pontos para coordenadas normalizadas
    std::vector<std::pair<double, double>> norm_cores;
    std::vector<std::pair<double, double>> norm_deltas;
    
    for (const auto& c : cores) {
        double cx = (2.0 * c.x / m_width) - 1.0;
        double cy = (2.0 * c.y / m_height) - 1.0;
        norm_cores.push_back({cx, cy});
    }
    
    for (const auto& d : deltas) {
        double dx = (2.0 * d.x / m_width) - 1.0;
        double dy = (2.0 * d.y / m_height) - 1.0;
        norm_deltas.push_back({dx, dy});
    }
    
    // Calcular centro dos cores (se houver)
    double center_x = 0, center_y = 0;
    if (!norm_cores.empty()) {
        for (const auto& c : norm_cores) {
            center_x += c.first;
            center_y += c.second;
        }
        center_x /= norm_cores.size();
        center_y /= norm_cores.size();
    }
    
    const double eps = 0.02;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            // Se não houver singularidades, campo horizontal
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
            // Sherlock-Monro com alphas variados (cores ~+1, deltas ~-1)
            double total_angle = 0;
            for (size_t k = 0; k < cores.size(); ++k) {
                double dc_x = x - norm_cores[k].first;
                double dc_y = y - norm_cores[k].second;
                if (std::sqrt(dc_x*dc_x + dc_y*dc_y) < eps) { dc_x = eps; dc_y = 0; }
                total_angle += m_coreAlphas[k] * std::atan2(dc_y, dc_x);
            }
            
            for (size_t k = 0; k < deltas.size(); ++k) {
                double dd_x = x - norm_deltas[k].first;
                double dd_y = y - norm_deltas[k].second;
                if (std::sqrt(dd_x*dd_x + dd_y*dd_y) < eps) { dd_x = eps; dd_y = 0; }
                total_angle += m_deltaAlphas[k] * std::atan2(dd_y, dd_x);
            }
            
            double theta_ridge = 0.5 * total_angle;
            
            // Converter para Gabor: adicionar π/2
            double theta = theta_ridge + M_PI / 2.0;
            
            // Adicionar efeito de bolsa circular no centro (característica do Central Pocket)
            double dx = x - center_x;
            double dy = y - center_y;
            double r = std::sqrt(dx * dx + dy * dy);
            double center_weight = std::exp(-r * r / m_params.centralPocketConcentration);
            double theta_radial = std::atan2(dy, dx);
            theta = theta * (1.0 - center_weight * 0.3) + theta_radial * (center_weight * 0.3);
            
            // Edge blend opcional (usar whorlEdgeDecayFactor)
            if (m_params.whorlEdgeDecayFactor > 0) {
                double edge_y = std::clamp((std::abs(y) - 0.85) * 6.67, 0.0, 1.0);
                double edge_x = std::clamp((std::abs(x) - 0.85) * 6.67, 0.0, 1.0);
                double edge_factor = std::max(edge_y, edge_x) * m_params.whorlEdgeDecayFactor;
                theta = theta * (1.0 - edge_factor) + (M_PI / 2.0) * edge_factor;
            }
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateAccidentalOrientation() {
    // ALGORITMO v5.4: Accidental Whorl - Sherlock-Monro GENERALIZADO
    // Usa TODOS os cores e deltas + perturbação irregular
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação para Accidental (v5.4 - Sherlock-Monro Generalizado)";
        qDebug() << "[OrientationGenerator] accidentalIrregularity:" << m_params.accidentalIrregularity;
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    }
    
    // Converter todos os pontos para coordenadas normalizadas
    std::vector<std::pair<double, double>> norm_cores;
    std::vector<std::pair<double, double>> norm_deltas;
    
    for (const auto& c : cores) {
        double cx = (2.0 * c.x / m_width) - 1.0;
        double cy = (2.0 * c.y / m_height) - 1.0;
        norm_cores.push_back({cx, cy});
    }
    
    for (const auto& d : deltas) {
        double dx = (2.0 * d.x / m_width) - 1.0;
        double dy = (2.0 * d.y / m_height) - 1.0;
        norm_deltas.push_back({dx, dy});
    }
    
    // Calcular centro dos cores (se houver)
    double center_x = 0, center_y = 0;
    if (!norm_cores.empty()) {
        for (const auto& c : norm_cores) {
            center_x += c.first;
            center_y += c.second;
        }
        center_x /= norm_cores.size();
        center_y /= norm_cores.size();
    }
    
    const double eps = 0.02;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            // Se não houver singularidades, campo horizontal
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
            // Sherlock-Monro com alphas variados (cores ~+1, deltas ~-1)
            double total_angle = 0;
            for (size_t k = 0; k < cores.size(); ++k) {
                double dc_x = x - norm_cores[k].first;
                double dc_y = y - norm_cores[k].second;
                if (std::sqrt(dc_x*dc_x + dc_y*dc_y) < eps) { dc_x = eps; dc_y = 0; }
                total_angle += m_coreAlphas[k] * std::atan2(dc_y, dc_x);
            }
            
            for (size_t k = 0; k < deltas.size(); ++k) {
                double dd_x = x - norm_deltas[k].first;
                double dd_y = y - norm_deltas[k].second;
                if (std::sqrt(dd_x*dd_x + dd_y*dd_y) < eps) { dd_x = eps; dd_y = 0; }
                total_angle += m_deltaAlphas[k] * std::atan2(dd_y, dd_x);
            }
            
            double theta_ridge = 0.5 * total_angle;
            
            // Converter para Gabor: adicionar π/2
            double theta = theta_ridge + M_PI / 2.0;
            
            // Perturbação adicional para irregularidade (característica do Accidental)
            double dx = x - center_x;
            double dy = y - center_y;
            double r = std::sqrt(dx * dx + dy * dy);
            double irregularity = m_params.accidentalIrregularity * std::sin(5.0 * r) * std::cos(3.0 * std::atan2(dy, dx));
            theta += irregularity;
            
            // Edge blend opcional (usar whorlEdgeDecayFactor)
            if (m_params.whorlEdgeDecayFactor > 0) {
                double edge_y = std::clamp((std::abs(y) - 0.85) * 6.67, 0.0, 1.0);
                double edge_x = std::clamp((std::abs(x) - 0.85) * 6.67, 0.0, 1.0);
                double edge_factor = std::max(edge_y, edge_x) * m_params.whorlEdgeDecayFactor;
                theta = theta * (1.0 - edge_factor) + (M_PI / 2.0) * edge_factor;
            }
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateDefaultPoincare() {
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Gerando orientação padrão (Poincaré fallback)";
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double theta = 0.0;
            
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
            
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateFractionalOrientation() {
    // ALGORITMO v6.0: Orientação com índices de Poincaré fracionários
    // Usa Sherlock-Monro padrão: cores +1, deltas -1
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    if (!m_params.quietMode) {
        qDebug() << "[OrientationGenerator] Gerando orientação FRACIONÁRIA (v6.0)";
        qDebug() << "[OrientationGenerator] Cores:" << cores.size() << "Deltas:" << deltas.size();
    }
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double total_angle = 0.0;
            
            // Contribuição de cada core (+1)
            for (const auto& core : cores) {
                total_angle += computeSingularityContribution(i, j, core, 1.0);
            }
            
            // Contribuição de cada delta (-1)
            for (const auto& delta : deltas) {
                total_angle += computeSingularityContribution(i, j, delta, -1.0);
            }
            
            // Extrair orientação (divide por 2 para simetria de 180°)
            double theta = total_angle / 2.0 + M_PI / 2.0;
            
            // Normalizar para [0, PI)
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

double OrientationGenerator::computeSingularityContribution(double x, double y, 
                                                            const SingularPoint& sing, 
                                                            double alpha) {
    // Diferença para a singularidade
    double diff_x = x - sing.x;
    double diff_y = y - sing.y;
    
    const double eps = 1.0;
    double dist = std::sqrt(diff_x * diff_x + diff_y * diff_y);
    if (dist < eps) {
        diff_x = eps;
        diff_y = 0;
    }
    
    // Ângulo polar φ_k
    double phi = std::atan2(diff_y, diff_x);
    
    // Contribuição: α × φ
    return alpha * phi;
}

void OrientationGenerator::generateFOMFEMap() {
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Gerando mapa FOMFE";
    
    generatePoincareMap();
    
    FOMFEOrientationGenerator fomfe;
    fomfe.setSize(m_width, m_height);
    fomfe.setObservedOrientation(m_orientationMap);
    fomfe.setExpansionOrder(m_params.fomfeOrderM, m_params.fomfeOrderN);
    fomfe.fitCoefficients();
    
    m_orientationMap = fomfe.getOrientationMap();
}

void OrientationGenerator::applyLegendreSmoothing() {
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Aplicando Legendre smoothing";
    
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
    
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Recalculated orientation map with" << m_points.getCoreCount() << "cores," << m_points.getDeltaCount() << "deltas";
    
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
                
                // Visualização mostra a direção das CRISTAS (perpendicular a theta)
                // theta é a direção perpendicular às cristas (para Gabor)
                // Então crista_theta = theta + π/2
                double crista_theta = theta + M_PI / 2.0;
                
                double x1 = i - lineLength * std::cos(crista_theta) / 2.0;
                double y1 = j - lineLength * std::sin(crista_theta) / 2.0;
                double x2 = i + lineLength * std::cos(crista_theta) / 2.0;
                double y2 = j + lineLength * std::sin(crista_theta) / 2.0;
                
                painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
                linesDrawn++;
            }
        }
    }
    
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Drew" << linesDrawn << "lines," << m_points.getCoreCount() << "cores," << m_points.getDeltaCount() << "deltas";
    
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

void OrientationGenerator::smoothOrientationMap(double sigma) {
    // Suavização gaussiana do campo de orientação usando representação cos/sin
    // para evitar problemas de descontinuidade angular
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Aplicando suavização gaussiana com sigma:" << sigma;
    
    if (sigma <= 0 || m_orientationMap.empty()) {
        return;
    }
    
    // Converter orientação para representação cos(2θ) e sin(2θ)
    std::vector<double> cos2(m_width * m_height);
    std::vector<double> sin2(m_width * m_height);
    
    for (int i = 0; i < m_width * m_height; ++i) {
        cos2[i] = std::cos(2.0 * m_orientationMap[i]);
        sin2[i] = std::sin(2.0 * m_orientationMap[i]);
    }
    
    // Aplicar filtro gaussiano separável (horizontal + vertical)
    int kernelSize = static_cast<int>(std::ceil(sigma * 3)) * 2 + 1;
    std::vector<double> kernel(kernelSize);
    double sum = 0.0;
    int halfSize = kernelSize / 2;
    
    // Criar kernel gaussiano
    for (int i = 0; i < kernelSize; ++i) {
        double x = i - halfSize;
        kernel[i] = std::exp(-x * x / (2.0 * sigma * sigma));
        sum += kernel[i];
    }
    // Normalizar
    for (int i = 0; i < kernelSize; ++i) {
        kernel[i] /= sum;
    }
    
    // Buffers temporários
    std::vector<double> cos2_temp(m_width * m_height);
    std::vector<double> sin2_temp(m_width * m_height);
    std::vector<double> cos2_smooth(m_width * m_height);
    std::vector<double> sin2_smooth(m_width * m_height);
    
    // Convolução horizontal
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double sumCos = 0.0, sumSin = 0.0;
            for (int k = -halfSize; k <= halfSize; ++k) {
                int idx = std::clamp(i + k, 0, m_width - 1);
                sumCos += cos2[j * m_width + idx] * kernel[k + halfSize];
                sumSin += sin2[j * m_width + idx] * kernel[k + halfSize];
            }
            cos2_temp[j * m_width + i] = sumCos;
            sin2_temp[j * m_width + i] = sumSin;
        }
    }
    
    // Convolução vertical
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double sumCos = 0.0, sumSin = 0.0;
            for (int k = -halfSize; k <= halfSize; ++k) {
                int idx = std::clamp(j + k, 0, m_height - 1);
                sumCos += cos2_temp[idx * m_width + i] * kernel[k + halfSize];
                sumSin += sin2_temp[idx * m_width + i] * kernel[k + halfSize];
            }
            cos2_smooth[j * m_width + i] = sumCos;
            sin2_smooth[j * m_width + i] = sumSin;
        }
    }
    
    // Converter de volta para ângulo
    for (int i = 0; i < m_width * m_height; ++i) {
        m_orientationMap[i] = 0.5 * std::atan2(sin2_smooth[i], cos2_smooth[i]);
        // Normalizar para [0, PI)
        while (m_orientationMap[i] < 0) m_orientationMap[i] += M_PI;
        while (m_orientationMap[i] >= M_PI) m_orientationMap[i] -= M_PI;
    }
    
    if (!m_params.quietMode) qDebug() << "[OrientationGenerator] Suavização concluída";
}

}
