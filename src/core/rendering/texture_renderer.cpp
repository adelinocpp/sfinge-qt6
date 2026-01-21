#include "texture_renderer.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

namespace SFinGe {

TextureRenderer::TextureRenderer(const RenderingParameters& params, int width, int height, unsigned int seed)
    : m_params(params)
    , m_width(width)
    , m_height(height)
    , m_perlin(std::make_unique<PerlinNoise>(seed))
    , m_rng(seed)
{
}

std::vector<float> TextureRenderer::render(const std::vector<float>& ridgeMap,
                                           const std::vector<float>& shapeMap) const {
    qDebug() << "[TextureRenderer] Iniciando pipeline de renderização";
    qDebug() << "[TextureRenderer] Dimensões:" << m_width << "x" << m_height;
    
    // Passo 1: Gerar fundo com vinheta e ruído
    auto background = generateBackground();
    qDebug() << "[TextureRenderer] Fundo gerado";
    
    // Passo 2: Aplicar textura às cristas
    auto textured = applyTexture(ridgeMap);
    qDebug() << "[TextureRenderer] Textura aplicada";
    
    // Passo 3: Criar máscara de cristas
    std::vector<bool> ridgeMask(m_width * m_height);
    for (size_t i = 0; i < ridgeMap.size(); ++i) {
        ridgeMask[i] = ridgeMap[i] > 0.5f;
    }
    
    // Passo 4: Adicionar poros (se habilitado)
    if (m_params.enablePores) {
        textured = addPores(textured, ridgeMask);
        qDebug() << "[TextureRenderer] Poros adicionados";
    }
    
    // Passo 5: Combinar com fundo usando shapeMap como máscara alfa
    std::vector<float> combined(m_width * m_height);
    for (size_t i = 0; i < combined.size(); ++i) {
        float alpha = shapeMap.empty() ? 1.0f : shapeMap[i];
        combined[i] = textured[i] * alpha + background[i] * (1.0f - alpha);
    }
    qDebug() << "[TextureRenderer] Imagem combinada com fundo";
    
    // Passo 6: Aplicar blur final
    if (m_params.finalBlurSigma > 0) {
        combined = applyGaussianBlur(combined, m_params.finalBlurSigma);
        qDebug() << "[TextureRenderer] Blur final aplicado (sigma=" << m_params.finalBlurSigma << ")";
    }
    
    // Passo 7: Normalizar contraste
    combined = normalizeContrast(combined, shapeMap);
    qDebug() << "[TextureRenderer] Contraste normalizado";
    
    return combined;
}

std::vector<float> TextureRenderer::generateBackground() const {
    std::vector<float> background(m_width * m_height);
    
    // Cor base do fundo (cinza claro)
    const float baseColor = 0.92f;
    
    // Centro da imagem para vinheta
    float centerX = m_width / 2.0f;
    float centerY = m_height / 2.0f;
    float maxDist = std::sqrt(centerX * centerX + centerY * centerY);
    
    // Gerar ruído de baixa frequência para o fundo
    auto noiseField = m_perlin->fractal(m_width, m_height, 
                                        m_params.backgroundNoiseFrequency,
                                        4, 0.5, 2.0);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            
            // Calcular vinheta (escurecimento nas bordas)
            float dx = i - centerX;
            float dy = j - centerY;
            float dist = std::sqrt(dx * dx + dy * dy) / maxDist;
            float vignette = 1.0f - 0.3f * dist * dist;
            
            // Combinar cor base, vinheta e ruído
            float noise = static_cast<float>(noiseField[idx] - 0.5) * 2.0f * m_params.backgroundNoiseAmplitude;
            background[idx] = std::clamp(baseColor * vignette + noise, 0.0f, 1.0f);
        }
    }
    
    return background;
}

std::vector<float> TextureRenderer::applyTexture(const std::vector<float>& ridges) const {
    std::vector<float> textured(m_width * m_height);
    
    // Gerar campos de ruído separados para cristas e vales
    auto ridgeNoise = m_perlin->fractal(m_width, m_height,
                                        m_params.ridgeNoiseFrequency,
                                        3, 0.5, 2.0);
    auto valleyNoise = m_perlin->fractal(m_width, m_height,
                                         m_params.valleyNoiseFrequency,
                                         3, 0.5, 2.0);
    
    // Valores base para cristas (escuras) e vales (claros)
    const float ridgeBase = 0.15f;  // Cristas escuras
    const float valleyBase = 0.85f; // Vales claros
    
    for (size_t i = 0; i < ridges.size(); ++i) {
        float ridgeValue = ridges[i];
        
        if (ridgeValue > 0.5f) {
            // Pixel de crista
            float noise = static_cast<float>(ridgeNoise[i] - 0.5) * 2.0f * m_params.ridgeNoiseAmplitude;
            textured[i] = std::clamp(ridgeBase + noise, 0.0f, 1.0f);
        } else {
            // Pixel de vale
            float noise = static_cast<float>(valleyNoise[i] - 0.5) * 2.0f * m_params.valleyNoiseAmplitude;
            textured[i] = std::clamp(valleyBase + noise, 0.0f, 1.0f);
        }
    }
    
    return textured;
}

std::vector<float> TextureRenderer::addPores(const std::vector<float>& texturedRidges,
                                              const std::vector<bool>& ridgeMask) const {
    std::vector<float> result = texturedRidges;
    
    // Contar pixels de crista
    int ridgePixelCount = 0;
    std::vector<int> ridgeIndices;
    for (size_t i = 0; i < ridgeMask.size(); ++i) {
        if (ridgeMask[i]) {
            ridgePixelCount++;
            ridgeIndices.push_back(static_cast<int>(i));
        }
    }
    
    if (ridgeIndices.empty()) {
        return result;
    }
    
    // Calcular número de poros
    int numPores = static_cast<int>(ridgePixelCount * m_params.poreDensity);
    qDebug() << "[TextureRenderer] Adicionando" << numPores << "poros em" << ridgePixelCount << "pixels de crista";
    
    // Distribuições para posição e tamanho dos poros
    std::uniform_int_distribution<int> indexDist(0, static_cast<int>(ridgeIndices.size()) - 1);
    std::uniform_real_distribution<double> sizeDist(m_params.minPoreSize, m_params.maxPoreSize);
    std::uniform_real_distribution<double> intensityDist(m_params.minPoreIntensity, m_params.maxPoreIntensity);
    
    // Adicionar poros
    for (int p = 0; p < numPores; ++p) {
        int idx = ridgeIndices[indexDist(m_rng)];
        int x = idx % m_width;
        int y = idx / m_width;
        
        double poreSize = sizeDist(m_rng);
        double poreIntensity = intensityDist(m_rng);
        
        // Aplicar poro como um pequeno ponto mais claro
        int radius = static_cast<int>(std::ceil(poreSize));
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int nx = x + dx;
                int ny = y + dy;
                
                if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                    double dist = std::sqrt(dx * dx + dy * dy);
                    if (dist <= poreSize) {
                        int nidx = ny * m_width + nx;
                        // Clarear o pixel (simula reflexo de suor)
                        result[nidx] = std::clamp(result[nidx] + static_cast<float>(poreIntensity), 0.0f, 1.0f);
                    }
                }
            }
        }
    }
    
    return result;
}

std::vector<float> TextureRenderer::normalizeContrast(const std::vector<float>& image,
                                                       const std::vector<float>& shapeMap) const {
    std::vector<float> result = image;
    
    // Coletar valores dentro da região da impressão digital
    std::vector<float> values;
    for (size_t i = 0; i < image.size(); ++i) {
        float alpha = shapeMap.empty() ? 1.0f : shapeMap[i];
        if (alpha > 0.5f) {
            values.push_back(image[i]);
        }
    }
    
    if (values.empty()) {
        return result;
    }
    
    // Ordenar para calcular percentis
    std::sort(values.begin(), values.end());
    
    // Calcular percentis inferior e superior
    size_t lowerIdx = static_cast<size_t>(values.size() * m_params.contrastPercentileLower / 100.0);
    size_t upperIdx = static_cast<size_t>(values.size() * m_params.contrastPercentileUpper / 100.0);
    upperIdx = std::min(upperIdx, values.size() - 1);
    
    float lowerVal = values[lowerIdx];
    float upperVal = values[upperIdx];
    
    if (upperVal <= lowerVal) {
        return result;
    }
    
    // Normalizar contraste
    float range = upperVal - lowerVal;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = std::clamp((result[i] - lowerVal) / range, 0.0f, 1.0f);
    }
    
    return result;
}

std::vector<float> TextureRenderer::applyGaussianBlur(const std::vector<float>& image, double sigma) const {
    if (sigma <= 0) {
        return image;
    }
    
    std::vector<float> result = image;
    
    // Calcular tamanho do kernel (6*sigma para capturar 99.7% da distribuição)
    int kernelSize = static_cast<int>(std::ceil(sigma * 6.0));
    if (kernelSize % 2 == 0) kernelSize++;
    int halfKernel = kernelSize / 2;
    
    // Criar kernel gaussiano 1D
    std::vector<float> kernel(kernelSize);
    float kernelSum = 0.0f;
    for (int i = 0; i < kernelSize; ++i) {
        float x = static_cast<float>(i - halfKernel);
        kernel[i] = std::exp(-x * x / (2.0f * sigma * sigma));
        kernelSum += kernel[i];
    }
    // Normalizar
    for (auto& k : kernel) {
        k /= kernelSum;
    }
    
    // Aplicar blur separável (horizontal)
    std::vector<float> temp(m_width * m_height);
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            float sum = 0.0f;
            for (int k = -halfKernel; k <= halfKernel; ++k) {
                int ni = std::clamp(i + k, 0, m_width - 1);
                sum += image[j * m_width + ni] * kernel[k + halfKernel];
            }
            temp[j * m_width + i] = sum;
        }
    }
    
    // Aplicar blur separável (vertical)
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            float sum = 0.0f;
            for (int k = -halfKernel; k <= halfKernel; ++k) {
                int nj = std::clamp(j + k, 0, m_height - 1);
                sum += temp[nj * m_width + i] * kernel[k + halfKernel];
            }
            result[j * m_width + i] = sum;
        }
    }
    
    return result;
}

} // namespace SFinGe
