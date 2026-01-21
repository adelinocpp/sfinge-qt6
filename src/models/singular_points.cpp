#include "singular_points.h"
#include <QRandomGenerator>
#include <QDebug>
#include <cmath>
#include <random>

namespace SFinGe {

SingularPoints::SingularPoints() {
}

void SingularPoints::addCore(double x, double y) {
    m_cores.push_back(SingularPoint(x, y));
}

void SingularPoints::addDelta(double x, double y) {
    m_deltas.push_back(SingularPoint(x, y));
}

void SingularPoints::removeCore(int index) {
    if (index >= 0 && index < static_cast<int>(m_cores.size())) {
        m_cores.erase(m_cores.begin() + index);
    }
}

void SingularPoints::removeDelta(int index) {
    if (index >= 0 && index < static_cast<int>(m_deltas.size())) {
        m_deltas.erase(m_deltas.begin() + index);
    }
}

void SingularPoints::updateCore(int index, double x, double y) {
    if (index >= 0 && index < static_cast<int>(m_cores.size())) {
        m_cores[index].x = x;
        m_cores[index].y = y;
    }
}

void SingularPoints::updateDelta(int index, double x, double y) {
    if (index >= 0 && index < static_cast<int>(m_deltas.size())) {
        m_deltas[index].x = x;
        m_deltas[index].y = y;
    }
}

void SingularPoints::clearCores() {
    m_cores.clear();
}

void SingularPoints::clearDeltas() {
    m_deltas.clear();
}

void SingularPoints::clearAll() {
    m_cores.clear();
    m_deltas.clear();
}

SingularPoint SingularPoints::getCore(int index) const {
    if (index >= 0 && index < static_cast<int>(m_cores.size())) {
        return m_cores[index];
    }
    return SingularPoint();
}

SingularPoint SingularPoints::getDelta(int index) const {
    if (index >= 0 && index < static_cast<int>(m_deltas.size())) {
        return m_deltas[index];
    }
    return SingularPoint();
}

void SingularPoints::generateRandomPoints(FingerprintClass fpClass, int width, int height) {
    clearAll();
    
    qDebug() << "[SingularPoints] Generating points for class" << static_cast<int>(fpClass) << "in" << width << "x" << height;
    
    auto* rng = QRandomGenerator::global();
    
    // Melhorar seed: combinar tempo + classe + dimensões para maior variabilidade
    std::random_device rd;
    unsigned seed = rd() ^ (static_cast<unsigned>(fpClass) * 7919) ^ 
                    (static_cast<unsigned>(width) * 104729) ^ 
                    (static_cast<unsigned>(height) * 1299827);
    std::mt19937 gen(seed);
    
    // Jitter gaussiano: ±5% desvio padrão
    std::normal_distribution<double> gaussX(0.0, width * 0.075);
    std::normal_distribution<double> gaussY(0.0, height * 0.075);
    
    //TODO_APS: Parâmetros de geração de cores e deltas por tipo de impressão
    switch (fpClass) {
        case FingerprintClass::Arch:
            addDelta(width * 0.50 + gaussX(gen),
                    height * 0.35 + gaussY(gen));
            break;
            
        case FingerprintClass::TentedArch:
            addCore(width * 0.50 + gaussX(gen),
                   height * 0.28 + gaussY(gen));
            addDelta(width * 0.50 + gaussX(gen),
                    height * 0.72 + gaussY(gen));
            break;
            
        case FingerprintClass::LeftLoop:
            // Core mais à esquerda, delta mais centralizado; maior distância vertical
            addCore(width * 0.38 + gaussX(gen),
                   height * 0.32 + gaussY(gen));
            addDelta(width * 0.62 + gaussX(gen),
                    height * 0.68 + gaussY(gen));
            break;
            
        case FingerprintClass::RightLoop:
            // Core mais à direita, delta mais centralizado; maior distância vertical
            addCore(width * 0.62 + gaussX(gen),
                   height * 0.32 + gaussY(gen));
            addDelta(width * 0.38 + gaussX(gen),
                    height * 0.68 + gaussY(gen));
            break;
            
        case FingerprintClass::Whorl:
            // Plain Whorl: cores horizontais centro, deltas laterais-embaixo
            // Variar Y dos cores para criar spiral/circular/oval
            {
                double coreYvariation = (rng->generateDouble() - 0.5) * 0.08; // ±4% variação
                addCore(width * 0.42 + gaussX(gen) * 0.5,
                       height * (0.45 + coreYvariation) + gaussY(gen) * 0.3);
                addCore(width * 0.58 + gaussX(gen) * 0.5,
                       height * (0.45 - coreYvariation) + gaussY(gen) * 0.3);
                addDelta(width * 0.18 + gaussX(gen),
                        height * 0.68 + gaussY(gen));
                addDelta(width * 0.82 + gaussX(gen),
                        height * 0.68 + gaussY(gen));
            }
            break;
            
        case FingerprintClass::TwinLoop:
            // Double Loop: 2 CORES (loops lado a lado) + 2 deltas
            {
                // Deltas: afastados horizontalmente, abaixo dos cores
                double delta1X = width * 0.25 + rng->bounded(-10, 11);
                double delta2X = width * 0.75 + rng->bounded(-10, 11);
                double deltaY = height * 0.70 + rng->bounded(-8, 9);
                
                addDelta(delta1X, deltaY);
                addDelta(delta2X, deltaY);
                
                // 2 Cores: um à esquerda, outro à direita
                double core1X = width * 0.35 + rng->bounded(-10, 11);
                double core1Y = height * 0.38 + rng->bounded(-10, 11);
                addCore(core1X, core1Y);
                
                double core2X = width * 0.65 + rng->bounded(-10, 11);
                double core2Y = height * 0.38 + rng->bounded(-10, 11);
                addCore(core2X, core2Y);
            }
            break;
            
        case FingerprintClass::CentralPocket:
            // Central Pocket Loop: 2 cores muito próximos + 2 deltas laterais
            // Os cores ficam a 1-5% do tamanho da digital, com ângulo aleatório
            {
                double core1_x = width * 0.50 + gaussX(gen) * 0.5;
                double core1_y = height * 0.38 + gaussY(gen) * 0.5;
                addCore(core1_x, core1_y);
                
                // Segundo core a 1-5% de distância em ângulo aleatório
                double distance = (0.01 + rng->bounded(4) * 0.01) * std::min(width, height);
                double angle = rng->bounded(360) * M_PI / 180.0;
                double core2_x = core1_x + distance * std::cos(angle);
                double core2_y = core1_y + distance * std::sin(angle);
                addCore(core2_x, core2_y);
                
                addDelta(width * 0.25 + gaussX(gen),
                        height * 0.68 + gaussY(gen));
                addDelta(width * 0.75 + gaussX(gen),
                        height * 0.68 + gaussY(gen));
            }
            break;
            
        case FingerprintClass::Accidental:
            // Accidental Whorl: combinação de padrões (loop + whorl)
            // 2 cores em posições assimétricas + 2-3 deltas
            {
                // Core 1: posição de loop (lateral superior)
                addCore(width * 0.35 + gaussX(gen) * 0.5,
                       height * 0.30 + gaussY(gen) * 0.5);
                // Core 2: posição de whorl (central)
                addCore(width * 0.58 + gaussX(gen) * 0.5,
                       height * 0.45 + gaussY(gen) * 0.5);
                // Deltas: 2 laterais + 1 opcional central
                addDelta(width * 0.20 + gaussX(gen),
                        height * 0.65 + gaussY(gen));
                addDelta(width * 0.80 + gaussX(gen),
                        height * 0.65 + gaussY(gen));
                // Delta adicional para padrão mais complexo (50% chance)
                if (rng->bounded(2) == 0) {
                    addDelta(width * 0.50 + gaussX(gen),
                            height * 0.78 + gaussY(gen));
                }
            }
            break;
            
        default:
            addCore(width * 0.50, height * 0.45);
            addDelta(width * 0.50, height * 0.70);
            break;
    }
    
    qDebug() << "[SingularPoints] Generated" << getCoreCount() << "cores," << getDeltaCount() << "deltas";
    for (int i = 0; i < getCoreCount(); ++i) {
        qDebug() << "  Core" << i << ":" << m_cores[i].x << "," << m_cores[i].y;
    }
    for (int i = 0; i < getDeltaCount(); ++i) {
        qDebug() << "  Delta" << i << ":" << m_deltas[i].x << "," << m_deltas[i].y;
    }
}

QJsonObject SingularPoints::toJson() const {
    QJsonObject json;
    
    QJsonArray coresArray;
    for (const auto& core : m_cores) {
        QJsonObject coreObj;
        coreObj["x"] = core.x;
        coreObj["y"] = core.y;
        coresArray.append(coreObj);
    }
    json["cores"] = coresArray;
    
    QJsonArray deltasArray;
    for (const auto& delta : m_deltas) {
        QJsonObject deltaObj;
        deltaObj["x"] = delta.x;
        deltaObj["y"] = delta.y;
        deltasArray.append(deltaObj);
    }
    json["deltas"] = deltasArray;
    
    return json;
}

void SingularPoints::fromJson(const QJsonObject& json) {
    clearAll();
    
    if (json.contains("cores")) {
        QJsonArray coresArray = json["cores"].toArray();
        for (const auto& value : coresArray) {
            QJsonObject coreObj = value.toObject();
            double x = coreObj["x"].toDouble();
            double y = coreObj["y"].toDouble();
            addCore(x, y);
        }
    }
    
    if (json.contains("deltas")) {
        QJsonArray deltasArray = json["deltas"].toArray();
        for (const auto& value : deltasArray) {
            QJsonObject deltaObj = value.toObject();
            double x = deltaObj["x"].toDouble();
            double y = deltaObj["y"].toDouble();
            addDelta(x, y);
        }
    }
}

}
