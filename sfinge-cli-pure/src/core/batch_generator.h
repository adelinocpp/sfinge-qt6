#ifndef BATCH_GENERATOR_H
#define BATCH_GENERATOR_H

#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <random>
#include <functional>
#include "image.h"
#include "fingerprint_generator.h"
#include "models/singular_points.h"
#include "models/fingerprint_parameters.h"

namespace SFinGe {

struct VersionTransform {
    double rotation = 0.0;
    double noiseLevel = 0.0;
    double lensDistortion = 0.0;
    bool usePincushion = true;
    double homographyShiftX = 0.0;
    double homographyShiftY = 0.0;
    double homographyAngle = 0.0;
    int cropWidth = 500;
    int cropHeight = 600;
    bool applyBlur = false;
    int blurRadius = 0;
    double blurCenterX = 0.0;
    double blurCenterY = 0.0;
};

struct BatchConfig {
    int numFingerprints = 10;
    int versionsPerFingerprint = 3;
    int startIndex = 0;
    bool usePopulationDistribution = true;
    bool skipOriginal = true;
    bool applyEllipticalMask = true;
    bool quietMode = false;
    
    std::string outputDirectory = "./output";
    std::string filenamePrefix = "fingerprint";
    bool saveParameters = false;
};

struct FingerprintInstance {
    FingerprintParameters baseParams;
    SingularPoints basePoints;
    std::string identifier;
};

class BatchGenerator {
public:
    BatchGenerator();
    
    void setBatchConfig(const BatchConfig& config);
    void setNumWorkers(int workers) { m_numWorkers = workers; }
    
    bool generateBatch();
    void cancel() { m_cancelled = true; }
    
    using ProgressCallback = std::function<void(int, int, int)>;
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }

private:
    FingerprintInstance createBaseFingerprint(int index);
    VersionTransform generateVersionTransform(int versionIndex);
    VersionTransform generateVersionTransformLocal(int versionIndex, std::mt19937& rng);
    Image applyVersionTransforms(const Image& baseImage, const VersionTransform& transform);
    Image applyNoise(const Image& image, double noiseLevel);
    Image applyBlur(const Image& image, int radius, double centerX, double centerY);
    Image applyLensDistortion(const Image& image, double k);
    Image applyHomography(const Image& image, double shiftX, double shiftY, double angle);
    Image applyRotation(const Image& image, double angle);
    Image applyCrop(const Image& image, int targetWidth, int targetHeight);
    Image applyEllipticalMask(const Image& image);
    bool saveFingerprint(const Image& image, const FingerprintInstance& instance, int fpIndex, int versionIndex);
    FingerprintClass selectClassByPopulation();
    
    BatchConfig m_config;
    int m_numWorkers = 0;
    std::atomic<bool> m_cancelled{false};
    std::atomic<int> m_generated{0};
    std::mutex m_mutex;
    std::mt19937 m_rng;
    
    ProgressCallback m_progressCallback;
};

} // namespace SFinGe

#endif // BATCH_GENERATOR_H
