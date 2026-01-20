#ifndef RIDGE_GENERATOR_H
#define RIDGE_GENERATOR_H

#include <QImage>
#include <vector>
#include "models/fingerprint_parameters.h"
#include "gabor_filter.h"

namespace SFinGe {

class RidgeGenerator {
public:
    RidgeGenerator();
    
    void setParameters(const RidgeParameters& params, const DensityParameters& densityParams);
    void setOrientationMap(const std::vector<double>& orientationMap, int width, int height);
    void setDensityMap(const std::vector<float>& densityMap);
    void setShapeMap(const std::vector<float>& shapeMap);
    
    QImage generate();
    
    std::vector<float> getRidgeMap() const { return m_ridgeMap; }
    
private:
    void generateRidgeMap();
    double applyFilter(const GaborFilter& filter, int x, int y, const std::vector<float>& image);
    std::vector<float> renderFingerprint(const std::vector<float>& binaryRidge);
    
    RidgeParameters m_params;
    DensityParameters m_densityParams;
    std::vector<double> m_orientationMap;
    std::vector<float> m_densityMap;
    std::vector<float> m_shapeMap;
    std::vector<float> m_ridgeMap;
    int m_width;
    int m_height;
};

}

#endif
