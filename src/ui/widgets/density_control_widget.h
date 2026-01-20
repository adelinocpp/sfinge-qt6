#ifndef DENSITY_CONTROL_WIDGET_H
#define DENSITY_CONTROL_WIDGET_H

#include <QWidget>
#include <QDoubleSpinBox>
#include "models/fingerprint_parameters.h"

namespace SFinGe {

class DensityControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit DensityControlWidget(QWidget *parent = nullptr);
    
    void setParameters(const DensityParameters& params);
    void updateParameters(DensityParameters& params) const;

signals:
    void parametersChanged();

private:
    void setupUi();
    
    QDoubleSpinBox* m_zoomSpin;
    QDoubleSpinBox* m_amplifySpin;
    QDoubleSpinBox* m_minFreqSpin;
    QDoubleSpinBox* m_maxFreqSpin;
};

}

#endif
