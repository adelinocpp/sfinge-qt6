#ifndef BATCH_DIALOG_H
#define BATCH_DIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QCheckBox>
#include <QLabel>
#include "core/batch_worker.h"

namespace SFinGe {

class BatchDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit BatchDialog(QWidget* parent = nullptr);
    ~BatchDialog();
    
private slots:
    void onSelectDirectory();
    void onGenerate();
    void onCancel();
    void onProgressUpdated(int current, int total, const QString& status);
    void onBatchCompleted(int generated);
    void onError(const QString& message);
    
private:
    void setupUi();
    void updateProgress(int current, int total);
    
    // UI Controls
    QSpinBox* m_numFingerprintsSpinBox;
    QSpinBox* m_versionsSpinBox;
    QSpinBox* m_startIndexSpinBox;
    QLineEdit* m_directoryEdit;
    QPushButton* m_browseButton;
    QLineEdit* m_prefixEdit;
    QCheckBox* m_saveParamsCheckBox;
    QCheckBox* m_skipOriginalCheckBox;
    QCheckBox* m_ellipticalMaskCheckBox;
    QPushButton* m_generateButton;
    QPushButton* m_cancelButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    
    BatchWorker* m_batchWorker;
    bool m_generating;
};

}

#endif
