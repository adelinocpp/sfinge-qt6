#include "orientation_control_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QComboBox>

namespace SFinGe {

OrientationControlWidget::OrientationControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_imageWidth(500)
    , m_imageHeight(600) {
    setupUi();
}

void OrientationControlWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* suggestGroup = new QGroupBox(tr("Auto Suggest"), this);
    QVBoxLayout* suggestLayout = new QVBoxLayout(suggestGroup);
    
    QHBoxLayout* suggestControlLayout = new QHBoxLayout();
    QComboBox* fpClassCombo = new QComboBox(this);
    fpClassCombo->addItem(tr("Arch"), static_cast<int>(FingerprintClass::Arch));
    fpClassCombo->addItem(tr("Tented Arch"), static_cast<int>(FingerprintClass::TentedArch));
    fpClassCombo->addItem(tr("Left Loop"), static_cast<int>(FingerprintClass::LeftLoop));
    fpClassCombo->addItem(tr("Right Loop"), static_cast<int>(FingerprintClass::RightLoop));
    fpClassCombo->addItem(tr("Whorl"), static_cast<int>(FingerprintClass::Whorl));
    fpClassCombo->addItem(tr("Twin Loop"), static_cast<int>(FingerprintClass::TwinLoop));
    fpClassCombo->setCurrentIndex(3);
    m_fpClassCombo = fpClassCombo;
    
    QPushButton* suggestBtn = new QPushButton(tr("Suggest Points"), this);
    m_suggestBtn = suggestBtn;
    suggestControlLayout->addWidget(fpClassCombo);
    suggestControlLayout->addWidget(suggestBtn);
    suggestLayout->addLayout(suggestControlLayout);
    
    mainLayout->addWidget(suggestGroup);
    
    QGroupBox* coreGroup = new QGroupBox(tr("Core Points"), this);
    QVBoxLayout* coreLayout = new QVBoxLayout(coreGroup);
    m_coreList = new QListWidget(this);
    coreLayout->addWidget(m_coreList);
    
    QHBoxLayout* coreButtonLayout = new QHBoxLayout();
    m_addCoreBtn = new QPushButton(tr("Add"), this);
    m_removeCoreBtn = new QPushButton(tr("Remove"), this);
    coreButtonLayout->addWidget(m_addCoreBtn);
    coreButtonLayout->addWidget(m_removeCoreBtn);
    coreLayout->addLayout(coreButtonLayout);
    
    QGroupBox* deltaGroup = new QGroupBox(tr("Delta Points"), this);
    QVBoxLayout* deltaLayout = new QVBoxLayout(deltaGroup);
    m_deltaList = new QListWidget(this);
    deltaLayout->addWidget(m_deltaList);
    
    QHBoxLayout* deltaButtonLayout = new QHBoxLayout();
    m_addDeltaBtn = new QPushButton(tr("Add"), this);
    m_removeDeltaBtn = new QPushButton(tr("Remove"), this);
    deltaButtonLayout->addWidget(m_addDeltaBtn);
    deltaButtonLayout->addWidget(m_removeDeltaBtn);
    deltaLayout->addLayout(deltaButtonLayout);
    
    mainLayout->addWidget(coreGroup);
    mainLayout->addWidget(deltaGroup);
    
    // Controles de ajuste fino
    QGroupBox* advancedGroup = new QGroupBox(tr("Advanced Controls"), this);
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedGroup);
    
    // Core Convergence Strength
    QHBoxLayout* convStrengthLayout = new QHBoxLayout();
    QLabel* convLabel = new QLabel(tr("Convergence:"), this);
    convLabel->setMinimumWidth(90);
    convStrengthLayout->addWidget(convLabel);
    m_convStrengthSlider = new QSlider(Qt::Horizontal, this);
    m_convStrengthSlider->setRange(0, 100);
    m_convStrengthSlider->setValue(50);  // 50/100 = 0.5
    m_convStrengthLabel = new QLabel("0.00", this);
    m_convStrengthLabel->setMinimumWidth(35);
    convStrengthLayout->addWidget(m_convStrengthSlider, 1);
    convStrengthLayout->addWidget(m_convStrengthLabel);
    advancedLayout->addLayout(convStrengthLayout);
    
    // Core Convergence Radius
    QHBoxLayout* convRadiusLayout = new QHBoxLayout();
    QLabel* radiusLabel = new QLabel(tr("Radius:"), this);
    radiusLabel->setMinimumWidth(90);
    convRadiusLayout->addWidget(radiusLabel);
    m_convRadiusSlider = new QSlider(Qt::Horizontal, this);
    m_convRadiusSlider->setRange(20, 100);
    m_convRadiusSlider->setValue(50);
    m_convRadiusLabel = new QLabel("50", this);
    m_convRadiusLabel->setMinimumWidth(35);
    convRadiusLayout->addWidget(m_convRadiusSlider, 1);
    convRadiusLayout->addWidget(m_convRadiusLabel);
    advancedLayout->addLayout(convRadiusLayout);
    
    // Core Convergence Probability
    QHBoxLayout* convProbLayout = new QHBoxLayout();
    QLabel* probLabel = new QLabel(tr("Probability:"), this);
    probLabel->setMinimumWidth(90);
    convProbLayout->addWidget(probLabel);
    m_convProbSlider = new QSlider(Qt::Horizontal, this);
    m_convProbSlider->setRange(0, 100);
    m_convProbSlider->setValue(30);
    m_convProbLabel = new QLabel("0.30", this);
    m_convProbLabel->setMinimumWidth(35);
    convProbLayout->addWidget(m_convProbSlider, 1);
    convProbLayout->addWidget(m_convProbLabel);
    advancedLayout->addLayout(convProbLayout);
    
    // Vertical Bias Strength
    QHBoxLayout* biasStrengthLayout = new QHBoxLayout();
    QLabel* biasLabel = new QLabel(tr("Vertical Bias:"), this);
    biasLabel->setMinimumWidth(90);
    biasStrengthLayout->addWidget(biasLabel);
    m_biasStrengthSlider = new QSlider(Qt::Horizontal, this);
    m_biasStrengthSlider->setRange(0, 100);
    m_biasStrengthSlider->setValue(0);
    m_biasStrengthLabel = new QLabel("0.00", this);
    m_biasStrengthLabel->setMinimumWidth(35);
    biasStrengthLayout->addWidget(m_biasStrengthSlider, 1);
    biasStrengthLayout->addWidget(m_biasStrengthLabel);
    advancedLayout->addLayout(biasStrengthLayout);
    
    // Vertical Bias Radius
    QHBoxLayout* biasRadiusLayout = new QHBoxLayout();
    QLabel* biasRadiusLabel = new QLabel(tr("Bias Radius:"), this);
    biasRadiusLabel->setMinimumWidth(90);
    biasRadiusLayout->addWidget(biasRadiusLabel);
    m_biasRadiusSlider = new QSlider(Qt::Horizontal, this);
    m_biasRadiusSlider->setRange(40, 150);
    m_biasRadiusSlider->setValue(80);
    m_biasRadiusLabel = new QLabel("80", this);
    m_biasRadiusLabel->setMinimumWidth(35);
    biasRadiusLayout->addWidget(m_biasRadiusSlider, 1);
    biasRadiusLayout->addWidget(m_biasRadiusLabel);
    advancedLayout->addLayout(biasRadiusLayout);
    
    mainLayout->addWidget(advancedGroup);
    mainLayout->addStretch();
    
    connect(m_addCoreBtn, &QPushButton::clicked, this, &OrientationControlWidget::onAddCore);
    connect(m_removeCoreBtn, &QPushButton::clicked, this, &OrientationControlWidget::onRemoveCore);
    connect(m_addDeltaBtn, &QPushButton::clicked, this, &OrientationControlWidget::onAddDelta);
    connect(m_removeDeltaBtn, &QPushButton::clicked, this, &OrientationControlWidget::onRemoveDelta);
    connect(m_suggestBtn, &QPushButton::clicked, this, &OrientationControlWidget::onSuggestPoints);
    
    connect(m_convStrengthSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_convRadiusSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_convProbSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_biasStrengthSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_biasRadiusSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
}

void OrientationControlWidget::onAdvancedParameterChanged() {
    m_convStrengthLabel->setText(QString::number(m_convStrengthSlider->value() / 100.0, 'f', 2));
    m_convRadiusLabel->setText(QString::number(m_convRadiusSlider->value()));
    m_convProbLabel->setText(QString::number(m_convProbSlider->value() / 100.0, 'f', 2));
    m_biasStrengthLabel->setText(QString::number(m_biasStrengthSlider->value() / 100.0, 'f', 2));
    m_biasRadiusLabel->setText(QString::number(m_biasRadiusSlider->value()));
    
        
    emit parametersChanged();
}

void OrientationControlWidget::updateOrientationParameters(OrientationParameters& params) const {
    params.coreConvergenceStrength = m_convStrengthSlider->value() / 100.0;
    params.coreConvergenceRadius = m_convRadiusSlider->value();
    params.coreConvergenceProbability = m_convProbSlider->value() / 100.0;
    params.verticalBiasStrength = m_biasStrengthSlider->value() / 100.0;
    params.verticalBiasRadius = m_biasRadiusSlider->value();
}

void OrientationControlWidget::setSingularPoints(const SingularPoints& points) {
    m_points = points;
    updateCoreLists();
    updateDeltaLists();
}

void OrientationControlWidget::updateSingularPoints(SingularPoints& points) const {
    points = m_points;
}

SingularPoints OrientationControlWidget::getSingularPoints() const {
    return m_points;
}

void OrientationControlWidget::setImageDimensions(int width, int height) {
    m_imageWidth = width;
    m_imageHeight = height;
}

void OrientationControlWidget::onAddCore() {
    bool ok;
    double x = QInputDialog::getDouble(this, tr("Add Core"), tr("X coordinate:"), 75.5, 0, 1000, 1, &ok);
    if (ok) {
        double y = QInputDialog::getDouble(this, tr("Add Core"), tr("Y coordinate:"), 100.5, 0, 1000, 1, &ok);
        if (ok) {
            m_points.addCore(x, y);
            updateCoreLists();
            emit pointsChanged();
        }
    }
}

void OrientationControlWidget::onRemoveCore() {
    int currentRow = m_coreList->currentRow();
    if (currentRow >= 0) {
        m_points.removeCore(currentRow);
        updateCoreLists();
        emit pointsChanged();
    }
}

void OrientationControlWidget::onAddDelta() {
    bool ok;
    double x = QInputDialog::getDouble(this, tr("Add Delta"), tr("X coordinate:"), 130.5, 0, 1000, 1, &ok);
    if (ok) {
        double y = QInputDialog::getDouble(this, tr("Add Delta"), tr("Y coordinate:"), 187.5, 0, 1000, 1, &ok);
        if (ok) {
            m_points.addDelta(x, y);
            updateDeltaLists();
            emit pointsChanged();
        }
    }
}

void OrientationControlWidget::onRemoveDelta() {
    int currentRow = m_deltaList->currentRow();
    if (currentRow >= 0) {
        m_points.removeDelta(currentRow);
        updateDeltaLists();
        emit pointsChanged();
    }
}

void OrientationControlWidget::updateCoreLists() {
    m_coreList->clear();
    for (int i = 0; i < m_points.getCoreCount(); ++i) {
        SingularPoint core = m_points.getCore(i);
        m_coreList->addItem(QString("Core %1: (%2, %3)")
                           .arg(i + 1)
                           .arg(core.x, 0, 'f', 1)
                           .arg(core.y, 0, 'f', 1));
    }
}

void OrientationControlWidget::updateDeltaLists() {
    m_deltaList->clear();
    for (int i = 0; i < m_points.getDeltaCount(); ++i) {
        SingularPoint delta = m_points.getDelta(i);
        m_deltaList->addItem(QString("Delta %1: (%2, %3)")
                            .arg(i + 1)
                            .arg(delta.x, 0, 'f', 1)
                            .arg(delta.y, 0, 'f', 1));
    }
}

void OrientationControlWidget::onSuggestPoints() {
    int classIndex = m_fpClassCombo->currentData().toInt();
    FingerprintClass fpClass = static_cast<FingerprintClass>(classIndex);
    
    m_points.clearAll();
    
    qDebug() << "[OrientationControlWidget] Suggesting points for" << m_imageWidth << "x" << m_imageHeight;
    m_points.generateRandomPoints(fpClass, m_imageWidth, m_imageHeight);
    
    updateCoreLists();
    updateDeltaLists();
    emit pointsChanged();
}

FingerprintClass OrientationControlWidget::getCurrentClass() const {
    int classIndex = m_fpClassCombo->currentData().toInt();
    return static_cast<FingerprintClass>(classIndex);
}

void OrientationControlWidget::setCurrentClass(FingerprintClass fpClass) {
    for (int i = 0; i < m_fpClassCombo->count(); ++i) {
        if (m_fpClassCombo->itemData(i).toInt() == static_cast<int>(fpClass)) {
            m_fpClassCombo->setCurrentIndex(i);
            break;
        }
    }
}

}
