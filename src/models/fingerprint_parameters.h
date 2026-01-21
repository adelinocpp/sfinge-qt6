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
    TentedArch = 2,     // Arco Tendido: 1 core, 1 delta
    LeftLoop = 3,       // Presilha Esquerda: 1 core, 1 delta
    RightLoop = 4,      // Presilha Direita: 1 core, 1 delta
    Whorl = 5,          // Verticílio Plano: 2 cores, 2 deltas
    TwinLoop = 6,       // Loop Duplo (Double Loop): 1 core, 2 deltas
    CentralPocket = 7,  // Bolsa Central: 1 core, 2 deltas (whorl pequeno no centro)
    Accidental = 8      // Acidental: combinação de padrões (2+ deltas)
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
    
    // --- PARÂMETROS DE ARCH ---
    double archAmplitude = 0.22; // Amplitude da ondulação senoidal (0.15 a 0.30)
    
    // --- PARÂMETROS DE TENTED ARCH ---
    double tentedArchPeakInfluenceDecay = 0.12; // Fator de decaimento da influência do pico (0.08 a 0.18)
    
    // --- PARÂMETROS DE LOOP ---
    double loopVerticalBiasStrength = 0.4; // Força do bias que curva o loop para baixo
    double loopEdgeBlendFactor = 0.4; // Força da transição para horizontal nas bordas
    double loopVerticalBiasRadiusFactor = 1.5; // Fator do raio de influência (relativo à altura)
    
    // --- PARÂMETROS DE WHORL ---
    double whorlSpiralFactor = 0.12; // Fator de espiral (muito sutil)
    double whorlEdgeDecayFactor = 0.18; // Decaimento da transição para as bordas
    
    // --- PARÂMETROS DE TWIN LOOP ---
    double twinLoopSmoothing = 7.0; // Sigma de suavização específico
    
    // --- PARÂMETROS DE CENTRAL POCKET ---
    double centralPocketConcentration = 0.06; // Concentração da bolsa central
    
    // --- PARÂMETROS DE ACCIDENTAL ---
    double accidentalIrregularity = 0.08; // Intensidade da irregularidade
    
    // --- SUAVIZAÇÃO ---
    double smoothingSigma = 6.0; // Sigma para suavização gaussiana do campo
    bool enableSmoothing = true; // Habilitar suavização do campo de orientação
    
    // --- MODO SILENCIOSO ---
    bool quietMode = false; // Desabilitar mensagens de debug
};

// Módulo 2: Parâmetros de Rendering Avançado
struct RenderingParameters {
    // Parâmetros de Ruído
    double backgroundNoiseFrequency = 0.03;
    double backgroundNoiseAmplitude = 0.02;
    double ridgeNoiseFrequency = 0.05;
    double ridgeNoiseAmplitude = 0.02;
    double valleyNoiseFrequency = 0.08;
    double valleyNoiseAmplitude = 0.02;

    // Parâmetros dos Poros
    bool enablePores = true;
    double poreDensity = 0.0015; // Poros por pixel de crista
    double minPoreSize = 0.5; // Em pixels
    double maxPoreSize = 1.0; // Em pixels
    double minPoreIntensity = 0.02; // Aumento de brilho
    double maxPoreIntensity = 0.04;

    // Parâmetros de Renderização Final
    double finalBlurSigma = 0.5;
    double contrastPercentileLower = 2.0;
    double contrastPercentileUpper = 98.0;
};

// Módulo 3: Parâmetros de Variação e Distorção
struct VariationParameters {
    // Distorção Plástica
    bool enablePlasticDistortion = false;
    double plasticDistortionStrength = 2.0;
    int plasticDistortionBumps = 2;

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

    // Condição da Pele
    bool enableSkinCondition = false;
    double skinConditionFactor = 0.1;
};

struct ClassificationParameters {
    FingerprintClass fingerprintClass = FingerprintClass::RightLoop;
};

struct RidgeParameters {
    int gaborFilterSize = 8;
    int cacheDegrees = 36;
    int cacheFrequencies = 10;
    int maxIterations = 180;
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
    bool enableExplicitMinutiae = true;
    MinutiaeStatistics stats;
    int targetMinutiae = -1;
    double insertionProbability = 0.7;
    double removalProbability = 0.3;
};

class FingerprintParameters {
public:
    FingerprintParameters();
    
    ShapeParameters shape;
    DensityParameters density;
    OrientationParameters orientation;
    RidgeParameters ridge;
    ClassificationParameters classification;
    RenderingParameters rendering;
    VariationParameters variation;
    MinutiaeParameters minutiae;
    
    bool loadFromJson(const QString& filePath);
    bool saveToJson(const QString& filePath) const;
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    void reset();
    bool validate() const;
};

}

#endif
