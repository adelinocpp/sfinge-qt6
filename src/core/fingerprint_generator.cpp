#include "fingerprint_generator.h"

namespace SFinGe {

FingerprintGenerator::FingerprintGenerator(QObject* parent)
    : QObject(parent) {
}

void FingerprintGenerator::setParameters(const FingerprintParameters& params) {
    m_params = params;
}

void FingerprintGenerator::setSingularPoints(const SingularPoints& points) {
    m_points = points;
}

QImage FingerprintGenerator::generateShape() {
    emit progressChanged(10, "Generating shape...");
    
    // SEMPRE regenerar para refletir mudanças de parâmetros
    m_shapeGenerator.setParameters(m_params.shape);
    m_shapeImage = m_shapeGenerator.generate();
    
    if (!m_shapeImage.isNull()) {
        m_shapeImage.setDotsPerMeterX(500 * 39.3701);
        m_shapeImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    return m_shapeImage;
}

QImage FingerprintGenerator::generateDensity() {
    // SEMPRE regenerar shape primeiro (F6 deve refletir mudanças)
    generateShape();
    
    emit progressChanged(30, "Generating density map...");
    
    // SEMPRE regenerar density com novos parâmetros
    m_densityGenerator.setParameters(m_params.density);
    m_densityGenerator.setShapeMap(m_shapeGenerator.getShapeMap(), 
                                   m_shapeGenerator.getWidth(), 
                                   m_shapeGenerator.getHeight());
    m_densityImage = m_densityGenerator.generate();
    
    if (!m_densityImage.isNull()) {
        m_densityImage.setDotsPerMeterX(500 * 39.3701);
        m_densityImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    return m_densityImage;
}

QImage FingerprintGenerator::generateOrientation() {
    // SEMPRE regenerar shape primeiro (F7 deve refletir mudanças)
    generateShape();
    
    emit progressChanged(50, "Generating orientation field...");
    
    m_orientationGenerator.setParameters(m_params.orientation);
    m_orientationGenerator.setFingerprintClass(m_params.classification.fingerprintClass);
    m_orientationGenerator.setSingularPoints(m_points);
    m_orientationGenerator.setShapeMap(m_shapeGenerator.getShapeMap(),
                                      m_shapeGenerator.getWidth(),
                                      m_shapeGenerator.getHeight());
    m_orientationImage = m_orientationGenerator.generate();
    
    if (!m_orientationImage.isNull()) {
        m_orientationImage.setDotsPerMeterX(500 * 39.3701);
        m_orientationImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    return m_orientationImage;
}

QImage FingerprintGenerator::generateOrientationVisualization() {
    if (m_shapeImage.isNull()) {
        generateShape();
    }
    
    m_orientationGenerator.setParameters(m_params.orientation);
    m_orientationGenerator.setFingerprintClass(m_params.classification.fingerprintClass);
    m_orientationGenerator.setShapeMap(m_shapeGenerator.getShapeMap(),
                                      m_shapeGenerator.getWidth(),
                                      m_shapeGenerator.getHeight());
    m_orientationGenerator.setSingularPoints(m_points);
    
    return m_orientationGenerator.generateVisualization();
}

QImage FingerprintGenerator::generateFingerprint() {
    emit progressChanged(0, "Starting fingerprint generation...");
    
    // SEMPRE regenerar tudo para garantir novos pontos singulares
    generateShape();
    generateDensity();
    generateOrientation();
    
    emit progressChanged(70, "Generating ridge pattern...");
    
    m_ridgeGenerator.setParameters(m_params.ridge, m_params.density);
    m_ridgeGenerator.setOrientationMap(m_orientationGenerator.getOrientationMap(),
                                      m_shapeGenerator.getWidth(),
                                      m_shapeGenerator.getHeight());
    m_ridgeGenerator.setDensityMap(m_densityGenerator.getDensityMap());
    m_ridgeGenerator.setShapeMap(m_shapeGenerator.getShapeMap());
    
    m_fingerprintImage = m_ridgeGenerator.generate();
    
    if (!m_fingerprintImage.isNull()) {
        m_fingerprintImage.setDotsPerMeterX(500 * 39.3701);
        m_fingerprintImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    emit progressChanged(100, "Fingerprint generation complete!");
    emit generationComplete();
    
    return m_fingerprintImage;
}

}
