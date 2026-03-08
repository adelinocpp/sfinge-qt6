#include "fingerprint_parameters.h"
#include <QFile>
#include <QJsonDocument>

namespace SFinGe {

FingerprintParameters::FingerprintParameters() {
    reset();
}

void FingerprintParameters::reset() {
    // Repõe todos os campos com os mesmos valores padrão declarados em fingerprint_parameters.h
    // IMPORTANTE: manter sincronizado com os defaults dos structs no .h

    // Shape — 500×600px base (batch usa 2× → 1000×1200); proporções GUI iguais ao SFinGe original
    shape.left = 250;
    shape.right = 250;
    shape.top = 333;    // sT > sL: ponta apontada
    shape.middle = 100;
    shape.bottom = 167;
    shape.fingerType = FingerType::Index;

    // Density — frequências validadas MEMORY.md; gaborFilterSize=20 adequado para 1/12–1/9
    density.minFrequency = 1.0f / 12.0f;
    density.maxFrequency = 1.0f / 9.0f;
    density.zoom = 2.0;
    density.amplify = 1.5;

    orientation.nCores = 1;
    orientation.nDeltas = 1;
    orientation.verticalBiasStrength = 0.0;
    orientation.verticalBiasRadius = 100.0;
    orientation.anisotropyFactorX = 1.0;
    orientation.anisotropyFactorY = 1.0;

    // Ridge — kernel 41×41 para suportar frequências baixas sem clipping excessivo
    ridge.gaborFilterSize = 20;
    ridge.cacheDegrees = 36;
    ridge.cacheFrequencies = 20;
    ridge.maxIterations = 180;
    ridge.initialSeedDensity = 0.001;

    // Minutiae
    minutiae.enableExplicitMinutiae = false;
    minutiae.useContinuousPhase = false;
    minutiae.phaseNoiseLevel = 0.1;
    minutiae.useQualityMask = false;
    minutiae.minutiaeDensity = "low";
    minutiae.customBifurcations = -1;
    minutiae.customEndings = -1;
    minutiae.coherenceThreshold = 0.5;
    minutiae.qualityWindowSize = 16;
    minutiae.frequencySmoothSigma = 10.0;
}

bool FingerprintParameters::validate() const {
    if (shape.left < 10 || shape.left > 500) return false;
    if (shape.right < 10 || shape.right > 500) return false;
    if (shape.top < 10 || shape.top > 600) return false;
    if (shape.bottom < 10 || shape.bottom > 400) return false;
    if (shape.middle < 10 || shape.middle > 300) return false;
    
    if (density.minFrequency <= 0 || density.maxFrequency <= 0) return false;
    if (density.minFrequency >= density.maxFrequency) return false;
    if (density.zoom <= 0) return false;
    
    if (orientation.nCores < 0 || orientation.nDeltas < 0) return false;
    
    return true;
}

QJsonObject FingerprintParameters::toJson() const {
    QJsonObject json;
    
    QJsonObject shapeObj;
    shapeObj["left"] = shape.left;
    shapeObj["right"] = shape.right;
    shapeObj["top"] = shape.top;
    shapeObj["bottom"] = shape.bottom;
    shapeObj["middle"] = shape.middle;
    shapeObj["fingerType"] = static_cast<int>(shape.fingerType);
    json["shape"] = shapeObj;
    
    QJsonObject densityObj;
    densityObj["minFrequency"] = density.minFrequency;
    densityObj["maxFrequency"] = density.maxFrequency;
    densityObj["zoom"] = density.zoom;
    densityObj["amplify"] = density.amplify;
    json["density"] = densityObj;
    
    QJsonObject orientationObj;
    orientationObj["nCores"] = orientation.nCores;
    orientationObj["nDeltas"] = orientation.nDeltas;
    orientationObj["verticalBiasStrength"] = orientation.verticalBiasStrength;
    orientationObj["verticalBiasRadius"] = orientation.verticalBiasRadius;
    orientationObj["coreConvergenceStrength"] = orientation.coreConvergenceStrength;
    orientationObj["coreConvergenceRadius"] = orientation.coreConvergenceRadius;
    orientationObj["coreConvergenceProbability"] = orientation.coreConvergenceProbability;
    orientationObj["anisotropyFactorX"] = orientation.anisotropyFactorX;
    orientationObj["anisotropyFactorY"] = orientation.anisotropyFactorY;
    orientationObj["method"] = static_cast<int>(orientation.method);
    orientationObj["fomfeOrderM"] = orientation.fomfeOrderM;
    orientationObj["fomfeOrderN"] = orientation.fomfeOrderN;
    orientationObj["legendreOrder"] = orientation.legendreOrder;
    // Parâmetros v2.0
    orientationObj["archAmplitude"] = orientation.archAmplitude;
    orientationObj["tentedArchPeakInfluenceDecay"] = orientation.tentedArchPeakInfluenceDecay;
    orientationObj["loopVerticalBiasStrength"] = orientation.loopVerticalBiasStrength;
    orientationObj["loopEdgeBlendFactor"] = orientation.loopEdgeBlendFactor;
    orientationObj["loopVerticalBiasRadiusFactor"] = orientation.loopVerticalBiasRadiusFactor;
    orientationObj["whorlSpiralFactor"] = orientation.whorlSpiralFactor;
    orientationObj["whorlEdgeDecayFactor"] = orientation.whorlEdgeDecayFactor;
    orientationObj["twinLoopSmoothing"] = orientation.twinLoopSmoothing;
    orientationObj["centralPocketConcentration"] = orientation.centralPocketConcentration;
    orientationObj["accidentalIrregularity"] = orientation.accidentalIrregularity;
    orientationObj["smoothingSigma"] = orientation.smoothingSigma;
    orientationObj["enableSmoothing"] = orientation.enableSmoothing;
    json["orientation"] = orientationObj;
    
    QJsonObject ridgeObj;
    ridgeObj["gaborFilterSize"] = ridge.gaborFilterSize;
    ridgeObj["cacheDegrees"] = ridge.cacheDegrees;
    ridgeObj["cacheFrequencies"] = ridge.cacheFrequencies;
    ridgeObj["maxIterations"] = ridge.maxIterations;
    ridgeObj["useSIMD"] = ridge.useSIMD;
    ridgeObj["initialSeedDensity"] = ridge.initialSeedDensity;
    json["ridge"] = ridgeObj;
    
    // Módulo 2: Parâmetros de Rendering
    QJsonObject renderingObj;
    renderingObj["backgroundNoiseFrequency"] = rendering.backgroundNoiseFrequency;
    renderingObj["backgroundNoiseAmplitude"] = rendering.backgroundNoiseAmplitude;
    renderingObj["ridgeNoiseFrequency"] = rendering.ridgeNoiseFrequency;
    renderingObj["ridgeNoiseAmplitude"] = rendering.ridgeNoiseAmplitude;
    renderingObj["valleyNoiseFrequency"] = rendering.valleyNoiseFrequency;
    renderingObj["valleyNoiseAmplitude"] = rendering.valleyNoiseAmplitude;
    renderingObj["enablePores"] = rendering.enablePores;
    renderingObj["poreDensity"] = rendering.poreDensity;
    // Advanced pore parameters
    renderingObj["poreMinSize"] = rendering.poreMinSize;
    renderingObj["poreMaxSize"] = rendering.poreMaxSize;
    renderingObj["poreMeanSize"] = rendering.poreMeanSize;
    renderingObj["poreSizeStdDev"] = rendering.poreSizeStdDev;
    renderingObj["poreCircularRatio"] = rendering.poreCircularRatio;
    renderingObj["poreEllipticalRatio"] = rendering.poreEllipticalRatio;
    renderingObj["poreIrregularRatio"] = rendering.poreIrregularRatio;
    renderingObj["poreEllipseAspectMin"] = rendering.poreEllipseAspectMin;
    renderingObj["poreEllipseAspectMax"] = rendering.poreEllipseAspectMax;
    renderingObj["minPoreIntensity"] = rendering.minPoreIntensity;
    renderingObj["maxPoreIntensity"] = rendering.maxPoreIntensity;
    renderingObj["poreOpacityVariation"] = rendering.poreOpacityVariation;
    renderingObj["enablePoreClustering"] = rendering.enablePoreClustering;
    renderingObj["poreClusteringFactor"] = rendering.poreClusteringFactor;
    renderingObj["poreClusterSize"] = rendering.poreClusterSize;
    renderingObj["incipientRidgeRatio"] = rendering.incipientRidgeRatio;
    renderingObj["finalBlurSigma"] = rendering.finalBlurSigma;
    renderingObj["contrastPercentileLower"] = rendering.contrastPercentileLower;
    renderingObj["contrastPercentileUpper"] = rendering.contrastPercentileUpper;
    renderingObj["enableDirectionalLighting"] = rendering.enableDirectionalLighting;
    renderingObj["lightAzimuth"] = rendering.lightAzimuth;
    renderingObj["lightElevation"] = rendering.lightElevation;
    renderingObj["lightIntensity"] = rendering.lightIntensity;
    renderingObj["ridgeHeight"] = rendering.ridgeHeight;
    renderingObj["ambientLight"] = rendering.ambientLight;
    renderingObj["enableSpecular"] = rendering.enableSpecular;
    renderingObj["specularStrength"] = rendering.specularStrength;
    renderingObj["specularShininess"] = rendering.specularShininess;
    renderingObj["enableArtifacts"] = rendering.enableArtifacts;
    renderingObj["enableMoisture"] = rendering.enableMoisture;
    renderingObj["moistureDensity"] = rendering.moistureDensity;
    renderingObj["moistureMinSize"] = rendering.moistureMinSize;
    renderingObj["moistureMaxSize"] = rendering.moistureMaxSize;
    renderingObj["moistureIntensity"] = rendering.moistureIntensity;
    renderingObj["enableDirt"] = rendering.enableDirt;
    renderingObj["dirtDensity"] = rendering.dirtDensity;
    renderingObj["dirtMinSize"] = rendering.dirtMinSize;
    renderingObj["dirtMaxSize"] = rendering.dirtMaxSize;
    renderingObj["dirtIntensity"] = rendering.dirtIntensity;
    renderingObj["enableScars"] = rendering.enableScars;
    renderingObj["numScars"] = rendering.numScars;
    renderingObj["scarMinLength"] = rendering.scarMinLength;
    renderingObj["scarMaxLength"] = rendering.scarMaxLength;
    renderingObj["scarWidth"] = rendering.scarWidth;
    renderingObj["scarIntensity"] = rendering.scarIntensity;
    renderingObj["enableDryness"] = rendering.enableDryness;
    renderingObj["numDryCracks"] = rendering.numDryCracks;
    renderingObj["dryCrackLength"] = rendering.dryCrackLength;
    renderingObj["dryCrackWidth"] = rendering.dryCrackWidth;
    renderingObj["dryCrackIntensity"] = rendering.dryCrackIntensity;
    renderingObj["enableSmudges"] = rendering.enableSmudges;
    renderingObj["smudgeDensity"] = rendering.smudgeDensity;
    renderingObj["smudgeMinSize"] = rendering.smudgeMinSize;
    renderingObj["smudgeMaxSize"] = rendering.smudgeMaxSize;
    renderingObj["smudgeIntensity"] = rendering.smudgeIntensity;
    renderingObj["enableAntiAliasing"] = rendering.enableAntiAliasing;
    renderingObj["antiAliasingSigma"] = rendering.antiAliasingSigma;
    renderingObj["ridgeTransitionWidth"] = rendering.ridgeTransitionWidth;
    json["rendering"] = renderingObj;
    
    // Módulo 3: Parâmetros de Variação
    QJsonObject variationObj;
    variationObj["enablePlasticDistortion"] = variation.enablePlasticDistortion;
    variationObj["plasticDistortionStrength"] = variation.plasticDistortionStrength;
    variationObj["plasticDistortionBumps"] = variation.plasticDistortionBumps;
    variationObj["enableMultiScaleDistortion"] = variation.enableMultiScaleDistortion;
    variationObj["distortionOctaves"] = variation.distortionOctaves;
    variationObj["distortionPersistence"] = variation.distortionPersistence;
    variationObj["enableRadialDistortion"] = variation.enableRadialDistortion;
    variationObj["radialDistortionCenter"] = variation.radialDistortionCenter;
    variationObj["radialDistortionStrength"] = variation.radialDistortionStrength;
    variationObj["enableDirectionalDistortion"] = variation.enableDirectionalDistortion;
    variationObj["directionalDistortionStrength"] = variation.directionalDistortionStrength;
    variationObj["enableShearDistortion"] = variation.enableShearDistortion;
    variationObj["shearAngle"] = variation.shearAngle;
    variationObj["shearStrength"] = variation.shearStrength;
    variationObj["enableLensDistortion"] = variation.enableLensDistortion;
    variationObj["lensDistortionK1"] = variation.lensDistortionK1;
    variationObj["lensDistortionK2"] = variation.lensDistortionK2;
    variationObj["enableRotation"] = variation.enableRotation;
    variationObj["maxRotationAngle"] = variation.maxRotationAngle;
    variationObj["enableTranslation"] = variation.enableTranslation;
    variationObj["maxTranslationX"] = variation.maxTranslationX;
    variationObj["maxTranslationY"] = variation.maxTranslationY;
    variationObj["enableSkinCondition"] = variation.enableSkinCondition;
    variationObj["skinConditionFactor"] = variation.skinConditionFactor;
    json["variation"] = variationObj;
    
    return json;
}

void FingerprintParameters::fromJson(const QJsonObject& json) {
    if (json.contains("shape")) {
        QJsonObject shapeObj = json["shape"].toObject();
        shape.left = shapeObj["left"].toInt(50);
        shape.right = shapeObj["right"].toInt(50);
        shape.top = shapeObj["top"].toInt(50);
        shape.bottom = shapeObj["bottom"].toInt(50);
        shape.middle = shapeObj["middle"].toInt(50);
        shape.fingerType = static_cast<FingerType>(shapeObj["fingerType"].toInt(1));
    }
    
    if (json.contains("density")) {
        QJsonObject densityObj = json["density"].toObject();
        density.minFrequency = densityObj["minFrequency"].toDouble(1.0 / 15.0);
        density.maxFrequency = densityObj["maxFrequency"].toDouble(1.0 / 5.0);
        density.zoom = densityObj["zoom"].toDouble(1.0);
        density.amplify = densityObj["amplify"].toDouble(0.5);
    }
    
    if (json.contains("orientation")) {
        QJsonObject orientationObj = json["orientation"].toObject();
        orientation.nCores = orientationObj["nCores"].toInt(1);
        orientation.nDeltas = orientationObj["nDeltas"].toInt(1);
        orientation.verticalBiasStrength = orientationObj["verticalBiasStrength"].toDouble(0.0);
        orientation.verticalBiasRadius = orientationObj["verticalBiasRadius"].toDouble(100.0);
        orientation.coreConvergenceStrength = orientationObj["coreConvergenceStrength"].toDouble(0.2);
        orientation.coreConvergenceRadius = orientationObj["coreConvergenceRadius"].toDouble(50.0);
        orientation.coreConvergenceProbability = orientationObj["coreConvergenceProbability"].toDouble(0.3);
        orientation.anisotropyFactorX = orientationObj["anisotropyFactorX"].toDouble(1.0);
        orientation.anisotropyFactorY = orientationObj["anisotropyFactorY"].toDouble(1.0);
        orientation.method = static_cast<OrientationMethod>(orientationObj["method"].toInt(0));
        orientation.fomfeOrderM = orientationObj["fomfeOrderM"].toInt(5);
        orientation.fomfeOrderN = orientationObj["fomfeOrderN"].toInt(5);
        orientation.legendreOrder = orientationObj["legendreOrder"].toInt(5);
        // Parâmetros v2.0
        orientation.archAmplitude = orientationObj["archAmplitude"].toDouble(0.22);
        orientation.tentedArchPeakInfluenceDecay = orientationObj["tentedArchPeakInfluenceDecay"].toDouble(0.12);
        orientation.loopVerticalBiasStrength = orientationObj["loopVerticalBiasStrength"].toDouble(0.4);
        orientation.loopEdgeBlendFactor = orientationObj["loopEdgeBlendFactor"].toDouble(0.0);
        orientation.loopVerticalBiasRadiusFactor = orientationObj["loopVerticalBiasRadiusFactor"].toDouble(1.5);
        orientation.whorlSpiralFactor = orientationObj["whorlSpiralFactor"].toDouble(0.12);
        orientation.whorlEdgeDecayFactor = orientationObj["whorlEdgeDecayFactor"].toDouble(0.0);
        orientation.twinLoopSmoothing = orientationObj["twinLoopSmoothing"].toDouble(7.0);
        orientation.centralPocketConcentration = orientationObj["centralPocketConcentration"].toDouble(0.06);
        orientation.accidentalIrregularity = orientationObj["accidentalIrregularity"].toDouble(0.08);
        orientation.smoothingSigma = orientationObj["smoothingSigma"].toDouble(6.0);
        orientation.enableSmoothing = orientationObj["enableSmoothing"].toBool(true);
    }
    
    if (json.contains("ridge")) {
        QJsonObject ridgeObj = json["ridge"].toObject();
        ridge.gaborFilterSize = ridgeObj["gaborFilterSize"].toInt(25);
        ridge.cacheDegrees = ridgeObj["cacheDegrees"].toInt(36);
        ridge.cacheFrequencies = ridgeObj["cacheFrequencies"].toInt(10);
        ridge.maxIterations = ridgeObj["maxIterations"].toInt(180);
        ridge.useSIMD = ridgeObj["useSIMD"].toBool(true);
        ridge.initialSeedDensity = ridgeObj["initialSeedDensity"].toDouble(0.001);
    }
    
    // Módulo 2: Parâmetros de Rendering
    if (json.contains("rendering")) {
        QJsonObject renderingObj = json["rendering"].toObject();
        rendering.backgroundNoiseFrequency = renderingObj["backgroundNoiseFrequency"].toDouble(0.03);
        rendering.backgroundNoiseAmplitude = renderingObj["backgroundNoiseAmplitude"].toDouble(0.02);
        rendering.ridgeNoiseFrequency = renderingObj["ridgeNoiseFrequency"].toDouble(0.1);
        rendering.ridgeNoiseAmplitude = renderingObj["ridgeNoiseAmplitude"].toDouble(0.05);
        rendering.valleyNoiseFrequency = renderingObj["valleyNoiseFrequency"].toDouble(0.08);
        rendering.valleyNoiseAmplitude = renderingObj["valleyNoiseAmplitude"].toDouble(0.02);
        rendering.enablePores = renderingObj["enablePores"].toBool(true);
        rendering.poreDensity = renderingObj["poreDensity"].toDouble(0.008);
        // Advanced pore parameters
        rendering.poreMinSize = renderingObj["poreMinSize"].toDouble(1.5);
        rendering.poreMaxSize = renderingObj["poreMaxSize"].toDouble(5.0);
        rendering.poreMeanSize = renderingObj["poreMeanSize"].toDouble(1.875);
        rendering.poreSizeStdDev = renderingObj["poreSizeStdDev"].toDouble(0.3);
        rendering.poreCircularRatio = renderingObj["poreCircularRatio"].toDouble(0.6);
        rendering.poreEllipticalRatio = renderingObj["poreEllipticalRatio"].toDouble(0.3);
        rendering.poreIrregularRatio = renderingObj["poreIrregularRatio"].toDouble(0.1);
        rendering.poreEllipseAspectMin = renderingObj["poreEllipseAspectMin"].toDouble(1.2);
        rendering.poreEllipseAspectMax = renderingObj["poreEllipseAspectMax"].toDouble(2.0);
        rendering.minPoreIntensity = renderingObj["minPoreIntensity"].toDouble(0.375);
        rendering.maxPoreIntensity = renderingObj["maxPoreIntensity"].toDouble(0.6375);
        rendering.poreOpacityVariation = renderingObj["poreOpacityVariation"].toDouble(0.3);
        rendering.enablePoreClustering = renderingObj["enablePoreClustering"].toBool(true);
        rendering.poreClusteringFactor = renderingObj["poreClusteringFactor"].toDouble(0.3);
        rendering.poreClusterSize = renderingObj["poreClusterSize"].toInt(3);
        rendering.incipientRidgeRatio = renderingObj["incipientRidgeRatio"].toDouble(0.0);
        rendering.finalBlurSigma = renderingObj["finalBlurSigma"].toDouble(0.5);
        rendering.contrastPercentileLower = renderingObj["contrastPercentileLower"].toDouble(2.0);
        rendering.contrastPercentileUpper = renderingObj["contrastPercentileUpper"].toDouble(98.0);
        rendering.enableDirectionalLighting = renderingObj["enableDirectionalLighting"].toBool(false);
        rendering.lightAzimuth = renderingObj["lightAzimuth"].toDouble(45.0);
        rendering.lightElevation = renderingObj["lightElevation"].toDouble(45.0);
        rendering.lightIntensity = renderingObj["lightIntensity"].toDouble(0.6);
        rendering.ridgeHeight = renderingObj["ridgeHeight"].toDouble(0.15);
        rendering.ambientLight = renderingObj["ambientLight"].toDouble(0.4);
        rendering.enableSpecular = renderingObj["enableSpecular"].toBool(false);
        rendering.specularStrength = renderingObj["specularStrength"].toDouble(0.3);
        rendering.specularShininess = renderingObj["specularShininess"].toDouble(20.0);
        rendering.enableArtifacts = renderingObj["enableArtifacts"].toBool(false);
        rendering.enableMoisture = renderingObj["enableMoisture"].toBool(true);
        rendering.moistureDensity = renderingObj["moistureDensity"].toDouble(0.002);
        rendering.moistureMinSize = renderingObj["moistureMinSize"].toDouble(15.0);
        rendering.moistureMaxSize = renderingObj["moistureMaxSize"].toDouble(40.0);
        rendering.moistureIntensity = renderingObj["moistureIntensity"].toDouble(0.15);
        rendering.enableDirt = renderingObj["enableDirt"].toBool(false);
        rendering.dirtDensity = renderingObj["dirtDensity"].toDouble(0.001);
        rendering.dirtMinSize = renderingObj["dirtMinSize"].toDouble(10.0);
        rendering.dirtMaxSize = renderingObj["dirtMaxSize"].toDouble(30.0);
        rendering.dirtIntensity = renderingObj["dirtIntensity"].toDouble(0.2);
        rendering.enableScars = renderingObj["enableScars"].toBool(true);
        rendering.numScars = renderingObj["numScars"].toInt(1);
        rendering.scarMinLength = renderingObj["scarMinLength"].toDouble(75.0);
        rendering.scarMaxLength = renderingObj["scarMaxLength"].toDouble(200.0);
        rendering.scarWidth = renderingObj["scarWidth"].toDouble(2.5);
        rendering.scarIntensity = renderingObj["scarIntensity"].toDouble(0.375);
        rendering.enableDryness = renderingObj["enableDryness"].toBool(false);
        rendering.numDryCracks = renderingObj["numDryCracks"].toInt(3);
        rendering.dryCrackLength = renderingObj["dryCrackLength"].toDouble(20.0);
        rendering.dryCrackWidth = renderingObj["dryCrackWidth"].toDouble(0.5);
        rendering.dryCrackIntensity = renderingObj["dryCrackIntensity"].toDouble(0.1);
        rendering.enableSmudges = renderingObj["enableSmudges"].toBool(false);
        rendering.smudgeDensity = renderingObj["smudgeDensity"].toDouble(0.0015);
        rendering.smudgeMinSize = renderingObj["smudgeMinSize"].toDouble(20.0);
        rendering.smudgeMaxSize = renderingObj["smudgeMaxSize"].toDouble(50.0);
        rendering.smudgeIntensity = renderingObj["smudgeIntensity"].toDouble(0.12);
        rendering.enableAntiAliasing = renderingObj["enableAntiAliasing"].toBool(true);
        rendering.antiAliasingSigma = renderingObj["antiAliasingSigma"].toDouble(1.5);
        rendering.ridgeTransitionWidth = renderingObj["ridgeTransitionWidth"].toDouble(1.5);
    }
    
    // Módulo 3: Parâmetros de Variação
    if (json.contains("variation")) {
        QJsonObject variationObj = json["variation"].toObject();
        variation.enablePlasticDistortion = variationObj["enablePlasticDistortion"].toBool(true);
        variation.plasticDistortionStrength = variationObj["plasticDistortionStrength"].toDouble(0.8);
        variation.plasticDistortionBumps = variationObj["plasticDistortionBumps"].toInt(2);
        variation.enableMultiScaleDistortion = variationObj["enableMultiScaleDistortion"].toBool(true);
        variation.distortionOctaves = variationObj["distortionOctaves"].toInt(2);
        variation.distortionPersistence = variationObj["distortionPersistence"].toDouble(0.5);
        variation.enableRadialDistortion = variationObj["enableRadialDistortion"].toBool(true);
        variation.radialDistortionCenter = variationObj["radialDistortionCenter"].toDouble(0.5);
        variation.radialDistortionStrength = variationObj["radialDistortionStrength"].toDouble(0.5);
        variation.enableDirectionalDistortion = variationObj["enableDirectionalDistortion"].toBool(true);
        variation.directionalDistortionStrength = variationObj["directionalDistortionStrength"].toDouble(0.2);
        variation.enableShearDistortion = variationObj["enableShearDistortion"].toBool(false);
        variation.shearAngle = variationObj["shearAngle"].toDouble(0.1);
        variation.shearStrength = variationObj["shearStrength"].toDouble(1.0);
        variation.enableLensDistortion = variationObj["enableLensDistortion"].toBool(false);
        variation.lensDistortionK1 = variationObj["lensDistortionK1"].toDouble(0.02);
        variation.lensDistortionK2 = variationObj["lensDistortionK2"].toDouble(0.005);
        variation.enableRotation = variationObj["enableRotation"].toBool(false);
        variation.maxRotationAngle = variationObj["maxRotationAngle"].toDouble(5.0);
        variation.enableTranslation = variationObj["enableTranslation"].toBool(false);
        variation.maxTranslationX = variationObj["maxTranslationX"].toDouble(10.0);
        variation.maxTranslationY = variationObj["maxTranslationY"].toDouble(10.0);
        variation.enableSkinCondition = variationObj["enableSkinCondition"].toBool(true);
        variation.skinConditionFactor = variationObj["skinConditionFactor"].toDouble(0.1);
    }
}

bool FingerprintParameters::loadFromJson(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }
    
    fromJson(doc.object());
    return validate();
}

bool FingerprintParameters::saveToJson(const QString& filePath) const {
    if (!validate()) {
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

}
