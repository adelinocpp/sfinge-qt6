#include "singular_points_pure.h"
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SingularPointsPure::SingularPointsPure() {
}

SingularPointsPure::~SingularPointsPure() {
}

void SingularPointsPure::clear() {
    m_cores.clear();
    m_deltas.clear();
}

void SingularPointsPure::addCore(double x, double y, double alpha, double influenceRadius, double decayExponent) {
    CorePoint core;
    core.x = x;
    core.y = y;
    core.alpha = alpha;
    core.influenceRadius = influenceRadius;
    core.decayExponent = decayExponent;
    m_cores.push_back(core);
}

int SingularPointsPure::getCoreCount() const {
    return static_cast<int>(m_cores.size());
}

CorePoint SingularPointsPure::getCore(int index) const {
    if (index >= 0 && index < static_cast<int>(m_cores.size())) {
        return m_cores[index];
    }
    return CorePoint();
}

void SingularPointsPure::updateCoreAlpha(int index, double newAlpha) {
    if (index >= 0 && index < static_cast<int>(m_cores.size())) {
        m_cores[index].alpha = newAlpha;
    }
}

void SingularPointsPure::addDelta(double x, double y, double alpha, double influenceRadius, double decayExponent) {
    DeltaPoint delta;
    delta.x = x;
    delta.y = y;
    delta.alpha = alpha;
    delta.influenceRadius = influenceRadius;
    delta.decayExponent = decayExponent;
    m_deltas.push_back(delta);
}

int SingularPointsPure::getDeltaCount() const {
    return static_cast<int>(m_deltas.size());
}

DeltaPoint SingularPointsPure::getDelta(int index) const {
    if (index >= 0 && index < static_cast<int>(m_deltas.size())) {
        return m_deltas[index];
    }
    return DeltaPoint();
}

void SingularPointsPure::updateDeltaAlpha(int index, double newAlpha) {
    if (index >= 0 && index < static_cast<int>(m_deltas.size())) {
        m_deltas[index].alpha = newAlpha;
    }
}

void SingularPointsPure::generateRandomPoints(FingerprintClass fingerprintClass, int width, int height) {
    clear();
    
    switch (fingerprintClass) {
        case FingerprintClass::LeftLoop:
        case FingerprintClass::RightLoop:
            generateLoopPoints(width, height);
            break;
        case FingerprintClass::Whorl:
            generateWhorlPoints(width, height);
            break;
        case FingerprintClass::Arch:
            generateArchPoints(width, height);
            break;
        case FingerprintClass::TentedArch:
            generateArchPoints(width, height);
            break;
        case FingerprintClass::TwinLoop:
            generateTwinLoopPoints(width, height);
            break;
    }
}

void SingularPointsPure::generateLoopPoints(int width, int height) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(width * 0.3, width * 0.7);
    std::uniform_int_distribution<> yDist(height * 0.2, height * 0.6);
    std::normal_distribution<> alphaDist(1.0, 0.1);
    
    // Add one core point
    double coreX = xDist(gen);
    double coreY = yDist(gen);
    double coreAlpha = alphaDist(gen);
    addCore(coreX, coreY, coreAlpha, 100.0, 1.0);
    
    // Add one delta point
    double deltaX = coreX + (coreAlpha > 0 ? -50 : 50);
    double deltaY = coreY + 80;
    double deltaAlpha = -1.0;
    addDelta(deltaX, deltaY, deltaAlpha, 80.0, 1.0);
}

void SingularPointsPure::generateWhorlPoints(int width, int height) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(width * 0.3, width * 0.7);
    std::uniform_int_distribution<> yDist(height * 0.3, height * 0.7);
    std::normal_distribution<> alphaDist(1.0, 0.1);
    
    // Add two core points for whorl
    double centerX = xDist(gen);
    double centerY = yDist(gen);
    
    addCore(centerX - 30, centerY, alphaDist(gen), 80.0, 1.0);
    addCore(centerX + 30, centerY, alphaDist(gen), 80.0, 1.0);
    
    // Add two delta points
    addDelta(centerX, centerY - 60, -1.0, 70.0, 1.0);
    addDelta(centerX, centerY + 60, -1.0, 70.0, 1.0);
}

void SingularPointsPure::generateArchPoints(int width, int height) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(width * 0.3, width * 0.7);
    std::uniform_int_distribution<> yDist(height * 0.6, height * 0.8);
    
    // Arch has no core points, just deltas
    double deltaX = xDist(gen);
    double deltaY = yDist(gen);
    addDelta(deltaX - 40, deltaY, -1.0, 60.0, 1.0);
    addDelta(deltaX + 40, deltaY, -1.0, 60.0, 1.0);
}

void SingularPointsPure::generateTwinLoopPoints(int width, int height) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(width * 0.2, width * 0.8);
    std::uniform_int_distribution<> yDist(height * 0.2, height * 0.6);
    std::normal_distribution<> alphaDist(1.0, 0.1);
    
    // Two cores for twin loop
    addCore(xDist(gen) - 40, yDist(gen), alphaDist(gen), 80.0, 1.0);
    addCore(xDist(gen) + 40, yDist(gen), alphaDist(gen), 80.0, 1.0);
    
    // Two deltas
    addDelta(xDist(gen) - 60, yDist(gen) + 60, -1.0, 70.0, 1.0);
    addDelta(xDist(gen) + 60, yDist(gen) + 60, -1.0, 70.0, 1.0);
}
