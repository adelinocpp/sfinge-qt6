#ifndef ORIENTATION_SMOOTHER_H
#define ORIENTATION_SMOOTHER_H

#include <vector>

namespace SFinGe {

class OrientationSmoother {
public:
    OrientationSmoother();
    
    void setOrientationMap(const std::vector<double>& orientationMap, int width, int height);
    void setLegendreOrder(int order);
    void setSingularPoints(const std::vector<std::pair<int,int>>& points);
    
    std::vector<double> smoothLegendre();
    std::vector<double> smoothAdaptiveLegendre();
    
private:
    double legendrePolynomial(int n, double x) const;
    double evaluateLegendreField(int i, int j, const std::vector<double>& coeffs, int order) const;
    std::vector<double> fitLegendreCoefficients(int order) const;
    
    int m_width;
    int m_height;
    int m_order;
    std::vector<double> m_orientationMap;
    std::vector<std::pair<int,int>> m_singularPoints;
};

}

#endif
