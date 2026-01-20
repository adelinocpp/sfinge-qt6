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
    json["orientation"] = orientationObj;
    
    QJsonObject ridgeObj;
    ridgeObj["gaborFilterSize"] = ridge.gaborFilterSize;
    ridgeObj["cacheDegrees"] = ridge.cacheDegrees;
    ridgeObj["cacheFrequencies"] = ridge.cacheFrequencies;
    ridgeObj["maxIterations"] = ridge.maxIterations;
    json["ridge"] = ridgeObj;
    
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
    }
    
    if (json.contains("ridge")) {
        QJsonObject ridgeObj = json["ridge"].toObject();
        ridge.gaborFilterSize = ridgeObj["gaborFilterSize"].toInt(25);
        ridge.cacheDegrees = ridgeObj["cacheDegrees"].toInt(36);
        ridge.cacheFrequencies = ridgeObj["cacheFrequencies"].toInt(10);
        ridge.maxIterations = ridgeObj["maxIterations"].toInt(50);
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
