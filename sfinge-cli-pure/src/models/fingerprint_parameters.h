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
    // 280×360px @ 500 DPI — sT/sL=1.43 (SFinGe original usa ~1.5)
    // sT > sL → ponta do dedo apontada; sM curto → corpo não retangular
    int left   = 140;
    int right  = 140;
    int top    = 200;  // sT > sL: ponta alongada (original: sT=150 > sL=100)
    int middle = 60;   // retângulo curto — 17% da altura total
    int bottom = 100;  // base arredondada
    FingerType fingerType = FingerType::Index;
};

struct DensityParameters {
    // Frequência de cristas — range estreito para espaçamento mais homogéneo:
    //   minF = 1/12 ≈ 0.083 (período 12px — esparso moderado)
    //   maxF = 1/7  ≈ 0.143 (período 7px  — denso moderado; fator ~1.7× vs 3× original)
    float minFrequency = 1.0f / 24.0f;  // período 24px — sincronizado com Qt6
    float maxFrequency = 1.0f / 18.0f;  // período 18px — sincronizado com Qt6
    double zoom = 2.0;
    double amplify = 1.5;
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
    double verticalBiasRadius = 100.0;
    double coreConvergenceStrength = 0.2;
    double coreConvergenceRadius = 50.0;
    double coreConvergenceProbability = 0.3;
    double anisotropyFactorX = 1.0;
    double anisotropyFactorY = 1.0;
    OrientationMethod method = OrientationMethod::PoincareSmoothed;  // CSV: orientation_method_poincare-smoothed ✓
    int fomfeOrderM = 5;
    int fomfeOrderN = 5;
    int legendreOrder = 5;

    double archAmplitude = 0.22;
    double tentedArchPeakInfluenceDecay = 0.12;
    double loopVerticalBiasStrength = 0.4;
    double loopEdgeBlendFactor = 0.4;
    double loopVerticalBiasRadiusFactor = 1.5;
    double whorlSpiralFactor = 0.12;
    double whorlEdgeDecayFactor = 0.18;
    double twinLoopSmoothing = 7.0;
    double centralPocketConcentration = 0.06;
    double accidentalIrregularity = 0.08;
    double smoothingSigma = 7.5;  // CSV: smooth_sigma_high ✓
    bool enableSmoothing = true;
    bool quietMode = false;
};

struct ClassificationParameters {
    FingerprintClass fingerprintClass = FingerprintClass::RightLoop;
};

// Parâmetros de Rendering Avançado — sincronizado com sfinge-qt6 Rodada 6
struct RenderingParameters {
    // Ruído de textura
    double backgroundNoiseFrequency = 0.03;
    double backgroundNoiseAmplitude = 0.02;
    double ridgeNoiseFrequency = 0.1;
    double ridgeNoiseAmplitude = 0.125;  // CSV: ridge_noise_amp_high ✓
    double valleyNoiseFrequency = 0.08;
    double valleyNoiseAmplitude = 0.02;

    // Poros de suor: 150-300 µm @ 500 DPI → 3-6 pixels de diâmetro
    bool enablePores = true;
    double poreDensity = 0.008;          // 0.8% dos pixels de crista
    double poreMinSize = 1.5;            // mínimo visível (~75 µm @ 500 DPI)
    double poreMaxSize = 5.0;            // máximo (~250 µm @ 500 DPI)
    double poreMeanSize = 1.875;         // médio log-normal (CSV: pore_mean_size_low ✓)
    double poreSizeStdDev = 0.5;
    double poreCircularRatio = 0.6;      // 60% circulares
    double poreEllipticalRatio = 0.3;    // 30% elípticos
    double poreIrregularRatio = 0.1;     // 10% irregulares
    double poreEllipseAspectMin = 1.2;
    double poreEllipseAspectMax = 2.0;
    double minPoreIntensity = 0.375;     // CSV: pore_intensity_min_low ✓
    double maxPoreIntensity = 0.6375;    // CSV: pore_intensity_max_low ✓
    double poreOpacityVariation = 0.3;
    bool enablePoreClustering = true;
    double poreClusteringFactor = 0.3;
    int poreClusterSize = 3;
    double incipientRidgeRatio = 0.0;    // 0% cristas incipientes (reduz bifurcações artificiais)

    // Renderização final
    double finalBlurSigma = 0.5;
    double contrastPercentileLower = 2.0;
    double contrastPercentileUpper = 98.0;

    // Iluminação Direcional (desabilitada por padrão)
    bool enableDirectionalLighting = false;
    double lightAzimuth = 56.25;         // CSV: light_azimuth_high ✓
    double lightElevation = 33.75;       // CSV: light_elevation_low ✓
    double lightIntensity = 0.6;
    double ridgeHeight = 0.1875;         // CSV: ridge_height_high ✓
    double ambientLight = 0.5;           // CSV: ambient_light_high ✓
    bool enableSpecular = false;
    double specularStrength = 0.3;
    double specularShininess = 20.0;

    // Artefatos realistas
    bool enableArtifacts = true;

    // Umidade / Suor
    bool enableMoisture = true;
    double moistureDensity = 0.002;
    double moistureMinSize = 15.0;
    double moistureMaxSize = 40.0;
    double moistureIntensity = 0.15;

    // Sujeira (desabilitada por padrão — causa manchas escuras)
    bool enableDirt = false;
    double dirtDensity = 0.001;
    double dirtMinSize = 10.0;
    double dirtMaxSize = 30.0;
    double dirtIntensity = 0.2;

    // Cicatrizes lineares
    bool enableScars = true;
    int numScars = 1;
    double scarMinLength = 75.0;         // 2× do original 37.5
    double scarMaxLength = 200.0;        // 2× do original 100
    double scarWidth = 2.5;              // CSV: scar_width_high ✓
    double scarIntensity = 0.375;        // CSV: scar_intensity_high ✓

    // Ressecamento (desabilitado por padrão)
    bool enableDryness = false;
    int numDryCracks = 3;
    double dryCrackLength = 20.0;
    double dryCrackWidth = 0.5;
    double dryCrackIntensity = 0.1;

    // Borrões (desabilitado por padrão)
    bool enableSmudges = false;
    double smudgeDensity = 0.0015;
    double smudgeMinSize = 20.0;
    double smudgeMaxSize = 50.0;
    double smudgeIntensity = 0.12;

    // Anti-aliasing das transições crista-vale
    bool enableAntiAliasing = true;
    double antiAliasingSigma = 1.5;      // Rodada 6: era 0.8 — AA mais suave
    double ridgeTransitionWidth = 1.5;
};

// Parâmetros de Variação e Distorção — sincronizado com sfinge-qt6
struct VariationParameters {
    // Distorção Plástica/Elástica (Cappelli Step 7)
    bool enablePlasticDistortion = true;
    double plasticDistortionStrength = 0.8;   // reduzido de 2.0
    int plasticDistortionBumps = 2;

    // Distorção Multi-escala
    bool enableMultiScaleDistortion = true;
    int distortionOctaves = 2;
    double distortionPersistence = 0.5;

    // Distorção Radial
    bool enableRadialDistortion = true;
    double radialDistortionCenter = 0.5;
    double radialDistortionStrength = 0.5;

    // Distorção Direcional
    bool enableDirectionalDistortion = true;
    double directionalDistortionStrength = 0.25;  // CSV: directional_strength_high ✓

    // Cisalhamento (desabilitado por padrão)
    bool enableShearDistortion = false;
    double shearAngle = 0.1;
    double shearStrength = 1.0;

    // Distorção de Lente
    bool enableLensDistortion = false;
    double lensDistortionK1 = 0.02;
    double lensDistortionK2 = 0.005;

    // Rotação e Translação
    bool enableRotation = false;
    double maxRotationAngle = 5.0;
    bool enableTranslation = false;
    double maxTranslationX = 10.0;
    double maxTranslationY = 10.0;

    // Condição da Pele (Cappelli Step 6)
    bool enableSkinCondition = true;
    double skinConditionFactor = 0.125;  // CSV: skin_condition_factor_high ✓
};

struct RidgeParameters {
    int gaborFilterSize = 20;   // Kernel 41×41 — permite minFreq até 1/22 sem clipping excessivo (~7%)
    int cacheDegrees = 36;
    int cacheFrequencies = 20;  // 20 bins de frequência — SFinGe original (era 10!)
    int maxIterations = 180;    // convergência completa — SFinGe original (era 10!)
    bool useSIMD = true;

    // Densidade de sementes iniciais (SFinGe original: 0.001 = 0.1%)
    double initialSeedDensity = 0.001;
};

struct MinutiaeStatistics {
    int minMinutiae = 20;
    int maxMinutiae = 70;
    int typicalMinutiae = 40;
    double bifurcationRatio = 0.45;
    double coreConcentration = 0.6;
    double coreRadiusFactor = 0.4;
    double minSpacing = 24.0;
    double minQuality = 0.5;
    double maxQuality = 1.0;
};

struct MinutiaeParameters {
    bool useContinuousPhase = false;  // false = método original (SFinGe)
    double phaseNoiseLevel = 0.1;
    bool useQualityMask = false;
    std::string minutiaeDensity = "low";
    double coherenceThreshold = 0.3;
    int qualityWindowSize = 15;
    double frequencySmoothSigma = 1.5;
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
