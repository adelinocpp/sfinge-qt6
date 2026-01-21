#ifndef RIDGE_GENERATOR_H
#define RIDGE_GENERATOR_H

#include <vector>
#include <random>
#include "image.h"
#include "models/fingerprint_parameters.h"
#include "gabor_filter.h"
#include "minutiae_generator.h"

namespace SFinGe {

class RidgeGenerator {
public:
    RidgeGenerator();
    
    // Reinicializa o RNG com nova seed - DEVE ser chamado antes de cada geração
    void reseed();
    
    void setParameters(const RidgeParameters& params, const DensityParameters& densityParams,
                        const RenderingParameters& renderParams, const VariationParameters& varParams);
    void setMinutiaeParameters(const MinutiaeParameters& params);
    void setOrientationMap(const std::vector<double>& orientationMap, int width, int height);
    void setDensityMap(const std::vector<float>& densityMap);
    void setShapeMap(const std::vector<float>& shapeMap);
    void setCorePosition(double coreX, double coreY);
    
    Image generate();
    
    std::vector<float> getRidgeMap() const { return m_ridgeMap; }
    
    // Estatísticas de minúcias
    int getMinutiaeCount() const { return m_minutiaeGenerator.getMinutiaeCount(); }
    int getBifurcationCount() const { return m_minutiaeGenerator.getBifurcationCount(); }
    int getEndingCount() const { return m_minutiaeGenerator.getEndingCount(); }
    const std::vector<Minutia>& getMinutiae() const { return m_minutiaeGenerator.getMinutiae(); }
    
private:
    void generateRidgeMap();
    double applyFilter(const GaborFilter& filter, int x, int y, const std::vector<float>& image);
    std::vector<float> renderFingerprint(const std::vector<float>& binaryRidge);
    
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
    
    std::vector<int> m_perm;
    std::mt19937 m_rng;
    MinutiaeGenerator m_minutiaeGenerator;
};

}

#endif
