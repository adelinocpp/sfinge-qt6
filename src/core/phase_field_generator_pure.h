#ifndef PHASE_FIELD_GENERATOR_PURE_H
#define PHASE_FIELD_GENERATOR_PURE_H

#include <vector>
#include <random>

namespace SFinGe {

class PhaseFieldGeneratorPure {
public:
    PhaseFieldGeneratorPure();
    ~PhaseFieldGeneratorPure();
    
    /**
     * @brief Gera campo de fase contínuo via integração do campo de orientação
     * @param orientationField Campo de orientação θ(x,y) em radianos
     * @param frequencyField Frequência local (cristas/mm)
     * @param dpi Resolução da imagem
     * @return Matriz 2D com fase contínua
     */
    std::vector<std::vector<double>> generate(
        const std::vector<std::vector<double>>& orientationField,
        const std::vector<std::vector<double>>& frequencyField,
        int dpi
    );
    
    /**
     * @brief Define nível de ruído controlado
     * @param noiseLevel Nível de ruído (0.0 a 1.0)
     */
    void setNoiseLevel(double noiseLevel);
    
private:
    double m_noiseLevel = 0.1;  // 10% de variação padrão
    std::mt19937 m_rng;
    
    /**
     * @brief Integra fase ao longo de uma linha (horizontal)
     */
    void integrateLine(
        std::vector<double>& phaseLine,
        const std::vector<double>& orientationLine,
        const std::vector<double>& frequencyLine,
        double pixelSpacing
    );
    
    /**
     * @brief Suaviza transições entre linhas consecutivas
     */
    void smoothVerticalTransitions(
        std::vector<std::vector<double>>& phaseField
    );
};

}

#endif // PHASE_FIELD_GENERATOR_PURE_H
