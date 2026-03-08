#include "density_generator.h"
#include "math_utils.h"
#include <algorithm>

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
        
        // Gerar ruído em baixa resolução com offset aleatório por fingerprint
        auto lowRes = renderClouds(res, res, m_params.zoom, m_params.amplify, m_noiseOffsetX, m_noiseOffsetY);
        
        // Interpolar para o tamanho real (resize bilinear)
        std::vector<float> upscaled = resizeNoise(lowRes, res, res, m_width, m_height);
        
        // Adicionar camada (1/3 de contribuição cada)
        for (int i = 0; i < m_width * m_height; ++i) {
            m_densityMap[i] += upscaled[i] / 3.0f;
        }
    }
    
    // Normalizar para range [minFreq, maxFreq] — SEM mascarar pela shape aqui.
    // O SFinGe original (DemoOCV) aplica a shape mask apenas no ridge map final
    // (master_fng = finger.mul(shape)). Mascarar a density força freq=0 fora da shape,
    // causando índice inválido no cache Gabor [minF, maxF] e artefatos nas bordas.
    for (int i = 0; i < m_width * m_height; ++i) {
        float normalized = m_densityMap[i];
        // Converter de [0,1] para [minFreq, maxFreq] — válido em todos os pixels
        m_densityMap[i] = m_params.minFrequency + normalized * (m_params.maxFrequency - m_params.minFrequency);
    }
}

std::vector<float> DensityGenerator::resizeNoise(const std::vector<float>& input,
                                                  int inWidth, int inHeight,
                                                  int outWidth, int outHeight) {
    // Interpolação bicúbica Catmull-Rom para suavidade superior à bilinear.
    // Evita gradientes lineares visíveis no mapa de densidade.

    auto sample = [&](int ix, int iy) -> float {
        ix = std::clamp(ix, 0, inWidth  - 1);
        iy = std::clamp(iy, 0, inHeight - 1);
        return input[iy * inWidth + ix];
    };

    // Spline Catmull-Rom 1D: (a, b, c, d) são amostras em t-1, t, t+1, t+2;
    // 't' é a fração [0,1] entre b e c.
    auto cubicInterp = [](float a, float b, float c, float d, float t) -> float {
        const float t2 = t * t;
        const float t3 = t2 * t;
        return 0.5f * ((2.0f * b)
                       + (-a + c) * t
                       + (2.0f * a - 5.0f * b + 4.0f * c - d) * t2
                       + (-a + 3.0f * b - 3.0f * c + d) * t3);
    };

    std::vector<float> output(outWidth * outHeight);

    const float xRatio = static_cast<float>(inWidth  - 1) / outWidth;
    const float yRatio = static_cast<float>(inHeight - 1) / outHeight;

    for (int y = 0; y < outHeight; ++y) {
        const float srcY = y * yRatio;
        const int iy = static_cast<int>(srcY);
        const float fy = srcY - iy;

        for (int x = 0; x < outWidth; ++x) {
            const float srcX = x * xRatio;
            const int ix = static_cast<int>(srcX);
            const float fx = srcX - ix;

            // Interpolar em X para cada uma das 4 linhas vizinhas
            float row[4];
            for (int dy = -1; dy <= 2; ++dy) {
                float a = sample(ix - 1, iy + dy);
                float b = sample(ix,     iy + dy);
                float c = sample(ix + 1, iy + dy);
                float d = sample(ix + 2, iy + dy);
                row[dy + 1] = cubicInterp(a, b, c, d, fx);
            }

            // Interpolar em Y com os resultados das 4 linhas
            float val = cubicInterp(row[0], row[1], row[2], row[3], fy);
            output[y * outWidth + x] = std::clamp(val, 0.0f, 1.0f);
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
