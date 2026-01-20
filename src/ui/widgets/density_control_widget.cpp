#include "density_control_widget.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>

namespace SFinGe {

DensityControlWidget::DensityControlWidget(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void DensityControlWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* densityGroup = new QGroupBox(tr("Density Parameters"), this);
    QFormLayout* formLayout = new QFormLayout(densityGroup);
    
    m_zoomSpin = new QDoubleSpinBox(this);
    m_zoomSpin->setRange(0.1, 10.0);
    m_zoomSpin->setSingleStep(0.1);
    m_zoomSpin->setValue(1.0);
    formLayout->addRow(tr("Zoom:"), m_zoomSpin);
    
    m_amplifySpin = new QDoubleSpinBox(this);
    m_amplifySpin->setRange(0.0, 1.0);
    m_amplifySpin->setSingleStep(0.05);
    m_amplifySpin->setValue(0.5);
    formLayout->addRow(tr("Amplify:"), m_amplifySpin);
    
    m_minFreqSpin = new QDoubleSpinBox(this);
    m_minFreqSpin->setRange(0.01, 0.5);
    m_minFreqSpin->setSingleStep(0.01);
    m_minFreqSpin->setDecimals(3);
    m_minFreqSpin->setValue(1.0 / 15.0);
    formLayout->addRow(tr("Min Frequency:"), m_minFreqSpin);
    
    m_maxFreqSpin = new QDoubleSpinBox(this);
    m_maxFreqSpin->setRange(0.01, 0.5);
    m_maxFreqSpin->setSingleStep(0.01);
    m_maxFreqSpin->setDecimals(3);
    m_maxFreqSpin->setValue(1.0 / 5.0);
    formLayout->addRow(tr("Max Frequency:"), m_maxFreqSpin);
    
    mainLayout->addWidget(densityGroup);
    mainLayout->addStretch();
    
    connect(m_zoomSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &DensityControlWidget::parametersChanged);
    connect(m_amplifySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &DensityControlWidget::parametersChanged);
    connect(m_minFreqSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &DensityControlWidget::parametersChanged);
    connect(m_maxFreqSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &DensityControlWidget::parametersChanged);
}

void DensityControlWidget::setParameters(const DensityParameters& params) {
    m_zoomSpin->setValue(params.zoom);
    m_amplifySpin->setValue(params.amplify);
    m_minFreqSpin->setValue(params.minFrequency);
    m_maxFreqSpin->setValue(params.maxFrequency);
}

void DensityControlWidget::updateParameters(DensityParameters& params) const {
    params.zoom = m_zoomSpin->value();
    params.amplify = m_amplifySpin->value();
    params.minFrequency = m_minFreqSpin->value();
    params.maxFrequency = m_maxFreqSpin->value();
}

}
