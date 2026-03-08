#include "texture_renderer.h"
#include <algorithm>
#include <cmath>

namespace SFinGe {

TextureRenderer::TextureRenderer(const RenderingParameters& params, int width, int height, unsigned int seed)
    : m_params(params)
    , m_width(width)
    , m_height(height)
    , m_perlin(std::make_unique<PerlinNoise>(seed))
    , m_rng(seed)
{
}

std::vector<float> TextureRenderer::render(const std::vector<float>& ridgeMap,
                                           const std::vector<float>& shapeMap,
                                           const std::vector<double>& orientationMap) const {

    // Passo 1: Aplicar anti-aliasing ao ridge map (se habilitado)
    std::vector<float> processedRidgeMap = ridgeMap;
    if (m_params.enableAntiAliasing) {
        processedRidgeMap = applyAntiAliasing(ridgeMap);
    }

    // Passo 2: Gerar fundo com vinheta e ruído
    auto background = generateBackground();

    // Passo 3: Aplicar textura às cristas
    auto textured = applyTexture(processedRidgeMap);

    // Passo 4: Criar máscara de cristas (usar processedRidgeMap para anti-aliasing)
    std::vector<bool> ridgeMask(m_width * m_height);
    for (size_t i = 0; i < processedRidgeMap.size(); ++i) {
        ridgeMask[i] = processedRidgeMap[i] > 0.5f;
    }
    
    // Passo 5: Adicionar poros (se habilitado)
    if (m_params.enablePores) {
        textured = addPores(textured, ridgeMask, orientationMap);
    }
    
    // Passo 6: Combinar com fundo usando shapeMap como máscara alfa
    std::vector<float> combined(m_width * m_height);
    for (size_t i = 0; i < combined.size(); ++i) {
        float alpha = shapeMap.empty() ? 1.0f : shapeMap[i];
        combined[i] = textured[i] * alpha + background[i] * (1.0f - alpha);
    }

    // Passo 7: Aplicar iluminação direcional (se habilitado)
    if (m_params.enableDirectionalLighting) {
        combined = applyDirectionalLighting(combined, processedRidgeMap, shapeMap);
    }

    // Passo 8: Aplicar manchas e artefatos (se habilitado)
    if (m_params.enableArtifacts) {
        combined = applyArtifacts(combined, processedRidgeMap, shapeMap);
    }

    // Passo 9: Aplicar blur final
    if (m_params.finalBlurSigma > 0) {
        combined = applyGaussianBlur(combined, m_params.finalBlurSigma);
    }

    // Passo 10: Normalizar contraste
    combined = normalizeContrast(combined, shapeMap);
    
    return combined;
}

std::vector<float> TextureRenderer::applyAntiAliasing(const std::vector<float>& ridgeMap) const {
    if (!m_params.enableAntiAliasing || m_params.antiAliasingSigma <= 0) {
        return ridgeMap;
    }

    // Aplicar um pequeno blur gaussiano para suavizar as transições
    // Isso converte bordas duras (0→1) em gradientes suaves (0→...→1)
    std::vector<float> smoothed = applyGaussianBlur(ridgeMap, m_params.antiAliasingSigma);

    // Opcional: Ajustar o contraste para manter a definição das cristas
    // mas preservar a suavidade nas bordas
    if (m_params.ridgeTransitionWidth > 0.5) {
        // Aplicar uma curva sigmóide suave para preservar contraste interno
        // mas manter transições suaves
        double steepness = 4.0 / m_params.ridgeTransitionWidth;
        for (size_t i = 0; i < smoothed.size(); ++i) {
            // Sigmoid: 1 / (1 + exp(-steepness * (x - 0.5)))
            double x = smoothed[i];
            double sigmoid = 1.0 / (1.0 + std::exp(-steepness * (x - 0.5)));
            smoothed[i] = static_cast<float>(sigmoid);
        }
    }

    return smoothed;
}

std::vector<float> TextureRenderer::generateBackground() const {
    std::vector<float> background(m_width * m_height);
    
    // Cor base do fundo (cinza claro)
    const float baseColor = 0.92f;
    
    // Centro da imagem para vinheta
    float centerX = m_width / 2.0f;
    float centerY = m_height / 2.0f;
    float maxDist = std::sqrt(centerX * centerX + centerY * centerY);
    
    // Gerar ruído de baixa frequência para o fundo
    auto noiseField = m_perlin->fractal(m_width, m_height, 
                                        m_params.backgroundNoiseFrequency,
                                        4, 0.5, 2.0);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            int idx = j * m_width + i;
            
            // Calcular vinheta (escurecimento nas bordas)
            float dx = i - centerX;
            float dy = j - centerY;
            float dist = std::sqrt(dx * dx + dy * dy) / maxDist;
            float vignette = 1.0f - 0.3f * dist * dist;
            
            // Combinar cor base, vinheta e ruído
            float noise = static_cast<float>(noiseField[idx] - 0.5) * 2.0f * m_params.backgroundNoiseAmplitude;
            background[idx] = std::clamp(baseColor * vignette + noise, 0.0f, 1.0f);
        }
    }
    
    return background;
}

std::vector<float> TextureRenderer::applyTexture(const std::vector<float>& ridges) const {
    // Implementação do Cappelli 2004, Step 8:
    // a) Isola camada de vale
    // b) Aplica textura/ruído às cristas
    // c) Suaviza com janela 3×3
    // d) Sobrepõe camada de vale (restaura bordas nítidas)

    std::vector<float> textured(m_width * m_height);

    // Gerar campos de ruído separados para cristas e vales
    auto ridgeNoise = m_perlin->fractal(m_width, m_height,
                                        m_params.ridgeNoiseFrequency,
                                        3, 0.5, 2.0);
    auto valleyNoise = m_perlin->fractal(m_width, m_height,
                                         m_params.valleyNoiseFrequency,
                                         3, 0.5, 2.0);

    // Valores base para cristas (escuras) e vales (claros)
    // 0.22/0.80 em vez de 0.15/0.85 → menos contraste, aparência menos binarizada
    const float ridgeBase  = 0.22f;
    const float valleyBase = 0.80f;

    // Step 8b: Aplicar textura com blend suave (preserva anti-aliasing)
    for (size_t i = 0; i < ridges.size(); ++i) {
        const float ridgeValue = ridges[i];

        const float rNoise = static_cast<float>(ridgeNoise[i]  - 0.5) * 2.0f * m_params.ridgeNoiseAmplitude;
        const float vNoise = static_cast<float>(valleyNoise[i] - 0.5) * 2.0f * m_params.valleyNoiseAmplitude;

        const float blendedBase  = ridgeValue * ridgeBase  + (1.0f - ridgeValue) * valleyBase;
        const float blendedNoise = ridgeValue * rNoise     + (1.0f - ridgeValue) * vNoise;

        textured[i] = std::clamp(blendedBase + blendedNoise, 0.0f, 1.0f);
    }

    // Step 8c: Suavização 3×3 apenas nos pixels de crista
    // (evita borrar os contornos nítidos dos vales)
    std::vector<float> smoothed = textured;
    for (int y = 1; y < m_height - 1; ++y) {
        for (int x = 1; x < m_width - 1; ++x) {
            const int idx = y * m_width + x;
            if (ridges[idx] < 0.5f) continue;  // Vale: não suavizar

            float sum = 0.0f;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    sum += textured[(y + dy) * m_width + (x + dx)];
            smoothed[idx] = sum / 9.0f;
        }
    }

    // Step 8d: Restaurar pixels de vale sem suavização (bordas nítidas)
    for (size_t i = 0; i < ridges.size(); ++i) {
        if (ridges[i] < 0.5f) {
            smoothed[i] = textured[i];
        }
    }

    return smoothed;
}

std::vector<float> TextureRenderer::addPores(const std::vector<float>& texturedRidges,
                                              const std::vector<bool>& ridgeMask,
                                              const std::vector<double>& orientationMap) const {
    std::vector<float> result = texturedRidges;

    // Contar pixels de crista
    int ridgePixelCount = 0;
    std::vector<int> ridgeIndices;
    for (size_t i = 0; i < ridgeMask.size(); ++i) {
        if (ridgeMask[i]) {
            ridgePixelCount++;
            ridgeIndices.push_back(static_cast<int>(i));
        }
    }

    if (ridgeIndices.empty()) {
        return result;
    }

    // Calcular número de poros
    int numPores = static_cast<int>(ridgePixelCount * m_params.poreDensity);

    // Distribuições estatísticas
    std::uniform_int_distribution<int> indexDist(0, static_cast<int>(ridgeIndices.size()) - 1);
    std::uniform_real_distribution<double> uniformDist(0.0, 1.0);
    std::uniform_real_distribution<double> intensityDist(m_params.minPoreIntensity, m_params.maxPoreIntensity);
    std::uniform_real_distribution<double> angleDist(0.0, 2.0 * M_PI);
    std::uniform_real_distribution<double> aspectDist(m_params.poreEllipseAspectMin, m_params.poreEllipseAspectMax);

    // Distribuição log-normal para tamanhos mais realistas
    std::lognormal_distribution<double> sizeDist(std::log(m_params.poreMeanSize), m_params.poreSizeStdDev);

    // Tipos de poros
    enum class PoreType { Circular, Elliptical, Irregular, IncipientRidge };

    // Processar poros (incluindo clusters se habilitado)
    int poresAdded = 0;
    while (poresAdded < numPores) {
        // Decidir se será um cluster ou poro único
        bool isCluster = m_params.enablePoreClustering &&
                        (uniformDist(m_rng) < m_params.poreClusteringFactor);

        int poresInGroup = isCluster ? m_params.poreClusterSize : 1;
        int baseIdx = ridgeIndices[indexDist(m_rng)];
        int baseX = baseIdx % m_width;
        int baseY = baseIdx / m_width;

        for (int i = 0; i < poresInGroup && poresAdded < numPores; ++i) {
            // Posição do poro (com offset se for cluster)
            int x = baseX;
            int y = baseY;
            if (isCluster && i > 0) {
                // Offset para cluster (raio de 2-5 pixels)
                double clusterRadius = 2.0 + uniformDist(m_rng) * 3.0;
                double clusterAngle = angleDist(m_rng);
                x += static_cast<int>(clusterRadius * std::cos(clusterAngle));
                y += static_cast<int>(clusterRadius * std::sin(clusterAngle));
                x = std::clamp(x, 0, m_width - 1);
                y = std::clamp(y, 0, m_height - 1);
            }

            // Determinar tipo de poro
            double typeRoll = uniformDist(m_rng);
            PoreType poreType;
            if (typeRoll < m_params.incipientRidgeRatio) {
                poreType = PoreType::IncipientRidge;
            } else {
                double shapeRoll = uniformDist(m_rng);
                double circularThreshold = m_params.poreCircularRatio;
                double ellipticalThreshold = circularThreshold + m_params.poreEllipticalRatio;

                if (shapeRoll < circularThreshold) {
                    poreType = PoreType::Circular;
                } else if (shapeRoll < ellipticalThreshold) {
                    poreType = PoreType::Elliptical;
                } else {
                    poreType = PoreType::Irregular;
                }
            }

            // Gerar tamanho com distribuição log-normal (mais realista)
            double poreSize = sizeDist(m_rng);
            poreSize = std::clamp(poreSize, m_params.poreMinSize, m_params.poreMaxSize);

            // Intensidade e opacidade com variação
            double baseIntensity = intensityDist(m_rng);
            double opacity = 1.0 - uniformDist(m_rng) * m_params.poreOpacityVariation;
            double poreIntensity = baseIntensity * opacity;

            // Ângulo da crista no pixel corrente:
            // O orientationMap armazena ângulo perpendicular às cristas (para Gabor).
            // O eixo das cristas = orientationMap - π/2.
            // Adiciona variação aleatória pequena (±15°) para naturalidade.
            const bool hasOrient = !orientationMap.empty() &&
                                   (static_cast<size_t>(y * m_width + x) < orientationMap.size());
            double ridgeAngle;
            if (hasOrient) {
                // Converter de perpendicular-à-crista para direção-da-crista
                ridgeAngle = orientationMap[y * m_width + x] - M_PI / 2.0;
                // Pequena variação aleatória (±15°)
                ridgeAngle += (uniformDist(m_rng) - 0.5) * (M_PI / 6.0);
            } else {
                ridgeAngle = angleDist(m_rng);
            }

            // Renderizar poro baseado no tipo
            if (poreType == PoreType::Circular) {
                // Poro circular clássico
                renderCircularPore(result, x, y, poreSize, poreIntensity);
            } else if (poreType == PoreType::Elliptical) {
                // Poro elíptico orientado na direção da crista
                double aspectRatio = aspectDist(m_rng);
                renderEllipticalPore(result, x, y, poreSize, aspectRatio, ridgeAngle, poreIntensity);
            } else if (poreType == PoreType::Irregular) {
                // Poro irregular (forma orgânica)
                renderIrregularPore(result, x, y, poreSize, poreIntensity);
            } else { // IncipientRidge
                // Crista incipiente alinhada à direção da crista
                double length = poreSize * 2.0 + uniformDist(m_rng) * 2.0;
                renderIncipientRidge(result, x, y, length, ridgeAngle, poreIntensity * 0.6);
            }

            poresAdded++;
        }
    }

    return result;
}

// Métodos auxiliares para renderizar diferentes tipos de poros
void TextureRenderer::renderCircularPore(std::vector<float>& image, int cx, int cy,
                                        double radius, double intensity) const {
    // Poro = buraco branco (luminoso) na crista escura.
    // Interpolamos em direção ao valor de vale (claro) usando falloff quadrático.
    const float poreTarget = 0.82f;  // valor-alvo (próximo do vale ≈ 0.85)
    int iRadius = static_cast<int>(std::ceil(radius));
    for (int dy = -iRadius; dy <= iRadius; ++dy) {
        for (int dx = -iRadius; dx <= iRadius; ++dx) {
            int nx = cx + dx;
            int ny = cy + dy;
            if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                double dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= radius) {
                    double falloff = 1.0 - (dist / radius);
                    falloff = falloff * falloff;  // suavização quadrática
                    int idx = ny * m_width + nx;
                    float blend = static_cast<float>(intensity * falloff);
                    image[idx] = image[idx] * (1.0f - blend) + poreTarget * blend;
                    image[idx] = std::clamp(image[idx], 0.0f, 1.0f);
                }
            }
        }
    }
}

void TextureRenderer::renderEllipticalPore(std::vector<float>& image, int cx, int cy,
                                          double semiMajor, double aspectRatio,
                                          double angle, double intensity) const {
    double semiMinor = semiMajor / aspectRatio;
    int maxRadius = static_cast<int>(std::ceil(semiMajor * aspectRatio));

    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);

    for (int dy = -maxRadius; dy <= maxRadius; ++dy) {
        for (int dx = -maxRadius; dx <= maxRadius; ++dx) {
            int nx = cx + dx;
            int ny = cy + dy;

            if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                // Rotacionar coordenadas
                double xRot = dx * cosAngle + dy * sinAngle;
                double yRot = -dx * sinAngle + dy * cosAngle;

                // Distância elíptica normalizada
                double ellipseDist = (xRot * xRot) / (semiMajor * semiMajor) +
                                    (yRot * yRot) / (semiMinor * semiMinor);

                if (ellipseDist <= 1.0) {
                    double falloff = 1.0 - ellipseDist;
                    falloff = falloff * falloff;
                    int idx = ny * m_width + nx;
                    const float poreTarget = 0.82f;
                    float blend = static_cast<float>(intensity * falloff);
                    image[idx] = image[idx] * (1.0f - blend) + poreTarget * blend;
                    image[idx] = std::clamp(image[idx], 0.0f, 1.0f);
                }
            }
        }
    }
}

void TextureRenderer::renderIrregularPore(std::vector<float>& image, int cx, int cy,
                                         double baseRadius, double intensity) const {
    // Usar Perlin noise para criar forma irregular
    int iRadius = static_cast<int>(std::ceil(baseRadius * 1.5));

    for (int dy = -iRadius; dy <= iRadius; ++dy) {
        for (int dx = -iRadius; dx <= iRadius; ++dx) {
            int nx = cx + dx;
            int ny = cy + dy;

            if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                double dist = std::sqrt(dx * dx + dy * dy);

                // Usar Perlin noise para perturbar o raio
                double angle = std::atan2(dy, dx);
                double noiseValue = m_perlin->noise(cx * 0.1 + std::cos(angle) * 5.0,
                                                    cy * 0.1 + std::sin(angle) * 5.0);
                double perturbedRadius = baseRadius * (1.0 + noiseValue * 0.4);

                if (dist <= perturbedRadius) {
                    double falloff = 1.0 - (dist / perturbedRadius);
                    falloff = falloff * falloff;
                    int idx = ny * m_width + nx;
                    const float poreTarget = 0.82f;
                    float blend = static_cast<float>(intensity * falloff);
                    image[idx] = image[idx] * (1.0f - blend) + poreTarget * blend;
                    image[idx] = std::clamp(image[idx], 0.0f, 1.0f);
                }
            }
        }
    }
}

void TextureRenderer::renderIncipientRidge(std::vector<float>& image, int cx, int cy,
                                          double length, double angle, double intensity) const {
    // Renderizar uma linha fina (crista incipiente)
    double cosA = std::cos(angle);
    double sinA = std::sin(angle);
    int steps = static_cast<int>(std::ceil(length));

    for (int i = 0; i < steps; ++i) {
        double t = (i - steps / 2.0) / steps; // -0.5 a 0.5
        int x = cx + static_cast<int>(t * length * cosA);
        int y = cy + static_cast<int>(t * length * sinA);

        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            // Linha com espessura 0.5-1.0 pixels
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                        double dist = std::sqrt(dx * dx + dy * dy);
                        if (dist <= 0.7) {
                            double falloff = 1.0 - dist / 0.7;
                            int idx = ny * m_width + nx;
                            image[idx] = std::clamp(image[idx] + static_cast<float>(intensity * falloff), 0.0f, 1.0f);
                        }
                    }
                }
            }
        }
    }
}

std::vector<float> TextureRenderer::applyDirectionalLighting(const std::vector<float>& image,
                                                            const std::vector<float>& ridgeMap,
                                                            const std::vector<float>& shapeMap) const {
    if (!m_params.enableDirectionalLighting) {
        return image;
    }

    std::vector<float> result = image;

    // Converter ângulos de graus para radianos
    const double PI = 3.14159265358979323846;
    double azimuthRad = m_params.lightAzimuth * PI / 180.0;
    double elevationRad = m_params.lightElevation * PI / 180.0;

    // Calcular vetor de direção da luz (normalizado)
    double lightX = std::cos(elevationRad) * std::cos(azimuthRad);
    double lightY = std::cos(elevationRad) * std::sin(azimuthRad);
    double lightZ = std::sin(elevationRad);

    // Vetor da câmera (sempre perpendicular à superfície para vista top-down)
    const double viewX = 0.0;
    const double viewY = 0.0;
    const double viewZ = 1.0;

    // Para cada pixel, calcular iluminação
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;

            // Verificar se está dentro da região da impressão digital
            float alpha = shapeMap.empty() ? 1.0f : shapeMap[idx];
            if (alpha < 0.1f) {
                continue; // Pular região fora da impressão
            }

            // Calcular gradiente da altura (baseado no ridge map)
            // Valores maiores = cristas (elevadas), valores menores = vales (deprimidos)
            double heightCenter = ridgeMap[idx] * m_params.ridgeHeight;

            // Gradientes usando diferenças finitas (Sobel-like)
            double gx = 0.0, gy = 0.0;
            if (x > 0 && x < m_width - 1) {
                double heightLeft = ridgeMap[y * m_width + (x - 1)] * m_params.ridgeHeight;
                double heightRight = ridgeMap[y * m_width + (x + 1)] * m_params.ridgeHeight;
                gx = (heightRight - heightLeft) * 0.5;
            }
            if (y > 0 && y < m_height - 1) {
                double heightTop = ridgeMap[(y - 1) * m_width + x] * m_params.ridgeHeight;
                double heightBottom = ridgeMap[(y + 1) * m_width + x] * m_params.ridgeHeight;
                gy = (heightBottom - heightTop) * 0.5;
            }

            // Normal da superfície (normalizado)
            // A normal aponta "para cima" da superfície
            double nx = -gx;
            double ny = -gy;
            double nz = 1.0;
            double nLen = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= nLen;
            ny /= nLen;
            nz /= nLen;

            // Iluminação difusa (Lambertian): dot(N, L)
            double diffuse = std::max(0.0, nx * lightX + ny * lightY + nz * lightZ);

            // Iluminação especular (Blinn-Phong)
            double specular = 0.0;
            if (m_params.enableSpecular && diffuse > 0.0) {
                // Vetor half-way entre luz e câmera
                double hx = lightX + viewX;
                double hy = lightY + viewY;
                double hz = lightZ + viewZ;
                double hLen = std::sqrt(hx * hx + hy * hy + hz * hz);
                hx /= hLen;
                hy /= hLen;
                hz /= hLen;

                // Specular = dot(N, H)^shininess
                double ndoth = std::max(0.0, nx * hx + ny * hy + nz * hz);
                specular = std::pow(ndoth, m_params.specularShininess) * m_params.specularStrength;
            }

            // Combinar luz ambiente + difusa + especular
            double lighting = m_params.ambientLight +
                            diffuse * m_params.lightIntensity +
                            specular;
            lighting = std::clamp(lighting, 0.0, 2.0); // Permitir overexposure moderado

            // Aplicar iluminação à imagem
            result[idx] = std::clamp(static_cast<float>(image[idx] * lighting), 0.0f, 1.0f);
        }
    }

    return result;
}

std::vector<float> TextureRenderer::applyArtifacts(const std::vector<float>& image,
                                                   const std::vector<float>& ridgeMap,
                                                   const std::vector<float>& shapeMap) const {
    if (!m_params.enableArtifacts) {
        return image;
    }

    std::vector<float> result = image;

    std::uniform_real_distribution<double> uniformDist(0.0, 1.0);
    std::uniform_real_distribution<double> angleDist(0.0, 2.0 * 3.14159265358979323846);

    // 1. Manchas de umidade (áreas mais brilhantes)
    if (m_params.enableMoisture) {
        int numMoisture = static_cast<int>(m_width * m_height * m_params.moistureDensity);
        for (int i = 0; i < numMoisture; ++i) {
            int cx = static_cast<int>(uniformDist(m_rng) * m_width);
            int cy = static_cast<int>(uniformDist(m_rng) * m_height);
            int centerIdx = cy * m_width + cx;

            // Verificar se está dentro da impressão
            float alpha = shapeMap.empty() ? 1.0f : shapeMap[centerIdx];
            if (alpha < 0.05f) continue;

            double size = m_params.moistureMinSize +
                         uniformDist(m_rng) * (m_params.moistureMaxSize - m_params.moistureMinSize);
            double intensity = m_params.moistureIntensity * (0.5 + uniformDist(m_rng) * 0.5);

            // Renderizar mancha com gradiente suave usando Perlin noise
            int iRadius = static_cast<int>(std::ceil(size));
            for (int dy = -iRadius; dy <= iRadius; ++dy) {
                for (int dx = -iRadius; dx <= iRadius; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                        double dist = std::sqrt(dx * dx + dy * dy);
                        if (dist <= size) {
                            // Gradiente + Perlin noise para forma irregular
                            double falloff = 1.0 - (dist / size);
                            falloff = falloff * falloff;
                            double noiseValue = m_perlin->noise(nx * 0.1, ny * 0.1);
                            double effect = falloff * (0.7 + 0.3 * noiseValue) * intensity;

                            int idx = ny * m_width + nx;
                            result[idx] = std::clamp(result[idx] + static_cast<float>(effect), 0.0f, 1.0f);
                        }
                    }
                }
            }
        }
    }

    // 2. Sujeira / Manchas escuras
    if (m_params.enableDirt) {
        int numDirt = static_cast<int>(m_width * m_height * m_params.dirtDensity);
        for (int i = 0; i < numDirt; ++i) {
            int cx = static_cast<int>(uniformDist(m_rng) * m_width);
            int cy = static_cast<int>(uniformDist(m_rng) * m_height);
            int centerIdx = cy * m_width + cx;

            float alpha = shapeMap.empty() ? 1.0f : shapeMap[centerIdx];
            if (alpha < 0.05f) continue;

            double size = m_params.dirtMinSize +
                         uniformDist(m_rng) * (m_params.dirtMaxSize - m_params.dirtMinSize);
            double intensity = m_params.dirtIntensity * (0.5 + uniformDist(m_rng) * 0.5);

            int iRadius = static_cast<int>(std::ceil(size));
            for (int dy = -iRadius; dy <= iRadius; ++dy) {
                for (int dx = -iRadius; dx <= iRadius; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                        double dist = std::sqrt(dx * dx + dy * dy);
                        // Forma irregular usando Perlin noise
                        double angle = std::atan2(dy, dx);
                        double noiseValue = m_perlin->noise(cx * 0.05 + std::cos(angle) * 3.0,
                                                           cy * 0.05 + std::sin(angle) * 3.0);
                        double perturbedSize = size * (0.8 + 0.4 * noiseValue);

                        if (dist <= perturbedSize) {
                            double falloff = 1.0 - (dist / perturbedSize);
                            falloff = falloff * falloff;
                            double effect = falloff * intensity;

                            int idx = ny * m_width + nx;
                            result[idx] = std::clamp(result[idx] - static_cast<float>(effect), 0.0f, 1.0f);
                        }
                    }
                }
            }
        }
    }

    // 3. Cicatrizes (linhas que interrompem cristas)
    if (m_params.enableScars) {
        for (int i = 0; i < m_params.numScars; ++i) {
            int startX = static_cast<int>(uniformDist(m_rng) * m_width);
            int startY = static_cast<int>(uniformDist(m_rng) * m_height);

            double angle = angleDist(m_rng);
            double length = m_params.scarMinLength +
                          uniformDist(m_rng) * (m_params.scarMaxLength - m_params.scarMinLength);

            int endX = startX + static_cast<int>(length * std::cos(angle));
            int endY = startY + static_cast<int>(length * std::sin(angle));

            // Bresenham-like line drawing com largura
            int dx = std::abs(endX - startX);
            int dy = std::abs(endY - startY);
            int sx = startX < endX ? 1 : -1;
            int sy = startY < endY ? 1 : -1;
            int err = dx - dy;

            int x = startX, y = startY;
            int steps = 0;
            int maxSteps = dx + dy + 1;

            while (steps < maxSteps) {
                // Renderizar com largura
                int iWidth = static_cast<int>(std::ceil(m_params.scarWidth));
                for (int wy = -iWidth; wy <= iWidth; ++wy) {
                    for (int wx = -iWidth; wx <= iWidth; ++wx) {
                        int nx = x + wx;
                        int ny = y + wy;
                        if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                            double dist = std::sqrt(wx * wx + wy * wy);
                            if (dist <= m_params.scarWidth) {
                                int idx = ny * m_width + nx;
                                float alpha = shapeMap.empty() ? 1.0f : shapeMap[idx];
                                if (alpha > 0.05f) {
                                    double falloff = 1.0 - (dist / m_params.scarWidth);
                                    // Degradê longitudinal senoidal: fade nas pontas, máximo no centro
                                    double longi  = std::sin(M_PI * static_cast<double>(steps) / maxSteps);
                                    // Cicatrizes = ausência de tinta → clareiam (não escurecem)
                                    double effect = m_params.scarIntensity * falloff * longi;
                                    result[idx] = std::clamp(result[idx] + static_cast<float>((1.0f - result[idx]) * effect), 0.0f, 1.0f);
                                }
                            }
                        }
                    }
                }

                if (x == endX && y == endY) break;
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x += sx; }
                if (e2 < dx) { err += dx; y += sy; }
                steps++;
            }
        }
    }

    // 4. Ressecamento / Rachaduras finas
    if (m_params.enableDryness) {
        for (int i = 0; i < m_params.numDryCracks; ++i) {
            int startX = static_cast<int>(uniformDist(m_rng) * m_width);
            int startY = static_cast<int>(uniformDist(m_rng) * m_height);

            double angle = angleDist(m_rng);
            double length = m_params.dryCrackLength * (0.5 + uniformDist(m_rng) * 1.0);

            int endX = startX + static_cast<int>(length * std::cos(angle));
            int endY = startY + static_cast<int>(length * std::sin(angle));

            // Linha fina com variação
            int dx = std::abs(endX - startX);
            int dy = std::abs(endY - startY);
            int sx = startX < endX ? 1 : -1;
            int sy = startY < endY ? 1 : -1;
            int err = dx - dy;

            int x = startX, y = startY;
            int steps = 0;
            int maxSteps = dx + dy + 1;

            while (steps < maxSteps) {
                if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
                    int idx = y * m_width + x;
                    float alpha = shapeMap.empty() ? 1.0f : shapeMap[idx];
                    if (alpha > 0.05f) {
                        result[idx] = std::clamp(result[idx] - static_cast<float>(m_params.dryCrackIntensity),
                                                0.0f, 1.0f);
                    }
                }

                if (x == endX && y == endY) break;
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x += sx; }
                if (e2 < dx) { err += dx; y += sy; }
                steps++;
            }
        }
    }

    // 5. Borrões / Smudges (tinta excessiva)
    if (m_params.enableSmudges) {
        int numSmudges = static_cast<int>(m_width * m_height * m_params.smudgeDensity);
        for (int i = 0; i < numSmudges; ++i) {
            int cx = static_cast<int>(uniformDist(m_rng) * m_width);
            int cy = static_cast<int>(uniformDist(m_rng) * m_height);
            int centerIdx = cy * m_width + cx;

            float alpha = shapeMap.empty() ? 1.0f : shapeMap[centerIdx];
            if (alpha < 0.05f) continue;

            double size = m_params.smudgeMinSize +
                         uniformDist(m_rng) * (m_params.smudgeMaxSize - m_params.smudgeMinSize);
            double intensity = m_params.smudgeIntensity * (0.6 + uniformDist(m_rng) * 0.4);

            // Borrões com forma alongada (direção aleatória)
            double smudgeAngle = angleDist(m_rng);
            double aspectRatio = 1.5 + uniformDist(m_rng) * 1.0; // 1.5 a 2.5

            int iRadius = static_cast<int>(std::ceil(size * aspectRatio));
            for (int dy = -iRadius; dy <= iRadius; ++dy) {
                for (int dx = -iRadius; dx <= iRadius; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                        // Rotacionar para direção do smudge
                        double xRot = dx * std::cos(smudgeAngle) + dy * std::sin(smudgeAngle);
                        double yRot = -dx * std::sin(smudgeAngle) + dy * std::cos(smudgeAngle);

                        // Distância elíptica
                        double ellipseDist = (xRot * xRot) / (size * aspectRatio * size * aspectRatio) +
                                           (yRot * yRot) / (size * size);

                        if (ellipseDist <= 1.0) {
                            double falloff = 1.0 - ellipseDist;
                            falloff = falloff * falloff;
                            double effect = falloff * intensity;

                            int idx = ny * m_width + nx;
                            result[idx] = std::clamp(result[idx] - static_cast<float>(effect), 0.0f, 1.0f);
                        }
                    }
                }
            }
        }
    }

    return result;
}

std::vector<float> TextureRenderer::normalizeContrast(const std::vector<float>& image,
                                                       const std::vector<float>& shapeMap) const {
    std::vector<float> result = image;
    
    // Coletar valores dentro da região da impressão digital (inclui zona de feathering)
    std::vector<float> values;
    for (size_t i = 0; i < image.size(); ++i) {
        float alpha = shapeMap.empty() ? 1.0f : shapeMap[i];
        if (alpha > 0.05f) {
            values.push_back(image[i]);
        }
    }
    
    if (values.empty()) {
        return result;
    }
    
    // Ordenar para calcular percentis
    std::sort(values.begin(), values.end());
    
    // Calcular percentis inferior e superior
    size_t lowerIdx = static_cast<size_t>(values.size() * m_params.contrastPercentileLower / 100.0);
    size_t upperIdx = static_cast<size_t>(values.size() * m_params.contrastPercentileUpper / 100.0);
    upperIdx = std::min(upperIdx, values.size() - 1);
    
    float lowerVal = values[lowerIdx];
    float upperVal = values[upperIdx];
    
    if (upperVal <= lowerVal) {
        return result;
    }
    
    // Normalizar contraste
    float range = upperVal - lowerVal;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = std::clamp((result[i] - lowerVal) / range, 0.0f, 1.0f);
    }
    
    return result;
}

std::vector<float> TextureRenderer::applyGaussianBlur(const std::vector<float>& image, double sigma) const {
    if (sigma <= 0) {
        return image;
    }
    
    std::vector<float> result = image;
    
    // Calcular tamanho do kernel (6*sigma para capturar 99.7% da distribuição)
    int kernelSize = static_cast<int>(std::ceil(sigma * 6.0));
    if (kernelSize % 2 == 0) kernelSize++;
    int halfKernel = kernelSize / 2;
    
    // Criar kernel gaussiano 1D
    std::vector<float> kernel(kernelSize);
    float kernelSum = 0.0f;
    for (int i = 0; i < kernelSize; ++i) {
        float x = static_cast<float>(i - halfKernel);
        kernel[i] = std::exp(-x * x / (2.0f * sigma * sigma));
        kernelSum += kernel[i];
    }
    // Normalizar
    for (auto& k : kernel) {
        k /= kernelSum;
    }
    
    // Aplicar blur separável (horizontal)
    std::vector<float> temp(m_width * m_height);
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            float sum = 0.0f;
            for (int k = -halfKernel; k <= halfKernel; ++k) {
                int ni = std::clamp(i + k, 0, m_width - 1);
                sum += image[j * m_width + ni] * kernel[k + halfKernel];
            }
            temp[j * m_width + i] = sum;
        }
    }
    
    // Aplicar blur separável (vertical)
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            float sum = 0.0f;
            for (int k = -halfKernel; k <= halfKernel; ++k) {
                int nj = std::clamp(j + k, 0, m_height - 1);
                sum += temp[nj * m_width + i] * kernel[k + halfKernel];
            }
            result[j * m_width + i] = sum;
        }
    }
    
    return result;
}

} // namespace SFinGe
