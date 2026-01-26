#ifndef QUALITYMASKGENERATOR_H
#define QUALITYMASKGENERATOR_H

#include <vector>

namespace SFinGe {

class QualityMaskGenerator {
public:
    QualityMaskGenerator();
    ~QualityMaskGenerator();
    
    /**
     * @brief Calcula máscara de qualidade baseada em coerência do campo
     * @param orientationField Campo de orientação
     * @return Máscara (0.0 = baixa qualidade, 1.0 = alta qualidade)
     */
    std::vector<std::vector<double>> generate(
        const std::vector<std::vector<double>>& orientationField
    );
    
    void setCoherenceThreshold(double threshold);
    void setWindowSize(int size);
    
private:
    double m_coherenceThreshold = 0.5;
    int m_windowSize = 16;  // pixels
    
    /**
     * @brief Calcula coerência local usando representação complexa
     */
    double computeLocalCoherence(
        const std::vector<std::vector<double>>& orientationField,
        int centerX, int centerY
    );
};

}

#endif // QUALITYMASKGENERATOR_H
