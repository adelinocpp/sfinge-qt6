#ifndef SHAPE_GENERATOR_H
#define SHAPE_GENERATOR_H

#include <QImage>
#include <vector>
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class ShapeGenerator {
public:
    ShapeGenerator();
    
    void setParameters(const ShapeParameters& params);
    
    QImage generate();
    
    std::vector<float> getShapeMap() const { return m_shapeMap; }
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
private:
    void generateShapeMap();
    
    ShapeParameters m_params;
    std::vector<float> m_shapeMap;
    int m_width;
    int m_height;
};

}

#endif
