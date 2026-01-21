#ifndef FINGERPRINT_PARAMETERS_PURE_H
#define FINGERPRINT_PARAMETERS_PURE_H

#include <string>

enum class FingerprintClass {
    LeftLoop = 0,
    RightLoop = 1,
    Whorl = 2,
    Arch = 3,
    TentedArch = 4,
    TwinLoop = 5
};

struct ShapeParameters {
    int left = 500;
    int right = 500;
    int top = 480;
    int middle = 240;
    int bottom = 480;
};

struct DensityParameters {
    double mean = 0.5;
    double variance = 0.1;
    double noise = 0.05;
};

struct OrientationParameters {
    double loopEdgeBlendFactor = 0.0;
    double whorlEdgeDecayFactor = 0.0;
    double whorlSpiralFactor = 0.1;
};

struct RenderingParameters {
    bool enableNoise = false;
    bool enableContrast = false;
    bool enableDistortion = false;
    double noiseAmplitude = 0.1;
    double contrastFactor = 1.0;
    double distortionAmplitude = 0.05;
};

struct VariationParameters {
    double rotationRange = 0.1;
    double scaleRange = 0.1;
    double translationRange = 0.05;
};

struct ClassificationParameters {
    FingerprintClass fingerprintClass = FingerprintClass::RightLoop;
    double confidence = 0.9;
};

struct FingerprintParametersPure {
    ShapeParameters shape;
    DensityParameters density;
    OrientationParameters orientation;
    RenderingParameters rendering;
    VariationParameters variation;
    ClassificationParameters classification;
    
    void reset() {
        shape = ShapeParameters();
        density = DensityParameters();
        orientation = OrientationParameters();
        rendering = RenderingParameters();
        variation = VariationParameters();
        classification = ClassificationParameters();
    }
};

#endif
