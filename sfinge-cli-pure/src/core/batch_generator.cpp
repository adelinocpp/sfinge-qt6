#include "batch_generator.h"
#include <thread>
#include <queue>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SFinGe {

BatchGenerator::BatchGenerator() : m_rng(std::random_device{}()) {}

void BatchGenerator::setBatchConfig(const BatchConfig& config) {
    m_config = config;
}

FingerprintClass BatchGenerator::selectClassByPopulation() {
    // Population distribution (approximate):
    // Loops: 60-65%, Whorls: 30-35%, Arches: 5%
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(m_rng);
    
    if (r < 0.025) return FingerprintClass::Arch;
    if (r < 0.05) return FingerprintClass::TentedArch;
    if (r < 0.35) return FingerprintClass::LeftLoop;
    if (r < 0.65) return FingerprintClass::RightLoop;
    if (r < 0.80) return FingerprintClass::Whorl;
    if (r < 0.90) return FingerprintClass::TwinLoop;
    if (r < 0.95) return FingerprintClass::CentralPocket;
    return FingerprintClass::Accidental;
}

FingerprintInstance BatchGenerator::createBaseFingerprint(int index) {
    FingerprintInstance instance;
    
    char buf[32];
    snprintf(buf, sizeof(buf), "FP_%03d", index + 1);
    instance.identifier = buf;
    
    instance.baseParams.reset();
    
    std::uniform_int_distribution<int> shapeDist(-30, 30);
    instance.baseParams.shape.left = 500 + shapeDist(m_rng);
    instance.baseParams.shape.right = 500 + shapeDist(m_rng);
    instance.baseParams.shape.top = 480 + shapeDist(m_rng);
    instance.baseParams.shape.middle = 240 + shapeDist(m_rng) / 2;
    instance.baseParams.shape.bottom = 480 + shapeDist(m_rng);
    
    int width = instance.baseParams.shape.left + instance.baseParams.shape.right;
    int height = instance.baseParams.shape.top + instance.baseParams.shape.middle + 
                 instance.baseParams.shape.bottom;
    
    FingerprintClass selectedClass = selectClassByPopulation();
    
    instance.basePoints.generateRandomPoints(selectedClass, width, height);
    instance.baseParams.classification.fingerprintClass = selectedClass;
    
    instance.baseParams.orientation.loopEdgeBlendFactor = 0.0;
    instance.baseParams.orientation.whorlEdgeDecayFactor = 0.0;
    instance.baseParams.orientation.quietMode = m_config.quietMode;
    
    return instance;
}

VersionTransform BatchGenerator::generateVersionTransform(int versionIndex) {
    return generateVersionTransformLocal(versionIndex, m_rng);
}

VersionTransform BatchGenerator::generateVersionTransformLocal(int versionIndex, std::mt19937& rng) {
    VersionTransform transform;
    
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    
    // Rotation (-15 to +15 degrees)
    transform.rotation = (dist01(rng) - 0.5) * 30.0;
    
    // Noise (0.03 to 0.08)
    transform.noiseLevel = 0.03 + dist01(rng) * 0.05;
    
    // Lens distortion (-0.16 to +0.16)
    transform.usePincushion = true;
    double magnitude = 0.08 + dist01(rng) * 0.08;
    transform.lensDistortion = (dist01(rng) < 0.5) ? -magnitude : magnitude;
    
    // Homography shift (-20 to +20 pixels)
    transform.homographyShiftX = (dist01(rng) - 0.5) * 40.0;
    transform.homographyShiftY = (dist01(rng) - 0.5) * 40.0;
    
    // Homography angle (-10 to +10 degrees)
    transform.homographyAngle = (dist01(rng) - 0.5) * 20.0;
    
    // Crop region - aumentado para preservar mais área útil
    transform.cropWidth = 750;
    transform.cropHeight = 900;
    
    // Blur
    transform.applyBlur = true;
    std::uniform_int_distribution<int> blurDist(25, 150);
    transform.blurRadius = blurDist(rng);
    transform.blurCenterX = 50.0 + dist01(rng) * 400.0;
    transform.blurCenterY = 50.0 + dist01(rng) * 500.0;
    
    return transform;
}

Image BatchGenerator::applyNoise(const Image& image, double noiseLevel) {
    Image result = image.copy();
    std::uniform_real_distribution<double> dist(-0.5, 0.5);
    
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            uint8_t gray = result.pixel(x, y);
            double noise = dist(m_rng) * 255.0 * noiseLevel;
            int newGray = std::clamp(static_cast<int>(gray + noise), 0, 255);
            result.setPixel(x, y, static_cast<uint8_t>(newGray));
        }
    }
    
    return result;
}

Image BatchGenerator::applyBlur(const Image& image, int radius, double centerX, double centerY) {
    Image result = image.copy();
    
    const double kernel[3][3] = {
        {1.0/16, 2.0/16, 1.0/16},
        {2.0/16, 4.0/16, 2.0/16},
        {1.0/16, 2.0/16, 1.0/16}
    };
    
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            double dx = x - centerX;
            double dy = y - centerY;
            double dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist <= radius) {
                double blurIntensity = 1.0 - (dist / radius);
                
                double sum = 0;
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int px = std::clamp(x + kx, 0, image.width() - 1);
                        int py = std::clamp(y + ky, 0, image.height() - 1);
                        sum += image.pixel(px, py) * kernel[ky + 1][kx + 1];
                    }
                }
                
                uint8_t original = image.pixel(x, y);
                int blurred = static_cast<int>(original * (1-blurIntensity) + sum * blurIntensity);
                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(blurred, 0, 255)));
            }
        }
    }
    
    return result;
}

Image BatchGenerator::applyLensDistortion(const Image& image, double k) {
    Image result(image.width(), image.height());
    result.fill(255);
    
    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double nx = (x - cx) / cx;
            double ny = (y - cy) / cy;
            double r = std::sqrt(nx * nx + ny * ny);
            
            if (r < 0.001) {
                result.setPixel(x, y, image.pixel(x, y));
                continue;
            }
            
            double rDistorted = r * (1.0 + k * r * r);
            
            double srcX = cx + (nx / r) * rDistorted * cx;
            double srcY = cy + (ny / r) * rDistorted * cy;
            
            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;
                
                double p00 = image.pixel(x0, y0);
                double p10 = image.pixel(x0 + 1, y0);
                double p01 = image.pixel(x0, y0 + 1);
                double p11 = image.pixel(x0 + 1, y0 + 1);
                
                double gray = p00 * (1 - fx) * (1 - fy) +
                             p10 * fx * (1 - fy) +
                             p01 * (1 - fx) * fy +
                             p11 * fx * fy;
                
                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(gray, 0.0, 255.0)));
            }
        }
    }
    
    return result;
}

Image BatchGenerator::applyHomography(const Image& image, double shiftX, double shiftY, double angle) {
    Image result(image.width(), image.height());
    result.fill(255);
    
    double rad = angle * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);
    
    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double nx = x - cx;
            double ny = y - cy;
            
            double srcX = nx * cosA - ny * sinA * 0.3 + shiftX + cx;
            double srcY = nx * sinA * 0.3 + ny * cosA + shiftY + cy;
            
            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;
                
                double p00 = image.pixel(x0, y0);
                double p10 = image.pixel(x0 + 1, y0);
                double p01 = image.pixel(x0, y0 + 1);
                double p11 = image.pixel(x0 + 1, y0 + 1);
                
                double gray = p00 * (1 - fx) * (1 - fy) +
                             p10 * fx * (1 - fy) +
                             p01 * (1 - fx) * fy +
                             p11 * fx * fy;
                
                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(gray, 0.0, 255.0)));
            }
        }
    }
    
    return result;
}

Image BatchGenerator::applyRotation(const Image& image, double angle) {
    Image result(image.width(), image.height());
    result.fill(255);
    
    double rad = angle * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);
    
    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double nx = x - cx;
            double ny = y - cy;
            
            double srcX = nx * cosA + ny * sinA + cx;
            double srcY = -nx * sinA + ny * cosA + cy;
            
            if (srcX >= 0 && srcX < width - 1 && srcY >= 0 && srcY < height - 1) {
                int x0 = static_cast<int>(srcX);
                int y0 = static_cast<int>(srcY);
                double fx = srcX - x0;
                double fy = srcY - y0;
                
                double p00 = image.pixel(x0, y0);
                double p10 = image.pixel(x0 + 1, y0);
                double p01 = image.pixel(x0, y0 + 1);
                double p11 = image.pixel(x0 + 1, y0 + 1);
                
                double gray = p00 * (1 - fx) * (1 - fy) +
                             p10 * fx * (1 - fy) +
                             p01 * (1 - fx) * fy +
                             p11 * fx * fy;
                
                result.setPixel(x, y, static_cast<uint8_t>(std::clamp(gray, 0.0, 255.0)));
            }
        }
    }
    
    return result;
}

Image BatchGenerator::applyCrop(const Image& image, int targetWidth, int targetHeight) {
    int startX = (image.width() - targetWidth) / 2;
    int startY = (image.height() - targetHeight) / 2;
    
    startX = std::max(0, startX);
    startY = std::max(0, startY);
    
    Image result(targetWidth, targetHeight);
    result.fill(255);
    
    for (int y = 0; y < targetHeight && (startY + y) < image.height(); ++y) {
        for (int x = 0; x < targetWidth && (startX + x) < image.width(); ++x) {
            result.setPixel(x, y, image.pixel(startX + x, startY + y));
        }
    }
    
    return result;
}

Image BatchGenerator::applyEllipticalMask(const Image& image) {
    Image result = image.copy();
    
    int width = image.width();
    int height = image.height();
    double cx = width / 2.0;
    double cy = height / 2.0;
    double rx = width / 2.0 * 0.95;
    double ry = height / 2.0 * 0.95;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double dx = (x - cx) / rx;
            double dy = (y - cy) / ry;
            double dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist > 1.0) {
                result.setPixel(x, y, 255);
            } else if (dist > 0.85) {
                double fade = (dist - 0.85) / 0.15;
                uint8_t original = image.pixel(x, y);
                uint8_t blended = static_cast<uint8_t>(original * (1 - fade) + 255 * fade);
                result.setPixel(x, y, blended);
            }
        }
    }
    
    return result;
}

Image BatchGenerator::applyVersionTransforms(const Image& baseImage, const VersionTransform& transform) {
    Image result = baseImage.copy();
    
    if (transform.noiseLevel > 0.001) {
        result = applyNoise(result, transform.noiseLevel);
    }
    
    if (transform.applyBlur && transform.blurRadius > 0) {
        result = applyBlur(result, transform.blurRadius, transform.blurCenterX, transform.blurCenterY);
    }
    
    if (std::abs(transform.lensDistortion) > 0.001) {
        result = applyLensDistortion(result, transform.lensDistortion);
    }
    
    if (std::abs(transform.homographyAngle) > 0.1 || 
        std::abs(transform.homographyShiftX) > 0.1 || 
        std::abs(transform.homographyShiftY) > 0.1) {
        result = applyHomography(result, transform.homographyShiftX, 
                                 transform.homographyShiftY, transform.homographyAngle);
    }
    
    if (std::abs(transform.rotation) > 0.1) {
        result = applyRotation(result, transform.rotation);
    }
    
    result = applyCrop(result, transform.cropWidth, transform.cropHeight);
    result.setDPI(500);
    
    return result;
}

bool BatchGenerator::saveFingerprint(const Image& image, const FingerprintInstance& instance, 
                                     int fpIndex, int versionIndex) {
    char filename[512];
    int actualIndex = m_config.startIndex + fpIndex;
    snprintf(filename, sizeof(filename), "%s/%s_%04d_v%02d.png",
             m_config.outputDirectory.c_str(),
             m_config.filenamePrefix.c_str(),
             actualIndex, versionIndex);
    
    return image.save(filename);
}

bool BatchGenerator::generateBatch() {
    m_cancelled = false;
    m_generated = 0;
    
    // Create output directory
    std::filesystem::create_directories(m_config.outputDirectory);
    
    int numWorkers = m_numWorkers > 0 ? m_numWorkers : std::thread::hardware_concurrency();
    if (numWorkers < 1) numWorkers = 1;
    
    if (!m_config.quietMode) {
        std::cout << "Starting parallel batch generation with " << numWorkers << " workers\n";
        std::cout << "Total fingerprints: " << m_config.numFingerprints << "\n";
    }
    
    // Pre-create all fingerprint instances
    std::vector<FingerprintInstance> instances(m_config.numFingerprints);
    for (int i = 0; i < m_config.numFingerprints && !m_cancelled; ++i) {
        instances[i] = createBaseFingerprint(i);
    }
    
    // Task queue
    std::queue<int> taskQueue;
    for (int i = 0; i < m_config.numFingerprints; ++i) {
        taskQueue.push(i);
    }
    
    std::mutex queueMutex;
    std::atomic<int> completedFps(0);
    
    // Worker function - cada thread tem seu próprio RNG para evitar race conditions
    auto workerFunc = [&]() {
        // RNG local por thread para evitar race conditions
        std::random_device rd;
        std::mt19937 localRng(rd());
        
        while (!m_cancelled) {
            int taskIndex;
            bool hasTask = false;
            
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!taskQueue.empty()) {
                    taskIndex = taskQueue.front();
                    taskQueue.pop();
                    hasTask = true;
                }
            }
            
            if (!hasTask) break;
            
            // CRIAR NOVO GENERATOR PARA CADA TAREFA para garantir isolamento total
            FingerprintGenerator localGenerator;
            
            // Configure generator
            localGenerator.setParameters(instances[taskIndex].baseParams);
            localGenerator.setSingularPoints(instances[taskIndex].basePoints);
            
            // Generate base image
            Image baseFingerprint = localGenerator.generateFingerprint();
            
            // Generate all versions
            int startIdx = m_config.skipOriginal ? 1 : 0;
            for (int verIdx = startIdx; verIdx <= m_config.versionsPerFingerprint && !m_cancelled; ++verIdx) {
                Image transformedFingerprint;
                
                if (verIdx == 0) {
                    transformedFingerprint = baseFingerprint.copy();
                } else {
                    VersionTransform transform = generateVersionTransformLocal(verIdx, localRng);
                    transformedFingerprint = applyVersionTransforms(baseFingerprint, transform);
                }
                
                if (m_config.applyEllipticalMask) {
                    transformedFingerprint = applyEllipticalMask(transformedFingerprint);
                }
                
                if (!saveFingerprint(transformedFingerprint, instances[taskIndex], taskIndex, verIdx)) {
                    continue;
                }
                
                m_generated.fetch_add(1);
            }
            
            int fpCompleted = completedFps.fetch_add(1) + 1;
            
            if (m_progressCallback) {
                m_progressCallback(fpCompleted, m_config.numFingerprints, m_generated.load());
            }
        }
    };
    
    // Create and start threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numWorkers; ++i) {
        threads.emplace_back(workerFunc);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    return !m_cancelled;
}

} // namespace SFinGe
