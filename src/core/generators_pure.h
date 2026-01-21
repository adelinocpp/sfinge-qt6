#ifndef GENERATORS_PURE_H
#define GENERATORS_PURE_H

#include "image_buffer.h"
#include "models/fingerprint_parameters_pure.h"
#include "models/singular_points_pure.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class ShapeGeneratorPure {
public:
    void setParameters(const ShapeParameters& params);
    void generateShape(ImageBuffer& shapeMap);
    
private:
    ShapeParameters m_params;
    bool isInsideEllipse(double x, double y, double centerX, double centerY, double radiusX, double radiusY);
};

class DensityGeneratorPure {
public:
    void setParameters(const DensityParameters& params);
    void setSingularPoints(const SingularPointsPure& points);
    void generateDensity(ImageBuffer& densityMap);
    
private:
    DensityParameters m_params;
    SingularPointsPure m_points;
};

class OrientationGeneratorPure {
public:
    void setParameters(const OrientationParameters& params);
    void setSingularPoints(const SingularPointsPure& points);
    void generateOrientation(ImageBuffer& orientationMap);
    
private:
    OrientationParameters m_params;
    SingularPointsPure m_points;
    double calculateOrientation(double x, double y);
};

class RidgeGeneratorPure {
public:
    void generateRidges(ImageBuffer& ridgeMap, 
                       const ImageBuffer& shapeMap,
                       const ImageBuffer& densityMap, 
                       const ImageBuffer& orientationMap,
                       const RenderingParameters& rendering);
    
private:
    double applyGaborFilter(double x, double y, double orientation, double frequency);
    void addNoise(ImageBuffer& image, double amplitude);
    void applyContrast(ImageBuffer& image, double factor);
};

#endif
