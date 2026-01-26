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
    fpClassCombo->addItem(tr("Central Pocket"), static_cast<int>(FingerprintClass::CentralPocket));
    fpClassCombo->addItem(tr("Accidental"), static_cast<int>(FingerprintClass::Accidental));
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
    
    // Painel dinâmico de parâmetros
    setupDynamicPanel();
    mainLayout->addWidget(m_dynamicGroup);
    
    // Minutiae Control Group
    QGroupBox* minutiaeGroup = new QGroupBox(tr("Minutiae Control"), this);
    QVBoxLayout* minutiaeLayout = new QVBoxLayout(minutiaeGroup);
    
    // Toggle para método melhorado
    QHBoxLayout* methodLayout = new QHBoxLayout();
    m_continuousPhaseCheckBox = new QCheckBox(tr("Use Continuous Phase (Improved Method)"), this);
    m_continuousPhaseCheckBox->setChecked(false);  // Padrão: método original
    methodLayout->addWidget(m_continuousPhaseCheckBox);
    minutiaeLayout->addLayout(methodLayout);
    
    // Phase Noise Control
    QHBoxLayout* noiseLayout = new QHBoxLayout();
    noiseLayout->addWidget(new QLabel(tr("Phase Noise Level:"), this));
    m_phaseNoiseSlider = new QSlider(Qt::Horizontal, this);
    m_phaseNoiseSlider->setRange(0, 50);  // 0% a 50%
    m_phaseNoiseSlider->setValue(10);  // 10% padrão
    m_phaseNoiseLabel = new QLabel("10%", this);
    noiseLayout->addWidget(m_phaseNoiseSlider);
    noiseLayout->addWidget(m_phaseNoiseLabel);
    minutiaeLayout->addLayout(noiseLayout);
    
    // Quality Mask Control
    QHBoxLayout* qualityLayout = new QHBoxLayout();
    m_qualityMaskCheckBox = new QCheckBox(tr("Use Quality Mask"), this);
    m_qualityMaskCheckBox->setChecked(true);
    qualityLayout->addWidget(m_qualityMaskCheckBox);
    minutiaeLayout->addLayout(qualityLayout);
    
    // Minutiae Density Control
    QHBoxLayout* densityLayout = new QHBoxLayout();
    densityLayout->addWidget(new QLabel(tr("Minutiae Density:"), this));
    m_minutiaeDensityCombo = new QComboBox(this);
    m_minutiaeDensityCombo->addItem("Low (25-30)");
    m_minutiaeDensityCombo->addItem("Medium (40-50)");
    m_minutiaeDensityCombo->addItem("High (60-80)");
    m_minutiaeDensityCombo->setCurrentIndex(0);
    densityLayout->addWidget(m_minutiaeDensityCombo);
    densityLayout->addStretch();
    minutiaeLayout->addLayout(densityLayout);
    
    // Coherence Threshold
    QHBoxLayout* coherenceLayout = new QHBoxLayout();
    coherenceLayout->addWidget(new QLabel(tr("Coherence Threshold:"), this));
    m_coherenceThresholdSlider = new QSlider(Qt::Horizontal, this);
    m_coherenceThresholdSlider->setRange(20, 80);  // 0.2 a 0.8
    m_coherenceThresholdSlider->setValue(50);  // 0.5 padrão
    m_coherenceThresholdLabel = new QLabel("0.5", this);
    coherenceLayout->addWidget(m_coherenceThresholdSlider);
    coherenceLayout->addWidget(m_coherenceThresholdLabel);
    minutiaeLayout->addLayout(coherenceLayout);
    
    // Quality Window Size
    QHBoxLayout* windowLayout = new QHBoxLayout();
    windowLayout->addWidget(new QLabel(tr("Quality Window Size:"), this));
    m_qualityWindowSizeSlider = new QSlider(Qt::Horizontal, this);
    m_qualityWindowSizeSlider->setRange(8, 32);
    m_qualityWindowSizeSlider->setValue(16);
    m_qualityWindowSizeLabel = new QLabel("16px", this);
    windowLayout->addWidget(m_qualityWindowSizeSlider);
    windowLayout->addWidget(m_qualityWindowSizeLabel);
    minutiaeLayout->addLayout(windowLayout);
    
    // Frequency Smooth Sigma
    QHBoxLayout* smoothLayout = new QHBoxLayout();
    smoothLayout->addWidget(new QLabel(tr("Frequency Smooth Sigma:"), this));
    m_frequencySmoothSlider = new QSlider(Qt::Horizontal, this);
    m_frequencySmoothSlider->setRange(5, 20);
    m_frequencySmoothSlider->setValue(10);
    m_frequencySmoothLabel = new QLabel("10.0", this);
    smoothLayout->addWidget(m_frequencySmoothSlider);
    smoothLayout->addWidget(m_frequencySmoothLabel);
    minutiaeLayout->addLayout(smoothLayout);
    
    mainLayout->addWidget(minutiaeGroup);
    mainLayout->addStretch();
    
    connect(m_addCoreBtn, &QPushButton::clicked, this, &OrientationControlWidget::onAddCore);
    connect(m_removeCoreBtn, &QPushButton::clicked, this, &OrientationControlWidget::onRemoveCore);
    connect(m_addDeltaBtn, &QPushButton::clicked, this, &OrientationControlWidget::onAddDelta);
    connect(m_removeDeltaBtn, &QPushButton::clicked, this, &OrientationControlWidget::onRemoveDelta);
    connect(m_suggestBtn, &QPushButton::clicked, this, &OrientationControlWidget::onSuggestPoints);
    connect(m_fpClassCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &OrientationControlWidget::onClassChanged);
    
    // Conectar sinais dos controles de minutiae
    connect(m_continuousPhaseCheckBox, &QCheckBox::toggled, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_phaseNoiseSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_qualityMaskCheckBox, &QCheckBox::toggled, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_minutiaeDensityCombo, &QComboBox::currentTextChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_coherenceThresholdSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_qualityWindowSizeSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_frequencySmoothSlider, &QSlider::valueChanged, this, &OrientationControlWidget::onAdvancedParameterChanged);
    
    // Conexões para seleção de pontos
    connect(m_coreList, &QListWidget::currentRowChanged, this, &OrientationControlWidget::onCoreSelectionChanged);
    connect(m_deltaList, &QListWidget::currentRowChanged, this, &OrientationControlWidget::onDeltaSelectionChanged);
    
    // Inicializar painel para a classe selecionada
    onClassChanged(m_fpClassCombo->currentIndex());
}

void OrientationControlWidget::setupDynamicPanel() {
    m_dynamicGroup = new QGroupBox(tr("Parameters"), this);
    QVBoxLayout* dynamicLayout = new QVBoxLayout(m_dynamicGroup);
    
    m_paramStack = new QStackedWidget(this);
    
    // Página 0: Arch
    QWidget* archPage = new QWidget();
    QVBoxLayout* archLayout = new QVBoxLayout(archPage);
    QHBoxLayout* archAmpLayout = new QHBoxLayout();
    archAmpLayout->addWidget(new QLabel(tr("Amplitude:"), this));
    m_archAmplitude = new QDoubleSpinBox(this);
    m_archAmplitude->setRange(-999.0, 999.0);
    m_archAmplitude->setSingleStep(0.05);
    m_archAmplitude->setValue(0.70);  // Valor padrão entre 0.5 e 1.0
    archAmpLayout->addWidget(m_archAmplitude);
    archLayout->addLayout(archAmpLayout);
    archLayout->addStretch();
    m_paramStack->addWidget(archPage);
    
    // Página 1: Tented Arch
    QWidget* tentedPage = new QWidget();
    QVBoxLayout* tentedLayout = new QVBoxLayout(tentedPage);
    QHBoxLayout* tentedAmpLayout = new QHBoxLayout();
    tentedAmpLayout->addWidget(new QLabel(tr("Amplitude:"), this));
    m_tentedArchAmplitude = new QDoubleSpinBox(this);
    m_tentedArchAmplitude->setRange(-999.0, 999.0);
    m_tentedArchAmplitude->setSingleStep(0.01);
    m_tentedArchAmplitude->setValue(0.15);
    tentedAmpLayout->addWidget(m_tentedArchAmplitude);
    tentedLayout->addLayout(tentedAmpLayout);
    
    QHBoxLayout* tentedDecayLayout = new QHBoxLayout();
    tentedDecayLayout->addWidget(new QLabel(tr("Peak Decay:"), this));
    m_tentedArchDecay = new QDoubleSpinBox(this);
    m_tentedArchDecay->setRange(-999.0, 999.0);
    m_tentedArchDecay->setSingleStep(0.01);
    m_tentedArchDecay->setValue(0.06);
    tentedDecayLayout->addWidget(m_tentedArchDecay);
    tentedLayout->addLayout(tentedDecayLayout);
    tentedLayout->addStretch();
    m_paramStack->addWidget(tentedPage);
    
    // Página 2: Left Loop
    QWidget* leftLoopPage = new QWidget();
    QVBoxLayout* leftLoopLayout = new QVBoxLayout(leftLoopPage);
    QHBoxLayout* loopBlendLayout = new QHBoxLayout();
    loopBlendLayout->addWidget(new QLabel(tr("Edge Blend:"), this));
    m_loopEdgeBlend = new QDoubleSpinBox(this);
    m_loopEdgeBlend->setRange(-999.0, 999.0);
    m_loopEdgeBlend->setSingleStep(0.1);
    m_loopEdgeBlend->setValue(0.8);  // Valor padrão maior para efeito visível
    loopBlendLayout->addWidget(m_loopEdgeBlend);
    leftLoopLayout->addLayout(loopBlendLayout);
    leftLoopLayout->addStretch();
    m_paramStack->addWidget(leftLoopPage);
    
    // Página 3: Right Loop - NÃO adiciona widget, usa índice 2 (Left Loop)
    // O mapeamento é feito em updateDynamicPanel()
    
    // Página 4: Whorl
    QWidget* whorlPage = new QWidget();
    QVBoxLayout* whorlLayout = new QVBoxLayout(whorlPage);
    QHBoxLayout* whorlSpiralLayout = new QHBoxLayout();
    whorlSpiralLayout->addWidget(new QLabel(tr("Spiral:"), this));
    m_whorlSpiral = new QDoubleSpinBox(this);
    m_whorlSpiral->setRange(-999.0, 999.0);
    m_whorlSpiral->setSingleStep(0.02);
    m_whorlSpiral->setValue(0.12);
    whorlSpiralLayout->addWidget(m_whorlSpiral);
    whorlLayout->addLayout(whorlSpiralLayout);
    
    QHBoxLayout* whorlDecayLayout = new QHBoxLayout();
    whorlDecayLayout->addWidget(new QLabel(tr("Edge Decay:"), this));
    m_whorlDecay = new QDoubleSpinBox(this);
    m_whorlDecay->setRange(-999.0, 999.0);
    m_whorlDecay->setSingleStep(0.02);
    m_whorlDecay->setValue(0.18);
    whorlDecayLayout->addWidget(m_whorlDecay);
    whorlLayout->addLayout(whorlDecayLayout);
    whorlLayout->addStretch();
    m_paramStack->addWidget(whorlPage);
    
    // Página 5: Twin Loop
    QWidget* twinLoopPage = new QWidget();
    QVBoxLayout* twinLoopLayout = new QVBoxLayout(twinLoopPage);
    QHBoxLayout* twinSmoothLayout = new QHBoxLayout();
    twinSmoothLayout->addWidget(new QLabel(tr("Smoothing:"), this));
    m_twinLoopSmoothing = new QDoubleSpinBox(this);
    m_twinLoopSmoothing->setRange(-999.0, 999.0);
    m_twinLoopSmoothing->setSingleStep(1.0);
    m_twinLoopSmoothing->setValue(7.0);
    twinSmoothLayout->addWidget(m_twinLoopSmoothing);
    twinLoopLayout->addLayout(twinSmoothLayout);
    twinLoopLayout->addStretch();
    m_paramStack->addWidget(twinLoopPage);
    
    // Página 6: Central Pocket
    QWidget* centralPocketPage = new QWidget();
    QVBoxLayout* centralPocketLayout = new QVBoxLayout(centralPocketPage);
    QHBoxLayout* cpConcentrationLayout = new QHBoxLayout();
    cpConcentrationLayout->addWidget(new QLabel(tr("Concentration:"), this));
    m_centralPocketConcentration = new QDoubleSpinBox(this);
    m_centralPocketConcentration->setRange(-999.0, 999.0);
    m_centralPocketConcentration->setSingleStep(0.01);
    m_centralPocketConcentration->setValue(0.06);
    cpConcentrationLayout->addWidget(m_centralPocketConcentration);
    centralPocketLayout->addLayout(cpConcentrationLayout);
    centralPocketLayout->addStretch();
    m_paramStack->addWidget(centralPocketPage);
    
    // Página 7: Accidental
    QWidget* accidentalPage = new QWidget();
    QVBoxLayout* accidentalLayout = new QVBoxLayout(accidentalPage);
    QHBoxLayout* accIrregularityLayout = new QHBoxLayout();
    accIrregularityLayout->addWidget(new QLabel(tr("Irregularity:"), this));
    m_accidentalIrregularity = new QDoubleSpinBox(this);
    m_accidentalIrregularity->setRange(-999.0, 999.0);
    m_accidentalIrregularity->setSingleStep(0.02);
    m_accidentalIrregularity->setValue(0.08);
    accIrregularityLayout->addWidget(m_accidentalIrregularity);
    accidentalLayout->addLayout(accIrregularityLayout);
    accidentalLayout->addStretch();
    m_paramStack->addWidget(accidentalPage);
    
    dynamicLayout->addWidget(m_paramStack);
    
    // Conectar sinais de alteração de parâmetros
    connect(m_archAmplitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_tentedArchAmplitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_tentedArchDecay, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_loopEdgeBlend, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_whorlSpiral, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_whorlDecay, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_twinLoopSmoothing, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_centralPocketConcentration, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
    connect(m_accidentalIrregularity, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OrientationControlWidget::onAdvancedParameterChanged);
}

void OrientationControlWidget::onClassChanged(int index) {
    FingerprintClass fpClass = static_cast<FingerprintClass>(m_fpClassCombo->itemData(index).toInt());
    updateDynamicPanel(fpClass);
}

void OrientationControlWidget::updateDynamicPanel(FingerprintClass fpClass) {
    // Mapear classe para índice da página
    // Nota: Right Loop usa a mesma página do Left Loop (índice 2)
    int pageIndex = 0;
    switch (fpClass) {
        case FingerprintClass::Arch:          pageIndex = 0; break;
        case FingerprintClass::TentedArch:    pageIndex = 1; break;
        case FingerprintClass::LeftLoop:      pageIndex = 2; break;
        case FingerprintClass::RightLoop:     pageIndex = 2; break;
        case FingerprintClass::Whorl:         pageIndex = 3; break;
        case FingerprintClass::TwinLoop:      pageIndex = 4; break;
        case FingerprintClass::CentralPocket: pageIndex = 5; break;
        case FingerprintClass::Accidental:    pageIndex = 6; break;
        default: pageIndex = 0; break;
    }
    m_paramStack->setCurrentIndex(pageIndex);
}

void OrientationControlWidget::onAdvancedParameterChanged() {
    // Atualizar labels dos parâmetros avançados
    m_phaseNoiseLabel->setText(QString::number(m_phaseNoiseSlider->value()) + "%");
    m_coherenceThresholdLabel->setText(QString::number(m_coherenceThresholdSlider->value() / 100.0, 'f', 2));
    m_qualityWindowSizeLabel->setText(QString::number(m_qualityWindowSizeSlider->value()) + "px");
    m_frequencySmoothLabel->setText(QString::number(m_frequencySmoothSlider->value(), 'f', 1));
    
    emit parametersChanged();
}

void OrientationControlWidget::updateOrientationParameters(OrientationParameters& params) const {
    // Atualizar parâmetros com valores do painel dinâmico
    params.archAmplitude = m_archAmplitude->value();
    params.tentedArchPeakInfluenceDecay = m_tentedArchDecay->value();
    params.loopEdgeBlendFactor = m_loopEdgeBlend->value();
    params.whorlSpiralFactor = m_whorlSpiral->value();
    params.whorlEdgeDecayFactor = m_whorlDecay->value();
    params.twinLoopSmoothing = m_twinLoopSmoothing->value();
    params.centralPocketConcentration = m_centralPocketConcentration->value();
    params.accidentalIrregularity = m_accidentalIrregularity->value();
}

void OrientationControlWidget::updateMinutiaeParameters(MinutiaeParameters& params) const {
    // Atualizar parâmetros de minutiae com valores da UI
    params.useContinuousPhase = m_continuousPhaseCheckBox->isChecked();
    params.phaseNoiseLevel = m_phaseNoiseSlider->value() / 100.0;  // Converter para 0.0-1.0
    params.useQualityMask = m_qualityMaskCheckBox->isChecked();
    
    // Mapear densidade
    QString densityText = m_minutiaeDensityCombo->currentText();
    if (densityText.contains("Low")) {
        params.minutiaeDensity = "low";
        params.customBifurcations = 15;
        params.customEndings = 10;
    } else if (densityText.contains("Medium")) {
        params.minutiaeDensity = "medium";
        params.customBifurcations = 25;
        params.customEndings = 20;
    } else if (densityText.contains("High")) {
        params.minutiaeDensity = "high";
        params.customBifurcations = 40;
        params.customEndings = 30;
    }
    
    params.coherenceThreshold = m_coherenceThresholdSlider->value() / 100.0;  // Converter para 0.0-1.0
    params.qualityWindowSize = m_qualityWindowSizeSlider->value();
    params.frequencySmoothSigma = m_frequencySmoothSlider->value();
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

void OrientationControlWidget::onCoreSelectionChanged() {
    // Seleção de core alterada - sem ação necessária
}

void OrientationControlWidget::onDeltaSelectionChanged() {
    // Seleção de delta alterada - sem ação necessária
}

}
