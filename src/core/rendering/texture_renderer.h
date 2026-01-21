#ifndef TEXTURE_RENDERER_H
#define TEXTURE_RENDERER_H

#include <vector>
#include <memory>
#include <random>
#include "perlin_noise.h"
#include "models/fingerprint_parameters.h"

namespace SFinGe {

/**
 * @brief Classe responsável pela renderização avançada de impressões digitais
 * 
 * Esta classe implementa um pipeline de texturização que transforma o mapa
 * de cristas binário em uma imagem final realista, incluindo:
 * - Textura de fundo com vinheta
 * - Ruído Perlin para textura da pele
 * - Simulação de poros de suor
 * - Normalização de contraste
 */
class TextureRenderer {
public:
    /**
     * @brief Construtor
     * @param params Parâmetros de renderização
     * @param width Largura da imagem
     * @param height Altura da imagem
     * @param seed Semente para reprodutibilidade
     */
    TextureRenderer(const RenderingParameters& params, int width, int height, unsigned int seed);

    /**
     * @brief Método principal que executa o pipeline de renderização
     * @param ridgeMap Mapa de cristas (valores 0-1, onde 1 = crista)
     * @param shapeMap Mapa de forma da impressão digital (máscara alfa)
     * @return Imagem renderizada como vetor de floats [0, 1]
     */
    std::vector<float> render(const std::vector<float>& ridgeMap,
                              const std::vector<float>& shapeMap) const;

private:
    /**
     * @brief Gera o fundo com vinheta e ruído
     * @return Vetor com valores de intensidade do fundo
     */
    std::vector<float> generateBackground() const;

    /**
     * @brief Aplica textura de pele às cristas e vales
     * @param ridges Mapa de cristas
     * @return Imagem com textura aplicada
     */
    std::vector<float> applyTexture(const std::vector<float>& ridges) const;

    /**
     * @brief Adiciona simulação de poros de suor
     * @param texturedRidges Imagem com textura
     * @param ridgeMask Máscara booleana das cristas
     * @return Imagem com poros adicionados
     */
    std::vector<float> addPores(const std::vector<float>& texturedRidges,
                                const std::vector<bool>& ridgeMask) const;

    /**
     * @brief Normaliza o contraste da imagem
     * @param image Imagem de entrada
     * @param shapeMap Mapa de forma para mascaramento
     * @return Imagem com contraste normalizado
     */
    std::vector<float> normalizeContrast(const std::vector<float>& image,
                                         const std::vector<float>& shapeMap) const;

    /**
     * @brief Aplica blur gaussiano à imagem
     * @param image Imagem de entrada
     * @param sigma Desvio padrão do kernel gaussiano
     * @return Imagem suavizada
     */
    std::vector<float> applyGaussianBlur(const std::vector<float>& image, double sigma) const;

    RenderingParameters m_params;
    int m_width;
    int m_height;
    std::unique_ptr<PerlinNoise> m_perlin;
    mutable std::mt19937 m_rng;
};

} // namespace SFinGe

#endif // TEXTURE_RENDERER_H
