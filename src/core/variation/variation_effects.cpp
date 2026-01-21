#include "variation_effects.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

namespace SFinGe {

VariationEffects::VariationEffects(const VariationParameters& params, unsigned int seed)
    : m_params(params)
    , m_rng(seed)
{
}

QImage VariationEffects::apply(const QImage& masterImage) const {
    if (masterImage.isNull()) {
        qWarning() << "[VariationEffects] Imagem mestre é nula";
        return QImage();
    }

    int width = masterImage.width();
    int height = masterImage.height();

    qDebug() << "[VariationEffects] Aplicando variações à imagem" << width << "x" << height;

    // Converter QImage para vetor de floats
    std::vector<float> image = qimageToVector(masterImage);

    // Aplicar distorção plástica
    if (m_params.enablePlasticDistortion) {
        image = applyPlasticDistortion(image, width, height);
        qDebug() << "[VariationEffects] Distorção plástica aplicada";
    }

    // Aplicar distorção de lente
    if (m_params.enableLensDistortion) {
        image = applyLensDistortion(image, width, height);
        qDebug() << "[VariationEffects] Distorção de lente aplicada";
    }

    // Aplicar rotação
    if (m_params.enableRotation) {
        image = applyRotation(image, width, height);
        qDebug() << "[VariationEffects] Rotação aplicada";
    }

    // Aplicar translação
    if (m_params.enableTranslation) {
        image = applyTranslation(image, width, height);
        qDebug() << "[VariationEffects] Translação aplicada";
    }

    // Aplicar condição da pele
    if (m_params.enableSkinCondition) {
        image = applySkinCondition(image, width, height);
        qDebug() << "[VariationEffects] Condição da pele aplicada";
    }

    // Converter de volta para QImage
    return vectorToQImage(image, width, height);
}

std::vector<float> VariationEffects::applyPlasticDistortion(const std::vector<float>& image,
                                                             int width, int height) const {
    std::vector<float> result(width * height, 1.0f);

    // Criar mapas de deslocamento
    std::vector<float> mapX(width * height);
    std::vector<float> mapY(width * height);

    // Inicializar com coordenadas identidade
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int idx = j * width + i;
            mapX[idx] = static_cast<float>(i);
            mapY[idx] = static_cast<float>(j);
        }
    }

    // Distribuições para parâmetros gaussianos
    std::uniform_real_distribution<double> centerXDist(width * 0.2, width * 0.8);
    std::uniform_real_distribution<double> centerYDist(height * 0.2, height * 0.8);
    std::uniform_real_distribution<double> sigmaDist(width * 0.1, width * 0.3);
    std::uniform_real_distribution<double> magDist(-m_params.plasticDistortionStrength, 
                                                    m_params.plasticDistortionStrength);
    std::uniform_real_distribution<double> angleDist(0, 2.0 * M_PI);

    // Adicionar múltiplos "solavancos" gaussianos
    for (int bump = 0; bump < m_params.plasticDistortionBumps; ++bump) {
        double cx = centerXDist(m_rng);
        double cy = centerYDist(m_rng);
        double sigma = sigmaDist(m_rng);
        double mag = magDist(m_rng);
        double angle = angleDist(m_rng);

        double cosAngle = std::cos(angle);
        double sinAngle = std::sin(angle);

        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                int idx = j * width + i;

                double dx = i - cx;
                double dy = j - cy;
                double distSq = dx * dx + dy * dy;

                // Gaussiana 2D
                double gaussian = std::exp(-distSq / (2.0 * sigma * sigma));

                // Adicionar deslocamento
                mapX[idx] += static_cast<float>(mag * cosAngle * gaussian);
                mapY[idx] += static_cast<float>(mag * sinAngle * gaussian);
            }
        }
    }

    // Aplicar remapeamento com interpolação bilinear
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int idx = j * width + i;
            result[idx] = bilinearSample(image, width, height, mapX[idx], mapY[idx]);
        }
    }

    return result;
}

std::vector<float> VariationEffects::applyLensDistortion(const std::vector<float>& image,
                                                          int width, int height) const {
    std::vector<float> result(width * height, 1.0f);

    // Centro da imagem
    double cx = width / 2.0;
    double cy = height / 2.0;

    // Normalização (raio máximo)
    double maxR = std::sqrt(cx * cx + cy * cy);

    // Coeficientes de distorção radial (modelo Brown-Conrady)
    double k1 = m_params.lensDistortionK1;
    double k2 = m_params.lensDistortionK2;

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // Coordenadas normalizadas relativas ao centro
            double x = (i - cx) / maxR;
            double y = (j - cy) / maxR;
            double r2 = x * x + y * y;
            double r4 = r2 * r2;

            // Fator de distorção radial
            double factor = 1.0 + k1 * r2 + k2 * r4;

            // Coordenadas distorcidas (invertidas para mapear de destino para origem)
            double srcX = cx + x * maxR / factor;
            double srcY = cy + y * maxR / factor;

            int idx = j * width + i;
            result[idx] = bilinearSample(image, width, height, 
                                         static_cast<float>(srcX), 
                                         static_cast<float>(srcY));
        }
    }

    return result;
}

std::vector<float> VariationEffects::applyRotation(const std::vector<float>& image,
                                                    int width, int height) const {
    std::vector<float> result(width * height, 1.0f);

    // Gerar ângulo aleatório dentro dos limites
    std::uniform_real_distribution<double> angleDist(-m_params.maxRotationAngle, 
                                                      m_params.maxRotationAngle);
    double angleDeg = angleDist(m_rng);
    double angleRad = angleDeg * M_PI / 180.0;

    qDebug() << "[VariationEffects] Rotação:" << angleDeg << "graus";

    // Centro de rotação
    double cx = width / 2.0;
    double cy = height / 2.0;

    double cosA = std::cos(angleRad);
    double sinA = std::sin(angleRad);

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // Coordenadas relativas ao centro
            double x = i - cx;
            double y = j - cy;

            // Rotação inversa (de destino para origem)
            double srcX = cx + x * cosA + y * sinA;
            double srcY = cy - x * sinA + y * cosA;

            int idx = j * width + i;
            result[idx] = bilinearSample(image, width, height,
                                         static_cast<float>(srcX),
                                         static_cast<float>(srcY));
        }
    }

    return result;
}

std::vector<float> VariationEffects::applyTranslation(const std::vector<float>& image,
                                                       int width, int height) const {
    std::vector<float> result(width * height, 1.0f);

    // Gerar translação aleatória
    std::uniform_real_distribution<double> txDist(-m_params.maxTranslationX, m_params.maxTranslationX);
    std::uniform_real_distribution<double> tyDist(-m_params.maxTranslationY, m_params.maxTranslationY);

    double tx = txDist(m_rng);
    double ty = tyDist(m_rng);

    qDebug() << "[VariationEffects] Translação: (" << tx << "," << ty << ") pixels";

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // Coordenadas de origem (inverso da translação)
            float srcX = static_cast<float>(i - tx);
            float srcY = static_cast<float>(j - ty);

            int idx = j * width + i;
            result[idx] = bilinearSample(image, width, height, srcX, srcY);
        }
    }

    return result;
}

std::vector<float> VariationEffects::applySkinCondition(const std::vector<float>& image,
                                                         int width, int height) const {
    std::vector<float> result = image;

    // Gerar fator aleatório baseado no parâmetro
    std::uniform_real_distribution<double> factorDist(-std::abs(m_params.skinConditionFactor),
                                                       std::abs(m_params.skinConditionFactor));
    double factor = factorDist(m_rng);

    qDebug() << "[VariationEffects] Condição da pele: fator =" << factor 
             << (factor > 0 ? "(úmida - dilate)" : "(seca - erode)");

    // Tamanho do kernel para erosão/dilatação
    int kernelSize = 3;
    int halfKernel = kernelSize / 2;

    if (factor > 0) {
        // Dilatação (pele úmida - cristas mais grossas)
        for (int j = halfKernel; j < height - halfKernel; ++j) {
            for (int i = halfKernel; i < width - halfKernel; ++i) {
                float minVal = 1.0f;
                for (int kj = -halfKernel; kj <= halfKernel; ++kj) {
                    for (int ki = -halfKernel; ki <= halfKernel; ++ki) {
                        int nidx = (j + kj) * width + (i + ki);
                        minVal = std::min(minVal, image[nidx]);
                    }
                }
                int idx = j * width + i;
                // Interpolar entre original e dilatado
                result[idx] = image[idx] * (1.0f - static_cast<float>(factor)) + 
                              minVal * static_cast<float>(factor);
            }
        }
    } else if (factor < 0) {
        // Erosão (pele seca - cristas mais finas)
        double absFactor = std::abs(factor);
        for (int j = halfKernel; j < height - halfKernel; ++j) {
            for (int i = halfKernel; i < width - halfKernel; ++i) {
                float maxVal = 0.0f;
                for (int kj = -halfKernel; kj <= halfKernel; ++kj) {
                    for (int ki = -halfKernel; ki <= halfKernel; ++ki) {
                        int nidx = (j + kj) * width + (i + ki);
                        maxVal = std::max(maxVal, image[nidx]);
                    }
                }
                int idx = j * width + i;
                // Interpolar entre original e erodido
                result[idx] = image[idx] * (1.0f - static_cast<float>(absFactor)) + 
                              maxVal * static_cast<float>(absFactor);
            }
        }
    }

    return result;
}

std::vector<float> VariationEffects::qimageToVector(const QImage& image) {
    QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
    int width = grayscale.width();
    int height = grayscale.height();

    std::vector<float> result(width * height);

    for (int j = 0; j < height; ++j) {
        const uchar* line = grayscale.constScanLine(j);
        for (int i = 0; i < width; ++i) {
            result[j * width + i] = line[i] / 255.0f;
        }
    }

    return result;
}

QImage VariationEffects::vectorToQImage(const std::vector<float>& data, int width, int height) {
    QImage result(width, height, QImage::Format_Grayscale8);

    for (int j = 0; j < height; ++j) {
        uchar* line = result.scanLine(j);
        for (int i = 0; i < width; ++i) {
            float val = std::clamp(data[j * width + i], 0.0f, 1.0f);
            line[i] = static_cast<uchar>(val * 255.0f);
        }
    }

    return result;
}

float VariationEffects::bilinearSample(const std::vector<float>& image, int width, int height,
                                        float x, float y, float defaultValue) {
    // Verificar limites
    if (x < 0 || x >= width - 1 || y < 0 || y >= height - 1) {
        return defaultValue;
    }

    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = x - x0;
    float fy = y - y0;

    // Obter valores dos 4 pixels vizinhos
    float v00 = image[y0 * width + x0];
    float v10 = image[y0 * width + x1];
    float v01 = image[y1 * width + x0];
    float v11 = image[y1 * width + x1];

    // Interpolação bilinear
    float v0 = v00 * (1.0f - fx) + v10 * fx;
    float v1 = v01 * (1.0f - fx) + v11 * fx;

    return v0 * (1.0f - fy) + v1 * fy;
}

} // namespace SFinGe
