#ifndef BATCH_WORKER_H
#define BATCH_WORKER_H

#include <QThread>
#include "batch_generator.h"

namespace SFinGe {

class BatchWorker : public QThread {
    Q_OBJECT
    
public:
    explicit BatchWorker(QObject* parent = nullptr);
    ~BatchWorker();
    
    void setBatchConfig(const BatchConfig& config);
    void cancelBatch();
    
signals:
    void progressUpdated(int current, int total, const QString& status);
    void batchCompleted(int generated);
    void error(const QString& message);
    
protected:
    void run() override;
    
private:
    BatchConfig m_config;
    BatchGenerator* m_batchGenerator;
    bool m_cancelled;
};

}

#endif
