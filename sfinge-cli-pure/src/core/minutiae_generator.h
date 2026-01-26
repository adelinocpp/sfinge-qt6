#ifndef MINUTIAE_GENERATOR_H
#define MINUTIAE_GENERATOR_H

#include <vector>
#include <random>
#include "models/fingerprint_parameters.h"

namespace SFinGe {

enum class MinutiaeType {
    RidgeEnding = 0,    // Terminação de crista
    Bifurcation = 1     // Bifurcação
};

struct Minutia {
    double x;           // Posição X
    double y;           // Posição Y
    double angle;       // Ângulo da minúcia (direção da crista)
    MinutiaeType type;  // Tipo: terminação ou bifurcação
    double quality;     // Qualidade (0-1)
};

// MinutiaeStatistics e MinutiaeParameters são definidos em fingerprint_parameters.h

class MinutiaeGenerator {
public:
    MinutiaeGenerator();
    
    void reseed();
    void setParameters(const MinutiaeParameters& params);
    void setOrientationMap(const std::vector<double>& orientationMap, int width, int height);
    void setShapeMap(const std::vector<float>& shapeMap);
    void setRidgeMap(const std::vector<float>& ridgeMap);
    void setCorePosition(double coreX, double coreY);
    
    // Gerar minúcias e aplicar ao mapa de cristas
    std::vector<Minutia> generateMinutiae();
    void applyMinutiae(std::vector<float>& ridgeMap);
    
    // Estatísticas
    int getMinutiaeCount() const { return static_cast<int>(m_minutiae.size()); }
    int getBifurcationCount() const;
    int getEndingCount() const;
    const std::vector<Minutia>& getMinutiae() const { return m_minutiae; }
    
private:
    // Métodos auxiliares
    int calculateTargetCount();
    bool isValidPosition(double x, double y) const;
    bool hasMinimumSpacing(double x, double y) const;
    double getLocalOrientation(double x, double y) const;
    bool isOnRidge(double x, double y) const;
    
    // Inserção de minúcias
    void insertRidgeEnding(std::vector<float>& ridgeMap, const Minutia& m);
    void insertBifurcation(std::vector<float>& ridgeMap, const Minutia& m);
    
    // Métodos melhorados (campo de fase contínuo)
    void insertRidgeEndingImproved(std::vector<float>& ridgeMap, const Minutia& m);
    void insertBifurcationImproved(std::vector<float>& ridgeMap, const Minutia& m);
    
    // Geração de posições
    std::pair<double, double> generatePosition();
    
    MinutiaeParameters m_params;
    std::vector<double> m_orientationMap;
    std::vector<float> m_shapeMap;
    std::vector<float> m_ridgeMap;
    std::vector<Minutia> m_minutiae;
    
    int m_width;
    int m_height;
    double m_coreX;
    double m_coreY;
    
    std::mt19937 m_rng;
};

} // namespace SFinGe

#endif // MINUTIAE_GENERATOR_H
