#ifndef FINGERPRINT_GENERATOR_PURE_H
#define FINGERPRINT_GENERATOR_PURE_H

#include "image_buffer.h"
#include "models/fingerprint_parameters_pure.h"
#include "models/singular_points_pure.h"

namespace SFinGe {

class FingerprintGeneratorPure {
public:
    FingerprintGeneratorPure();
    ~FingerprintGeneratorPure();
    
    void setParameters(const FingerprintParametersPure& params);
    void setSingularPoints(const SingularPointsPure& points);
    
    ImageBuffer generateFingerprint();
    
private:
    FingerprintParametersPure m_params;
    SingularPointsPure m_points;
    
    // Core generation methods
    void generateShapeMap(ImageBuffer& shapeMap);
    void generateDensityMap(ImageBuffer& densityMap);
    void generateOrientationMap(ImageBuffer& orientationMap);
    void generateRidgeMap(ImageBuffer& ridgeMap, const ImageBuffer& shapeMap, 
                         const ImageBuffer& densityMap, const ImageBuffer& orientationMap);
    
    // Helper methods
    double calculateOrientation(double x, double y, const SingularPointsPure& points);
    double calculateDensity(double x, double y, const SingularPointsPure& points);
    bool isInsideShape(double x, double y, const ShapeParameters& shape);
};

}

#endif
