#include "fomfe_orientation_generator.h"
#include <cmath>
#include <QDebug>
#include <QPainter>
#include <algorithm>

namespace SFinGe {

FOMFEOrientationGenerator::FOMFEOrientationGenerator()
    : m_width(0), m_height(0), m_M(5), m_N(5), m_omega_x(0), m_omega_y(0) {
}

void FOMFEOrientationGenerator::setSize(int width, int height) {
    m_width = width;
    m_height = height;
    
    double l = width / 2.0;
    double h = height / 2.0;
    m_omega_x = M_PI / l;
    m_omega_y = M_PI / h;
    
    qDebug() << "[FOMFE] Dimensões:" << m_width << "x" << m_height;
    qDebug() << "[FOMFE] Frequências fundamentais: ωx =" << m_omega_x << ", ωy =" << m_omega_y;
}

void FOMFEOrientationGenerator::setObservedOrientation(const std::vector<double>& observedMap) {
    m_observedMap = observedMap;
}

void FOMFEOrientationGenerator::setExpansionOrder(int M, int N) {
    m_M = M;
    m_N = N;
    qDebug() << "[FOMFE] Ordem de expansão: M =" << m_M << ", N =" << m_N;
}

double FOMFEOrientationGenerator::computeBasisFunction(int m, int n, int x, int y, int component) const {
    double cx = m_width / 2.0;
    double cy = m_height / 2.0;
    double xi = (x - cx);
    double eta = (y - cy);
    
    double cos_mx = std::cos(m * m_omega_x * xi);
    double sin_mx = std::sin(m * m_omega_x * xi);
    double cos_ny = std::cos(n * m_omega_y * eta);
    double sin_ny = std::sin(n * m_omega_y * eta);
    
    switch(component) {
        case 0: return cos_mx * cos_ny; // a_mn
        case 1: return cos_mx * sin_ny; // b_mn
        case 2: return sin_mx * cos_ny; // c_mn
        case 3: return sin_mx * sin_ny; // d_mn
        default: return 0.0;
    }
}

void FOMFEOrientationGenerator::fitCoefficients() {
    qDebug() << "[FOMFE] Iniciando fitting de coeficientes...";
    
    int numCoeffs = (m_M + 1) * (m_N + 1) * 4;
    m_coefficients.resize(numCoeffs, 0.0);
    
    // Método de mínimos quadrados simplificado
    // Para cada coeficiente, calcular integral discreta
    
    int coeffIdx = 0;
    for (int m = 0; m <= m_M; ++m) {
        for (int n = 0; n <= m_N; ++n) {
            for (int comp = 0; comp < 4; ++comp) {
                double sum = 0.0;
                int count = 0;
                
                // Amostragem esparsa para performance (1 a cada 4 pixels)
                for (int j = 0; j < m_height; j += 4) {
                    for (int i = 0; i < m_width; i += 4) {
                        int idx = j * m_width + i;
                        double basis = computeBasisFunction(m, n, i, j, comp);
                        double observed = m_observedMap[idx];
                        sum += observed * basis;
                        count++;
                    }
                }
                
                m_coefficients[coeffIdx] = sum / count;
                coeffIdx++;
            }
        }
    }
    
    qDebug() << "[FOMFE] Fitting concluído. Total de coeficientes:" << numCoeffs;
    
    // Gerar mapa ajustado
    m_fittedMap.resize(m_width * m_height);
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            m_fittedMap[j * m_width + i] = evaluateAt(i, j);
        }
    }
}

double FOMFEOrientationGenerator::evaluateAt(int x, int y) const {
    double theta = 0.0;
    int coeffIdx = 0;
    
    for (int m = 0; m <= m_M; ++m) {
        for (int n = 0; n <= m_N; ++n) {
            for (int comp = 0; comp < 4; ++comp) {
                double basis = computeBasisFunction(m, n, x, y, comp);
                theta += m_coefficients[coeffIdx] * basis;
                coeffIdx++;
            }
        }
    }
    
    // Normalizar para [0, π)
    while (theta < 0) theta += M_PI;
    while (theta >= M_PI) theta -= M_PI;
    
    return theta;
}

std::vector<double> FOMFEOrientationGenerator::getOrientationMap() const {
    return m_fittedMap;
}

QImage FOMFEOrientationGenerator::generateVisualization() const {
    QImage image(m_width, m_height, QImage::Format_RGB32);
    image.fill(Qt::white);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPen pen(Qt::blue);
    pen.setWidth(1);
    painter.setPen(pen);
    
    int spacing = 12;
    int lineLength = 10;
    
    for (int j = spacing/2; j < m_height; j += spacing) {
        for (int i = spacing/2; i < m_width; i += spacing) {
            double theta = m_fittedMap[j * m_width + i];
            
            double x1 = i - lineLength * std::cos(theta) / 2.0;
            double y1 = j - lineLength * std::sin(theta) / 2.0;
            double x2 = i + lineLength * std::cos(theta) / 2.0;
            double y2 = j + lineLength * std::sin(theta) / 2.0;
            
            painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }
    }
    
    painter.end();
    return image;
}

}
