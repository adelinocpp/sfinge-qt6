#ifndef RIDGE_GENERATOR_H
#define RIDGE_GENERATOR_H

#include <QImage>
#include <vector>
#include <random>
#include "models/fingerprint_parameters.h"
#include "gabor_filter.h"
#include "minutiae_generator.h"
#include "phase_field_generator.h"
#include "quality_mask_generator.h"
#include "frequency_field_smoother.h"

namespace SFinGe {

class RidgeGenerator {
public:
    RidgeGenerator();
    
    void setParameters(const RidgeParameters& params, const DensityParameters& densityParams,
                        const RenderingParameters& renderParams, const VariationParameters& varParams);
    void setMinutiaeParameters(const MinutiaeParameters& params);
    void setOrientationMap(const std::vector<double>& orientationMap, int width, int height);
    void setDensityMap(const std::vector<float>& densityMap);
    void setShapeMap(const std::vector<float>& shapeMap);
    void setCorePosition(double coreX, double coreY);
    
    QImage generate();
    
    std::vector<float> getRidgeMap() const { return m_ridgeMap; }
    
    // Estatísticas de minúcias
    int getMinutiaeCount() const { return m_minutiaeGenerator.getMinutiaeCount(); }
    int getBifurcationCount() const { return m_minutiaeGenerator.getBifurcationCount(); }
    int getEndingCount() const { return m_minutiaeGenerator.getEndingCount(); }
    const std::vector<Minutia>& getMinutiae() const { return m_minutiaeGenerator.getMinutiae(); }
    
private:
    void generateRidgeMap();
    void generateRidgeMapOriginal();
    void generateRidgeMapImproved();
    double applyFilter(const GaborFilter& filter, int x, int y, const std::vector<float>& image);
    std::vector<float> renderFingerprint(const std::vector<float>& binaryRidge);
    
    // Funções de realismo
    void applyGaussianNoise(std::vector<float>& image, double amplitude);
    void applyLocalContrastVariation(std::vector<float>& image);
    void applyElasticDistortion(std::vector<float>& image);
    void applySkinCondition(std::vector<float>& image);
    double perlinNoise(double x, double y);
    double fade(double t);
    double lerp(double a, double b, double t);
    double grad(int hash, double x, double y);
    
    RidgeParameters m_params;
    DensityParameters m_densityParams;
    RenderingParameters m_renderParams;
    VariationParameters m_varParams;
    MinutiaeParameters m_minutiaeParams;
    std::vector<double> m_orientationMap;
    std::vector<float> m_densityMap;
    std::vector<float> m_shapeMap;
    std::vector<float> m_ridgeMap;
    int m_width;
    int m_height;
    double m_coreX;
    double m_coreY;
    
    // Perlin noise permutation table
    std::vector<int> m_perm;
    std::mt19937 m_rng;
    MinutiaeGenerator m_minutiaeGenerator;
    
    // Novos geradores para controle melhorado
    PhaseFieldGenerator m_phaseGenerator;
    QualityMaskGenerator m_qualityGenerator;
    FrequencyFieldSmoother m_frequencySmoother;
};

}

#endif
