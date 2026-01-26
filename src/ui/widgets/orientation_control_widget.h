#ifndef ORIENTATION_CONTROL_WIDGET_H
#define ORIENTATION_CONTROL_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QCheckBox>
#include <QComboBox>
#include "models/singular_points.h"
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class OrientationControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit OrientationControlWidget(QWidget *parent = nullptr);
    
    void setSingularPoints(const SingularPoints& points);
    void setImageDimensions(int width, int height);
    SingularPoints getSingularPoints() const;
    FingerprintClass getCurrentClass() const;
    void setCurrentClass(FingerprintClass fpClass);
    void updateSingularPoints(SingularPoints& points) const;
    void updateOrientationParameters(OrientationParameters& params) const;
    void updateMinutiaeParameters(MinutiaeParameters& params) const;

signals:
    void pointsChanged();
    void parametersChanged();

private slots:
    void onAddCore();
    void onRemoveCore();
    void onAddDelta();
    void onRemoveDelta();
    void onSuggestPoints();
    void onAdvancedParameterChanged();
    void onClassChanged(int index);

private:
    void setupUi();
    void setupDynamicPanel();
    void updateCoreLists();
    void updateDeltaLists();
    void updateDynamicPanel(FingerprintClass fpClass);
    
    SingularPoints m_points;
    int m_imageWidth;
    int m_imageHeight;
    
    QListWidget* m_coreList;
    QListWidget* m_deltaList;
    QPushButton* m_addCoreBtn;
    QPushButton* m_removeCoreBtn;
    QPushButton* m_addDeltaBtn;
    QPushButton* m_removeDeltaBtn;
    QComboBox* m_fpClassCombo;
    QPushButton* m_suggestBtn;
    
    // Painel dinâmico de parâmetros
    QGroupBox* m_dynamicGroup;
    QStackedWidget* m_paramStack;
    
    // Parâmetros Arch
    QDoubleSpinBox* m_archAmplitude;
    
    // Parâmetros Tented Arch
    QDoubleSpinBox* m_tentedArchAmplitude;
    QDoubleSpinBox* m_tentedArchDecay;
    
    // Parâmetros Loop
    QDoubleSpinBox* m_loopEdgeBlend;
    
    // Parâmetros Whorl
    QDoubleSpinBox* m_whorlSpiral;
    QDoubleSpinBox* m_whorlDecay;
    
    // Parâmetros Twin Loop
    QDoubleSpinBox* m_twinLoopSmoothing;
    
    // Parâmetros Central Pocket
    QDoubleSpinBox* m_centralPocketConcentration;
    
    // Parâmetros Accidental
    QDoubleSpinBox* m_accidentalIrregularity;
    
    // Minutiae Control
    QCheckBox* m_continuousPhaseCheckBox;
    QSlider* m_phaseNoiseSlider;
    QLabel* m_phaseNoiseLabel;
    QCheckBox* m_qualityMaskCheckBox;
    QComboBox* m_minutiaeDensityCombo;
    QSlider* m_coherenceThresholdSlider;
    QLabel* m_coherenceThresholdLabel;
    QSlider* m_qualityWindowSizeSlider;
    QLabel* m_qualityWindowSizeLabel;
    QSlider* m_frequencySmoothSlider;
    QLabel* m_frequencySmoothLabel;
    
private slots:
    void onCoreSelectionChanged();
    void onDeltaSelectionChanged();
};

}

#endif
