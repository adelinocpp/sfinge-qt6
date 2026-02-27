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
     * @param orientationMap Mapa de orientação (ângulo em radianos, perpendicular às cristas).
     *        Se vazio, poros elípticos usam ângulo aleatório.
     * @return Imagem renderizada como vetor de floats [0, 1]
     */
    std::vector<float> render(const std::vector<float>& ridgeMap,
                              const std::vector<float>& shapeMap,
                              const std::vector<double>& orientationMap = {}) const;

private:
    /**
     * @brief Gera o fundo com vinheta e ruído
     * @return Vetor com valores de intensidade do fundo
     */
    std::vector<float> generateBackground() const;

    /**
     * @brief Aplica anti-aliasing às transições crista-vale
     * @param ridgeMap Mapa de cristas binário
     * @return Mapa de cristas com bordas suavizadas
     */
    std::vector<float> applyAntiAliasing(const std::vector<float>& ridgeMap) const;

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
     * @param orientationMap Mapa de orientação para alinhar poros elípticos (opcional)
     * @return Imagem com poros adicionados
     */
    std::vector<float> addPores(const std::vector<float>& texturedRidges,
                                const std::vector<bool>& ridgeMask,
                                const std::vector<double>& orientationMap) const;

    /**
     * @brief Aplica iluminação direcional simulando captura em ângulos diferentes
     * @param image Imagem de entrada
     * @param ridgeMap Mapa de cristas para calcular normais de superfície
     * @param shapeMap Mapa de forma para mascaramento
     * @return Imagem com iluminação direcional aplicada
     */
    std::vector<float> applyDirectionalLighting(const std::vector<float>& image,
                                                const std::vector<float>& ridgeMap,
                                                const std::vector<float>& shapeMap) const;

    /**
     * @brief Aplica manchas e artefatos realistas (umidade, sujeira, cicatrizes)
     * @param image Imagem de entrada
     * @param ridgeMap Mapa de cristas para posicionamento de artefatos
     * @param shapeMap Mapa de forma para mascaramento
     * @return Imagem com artefatos aplicados
     */
    std::vector<float> applyArtifacts(const std::vector<float>& image,
                                     const std::vector<float>& ridgeMap,
                                     const std::vector<float>& shapeMap) const;

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

    /**
     * @brief Renderiza um poro circular
     */
    void renderCircularPore(std::vector<float>& image, int cx, int cy,
                           double radius, double intensity) const;

    /**
     * @brief Renderiza um poro elíptico
     */
    void renderEllipticalPore(std::vector<float>& image, int cx, int cy,
                             double semiMajor, double aspectRatio,
                             double angle, double intensity) const;

    /**
     * @brief Renderiza um poro com forma irregular
     */
    void renderIrregularPore(std::vector<float>& image, int cx, int cy,
                            double baseRadius, double intensity) const;

    /**
     * @brief Renderiza uma crista incipiente (linha fina)
     */
    void renderIncipientRidge(std::vector<float>& image, int cx, int cy,
                             double length, double angle, double intensity) const;

    RenderingParameters m_params;
    int m_width;
    int m_height;
    std::unique_ptr<PerlinNoise> m_perlin;
    mutable std::mt19937 m_rng;
};

} // namespace SFinGe

#endif // TEXTURE_RENDERER_H
