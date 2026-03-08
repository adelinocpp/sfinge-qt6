#include "batch_generator.h"
#include <thread>
#include <queue>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

// Helper: computa alpha [0,1] do modelo 4 seções para o ponto (fx, fy)
// no sistema de coordenadas da impressão base (origem = canto superior esquerdo).
// Mesmo modelo que generateShapeMap() — semi-elipses top/bottom + retângulo central.
static float computeShapeAlpha(float fx, float fy,
    int left, int right, int top, int middle, int bottom) {
    const float cx     = static_cast<float>(left);
    const float cy_top = static_cast<float>(top);
    const float cy_bot = static_cast<float>(top + middle);
    const float ax_l   = static_cast<float>(left);
    const float ax_r   = static_cast<float>(right);
    const float ay_top = static_cast<float>(top);
    const float ay_bot = static_cast<float>(bottom);
    const float feather = 0.08f;

    if (fy < cy_top) {
        float dx = fx - cx, dy = fy - cy_top;
        float ax = (dx <= 0.f) ? ax_l : ax_r;
        float v = std::sqrt((dx/ax)*(dx/ax) + (dy/ay_top)*(dy/ay_top));
        if (v >= 1.0f)            return 0.0f;
        if (v >= 1.0f - feather)  return (1.0f - v) / feather;
        return 1.0f;
    } else if (fy > cy_bot) {
        float dx = fx - cx, dy = fy - cy_bot;
        float ax = (dx <= 0.f) ? ax_l : ax_r;
        float v = std::sqrt((dx/ax)*(dx/ax) + (dy/ay_bot)*(dy/ay_bot));
        if (v >= 1.0f)            return 0.0f;
        if (v >= 1.0f - feather)  return (1.0f - v) / feather;
        return 1.0f;
    } else {
        float feather_px = feather * std::min(ax_l, ax_r);
        float distEdge = std::min(fx, static_cast<float>(left + right - 1) - fx);
        return std::min(1.0f, distEdge / feather_px);
    }
}

BatchGenerator::BatchGenerator() : m_rng(std::random_device{}()) {}

void BatchGenerator::setBatchConfig(const BatchConfig& config) {
    m_config = config;
}

FingerprintClass BatchGenerator::selectClassByPopulation() {
    // Population distribution (approximate):
    // Loops: 60-65%, Whorls: 30-35%, Arches: 5%
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(m_rng);

    if (r < 0.025) return FingerprintClass::Arch;
    if (r < 0.05) return FingerprintClass::TentedArch;
    if (r < 0.35) return FingerprintClass::LeftLoop;
    if (r < 0.65) return FingerprintClass::RightLoop;
    if (r < 0.80) return FingerprintClass::Whorl;
    if (r < 0.90) return FingerprintClass::TwinLoop;
    if (r < 0.95) return FingerprintClass::CentralPocket;
    return FingerprintClass::Accidental;
}

FingerprintInstance BatchGenerator::createBaseFingerprint(int index) {
    FingerprintInstance instance;

    char buf[32];
    snprintf(buf, sizeof(buf), "FP_%03d", index + 1);
    instance.identifier = buf;

    instance.baseParams.reset();

    // Aplicar overrides de config (density, orientation, ridge, variation)
    instance.baseParams.density = m_config.density;
    instance.baseParams.orientation = m_config.orientation;
    instance.baseParams.orientation.quietMode = m_config.quietMode;
    instance.baseParams.ridge = m_config.ridge;
    instance.baseParams.variation = m_config.variation;

    int width, height;

    if (m_config.useFixedShape) {
        // Usar shape fixo especificado pelo utilizador
        instance.baseParams.shape = m_config.shape;
        width  = m_config.shape.left + m_config.shape.right;
        height = m_config.shape.top  + m_config.shape.middle + m_config.shape.bottom;
    } else {
        // Shape GRANDE para crop posterior — mesmo esquema que Qt6 batch → output 500×600
        // Base ~1000×1200px; cada versão é crop 500×600 com offset polar ±100px
        // Shape base 2× GUI (left=250,right=250,top=333,mid=100,bot=167) → base 1000×1200
        // scaled: left*500/1000=250 ✓  top*600/1200=333 ✓  mid*600/1200=100 ✓  bot*600/1200=167 ✓
        std::uniform_int_distribution<int> shapeDist(-20, 20);
        instance.baseParams.shape.left   = 500 + shapeDist(m_rng);  // scaled→250
        instance.baseParams.shape.right  = 500 + shapeDist(m_rng);  // scaled→250
        instance.baseParams.shape.top    = 666 + shapeDist(m_rng);  // scaled→333 (ponta apontada)
        instance.baseParams.shape.middle = 200 + shapeDist(m_rng);  // scaled→100
        instance.baseParams.shape.bottom = 334 + shapeDist(m_rng);  // scaled→167 (base arredondada)
        width  = instance.baseParams.shape.left + instance.baseParams.shape.right;
        height = instance.baseParams.shape.top  + instance.baseParams.shape.middle +
                 instance.baseParams.shape.bottom;
    }

    FingerprintClass selectedClass;
    if (m_config.useFixedClass) {
        selectedClass = m_config.classification.fingerprintClass;
    } else {
        selectedClass = selectClassByPopulation();
    }

    instance.basePoints.generateRandomPoints(selectedClass, width, height);
    instance.baseParams.classification.fingerprintClass = selectedClass;

    // Repassar parâmetros de minutiae e rendering da configuração do batch
    instance.baseParams.minutiae = m_config.minutiae;
    instance.baseParams.rendering = m_config.rendering;
    // Moisture é per-impression (varia por versão) — não faz parte do masterprint
    instance.baseParams.rendering.enableMoisture = false;

    return instance;
}

VersionTransform BatchGenerator::generateVersionTransform(int versionIndex) {
    return generateVersionTransformLocal(versionIndex, m_rng);
}

VersionTransform BatchGenerator::generateVersionTransformLocal(int versionIndex, std::mt19937& rng) {
    VersionTransform transform;

    std::uniform_real_distribution<double> dist01(0.0, 1.0);

    // Rotation (-15 to +15 degrees)
    transform.rotation = (dist01(rng) - 0.5) * 30.0;

    // Noise (0.03 to 0.08)
    transform.noiseLevel = 0.03 + dist01(rng) * 0.05;

    // Lens distortion (-0.16 to +0.16)
    transform.usePincushion = true;
    double magnitude = 0.08 + dist01(rng) * 0.08;
    transform.lensDistortion = (dist01(rng) < 0.5) ? -magnitude : magnitude;

    // Homography shift (-20 to +20 pixels)
    transform.homographyShiftX = (dist01(rng) - 0.5) * 40.0;
    transform.homographyShiftY = (dist01(rng) - 0.5) * 40.0;

    // Homography angle (-10 to +10 degrees)
    transform.homographyAngle = (dist01(rng) - 0.5) * 20.0;

    // Crop: 500×600 (a partir de base ~1000×1200)
    transform.cropWidth  = 500;
    transform.cropHeight = 600;

    // Blur
    transform.applyBlur = true;
    std::uniform_int_distribution<int> blurDist(25, 150);
    transform.blurRadius = blurDist(rng);
    transform.blurCenterX = 50.0 + dist01(rng) * 400.0;
    transform.blurCenterY = 50.0 + dist01(rng) * 500.0;

    return transform;
}

Image BatchGenerator::applyNoise(const Image& image, double noiseLevel) {
    Image result = image.copy();
    std::uniform_real_distribution<double> dist(-0.5, 0.5);

    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            uint8_t gray = result.pixel(x, y);
            double noise = dist(m_rng) * 255.0 * noiseLevel;
            int newGray = std::clamp(static_cast<int>(gray + noise), 0, 255);
            result.setPixel(x, y, static_cast<uint8_t>(newGray));
        }
    }

    return result;
}

Image BatchGenerator::applyBlur(const Image& image, int radius, double centerX, double centerY) {
    Image result = image.copy();

    const double kernel[3][3] = {
        {1.0/16, 2.0/16, 1.0/16},
        {2.0/16, 4.0/16, 2.0/16},
        {1.0/16, 2.0/16, 1.0/16}
    };

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            double dx = x - centerX;
            double dy = y - centerY;
            double dist = std::sqrt(dx*dx + dy*dy);

            if (dist <= radius) {
                double blurIntensity = 1.0 - (dist / radius);

                double sum = 0;
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int px = std::clamp(x + kx, 0, image.width() - 1);
                        int py = std::clamp(y + ky, 0, image.height() - 1);
                        sum += image.pixel(px, py) * kernel[ky + 1][kx + 1];
                    }
                }

                uint8_t original = image.pixel(x, y);
                int blurred = static_cast<int>(original * (1-blurIntensity) + sum * blurIntensity);
                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(blurred, 0, 255)));
            }
        }
    }

    return result;
}

Image BatchGenerator::applyLensDistortion(const Image& image, double k) {
    Image result(image.width(), image.height());
    result.fill(255);

    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double nx = (x - cx) / cx;
            double ny = (y - cy) / cy;
            double r = std::sqrt(nx * nx + ny * ny);

            if (r < 0.001) {
                result.setPixel(x, y, image.pixel(x, y));
                continue;
            }

            double rDistorted = r * (1.0 + k * r * r);

            double srcX = cx + (nx / r) * rDistorted * cx;
            double srcY = cy + (ny / r) * rDistorted * cy;

            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;

                double p00 = image.pixel(x0, y0);
                double p10 = image.pixel(x0 + 1, y0);
                double p01 = image.pixel(x0, y0 + 1);
                double p11 = image.pixel(x0 + 1, y0 + 1);

                double gray = p00 * (1 - fx) * (1 - fy) +
                             p10 * fx * (1 - fy) +
                             p01 * (1 - fx) * fy +
                             p11 * fx * fy;

                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(gray, 0.0, 255.0)));
            }
        }
    }

    return result;
}

Image BatchGenerator::applyHomography(const Image& image, double shiftX, double shiftY, double angle) {
    Image result(image.width(), image.height());
    result.fill(255);

    double rad = angle * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double nx = x - cx;
            double ny = y - cy;

            double srcX = nx * cosA - ny * sinA * 0.3 + shiftX + cx;
            double srcY = nx * sinA * 0.3 + ny * cosA + shiftY + cy;

            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;

                double p00 = image.pixel(x0, y0);
                double p10 = image.pixel(x0 + 1, y0);
                double p01 = image.pixel(x0, y0 + 1);
                double p11 = image.pixel(x0 + 1, y0 + 1);

                double gray = p00 * (1 - fx) * (1 - fy) +
                             p10 * fx * (1 - fy) +
                             p01 * (1 - fx) * fy +
                             p11 * fx * fy;

                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(gray, 0.0, 255.0)));
            }
        }
    }

    return result;
}

Image BatchGenerator::applyRotation(const Image& image, double angle) {
    Image result(image.width(), image.height());
    result.fill(255);

    double rad = angle * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double nx = x - cx;
            double ny = y - cy;

            double srcX = nx * cosA + ny * sinA + cx;
            double srcY = -nx * sinA + ny * cosA + cy;

            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;

                double p00 = image.pixel(x0, y0);
                double p10 = image.pixel(x0 + 1, y0);
                double p01 = image.pixel(x0, y0 + 1);
                double p11 = image.pixel(x0 + 1, y0 + 1);

                double gray = p00 * (1 - fx) * (1 - fy) +
                             p10 * fx * (1 - fy) +
                             p01 * (1 - fx) * fy +
                             p11 * fx * fy;

                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(gray, 0.0, 255.0)));
            }
        }
    }

    return result;
}

Image BatchGenerator::applyCrop(const Image& image, int targetWidth, int targetHeight,
                                 std::mt19937& rng) {
    if (image.width() < targetWidth || image.height() < targetHeight) {
        // Imagem menor que o alvo: centrar com fundo branco
        Image result(targetWidth, targetHeight);
        result.fill(255);
        int ox = (targetWidth  - image.width())  / 2;
        int oy = (targetHeight - image.height()) / 2;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
                result.setPixel(ox + x, oy + y, image.pixel(x, y));
        return result;
    }

    // Randomização polar: centro do crop varia ±100px em torno do centro da imagem
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    double radius = dist01(rng) * 100.0;
    double angle  = dist01(rng) * 2.0 * M_PI;
    int cx = image.width()  / 2 + static_cast<int>(radius * std::cos(angle));
    int cy = image.height() / 2 + static_cast<int>(radius * std::sin(angle));

    // Garantir margens de 30px das bordas
    int cropX = std::max(30, std::min(image.width()  - targetWidth  - 30, cx - targetWidth  / 2));
    int cropY = std::max(30, std::min(image.height() - targetHeight - 30, cy - targetHeight / 2));

    Image result(targetWidth, targetHeight);
    result.fill(255);
    for (int y = 0; y < targetHeight; ++y)
        for (int x = 0; x < targetWidth; ++x)
            result.setPixel(x, y, image.pixel(cropX + x, cropY + y));
    return result;
}

Image BatchGenerator::applyShapeMask(const Image& image, const ShapeParameters& shape,
                                      int originX, int originY) const {
    // Aplica o shape mask 4 seções ao output: pixels fora do shape → branco
    // originX/Y: deslocamento do canto sup-esq do crop no espaço da impressão base
    Image result = image.copy();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            float alpha = computeShapeAlpha(
                static_cast<float>(x + originX),
                static_cast<float>(y + originY),
                shape.left, shape.right, shape.top, shape.middle, shape.bottom);
            uint8_t p = image.pixel(x, y);
            result.setPixel(x, y,
                static_cast<uint8_t>(p * alpha + 255.f * (1.f - alpha)));
        }
    }
    return result;
}

Image BatchGenerator::applyMoisture(const Image& image, std::mt19937& rng) const {
    // Manchas brancas de suor — variam por impressão (Cappelli Step 8b)
    Image result = image.copy();
    std::uniform_int_distribution<int> posX(30, image.width()  - 30);
    std::uniform_int_distribution<int> posY(30, image.height() - 30);
    std::uniform_real_distribution<double> sizeR(15.0, 40.0);
    std::uniform_int_distribution<int> countR(2, 7);
    int numBlobs = countR(rng);
    for (int b = 0; b < numBlobs; ++b) {
        int cx = posX(rng), cy = posY(rng);
        double radius = sizeR(rng);
        int r = static_cast<int>(radius) + 1;
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                double d = std::sqrt(static_cast<double>(dx*dx + dy*dy));
                if (d > radius) continue;
                int px = cx + dx, py = cy + dy;
                if (px < 0 || px >= image.width() || py < 0 || py >= image.height()) continue;
                double alpha = (1.0 - d / radius) * 0.18;
                uint8_t orig = image.pixel(px, py);
                result.setPixel(px, py,
                    static_cast<uint8_t>(std::min(255, static_cast<int>(orig + alpha * (255 - orig)))));
            }
        }
    }
    return result;
}

Image BatchGenerator::applyVersionTransforms(const Image& baseImage,
    const VersionTransform& transform, const ShapeParameters& shape,
    int baseW, int baseH, std::mt19937& rng) {
    Image result = baseImage.copy();

    if (transform.noiseLevel > 0.001) {
        result = applyNoise(result, transform.noiseLevel);
    }

    if (transform.applyBlur && transform.blurRadius > 0) {
        result = applyBlur(result, transform.blurRadius, transform.blurCenterX, transform.blurCenterY);
    }

    if (std::abs(transform.lensDistortion) > 0.001) {
        result = applyLensDistortion(result, transform.lensDistortion);
    }

    if (std::abs(transform.homographyAngle) > 0.1 ||
        std::abs(transform.homographyShiftX) > 0.1 ||
        std::abs(transform.homographyShiftY) > 0.1) {
        result = applyHomography(result, transform.homographyShiftX,
                                 transform.homographyShiftY, transform.homographyAngle);
    }

    if (std::abs(transform.rotation) > 0.1) {
        result = applyRotation(result, transform.rotation);
    }

    // Crop com randomização polar (simula diferentes áreas de contacto)
    result = applyCrop(result, transform.cropWidth, transform.cropHeight, rng);
    result.setDPI(500);

    // Shape mask 4 seções (Cappelli spec) — aplicar no espaço do output com shape escalado
    // O crop está sempre no centro do base (originX/Y = 250/300) → dentro do shape → alpha=1 em tudo.
    // Solução: escalar shape proporcionalmente para o output 500×600 e aplicar em (0,0).
    {
        ShapeParameters scaledShape = shape;
        int baseShapeW = shape.left + shape.right;
        int baseShapeH = shape.top + shape.middle + shape.bottom;
        scaledShape.left   = shape.left   * transform.cropWidth  / baseShapeW;
        scaledShape.right  = shape.right  * transform.cropWidth  / baseShapeW;
        scaledShape.top    = shape.top    * transform.cropHeight / baseShapeH;
        scaledShape.middle = shape.middle * transform.cropHeight / baseShapeH;
        scaledShape.bottom = shape.bottom * transform.cropHeight / baseShapeH;
        result = applyShapeMask(result, scaledShape, 0, 0);
    }

    // Moisture por versão: manchas brancas (suor) — posição e intensidade variáveis por impressão
    std::uniform_int_distribution<int> coin(0, 1);
    if (coin(rng))
        result = applyMoisture(result, rng);

    return result;
}

bool BatchGenerator::saveFingerprint(const Image& image, const FingerprintInstance& instance,
                                     int fpIndex, int versionIndex) {
    char filename[512];
    int actualIndex = m_config.startIndex + fpIndex;
    snprintf(filename, sizeof(filename), "%s/%s_%04d_v%02d.png",
             m_config.outputDirectory.c_str(),
             m_config.filenamePrefix.c_str(),
             actualIndex, versionIndex);

    return image.save(filename);
}

bool BatchGenerator::generateBatch() {
    m_cancelled = false;
    m_generated = 0;

    // Seed global para reprodutibilidade (afeta shape/class aleatórios)
    if (m_config.globalSeed != 0) {
        m_rng.seed(m_config.globalSeed);
    }

    // Create output directory
    std::filesystem::create_directories(m_config.outputDirectory);

    int numWorkers = m_numWorkers > 0 ? m_numWorkers : std::thread::hardware_concurrency();
    if (numWorkers < 1) numWorkers = 1;

    if (!m_config.quietMode) {
        std::cout << "Starting parallel batch generation with " << numWorkers << " workers\n";
        std::cout << "Total fingerprints: " << m_config.numFingerprints << "\n";
    }

    // Pre-create all fingerprint instances
    std::vector<FingerprintInstance> instances(m_config.numFingerprints);
    for (int i = 0; i < m_config.numFingerprints && !m_cancelled; ++i) {
        instances[i] = createBaseFingerprint(i);
    }

    // Task queue
    std::queue<int> taskQueue;
    for (int i = 0; i < m_config.numFingerprints; ++i) {
        taskQueue.push(i);
    }

    std::mutex queueMutex;
    std::atomic<int> completedFps(0);

    // Worker function — cada thread tem seu próprio RNG para evitar race conditions
    auto workerFunc = [&]() {
        std::random_device rd;
        std::mt19937 localRng(m_config.globalSeed != 0
            ? static_cast<std::mt19937::result_type>(m_config.globalSeed + 1000u)
            : rd());

        while (!m_cancelled) {
            int taskIndex;
            bool hasTask = false;

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!taskQueue.empty()) {
                    taskIndex = taskQueue.front();
                    taskQueue.pop();
                    hasTask = true;
                }
            }

            if (!hasTask) break;

            // CRIAR NOVO GENERATOR PARA CADA TAREFA para garantir isolamento total
            FingerprintGenerator localGenerator;
            if (m_config.globalSeed != 0) {
                localGenerator.setSeed(m_config.globalSeed + static_cast<unsigned int>(taskIndex) * 100u);
            }

            // Configure generator
            localGenerator.setParameters(instances[taskIndex].baseParams);
            localGenerator.setSingularPoints(instances[taskIndex].basePoints);

            // Generate base image (~1000×1200px)
            Image baseFingerprint = localGenerator.generateFingerprint();
            int baseW = baseFingerprint.width();
            int baseH = baseFingerprint.height();

            // Generate all versions
            int startIdx = m_config.skipOriginal ? 1 : 0;
            for (int verIdx = startIdx; verIdx <= m_config.versionsPerFingerprint && !m_cancelled; ++verIdx) {
                Image transformedFingerprint;

                if (verIdx == 0) {
                    // Versão original: sem distorções — shape já implícita do TextureRenderer
                    transformedFingerprint = baseFingerprint.copy();
                } else {
                    VersionTransform transform = generateVersionTransformLocal(verIdx, localRng);
                    transformedFingerprint = applyVersionTransforms(baseFingerprint, transform,
                        instances[taskIndex].baseParams.shape, baseW, baseH, localRng);
                }

                if (!saveFingerprint(transformedFingerprint, instances[taskIndex], taskIndex, verIdx)) {
                    continue;
                }

                m_generated.fetch_add(1);
            }

            int fpCompleted = completedFps.fetch_add(1) + 1;

            if (m_progressCallback) {
                m_progressCallback(fpCompleted, m_config.numFingerprints, m_generated.load());
            }
        }
    };

    // Create and start threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numWorkers; ++i) {
        threads.emplace_back(workerFunc);
    }

    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }

    return !m_cancelled;
}

} // namespace SFinGe
