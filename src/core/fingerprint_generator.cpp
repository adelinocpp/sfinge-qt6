#include "fingerprint_generator.h"
#include "rendering/texture_renderer.h"
#include "variation/variation_effects.h"
#include <QDebug>
#include <random>

namespace SFinGe {

FingerprintGenerator::FingerprintGenerator(QObject* parent)
    : QObject(parent)
    , m_currentSeed(0)
{
    // Inicializar seed com valor aleatório
    std::random_device rd;
    m_currentSeed = rd();
}

FingerprintGenerator::~FingerprintGenerator() {
}

void FingerprintGenerator::setParameters(const FingerprintParameters& params) {
    m_params = params;
}

void FingerprintGenerator::setSingularPoints(const SingularPoints& points) {
    m_points = points;
}

QImage FingerprintGenerator::generateShape() {
    emit progressChanged(10, "Generating shape...");
    
    // SEMPRE regenerar para refletir mudanças de parâmetros
    m_shapeGenerator.setParameters(m_params.shape);
    m_shapeImage = m_shapeGenerator.generate();
    
    if (!m_shapeImage.isNull()) {
        m_shapeImage.setDotsPerMeterX(500 * 39.3701);
        m_shapeImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    return m_shapeImage;
}

QImage FingerprintGenerator::generateDensity() {
    // SEMPRE regenerar shape primeiro (F6 deve refletir mudanças)
    generateShape();
    
    emit progressChanged(30, "Generating density map...");
    
    // SEMPRE regenerar density com novos parâmetros
    m_densityGenerator.setParameters(m_params.density);
    m_densityGenerator.setShapeMap(m_shapeGenerator.getShapeMap(), 
                                   m_shapeGenerator.getWidth(), 
                                   m_shapeGenerator.getHeight());
    m_densityImage = m_densityGenerator.generate();
    
    if (!m_densityImage.isNull()) {
        m_densityImage.setDotsPerMeterX(500 * 39.3701);
        m_densityImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    return m_densityImage;
}

QImage FingerprintGenerator::generateOrientation() {
    // SEMPRE regenerar shape primeiro (F7 deve refletir mudanças)
    generateShape();
    
    emit progressChanged(50, "Generating orientation field...");
    
    m_orientationGenerator.setParameters(m_params.orientation);
    m_orientationGenerator.setFingerprintClass(m_params.classification.fingerprintClass);
    m_orientationGenerator.setSingularPoints(m_points);
    m_orientationGenerator.setShapeMap(m_shapeGenerator.getShapeMap(),
                                      m_shapeGenerator.getWidth(),
                                      m_shapeGenerator.getHeight());
    m_orientationImage = m_orientationGenerator.generate();
    
    if (!m_orientationImage.isNull()) {
        m_orientationImage.setDotsPerMeterX(500 * 39.3701);
        m_orientationImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    return m_orientationImage;
}

QImage FingerprintGenerator::generateOrientationVisualization() {
    if (m_shapeImage.isNull()) {
        generateShape();
    }
    
    m_orientationGenerator.setParameters(m_params.orientation);
    m_orientationGenerator.setFingerprintClass(m_params.classification.fingerprintClass);
    m_orientationGenerator.setShapeMap(m_shapeGenerator.getShapeMap(),
                                      m_shapeGenerator.getWidth(),
                                      m_shapeGenerator.getHeight());
    m_orientationGenerator.setSingularPoints(m_points);
    
    return m_orientationGenerator.generateVisualization();
}

QImage FingerprintGenerator::generateFingerprint() {
    emit progressChanged(0, "Starting fingerprint generation...");
    
    // SEMPRE regenerar tudo para garantir novos pontos singulares
    generateShape();
    generateDensity();
    generateOrientation();
    
    emit progressChanged(70, "Generating ridge pattern...");
    
    m_ridgeGenerator.setParameters(m_params.ridge, m_params.density, m_params.rendering, m_params.variation);
    m_ridgeGenerator.setMinutiaeParameters(m_params.minutiae);
    m_ridgeGenerator.setOrientationMap(m_orientationGenerator.getOrientationMap(),
                                      m_shapeGenerator.getWidth(),
                                      m_shapeGenerator.getHeight());
    m_ridgeGenerator.setDensityMap(m_densityGenerator.getDensityMap());
    m_ridgeGenerator.setShapeMap(m_shapeGenerator.getShapeMap());
    
    // Set core position from singular points
    const auto& cores = m_points.getCores();
    if (!cores.empty()) {
        m_ridgeGenerator.setCorePosition(cores[0].x, cores[0].y);
    } else {
        m_ridgeGenerator.setCorePosition(m_shapeGenerator.getWidth() / 2.0, 
                                         m_shapeGenerator.getHeight() * 0.4);
    }
    
    m_fingerprintImage = m_ridgeGenerator.generate();
    
    if (!m_fingerprintImage.isNull()) {
        m_fingerprintImage.setDotsPerMeterX(500 * 39.3701);
        m_fingerprintImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    emit progressChanged(100, "Fingerprint generation complete!");
    emit generationComplete();
    
    return m_fingerprintImage;
}

QImage FingerprintGenerator::generateMasterprint() {
    emit progressChanged(0, "Starting masterprint generation...");
    
    // Gerar etapas básicas
    generateShape();
    generateDensity();
    generateOrientation();
    
    emit progressChanged(70, "Generating ridge pattern...");
    
    m_ridgeGenerator.setParameters(m_params.ridge, m_params.density, m_params.rendering, m_params.variation);
    m_ridgeGenerator.setOrientationMap(m_orientationGenerator.getOrientationMap(),
                                      m_shapeGenerator.getWidth(),
                                      m_shapeGenerator.getHeight());
    m_ridgeGenerator.setDensityMap(m_densityGenerator.getDensityMap());
    m_ridgeGenerator.setShapeMap(m_shapeGenerator.getShapeMap());
    
    // Obter mapa de cristas binário
    QImage ridgeImage = m_ridgeGenerator.generate();
    
    emit progressChanged(85, "Applying advanced rendering...");
    
    // Módulo 2: Aplicar renderização avançada com TextureRenderer
    int width = m_shapeGenerator.getWidth();
    int height = m_shapeGenerator.getHeight();
    
    // Converter ridgeImage para vetor de floats
    std::vector<float> ridgeMap(width * height);
    QImage grayscale = ridgeImage.convertToFormat(QImage::Format_Grayscale8);
    for (int j = 0; j < height; ++j) {
        const uchar* line = grayscale.constScanLine(j);
        for (int i = 0; i < width; ++i) {
            ridgeMap[j * width + i] = line[i] / 255.0f;
        }
    }
    
    // Criar TextureRenderer e aplicar
    TextureRenderer renderer(m_params.rendering, width, height, m_currentSeed);
    auto renderedData = renderer.render(ridgeMap, m_shapeGenerator.getShapeMap());
    
    // Converter de volta para QImage
    m_masterprintImage = QImage(width, height, QImage::Format_Grayscale8);
    for (int j = 0; j < height; ++j) {
        uchar* line = m_masterprintImage.scanLine(j);
        for (int i = 0; i < width; ++i) {
            float val = std::clamp(renderedData[j * width + i], 0.0f, 1.0f);
            line[i] = static_cast<uchar>(val * 255.0f);
        }
    }
    
    if (!m_masterprintImage.isNull()) {
        m_masterprintImage.setDotsPerMeterX(500 * 39.3701);
        m_masterprintImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    emit progressChanged(100, "Masterprint generation complete!");
    emit generationComplete();
    
    qDebug() << "[FingerprintGenerator] Masterprint gerada com sucesso";
    return m_masterprintImage;
}

QImage FingerprintGenerator::generateVariation(const QImage& masterImage, unsigned int seed) {
    if (masterImage.isNull()) {
        qWarning() << "[FingerprintGenerator] Imagem mestre é nula. Não é possível gerar variação.";
        emit generationError("Master image is null. Cannot generate variation.");
        return QImage();
    }
    
    emit progressChanged(10, "Applying variations...");
    
    // Módulo 3: Aplicar efeitos de variação
    VariationEffects variationEffects(m_params.variation, seed);
    QImage variationImage = variationEffects.apply(masterImage);
    
    if (!variationImage.isNull()) {
        variationImage.setDotsPerMeterX(500 * 39.3701);
        variationImage.setDotsPerMeterY(500 * 39.3701);
    }
    
    emit progressChanged(100, "Variation generation complete!");
    
    qDebug() << "[FingerprintGenerator] Variação gerada com seed:" << seed;
    return variationImage;
}

}
