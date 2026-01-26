#include "frequency_field_smoother.h"
#include <cmath>
#include <algorithm>

namespace SFinGe {

FrequencyFieldSmoother::FrequencyFieldSmoother() {
}

FrequencyFieldSmoother::~FrequencyFieldSmoother() {
}

std::vector<std::vector<double>> FrequencyFieldSmoother::smooth(
    const std::vector<std::vector<double>>& frequencyField,
    double sigma
) {
    int height = frequencyField.size();
    int width = frequencyField[0].size();
    
    // Cria kernel Gaussiano
    auto kernel = createGaussianKernel(sigma);
    int kernelSize = kernel.size();
    int halfKernel = kernelSize / 2;
    
    // Aplica convolução
    auto smoothed = convolve(frequencyField, kernel);
    
    // Clampa para faixa válida
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            smoothed[y][x] = std::max(m_minFrequency, 
                                     std::min(m_maxFrequency, smoothed[y][x]));
        }
    }
    
    return smoothed;
}

void FrequencyFieldSmoother::setFrequencyRange(double minFreq, double maxFreq) {
    m_minFrequency = minFreq;
    m_maxFrequency = maxFreq;
}

std::vector<std::vector<double>> FrequencyFieldSmoother::createGaussianKernel(
    double sigma
) {
    // Kernel de tamanho ímpar, aproximadamente 6*sigma
    int size = static_cast<int>(6.0 * sigma);
    if (size % 2 == 0) size++;
    
    std::vector<std::vector<double>> kernel(
        size, std::vector<double>(size, 0.0)
    );
    
    int half = size / 2;
    double sum = 0.0;
    
    // Calcula valores do kernel
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            double dx = x - half;
            double dy = y - half;
            double value = std::exp(-(dx*dx + dy*dy) / (2.0 * sigma * sigma));
            kernel[y][x] = value;
            sum += value;
        }
    }
    
    // Normaliza
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            kernel[y][x] /= sum;
        }
    }
    
    return kernel;
}

std::vector<std::vector<double>> FrequencyFieldSmoother::convolve(
    const std::vector<std::vector<double>>& input,
    const std::vector<std::vector<double>>& kernel
) {
    int height = input.size();
    int width = input[0].size();
    int kernelSize = kernel.size();
    int halfKernel = kernelSize / 2;
    
    std::vector<std::vector<double>> output(
        height, std::vector<double>(width, 0.0)
    );
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double sum = 0.0;
            
            // Aplica kernel
            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = 0; kx < kernelSize; kx++) {
                    int iy = y + ky - halfKernel;
                    int ix = x + kx - halfKernel;
                    
                    // Verifica bordas
                    if (iy >= 0 && iy < height && ix >= 0 && ix < width) {
                        sum += input[iy][ix] * kernel[ky][kx];
                    }
                }
            }
            
            output[y][x] = sum;
        }
    }
    
    return output;
}

}
