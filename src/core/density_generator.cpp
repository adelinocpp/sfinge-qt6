#include "density_generator.h"
#include "math_utils.h"

namespace SFinGe {

DensityGenerator::DensityGenerator() 
    : m_width(0), m_height(0) {
}

void DensityGenerator::setParameters(const DensityParameters& params) {
    m_params = params;
}

void DensityGenerator::setShapeMap(const std::vector<float>& shapeMap, int width, int height) {
    m_shapeMap = shapeMap;
    m_width = width;
    m_height = height;
}

void DensityGenerator::generateDensityMap() {
    // Algoritmo do SFINGE original: múltiplas camadas de ruído em diferentes resoluções
    // Isso gera uma densidade mais realista variando a frequência das cristas
    
    m_densityMap.resize(m_width * m_height, 0.0f);
    
    // 3 camadas com resoluções diferentes (baseado no código original)
    int resolutions[] = {5, 6, 7};
    
    for (int layer = 0; layer < 3; ++layer) {
        int res = resolutions[layer];
        
        // Gerar ruído em baixa resolução
        auto lowRes = renderClouds(res, res, m_params.zoom, m_params.amplify);
        
        // Interpolar para o tamanho real (resize bilinear)
        std::vector<float> upscaled = resizeNoise(lowRes, res, res, m_width, m_height);
        
        // Adicionar camada (1/3 de contribuição cada)
        for (int i = 0; i < m_width * m_height; ++i) {
            m_densityMap[i] += upscaled[i] / 3.0f;
        }
    }
    
    // Normalizar para range [minFreq, maxFreq]
    for (int i = 0; i < m_width * m_height; ++i) {
        float normalized = m_densityMap[i];
        // Converter de [0,1] para [minFreq, maxFreq]
        m_densityMap[i] = m_params.minFrequency + normalized * (m_params.maxFrequency - m_params.minFrequency);
        // Aplicar máscara de forma
        m_densityMap[i] *= m_shapeMap[i];
    }
}

std::vector<float> DensityGenerator::resizeNoise(const std::vector<float>& input, 
                                                  int inWidth, int inHeight,
                                                  int outWidth, int outHeight) {
    std::vector<float> output(outWidth * outHeight);
    
    float xRatio = static_cast<float>(inWidth - 1) / outWidth;
    float yRatio = static_cast<float>(inHeight - 1) / outHeight;
    
    for (int y = 0; y < outHeight; ++y) {
        for (int x = 0; x < outWidth; ++x) {
            float srcX = x * xRatio;
            float srcY = y * yRatio;
            
            int x1 = static_cast<int>(srcX);
            int y1 = static_cast<int>(srcY);
            int x2 = std::min(x1 + 1, inWidth - 1);
            int y2 = std::min(y1 + 1, inHeight - 1);
            
            float fx = srcX - x1;
            float fy = srcY - y1;
            
            // Interpolação bilinear
            float v1 = input[y1 * inWidth + x1];
            float v2 = input[y1 * inWidth + x2];
            float v3 = input[y2 * inWidth + x1];
            float v4 = input[y2 * inWidth + x2];
            
            float i1 = v1 * (1 - fx) + v2 * fx;
            float i2 = v3 * (1 - fx) + v4 * fx;
            
            output[y * outWidth + x] = i1 * (1 - fy) + i2 * fy;
        }
    }
    
    return output;
}

QImage DensityGenerator::generate() {
    generateDensityMap();
    
    float minDensity = 1e10f;
    float maxDensity = -1e10f;
    for (int i = 0; i < m_width * m_height; ++i) {
        if (m_shapeMap[i] > 0.5) {
            minDensity = std::min(minDensity, m_densityMap[i]);
            maxDensity = std::max(maxDensity, m_densityMap[i]);
        }
    }
    
    float range = maxDensity - minDensity;
    if (range < 0.0001f) range = 1.0f;
    
    QImage image(m_width, m_height, QImage::Format_Grayscale8);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            if (m_shapeMap[idx] > 0.5) {
                float normalized = (m_densityMap[idx] - minDensity) / range;
                int gray = static_cast<int>(normalized * 255.0f);
                gray = std::min(255, std::max(0, gray));
                image.setPixel(i, j, qRgb(gray, gray, gray));
            } else {
                image.setPixel(i, j, qRgb(255, 255, 255));
            }
        }
    }
    
    return image;
}

}
