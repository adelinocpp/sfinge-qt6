#ifndef VARIATION_EFFECTS_H
#define VARIATION_EFFECTS_H

#include <QImage>
#include <vector>
#include <random>
#include "models/fingerprint_parameters.h"

namespace SFinGe {

/**
 * @brief Classe para aplicar variações e distorções realistas a impressões digitais
 * 
 * Esta classe implementa um módulo de pós-processamento para simular a variabilidade
 * intra-classe (diferentes capturas do mesmo dedo), incluindo:
 * - Distorção plástica (deformação da pele)
 * - Distorção de lente (modelo Brown-Conrady)
 * - Rotação e translação
 * - Condição da pele (úmida/seca)
 */
class VariationEffects {
public:
    /**
     * @brief Construtor
     * @param params Parâmetros de variação
     * @param seed Semente para reprodutibilidade
     */
    VariationEffects(const VariationParameters& params, unsigned int seed);

    /**
     * @brief Aplica todas as variações configuradas a uma imagem mestre
     * @param masterImage Imagem de impressão digital mestre
     * @return Imagem com variações aplicadas
     */
    QImage apply(const QImage& masterImage) const;

private:
    /**
     * @brief Aplica distorção plástica (deformação gaussiana)
     * @param image Dados da imagem como vetor
     * @param width Largura da imagem
     * @param height Altura da imagem
     * @return Imagem distorcida
     */
    std::vector<float> applyPlasticDistortion(const std::vector<float>& image, 
                                               int width, int height) const;

    /**
     * @brief Aplica distorção de lente (modelo Brown-Conrady)
     * @param image Dados da imagem
     * @param width Largura
     * @param height Altura
     * @return Imagem com distorção de lente
     */
    std::vector<float> applyLensDistortion(const std::vector<float>& image,
                                            int width, int height) const;

    /**
     * @brief Aplica rotação à imagem
     * @param image Dados da imagem
     * @param width Largura
     * @param height Altura
     * @return Imagem rotacionada
     */
    std::vector<float> applyRotation(const std::vector<float>& image,
                                      int width, int height) const;

    /**
     * @brief Aplica translação à imagem
     * @param image Dados da imagem
     * @param width Largura
     * @param height Altura
     * @return Imagem transladada
     */
    std::vector<float> applyTranslation(const std::vector<float>& image,
                                         int width, int height) const;

    /**
     * @brief Aplica efeito de condição da pele (erosão/dilatação)
     * @param image Dados da imagem
     * @param width Largura
     * @param height Altura
     * @return Imagem com efeito de pele aplicado
     */
    std::vector<float> applySkinCondition(const std::vector<float>& image,
                                           int width, int height) const;

    /**
     * @brief Converte QImage para vetor de floats
     */
    static std::vector<float> qimageToVector(const QImage& image);

    /**
     * @brief Converte vetor de floats para QImage
     */
    static QImage vectorToQImage(const std::vector<float>& data, int width, int height);

    /**
     * @brief Interpolação bilinear para amostragem de pixels
     */
    static float bilinearSample(const std::vector<float>& image, int width, int height,
                                float x, float y, float defaultValue = 1.0f);

    VariationParameters m_params;
    mutable std::mt19937 m_rng;
};

} // namespace SFinGe

#endif // VARIATION_EFFECTS_H
