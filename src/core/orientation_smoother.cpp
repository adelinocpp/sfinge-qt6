#include "orientation_smoother.h"
#include <cmath>
#include <QDebug>
#include <algorithm>

namespace SFinGe {

OrientationSmoother::OrientationSmoother()
    : m_width(0), m_height(0), m_order(5) {
}

void OrientationSmoother::setOrientationMap(const std::vector<double>& orientationMap, int width, int height) {
    m_orientationMap = orientationMap;
    m_width = width;
    m_height = height;
}

void OrientationSmoother::setLegendreOrder(int order) {
    m_order = order;
}

void OrientationSmoother::setSingularPoints(const std::vector<std::pair<int,int>>& points) {
    m_singularPoints = points;
}

double OrientationSmoother::legendrePolynomial(int n, double x) const {
    if (n == 0) return 1.0;
    if (n == 1) return x;
    
    double P_n_minus_2 = 1.0;
    double P_n_minus_1 = x;
    double P_n = 0.0;
    
    for (int k = 2; k <= n; ++k) {
        P_n = ((2.0 * k - 1.0) * x * P_n_minus_1 - (k - 1.0) * P_n_minus_2) / k;
        P_n_minus_2 = P_n_minus_1;
        P_n_minus_1 = P_n;
    }
    
    return P_n;
}

std::vector<double> OrientationSmoother::fitLegendreCoefficients(int order) const {
    int numCoeffs = (order + 1) * (order + 1);
    std::vector<double> coeffs(numCoeffs, 0.0);
    
    // Normalizar coordenadas para [-1, 1]
    for (int m = 0; m <= order; ++m) {
        for (int n = 0; n <= order; ++n) {
            double sum = 0.0;
            int count = 0;
            
            // Amostragem esparsa
            for (int j = 0; j < m_height; j += 4) {
                for (int i = 0; i < m_width; i += 4) {
                    double x_norm = 2.0 * i / m_width - 1.0;
                    double y_norm = 2.0 * j / m_height - 1.0;
                    
                    double P_m = legendrePolynomial(m, x_norm);
                    double P_n = legendrePolynomial(n, y_norm);
                    
                    int idx = j * m_width + i;
                    sum += m_orientationMap[idx] * P_m * P_n;
                    count++;
                }
            }
            
            coeffs[m * (order + 1) + n] = sum / count;
        }
    }
    
    return coeffs;
}

double OrientationSmoother::evaluateLegendreField(int i, int j, const std::vector<double>& coeffs, int order) const {
    double x_norm = 2.0 * i / m_width - 1.0;
    double y_norm = 2.0 * j / m_height - 1.0;
    
    double theta = 0.0;
    for (int m = 0; m <= order; ++m) {
        for (int n = 0; n <= order; ++n) {
            double P_m = legendrePolynomial(m, x_norm);
            double P_n = legendrePolynomial(n, y_norm);
            theta += coeffs[m * (order + 1) + n] * P_m * P_n;
        }
    }
    
    while (theta < 0) theta += M_PI;
    while (theta >= M_PI) theta -= M_PI;
    
    return theta;
}

std::vector<double> OrientationSmoother::smoothLegendre() {
    qDebug() << "[Smoother] Aplicando suavização Legendre, ordem =" << m_order;
    
    std::vector<double> coeffs = fitLegendreCoefficients(m_order);
    std::vector<double> smoothed(m_width * m_height);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            smoothed[j * m_width + i] = evaluateLegendreField(i, j, coeffs, m_order);
        }
    }
    
    return smoothed;
}

std::vector<double> OrientationSmoother::smoothAdaptiveLegendre() {
    qDebug() << "[Smoother] Aplicando suavização Legendre adaptativa";
    
    // Ordem baixa globalmente
    std::vector<double> coeffsLow = fitLegendreCoefficients(3);
    
    // Ordem alta perto de singularidades
    std::vector<double> coeffsHigh = fitLegendreCoefficients(m_order);
    
    std::vector<double> smoothed(m_width * m_height);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            
            // Calcular distância mínima aos pontos singulares
            double minDist = 1e9;
            for (const auto& sp : m_singularPoints) {
                double dx = i - sp.first;
                double dy = j - sp.second;
                double dist = std::sqrt(dx*dx + dy*dy);
                minDist = std::min(minDist, dist);
            }
            
            // Blend: ordem alta perto (< 50px), ordem baixa longe
            double weight = std::exp(-minDist / 50.0);
            
            double thetaHigh = evaluateLegendreField(i, j, coeffsHigh, m_order);
            double thetaLow = evaluateLegendreField(i, j, coeffsLow, 3);
            
            // Interpolar considerando periodicidade
            double diff = thetaHigh - thetaLow;
            while (diff > M_PI_2) diff -= M_PI;
            while (diff < -M_PI_2) diff += M_PI;
            
            double theta = thetaLow + weight * diff;
            
            while (theta < 0) theta += M_PI;
            while (theta >= M_PI) theta -= M_PI;
            
            smoothed[idx] = theta;
        }
    }
    
    return smoothed;
}

}
