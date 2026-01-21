#include "fingerprint_parameters.h"
#include <QFile>
#include <QJsonDocument>

namespace SFinGe {

FingerprintParameters::FingerprintParameters() {
    reset();
}

void FingerprintParameters::reset() {
    // Parâmetros ajustados para 500 DPI - tamanho real de digital (1.0" × 1.2")
    // Width: 500px = 1.0" @ 500 DPI, Height: 600px = 1.2" @ 500 DPI
    shape.left = 250;
    shape.right = 250;
    shape.top = 300;
    shape.bottom = 200;
    shape.middle = 100;
    shape.fingerType = FingerType::Index;
    
    // Densidade: 11 linhas/5mm = 2.2 linhas/mm = 55.88 linhas/polegada
    // A 500 DPI: ~9 pixels por linha (ridge + valley), então ~4.5px por crista
    // Densidade: 11 linhas/5mm = período 7-10px
    density.minFrequency = 1.0f / 10.0f;
    density.maxFrequency = 1.0f / 7.0f;
    density.zoom = 2.0;
    density.amplify = 1.5;
    
    orientation.nCores = 1;
    orientation.nDeltas = 1;
    orientation.verticalBiasStrength = 0.0;
    orientation.verticalBiasRadius = 100.0;
    orientation.anisotropyFactorX = 1.0;
    orientation.anisotropyFactorY = 1.0;
    
    // Parâmetros do filtro Gabor (otimizados para performance)
    ridge.gaborFilterSize = 8;    // Kernel 17x17 - bom balanço
    ridge.cacheDegrees = 36;      // FG_CACHE_DEG original
    ridge.cacheFrequencies = 10;  // Qualidade boa
    ridge.maxIterations = 20;     // Reduzido com early stopping
}

bool FingerprintParameters::validate() const {
    if (shape.left < 10 || shape.left > 200) return false;
    if (shape.right < 10 || shape.right > 200) return false;
    if (shape.top < 10 || shape.top > 200) return false;
    if (shape.bottom < 10 || shape.bottom > 200) return false;
    if (shape.middle < 10 || shape.middle > 200) return false;
    
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
    orientationObj["anisotropyFactorX"] = orientation.anisotropyFactorX;
    orientationObj["anisotropyFactorY"] = orientation.anisotropyFactorY;
    // Parâmetros v2.0
    orientationObj["archAmplitude"] = orientation.archAmplitude;
    orientationObj["tentedArchPeakInfluenceDecay"] = orientation.tentedArchPeakInfluenceDecay;
    orientationObj["loopVerticalBiasStrength"] = orientation.loopVerticalBiasStrength;
    orientationObj["loopEdgeBlendFactor"] = orientation.loopEdgeBlendFactor;
    orientationObj["loopVerticalBiasRadiusFactor"] = orientation.loopVerticalBiasRadiusFactor;
    orientationObj["whorlSpiralFactor"] = orientation.whorlSpiralFactor;
    orientationObj["whorlEdgeDecayFactor"] = orientation.whorlEdgeDecayFactor;
    orientationObj["smoothingSigma"] = orientation.smoothingSigma;
    orientationObj["enableSmoothing"] = orientation.enableSmoothing;
    json["orientation"] = orientationObj;
    
    QJsonObject ridgeObj;
    ridgeObj["gaborFilterSize"] = ridge.gaborFilterSize;
    ridgeObj["cacheDegrees"] = ridge.cacheDegrees;
    ridgeObj["cacheFrequencies"] = ridge.cacheFrequencies;
    ridgeObj["maxIterations"] = ridge.maxIterations;
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
    renderingObj["minPoreSize"] = rendering.minPoreSize;
    renderingObj["maxPoreSize"] = rendering.maxPoreSize;
    renderingObj["minPoreIntensity"] = rendering.minPoreIntensity;
    renderingObj["maxPoreIntensity"] = rendering.maxPoreIntensity;
    renderingObj["finalBlurSigma"] = rendering.finalBlurSigma;
    renderingObj["contrastPercentileLower"] = rendering.contrastPercentileLower;
    renderingObj["contrastPercentileUpper"] = rendering.contrastPercentileUpper;
    json["rendering"] = renderingObj;
    
    // Módulo 3: Parâmetros de Variação
    QJsonObject variationObj;
    variationObj["enablePlasticDistortion"] = variation.enablePlasticDistortion;
    variationObj["plasticDistortionStrength"] = variation.plasticDistortionStrength;
    variationObj["plasticDistortionBumps"] = variation.plasticDistortionBumps;
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
        orientation.anisotropyFactorX = orientationObj["anisotropyFactorX"].toDouble(1.0);
        orientation.anisotropyFactorY = orientationObj["anisotropyFactorY"].toDouble(1.0);
        // Parâmetros v2.0
        orientation.archAmplitude = orientationObj["archAmplitude"].toDouble(0.15);
        orientation.tentedArchPeakInfluenceDecay = orientationObj["tentedArchPeakInfluenceDecay"].toDouble(0.06);
        orientation.loopVerticalBiasStrength = orientationObj["loopVerticalBiasStrength"].toDouble(0.4);
        orientation.loopEdgeBlendFactor = orientationObj["loopEdgeBlendFactor"].toDouble(0.4);
        orientation.loopVerticalBiasRadiusFactor = orientationObj["loopVerticalBiasRadiusFactor"].toDouble(1.5);
        orientation.whorlSpiralFactor = orientationObj["whorlSpiralFactor"].toDouble(0.12);
        orientation.whorlEdgeDecayFactor = orientationObj["whorlEdgeDecayFactor"].toDouble(0.18);
        orientation.smoothingSigma = orientationObj["smoothingSigma"].toDouble(6.0);
        orientation.enableSmoothing = orientationObj["enableSmoothing"].toBool(true);
    }
    
    if (json.contains("ridge")) {
        QJsonObject ridgeObj = json["ridge"].toObject();
        ridge.gaborFilterSize = ridgeObj["gaborFilterSize"].toInt(25);
        ridge.cacheDegrees = ridgeObj["cacheDegrees"].toInt(36);
        ridge.cacheFrequencies = ridgeObj["cacheFrequencies"].toInt(10);
        ridge.maxIterations = ridgeObj["maxIterations"].toInt(50);
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
        rendering.poreDensity = renderingObj["poreDensity"].toDouble(0.0015);
        rendering.minPoreSize = renderingObj["minPoreSize"].toDouble(0.5);
        rendering.maxPoreSize = renderingObj["maxPoreSize"].toDouble(1.0);
        rendering.minPoreIntensity = renderingObj["minPoreIntensity"].toDouble(0.02);
        rendering.maxPoreIntensity = renderingObj["maxPoreIntensity"].toDouble(0.04);
        rendering.finalBlurSigma = renderingObj["finalBlurSigma"].toDouble(0.5);
        rendering.contrastPercentileLower = renderingObj["contrastPercentileLower"].toDouble(2.0);
        rendering.contrastPercentileUpper = renderingObj["contrastPercentileUpper"].toDouble(98.0);
    }
    
    // Módulo 3: Parâmetros de Variação
    if (json.contains("variation")) {
        QJsonObject variationObj = json["variation"].toObject();
        variation.enablePlasticDistortion = variationObj["enablePlasticDistortion"].toBool(true);
        variation.plasticDistortionStrength = variationObj["plasticDistortionStrength"].toDouble(8.0);
        variation.plasticDistortionBumps = variationObj["plasticDistortionBumps"].toInt(3);
        variation.enableLensDistortion = variationObj["enableLensDistortion"].toBool(true);
        variation.lensDistortionK1 = variationObj["lensDistortionK1"].toDouble(0.08);
        variation.lensDistortionK2 = variationObj["lensDistortionK2"].toDouble(0.02);
        variation.enableRotation = variationObj["enableRotation"].toBool(true);
        variation.maxRotationAngle = variationObj["maxRotationAngle"].toDouble(15.0);
        variation.enableTranslation = variationObj["enableTranslation"].toBool(true);
        variation.maxTranslationX = variationObj["maxTranslationX"].toDouble(50.0);
        variation.maxTranslationY = variationObj["maxTranslationY"].toDouble(50.0);
        variation.enableSkinCondition = variationObj["enableSkinCondition"].toBool(true);
        variation.skinConditionFactor = variationObj["skinConditionFactor"].toDouble(0.3);
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
