#ifndef FINGERPRINT_WORKER_H
#define FINGERPRINT_WORKER_H

#include <QThread>
#include <QImage>
#include "fingerprint_generator.h"
#include "models/fingerprint_parameters.h"
#include "models/singular_points.h"

namespace SFinGe {

class FingerprintWorker : public QThread {
    Q_OBJECT
    
public:
    explicit FingerprintWorker(QObject* parent = nullptr);
    ~FingerprintWorker();
    
    void setParameters(const FingerprintParameters& params);
    void setSingularPoints(const SingularPoints& points);
    void setApplyEllipticalMask(bool apply);
    void cancelGeneration();
    
signals:
    void progressChanged(int percentage, const QString& message);
    void fingerprintGenerated(const QImage& image);
    void generationFailed(const QString& error);
    
protected:
    void run() override;
    
private:
    QImage applyEllipticalMask(const QImage& image) const;
    
    FingerprintParameters m_params;
    SingularPoints m_points;
    FingerprintGenerator* m_generator;
    bool m_cancelled;
    bool m_applyEllipticalMask;
};

}

#endif
