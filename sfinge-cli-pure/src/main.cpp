#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <cstring>
#include <thread>
#include "core/batch_generator.h"

void printUsage() {
    std::cout << "SFINGE CLI Pure - Synthetic Fingerprint Generator (No Qt Dependencies)\n\n";
    std::cout << "Usage:\n";
    std::cout << "  sfinge-cli [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -n, --num <count>       Number of fingerprints (default: 10)\n";
    std::cout << "  -v, --versions <count>  Versions per fingerprint (default: 3)\n";
    std::cout << "  -o, --output <dir>      Output directory (default: ./output)\n";
    std::cout << "  -p, --prefix <name>     Filename prefix (default: fingerprint)\n";
    std::cout << "  -s, --start <index>     Start index (default: 0)\n";
    std::cout << "  -j, --jobs <count>      Parallel jobs (default: CPU cores)\n";
    std::cout << "  --skip-original         Skip v0 (original) images\n";
    std::cout << "  --no-mask               Disable elliptical mask\n";
    std::cout << "  --save-params           Save parameters JSON\n";
    std::cout << "  --continuous-phase     Use continuous phase (improved method)\n";
    std::cout << "  --phase-noise <level>   Phase noise level (0.0-1.0, default: 0.1)\n";
    std::cout << "  --use-quality-mask     Use quality mask for minutiae\n";
    std::cout << "  --minutiae-density <d>  Density: low/medium/high (default: low)\n";
    std::cout << "  --coherence-threshold <t> Coherence threshold (0.0-1.0, default: 0.3)\n";
    std::cout << "  --quality-window-size <s> Quality window size (default: 15)\n";
    std::cout << "  --frequency-smooth-sigma <s> Frequency smooth sigma (default: 1.5)\n";
    std::cout << "  -q, --quiet             Suppress debug output\n";
    std::cout << "  -h, --help              Show this help\n";
}

int main(int argc, char* argv[]) {
    SFinGe::BatchConfig config;
    int jobs = std::thread::hardware_concurrency();
    bool quietMode = false;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        }
        else if ((arg == "-n" || arg == "--num") && i + 1 < argc) {
            config.numFingerprints = std::stoi(argv[++i]);
        }
        else if ((arg == "-v" || arg == "--versions") && i + 1 < argc) {
            config.versionsPerFingerprint = std::stoi(argv[++i]);
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            config.outputDirectory = argv[++i];
        }
        else if ((arg == "-p" || arg == "--prefix") && i + 1 < argc) {
            config.filenamePrefix = argv[++i];
        }
        else if ((arg == "-s" || arg == "--start") && i + 1 < argc) {
            config.startIndex = std::stoi(argv[++i]);
        }
        else if ((arg == "-j" || arg == "--jobs") && i + 1 < argc) {
            jobs = std::stoi(argv[++i]);
        }
        else if (arg == "--skip-original") {
            config.skipOriginal = true;
        }
        else if (arg == "--no-mask") {
            config.applyEllipticalMask = false;
        }
        else if (arg == "--save-params") {
            config.saveParameters = true;
        }
        else if (arg == "--continuous-phase") {
            config.minutiae.useContinuousPhase = true;
        }
        else if ((arg == "--phase-noise") && i + 1 < argc) {
            config.minutiae.phaseNoiseLevel = std::stod(argv[++i]);
        }
        else if (arg == "--use-quality-mask") {
            config.minutiae.useQualityMask = true;
        }
        else if ((arg == "--minutiae-density") && i + 1 < argc) {
            config.minutiae.minutiaeDensity = argv[++i];
        }
        else if ((arg == "--coherence-threshold") && i + 1 < argc) {
            config.minutiae.coherenceThreshold = std::stod(argv[++i]);
        }
        else if ((arg == "--quality-window-size") && i + 1 < argc) {
            config.minutiae.qualityWindowSize = std::stoi(argv[++i]);
        }
        else if ((arg == "--frequency-smooth-sigma") && i + 1 < argc) {
            config.minutiae.frequencySmoothSigma = std::stod(argv[++i]);
        }
        else if (arg == "-q" || arg == "--quiet") {
            quietMode = true;
        }
    }
    
    config.quietMode = quietMode;
    
    if (jobs < 1) jobs = 1;
    
    std::cout << "=== SFINGE CLI Pure - Batch Generation ===\n";
    std::cout << "Fingerprints: " << config.numFingerprints << "\n";
    std::cout << "Versions per FP: " << config.versionsPerFingerprint << "\n";
    std::cout << "Skip original: " << (config.skipOriginal ? "yes" : "no") << "\n";
    std::cout << "Output: " << config.outputDirectory << "\n";
    std::cout << "Parallel jobs: " << jobs << "\n";
    std::cout << "Minutiae method: " << (config.minutiae.useContinuousPhase ? "continuous phase" : "original") << "\n";
    if (config.minutiae.useContinuousPhase) {
        std::cout << "Phase noise: " << config.minutiae.phaseNoiseLevel << "\n";
        std::cout << "Quality mask: " << (config.minutiae.useQualityMask ? "enabled" : "disabled") << "\n";
        std::cout << "Minutiae density: " << config.minutiae.minutiaeDensity << "\n";
    }
    std::cout << "==========================================\n\n";
    
    SFinGe::BatchGenerator generator;
    generator.setBatchConfig(config);
    generator.setNumWorkers(jobs);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastUpdateTime = startTime;
    const auto updateInterval = std::chrono::seconds(5);
    
    generator.setProgressCallback([&](int fpCompleted, int totalFps, int imgCount) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        
        // Update every 5 seconds or on last fingerprint
        auto sinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime);
        if (sinceLastUpdate >= updateInterval || fpCompleted == totalFps) {
            lastUpdateTime = now;
            
            double elapsedSec = elapsed / 1000.0;
            double avgTimePerFp = (fpCompleted > 0) ? elapsedSec / fpCompleted : 0;
            double remaining = avgTimePerFp * (totalFps - fpCompleted);
            int remainingSec = static_cast<int>(remaining);
            
            double imgsPerSec = (elapsed > 0) ? (imgCount * 1000.0 / elapsed) : 0;
            
            std::cout << "\rFP [" << fpCompleted << "/" << totalFps << "] "
                      << "Images: " << imgCount << " | "
                      << std::fixed << std::setprecision(2) << imgsPerSec << " img/s | "
                      << "Elapsed: " << static_cast<int>(elapsedSec) << "s, ETA: "
                      << remainingSec / 60 << ":" << std::setfill('0') << std::setw(2) << remainingSec % 60
                      << "          " << std::flush;
        }
    });
    
    bool success = generator.generateBatch();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    std::cout << "\n\nBatch completed!\n";
    std::cout << "Elapsed time: " << totalMs / 1000 << "." << totalMs % 1000 << " seconds\n";
    
    return success ? 0 : 1;
}
