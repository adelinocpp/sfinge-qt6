#ifndef FOMFE_ORIENTATION_GENERATOR_H
#define FOMFE_ORIENTATION_GENERATOR_H

#include <vector>
#include <QImage>

namespace SFinGe {

class FOMFEOrientationGenerator {
public:
    FOMFEOrientationGenerator();
    
    void setSize(int width, int height);
    void setObservedOrientation(const std::vector<double>& observedMap);
    void setExpansionOrder(int M, int N);
    
    void fitCoefficients();
    
    std::vector<double> getOrientationMap() const;
    QImage generateVisualization() const;
    
private:
    double evaluateAt(int x, int y) const;
    double computeBasisFunction(int m, int n, int x, int y, int component) const;
    
    int m_width;
    int m_height;
    int m_M; // ordem em x
    int m_N; // ordem em y
    
    double m_omega_x;
    double m_omega_y;
    
    std::vector<double> m_observedMap;
    std::vector<double> m_coefficients; // a_mn, b_mn, c_mn, d_mn
    std::vector<double> m_fittedMap;
};

}

#endif
