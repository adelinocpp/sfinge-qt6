#include "orientation_generator.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

OrientationGenerator::OrientationGenerator() 
    : m_width(0), m_height(0), m_fpClass(FingerprintClass::RightLoop),
      m_rng(std::random_device{}()) {
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
}

void OrientationGenerator::setFingerprintClass(FingerprintClass fpClass) {
    m_fpClass = fpClass;
}

void OrientationGenerator::generateVariedAlphas() {
    std::normal_distribution<double> coreAlphaDist(1.0, 0.025);
    std::normal_distribution<double> deltaAlphaDist(-1.0, 0.025);
    
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
    m_coreAlphas.resize(cores.size());
    m_deltaAlphas.resize(deltas.size());
    
    for (size_t i = 0; i < cores.size(); ++i) {
        m_coreAlphas[i] = coreAlphaDist(m_rng);
    }
    
    for (size_t i = 0; i < deltas.size(); ++i) {
        m_deltaAlphas[i] = deltaAlphaDist(m_rng);
    }
}

void OrientationGenerator::generateOrientationMap() {
    generatePoincareMap();
}

void OrientationGenerator::generatePoincareMap() {
    m_orientationMap.resize(m_width * m_height);
    
    generateVariedAlphas();
    
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
    
    if (m_params.enableSmoothing) {
        double sigma = m_params.smoothingSigma;
        if (m_fpClass == FingerprintClass::TwinLoop && m_params.twinLoopSmoothing > 0) {
            sigma = m_params.twinLoopSmoothing;
        }
        if (sigma > 0) {
            smoothOrientationMap(sigma);
        }
    }
}

void OrientationGenerator::generateArchOrientation() {
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            double ondulacao = m_params.archAmplitude * std::sin(M_PI * x) * (1.0 - 0.3 * std::abs(y));
            double theta = M_PI / 2.0 + ondulacao;
            
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateTentedArchOrientation() {
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
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
    
    const double eps = 1e-6;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
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
            double theta = theta_ridge + M_PI / 2.0;
            
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateLoopOrientation() {
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
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
    
    const double eps = 1e-6;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
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
            double theta = theta_ridge + M_PI / 2.0;
            
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateWhorlOrientation() {
    const auto& cores = m_points.getCores();
    const auto& deltas = m_points.getDeltas();
    
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
    
    double center_x = 0, center_y = 0;
    if (!norm_cores.empty()) {
        for (const auto& c : norm_cores) {
            center_x += c.first;
            center_y += c.second;
        }
        center_x /= norm_cores.size();
        center_y /= norm_cores.size();
    }
    
    const double eps = 1e-6;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double x = (2.0 * i / m_width) - 1.0;
            double y = (2.0 * j / m_height) - 1.0;
            
            if (cores.empty() && deltas.empty()) {
                m_orientationMap[j * m_width + i] = M_PI / 2.0;
                continue;
            }
            
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
            double theta = theta_ridge + M_PI / 2.0;
            
            double dx = x - center_x;
            double dy = y - center_y;
            double r = std::sqrt(dx * dx + dy * dy);
            double spiral = m_params.whorlSpiralFactor * r;
            theta += spiral;
            
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            m_orientationMap[j * m_width + i] = theta;
        }
    }
}

void OrientationGenerator::generateTwinLoopOrientation() {
    generateWhorlOrientation();
}

void OrientationGenerator::generateCentralPocketOrientation() {
    generateWhorlOrientation();
}

void OrientationGenerator::generateAccidentalOrientation() {
    generateWhorlOrientation();
}

void OrientationGenerator::generateDefaultPoincare() {
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            m_orientationMap[j * m_width + i] = M_PI / 2.0;
        }
    }
}

void OrientationGenerator::smoothOrientationMap(double sigma) {
    if (sigma <= 0 || m_orientationMap.empty()) return;
    
    std::vector<double> cos2(m_width * m_height);
    std::vector<double> sin2(m_width * m_height);
    
    for (int i = 0; i < m_width * m_height; ++i) {
        cos2[i] = std::cos(2.0 * m_orientationMap[i]);
        sin2[i] = std::sin(2.0 * m_orientationMap[i]);
    }
    
    int kernelRadius = static_cast<int>(std::ceil(3.0 * sigma));
    std::vector<double> kernel(2 * kernelRadius + 1);
    double kernelSum = 0;
    
    for (int k = -kernelRadius; k <= kernelRadius; ++k) {
        kernel[k + kernelRadius] = std::exp(-0.5 * k * k / (sigma * sigma));
        kernelSum += kernel[k + kernelRadius];
    }
    for (auto& k : kernel) k /= kernelSum;
    
    std::vector<double> cos2_smooth(m_width * m_height);
    std::vector<double> sin2_smooth(m_width * m_height);
    
    // Horizontal pass
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double sumCos = 0, sumSin = 0;
            for (int k = -kernelRadius; k <= kernelRadius; ++k) {
                int ii = std::clamp(i + k, 0, m_width - 1);
                sumCos += cos2[j * m_width + ii] * kernel[k + kernelRadius];
                sumSin += sin2[j * m_width + ii] * kernel[k + kernelRadius];
            }
            cos2_smooth[j * m_width + i] = sumCos;
            sin2_smooth[j * m_width + i] = sumSin;
        }
    }
    
    // Vertical pass
    cos2 = cos2_smooth;
    sin2 = sin2_smooth;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            double sumCos = 0, sumSin = 0;
            for (int k = -kernelRadius; k <= kernelRadius; ++k) {
                int jj = std::clamp(j + k, 0, m_height - 1);
                sumCos += cos2[jj * m_width + i] * kernel[k + kernelRadius];
                sumSin += sin2[jj * m_width + i] * kernel[k + kernelRadius];
            }
            cos2_smooth[j * m_width + i] = sumCos;
            sin2_smooth[j * m_width + i] = sumSin;
        }
    }
    
    for (int i = 0; i < m_width * m_height; ++i) {
        m_orientationMap[i] = 0.5 * std::atan2(sin2_smooth[i], cos2_smooth[i]);
        while (m_orientationMap[i] < 0) m_orientationMap[i] += M_PI;
        while (m_orientationMap[i] >= M_PI) m_orientationMap[i] -= M_PI;
    }
}

Image OrientationGenerator::generate() {
    generateOrientationMap();
    
    Image image(m_width, m_height);
    image.fill(255);
    
    return image;
}

void OrientationGenerator::reseed() {
    std::random_device rd;
    m_rng.seed(rd());
}

} // namespace SFinGe
