#ifndef ORIENTATION_GENERATOR_H
#define ORIENTATION_GENERATOR_H

#include <vector>
#include <random>
#include "image.h"
#include "models/singular_points.h"
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class OrientationGenerator {
public:
    OrientationGenerator();
    
    void reseed();
    
    void setSingularPoints(const SingularPoints& points);
    void setShapeMap(const std::vector<float>& shapeMap, int width, int height);
    void setParameters(const OrientationParameters& params);
    void setFingerprintClass(FingerprintClass fpClass);
    
    Image generate();
    
    std::vector<double> getOrientationMap() const { return m_orientationMap; }
    
private:
    void generateOrientationMap();
    void generatePoincareMap();
    
    void generateArchOrientation();
    void generateTentedArchOrientation();
    void generateLoopOrientation();
    void generateWhorlOrientation();
    void generateTwinLoopOrientation();
    void generateCentralPocketOrientation();
    void generateAccidentalOrientation();
    void generateDefaultPoincare();
    
    void generateVariedAlphas();
    void smoothOrientationMap(double sigma);
    
    SingularPoints m_points;
    std::vector<float> m_shapeMap;
    std::vector<double> m_orientationMap;
    OrientationParameters m_params;
    FingerprintClass m_fpClass;
    int m_width;
    int m_height;
    
    std::vector<double> m_coreAlphas;
    std::vector<double> m_deltaAlphas;
    std::mt19937 m_rng;
};

} // namespace SFinGe

#endif // ORIENTATION_GENERATOR_H
