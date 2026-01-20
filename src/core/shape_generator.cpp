#include "shape_generator.h"
#include "math_utils.h"
#include <QPainter>

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
    m_shapeMap.resize(m_width * m_height);
    
    // Shape retangular completo - sem buracos
    // Todos os pixels dentro do retângulo são 1.0 (área da digital)
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            m_shapeMap[j * m_width + i] = 1.0f;
        }
    }
}

QImage ShapeGenerator::generate() {
    generateShapeMap();
    
    QImage image(m_width, m_height, QImage::Format_Grayscale8);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int gray = m_shapeMap[(m_height - j - 1) * m_width + i] > 0 ? 0 : 255;
            image.setPixel(i, j, qRgb(gray, gray, gray));
        }
    }
    
    return image;
}

}
