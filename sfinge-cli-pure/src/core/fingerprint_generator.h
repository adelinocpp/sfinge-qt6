#ifndef FINGERPRINT_GENERATOR_H
#define FINGERPRINT_GENERATOR_H

#include "image.h"
#include "orientation_generator.h"
#include "ridge_generator.h"
#include "models/singular_points.h"
#include "models/fingerprint_parameters.h"
#include <random>

namespace SFinGe {

class FingerprintGenerator {
public:
    FingerprintGenerator();
    
    void setParameters(const FingerprintParameters& params);
    void setSingularPoints(const SingularPoints& points);
    
    Image generateFingerprint();
    
private:
    void generateShapeMap();
    void generateDensityMap();
    void generateOrientationMap();
    
    FingerprintParameters m_params;
    SingularPoints m_points;
    
    int m_width;
    int m_height;
    
    std::vector<float> m_shapeMap;
    std::vector<float> m_densityMap;
    std::vector<double> m_orientationMap;
    
    OrientationGenerator m_orientationGenerator;
    RidgeGenerator m_ridgeGenerator;
    std::mt19937 m_rng;
};

} // namespace SFinGe

#endif // FINGERPRINT_GENERATOR_H
