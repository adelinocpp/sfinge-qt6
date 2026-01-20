#include "batch_worker.h"
#include <QDir>

namespace SFinGe {

BatchWorker::BatchWorker(QObject* parent)
    : QThread(parent)
    , m_batchGenerator(new BatchGenerator())
    , m_cancelled(false) {
    
    // Conectar sinais do BatchGenerator
    connect(m_batchGenerator, &BatchGenerator::progressUpdated, 
            this, &BatchWorker::progressUpdated);
    connect(m_batchGenerator, &BatchGenerator::batchCompleted,
            this, &BatchWorker::batchCompleted);
    connect(m_batchGenerator, &BatchGenerator::error,
            this, &BatchWorker::error);
}

BatchWorker::~BatchWorker() {
    cancelBatch();
    wait();
    if (m_batchGenerator) {
        delete m_batchGenerator;
    }
}

void BatchWorker::setBatchConfig(const BatchConfig& config) {
    m_config = config;
    if (m_batchGenerator) {
        m_batchGenerator->setBatchConfig(config);
    }
}

void BatchWorker::cancelBatch() {
    m_cancelled = true;
    if (m_batchGenerator) {
        m_batchGenerator->cancel();
    }
}

void BatchWorker::run() {
    m_cancelled = false;
    
    // Delegar toda geração ao BatchGenerator
    if (m_batchGenerator) {
        m_batchGenerator->generateBatch();
    }
}

}
