#include "fingerprint_generator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

FingerprintGenerator::FingerprintGenerator() 
    : m_width(0), m_height(0), m_rng(std::random_device{}()) {
}

void FingerprintGenerator::setParameters(const FingerprintParameters& params) {
    m_params = params;
    m_width = params.shape.left + params.shape.right;
    m_height = params.shape.top + params.shape.middle + params.shape.bottom;
}

void FingerprintGenerator::setSingularPoints(const SingularPoints& points) {
    m_points = points;
}

void FingerprintGenerator::generateShapeMap() {
    m_shapeMap.resize(m_width * m_height, 1.0f);
    
    double cx = m_width / 2.0;
    double cy = m_height / 2.0;
    double rx = m_width / 2.0 * 0.95;
    double ry = m_height / 2.0 * 0.95;
    
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            double dx = (x - cx) / rx;
            double dy = (y - cy) / ry;
            double dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist > 1.0) {
                m_shapeMap[y * m_width + x] = 0.0f;
            } else if (dist > 0.85) {
                m_shapeMap[y * m_width + x] = static_cast<float>((1.0 - dist) / 0.15);
            }
        }
    }
}

void FingerprintGenerator::generateDensityMap() {
    m_densityMap.resize(m_width * m_height);
    
    std::uniform_real_distribution<float> dist(m_params.density.minFrequency, 
                                                m_params.density.maxFrequency);
    
    // Generate base density with some spatial coherence
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            // Base frequency with slight variation
            float baseFreq = (m_params.density.minFrequency + m_params.density.maxFrequency) / 2.0f;
            float variation = (dist(m_rng) - baseFreq) * 0.3f;
            m_densityMap[y * m_width + x] = baseFreq + variation;
        }
    }
}

void FingerprintGenerator::generateOrientationMap() {
    m_orientationGenerator.setShapeMap(m_shapeMap, m_width, m_height);
    m_orientationGenerator.setSingularPoints(m_points);
    m_orientationGenerator.setParameters(m_params.orientation);
    m_orientationGenerator.setFingerprintClass(m_params.classification.fingerprintClass);
    
    m_orientationGenerator.generate();
    m_orientationMap = m_orientationGenerator.getOrientationMap();
}

Image FingerprintGenerator::generateFingerprint() {
    // Reinicializar RNG para garantir isolamento entre threads
    std::random_device rd;
    m_rng.seed(rd());
    
    // Reinicializar RNG do orientation generator
    m_orientationGenerator.reseed();
    
    generateShapeMap();
    generateDensityMap();
    generateOrientationMap();
    
    // Reinicializar RNG do ridge generator para evitar contaminação entre threads
    m_ridgeGenerator.reseed();
    
    // Configure ridge generator
    m_ridgeGenerator.setParameters(m_params.ridge, m_params.density, 
                                   m_params.rendering, m_params.variation);
    m_ridgeGenerator.setMinutiaeParameters(m_params.minutiae);
    m_ridgeGenerator.setOrientationMap(m_orientationMap, m_width, m_height);
    m_ridgeGenerator.setDensityMap(m_densityMap);
    m_ridgeGenerator.setShapeMap(m_shapeMap);
    
    // Set core position from singular points
    const auto& cores = m_points.getCores();
    if (!cores.empty()) {
        m_ridgeGenerator.setCorePosition(cores[0].x, cores[0].y);
    } else {
        m_ridgeGenerator.setCorePosition(m_width / 2.0, m_height * 0.4);
    }
    
    // Generate fingerprint image
    Image result = m_ridgeGenerator.generate();
    result.setDPI(500);
    
    return result;
}

} // namespace SFinGe
