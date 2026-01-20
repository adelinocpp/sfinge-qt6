#include "shape_control_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>

namespace SFinGe {

ShapeControlWidget::ShapeControlWidget(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void ShapeControlWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* shapeGroup = new QGroupBox(tr("Shape Dimensions"), this);
    QFormLayout* formLayout = new QFormLayout(shapeGroup);
    
    m_leftSlider = new QSlider(Qt::Horizontal, this);
    m_leftSlider->setRange(10, 500);
    m_leftSlider->setValue(250);
    m_leftLabel = new QLabel("250", this);
    QHBoxLayout* leftLayout = new QHBoxLayout();
    leftLayout->addWidget(m_leftSlider);
    leftLayout->addWidget(m_leftLabel);
    formLayout->addRow(tr("Left:"), leftLayout);
    
    m_rightSlider = new QSlider(Qt::Horizontal, this);
    m_rightSlider->setRange(10, 500);
    m_rightSlider->setValue(250);
    m_rightLabel = new QLabel("250", this);
    QHBoxLayout* rightLayout = new QHBoxLayout();
    rightLayout->addWidget(m_rightSlider);
    rightLayout->addWidget(m_rightLabel);
    formLayout->addRow(tr("Right:"), rightLayout);
    
    m_topSlider = new QSlider(Qt::Horizontal, this);
    m_topSlider->setRange(10, 600);
    m_topSlider->setValue(300);
    m_topLabel = new QLabel("300", this);
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(m_topSlider);
    topLayout->addWidget(m_topLabel);
    formLayout->addRow(tr("Top:"), topLayout);
    
    m_bottomSlider = new QSlider(Qt::Horizontal, this);
    m_bottomSlider->setRange(10, 500);
    m_bottomSlider->setValue(200);
    m_bottomLabel = new QLabel("200", this);
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(m_bottomSlider);
    bottomLayout->addWidget(m_bottomLabel);
    formLayout->addRow(tr("Bottom:"), bottomLayout);
    
    m_middleSlider = new QSlider(Qt::Horizontal, this);
    m_middleSlider->setRange(10, 400);
    m_middleSlider->setValue(100);
    m_middleLabel = new QLabel("100", this);
    QHBoxLayout* middleLayout = new QHBoxLayout();
    middleLayout->addWidget(m_middleSlider);
    middleLayout->addWidget(m_middleLabel);
    formLayout->addRow(tr("Middle:"), middleLayout);
    
    m_fingerTypeCombo = new QComboBox(this);
    m_fingerTypeCombo->addItem(tr("Thumb"), static_cast<int>(FingerType::Thumb));
    m_fingerTypeCombo->addItem(tr("Index"), static_cast<int>(FingerType::Index));
    m_fingerTypeCombo->addItem(tr("Middle"), static_cast<int>(FingerType::Middle));
    m_fingerTypeCombo->addItem(tr("Ring"), static_cast<int>(FingerType::Ring));
    m_fingerTypeCombo->addItem(tr("Little"), static_cast<int>(FingerType::Little));
    m_fingerTypeCombo->setCurrentIndex(1);
    formLayout->addRow(tr("Finger Type:"), m_fingerTypeCombo);
    
    mainLayout->addWidget(shapeGroup);
    mainLayout->addStretch();
    
    connect(m_leftSlider, &QSlider::valueChanged, this, &ShapeControlWidget::onSliderChanged);
    connect(m_rightSlider, &QSlider::valueChanged, this, &ShapeControlWidget::onSliderChanged);
    connect(m_topSlider, &QSlider::valueChanged, this, &ShapeControlWidget::onSliderChanged);
    connect(m_bottomSlider, &QSlider::valueChanged, this, &ShapeControlWidget::onSliderChanged);
    connect(m_middleSlider, &QSlider::valueChanged, this, &ShapeControlWidget::onSliderChanged);
    connect(m_fingerTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShapeControlWidget::parametersChanged);
}

void ShapeControlWidget::setParameters(const ShapeParameters& params) {
    m_leftSlider->setValue(params.left);
    m_rightSlider->setValue(params.right);
    m_topSlider->setValue(params.top);
    m_bottomSlider->setValue(params.bottom);
    m_middleSlider->setValue(params.middle);
    m_fingerTypeCombo->setCurrentIndex(static_cast<int>(params.fingerType));
    updateLabels();
}

void ShapeControlWidget::updateParameters(ShapeParameters& params) const {
    params.left = m_leftSlider->value();
    params.right = m_rightSlider->value();
    params.top = m_topSlider->value();
    params.bottom = m_bottomSlider->value();
    params.middle = m_middleSlider->value();
    params.fingerType = static_cast<FingerType>(m_fingerTypeCombo->currentData().toInt());
}

void ShapeControlWidget::onSliderChanged() {
    updateLabels();
    emit parametersChanged();
}

void ShapeControlWidget::updateLabels() {
    m_leftLabel->setText(QString::number(m_leftSlider->value()));
    m_rightLabel->setText(QString::number(m_rightSlider->value()));
    m_topLabel->setText(QString::number(m_topSlider->value()));
    m_bottomLabel->setText(QString::number(m_bottomSlider->value()));
    m_middleLabel->setText(QString::number(m_middleSlider->value()));
}

}
