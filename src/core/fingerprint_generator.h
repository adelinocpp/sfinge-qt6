#ifndef FINGERPRINT_GENERATOR_H
#define FINGERPRINT_GENERATOR_H

#include <QObject>
#include <QImage>
#include "models/fingerprint_parameters.h"
#include "models/singular_points.h"
#include "shape_generator.h"
#include "density_generator.h"
#include "orientation_generator.h"
#include "ridge_generator.h"

namespace SFinGe {

class FingerprintGenerator : public QObject {
    Q_OBJECT
    
public:
    explicit FingerprintGenerator(QObject* parent = nullptr);
    
    void setParameters(const FingerprintParameters& params);
    void setSingularPoints(const SingularPoints& points);
    
    QImage generateShape();
    QImage generateDensity();
    QImage generateOrientation();
    QImage generateOrientationVisualization();
    QImage generateFingerprint();
    
    QImage getShapeImage() const { return m_shapeImage; }
    QImage getDensityImage() const { return m_densityImage; }
    QImage getOrientationImage() const { return m_orientationImage; }
    QImage getFingerprintImage() const { return m_fingerprintImage; }
    
signals:
    void progressChanged(int percentage, const QString& message);
    void generationComplete();
    void generationError(const QString& error);
    
private:
    FingerprintParameters m_params;
    SingularPoints m_points;
    
    ShapeGenerator m_shapeGenerator;
    DensityGenerator m_densityGenerator;
    OrientationGenerator m_orientationGenerator;
    RidgeGenerator m_ridgeGenerator;
    
    QImage m_shapeImage;
    QImage m_densityImage;
    QImage m_orientationImage;
    QImage m_fingerprintImage;
};

}

#endif
