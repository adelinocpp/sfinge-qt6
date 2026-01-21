#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include <cmath>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include "core/fingerprint_generator_pure.h"
#include "models/fingerprint_parameters.h"
#include "models/singular_points.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct BatchConfig {
    int numFingerprints = 10;
    int versionsPerFingerprint = 3;
    std::string outputDirectory = "./output";
    std::string filenamePrefix = "fingerprint";
    int startIndex = 0;
    bool skipOriginal = true;
    bool quietMode = false;
    int numJobs = std::thread::hardware_concurrency();
};

struct FingerprintInstance {
    SFinGe::FingerprintParameters baseParams;
    SingularPoints basePoints;
    std::string identifier;
};

void printUsage() {
    std::cout << "SFINGE-Pure CLI - Synthetic Fingerprint Generator (No Qt)\n\n";
    std::cout << "Usage:\n";
    std::cout << "  sfinge-pure [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -n, --num <count>       Number of fingerprints (default: 10)\n";
    std::cout << "  -v, --versions <count>  Versions per fingerprint (default: 3)\n";
    std::cout << "  -o, --output <dir>      Output directory (default: ./output)\n";
    std::cout << "  -p, --prefix <name>     Filename prefix (default: fingerprint)\n";
    std::cout << "  -s, --start <index>     Start index (default: 0)\n";
    std::cout << "  -j, --jobs <count>      Parallel jobs (default: CPU cores)\n";
    std::cout << "  --skip-original         Skip v0 (original) images\n";
    std::cout << "  -q, --quiet             Suppress debug, show only elapsed time\n";
    std::cout << "  -h, --help              Show this help\n";
}

bool createDirectory(const std::string& path) {
    struct stat st = {0};
    if (stat(path.c_str(), &st) == -1) {
        return mkdir(path.c_str(), 0755) == 0;
    }
    return true;
}

FingerprintInstance createBaseFingerprint(int index) {
    FingerprintInstance instance;
    
    instance.identifier = "FP_" + std::to_string(index + 1);
    
    instance.baseParams.reset();
    
    // Random shape parameters
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> shapeDist(-30, 30);
    
    instance.baseParams.shape.left = 500 + shapeDist(gen);
    instance.baseParams.shape.right = 500 + shapeDist(gen);
    instance.baseParams.shape.top = 480 + shapeDist(gen);
    instance.baseParams.shape.middle = 240 + shapeDist(gen);
    instance.baseParams.shape.bottom = 480 + shapeDist(gen);
    
    int width = instance.baseParams.shape.left + instance.baseParams.shape.right;
    int height = instance.baseParams.shape.top + instance.baseParams.shape.middle + 
                 instance.baseParams.shape.bottom;
    
    // Select fingerprint class (simplified - always RightLoop for now)
    FingerprintClass selectedClass = FingerprintClass::RightLoop;
    instance.baseParams.classification.fingerprintClass = selectedClass;
    
    // Generate singular points
    instance.basePoints.generateRandomPoints(selectedClass, width, height);
    
    // Apply Gaussian variation to alphas
    std::normal_distribution<double> coreAlphaDist(1.0, 0.025);
    std::normal_distribution<double> deltaAlphaDist(-1.0, 0.025);
    
    for (int i = 0; i < instance.basePoints.getCoreCount(); ++i) {
        double newAlpha = coreAlphaDist(gen);
        instance.basePoints.updateCoreAlpha(i, newAlpha);
    }
    
    for (int i = 0; i < instance.basePoints.getDeltaCount(); ++i) {
        double newAlpha = deltaAlphaDist(gen);
        instance.basePoints.updateDeltaAlpha(i, newAlpha);
    }
    
    return instance;
}

bool saveFingerprint(const SFinGe::ImageBuffer& fingerprint, 
                    const FingerprintInstance& instance, 
                    int fpIndex, 
                    int version) {
    std::string filename = instance.identifier + "_v" + std::to_string(version) + ".pgm";
    std::string filepath = instance.identifier + "_" + std::to_string(version) + ".pgm";
    
    return fingerprint.saveToFile(filepath.c_str());
}

void generateBatch(const BatchConfig& config) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Create output directory
    if (!createDirectory(config.outputDirectory)) {
        std::cerr << "Error: Failed to create output directory\n";
        return;
    }
    
    int imagesPerFingerprint = config.versionsPerFingerprint + (config.skipOriginal ? 0 : 1);
    int totalImages = config.numFingerprints * imagesPerFingerprint;
    
    if (!config.quietMode) {
        std::cout << "=== SFINGE-Pure CLI Batch Generation ===\n";
        std::cout << "Fingerprints: " << config.numFingerprints << "\n";
        std::cout << "Versions per FP: " << config.versionsPerFingerprint << "\n";
        std::cout << "Skip original: " << (config.skipOriginal ? "yes" : "no") << "\n";
        std::cout << "Output: " << config.outputDirectory << "\n";
        std::cout << "Parallel jobs: " << config.numJobs << "\n";
        std::cout << "===================================\n\n";
    }
    
    int generated = 0;
    
    for (int i = 0; i < config.numFingerprints; ++i) {
        // Create base fingerprint
        FingerprintInstance instance = createBaseFingerprint(i);
        
        // Generate fingerprint
        SFinGe::FingerprintGeneratorPure generator;
        generator.setParameters(instance.baseParams);
        generator.setSingularPoints(instance.basePoints);
        
        SFinGe::ImageBuffer fingerprint = generator.generateFingerprint();
        
        // Generate versions
        int startVersion = config.skipOriginal ? 1 : 0;
        for (int version = startVersion; version <= config.versionsPerFingerprint; ++version) {
            // Apply simple transformations for different versions
            SFinGe::ImageBuffer versionFingerprint = fingerprint; // Copy
            
            // Save fingerprint
            if (saveFingerprint(versionFingerprint, instance, i, version)) {
                generated++;
            }
            
            // Progress update
            if (!config.quietMode && generated > 0) {
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
                
                double avgTimePerImage = static_cast<double>(elapsed) / generated;
                int remainingImages = totalImages - generated;
                int eta = static_cast<int>(avgTimePerImage * remainingImages);
                
                std::cout << "\r[" << generated << "/" << totalImages << "] "
                          << "Elapsed: " << elapsed << "s, ETA: " 
                          << eta / 60 << ":" << std::setfill('0') << std::setw(2) << eta % 60
                          << "          " << std::flush;
            }
        }
        
        if (!config.quietMode) {
            std::cout << "\nCompleted fingerprint " << (i + 1) << "/" << config.numFingerprints << "\n";
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    if (!config.quietMode) {
        std::cout << "\nBatch completed! Generated " << generated << " images.\n";
        std::cout << "Elapsed time: " << totalTime / 1000.0 << " seconds\n";
    }
}

int main(int argc, char* argv[]) {
    BatchConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "-n" || arg == "--num") {
            if (i + 1 < argc) {
                config.numFingerprints = std::atoi(argv[++i]);
            }
        } else if (arg == "-v" || arg == "--versions") {
            if (i + 1 < argc) {
                config.versionsPerFingerprint = std::atoi(argv[++i]);
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                config.outputDirectory = argv[++i];
            }
        } else if (arg == "-p" || arg == "--prefix") {
            if (i + 1 < argc) {
                config.filenamePrefix = argv[++i];
            }
        } else if (arg == "-s" || arg == "--start") {
            if (i + 1 < argc) {
                config.startIndex = std::atoi(argv[++i]);
            }
        } else if (arg == "-j" || arg == "--jobs") {
            if (i + 1 < argc) {
                config.numJobs = std::atoi(argv[++i]);
            }
        } else if (arg == "--skip-original") {
            config.skipOriginal = true;
        } else if (arg == "-q" || arg == "--quiet") {
            config.quietMode = true;
        }
    }
    
    generateBatch(config);
    
    return 0;
}
