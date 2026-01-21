#include "batch_generator_core.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QThread>
#include <QDebug>
#include <cmath>
#include <random>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

BatchGeneratorCore::BatchGeneratorCore(QObject* parent)
    : QObject(parent)
    , m_generator(new FingerprintGenerator(this))
    , m_cancelled(false)
    , m_firstImageTime(0) {
}

BatchGeneratorCore::~BatchGeneratorCore() {
}

void BatchGeneratorCore::setBatchConfig(const BatchConfig& config) {
    m_config = config;
}

bool BatchGeneratorCore::generateBatch() {
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
    
    qDebug() << "Starting parallel batch generation with" << numWorkers << "workers";
    qDebug() << "Total fingerprints:" << m_config.numFingerprints << "Total images:" << totalImages;
    
    // Pré-criar todas as instâncias de impressão digital
    std::vector<FingerprintInstance> instances(m_config.numFingerprints);
    for (int i = 0; i < m_config.numFingerprints && !m_cancelled; ++i) {
        instances[i] = createBaseFingerprint(i);
    }
    
    // Criar fila de tarefas
    QQueue<int> taskQueue;
    for (int i = 0; i < m_config.numFingerprints; ++i) {
        taskQueue.enqueue(i);
    }
    
    QMutex taskMutex;
    QMutex progressMutex;
    QAtomicInt completedFps(0);
    
    // Função de processamento para cada worker
    auto workerFunc = [&]() {
        // Cada thread precisa de seu próprio FingerprintGenerator
        FingerprintGenerator localGenerator;
        
        while (!m_cancelled) {
            int taskIndex;
            bool hasTask = false;
            
            {
                QMutexLocker locker(&taskMutex);
                if (!taskQueue.isEmpty()) {
                    taskIndex = taskQueue.dequeue();
                    hasTask = true;
                }
            }
            
            if (!hasTask) break;
            
            // Configurar gerador local
            localGenerator.setParameters(instances[taskIndex].baseParams);
            localGenerator.setSingularPoints(instances[taskIndex].basePoints);
            
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
                if (!saveFingerprint(transformedFingerprint, instances[taskIndex], taskIndex, verIdx)) {
                    continue;
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

void BatchGeneratorCore::cancel() {
    m_cancelled = true;
}

FingerprintInstance BatchGeneratorCore::createBaseFingerprint(int index) {
    FingerprintInstance instance;
    
    instance.identifier = QString("FP_%1").arg(index + 1, 3, 10, QChar('0'));
    
    instance.baseParams.reset();
    
    auto* rng = QRandomGenerator::global();
    instance.baseParams.shape.left = 500 + rng->bounded(-30, 31);
    instance.baseParams.shape.right = 500 + rng->bounded(-30, 31);
    instance.baseParams.shape.top = 480 + rng->bounded(-30, 31);
    instance.baseParams.shape.middle = 240 + rng->bounded(-20, 21);
    instance.baseParams.shape.bottom = 480 + rng->bounded(-30, 31);
    
    int width = instance.baseParams.shape.left + instance.baseParams.shape.right;
    int height = instance.baseParams.shape.top + instance.baseParams.shape.middle + 
                 instance.baseParams.shape.bottom;
    
    // Selecionar tipo por distribuição populacional
    FingerprintClass selectedClass = FingerprintClass::RightLoop; // Simplificado
    
    instance.basePoints.generateRandomPoints(selectedClass, width, height);
    instance.baseParams.classification.fingerprintClass = selectedClass;
    
    // Zerar edge blend na geração em massa
    instance.baseParams.orientation.loopEdgeBlendFactor = 0.0;
    instance.baseParams.orientation.whorlEdgeDecayFactor = 0.0;
    
    // Aplicar variação gaussiana nos alphas
    static std::mt19937 gen(std::random_device{}());
    std::normal_distribution<double> coreAlphaDist(1.0, 0.025);
    std::normal_distribution<double> deltaAlphaDist(-1.0, 0.025);
    
    for (int i = 0; i < instance.basePoints.getCoreCount(); ++i) {
        double newAlpha = coreAlphaDist(gen);
        instance.basePoints.updateCoreAlpha(i, newAlpha);
    }
    
    for (int i = 0; i < instance.basePoints.getDeltaCount(); ++i) {
        double newAlpha = deltaAlphaDist(gen);
        instance.basePoints.updateDeltaAlpha(i, newAlpha);
    }
    
    return instance;
}

}
