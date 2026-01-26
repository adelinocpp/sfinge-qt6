#ifndef FREQUENCYFIELDSMOOTHER_H
#define FREQUENCYFIELDSMOOTHER_H

#include <vector>

namespace SFinGe {

class FrequencyFieldSmoother {
public:
    FrequencyFieldSmoother();
    ~FrequencyFieldSmoother();
    
    /**
     * @brief Suaviza campo de frequência com filtro Gaussiano
     * @param frequencyField Campo original (cristas/mm)
     * @param sigma Desvio padrão da Gaussiana (em pixels)
     * @return Campo suavizado
     */
    std::vector<std::vector<double>> smooth(
        const std::vector<std::vector<double>>& frequencyField,
        double sigma = 10.0
    );
    
    /**
     * @brief Define faixas válidas de frequência
     */
    void setFrequencyRange(double minFreq, double maxFreq);
    
private:
    double m_minFrequency = 7.0;   // cristas/mm
    double m_maxFrequency = 15.0;  // cristas/mm
    
    /**
     * @brief Cria kernel Gaussiano
     */
    std::vector<std::vector<double>> createGaussianKernel(
        double sigma
    );
    
    /**
     * @brief Aplica convolução
     */
    std::vector<std::vector<double>> convolve(
        const std::vector<std::vector<double>>& input,
        const std::vector<std::vector<double>>& kernel
    );
};

}

#endif // FREQUENCYFIELDSMOOTHER_H
