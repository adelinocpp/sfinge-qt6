#ifndef ORIENTATION_CONTROL_WIDGET_H
#define ORIENTATION_CONTROL_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
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

private:
    void setupUi();
    void updateCoreLists();
    void updateDeltaLists();
    
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
    
    QSlider* m_convStrengthSlider;
    QLabel* m_convStrengthLabel;
    QSlider* m_convRadiusSlider;
    QLabel* m_convRadiusLabel;
    QSlider* m_convProbSlider;
    QLabel* m_convProbLabel;
    QSlider* m_biasStrengthSlider;
    QLabel* m_biasStrengthLabel;
    QSlider* m_biasRadiusSlider;
    QLabel* m_biasRadiusLabel;
};

}

#endif
