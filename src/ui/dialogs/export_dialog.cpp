#include "export_dialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

namespace SFinGe {

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi();
}

void ExportDialog::setupUi() {
    setWindowTitle(tr("Export Options"));
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();
    
    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("PNG", "png");
    m_formatCombo->addItem("BMP", "bmp");
    m_formatCombo->addItem("TIFF", "tiff");
    formLayout->addRow(tr("Format:"), m_formatCombo);
    
    m_qualitySpin = new QSpinBox(this);
    m_qualitySpin->setRange(1, 100);
    m_qualitySpin->setValue(95);
    formLayout->addRow(tr("Quality:"), m_qualitySpin);
    
    mainLayout->addLayout(formLayout);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

QString ExportDialog::getFormat() const {
    return m_formatCombo->currentData().toString();
}

int ExportDialog::getQuality() const {
    return m_qualitySpin->value();
}

}
