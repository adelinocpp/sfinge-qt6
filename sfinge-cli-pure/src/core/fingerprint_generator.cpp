#include "fingerprint_generator.h"
#include "math_utils.h"
#include "rendering/texture_renderer.h"
#include <cmath>
#include <algorithm>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

FingerprintGenerator::FingerprintGenerator()
    : m_width(0), m_height(0), m_rng(std::random_device{}()) {
}

void FingerprintGenerator::setParameters(const FingerprintParameters& params) {
    m_params = params;
    m_width = params.shape.left + params.shape.right;
    m_height = params.shape.top + params.shape.middle + params.shape.bottom;
}

void FingerprintGenerator::setSingularPoints(const SingularPoints& points) {
    m_points = points;
}

// Modelo de 4 seções com meias-elipses — port de shape_generator.cpp (sfinge-qt6)
// sT > sL → ponta apontada; sM curto → não retangular
void FingerprintGenerator::generateShapeMap() {
    m_shapeMap.resize(m_width * m_height, 0.0f);

    const float cx      = static_cast<float>(m_params.shape.left);
    const float cy_top  = static_cast<float>(m_params.shape.top);
    const float cy_bot  = static_cast<float>(m_params.shape.top + m_params.shape.middle);
    const float ax_left  = static_cast<float>(m_params.shape.left);
    const float ax_right = static_cast<float>(m_params.shape.right);
    const float ay_top   = static_cast<float>(m_params.shape.top);
    const float ay_bot   = static_cast<float>(m_params.shape.bottom);
    const float feather  = 0.08f;  // 8% de degradê nas bordas

    for (int y = 0; y < m_height; ++y) {
        const float fy = static_cast<float>(y);
        for (int x = 0; x < m_width; ++x) {
            const float fx = static_cast<float>(x);
            float alpha = 0.0f;

            if (fy < cy_top) {
                // Seção superior: meias-elipses centradas em (cx, cy_top)
                const float dx = fx - cx;
                const float dy = fy - cy_top;
                const float ax = (dx <= 0.0f) ? ax_left : ax_right;
                const float v  = std::sqrt((dx/ax)*(dx/ax) + (dy/ay_top)*(dy/ay_top));
                if (v < 1.0f - feather)     alpha = 1.0f;
                else if (v < 1.0f)          alpha = (1.0f - v) / feather;
            } else if (fy > cy_bot) {
                // Seção inferior: meias-elipses centradas em (cx, cy_bot)
                const float dx = fx - cx;
                const float dy = fy - cy_bot;
                const float ax = (dx <= 0.0f) ? ax_left : ax_right;
                const float v  = std::sqrt((dx/ax)*(dx/ax) + (dy/ay_bot)*(dy/ay_bot));
                if (v < 1.0f - feather)     alpha = 1.0f;
                else if (v < 1.0f)          alpha = (1.0f - v) / feather;
            } else {
                // Seção média: retângulo com degradê lateral
                const float feather_px = feather * std::min(ax_left, ax_right);
                const float distEdge   = std::min(fx, static_cast<float>(m_width - 1) - fx);
                alpha = std::min(1.0f, distEdge / feather_px);
            }
            m_shapeMap[y * m_width + x] = alpha;
        }
    }
}

// Mapa de densidade com Perlin multi-camada — port de density_generator.cpp (sfinge-qt6)
// Usa renderClouds() de math_utils.h + bicúbico Catmull-Rom (SEM mascarar com shapeMap)
void FingerprintGenerator::generateDensityMap() {
    m_densityMap.resize(m_width * m_height, 0.0f);

    // Upscale bicúbico Catmull-Rom (port de density_generator.cpp::resizeNoise)
    auto resizeNoise = [&](const std::vector<float>& input,
                           int inW, int inH, int outW, int outH) -> std::vector<float> {
        auto sample = [&](int ix, int iy) -> float {
            ix = std::clamp(ix, 0, inW - 1);
            iy = std::clamp(iy, 0, inH - 1);
            return input[iy * inW + ix];
        };
        auto cubic = [](float a, float b, float c, float d, float t) -> float {
            float t2 = t*t, t3 = t2*t;
            return 0.5f * (2.f*b + (-a+c)*t + (2.f*a-5.f*b+4.f*c-d)*t2 + (-a+3.f*b-3.f*c+d)*t3);
        };
        std::vector<float> out(outW * outH);
        const float xr = static_cast<float>(inW - 1) / outW;
        const float yr = static_cast<float>(inH - 1) / outH;
        for (int y = 0; y < outH; ++y) {
            const float sy = y * yr;
            const int   iy = static_cast<int>(sy);
            const float fy = sy - iy;
            for (int x = 0; x < outW; ++x) {
                const float sx = x * xr;
                const int   ix = static_cast<int>(sx);
                const float fx = sx - ix;
                float row[4];
                for (int dy = -1; dy <= 2; ++dy)
                    row[dy+1] = cubic(sample(ix-1,iy+dy), sample(ix,iy+dy),
                                      sample(ix+1,iy+dy), sample(ix+2,iy+dy), fx);
                out[y * outW + x] = std::clamp(cubic(row[0],row[1],row[2],row[3],fy), 0.f, 1.f);
            }
        }
        return out;
    };

    // 3 camadas em resoluções baixas → upscale bicúbico → média (SFinGe original)
    const int resolutions[] = {5, 6, 7};
    for (int layer = 0; layer < 3; ++layer) {
        const int res = resolutions[layer];
        auto lowRes   = renderClouds(res, res, m_params.density.zoom, m_params.density.amplify,
                                     m_densityNoiseOffsetX, m_densityNoiseOffsetY);
        auto upscaled = resizeNoise(lowRes, res, res, m_width, m_height);
        for (int i = 0; i < m_width * m_height; ++i)
            m_densityMap[i] += upscaled[i] / 3.0f;
    }

    // Converter [0,1] → [minFreq, maxFreq] — SEM mascarar com shapeMap
    // (shape mask aplicada apenas no ridge final, como no SFinGe original DemoOCV)
    const float minF = m_params.density.minFrequency;
    const float maxF = m_params.density.maxFrequency;
    for (int i = 0; i < m_width * m_height; ++i)
        m_densityMap[i] = minF + m_densityMap[i] * (maxF - minF);
}

void FingerprintGenerator::generateOrientationMap() {
    m_orientationGenerator.setShapeMap(m_shapeMap, m_width, m_height);
    m_orientationGenerator.setSingularPoints(m_points);
    m_orientationGenerator.setParameters(m_params.orientation);
    m_orientationGenerator.setFingerprintClass(m_params.classification.fingerprintClass);

    m_orientationGenerator.generate();
    m_orientationMap = m_orientationGenerator.getOrientationMap();
}

Image FingerprintGenerator::generateFingerprint() {
    // Reinicializar RNG para isolamento entre threads
    std::random_device rd;
    m_rng.seed(rd());

    m_orientationGenerator.reseed();

    generateShapeMap();

    // Randomizar offset do mapa de densidade — evita padrão determinístico
    {
        std::uniform_real_distribution<double> off(-50.0, 50.0);
        m_densityNoiseOffsetX = off(m_rng);
        m_densityNoiseOffsetY = off(m_rng);
    }
    generateDensityMap();
    generateOrientationMap();

    m_ridgeGenerator.reseed();

    m_ridgeGenerator.setParameters(m_params.ridge, m_params.density,
                                   m_params.rendering, m_params.variation);
    m_ridgeGenerator.setMinutiaeParameters(m_params.minutiae);
    m_ridgeGenerator.setOrientationMap(m_orientationMap, m_width, m_height);
    m_ridgeGenerator.setDensityMap(m_densityMap);
    m_ridgeGenerator.setShapeMap(m_shapeMap);

    const auto& cores = m_points.getCores();
    if (!cores.empty()) {
        m_ridgeGenerator.setCorePosition(cores[0].x, cores[0].y);
    } else {
        m_ridgeGenerator.setCorePosition(m_width / 2.0, m_height * 0.4);
    }

    // Gerar ridge map bruto (1=crista, 0=vale em escala [0,255])
    Image ridgeImg = m_ridgeGenerator.generate();
    const int w = ridgeImg.width();
    const int h = ridgeImg.height();

    // Converter uint8_t → float [0,1]: pixels brancos=255=vale→0.0; preto=0=crista→1.0
    // ridge_generator retorna: cristas escuras (0), vales claros (255) — mesma convenção
    // do sfinge-qt6: ridgeMap[i] = 1 - pixel/255 para que 1=crista, 0=vale
    std::vector<float> ridgeMap(w * h);
    const uint8_t* px = ridgeImg.data();
    for (int i = 0; i < w * h; ++i)
        ridgeMap[i] = 1.0f - px[i] / 255.0f;

    // Aplicar TextureRenderer — mesmo pipeline que sfinge-qt6
    const unsigned int seed = static_cast<unsigned int>(rd());
    TextureRenderer renderer(m_params.rendering, w, h, seed);
    auto rendered = renderer.render(ridgeMap, m_shapeMap, m_orientationMap);

    // Converter float [0,1] → Image uint8_t
    Image result(w, h);
    result.setDPI(500);
    uint8_t* out = result.data();
    for (int i = 0; i < w * h; ++i)
        out[i] = static_cast<uint8_t>(std::clamp(rendered[i], 0.f, 1.f) * 255.f);

    return result;
}

} // namespace SFinGe
