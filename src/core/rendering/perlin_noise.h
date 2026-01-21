#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <vector>
#include <cstdint>

namespace SFinGe {

/**
 * @brief Implementação do algoritmo de Perlin Noise para geração de ruído coerente
 * 
 * Esta classe implementa o algoritmo clássico de Perlin Noise 2D,
 * usado para gerar texturas procedurais realistas nas impressões digitais.
 */
class PerlinNoise {
public:
    /**
     * @brief Construtor com semente para reprodutibilidade
     * @param seed Semente para o gerador de números aleatórios
     */
    explicit PerlinNoise(unsigned int seed = 0);

    /**
     * @brief Calcula o ruído Perlin 2D para um ponto (x, y)
     * @param x Coordenada X normalizada
     * @param y Coordenada Y normalizada
     * @return Valor de ruído no intervalo [-1, 1]
     */
    double noise(double x, double y) const;

    /**
     * @brief Gera ruído fractal (fBm - fractal Brownian motion) para um campo 2D
     * @param width Largura do campo
     * @param height Altura do campo
     * @param scale Escala base do ruído
     * @param octaves Número de oitavas (camadas de detalhe)
     * @param persistence Fator de persistência (redução de amplitude por oitava)
     * @param lacunarity Fator de lacunaridade (aumento de frequência por oitava)
     * @return Vetor com valores de ruído normalizados [0, 1]
     */
    std::vector<double> fractal(int width, int height, double scale,
                                int octaves = 4, double persistence = 0.5,
                                double lacunarity = 2.0) const;

private:
    // Função de interpolação suave (smoothstep)
    static double fade(double t);
    
    // Interpolação linear
    static double lerp(double t, double a, double b);
    
    // Calcula o produto escalar do gradiente
    double grad(int hash, double x, double y) const;
    
    // Tabela de permutação (256 valores, duplicados para evitar overflow)
    std::vector<int> p;
};

} // namespace SFinGe

#endif // PERLIN_NOISE_H
