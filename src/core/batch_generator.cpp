#include "batch_generator.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTransform>
#include <QPainter>
#include <QThread>
#include <QMetaObject>
#include <QDebug>
#include <cmath>
#include <random>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

BatchGenerator::BatchGenerator(QObject* parent)
    : QObject(parent)
    , m_generator(new FingerprintGenerator(this))
    , m_cancelled(false)
    , m_firstImageTime(0) {
}

BatchGenerator::~BatchGenerator() {
}

void BatchGenerator::setBatchConfig(const BatchConfig& config) {
    m_config = config;
}

bool BatchGenerator::generateBatch() {
    m_cancelled = false;
    m_firstImageTime = 0;
    m_timer.start();
    
    // Criar diretório de saída
    QDir dir(m_config.outputDirectory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit error(tr("Failed to create output directory"));
            return false;
        }
    }
    
    // Total = numFingerprints * (N versões + original se não pular)
    int imagesPerFingerprint = m_config.versionsPerFingerprint + (m_config.skipOriginal ? 0 : 1);
    int totalImages = m_config.numFingerprints * imagesPerFingerprint;
    int generated = 0;
    
    // Gerar cada impressão base
    for (int fpIdx = 0; fpIdx < m_config.numFingerprints && !m_cancelled; ++fpIdx) {
        emit progressUpdated(generated, totalImages, 
                           tr("Creating fingerprint %1 of %2").arg(fpIdx + 1).arg(m_config.numFingerprints));
        
        // Criar impressão base
        FingerprintInstance baseInstance = createBaseFingerprint(fpIdx);
        
        // Configurar gerador com impressão base
        m_generator->setParameters(baseInstance.baseParams);
        m_generator->setSingularPoints(baseInstance.basePoints);
        
        // Gerar impressão base UMA VEZ
        QImage baseFingerprint = m_generator->generateFingerprint();
        
        // Gerar versões: v0=original (1000x1200) + N versões transformadas (500x600)
        // Se versionsPerFingerprint=3 e skipOriginal=false, gera 4 imagens: v0 + v1,v2,v3
        // Se skipOriginal=true, pula v0 e gera apenas v1,v2,v3
        int startIdx = m_config.skipOriginal ? 1 : 0;
        for (int verIdx = startIdx; verIdx <= m_config.versionsPerFingerprint && !m_cancelled; ++verIdx) {
            // Calcular tempo restante após primeira impressão
            QString statusMsg = tr("Generating fingerprint %1 version %2").arg(fpIdx + 1).arg(verIdx);
            if (m_firstImageTime > 0 && generated > 0) {
                qint64 elapsed = m_timer.elapsed();
                qint64 avgTimePerImage = elapsed / generated;
                qint64 remaining = avgTimePerImage * (totalImages - generated);
                int remainingSeconds = remaining / 1000;
                int minutes = remainingSeconds / 60;
                int seconds = remainingSeconds % 60;
                statusMsg += tr(" - Estimated time: %1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
            }
            emit progressUpdated(generated, totalImages, statusMsg);
            
            QImage transformedFingerprint;
            
            if (verIdx == 0) {
                // Versão 1 (v0): ORIGINAL COMPLETA 1000x1200 SEM recorte
                transformedFingerprint = baseFingerprint.copy();
                transformedFingerprint.setDotsPerMeterX(500 * 39.3701);
                transformedFingerprint.setDotsPerMeterY(500 * 39.3701);
            } else {
                // Versões 2+ (v1+): aplicar transformações PERCEPTÍVEIS + recorte 500x600
                VersionTransform transform = generateVersionTransform(verIdx);
                transformedFingerprint = applyVersionTransforms(baseFingerprint, transform);
            }
            
            // Aplicar máscara elíptica se habilitado
            if (m_config.applyEllipticalMask) {
                transformedFingerprint = applyEllipticalMask(transformedFingerprint);
            }
            
            // Salvar imagem
            if (!saveFingerprint(transformedFingerprint, baseInstance, fpIdx, verIdx)) {
                emit error(tr("Failed to save fingerprint %1 version %2").arg(fpIdx + 1).arg(verIdx));
                continue;
            }
            
            // Salvar parâmetros se solicitado
            if (m_config.saveParameters) {
                QString paramFile = QString("%1/%2_%3_v%4_params.json")
                    .arg(m_config.outputDirectory)
                    .arg(m_config.filenamePrefix)
                    .arg(fpIdx + 1, 3, 10, QChar('0'))
                    .arg(verIdx);  // v0, v1, v2, v3... (sem +1)
                    
                saveParameters(baseInstance.baseParams, baseInstance.basePoints, paramFile);
            }
            
            generated++;
        }
    }
    
    if (!m_cancelled) {
        emit batchCompleted(generated);
    }
    
    return !m_cancelled;
}

bool BatchGenerator::generateBatchParallel() {
    m_cancelled = false;
    m_generated.storeRelaxed(0);
    m_timer.start();
    
    // Criar diretório de saída
    QDir dir(m_config.outputDirectory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit error(tr("Failed to create output directory"));
            return false;
        }
    }
    
    int numWorkers = m_numWorkers > 0 ? m_numWorkers : QThread::idealThreadCount();
    int imagesPerFingerprint = m_config.versionsPerFingerprint + (m_config.skipOriginal ? 0 : 1);
    int totalImages = m_config.numFingerprints * imagesPerFingerprint;
    
    if (!m_config.quietMode) {
        qDebug() << "Starting parallel batch generation with" << numWorkers << "workers";
        qDebug() << "Total fingerprints:" << m_config.numFingerprints << "Total images:" << totalImages;
    }
    
    // Pré-criar todas as instâncias de impressão digital
    std::vector<FingerprintInstance> instances(m_config.numFingerprints);
    for (int i = 0; i < m_config.numFingerprints && !m_cancelled; ++i) {
        instances[i] = createBaseFingerprint(i);
    }
    
    // Criar fila de tarefas para geração das imagens base (v0)
    // Cada tarefa gera uma impressão base e todas as suas versões
    struct BaseTask {
        int fpIndex;
        FingerprintInstance instance;
    };
    
    QQueue<BaseTask> baseTasks;
    for (int i = 0; i < m_config.numFingerprints; ++i) {
        baseTasks.enqueue({i, instances[i]});
    }
    
    QMutex taskMutex;
    QMutex progressMutex;
    QAtomicInt completedFps(0);
    
    // Função de processamento para cada worker
    auto workerFunc = [&]() {
        // Cada thread precisa de seu próprio FingerprintGenerator
        FingerprintGenerator localGenerator;
        
        while (!m_cancelled) {
            BaseTask task;
            bool hasTask = false;
            
            {
                QMutexLocker locker(&taskMutex);
                if (!baseTasks.isEmpty()) {
                    task = baseTasks.dequeue();
                    hasTask = true;
                }
            }
            
            if (!hasTask) break;
            
            // Configurar gerador local
            localGenerator.setParameters(task.instance.baseParams);
            localGenerator.setSingularPoints(task.instance.basePoints);
            
            // Gerar imagem base
            QImage baseFingerprint = localGenerator.generateFingerprint();
            
            // Gerar todas as versões desta impressão
            int startIdx = m_config.skipOriginal ? 1 : 0;
            for (int verIdx = startIdx; verIdx <= m_config.versionsPerFingerprint && !m_cancelled; ++verIdx) {
                QImage transformedFingerprint;
                
                if (verIdx == 0) {
                    transformedFingerprint = baseFingerprint.copy();
                    transformedFingerprint.setDotsPerMeterX(500 * 39.3701);
                    transformedFingerprint.setDotsPerMeterY(500 * 39.3701);
                } else {
                    VersionTransform transform = generateVersionTransform(verIdx);
                    transformedFingerprint = applyVersionTransforms(baseFingerprint, transform);
                }
                
                if (m_config.applyEllipticalMask) {
                    transformedFingerprint = applyEllipticalMask(transformedFingerprint);
                }
                
                // Salvar imagem
                if (!saveFingerprint(transformedFingerprint, task.instance, task.fpIndex, verIdx)) {
                    qWarning() << "Failed to save fingerprint" << task.fpIndex + 1 << "version" << verIdx;
                    continue;
                }
                
                // Salvar parâmetros se solicitado
                if (m_config.saveParameters) {
                    int actualIndex = m_config.startIndex + task.fpIndex;
                    QString paramFile = QString("%1/%2_%3_v%4_params.json")
                        .arg(m_config.outputDirectory)
                        .arg(m_config.filenamePrefix)
                        .arg(actualIndex, 4, 10, QChar('0'))
                        .arg(verIdx);
                    saveParameters(task.instance.baseParams, task.instance.basePoints, paramFile);
                }
                
                m_generated.fetchAndAddRelaxed(1);
            }
            
            // Emitir progresso ao completar cada digital
            int fpCompleted = completedFps.fetchAndAddRelaxed(1) + 1;
            int imgCompleted = m_generated.loadRelaxed();
            {
                QMutexLocker locker(&progressMutex);
                emit progressUpdated(fpCompleted, m_config.numFingerprints, QString::number(imgCompleted));
            }
        }
    };
    
    // Criar e iniciar threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numWorkers; ++i) {
        threads.emplace_back(workerFunc);
    }
    
    // Aguardar conclusão
    for (auto& t : threads) {
        t.join();
    }
    
    int generated = m_generated.loadRelaxed();
    if (!m_cancelled) {
        emit batchCompleted(generated);
    }
    
    return !m_cancelled;
}

void BatchGenerator::cancel() {
    m_cancelled = true;
}

// Implementação da classe WorkerThread
BatchGenerator::WorkerThread::WorkerThread(BatchGenerator* generator, int workerId)
    : m_generator(generator)
    , m_workerId(workerId)
    , m_running(true)
{
}

void BatchGenerator::WorkerThread::stop() {
    m_running = false;
}

void BatchGenerator::WorkerThread::run() {
    if (!m_generator->m_config.quietMode) qDebug() << "Worker" << m_workerId << "started processing";
    int tasksProcessed = 0;
    
    while (m_running && !m_generator->m_cancelled) {
        // Esperar por tarefa disponível (timeout de 1 segundo para permitir cancelamento)
        if (!m_generator->m_queueSemaphore.tryAcquire(1, 1000)) {
            continue;  // Timeout, verificar se deve continuar
        }
        
        // Obter tarefa da fila
        FingerprintTask task;
        bool hasTask = false;
        {
            QMutexLocker locker(&m_generator->m_queueMutex);
            if (!m_generator->m_taskQueue.isEmpty()) {
                task = m_generator->m_taskQueue.dequeue();
                hasTask = true;
            }
        }
        
        if (!hasTask) {
            continue;  // Fila vazia, continuar esperando
        }
        
        // Processar tarefa
        if (m_generator->m_cancelled) {
            break;
        }
        
        QImage transformedFingerprint;
        
        if (task.versionIndex == 0) {
            // Versão 1 (v0): ORIGINAL COMPLETA 1000x1200 SEM recorte
            transformedFingerprint = task.baseFingerprint.copy();
            transformedFingerprint.setDotsPerMeterX(500 * 39.3701);
            transformedFingerprint.setDotsPerMeterY(500 * 39.3701);
        } else {
            // Versões 2+ (v1+): aplicar transformações PERCEPTÍVEIS + recorte 500x600
            VersionTransform transform = m_generator->generateVersionTransform(task.versionIndex);
            transformedFingerprint = m_generator->applyVersionTransforms(task.baseFingerprint, transform);
        }
        
        // Aplicar máscara elíptica se habilitado
        if (m_generator->m_config.applyEllipticalMask) {
            transformedFingerprint = m_generator->applyEllipticalMask(transformedFingerprint);
        }
        
        // Salvar imagem
        if (!m_generator->m_cancelled) {
            if (!m_generator->saveFingerprint(transformedFingerprint, task.instance, task.fpIndex, task.versionIndex)) {
                // Erro ao salvar - emitir sinal na thread principal
                QMetaObject::invokeMethod(m_generator, [task]() {
                    // Note: Esta é uma simplificação - idealmente deveríamos passar o generator
                    // mas para evitar complexidade com QMetaObject, apenas logamos o erro
                    qWarning() << "Failed to save fingerprint" << task.fpIndex + 1 << "version" << task.versionIndex;
                }, Qt::QueuedConnection);
            }
            
            // Salvar parâmetros se solicitado
            if (m_generator->m_config.saveParameters) {
                int actualIndex = m_generator->m_config.startIndex + task.fpIndex;
                QString paramFile = QString("%1/%2_%3_v%4_params.json")
                    .arg(m_generator->m_config.outputDirectory)
                    .arg(m_generator->m_config.filenamePrefix)
                    .arg(actualIndex, 4, 10, QChar('0'))
                    .arg(task.versionIndex);
                    
                m_generator->saveParameters(task.instance.baseParams, task.instance.basePoints, paramFile);
            }
            
            // Incrementar contador de imagens geradas
            m_generator->m_generated.fetchAndAddRelaxed(1);
            tasksProcessed++;
            
            qDebug() << "Worker" << m_workerId << "processed task" << tasksProcessed 
                     << "(FP" << task.fpIndex + 1 << "v" << task.versionIndex << ")";
        }
    }
    
    // Worker terminando
    qDebug() << "Worker" << m_workerId << "finished. Total tasks processed:" << tasksProcessed;
    m_generator->m_activeWorkers.fetchAndAddRelaxed(-1);
}

FingerprintInstance BatchGenerator::createBaseFingerprint(int index) {
    FingerprintInstance instance;
    
    // Criar identificador único
    instance.identifier = QString("FP_%1").arg(index + 1, 3, 10, QChar('0'));
    
    // Parâmetros base - GERAR IMPRESSÃO 1000x1200px @ 500 DPI
    instance.baseParams.reset();
    
    auto* rng = QRandomGenerator::global();
    // Para width=1000: left ~500, right ~500
    instance.baseParams.shape.left = 500 + rng->bounded(-30, 31);
    instance.baseParams.shape.right = 500 + rng->bounded(-30, 31);
    // Para height=1200: top ~480, middle ~240, bottom ~480
    instance.baseParams.shape.top = 480 + rng->bounded(-30, 31);
    instance.baseParams.shape.middle = 240 + rng->bounded(-20, 21);
    instance.baseParams.shape.bottom = 480 + rng->bounded(-30, 31);
    
    // Gerar pontos singulares baseados no tipo de impressão
    int width = instance.baseParams.shape.left + instance.baseParams.shape.right;
    int height = instance.baseParams.shape.top + instance.baseParams.shape.middle + 
                 instance.baseParams.shape.bottom;
    
    // Selecionar tipo por distribuição populacional
    FingerprintClass selectedClass = selectClassByPopulation();
    
    instance.basePoints.generateRandomPoints(selectedClass, width, height);
    instance.baseParams.classification.fingerprintClass = selectedClass;
    
    // Zerar edge blend na geração em massa
    instance.baseParams.orientation.loopEdgeBlendFactor = 0.0;
    instance.baseParams.orientation.whorlEdgeDecayFactor = 0.0;
    
    // Modo quiet para processamento em lote
    instance.baseParams.orientation.quietMode = m_config.quietMode;
    
    // Randomizar parâmetros de orientação para presilhas
    if (selectedClass == FingerprintClass::RightLoop || selectedClass == FingerprintClass::LeftLoop) {
        instance.baseParams.orientation.coreConvergenceStrength = rng->generateDouble() * 0.25;
        instance.baseParams.orientation.coreConvergenceRadius = rng->bounded(41);
        instance.baseParams.orientation.verticalBiasStrength = rng->generateDouble() * 0.35;
        instance.baseParams.orientation.verticalBiasRadius = rng->bounded(41);
    }
    
    return instance;
}

VersionTransform BatchGenerator::generateVersionTransform(int versionIndex) const {
    VersionTransform transform;
    
    auto* rng = QRandomGenerator::global();
    
    // Versões 2+: Transformações PERCEPTÍVEIS
    
    // Rotação angular (-15 a +15 graus) - PERCEPTÍVEL
    transform.rotation = (rng->generateDouble() - 0.5) * 30.0;
    
    // Nível de ruído (0.03 a 0.08) - PERCEPTÍVEL
    transform.noiseLevel = 0.03 + rng->generateDouble() * 0.05;
    
    // Distorção de lente: PINCUSHION (sempre true)
    // Range dobrado: -0.16 a +0.16
    transform.usePincushion = true;
    double magnitude = 0.08 + rng->generateDouble() * 0.08;  // 0.08 a 0.16
    transform.lensDistortion = (rng->generateDouble() < 0.5) ? -magnitude : magnitude;
    
    // Deslocamento homográfico (-20 a +20 pixels em X e Y)
    transform.homographyShift = QPointF(
        (rng->generateDouble() - 0.5) * 40.0,
        (rng->generateDouble() - 0.5) * 40.0
    );
    
    // Ângulo de perspectiva homográfica (-10 a +10 graus)
    transform.homographyAngle = (rng->generateDouble() - 0.5) * 20.0;
    
    // Recorte final será 500x600 (largura x altura)
    transform.cropRegion = QRect(0, 0, 500, 600);
    
    // Blur circular aleatório - centro DENTRO do cropRegion
    transform.applyBlur = true;
    transform.blurRadius = 25 + rng->bounded(126);  // 25 a 150
    
    // Centro do blur dentro do cropRegion (margem de 50px das bordas)
    int cropW = transform.cropRegion.width();
    int cropH = transform.cropRegion.height();
    transform.blurCenter = QPointF(
        50.0 + rng->generateDouble() * (cropW - 100.0),  // 50 a 450
        50.0 + rng->generateDouble() * (cropH - 100.0)   // 50 a 550
    );
    
    return transform;
}

QImage BatchGenerator::applyVersionTransforms(const QImage& baseImage, const VersionTransform& transform) const {
    QImage result = baseImage.copy();
    
    // 1. Aplicar ruído
    if (transform.noiseLevel > 0.001) {
        result = applyNoise(result, transform.noiseLevel);
    }
    
    // 2. Aplicar BLUR circular (APÓS ruído, ANTES de lens/perspectiva)
    if (transform.applyBlur && transform.blurRadius > 0) {
        result = applyBlur(result, transform.blurRadius, transform.blurCenter);
    }
    
    // 3. Aplicar distorção de lente (Barrel ou Pincushion)
    if (std::abs(transform.lensDistortion) > 0.001) {
        result = applyLensDistortion(result, transform.lensDistortion);
    }
    
    // 4. Aplicar transformação homográfica (perspectiva)
    if (std::abs(transform.homographyAngle) > 0.1 || !transform.homographyShift.isNull()) {
        result = applyHomography(result, transform.homographyShift, transform.homographyAngle);
    }
    
    // 5. Aplicar rotação
    if (std::abs(transform.rotation) > 0.1) {
        result = applyRotation(result, transform.rotation);
    }
    
    // 6. Aplicar recorte final 500x600px (largura x altura) sem bordas vazias
    result = applyCrop(result, 500, 600);
    
    // 6. Garantir DPI 500
    result.setDotsPerMeterX(500 * 39.3701);
    result.setDotsPerMeterY(500 * 39.3701);
    
    return result;
}

// Funções auxiliares de transformação
QImage BatchGenerator::applyNoise(const QImage& image, double noiseLevel) const {
    QImage noisy = image.copy();
    auto* rng = QRandomGenerator::global();
    
    for (int y = 0; y < noisy.height(); ++y) {
        for (int x = 0; x < noisy.width(); ++x) {
            QRgb pixel = noisy.pixel(x, y);
            int gray = qGray(pixel);
            
            // Adicionar ruído gaussiano
            double noise = (rng->generateDouble() - 0.5) * 255.0 * noiseLevel;
            int newGray = qBound(0, static_cast<int>(gray + noise), 255);
            
            noisy.setPixel(x, y, qRgb(newGray, newGray, newGray));
        }
    }
    
    return noisy;
}

QImage BatchGenerator::applyBlur(const QImage& image, int radius, const QPointF& center) const {
    // Blur circular gaussiano
    QImage blurred = image.copy();
    
    // Kernel gaussiano simples (3x3 para performance)
    const int kernelSize = 3;
    const double kernel[3][3] = {
        {1.0/16, 2.0/16, 1.0/16},
        {2.0/16, 4.0/16, 2.0/16},
        {1.0/16, 2.0/16, 1.0/16}
    };
    
    // Aplicar blur apenas em região circular
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            // Calcular distância do pixel ao centro do blur
            double dx = x - center.x();
            double dy = y - center.y();
            double dist = std::sqrt(dx*dx + dy*dy);
            
            // Se dentro do raio, aplicar blur
            if (dist <= radius) {
                // Intensidade do blur (mais forte no centro, decay linear)
                double blurIntensity = 1.0 - (dist / radius);
                
                // Aplicar convolução gaussiana
                double sumR = 0, sumG = 0, sumB = 0;
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int px = qBound(0, x + kx, image.width() - 1);
                        int py = qBound(0, y + ky, image.height() - 1);
                        QRgb pixel = image.pixel(px, py);
                        double weight = kernel[ky + 1][kx + 1];
                        sumR += qRed(pixel) * weight;
                        sumG += qGreen(pixel) * weight;
                        sumB += qBlue(pixel) * weight;
                    }
                }
                
                // Interpolar entre original e blur
                QRgb original = image.pixel(x, y);
                int finalR = qBound(0, static_cast<int>(qRed(original) * (1-blurIntensity) + sumR * blurIntensity), 255);
                int finalG = qBound(0, static_cast<int>(qGreen(original) * (1-blurIntensity) + sumG * blurIntensity), 255);
                int finalB = qBound(0, static_cast<int>(qBlue(original) * (1-blurIntensity) + sumB * blurIntensity), 255);
                
                blurred.setPixel(x, y, qRgb(finalR, finalG, finalB));
            }
        }
    }
    
    return blurred;
}

QImage BatchGenerator::applyLensDistortion(const QImage& image, double k) const {
    QImage distorted(image.size(), image.format());
    distorted.fill(Qt::white);
    
    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;
    double maxRadius = std::sqrt(cx * cx + cy * cy);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Normalizar coordenadas (-1 a +1)
            double nx = (x - cx) / cx;
            double ny = (y - cy) / cy;
            double r = std::sqrt(nx * nx + ny * ny);
            
            // Aplicar distorção barrel: r' = r * (1 + k * r^2)
            double rDistorted = r * (1.0 + k * r * r);
            
            // Converter de volta para coordenadas de imagem
            double srcX = cx + (nx / r) * rDistorted * cx;
            double srcY = cy + (ny / r) * rDistorted * cy;
            
            // Interpolação bilinear
            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;
                
                QRgb p00 = image.pixel(x0, y0);
                QRgb p10 = image.pixel(x0 + 1, y0);
                QRgb p01 = image.pixel(x0, y0 + 1);
                QRgb p11 = image.pixel(x0 + 1, y0 + 1);
                
                int gray = static_cast<int>(
                    qGray(p00) * (1 - fx) * (1 - fy) +
                    qGray(p10) * fx * (1 - fy) +
                    qGray(p01) * (1 - fx) * fy +
                    qGray(p11) * fx * fy
                );
                
                distorted.setPixel(x, y, qRgb(gray, gray, gray));
            }
        }
    }
    
    return distorted;
}

QImage BatchGenerator::applyHomography(const QImage& image, const QPointF& shift, double angle) const {
    QImage result(image.size(), image.format());
    result.fill(Qt::white);
    
    double rad = angle * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);
    
    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Aplicar perspectiva simples (shear + deslocamento)
            double nx = x - cx;
            double ny = y - cy;
            
            // Transformação de perspectiva simples
            double srcX = nx * cosA - ny * sinA * 0.3 + shift.x() + cx;
            double srcY = nx * sinA * 0.3 + ny * cosA + shift.y() + cy;
            
            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;
                
                QRgb p00 = image.pixel(x0, y0);
                QRgb p10 = image.pixel(x0 + 1, y0);
                QRgb p01 = image.pixel(x0, y0 + 1);
                QRgb p11 = image.pixel(x0 + 1, y0 + 1);
                
                int gray = static_cast<int>(
                    qGray(p00) * (1 - fx) * (1 - fy) +
                    qGray(p10) * fx * (1 - fy) +
                    qGray(p01) * (1 - fx) * fy +
                    qGray(p11) * fx * fy
                );
                
                result.setPixel(x, y, qRgb(gray, gray, gray));
            }
        }
    }
    
    return result;
}

QImage BatchGenerator::applyRotation(const QImage& image, double angle) const {
    QTransform transform;
    transform.rotate(angle);
    
    QImage rotated = image.transformed(transform, Qt::SmoothTransformation);
    
    // Retornar ao tamanho original (crop central)
    int dx = (rotated.width() - image.width()) / 2;
    int dy = (rotated.height() - image.height()) / 2;
    
    if (dx >= 0 && dy >= 0) {
        return rotated.copy(dx, dy, image.width(), image.height());
    }
    
    return rotated;
}

QImage BatchGenerator::applyCrop(const QImage& image, int targetWidth, int targetHeight) const {
    // Recorte EM TORNO do centro usando coordenadas polares
    // Centro do retângulo de recorte varia em distância [0, 150px] e ângulo [0, 360°]
    // Imagem base é ~1000x1200, recorte é 500x600 (largura x altura)
    
    if (image.width() < targetWidth || image.height() < targetHeight) {
        // Se imagem menor que alvo (não deveria acontecer), retornar com padding
        QImage result(targetWidth, targetHeight, image.format());
        result.fill(Qt::white);
        
        int offsetX = (targetWidth - image.width()) / 2;
        int offsetY = (targetHeight - image.height()) / 2;
        
        QPainter painter(&result);
        painter.drawImage(offsetX, offsetY, image);
        return result;
    }
    
    auto* rng = QRandomGenerator::global();
    
    // Centro da imagem base
    int imageCenterX = image.width() / 2;
    int imageCenterY = image.height() / 2;
    
    // Coordenadas polares: distância radial [0, 150] e ângulo [0, 2π]
    double maxRadius = 150.0;
    double radius = rng->generateDouble() * maxRadius;
    double angle = rng->generateDouble() * 2.0 * M_PI;
    
    // Calcular centro do retângulo de recorte em coordenadas cartesianas
    int cropCenterX = imageCenterX + static_cast<int>(radius * std::cos(angle));
    int cropCenterY = imageCenterY + static_cast<int>(radius * std::sin(angle));
    
    // Calcular canto superior esquerdo do recorte
    int cropX = cropCenterX - targetWidth / 2;
    int cropY = cropCenterY - targetHeight / 2;
    
    // Garantir que recorte fica dentro dos limites da imagem (com margem de segurança)
    // Margem de 50px das bordas para garantir que não pega áreas sem cristas/vales
    int margin = 50;
    cropX = qBound(margin, cropX, image.width() - targetWidth - margin);
    cropY = qBound(margin, cropY, image.height() - targetHeight - margin);
    
    return image.copy(cropX, cropY, targetWidth, targetHeight);
}

QImage BatchGenerator::applyEllipticalMask(const QImage& image) const {
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

FingerprintClass BatchGenerator::selectClassByPopulation() const {
    // Distribuição populacional baseada em estatísticas forenses reais:
    // Fontes: Crime Scene Investigator Network, PIT-4, estudos populacionais
    // 
    // LOOPS TOTAIS: ~65% (LeftLoop e RightLoop IGUALMENTE distribuídos)
    // WHORLS TOTAIS: ~30% (Plain ~70%, Double/TwinLoop ~30%)
    // ARCHES TOTAIS: ~5% (Plain ~60%, Tented ~40%)
    //
    // LeftLoop: 32.5%  (metade dos loops)
    // RightLoop: 32.5% (metade dos loops)
    // Whorl (Plain): 21%
    // TwinLoop (Double): 9%
    // Arch (Plain): 3%
    // TentedArch: 2%
    
    auto* rng = QRandomGenerator::global();
    double random = rng->generateDouble(); // 0.0 a 1.0
    
    // Probabilidades acumuladas CORRIGIDAS com distribuição estatística real
    if (random < 0.325) return FingerprintClass::LeftLoop;     // 0.000 - 0.325 (32.5%)
    if (random < 0.650) return FingerprintClass::RightLoop;    // 0.325 - 0.650 (32.5%)
    if (random < 0.860) return FingerprintClass::Whorl;        // 0.650 - 0.860 (21.0%)
    if (random < 0.950) return FingerprintClass::TwinLoop;     // 0.860 - 0.950 (9.0%)
    if (random < 0.980) return FingerprintClass::Arch;         // 0.950 - 0.980 (3.0%)
    return FingerprintClass::TentedArch;                        // 0.980 - 1.000 (2.0%)
}

bool BatchGenerator::saveFingerprint(const QImage& image, const FingerprintInstance& instance,
                                    int fpIndex, int versionIndex) {
    // Usar startIndex para calcular o índice real da impressão
    int actualIndex = m_config.startIndex + fpIndex;
    QString filename = QString("%1/%2_%3_v%4.png")
        .arg(m_config.outputDirectory)
        .arg(m_config.filenamePrefix)
        .arg(actualIndex, 4, 10, QChar('0'))
        .arg(versionIndex, 2, 10, QChar('0'));
    
    return image.save(filename);
}

bool BatchGenerator::saveParameters(const FingerprintParameters& params, const SingularPoints& points,
                                   const QString& filename) {
    QJsonObject json;
    json["parameters"] = params.toJson();
    json["singular_points"] = points.toJson();
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(json);
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

}
