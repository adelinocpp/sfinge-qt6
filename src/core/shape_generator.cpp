#include "shape_generator.h"
#include "math_utils.h"
#include <QPainter>
#include <cmath>
#include <algorithm>

namespace SFinGe {

ShapeGenerator::ShapeGenerator()
    : m_width(0), m_height(0) {
}

void ShapeGenerator::setParameters(const ShapeParameters& params) {
    m_params = params;
    m_width = params.left + params.right;
    m_height = params.top + params.middle + params.bottom;
}

void ShapeGenerator::generateShapeMap() {
    m_shapeMap.resize(m_width * m_height, 0.0f);

    // Modelo elíptico Cappelli 2004:
    // - Seção superior: duas meias-elipses unidas no centro (cx, cy_top)
    //   formando a ponta arredondada do dedo
    // - Seção média: retângulo completo (largura total)
    // - Seção inferior: duas meias-elipses unidas em (cx, cy_bot)
    //   formando a base arredondada
    // Feathering: degradê suave de 8% do raio nas bordas

    const float cx     = static_cast<float>(m_params.left);
    const float cy_top = static_cast<float>(m_params.top);
    const float cy_bot = static_cast<float>(m_params.top + m_params.middle);

    const float ax_left  = static_cast<float>(m_params.left);
    const float ax_right = static_cast<float>(m_params.right);
    const float ay_top   = static_cast<float>(m_params.top);
    const float ay_bot   = static_cast<float>(m_params.bottom);

    // Zona de degradê = 8% do raio da elipse
    const float feather = 0.08f;

    for (int y = 0; y < m_height; ++y) {
        const float fy = static_cast<float>(y);
        for (int x = 0; x < m_width; ++x) {
            const float fx = static_cast<float>(x);
            float alpha = 0.0f;

            if (fy < cy_top) {
                // Seção superior: meias-elipses centradas em (cx, cy_top)
                const float dx = fx - cx;
                const float dy = fy - cy_top;  // negativo (acima)
                const float ax = (dx <= 0.0f) ? ax_left : ax_right;
                const float v  = std::sqrt((dx / ax) * (dx / ax) +
                                           (dy / ay_top) * (dy / ay_top));
                if (v < 1.0f - feather) {
                    alpha = 1.0f;
                } else if (v < 1.0f) {
                    alpha = (1.0f - v) / feather;
                }
            } else if (fy > cy_bot) {
                // Seção inferior: meias-elipses centradas em (cx, cy_bot)
                const float dx = fx - cx;
                const float dy = fy - cy_bot;  // positivo (abaixo)
                const float ax = (dx <= 0.0f) ? ax_left : ax_right;
                const float v  = std::sqrt((dx / ax) * (dx / ax) +
                                           (dy / ay_bot) * (dy / ay_bot));
                if (v < 1.0f - feather) {
                    alpha = 1.0f;
                } else if (v < 1.0f) {
                    alpha = (1.0f - v) / feather;
                }
            } else {
                // Seção média: retângulo completo com degradê sutil nas bordas
                const float feather_px = feather * std::min(ax_left, ax_right);
                const float distEdge   = std::min(fx, static_cast<float>(m_width - 1) - fx);
                alpha = std::min(1.0f, distEdge / feather_px);
            }

            m_shapeMap[y * m_width + x] = alpha;
        }
    }
}

QImage ShapeGenerator::generate() {
    generateShapeMap();

    QImage image(m_width, m_height, QImage::Format_Grayscale8);

    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            // Display binário sem flip: ponta no topo (consistente com fingerprint final)
            float alpha = m_shapeMap[j * m_width + i];
            int gray = (alpha >= 0.5f) ? 0 : 255;
            image.setPixel(i, j, qRgb(gray, gray, gray));
        }
    }

    return image;
}

}
