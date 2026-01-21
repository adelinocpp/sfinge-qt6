#ifndef ORIENTATION_GENERATOR_H
#define ORIENTATION_GENERATOR_H

#include <QImage>
#include <vector>
#include "models/singular_points.h"
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class OrientationGenerator {
public:
    OrientationGenerator();
    
    void setSingularPoints(const SingularPoints& points);
    void setShapeMap(const std::vector<float>& shapeMap, int width, int height);
    void setParameters(const OrientationParameters& params);
    void setFingerprintClass(FingerprintClass fpClass);
    
    QImage generate();
    QImage generateVisualization();
    
    std::vector<double> getOrientationMap() const { return m_orientationMap; }
    
private:
    void generateOrientationMap();
    void generatePoincareMap();
    void generateFOMFEMap();
    void applyLegendreSmoothing();
    
    // Módulo 1: Métodos específicos por tipo de impressão digital
    void generateArchOrientation();
    void generateTentedArchOrientation();
    void generateLoopOrientation();
    void generateWhorlOrientation();
    void generateTwinLoopOrientation();
    void generateCentralPocketOrientation();
    void generateAccidentalOrientation();
    void generateDefaultPoincare();
    
    // Gerador com índices de Poincaré fracionários
    void generateFractionalOrientation();
    double computeSingularityContribution(double x, double y, const SingularPoint& sing, double alpha);
    
    // Suavização do campo de orientação
    void smoothOrientationMap(double sigma);
    
    // Gerar alphas variados para Sherlock-Monro
    void generateVariedAlphas();
    
    SingularPoints m_points;
    std::vector<float> m_shapeMap;
    std::vector<double> m_orientationMap;
    OrientationParameters m_params;
    FingerprintClass m_fpClass;
    int m_width;
    int m_height;
    
    // Alphas variados para cada singularidade (Poincaré)
    std::vector<double> m_coreAlphas;   // média +1, dp 0.025
    std::vector<double> m_deltaAlphas;  // média -1, dp 0.025
};

}

#endif
