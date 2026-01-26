#ifndef FINGERPRINT_PARAMETERS_H
#define FINGERPRINT_PARAMETERS_H

#include <string>

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
    Arch = 1,
    TentedArch = 2,
    LeftLoop = 3,
    RightLoop = 4,
    Whorl = 5,
    TwinLoop = 6,
    CentralPocket = 7,
    Accidental = 8
};

struct ShapeParameters {
    int left = 500;
    int right = 500;
    int top = 480;
    int middle = 240;
    int bottom = 480;
    FingerType fingerType = FingerType::Index;
};

struct DensityParameters {
    // Densidade de cristas realista a 500 DPI:
    // - Distância inter-crista típica: 0.45-0.55mm
    // - A 500 DPI: 1mm = 19.7 pixels
    // - Período: 9-11 pixels (frequência: 1/11 a 1/9)
    float minFrequency = 1.0f / 11.0f;  // ~0.091 (período 11 pixels = 0.56mm)
    float maxFrequency = 1.0f / 9.0f;   // ~0.111 (período 9 pixels = 0.46mm)
    double zoom = 1.0;
    double amplify = 0.5;
};

enum class OrientationMethod {
    Poincare = 0,
    FOMFE = 1,
    PoincareSmoothed = 2
};

struct OrientationParameters {
    int nCores = 1;
    int nDeltas = 1;
    double verticalBiasStrength = 0.0;
    double verticalBiasRadius = 80.0;
    double coreConvergenceStrength = 0.2;
    double coreConvergenceRadius = 50.0;
    double coreConvergenceProbability = 0.3;
    double anisotropyFactorX = 1.0;
    double anisotropyFactorY = 1.0;
    OrientationMethod method = OrientationMethod::Poincare;
    int fomfeOrderM = 5;
    int fomfeOrderN = 5;
    int legendreOrder = 5;
    
    double archAmplitude = 0.22;
    double tentedArchPeakInfluenceDecay = 0.12;
    double loopVerticalBiasStrength = 0.4;
    double loopEdgeBlendFactor = 0.0;
    double loopVerticalBiasRadiusFactor = 1.5;
    double whorlSpiralFactor = 0.12;
    double whorlEdgeDecayFactor = 0.0;
    double twinLoopSmoothing = 7.0;
    double centralPocketConcentration = 0.06;
    double accidentalIrregularity = 0.08;
    double smoothingSigma = 6.0;
    bool enableSmoothing = true;
    bool quietMode = false;
};

struct ClassificationParameters {
    FingerprintClass fingerprintClass = FingerprintClass::RightLoop;
};

struct RenderingParameters {
    double backgroundNoiseFrequency = 0.03;
    double backgroundNoiseAmplitude = 0.02;
    double ridgeNoiseFrequency = 0.05;
    double ridgeNoiseAmplitude = 0.02;
    double valleyNoiseFrequency = 0.08;
    double valleyNoiseAmplitude = 0.02;
    bool enablePores = true;
    double poreDensity = 0.0015;
    double minPoreSize = 0.5;
    double maxPoreSize = 1.0;
    double minPoreIntensity = 0.02;
    double maxPoreIntensity = 0.04;
    double finalBlurSigma = 0.5;
    double contrastPercentileLower = 2.0;
    double contrastPercentileUpper = 98.0;
};

struct VariationParameters {
    bool enablePlasticDistortion = true;
    double plasticDistortionStrength = 2.0;
    int plasticDistortionBumps = 2;
    bool enableLensDistortion = true;
    double lensDistortionK1 = 0.2;
    double lensDistortionK2 = 0.05;
    bool enableRotation = true;
    double maxRotationAngle = 5.0;
    bool enableTranslation = true;
    double maxTranslationX = 10.0;
    double maxTranslationY = 10.0;
    bool enableSkinCondition = false;
    double skinConditionFactor = 0.1;
};

struct RidgeParameters {
    int gaborFilterSize = 10;  // Filtro grande para cristas mais suaves
    int cacheDegrees = 36;
    int cacheFrequencies = 10;
    int maxIterations = 10;    // Reduzido para 10
};

struct MinutiaeStatistics {
    int minMinutiae = 15;          // Reduzido de 20
    int maxMinutiae = 45;          // Reduzido de 70
    int typicalMinutiae = 25;      // Reduzido de 40
    double bifurcationRatio = 0.45;
    double coreConcentration = 0.6;
    double coreRadiusFactor = 0.4;
    double minSpacing = 36.0;      // Aumentado de 12 para maior espaçamento
    double minQuality = 0.5;
    double maxQuality = 1.0;
};

struct MinutiaeParameters {
    // Controle de método original vs melhorado
    bool useContinuousPhase = false;  // false = método original (random phase)
    
    // Parâmetros do método melhorado
    double phaseNoiseLevel = 0.1;    // Nível de ruído no campo de fase (0.0-1.0)
    bool useQualityMask = false;      // Usar máscara de qualidade
    std::string minutiaeDensity = "low";  // low/medium/high
    
    // Parâmetros avançados
    double coherenceThreshold = 0.3;  // Limiar de coerência da máscara
    int qualityWindowSize = 15;       // Tamanho da janela de qualidade
    double frequencySmoothSigma = 1.5; // Sigma para suavização de frequência
    
    // Parâmetros legados (mantidos para compatibilidade)
    bool enableExplicitMinutiae = false;
    MinutiaeStatistics stats;
    int targetMinutiae = -1;
    double insertionProbability = 0.7;
    double removalProbability = 0.3;
};

struct FingerprintParameters {
    ShapeParameters shape;
    DensityParameters density;
    OrientationParameters orientation;
    ClassificationParameters classification;
    RidgeParameters ridge;
    RenderingParameters rendering;
    VariationParameters variation;
    MinutiaeParameters minutiae;
    
    void reset() {
        shape = ShapeParameters();
        density = DensityParameters();
        orientation = OrientationParameters();
        classification = ClassificationParameters();
        ridge = RidgeParameters();
        rendering = RenderingParameters();
        variation = VariationParameters();
        minutiae = MinutiaeParameters();
    }
};

} // namespace SFinGe

#endif // FINGERPRINT_PARAMETERS_H
