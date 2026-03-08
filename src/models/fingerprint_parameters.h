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
    // 500×600px @ 500 DPI — escalado de 280×360 (fator 1.786×), sT/sL=1.33
    // sT > sL → ponta do dedo apontada; sM curto → corpo não retangular
    int left   = 250;
    int right  = 250;
    int top    = 333;  // sT/sL = 333/250 = 1.33 — ponta apontada
    int middle = 100;  // retângulo curto — 17% da altura total
    int bottom = 167;  // base arredondada
    FingerType fingerType = FingerType::Index;
};

struct DensityParameters {
    // Frequência de cristas — range estreito para espaçamento mais homogéneo:
    //   minF = 1/12 ≈ 0.083 (período 12px — esparso moderado)
    //   maxF = 1/7  ≈ 0.143 (período 7px  — denso moderado; fator ~1.7× vs 3× original)
    float minFrequency = 1.0f / 24.0f;  // período 24px — +10% vs 1/22; gaborFilterSize=20 necessário
    float maxFrequency = 1.0f / 18.0f;  // período 18px — +12% vs 1/16; fator 1.33×
    double zoom = 2.0;
    double amplify = 1.5;
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
    double verticalBiasRadius = 100.0;   // Raio de influência em pixels
    double coreConvergenceStrength = 0.2; // Convergência modesta no core (0-1)
    double coreConvergenceRadius = 50.0;  // Raio de convergência
    double coreConvergenceProbability = 0.3; // Probabilidade aleatória (0-1)
    double anisotropyFactorX = 1.0;
    double anisotropyFactorY = 1.0;
    OrientationMethod method = OrientationMethod::Poincare;  // Legendre linear sobre θ angular é matematicamente errado
    int fomfeOrderM = 5;
    int fomfeOrderN = 5;
    int legendreOrder = 5;
    
    // --- PARÂMETROS DE ARCH ---
    double archAmplitude = 0.22; // Amplitude da ondulação senoidal (0.15 a 0.30)
    
    // --- PARÂMETROS DE TENTED ARCH ---
    double tentedArchPeakInfluenceDecay = 0.12; // Fator de decaimento da influência do pico (0.08 a 0.18)
    
    // --- PARÂMETROS DE LOOP ---
    double loopVerticalBiasStrength = 0.4; // Força do bias que curva o loop para baixo
    double loopEdgeBlendFactor = 0.0; // Sem edge blend — referência FinGe.cpp não usa
    double loopVerticalBiasRadiusFactor = 1.5; // Fator do raio de influência (relativo à altura)
    
    // --- PARÂMETROS DE WHORL ---
    double whorlSpiralFactor = 0.12; // Fator de espiral (muito sutil)
    double whorlEdgeDecayFactor = 0.0; // Sem edge decay — referência FinGe.cpp não usa
    
    // --- PARÂMETROS DE TWIN LOOP ---
    double twinLoopSmoothing = 7.0; // Sigma de suavização específico
    
    // --- PARÂMETROS DE CENTRAL POCKET ---
    double centralPocketConcentration = 0.06; // Concentração da bolsa central
    
    // --- PARÂMETROS DE ACCIDENTAL ---
    double accidentalIrregularity = 0.08; // Intensidade da irregularidade
    
    // --- SUAVIZAÇÃO ---
    double smoothingSigma = 6.0; // Sigma suavização gaussiana — kernel 37px (validado pré-Rodada11)
    bool enableSmoothing = true; // Habilitar suavização do campo de orientação
    
    // --- MODO SILENCIOSO ---
    bool quietMode = false; // Desabilitar mensagens de debug
};

// Módulo 2: Parâmetros de Rendering Avançado
struct RenderingParameters {
    // Parâmetros de Ruído
    double backgroundNoiseFrequency = 0.03;
    double backgroundNoiseAmplitude = 0.02;
    double ridgeNoiseFrequency = 0.1;
    double ridgeNoiseAmplitude = 0.125;  // CSV: ridge_noise_amp_high ✓
    double valleyNoiseFrequency = 0.08;
    double valleyNoiseAmplitude = 0.02;

    // Parâmetros dos Poros - Sistema Avançado
    bool enablePores = true;
    // Poros de suor: 150-300 µm @ 500 DPI → 3-6 pixels de diâmetro
    double poreDensity = 0.008; // 0.8% dos pixels de crista recebem poros

    // Tamanhos em pixels — calibrados para 500 DPI
    double poreMinSize = 1.5;    // mínimo visível (~75 µm @ 500 DPI)
    double poreMaxSize = 5.0;    // máximo (~250 µm @ 500 DPI)
    double poreMeanSize = 1.875;   // médio log-normal (~94 µm @ 500 DPI) (CSV: pore_mean_size_low ✓)
    double poreSizeStdDev = 0.5; // desvio padrão

    // Formas dos poros
    double poreCircularRatio = 0.6;    // 60% circulares
    double poreEllipticalRatio = 0.3;  // 30% elípticos
    double poreIrregularRatio = 0.1;   // 10% irregulares
    double poreEllipseAspectMin = 1.2; // Razão de aspecto mínima para elipses
    double poreEllipseAspectMax = 2.0; // Razão de aspecto máxima

    // Intensidade: fração de interpolação em direção ao branco (vale)
    // 1.0 = poro completamente branco; 0.5 = meio-caminho entre crista e vale
    double minPoreIntensity = 0.375;   // interpolação mínima em direção ao vale (CSV: pore_intensity_min_low ✓)
    double maxPoreIntensity = 0.6375;  // interpolação máxima (CSV: pore_intensity_max_low ✓)
    double poreOpacityVariation = 0.3; // Variação na opacidade (0-1)

    // Distribuição espacial
    bool enablePoreClustering = true;  // Ativar agrupamentos de poros
    double poreClusteringFactor = 0.3; // Intensidade do agrupamento (0-1)
    int poreClusterSize = 3;           // Número de poros por cluster

    // Tipos de poros
    double incipientRidgeRatio = 0.0;  // 0% cristas incipientes (reduz bifurcações artificiais)

    // Parâmetros de Renderização Final
    double finalBlurSigma = 0.5;
    double contrastPercentileLower = 2.0;
    double contrastPercentileUpper = 98.0;

    // Iluminação Direcional - Simula captura em diferentes ângulos
    bool enableDirectionalLighting = false;
    double lightAzimuth = 56.25;         // Ângulo horizontal (0-360 graus) (CSV: light_azimuth_high ✓)
    double lightElevation = 33.75;       // Ângulo vertical (0-90 graus) (CSV: light_elevation_low ✓)
    double lightIntensity = 0.6;         // Intensidade do efeito (0-1)
    double ridgeHeight = 0.1875;         // Altura simulada das cristas (0-1) (CSV: ridge_height_high ✓)
    double ambientLight = 0.5;           // Luz ambiente mínima (0-1) (CSV: ambient_light_high ✓)
    bool enableSpecular = false;         // Ativar reflexos especulares
    double specularStrength = 0.3;       // Força dos reflexos (0-1)
    double specularShininess = 20.0;     // Nitidez dos reflexos (1-100)

    // Manchas e Artefatos - Imperfeições realistas
    bool enableArtifacts = true;        // Ativar sistema de artefatos

    // Umidade / Suor
    bool enableMoisture = false;         // Moisture aplicado externamente per-version; base sem moisture
    double moistureDensity = 0.002;      // Densidade de manchas (0-0.01)
    double moistureMinSize = 15.0;       // Tamanho mínimo (pixels)
    double moistureMaxSize = 40.0;       // Tamanho máximo
    double moistureIntensity = 0.15;     // Efeito no brilho (0-0.3)

    // Sujeira / Manchas escuras — desabilitado por padrão (causa manchas escuras)
    bool enableDirt = false;             // Manchas de sujeira
    double dirtDensity = 0.001;          // Densidade (0-0.01)
    double dirtMinSize = 10.0;           // Tamanho mínimo
    double dirtMaxSize = 30.0;           // Tamanho máximo
    double dirtIntensity = 0.2;          // Escurecimento (0-0.5)

    // Cicatrizes / Cortes — habilitado (usuário solicitou)
    bool enableScars = true;             // Cicatrizes lineares
    int numScars = 1;                    // Número de cicatrizes (0-5)
    double scarMinLength = 75.0;         // Comprimento mínimo (2× do original 37.5)
    double scarMaxLength = 200.0;        // Comprimento máximo (2× do original 100)
    double scarWidth = 2.5;              // Largura (pixels) (CSV: scar_width_high ✓)
    double scarIntensity = 0.375;        // Efeito nas cristas (0-1) (CSV: scar_intensity_high ✓)

    // Ressecamento / Rachaduras — desabilitado por padrão (causa manchas escuras)
    bool enableDryness = false;          // Linhas de ressecamento
    int numDryCracks = 3;                // Número de rachaduras (0-10)
    double dryCrackLength = 20.0;        // Comprimento médio
    double dryCrackWidth = 0.5;          // Largura (pixels)
    double dryCrackIntensity = 0.1;      // Efeito visual (0-0.3)

    // Borrões / Smudges (tinta excessiva) — desabilitado por padrão (causa manchas escuras)
    bool enableSmudges = false;          // Borrões
    double smudgeDensity = 0.0015;       // Densidade (0-0.01)
    double smudgeMinSize = 20.0;         // Tamanho mínimo
    double smudgeMaxSize = 50.0;         // Tamanho máximo
    double smudgeIntensity = 0.12;       // Escurecimento (0-0.3)

    // Anti-aliasing das transições crista-vale
    bool enableAntiAliasing = true;      // Ativar suavização de bordas
    double antiAliasingSigma = 1.5;      // era 0.8 — AA mais suave nas transições crista-vale
    double ridgeTransitionWidth = 1.5;   // Largura da transição suave (pixels)
};

// Módulo 3: Parâmetros de Variação e Distorção
struct VariationParameters {
    // Distorção Plástica/Elástica Avançada (Cappelli Step 7)
    bool enablePlasticDistortion = true;
    double plasticDistortionStrength = 0.8;   // era 2.0 — reduzido para não distorcer demais
    int plasticDistortionBumps = 2;           // era 3

    // Novos tipos de distorção
    bool enableMultiScaleDistortion = true;
    int distortionOctaves = 2;                // era 3
    double distortionPersistence = 0.5;

    bool enableRadialDistortion = true;
    double radialDistortionCenter = 0.5;
    double radialDistortionStrength = 0.5;    // era 1.5

    bool enableDirectionalDistortion = true;
    double directionalDistortionStrength = 0.25; // CSV: directional_strength_high ✓

    bool enableShearDistortion = false;       // Cisalhamento (movimento lateral)
    double shearAngle = 0.1;                  // Ângulo de cisalhamento (radianos)
    double shearStrength = 1.0;               // Força do cisalhamento

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

    // Condição da Pele (Cappelli Step 6: erosão=seca, dilatação=úmida)
    // factor > 0 → úmido (cristas mais grossas), factor < 0 → seco (cristas mais finas)
    bool enableSkinCondition = true;
    double skinConditionFactor = 0.125;  // CSV: skin_condition_factor_high ✓
};

struct ClassificationParameters {
    FingerprintClass fingerprintClass = FingerprintClass::RightLoop;
};

struct RidgeParameters {
    int gaborFilterSize = 20;  // Kernel 41x41 — permite minFreq até 1/22 sem clipping excessivo (~7%)
    int cacheDegrees = 36;
    int cacheFrequencies = 20;  // 20 bins de frequência — SFinGe original
    int maxIterations = 180;
    bool useSIMD = true;  // Enable SIMD optimizations (SSE2/AVX) for Gabor filtering

    // Initial seed density (controls minutiae quantity in original method)
    // Higher values = more initial seeds = more minutiae
    // Range: 0.0001 (very sparse) to 0.01 (very dense)
    // Default: 0.001 (0.1% - SFINGE original)
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
    // SFinGe original NÃO tem gerador explícito — minúcias surgem naturalmente do Gabor.
    // Habilitar este gerador DOBRA a quantidade de minúcias (natural + explícitas).
    bool enableExplicitMinutiae = false;
    MinutiaeStatistics stats;
    int targetMinutiae = -1;
    double insertionProbability = 0.7;
    double removalProbability = 0.3;
    
    // NOVOS PARÂMETROS PARA CONTROLE MELHORADO
    bool useContinuousPhase = false;  // Toggle para método antigo vs novo (padrão: método original)
    double phaseNoiseLevel = 0.1;      // 0.0 a 1.0 (ruído controlado)
    bool useQualityMask = true;        // Aplicar máscara de qualidade
    QString minutiaeDensity = "low";   // "low", "medium", "high"
    int customBifurcations = -1;       // -1 = usar preset
    int customEndings = -1;            // -1 = usar preset
    double coherenceThreshold = 0.5;   // Limiar para máscara de qualidade
    int qualityWindowSize = 16;        // Tamanho da janela de coerência
    double frequencySmoothSigma = 10.0; // Suavização do campo de frequência
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
