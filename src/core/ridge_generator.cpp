#include "ridge_generator.h"
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>
#include <random>

namespace SFinGe {

RidgeGenerator::RidgeGenerator() 
    : m_width(0), m_height(0), m_rng(std::random_device{}()) {
    // Inicializar tabela de permutação para Perlin noise
    m_perm.resize(512);
    std::vector<int> p(256);
    for (int i = 0; i < 256; ++i) p[i] = i;
    std::shuffle(p.begin(), p.end(), m_rng);
    for (int i = 0; i < 256; ++i) {
        m_perm[i] = p[i];
        m_perm[256 + i] = p[i];
    }
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
    std::vector<float> rendered(m_width * m_height);
    
    // 1. Suavização 3x3 completa para evitar buracos
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
    
    // 2. Aplicar efeitos de realismo
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

// Funções auxiliares para Perlin Noise
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
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    
    x -= std::floor(x);
    y -= std::floor(y);
    
    double u = fade(x);
    double v = fade(y);
    
    int A = m_perm[X] + Y;
    int B = m_perm[X + 1] + Y;
    
    return lerp(lerp(grad(m_perm[A], x, y), grad(m_perm[B], x - 1, y), u),
                lerp(grad(m_perm[A + 1], x, y - 1), grad(m_perm[B + 1], x - 1, y - 1), u), v);
}

void RidgeGenerator::applyGaussianNoise(std::vector<float>& image, double amplitude) {
    std::normal_distribution<double> noise(0.0, amplitude);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            if (m_shapeMap[idx] > 0.1f) {
                // Ruído Perlin para variação suave + ruído gaussiano para detalhes
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
    // Variação de contraste baseada em Perlin noise de baixa frequência
    double freq = 0.02;
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            if (m_shapeMap[idx] > 0.1f) {
                // Gerar fator de contraste local (0.7 a 1.3)
                double noise = perlinNoise(i * freq, j * freq);
                double contrastFactor = 1.0 + noise * 0.3;
                
                // Aplicar contraste em torno de 0.5
                float val = image[idx];
                val = static_cast<float>((val - 0.5) * contrastFactor + 0.5);
                image[idx] = std::clamp(val, 0.0f, 1.0f);
            }
        }
    }
}

void RidgeGenerator::applyElasticDistortion(std::vector<float>& image) {
    // Distorção elástica usando campos de deslocamento baseados em Perlin
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
            
            // Deslocamento baseado em Perlin noise
            double dx = perlinNoise(i * freq, j * freq) * strength;
            double dy = perlinNoise(i * freq + 100, j * freq + 100) * strength;
            
            // Coordenadas de origem com interpolação bilinear
            double srcX = i + dx;
            double srcY = j + dy;
            
            int x0 = static_cast<int>(std::floor(srcX));
            int y0 = static_cast<int>(std::floor(srcY));
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            
            double fx = srcX - x0;
            double fy = srcY - y0;
            
            // Clamp para bordas
            x0 = std::clamp(x0, 0, m_width - 1);
            x1 = std::clamp(x1, 0, m_width - 1);
            y0 = std::clamp(y0, 0, m_height - 1);
            y1 = std::clamp(y1, 0, m_height - 1);
            
            // Interpolação bilinear
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
    // Simula pele úmida (dilatação) ou seca (erosão)
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
            
            // factor > 0: úmida (dilata cristas = mais escuro = max)
            // factor < 0: seca (erode cristas = menos escuro = min)
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
