#include "fingerprint_worker.h"
#include <QDebug>

namespace SFinGe {

FingerprintWorker::FingerprintWorker(QObject* parent)
    : QThread(parent)
    , m_generator(nullptr)
    , m_cancelled(false)
    , m_applyEllipticalMask(false) {
}

FingerprintWorker::~FingerprintWorker() {
    cancelGeneration();
    wait();
    if (m_generator) {
        delete m_generator;
    }
}

void FingerprintWorker::setParameters(const FingerprintParameters& params) {
    m_params = params;
}

void FingerprintWorker::setSingularPoints(const SingularPoints& points) {
    m_points = points;
    qDebug() << "[FingerprintWorker] Received" << points.getCoreCount() << "cores and" << points.getDeltaCount() << "deltas";
}

void FingerprintWorker::setApplyEllipticalMask(bool apply) {
    m_applyEllipticalMask = apply;
}

void FingerprintWorker::cancelGeneration() {
    m_cancelled = true;
}

void FingerprintWorker::run() {
    qDebug() << "[FingerprintWorker] run() started";
    m_cancelled = false;
    
    if (!m_generator) {
        qDebug() << "[FingerprintWorker] Creating new generator";
        m_generator = new FingerprintGenerator();
    }
    
    disconnect(m_generator, nullptr, this, nullptr);
    connect(m_generator, &FingerprintGenerator::progressChanged,
            this, &FingerprintWorker::progressChanged, Qt::QueuedConnection);
    
    qDebug() << "[FingerprintWorker] Setting parameters";
    m_generator->setParameters(m_params);
    m_generator->setSingularPoints(m_points);
    
    try {
        if (!m_cancelled) {
            qDebug() << "[FingerprintWorker] Emitting start progress";
            emit progressChanged(0, tr("Starting generation..."));
        }
        
        if (!m_cancelled) {
            qDebug() << "[FingerprintWorker] Generating fingerprint...";
            emit progressChanged(10, tr("Generating shape..."));
            QImage image = m_generator->generateFingerprint();
            
            qDebug() << "[FingerprintWorker] Generated image size:" << image.width() << "x" << image.height() << "null:" << image.isNull();
            qDebug() << "[FingerprintWorker] Image DPI:" << (image.dotsPerMeterX() / 39.3701) << "x" << (image.dotsPerMeterY() / 39.3701);
            
            // Aplicar máscara elíptica se solicitado
            if (m_applyEllipticalMask && !image.isNull()) {
                image = applyEllipticalMask(image);
            }
            
            if (!m_cancelled && !image.isNull()) {
                emit progressChanged(100, tr("Complete"));
                qDebug() << "[FingerprintWorker] Emitting fingerprintGenerated signal";
                emit fingerprintGenerated(image);
            } else if (image.isNull()) {
                qDebug() << "[FingerprintWorker] ERROR: Generated image is null";
                emit generationFailed(tr("Generated image is null"));
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "[FingerprintWorker] Exception:" << e.what();
        emit generationFailed(QString::fromStdString(e.what()));
    } catch (...) {
        qDebug() << "[FingerprintWorker] Unknown exception";
        emit generationFailed(tr("Unknown error during fingerprint generation"));
    }
    
    qDebug() << "[FingerprintWorker] run() finished";
}

QImage FingerprintWorker::applyEllipticalMask(const QImage& image) const {
    // Criar resultado com mesmo formato da imagem original
    QImage result = image.copy();
    int width = result.width();
    int height = result.height();
    
    // Centro da elipse
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    // Raios da elipse (94% do tamanho para deixar margem)
    double rx = width * 0.47;
    double ry = height * 0.47;
    
    // Largura do fade out (10% do menor eixo)
    double fadeWidth = std::min(rx, ry) * 0.10;
    
    // Aplicar máscara com fade out suave usando métodos seguros do Qt
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calcular distância normalizada do centro da elipse
            double dx_norm = (x - cx) / rx;
            double dy_norm = (y - cy) / ry;
            double dist = std::sqrt(dx_norm * dx_norm + dy_norm * dy_norm);
            
            // Calcular alpha (transparência)
            double alpha = 1.0;
            if (dist > 1.0) {
                // Fora da elipse: fade out completo para branco
                alpha = 0.0;
            } else if (dist > (1.0 - fadeWidth / rx)) {
                // Na região de fade out: transição suave
                double fadePos = (dist - (1.0 - fadeWidth / rx)) / (fadeWidth / rx);
                // Usar função suave (smoothstep)
                alpha = 1.0 - (fadePos * fadePos * (3.0 - 2.0 * fadePos));
            }
            
            // Aplicar alpha blending com branco usando métodos seguros
            QRgb pixel = result.pixel(x, y);
            int gray = qGray(pixel);
            int newGray = static_cast<int>(gray * alpha + 255 * (1.0 - alpha));
            result.setPixel(x, y, qRgb(newGray, newGray, newGray));
        }
    }
    
    return result;
}

}
