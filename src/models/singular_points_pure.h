#ifndef SINGULAR_POINTS_PURE_H
#define SINGULAR_POINTS_PURE_H

#include <vector>
#include "fingerprint_parameters_pure.h"

struct CorePoint {
    double x;
    double y;
    double alpha;
    double influenceRadius;
    double decayExponent;
};

struct DeltaPoint {
    double x;
    double y;
    double alpha;
    double influenceRadius;
    double decayExponent;
};

class SingularPointsPure {
public:
    SingularPointsPure();
    ~SingularPointsPure();
    
    void clear();
    
    // Core points
    void addCore(double x, double y, double alpha = 1.0, double influenceRadius = 0.0, double decayExponent = 1.0);
    int getCoreCount() const;
    CorePoint getCore(int index) const;
    void updateCoreAlpha(int index, double newAlpha);
    
    // Delta points
    void addDelta(double x, double y, double alpha = -1.0, double influenceRadius = 0.0, double decayExponent = 1.0);
    int getDeltaCount() const;
    DeltaPoint getDelta(int index) const;
    void updateDeltaAlpha(int index, double newAlpha);
    
    // Generation
    void generateRandomPoints(FingerprintClass fingerprintClass, int width, int height);
    
private:
    std::vector<CorePoint> m_cores;
    std::vector<DeltaPoint> m_deltas;
    
    void generateLoopPoints(int width, int height);
    void generateWhorlPoints(int width, int height);
    void generateArchPoints(int width, int height);
    void generateTwinLoopPoints(int width, int height);
};

#endif
