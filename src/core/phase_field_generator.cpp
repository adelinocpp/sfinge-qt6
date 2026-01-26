#include "phase_field_generator.h"
#include <cmath>
#include <random>

namespace SFinGe {

PhaseFieldGenerator::PhaseFieldGenerator() {
    std::random_device rd;
    m_rng.seed(rd());
}

PhaseFieldGenerator::~PhaseFieldGenerator() {
}

std::vector<std::vector<double>> PhaseFieldGenerator::generate(
    const std::vector<std::vector<double>>& orientationField,
    const std::vector<std::vector<double>>& frequencyField,
    int dpi
) {
    int height = orientationField.size();
    int width = orientationField[0].size();
    
    // Inicializa campo de fase
    std::vector<std::vector<double>> phaseField(
        height, std::vector<double>(width, 0.0)
    );
    
    // Conversão: frequência (cristas/mm) → fase/pixel
    double mmPerPixel = 25.4 / dpi;
    
    // PASSO 1: Integração horizontal (linha por linha)
    for (int y = 0; y < height; y++) {
        for (int x = 1; x < width; x++) {
            double theta = orientationField[y][x];
            double freq = frequencyField[y][x];
            
            // Componente dx ao longo da crista
            // θ é perpendicular às cristas, então usamos cos(θ)
            double dxComponent = std::cos(theta);
            
            // Incremento de fase
            double freqPerPixel = freq * mmPerPixel;
            double phaseIncrement = 2.0 * M_PI * freqPerPixel * dxComponent;
            
            // Acumula fase
            phaseField[y][x] = phaseField[y][x-1] + phaseIncrement;
        }
    }
    
    // PASSO 2: Ajusta transições verticais para continuidade
    smoothVerticalTransitions(phaseField);
    
    // PASSO 3: Adiciona variação controlada (muito menor que aleatória!)
    if (m_noiseLevel > 0.0) {
        std::normal_distribution<> dist(0.0, m_noiseLevel);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                phaseField[y][x] += dist(m_rng);
            }
        }
    }
    
    return phaseField;
}

void PhaseFieldGenerator::setNoiseLevel(double noiseLevel) {
    m_noiseLevel = noiseLevel;
}

void PhaseFieldGenerator::integrateLine(
    std::vector<double>& phaseLine,
    const std::vector<double>& orientationLine,
    const std::vector<double>& frequencyLine,
    double pixelSpacing
) {
    // Implementação da integração horizontal
    for (size_t x = 1; x < phaseLine.size(); x++) {
        double theta = orientationLine[x];
        double freq = frequencyLine[x];
        
        double dxComponent = std::cos(theta);
        double freqPerPixel = freq * pixelSpacing;
        double phaseIncrement = 2.0 * M_PI * freqPerPixel * dxComponent;
        
        phaseLine[x] = phaseLine[x-1] + phaseIncrement;
    }
}

void PhaseFieldGenerator::smoothVerticalTransitions(
    std::vector<std::vector<double>>& phaseField
) {
    int height = phaseField.size();
    int width = phaseField[0].size();
    
    // Ajusta cada linha para ter continuidade com a anterior
    for (int y = 1; y < height; y++) {
        // Calcula offset médio entre linhas
        double totalOffset = 0.0;
        int samples = 0;
        
        for (int x = 0; x < width; x += 10) {  // Amostragem esparsa
            double diff = phaseField[y][x] - phaseField[y-1][x];
            totalOffset += diff;
            samples++;
        }
        
        if (samples > 0) {
            double avgOffset = totalOffset / samples;
            
            // Remove offset médio da linha atual
            for (int x = 0; x < width; x++) {
                phaseField[y][x] -= avgOffset;
            }
        }
    }
}

}
