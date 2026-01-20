#include "ridge_generator.h"
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>
#include <random>

namespace SFinGe {

RidgeGenerator::RidgeGenerator() 
    : m_width(0), m_height(0) {
}

void RidgeGenerator::setParameters(const RidgeParameters& params, const DensityParameters& densityParams) {
    m_params = params;
    m_densityParams = densityParams;
}

void RidgeGenerator::setOrientationMap(const std::vector<double>& orientationMap, int width, int height) {
    m_orientationMap = orientationMap;
    m_width = width;
    m_height = height;
}

void RidgeGenerator::setDensityMap(const std::vector<float>& densityMap) {
    m_densityMap = densityMap;
}

void RidgeGenerator::setShapeMap(const std::vector<float>& shapeMap) {
    m_shapeMap = shapeMap;
}

double RidgeGenerator::applyFilter(const GaborFilter& filter, int x, int y, 
                                   const std::vector<float>& image) {
    const auto& kernel = filter.getKernel();
    int filterSize = filter.getSize();
    int b = filterSize / 2;
    
    // Implementação igual ao SFINGE original (FinGe.cpp linha 216-218)
    // Calcula retângulos de overlap corretos para bordas
    
    // Região do filtro a ser usada
    int fil_x = std::max(b - x, 0);
    int fil_y = std::max(b - y, 0);
    int fil_width = std::min(std::min(filterSize, m_width + b - x), b + x) - fil_x;
    int fil_height = std::min(std::min(filterSize, m_height + b - y), b + y) - fil_y;
    
    // Região da imagem correspondente
    int img_x = std::max(x - b, 0);
    int img_y = std::max(y - b, 0);
    
    double sum = 0.0;
    
    for (int fy = 0; fy < fil_height; ++fy) {
        for (int fx = 0; fx < fil_width; ++fx) {
            int kernelIdx = (fil_y + fy) * filterSize + (fil_x + fx);
            int imgIdx = (img_y + fy) * m_width + (img_x + fx);
            sum += kernel[kernelIdx] * image[imgIdx];
        }
    }
    
    return sum;
}

void RidgeGenerator::generateRidgeMap() {
    // Garantir tamanho ímpar do filtro Gabor (2*size+1 é sempre ímpar)
    int filterSize = m_params.gaborFilterSize * 2 + 1;
    GaborFilterCache cache(m_params.cacheDegrees, m_params.cacheFrequencies,
                          m_densityParams.minFrequency, m_densityParams.maxFrequency,
                          filterSize);
    
    m_ridgeMap.resize(m_width * m_height);
    
    // Inicialização esparsa (0.1% como no SFINGE original)
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < m_width * m_height; ++i) {
        m_ridgeMap[i] = rng->generateDouble() < 0.001 ? 1.0f : 0.0f;
    }
    
    std::vector<float> prevRidge = m_ridgeMap;
    
    //TODO_APS: Geração de cristas e vales usando orientação e densidade (filtros Gabor iterativos)
    std::vector<float> newRidge(m_width * m_height, 0.0f);
    
    for (int iteration = 0; iteration < m_params.maxIterations; ++iteration) {
        std::fill(newRidge.begin(), newRidge.end(), 0.0f);
        
        for (int j = 0; j < m_height; ++j) {
            for (int i = 0; i < m_width; ++i) {
                int idx = j * m_width + i;
                
                // OTIMIZAÇÃO: Pular pixels fora da shape mask (background)
                if (m_shapeMap[idx] < 0.1f) {
                    continue;
                }
                
                double theta = m_orientationMap[idx];
                // Normalizar para [0, 2π) como no SFINGE original
                double thetaNorm = theta;
                if (thetaNorm < 0) thetaNorm += 2.0 * M_PI;
                
                double freq = m_densityMap[idx];
                
                // Mapear para índice do cache [0, cacheDegrees)
                // Cache usa theta em [0, 2π), então usamos theta direto
                int degIdx = std::min(static_cast<int>(thetaNorm / (2.0 * M_PI) * m_params.cacheDegrees), 
                                     m_params.cacheDegrees - 1);
                int freqIdx = std::min(static_cast<int>((freq - m_densityParams.minFrequency) / 
                                      (m_densityParams.maxFrequency - m_densityParams.minFrequency) * 
                                      m_params.cacheFrequencies), 
                                     m_params.cacheFrequencies - 1);
                
                if (degIdx < 0) degIdx = 0;
                if (freqIdx < 0) freqIdx = 0;
                
                const GaborFilter& filter = cache.getFilter(degIdx, freqIdx);
                double response = applyFilter(filter, i, j, m_ridgeMap);
                
                // Binarização suave
                newRidge[idx] = response > 0.0 ? 1.0f : 0.0f;
            }
        }
        
        // Early stopping: verificar convergência a cada 5 iterações
        if (iteration % 5 == 4 || iteration == m_params.maxIterations - 1) {
            int changes = 0;
            for (int i = 0; i < m_width * m_height; ++i) {
                if (m_ridgeMap[i] != newRidge[i]) {
                    changes++;
                }
            }
            
            double changeRatio = static_cast<double>(changes) / (m_width * m_height);
            m_ridgeMap = newRidge;
            
            // Convergência: 0.5% de mudança
            if (changeRatio < 0.005) {
                break;
            }
        } else {
            m_ridgeMap = newRidge;
        }
    }
    
    for (int i = 0; i < m_width * m_height; ++i) {
        m_ridgeMap[i] *= m_shapeMap[i];
    }
}

QImage RidgeGenerator::generate() {
    generateRidgeMap();
    
    // Aplicar rendering realista (suavização e ruído)
    std::vector<float> rendered = renderFingerprint(m_ridgeMap);
    
    QImage image(m_width, m_height, QImage::Format_Grayscale8);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            // Inverter: cristas = escuro (0), vales = claro (255)
            int idx = j * m_width + i;
            int gray = static_cast<int>(255 * (1.0f - rendered[idx]));
            gray = std::clamp(gray, 0, 255);
            image.setPixel(i, j, qRgb(gray, gray, gray));
        }
    }
    
    return image;
}

std::vector<float> RidgeGenerator::renderFingerprint(const std::vector<float>& binaryRidge) {
    // Etapa de rendering OTIMIZADA: converter padrão binário em imagem realista
    std::vector<float> rendered(m_width * m_height);
    
    // 1. Suavização 3x3 completa para evitar buracos
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            float shapeValue = m_shapeMap[idx];
            
            // OTIMIZAÇÃO: Pular pixels fora da shape
            if (shapeValue < 0.1f) {
                rendered[idx] = 0.0f;
                continue;
            }
            
            // Suavização 3x3 normalizada corretamente para bordas
            float sum = 0.0f;
            float weightSum = 0.0f;
            
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int ni = i + dx;
                    int nj = j + dy;
                    if (ni >= 0 && ni < m_width && nj >= 0 && nj < m_height) {
                        // Peso gaussiano simplificado: centro = 0.5, bordas = 0.3, cantos = 0.2
                        float weight = (dx == 0 && dy == 0) ? 0.5f : ((dx == 0 || dy == 0) ? 0.3f : 0.2f);
                        sum += binaryRidge[nj * m_width + ni] * weight;
                        weightSum += weight;
                    }
                }
            }
            
            float smoothed = (weightSum > 0.0f) ? (sum / weightSum) : 0.0f;
            
            // Aplicar contraste moderado
            smoothed = (smoothed - 0.5f) * 1.2f + 0.5f;
            smoothed = std::clamp(smoothed, 0.0f, 1.0f);
            
            rendered[idx] = smoothed * shapeValue;
        }
    }
    
    return rendered;
}

}
