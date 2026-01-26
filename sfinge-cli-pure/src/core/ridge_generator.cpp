#include "ridge_generator.h"
#include <cmath>
#include <algorithm>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

RidgeGenerator::RidgeGenerator() 
    : m_width(0), m_height(0), m_coreX(0), m_coreY(0), m_rng(std::random_device{}()) {
    m_perm.resize(512);
    reseed();
}

void RidgeGenerator::reseed() {
    // Reinicializar RNG com nova seed do sistema
    std::random_device rd;
    m_rng.seed(rd());
    
    // Reinicializar tabela de permutação para Perlin noise
    std::vector<int> p(256);
    for (int i = 0; i < 256; ++i) p[i] = i;
    std::shuffle(p.begin(), p.end(), m_rng);
    for (int i = 0; i < 256; ++i) {
        m_perm[i] = p[i];
        m_perm[256 + i] = p[i];
    }
}

void RidgeGenerator::setMinutiaeParameters(const MinutiaeParameters& params) {
    m_minutiaeParams = params;
}

void RidgeGenerator::setCorePosition(double coreX, double coreY) {
    m_coreX = coreX;
    m_coreY = coreY;
}

void RidgeGenerator::setParameters(const RidgeParameters& params, const DensityParameters& densityParams,
                                    const RenderingParameters& renderParams, const VariationParameters& varParams) {
    m_params = params;
    m_densityParams = densityParams;
    m_renderParams = renderParams;
    m_varParams = varParams;
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
    
    int fil_x = std::max(b - x, 0);
    int fil_y = std::max(b - y, 0);
    int fil_width = std::min(std::min(filterSize, m_width + b - x), b + x) - fil_x;
    int fil_height = std::min(std::min(filterSize, m_height + b - y), b + y) - fil_y;
    
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
    int filterSize = m_params.gaborFilterSize * 2 + 1;
    GaborFilterCache cache(m_params.cacheDegrees, m_params.cacheFrequencies,
                          m_densityParams.minFrequency, m_densityParams.maxFrequency,
                          filterSize);
    
    m_ridgeMap.resize(m_width * m_height);
    
    // Reinicializar tabela de permutação para cada geração (evita padrões repetidos)
    std::vector<int> p(256);
    for (int i = 0; i < 256; ++i) p[i] = i;
    std::shuffle(p.begin(), p.end(), m_rng);
    for (int i = 0; i < 256; ++i) {
        m_perm[i] = p[i];
        m_perm[256 + i] = p[i];
    }
    
    // Inicialização esparsa aleatória - reduzido para menos minúcias naturais
    // 0.0005 = 0.05% dos pixels (era 0.001 = 0.1%)
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < m_width * m_height; ++i) {
        m_ridgeMap[i] = dist(m_rng) < 0.0005 ? 1.0f : 0.0f;
    }
    
    std::vector<float> newRidge(m_width * m_height, 0.0f);
    
    for (int iteration = 0; iteration < m_params.maxIterations; ++iteration) {
        std::fill(newRidge.begin(), newRidge.end(), 0.0f);
        
        for (int j = 0; j < m_height; ++j) {
            for (int i = 0; i < m_width; ++i) {
                int idx = j * m_width + i;
                
                if (m_shapeMap[idx] < 0.1f) {
                    continue;
                }
                
                double theta = m_orientationMap[idx];
                double thetaNorm = theta;
                if (thetaNorm < 0) thetaNorm += 2.0 * M_PI;
                
                double freq = m_densityMap[idx];
                
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
                
                newRidge[idx] = response > 0.0 ? 1.0f : 0.0f;
            }
        }
        
        // Copiar resultado da iteração (early stopping desabilitado para mais iterações)
        m_ridgeMap = newRidge;
    }
    
    for (int i = 0; i < m_width * m_height; ++i) {
        m_ridgeMap[i] *= m_shapeMap[i];
    }
}

Image RidgeGenerator::generate() {
    generateRidgeMap();
    
    // Gerar e aplicar minúcias - método original ou melhorado
    if (m_minutiaeParams.enableExplicitMinutiae || m_minutiaeParams.useContinuousPhase) {
        m_minutiaeGenerator.reseed();
        m_minutiaeGenerator.setParameters(m_minutiaeParams);
        m_minutiaeGenerator.setOrientationMap(m_orientationMap, m_width, m_height);
        m_minutiaeGenerator.setShapeMap(m_shapeMap);
        m_minutiaeGenerator.setRidgeMap(m_ridgeMap);
        m_minutiaeGenerator.setCorePosition(m_coreX, m_coreY);
        
        m_minutiaeGenerator.generateMinutiae();
        m_minutiaeGenerator.applyMinutiae(m_ridgeMap);
    }
    
    std::vector<float> rendered = renderFingerprint(m_ridgeMap);
    
    Image image(m_width, m_height);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            int gray = static_cast<int>(255 * (1.0f - rendered[idx]));
            gray = std::clamp(gray, 0, 255);
            image.setPixel(i, j, static_cast<uint8_t>(gray));
        }
    }
    
    return image;
}

std::vector<float> RidgeGenerator::renderFingerprint(const std::vector<float>& binaryRidge) {
    std::vector<float> rendered(m_width * m_height);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            float shapeValue = m_shapeMap[idx];
            
            if (shapeValue < 0.1f) {
                rendered[idx] = 0.0f;
                continue;
            }
            
            float sum = 0.0f;
            float weightSum = 0.0f;
            
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int ni = i + dx;
                    int nj = j + dy;
                    if (ni >= 0 && ni < m_width && nj >= 0 && nj < m_height) {
                        float weight = (dx == 0 && dy == 0) ? 0.5f : ((dx == 0 || dy == 0) ? 0.3f : 0.2f);
                        sum += binaryRidge[nj * m_width + ni] * weight;
                        weightSum += weight;
                    }
                }
            }
            
            float smoothed = (weightSum > 0.0f) ? (sum / weightSum) : 0.0f;
            smoothed = (smoothed - 0.5f) * 1.2f + 0.5f;
            smoothed = std::clamp(smoothed, 0.0f, 1.0f);
            
            rendered[idx] = smoothed * shapeValue;
        }
    }
    
    if (m_varParams.enableSkinCondition) {
        applySkinCondition(rendered);
    }
    
    if (m_varParams.enablePlasticDistortion) {
        applyElasticDistortion(rendered);
    }
    
    applyLocalContrastVariation(rendered);
    applyGaussianNoise(rendered, m_renderParams.ridgeNoiseAmplitude);
    
    return rendered;
}

double RidgeGenerator::fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double RidgeGenerator::lerp(double a, double b, double t) {
    return a + t * (b - a);
}

double RidgeGenerator::grad(int hash, double x, double y) {
    int h = hash & 7;
    double u = h < 4 ? x : y;
    double v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
}

double RidgeGenerator::perlinNoise(double x, double y) {
    // Implementação simplificada usando hash baseado em coordenadas
    // Evita problemas com tabela de permutação compartilhada
    
    auto hash = [](int x, int y) -> int {
        int n = x + y * 57;
        n = (n << 13) ^ n;
        return (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    };
    
    auto smoothNoise = [&](int ix, int iy) -> double {
        int corners = hash(ix - 1, iy - 1) + hash(ix + 1, iy - 1) + 
                      hash(ix - 1, iy + 1) + hash(ix + 1, iy + 1);
        int sides = hash(ix - 1, iy) + hash(ix + 1, iy) + 
                    hash(ix, iy - 1) + hash(ix, iy + 1);
        int center = hash(ix, iy);
        return (corners / 16.0 + sides / 8.0 + center / 4.0) / 2147483647.0;
    };
    
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    double fx = x - ix;
    double fy = y - iy;
    
    // Interpolação suave
    double u = fade(fx);
    double v = fade(fy);
    
    double n00 = smoothNoise(ix, iy);
    double n10 = smoothNoise(ix + 1, iy);
    double n01 = smoothNoise(ix, iy + 1);
    double n11 = smoothNoise(ix + 1, iy + 1);
    
    double nx0 = lerp(n00, n10, u);
    double nx1 = lerp(n01, n11, u);
    
    return lerp(nx0, nx1, v) * 2.0 - 1.0;  // Normalizar para [-1, 1]
}

void RidgeGenerator::applyGaussianNoise(std::vector<float>& image, double amplitude) {
    std::normal_distribution<double> noise(0.0, amplitude);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            if (m_shapeMap[idx] > 0.1f) {
                double perlin = perlinNoise(i * m_renderParams.ridgeNoiseFrequency,
                                            j * m_renderParams.ridgeNoiseFrequency);
                double gaussian = noise(m_rng);
                
                image[idx] += static_cast<float>(perlin * amplitude * 0.5 + gaussian);
                image[idx] = std::clamp(image[idx], 0.0f, 1.0f);
            }
        }
    }
}

void RidgeGenerator::applyLocalContrastVariation(std::vector<float>& image) {
    double freq = 0.02;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            if (m_shapeMap[idx] > 0.1f) {
                double noise = perlinNoise(i * freq, j * freq);
                double contrastFactor = 1.0 + noise * 0.3;
                
                float val = image[idx];
                val = static_cast<float>((val - 0.5) * contrastFactor + 0.5);
                image[idx] = std::clamp(val, 0.0f, 1.0f);
            }
        }
    }
}

void RidgeGenerator::applyElasticDistortion(std::vector<float>& image) {
    double strength = m_varParams.plasticDistortionStrength;
    double freq = 0.01 * m_varParams.plasticDistortionBumps;
    
    std::vector<float> distorted(m_width * m_height, 0.0f);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            
            if (m_shapeMap[idx] < 0.1f) {
                distorted[idx] = 0.0f;
                continue;
            }
            
            double dx = perlinNoise(i * freq, j * freq) * strength;
            double dy = perlinNoise(i * freq + 100, j * freq + 100) * strength;
            
            double srcX = i + dx;
            double srcY = j + dy;
            
            int x0 = static_cast<int>(std::floor(srcX));
            int y0 = static_cast<int>(std::floor(srcY));
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            
            double fx = srcX - x0;
            double fy = srcY - y0;
            
            x0 = std::clamp(x0, 0, m_width - 1);
            x1 = std::clamp(x1, 0, m_width - 1);
            y0 = std::clamp(y0, 0, m_height - 1);
            y1 = std::clamp(y1, 0, m_height - 1);
            
            float v00 = image[y0 * m_width + x0];
            float v10 = image[y0 * m_width + x1];
            float v01 = image[y1 * m_width + x0];
            float v11 = image[y1 * m_width + x1];
            
            float top = static_cast<float>(v00 * (1 - fx) + v10 * fx);
            float bottom = static_cast<float>(v01 * (1 - fx) + v11 * fx);
            distorted[idx] = static_cast<float>(top * (1 - fy) + bottom * fy);
        }
    }
    
    image = distorted;
}

void RidgeGenerator::applySkinCondition(std::vector<float>& image) {
    double factor = m_varParams.skinConditionFactor;
    if (std::abs(factor) < 0.01) return;
    
    std::vector<float> result(m_width * m_height);
    int kernelSize = 3;
    int halfK = kernelSize / 2;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            
            if (m_shapeMap[idx] < 0.1f) {
                result[idx] = 0.0f;
                continue;
            }
            
            float minVal = 1.0f, maxVal = 0.0f;
            
            for (int dy = -halfK; dy <= halfK; ++dy) {
                for (int dx = -halfK; dx <= halfK; ++dx) {
                    int ni = std::clamp(i + dx, 0, m_width - 1);
                    int nj = std::clamp(j + dy, 0, m_height - 1);
                    float val = image[nj * m_width + ni];
                    minVal = std::min(minVal, val);
                    maxVal = std::max(maxVal, val);
                }
            }
            
            float original = image[idx];
            if (factor > 0) {
                result[idx] = original + static_cast<float>(factor * (maxVal - original));
            } else {
                result[idx] = original + static_cast<float>((-factor) * (minVal - original));
            }
            result[idx] = std::clamp(result[idx], 0.0f, 1.0f);
        }
    }
    
    image = result;
}

}
