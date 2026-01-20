#ifndef FINGERPRINT_PARAMETERS_H
#define FINGERPRINT_PARAMETERS_H

#include <QString>
#include <QJsonObject>

namespace SFinGe {

enum class FingerType {
    Thumb = 0,
    Index = 1,
    Middle = 2,
    Ring = 3,
    Little = 4
};

enum class FingerprintClass {
    None = 0,
    Arch = 1,           // Arco: sem core, sem delta
    TentedArch = 2,     // Arco Tendido: 1 core, sem delta
    LeftLoop = 3,       // Presilha Esquerda: 1 core, 1 delta
    RightLoop = 4,      // Presilha Direita: 1 core, 1 delta
    Whorl = 5,          // Verticílio: 2 cores, 2 deltas
    TwinLoop = 6        // Loop Duplo: 2 cores, 2 deltas
};

struct ShapeParameters {
    int left = 50;
    int right = 50;
    int top = 50;
    int bottom = 50;
    int middle = 50;
    FingerType fingerType = FingerType::Index;
};

struct DensityParameters {
    float minFrequency = 1.0f / 13.0f;
    float maxFrequency = 1.0f / 9.0f;
    double zoom = 1.0;
    double amplify = 0.5;
};

enum class OrientationMethod {
    Poincare = 0,       // Poincaré Index clássico
    FOMFE = 1,          // 2D Fourier Expansion
    PoincareSmoothed = 2 // Poincaré + Legendre smoothing
};

struct OrientationParameters {
    int nCores = 1;
    int nDeltas = 1;
    double verticalBiasStrength = 0.0;   // Desabilitado por padrão
    double verticalBiasRadius = 80.0;    // Raio de influência em pixels
    double coreConvergenceStrength = 0.2; // Convergência modesta no core (0-1)
    double coreConvergenceRadius = 50.0;  // Raio de convergência
    double coreConvergenceProbability = 0.3; // Probabilidade aleatória (0-1)
    double anisotropyFactorX = 1.0;
    double anisotropyFactorY = 1.0;
    OrientationMethod method = OrientationMethod::Poincare;
    int fomfeOrderM = 5;
    int fomfeOrderN = 5;
    int legendreOrder = 5;
};

struct ClassificationParameters {
    FingerprintClass fingerprintClass = FingerprintClass::RightLoop;
};

struct RidgeParameters {
    int gaborFilterSize = 8;
    int cacheDegrees = 36;
    int cacheFrequencies = 10;
    int maxIterations = 20;
};

class FingerprintParameters {
public:
    FingerprintParameters();
    
    ShapeParameters shape;
    DensityParameters density;
    OrientationParameters orientation;
    RidgeParameters ridge;
    ClassificationParameters classification;
    
    bool loadFromJson(const QString& filePath);
    bool saveToJson(const QString& filePath) const;
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    void reset();
    bool validate() const;
};

}

#endif
