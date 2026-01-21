#ifndef SINGULAR_POINTS_H
#define SINGULAR_POINTS_H

#include <vector>
#include <random>
#include "fingerprint_parameters.h"

namespace SFinGe {

struct SingularPoint {
    double x;
    double y;
};

class SingularPoints {
public:
    SingularPoints();
    
    void reseed();
    
    void addCore(double x, double y);
    void addDelta(double x, double y);
    
    void updateCore(int index, double x, double y);
    void updateDelta(int index, double x, double y);
    
    void clear();
    void clearCores();
    void clearDeltas();
    
    const std::vector<SingularPoint>& getCores() const { return m_cores; }
    const std::vector<SingularPoint>& getDeltas() const { return m_deltas; }
    
    int getCoreCount() const { return static_cast<int>(m_cores.size()); }
    int getDeltaCount() const { return static_cast<int>(m_deltas.size()); }
    
    void generateRandomPoints(FingerprintClass fpClass, int width, int height);
    void suggestPoints(FingerprintClass fpClass, int width, int height);

private:
    std::vector<SingularPoint> m_cores;
    std::vector<SingularPoint> m_deltas;
    std::mt19937 m_rng;
};

} // namespace SFinGe

#endif // SINGULAR_POINTS_H
