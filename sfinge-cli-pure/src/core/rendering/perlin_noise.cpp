#include "perlin_noise.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

namespace SFinGe {

PerlinNoise::PerlinNoise(unsigned int seed) {
    p.resize(512);
    
    // Criar sequência de 0 a 255
    std::vector<int> permutation(256);
    std::iota(permutation.begin(), permutation.end(), 0);
    
    // Embaralhar usando a semente fornecida
    std::mt19937 rng(seed);
    std::shuffle(permutation.begin(), permutation.end(), rng);
    
    // Duplicar a tabela de permutação para evitar overflow
    for (int i = 0; i < 256; ++i) {
        p[i] = permutation[i];
        p[256 + i] = permutation[i];
    }
}

double PerlinNoise::fade(double t) {
    // 6t^5 - 15t^4 + 10t^3 (smootherstep de Ken Perlin)
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

double PerlinNoise::lerp(double t, double a, double b) {
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y) const {
    // Converte os 4 bits mais baixos do hash em um dos 8 gradientes
    int h = hash & 7;
    double u = h < 4 ? x : y;
    double v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
}

double PerlinNoise::noise(double x, double y) const {
    // Encontrar célula da grade
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    
    // Posição relativa dentro da célula
    x -= std::floor(x);
    y -= std::floor(y);
    
    // Curvas de interpolação suave
    double u = fade(x);
    double v = fade(y);
    
    // Hash das coordenadas dos 4 cantos da célula
    int A = p[X] + Y;
    int AA = p[A];
    int AB = p[A + 1];
    int B = p[X + 1] + Y;
    int BA = p[B];
    int BB = p[B + 1];
    
    // Interpolar os gradientes
    double res = lerp(v,
        lerp(u, grad(p[AA], x, y), grad(p[BA], x - 1, y)),
        lerp(u, grad(p[AB], x, y - 1), grad(p[BB], x - 1, y - 1))
    );
    
    // Normalizar para [-1, 1]
    return res;
}

std::vector<double> PerlinNoise::fractal(int width, int height, double scale,
                                          int octaves, double persistence,
                                          double lacunarity) const {
    std::vector<double> result(width * height, 0.0);
    
    double maxAmplitude = 0.0;
    double amplitude = 1.0;
    
    // Calcular amplitude máxima para normalização
    for (int o = 0; o < octaves; ++o) {
        maxAmplitude += amplitude;
        amplitude *= persistence;
    }
    
    // Gerar ruído fractal
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            double frequency = scale;
            amplitude = 1.0;
            double noiseValue = 0.0;
            
            for (int o = 0; o < octaves; ++o) {
                double sampleX = i * frequency;
                double sampleY = j * frequency;
                
                noiseValue += noise(sampleX, sampleY) * amplitude;
                
                amplitude *= persistence;
                frequency *= lacunarity;
            }
            
            // Normalizar para [0, 1]
            result[j * width + i] = (noiseValue / maxAmplitude + 1.0) * 0.5;
        }
    }
    
    return result;
}

} // namespace SFinGe
