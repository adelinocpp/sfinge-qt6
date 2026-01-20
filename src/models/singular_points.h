#ifndef SINGULAR_POINTS_H
#define SINGULAR_POINTS_H

#include <vector>
#include <QJsonObject>
#include <QJsonArray>
#include "fingerprint_parameters.h"

namespace SFinGe {

struct SingularPoint {
    double x;
    double y;
    
    SingularPoint(double px = 0, double py = 0) : x(px), y(py) {}
};

class SingularPoints {
public:
    SingularPoints();
    
    void addCore(double x, double y);
    void addDelta(double x, double y);
    
    void removeCore(int index);
    void removeDelta(int index);
    
    void clearCores();
    void clearDeltas();
    void clearAll();
    
    const std::vector<SingularPoint>& getCores() const { return m_cores; }
    const std::vector<SingularPoint>& getDeltas() const { return m_deltas; }
    
    SingularPoint getCore(int index) const;
    SingularPoint getDelta(int index) const;
    
    int getCoreCount() const { return m_cores.size(); }
    int getDeltaCount() const { return m_deltas.size(); }
    
    // Geração automática de pontos baseado no tipo de impressão
    void generateRandomPoints(FingerprintClass fpClass, int width, int height);
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
private:
    std::vector<SingularPoint> m_cores;
    std::vector<SingularPoint> m_deltas;
};

}

#endif
