#ifndef DENSITY_GENERATOR_H
#define DENSITY_GENERATOR_H

#include <QImage>
#include <vector>
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class DensityGenerator {
public:
    DensityGenerator();
    
    void setParameters(const DensityParameters& params);
    void setShapeMap(const std::vector<float>& shapeMap, int width, int height);
    
    QImage generate();
    
    std::vector<float> getDensityMap() const { return m_densityMap; }
    
private:
    void generateDensityMap();
    std::vector<float> resizeNoise(const std::vector<float>& input, 
                                   int inWidth, int inHeight,
                                   int outWidth, int outHeight);
    
    DensityParameters m_params;
    std::vector<float> m_shapeMap;
    std::vector<float> m_densityMap;
    int m_width;
    int m_height;
};

}

#endif
