#include "gabor_filter.h"
#include <cmath>

namespace SFinGe {

GaborFilter::GaborFilter(int size, double sigma, double theta, double lambda, double gamma, double psi) {
    // Garantir tamanho ímpar
    if (size % 2 == 0) {
        size += 1;
    }
    m_size = size;
    m_kernel = createKernel(size, sigma, theta, lambda, gamma, psi);
}

std::vector<double> GaborFilter::createKernel(int size, double sigma, double theta, 
                                             double lambda, double gamma, double psi) {
    // Garantir que o tamanho seja ímpar para que halfSize funcione corretamente
    if (size % 2 == 0) {
        size += 1;
    }
    
    std::vector<double> kernel(size * size);
    
    double sigma_x = sigma;
    double sigma_y = sigma / gamma;
    
    int halfSize = size / 2;
    
    for (int y = -halfSize; y <= halfSize; ++y) {
        for (int x = -halfSize; x <= halfSize; ++x) {
            double x_theta = x * std::cos(theta) + y * std::sin(theta);
            double y_theta = -x * std::sin(theta) + y * std::cos(theta);
            
            double gaussian = std::exp(-0.5 * (x_theta * x_theta / (sigma_x * sigma_x) + 
                                               y_theta * y_theta / (sigma_y * sigma_y)));
            
            double sinusoid = std::cos(2.0 * M_PI * x_theta / lambda + psi);
            
            int idx = (y + halfSize) * size + (x + halfSize);
            kernel[idx] = gaussian * sinusoid;
        }
    }
    
    return kernel;
}

GaborFilterCache::GaborFilterCache(int cacheDegrees, int cacheFrequencies, 
                                   double minFreq, double maxFreq, int filterSize)
    : m_cacheDegrees(cacheDegrees)
    , m_cacheFrequencies(cacheFrequencies)
    , m_minFreq(minFreq)
    , m_maxFreq(maxFreq) {
    
    m_filters.reserve(cacheDegrees * cacheFrequencies);
    
    for (int i = 0; i < cacheDegrees; ++i) {
        for (int j = 0; j < cacheFrequencies; ++j) {
            // Convenção v4.0: theta do orientationMap já é perpendicular às cristas
            // NÃO adicionar +π/2 aqui
            double theta = indexToValue(i, 0, 2.0 * M_PI, cacheDegrees);
            double freq = indexToValue(j, minFreq, maxFreq, cacheFrequencies);
            double sigma = std::sqrt(-9.0 / (8.0 * freq * freq * std::log(0.001)));
            
            m_filters.emplace_back(filterSize, sigma, theta, 1.0 / freq, 1.0, 0.0);
        }
    }
}

const GaborFilter& GaborFilterCache::getFilter(int degreeIndex, int freqIndex) const {
    int index = degreeIndex * m_cacheFrequencies + freqIndex;
    return m_filters[index];
}

int GaborFilterCache::valueToIndex(double val, double min, double max, int n) const {
    double gap = (max - min) / n;
    return static_cast<int>(std::floor((val - min) / gap));
}

double GaborFilterCache::indexToValue(int index, double min, double max, int n) const {
    double gap = (max - min) / n;
    return gap * (static_cast<double>(index) + 0.5) + min;
}

}
