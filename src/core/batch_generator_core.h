#ifndef BATCH_GENERATOR_CORE_H
#define BATCH_GENERATOR_CORE_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QAtomicInt>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QSemaphore>
#include <QThreadPool>
#include <QRunnable>
#include <QCoreApplication>
#include "fingerprint_generator.h"
#include "../models/fingerprint_parameters.h"
#include "../models/singular_points.h"

namespace SFinGe {

struct VersionTransform {
    double rotation = 0.0;
    double noiseLevel = 0.0;
    double lensDistortion = 0.0;
    bool usePincushion = true;
    QPointF homographyShift;
    double homographyAngle = 0.0;
    QRect cropRegion;
    bool applyBlur = false;
    int blurRadius = 0;
    QPointF blurCenter;
};

struct BatchConfig {
    int numFingerprints = 10;
    int versionsPerFingerprint = 3;
    int startIndex = 0;
    bool usePopulationDistribution = true;
    bool skipOriginal = true;
    bool applyEllipticalMask = true;
    bool quietMode = false;
    
    QString outputDirectory = ".";
    QString filenamePrefix = "fingerprint";
    bool saveParameters = false;
};

struct FingerprintInstance {
    FingerprintParameters baseParams;
    SingularPoints basePoints;
    QString identifier;
};

class BatchGeneratorCore : public QObject {
    Q_OBJECT
    
public:
    explicit BatchGeneratorCore(QObject* parent = nullptr);
    ~BatchGeneratorCore();
    
    void setBatchConfig(const BatchConfig& config);
    BatchConfig getBatchConfig() const { return m_config; }
    void setNumWorkers(int workers) { m_numWorkers = workers; }
    
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
    QImage applyNoise(const QImage& image, double noiseLevel) const;
    QImage applyBlur(const QImage& image, int radius, const QPointF& center) const;
    QImage applyLensDistortion(const QImage& image, double k) const;
    QImage applyHomography(const QImage& image, const QPointF& shift, double angle) const;
    QImage applyRotation(const QImage& image, double angle) const;
    QImage applyCrop(const QImage& image, int targetWidth, int targetHeight) const;
    QImage applyEllipticalMask(const QImage& image) const;
    bool saveFingerprint(const QImage& image, const FingerprintInstance& instance, int fpIndex, int versionIndex);
    FingerprintClass selectClassByPopulation() const;
    
    BatchConfig m_config;
    FingerprintGenerator* m_generator;
    bool m_cancelled;
    QElapsedTimer m_timer;
    qint64 m_firstImageTime;
    int m_numWorkers = 0;
    
    QAtomicInt m_generated;
    QAtomicInt m_activeWorkers;
    QMutex m_queueMutex;
    QMutex m_progressMutex;
    QQueue<int> m_taskQueue;
    QSemaphore m_queueSemaphore;
};

}

#endif
