#ifndef BATCH_GENERATOR_H
#define BATCH_GENERATOR_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QElapsedTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QAtomicInt>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QSemaphore>
#include "fingerprint_generator.h"
#include "../models/fingerprint_parameters.h"
#include "../models/singular_points.h"

namespace SFinGe {

struct VersionTransform {
    double rotation = 0.0;           // Rotação em graus (-15 a +15)
    double noiseLevel = 0.0;         // Nível de ruído adicional (0.0 a 0.05)
    double lensDistortion = 0.0;     // Distorção de lente (-0.08 a +0.08)
    bool usePincushion = false;      // true=pincushion (negativo), false=barrel (positivo)
    QPointF homographyShift;         // Deslocamento homográfico em pixels
    double homographyAngle = 0.0;    // Ângulo de perspectiva (graus)
    QRect cropRegion;                // Região de recorte 500x600px
    // Blur circular aleatório
    bool applyBlur = false;          // Aplicar blur circular
    int blurRadius = 0;              // Raio do blur (100-300 pixels)
    QPointF blurCenter;              // Centro do blur na imagem
};

struct BatchConfig {
    int numFingerprints = 10;        // Número de impressões diferentes
    int versionsPerFingerprint = 3;  // Versões de cada impressão
    bool usePopulationDistribution = true;  // Sempre usar distribuição populacional
    bool skipOriginal = true;        // Excluir v0 (marcado por padrão)
    bool applyEllipticalMask = true; // Aplicar máscara elíptica com fade out (padrão: sim)
    
    QString outputDirectory = ".";
    QString filenamePrefix = "fingerprint";
    bool saveParameters = false;     // Salvar JSON (padrão: não)
};

struct FingerprintInstance {
    FingerprintParameters baseParams;
    SingularPoints basePoints;
    QString identifier;
};

class BatchGenerator : public QObject {
    Q_OBJECT
    
public:
    explicit BatchGenerator(QObject* parent = nullptr);
    ~BatchGenerator();
    
    void setBatchConfig(const BatchConfig& config);
    BatchConfig getBatchConfig() const { return m_config; }
    
    bool generateBatch();
    void cancel();
    
signals:
    void progressUpdated(int current, int total, const QString& status);
    void batchCompleted(int generated);
    void error(const QString& message);
    
private:
    FingerprintInstance createBaseFingerprint(int index);
    VersionTransform generateVersionTransform(int versionIndex) const;
    QImage applyVersionTransforms(const QImage& baseImage, const VersionTransform& transform) const;
    FingerprintClass selectClassByPopulation() const;  // Seleção por distribuição populacional
    
    // Funções de transformação de imagem
    QImage applyNoise(const QImage& image, double noiseLevel) const;
    QImage applyBlur(const QImage& image, int radius, const QPointF& center) const;
    QImage applyLensDistortion(const QImage& image, double k) const;  // Barrel ou Pincushion
    QImage applyHomography(const QImage& image, const QPointF& shift, double angle) const;
    QImage applyRotation(const QImage& image, double angle) const;
    QImage applyCrop(const QImage& image, int targetWidth, int targetHeight) const;
    QImage applyEllipticalMask(const QImage& image) const;  // Máscara elíptica com fade out
    
    bool saveFingerprint(const QImage& image, const FingerprintInstance& instance, 
                        int fpIndex, int versionIndex);
    bool saveParameters(const FingerprintParameters& params, const SingularPoints& points,
                       const QString& filename);
    
    // Estrutura de tarefa para fila
    struct FingerprintTask {
        FingerprintInstance instance;
        int fpIndex;
        int versionIndex;
        int totalImages;
        QImage baseFingerprint;  // Imagem base pré-gerada
    };
    
    // Classe worker para processamento contínuo
    class WorkerThread : public QThread {
    public:
        WorkerThread(BatchGenerator* generator, int workerId);
        void run() override;
        void stop();
        
    private:
        BatchGenerator* m_generator;
        int m_workerId;
        bool m_running;
    };
    
    BatchConfig m_config;
    FingerprintGenerator* m_generator;
    bool m_cancelled;
    QElapsedTimer m_timer;
    qint64 m_firstImageTime;
    
    // Variáveis para processamento com fila
    QAtomicInt m_generated;
    QAtomicInt m_activeWorkers;
    QMutex m_queueMutex;
    QMutex m_progressMutex;
    QQueue<FingerprintTask> m_taskQueue;
    QSemaphore m_queueSemaphore;  // Sinaliza tarefas disponíveis
    QList<WorkerThread*> m_workers;
};

}

#endif
