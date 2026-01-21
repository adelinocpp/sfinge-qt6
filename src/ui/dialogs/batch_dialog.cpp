#include "batch_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QCheckBox>

namespace SFinGe {

BatchDialog::BatchDialog(QWidget* parent)
    : QDialog(parent)
    , m_batchWorker(new BatchWorker(this))
    , m_generating(false) {
    
    setupUi();
    
    connect(m_batchWorker, &BatchWorker::progressUpdated, this, &BatchDialog::onProgressUpdated);
    connect(m_batchWorker, &BatchWorker::batchCompleted, this, &BatchDialog::onBatchCompleted);
    connect(m_batchWorker, &BatchWorker::error, this, &BatchDialog::onError);
}

BatchDialog::~BatchDialog() {
}

void BatchDialog::setupUi() {
    setWindowTitle(tr("Batch Fingerprint Generation"));
    setMinimumWidth(500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Configuração de quantidade
    QGroupBox* quantityGroup = new QGroupBox(tr("Quantity"), this);
    QFormLayout* quantityLayout = new QFormLayout(quantityGroup);
    
    m_numFingerprintsSpinBox = new QSpinBox(this);
    m_numFingerprintsSpinBox->setRange(1, 1000);
    m_numFingerprintsSpinBox->setValue(10);
    quantityLayout->addRow(tr("Number of fingerprints:"), m_numFingerprintsSpinBox);
    
    m_versionsSpinBox = new QSpinBox(this);
    m_versionsSpinBox->setRange(1, 10);
    m_versionsSpinBox->setValue(3);
    quantityLayout->addRow(tr("Versions per fingerprint:"), m_versionsSpinBox);
    
    m_startIndexSpinBox = new QSpinBox(this);
    m_startIndexSpinBox->setRange(0, 999999);
    m_startIndexSpinBox->setValue(0);
    m_startIndexSpinBox->setToolTip(tr("Starting index for fingerprint numbering (e.g., 0 = fingerprint_0000, 100 = fingerprint_0100)"));
    quantityLayout->addRow(tr("Start index:"), m_startIndexSpinBox);
    
    mainLayout->addWidget(quantityGroup);
    
    // Configuração de saída
    QGroupBox* outputGroup = new QGroupBox(tr("Output"), this);
    QFormLayout* outputLayout = new QFormLayout(outputGroup);
    
    QHBoxLayout* dirLayout = new QHBoxLayout();
    m_directoryEdit = new QLineEdit(this);
    m_directoryEdit->setText(QDir::currentPath());
    m_browseButton = new QPushButton(tr("Browse..."), this);
    connect(m_browseButton, &QPushButton::clicked, this, &BatchDialog::onSelectDirectory);
    dirLayout->addWidget(m_directoryEdit);
    dirLayout->addWidget(m_browseButton);
    outputLayout->addRow(tr("Output directory:"), dirLayout);
    
    m_prefixEdit = new QLineEdit(this);
    m_prefixEdit->setText("fingerprint");
    outputLayout->addRow(tr("Filename prefix:"), m_prefixEdit);
    
    // Checkbox para salvar JSON (desmarcado por padrão)
    m_saveParamsCheckBox = new QCheckBox(tr("Save parameters as JSON"), this);
    m_saveParamsCheckBox->setChecked(false);
    m_saveParamsCheckBox->setToolTip(tr("Save fingerprint parameters and singular points to JSON files"));
    outputLayout->addRow("", m_saveParamsCheckBox);
    
    // Checkbox para excluir original (marcado por padrão)
    m_skipOriginalCheckBox = new QCheckBox(tr("Exclude original image (v0)"), this);
    m_skipOriginalCheckBox->setChecked(true);
    m_skipOriginalCheckBox->setToolTip(tr("Skip saving the original 1200x1000 image, save only transformed versions"));
    outputLayout->addRow("", m_skipOriginalCheckBox);
    
    // Checkbox para máscara elíptica (marcado por padrão)
    m_ellipticalMaskCheckBox = new QCheckBox(tr("Apply elliptical mask with fade out"), this);
    m_ellipticalMaskCheckBox->setChecked(true);
    m_ellipticalMaskCheckBox->setToolTip(tr("Apply smooth elliptical mask fading to white at edges"));
    outputLayout->addRow("", m_ellipticalMaskCheckBox);
    
    mainLayout->addWidget(outputGroup);
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    mainLayout->addWidget(m_statusLabel);
    
    // Botões
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_generateButton = new QPushButton(tr("Generate"), this);
    m_generateButton->setDefault(true);
    connect(m_generateButton, &QPushButton::clicked, this, &BatchDialog::onGenerate);
    buttonLayout->addWidget(m_generateButton);
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &BatchDialog::onCancel);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
}

void BatchDialog::onSelectDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"),
                                                     m_directoryEdit->text());
    if (!dir.isEmpty()) {
        m_directoryEdit->setText(dir);
    }
}

void BatchDialog::onGenerate() {
    if (m_generating) {
        return;
    }
    
    QString directory = m_directoryEdit->text();
    if (directory.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select an output directory"));
        return;
    }
    
    BatchConfig config;
    config.numFingerprints = m_numFingerprintsSpinBox->value();
    config.versionsPerFingerprint = m_versionsSpinBox->value();
    config.startIndex = m_startIndexSpinBox->value();
    config.outputDirectory = directory;
    config.filenamePrefix = m_prefixEdit->text();
    config.usePopulationDistribution = true;
    config.saveParameters = m_saveParamsCheckBox->isChecked();
    config.skipOriginal = m_skipOriginalCheckBox->isChecked();
    config.applyEllipticalMask = m_ellipticalMaskCheckBox->isChecked();
    
    m_batchWorker->setBatchConfig(config);
    
    m_generating = true;
    m_generateButton->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLabel->setVisible(true);
    
    m_batchWorker->start();
}

void BatchDialog::onCancel() {
    if (m_generating) {
        m_batchWorker->cancelBatch();
        m_statusLabel->setText(tr("Cancelling..."));
        m_cancelButton->setEnabled(false);
    } else {
        reject();
    }
}

void BatchDialog::onProgressUpdated(int current, int total, const QString& status) {
    updateProgress(current, total);
    m_statusLabel->setText(status);
}

void BatchDialog::onBatchCompleted(int generated) {
    m_generating = false;
    m_generateButton->setEnabled(true);
    m_progressBar->setValue(m_progressBar->maximum());
    
    QMessageBox::information(this, tr("Batch Complete"),
                            tr("Successfully generated %1 fingerprint images.").arg(generated));
    
    accept();
}

void BatchDialog::onError(const QString& message) {
    QMessageBox::warning(this, tr("Error"), message);
}

void BatchDialog::updateProgress(int current, int total) {
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(current);
}

}
