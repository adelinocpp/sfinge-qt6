#ifndef SHAPE_CONTROL_WIDGET_H
#define SHAPE_CONTROL_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class ShapeControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit ShapeControlWidget(QWidget *parent = nullptr);
    
    void setParameters(const ShapeParameters& params);
    void updateParameters(ShapeParameters& params) const;

signals:
    void parametersChanged();

private slots:
    void onSliderChanged();

private:
    void setupUi();
    void updateLabels();
    
    QSlider* m_leftSlider;
    QSlider* m_rightSlider;
    QSlider* m_topSlider;
    QSlider* m_bottomSlider;
    QSlider* m_middleSlider;
    QComboBox* m_fingerTypeCombo;
    
    QLabel* m_leftLabel;
    QLabel* m_rightLabel;
    QLabel* m_topLabel;
    QLabel* m_bottomLabel;
    QLabel* m_middleLabel;
};

}

#endif
