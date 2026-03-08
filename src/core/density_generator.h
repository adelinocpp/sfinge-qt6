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
    void setNoiseOffset(double x0, double y0) { m_noiseOffsetX = x0; m_noiseOffsetY = y0; }
    
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
    double m_noiseOffsetX = 0.0;
    double m_noiseOffsetY = 0.0;
};

}

#endif
