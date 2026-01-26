#include "quality_mask_generator.h"
#include <cmath>
#include <complex>

namespace SFinGe {

QualityMaskGenerator::QualityMaskGenerator() {
}

QualityMaskGenerator::~QualityMaskGenerator() {
}

std::vector<std::vector<double>> QualityMaskGenerator::generate(
    const std::vector<std::vector<double>>& orientationField
) {
    int height = orientationField.size();
    int width = orientationField[0].size();
    
    std::vector<std::vector<double>> qualityMask(
        height, std::vector<double>(width, 0.0)
    );
    
    int halfWindow = m_windowSize / 2;
    
    // Para cada pixel, calcula coerência local
    for (int y = halfWindow; y < height - halfWindow; y++) {
        for (int x = halfWindow; x < width - halfWindow; x++) {
            double coherence = computeLocalCoherence(
                orientationField, x, y
            );
            
            // Mapeia coerência para qualidade
            // Alta coerência = alta qualidade
            qualityMask[y][x] = coherence;
        }
    }
    
    // Preenche bordas com qualidade média
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (qualityMask[y][x] == 0.0) {
                qualityMask[y][x] = m_coherenceThreshold;
            }
        }
    }
    
    return qualityMask;
}

void QualityMaskGenerator::setCoherenceThreshold(double threshold) {
    m_coherenceThreshold = threshold;
}

void QualityMaskGenerator::setWindowSize(int size) {
    m_windowSize = size;
}

double QualityMaskGenerator::computeLocalCoherence(
    const std::vector<std::vector<double>>& orientationField,
    int centerX, int centerY
) {
    int halfWindow = m_windowSize / 2;
    
    // Usa representação complexa para média circular
    std::complex<double> sum(0.0, 0.0);
    int count = 0;
    
    for (int dy = -halfWindow; dy <= halfWindow; dy++) {
        for (int dx = -halfWindow; dx <= halfWindow; dx++) {
            int y = centerY + dy;
            int x = centerX + dx;
            
            double theta = orientationField[y][x];
            
            // Converte para complexo (dobra ângulo por simetria 180°)
            std::complex<double> c = std::exp(
                std::complex<double>(0.0, 2.0 * theta)
            );
            
            sum += c;
            count++;
        }
    }
    
    // Coerência = magnitude da média normalizada
    std::complex<double> avg = sum / static_cast<double>(count);
    double coherence = std::abs(avg);
    
    return coherence;
}

}
