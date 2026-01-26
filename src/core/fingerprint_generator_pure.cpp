#include "fingerprint_generator_pure.h"
#include "generators_pure.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

FingerprintGeneratorPure::FingerprintGeneratorPure() {
}

FingerprintGeneratorPure::~FingerprintGeneratorPure() {
}

void FingerprintGeneratorPure::setParameters(const FingerprintParametersPure& params) {
    m_params = params;
}

void FingerprintGeneratorPure::setSingularPoints(const SingularPointsPure& points) {
    m_points = points;
}

ImageBuffer FingerprintGeneratorPure::generateFingerprint() {
    // Calculate image dimensions
    int width = m_params.shape.left + m_params.shape.right;
    int height = m_params.shape.top + m_params.shape.middle + m_params.shape.bottom;
    
    // Create intermediate maps
    ImageBuffer shapeMap(width, height);
    ImageBuffer densityMap(width, height);
    ImageBuffer orientationMap(width, height);
    
    // Generate maps
    generateShapeMap(shapeMap);
    generateDensityMap(densityMap);
    generateOrientationMap(orientationMap);
    
    // Generate final fingerprint
    ImageBuffer ridgeMap(width, height);
    generateRidgeMap(ridgeMap, shapeMap, densityMap, orientationMap);
    
    return ridgeMap;
}

void FingerprintGeneratorPure::generateShapeMap(ImageBuffer& shapeMap) {
    ShapeGeneratorPure shapeGen;
    shapeGen.setParameters(m_params.shape);
    shapeGen.generateShape(shapeMap);
}

void FingerprintGeneratorPure::generateDensityMap(ImageBuffer& densityMap) {
    DensityGeneratorPure densityGen;
    densityGen.setParameters(m_params.density);
    densityGen.setSingularPoints(m_points);
    densityGen.generateDensity(densityMap);
}

void FingerprintGeneratorPure::generateOrientationMap(ImageBuffer& orientationMap) {
    int width = orientationMap.getWidth();
    int height = orientationMap.getHeight();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double orientation = calculateOrientation(x, y, m_points);
            
            // Convert orientation to 0-255 range for storage
            uint8_t value = static_cast<uint8_t>((orientation / M_PI) * 255);
            orientationMap.setPixel(x, y, value);
        }
    }
}

void FingerprintGeneratorPure::generateRidgeMap(ImageBuffer& ridgeMap, const ImageBuffer& shapeMap, 
                                                const ImageBuffer& densityMap, const ImageBuffer& orientationMap) {
    int width = ridgeMap.getWidth();
    int height = ridgeMap.getHeight();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Check if inside fingerprint shape
            if (shapeMap.getPixel(x, y) > 128) {
                // Get local orientation and density
                double orientation = (orientationMap.getPixel(x, y) / 255.0) * M_PI;
                double density = densityMap.getPixel(x, y) / 255.0;
                
                // Generate ridge pattern (simplified Gabor filter)
                double ridgeSpacing = 8.0 / density;
                double phase = 2.0 * M_PI * x / ridgeSpacing;
                
                // Apply orientation
                double rotatedX = x * cos(orientation) - y * sin(orientation);
                double ridgeValue = 128 + 64 * sin(phase);
                
                // Add some noise
                ridgeValue += (rand() % 21) - 10; // Random noise between -10 and +10
                
                uint8_t pixelValue = static_cast<uint8_t>(std::max(0.0, std::min(255.0, ridgeValue)));
                ridgeMap.setPixel(x, y, pixelValue);
            } else {
                ridgeMap.setPixel(x, y, 255); // White background
            }
        }
    }
}

double FingerprintGeneratorPure::calculateOrientation(double x, double y, const SingularPointsPure& points) {
    double totalAngle = 0.0;
    
    // Calculate contribution from cores
    for (int i = 0; i < points.getCoreCount(); ++i) {
        auto core = points.getCore(i);
        double dx = x - core.x;
        double dy = y - core.y;
        double r = std::sqrt(dx*dx + dy*dy);
        
        if (r > 0.1) { // Avoid division by zero
            totalAngle += core.alpha * std::atan2(dy, dx);
        }
    }
    
    // Calculate contribution from deltas
    for (int i = 0; i < points.getDeltaCount(); ++i) {
        auto delta = points.getDelta(i);
        double dx = x - delta.x;
        double dy = y - delta.y;
        double r = std::sqrt(dx*dx + dy*dy);
        
        if (r > 0.1) { // Avoid division by zero
            totalAngle += delta.alpha * std::atan2(dy, dx);
        }
    }
    
    // Normalize to [0, PI]
    double orientation = 0.5 * totalAngle + M_PI / 2.0;
    while (orientation < 0) orientation += M_PI;
    while (orientation >= M_PI) orientation -= M_PI;
    
    return orientation;
}

double FingerprintGeneratorPure::calculateDensity(double x, double y, const SingularPointsPure& points) {
    // Simplified density calculation
    double baseDensity = 0.5; // 50% density
    
    // Modify density based on distance to singular points
    for (int i = 0; i < points.getCoreCount(); ++i) {
        auto core = points.getCore(i);
        double dx = x - core.x;
        double dy = y - core.y;
        double r = std::sqrt(dx*dx + dy*dy);
        
        if (r < 100) { // Influence radius
            baseDensity += 0.2 * (1.0 - r / 100.0);
        }
    }
    
    return std::max(0.1, std::min(1.0, baseDensity));
}

bool FingerprintGeneratorPure::isInsideShape(double x, double y, const ShapeParameters& shape) {
    // Simplified elliptical shape
    double centerX = shape.right;
    double centerY = shape.top + shape.middle / 2.0;
    
    double dx = (x - centerX) / shape.right;
    double dy = (y - centerY) / (shape.middle / 2.0);
    
    return (dx*dx + dy*dy) <= 1.0;
}

}
