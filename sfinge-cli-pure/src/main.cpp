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

    std::cout << "=== BATCH CONTROL ===\n";
    std::cout << "  -n, --num <N>              Number of fingerprints (default: 10)\n";
    std::cout << "  -v, --versions <N>         Versions per fingerprint (default: 3)\n";
    std::cout << "  -o, --output <dir>         Output directory (default: ./output)\n";
    std::cout << "  -p, --prefix <name>        Filename prefix (default: fingerprint)\n";
    std::cout << "  -s, --start <N>            Start index (default: 0)\n";
    std::cout << "  -j, --jobs <N>             Parallel jobs (default: CPU cores)\n";
    std::cout << "  --skip-original            Skip v0 (original, no distortions) images\n";
    std::cout << "  --include-original         Include v0 (original) images [enables skipOriginal=false]\n";
    std::cout << "  --no-mask                  Disable shape mask\n";
    std::cout << "  --save-params              Save parameters JSON\n";
    std::cout << "  --seed <N>                 Global RNG seed for reproducibility (0=random)\n";
    std::cout << "  -q, --quiet                Suppress debug output\n";
    std::cout << "  -h, --help                 Show this help\n\n";

    std::cout << "=== SHAPE ===\n";
    std::cout << "  --shape-left <N>           Left semi-width (default: 500, randomized ±20)\n";
    std::cout << "  --shape-right <N>          Right semi-width (default: 500)\n";
    std::cout << "  --shape-top <N>            Top semi-height (default: 480)\n";
    std::cout << "  --shape-middle <N>         Middle rectangle height (default: 240)\n";
    std::cout << "  --shape-bottom <N>         Bottom semi-height (default: 480)\n";
    std::cout << "  --finger-type <t>          Finger type: thumb/index/middle/ring/little (default: index)\n\n";

    std::cout << "=== FINGERPRINT CLASS ===\n";
    std::cout << "  --class <c>                Class: arch/tented-arch/left-loop/right-loop/\n";
    std::cout << "                             whorl/twin-loop/central-pocket/accidental (default: random)\n\n";

    std::cout << "=== DENSITY ===\n";
    std::cout << "  --density-min-freq <f>     Min ridge frequency (default: 0.0833)\n";
    std::cout << "  --density-max-freq <f>     Max ridge frequency (default: 0.1111)\n";
    std::cout << "  --density-zoom <f>         Perlin noise zoom (default: 2.0)\n";
    std::cout << "  --density-amplify <f>      Perlin noise amplification (default: 1.5)\n\n";

    std::cout << "=== RIDGE / GABOR ===\n";
    std::cout << "  --gabor-size <N>           Gabor filter half-size, kernel=(2N+1)x(2N+1) (default: 10)\n";
    std::cout << "  --cache-degrees <N>        Orientation cache resolution in degrees (default: 36)\n";
    std::cout << "  --cache-freqs <N>          Frequency cache bins (default: 20)\n";
    std::cout << "  --max-iterations <N>       Max Gabor iterations (default: 180)\n";
    std::cout << "  --seed-density <f>         Initial Gabor seed density (default: 0.001)\n\n";

    std::cout << "=== ORIENTATION ===\n";
    std::cout << "  --orientation-method <m>   Method: poincare/fomfe/poincare-smoothed (default: poincare)\n";
    std::cout << "  --smooth-sigma <f>         Orientation smoothing sigma (default: 6.0)\n";
    std::cout << "  --vertical-bias <f>        Vertical bias strength (default: 0.0)\n";
    std::cout << "  --vertical-bias-radius <f> Vertical bias radius (default: 100.0)\n";
    std::cout << "  --core-convergence <f>     Core convergence strength (default: 0.2)\n";
    std::cout << "  --core-convergence-radius <f> Core convergence radius (default: 50.0)\n";
    std::cout << "  --core-convergence-prob <f> Core convergence probability (default: 0.3)\n";
    std::cout << "  --anisotropy-x <f>         Anisotropy factor X (default: 1.0)\n";
    std::cout << "  --anisotropy-y <f>         Anisotropy factor Y (default: 1.0)\n";
    std::cout << "  --arch-amplitude <f>       Arch waveform amplitude (default: 0.22)\n";
    std::cout << "  --tented-arch-decay <f>    Tented arch peak decay (default: 0.12)\n";
    std::cout << "  --loop-bias <f>            Loop vertical bias strength (default: 0.4)\n";
    std::cout << "  --loop-edge-blend <f>      Loop edge blend factor (default: 0.4)\n";
    std::cout << "  --loop-bias-radius <f>     Loop bias radius factor (default: 1.5)\n";
    std::cout << "  --whorl-spiral <f>         Whorl spiral factor (default: 0.12)\n";
    std::cout << "  --whorl-edge-decay <f>     Whorl edge decay factor (default: 0.18)\n";
    std::cout << "  --twin-loop-smooth <f>     Twin loop smoothing sigma (default: 7.0)\n";
    std::cout << "  --central-pocket-conc <f>  Central pocket concentration (default: 0.06)\n";
    std::cout << "  --accidental-irregularity <f> Accidental irregularity (default: 0.08)\n";
    std::cout << "  --fomfe-order-m <N>        FOMFE order M (default: 5)\n";
    std::cout << "  --fomfe-order-n <N>        FOMFE order N (default: 5)\n\n";

    std::cout << "=== RENDERING — TEXTURE NOISE ===\n";
    std::cout << "  --bg-noise-freq <f>        Background noise frequency (default: 0.03)\n";
    std::cout << "  --bg-noise-amp <f>         Background noise amplitude (default: 0.02)\n";
    std::cout << "  --ridge-noise-freq <f>     Ridge noise frequency (default: 0.1)\n";
    std::cout << "  --ridge-noise <f>          Ridge noise amplitude (default: 0.10)\n";
    std::cout << "  --valley-noise-freq <f>    Valley noise frequency (default: 0.08)\n";
    std::cout << "  --valley-noise-amp <f>     Valley noise amplitude (default: 0.02)\n\n";

    std::cout << "=== RENDERING — PORES ===\n";
    std::cout << "  --enable-pores             Enable pore simulation (default: on)\n";
    std::cout << "  --no-pores                 Disable pore simulation\n";
    std::cout << "  --pore-density <f>         Pore density fraction (default: 0.008)\n";
    std::cout << "  --pore-min-size <f>        Min pore size in px (default: 1.5)\n";
    std::cout << "  --pore-max-size <f>        Max pore size in px (default: 5.0)\n";
    std::cout << "  --pore-mean-size <f>       Mean pore size log-normal (default: 2.5)\n";
    std::cout << "  --pore-size-stddev <f>     Pore size std dev (default: 0.5)\n";
    std::cout << "  --pore-circular-ratio <f>  Fraction of circular pores (default: 0.6)\n";
    std::cout << "  --pore-elliptical-ratio <f> Fraction of elliptical pores (default: 0.3)\n";
    std::cout << "  --pore-irregular-ratio <f> Fraction of irregular pores (default: 0.1)\n";
    std::cout << "  --pore-ellipse-aspect-min <f> Min ellipse aspect ratio (default: 1.2)\n";
    std::cout << "  --pore-ellipse-aspect-max <f> Max ellipse aspect ratio (default: 2.0)\n";
    std::cout << "  --pore-intensity-min <f>   Min pore interpolation to valley (default: 0.50)\n";
    std::cout << "  --pore-intensity-max <f>   Max pore interpolation to valley (default: 0.85)\n";
    std::cout << "  --pore-opacity-variation <f> Pore opacity variation (default: 0.3)\n";
    std::cout << "  --pore-clustering          Enable pore clustering (default: on)\n";
    std::cout << "  --no-pore-clustering       Disable pore clustering\n";
    std::cout << "  --pore-clustering-factor <f> Clustering intensity (default: 0.3)\n";
    std::cout << "  --pore-cluster-size <N>    Pores per cluster (default: 3)\n";
    std::cout << "  --incipient-ridge-ratio <f> Incipient ridge fraction (default: 0.0)\n\n";

    std::cout << "=== RENDERING — FINAL ===\n";
    std::cout << "  --anti-alias-sigma <f>     Anti-aliasing sigma (default: 1.5)\n";
    std::cout << "  --ridge-transition-width <f> Ridge-valley transition width px (default: 1.5)\n";
    std::cout << "  --final-blur-sigma <f>     Final blur sigma (default: 0.5)\n";
    std::cout << "  --contrast-lower <f>       Contrast stretch lower percentile (default: 2.0)\n";
    std::cout << "  --contrast-upper <f>       Contrast stretch upper percentile (default: 98.0)\n\n";

    std::cout << "=== RENDERING — SCARS ===\n";
    std::cout << "  --enable-scars             Enable scars (default: on)\n";
    std::cout << "  --no-scars                 Disable scars\n";
    std::cout << "  --num-scars <N>            Number of scars (default: 1)\n";
    std::cout << "  --scar-min-length <f>      Min scar length px (default: 30.0)\n";
    std::cout << "  --scar-max-length <f>      Max scar length px (default: 80.0)\n";
    std::cout << "  --scar-width <f>           Scar width px (default: 2.0)\n";
    std::cout << "  --scar-intensity <f>       Scar intensity 0-1 (default: 0.3)\n\n";

    std::cout << "=== RENDERING — MOISTURE ===\n";
    std::cout << "  --enable-moisture          Enable moisture blobs (default: on)\n";
    std::cout << "  --no-moisture              Disable moisture blobs\n";
    std::cout << "  --moisture-density <f>     Moisture blob density (default: 0.002)\n";
    std::cout << "  --moisture-min-size <f>    Min moisture blob size px (default: 15.0)\n";
    std::cout << "  --moisture-max-size <f>    Max moisture blob size px (default: 40.0)\n";
    std::cout << "  --moisture-intensity <f>   Moisture brightness effect (default: 0.15)\n\n";

    std::cout << "=== RENDERING — DIRT ===\n";
    std::cout << "  --enable-dirt              Enable dirt blobs (default: off)\n";
    std::cout << "  --no-dirt                  Disable dirt blobs\n";
    std::cout << "  --dirt-density <f>         Dirt blob density (default: 0.001)\n";
    std::cout << "  --dirt-min-size <f>        Min dirt blob size px (default: 10.0)\n";
    std::cout << "  --dirt-max-size <f>        Max dirt blob size px (default: 30.0)\n";
    std::cout << "  --dirt-intensity <f>       Dirt darkening effect (default: 0.2)\n\n";

    std::cout << "=== RENDERING — DRYNESS ===\n";
    std::cout << "  --enable-dryness           Enable dryness cracks (default: off)\n";
    std::cout << "  --no-dryness               Disable dryness cracks\n";
    std::cout << "  --dry-cracks <N>           Number of dry cracks (default: 3)\n";
    std::cout << "  --dry-crack-length <f>     Crack length px (default: 20.0)\n";
    std::cout << "  --dry-crack-width <f>      Crack width px (default: 0.5)\n";
    std::cout << "  --dry-crack-intensity <f>  Crack intensity (default: 0.1)\n\n";

    std::cout << "=== RENDERING — SMUDGES ===\n";
    std::cout << "  --enable-smudges           Enable smudges (default: off)\n";
    std::cout << "  --no-smudges               Disable smudges\n";
    std::cout << "  --smudge-density <f>       Smudge density (default: 0.0015)\n";
    std::cout << "  --smudge-min-size <f>      Min smudge size px (default: 20.0)\n";
    std::cout << "  --smudge-max-size <f>      Max smudge size px (default: 50.0)\n";
    std::cout << "  --smudge-intensity <f>     Smudge darkening (default: 0.12)\n\n";

    std::cout << "=== RENDERING — DIRECTIONAL LIGHTING ===\n";
    std::cout << "  --enable-lighting          Enable directional lighting (default: off)\n";
    std::cout << "  --no-lighting              Disable directional lighting\n";
    std::cout << "  --light-azimuth <f>        Light azimuth degrees (default: 45.0)\n";
    std::cout << "  --light-elevation <f>      Light elevation degrees (default: 45.0)\n";
    std::cout << "  --light-intensity <f>      Light intensity 0-1 (default: 0.6)\n";
    std::cout << "  --ridge-height <f>         Ridge height 0-1 (default: 0.15)\n";
    std::cout << "  --ambient-light <f>        Ambient light 0-1 (default: 0.4)\n";
    std::cout << "  --enable-specular          Enable specular highlights (default: off)\n";
    std::cout << "  --no-specular              Disable specular highlights\n";
    std::cout << "  --specular-strength <f>    Specular strength (default: 0.3)\n";
    std::cout << "  --specular-shininess <f>   Specular shininess (default: 20.0)\n\n";

    std::cout << "=== VARIATION / DISTORTION ===\n";
    std::cout << "  --plastic-distortion       Enable plastic/elastic distortion (default: on)\n";
    std::cout << "  --no-plastic-distortion    Disable plastic distortion\n";
    std::cout << "  --plastic-strength <f>     Plastic distortion strength (default: 0.8)\n";
    std::cout << "  --plastic-bumps <N>        Plastic distortion bumps (default: 2)\n";
    std::cout << "  --multi-scale              Enable multi-scale distortion (default: on)\n";
    std::cout << "  --no-multi-scale           Disable multi-scale distortion\n";
    std::cout << "  --distortion-octaves <N>   Multi-scale octaves (default: 2)\n";
    std::cout << "  --distortion-persistence <f> Multi-scale persistence (default: 0.5)\n";
    std::cout << "  --radial-distortion        Enable radial distortion (default: on)\n";
    std::cout << "  --no-radial-distortion     Disable radial distortion\n";
    std::cout << "  --radial-center <f>        Radial distortion center 0-1 (default: 0.5)\n";
    std::cout << "  --radial-strength <f>      Radial distortion strength (default: 0.5)\n";
    std::cout << "  --directional-distortion   Enable directional distortion (default: on)\n";
    std::cout << "  --no-directional-distortion Disable directional distortion\n";
    std::cout << "  --directional-strength <f> Directional distortion strength (default: 0.2)\n";
    std::cout << "  --enable-shear             Enable shear distortion (default: off)\n";
    std::cout << "  --no-shear                 Disable shear distortion\n";
    std::cout << "  --shear-angle <f>          Shear angle radians (default: 0.1)\n";
    std::cout << "  --shear-strength <f>       Shear strength (default: 1.0)\n";
    std::cout << "  --enable-lens              Enable lens distortion (default: off)\n";
    std::cout << "  --no-lens                  Disable lens distortion\n";
    std::cout << "  --lens-k1 <f>              Lens distortion K1 (default: 0.02)\n";
    std::cout << "  --lens-k2 <f>              Lens distortion K2 (default: 0.005)\n";
    std::cout << "  --enable-rotation          Enable rotation variation (default: off)\n";
    std::cout << "  --no-rotation              Disable rotation variation\n";
    std::cout << "  --max-rotation <f>         Max rotation degrees (default: 5.0)\n";
    std::cout << "  --enable-translation       Enable translation variation (default: off)\n";
    std::cout << "  --no-translation           Disable translation variation\n";
    std::cout << "  --max-translation-x <f>    Max translation X px (default: 10.0)\n";
    std::cout << "  --max-translation-y <f>    Max translation Y px (default: 10.0)\n";
    std::cout << "  --skin-condition           Enable skin condition (default: on)\n";
    std::cout << "  --no-skin-condition        Disable skin condition\n";
    std::cout << "  --skin-condition-factor <f> Skin condition factor -1..1 (default: 0.10)\n\n";

    std::cout << "=== MINUTIAE ===\n";
    std::cout << "  --continuous-phase         Use continuous phase method (default: off)\n";
    std::cout << "  --phase-noise <f>          Phase noise level 0-1 (default: 0.1)\n";
    std::cout << "  --use-quality-mask         Use quality mask for minutiae\n";
    std::cout << "  --minutiae-density <d>     Density: low/medium/high (default: low)\n";
    std::cout << "  --coherence-threshold <f>  Coherence threshold 0-1 (default: 0.3)\n";
    std::cout << "  --quality-window-size <N>  Quality window size (default: 15)\n";
    std::cout << "  --frequency-smooth-sigma <f> Frequency smooth sigma (default: 1.5)\n";
    std::cout << "  --min-minutiae <N>         Min minutiae count (default: 20)\n";
    std::cout << "  --max-minutiae <N>         Max minutiae count (default: 70)\n";
    std::cout << "  --typical-minutiae <N>     Typical minutiae count (default: 40)\n";
    std::cout << "  --bifurcation-ratio <f>    Bifurcation ratio 0-1 (default: 0.45)\n";
    std::cout << "  --core-concentration <f>   Core concentration 0-1 (default: 0.6)\n";
    std::cout << "  --core-radius-factor <f>   Core radius factor (default: 0.4)\n";
    std::cout << "  --min-spacing <f>          Min minutiae spacing px (default: 24.0)\n";
    std::cout << "  --insertion-prob <f>       Minutiae insertion probability (default: 0.7)\n";
    std::cout << "  --removal-prob <f>         Minutiae removal probability (default: 0.3)\n";
}

int main(int argc, char* argv[]) {
    SFinGe::BatchConfig config;
    int jobs = std::thread::hardware_concurrency();
    bool quietMode = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // ============================================================
        // HELP
        // ============================================================
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        }

        // ============================================================
        // BATCH CONTROL
        // ============================================================
        else if ((arg == "-n" || arg == "--num") && i + 1 < argc) {
            config.numFingerprints = std::stoi(argv[++i]);
        }
        else if ((arg == "-v" || arg == "--versions") && i + 1 < argc) {
            config.versionsPerFingerprint = std::stoi(argv[++i]);
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc && argv[i+1][0] != '-') {
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
        else if (arg == "--include-original") {
            config.skipOriginal = false;
        }
        else if (arg == "--no-mask") {
            config.applyEllipticalMask = false;
        }
        else if (arg == "--save-params") {
            config.saveParameters = true;
        }
        else if ((arg == "--seed") && i + 1 < argc) {
            config.globalSeed = static_cast<unsigned int>(std::stoul(argv[++i]));
        }
        else if (arg == "-q" || arg == "--quiet") {
            quietMode = true;
        }

        // ============================================================
        // SHAPE
        // ============================================================
        else if ((arg == "--shape-left") && i + 1 < argc) {
            config.shape.left = std::stoi(argv[++i]);
            config.useFixedShape = true;
        }
        else if ((arg == "--shape-right") && i + 1 < argc) {
            config.shape.right = std::stoi(argv[++i]);
            config.useFixedShape = true;
        }
        else if ((arg == "--shape-top") && i + 1 < argc) {
            config.shape.top = std::stoi(argv[++i]);
            config.useFixedShape = true;
        }
        else if ((arg == "--shape-middle") && i + 1 < argc) {
            config.shape.middle = std::stoi(argv[++i]);
            config.useFixedShape = true;
        }
        else if ((arg == "--shape-bottom") && i + 1 < argc) {
            config.shape.bottom = std::stoi(argv[++i]);
            config.useFixedShape = true;
        }
        else if ((arg == "--finger-type") && i + 1 < argc) {
            std::string ft = argv[++i];
            if      (ft == "thumb")  config.shape.fingerType = SFinGe::FingerType::Thumb;
            else if (ft == "index")  config.shape.fingerType = SFinGe::FingerType::Index;
            else if (ft == "middle") config.shape.fingerType = SFinGe::FingerType::Middle;
            else if (ft == "ring")   config.shape.fingerType = SFinGe::FingerType::Ring;
            else if (ft == "little") config.shape.fingerType = SFinGe::FingerType::Little;
            else { std::cerr << "Unknown finger-type: " << ft << "\n"; return 1; }
            config.useFixedShape = true;
        }

        // ============================================================
        // FINGERPRINT CLASS
        // ============================================================
        else if ((arg == "--class") && i + 1 < argc) {
            std::string cl = argv[++i];
            if      (cl == "arch")           config.classification.fingerprintClass = SFinGe::FingerprintClass::Arch;
            else if (cl == "tented-arch")    config.classification.fingerprintClass = SFinGe::FingerprintClass::TentedArch;
            else if (cl == "left-loop")      config.classification.fingerprintClass = SFinGe::FingerprintClass::LeftLoop;
            else if (cl == "right-loop")     config.classification.fingerprintClass = SFinGe::FingerprintClass::RightLoop;
            else if (cl == "whorl")          config.classification.fingerprintClass = SFinGe::FingerprintClass::Whorl;
            else if (cl == "twin-loop")      config.classification.fingerprintClass = SFinGe::FingerprintClass::TwinLoop;
            else if (cl == "central-pocket") config.classification.fingerprintClass = SFinGe::FingerprintClass::CentralPocket;
            else if (cl == "accidental")     config.classification.fingerprintClass = SFinGe::FingerprintClass::Accidental;
            else { std::cerr << "Unknown class: " << cl << "\n"; return 1; }
            config.useFixedClass = true;
        }

        // ============================================================
        // DENSITY
        // ============================================================
        else if ((arg == "--density-min-freq") && i + 1 < argc) {
            config.density.minFrequency = std::stof(argv[++i]);
        }
        else if ((arg == "--density-max-freq") && i + 1 < argc) {
            config.density.maxFrequency = std::stof(argv[++i]);
        }
        else if ((arg == "--density-zoom") && i + 1 < argc) {
            config.density.zoom = std::stod(argv[++i]);
        }
        else if ((arg == "--density-amplify") && i + 1 < argc) {
            config.density.amplify = std::stod(argv[++i]);
        }

        // ============================================================
        // RIDGE / GABOR
        // ============================================================
        else if ((arg == "--gabor-size") && i + 1 < argc) {
            config.ridge.gaborFilterSize = std::stoi(argv[++i]);
        }
        else if ((arg == "--cache-degrees") && i + 1 < argc) {
            config.ridge.cacheDegrees = std::stoi(argv[++i]);
        }
        else if ((arg == "--cache-freqs") && i + 1 < argc) {
            config.ridge.cacheFrequencies = std::stoi(argv[++i]);
        }
        else if ((arg == "--max-iterations") && i + 1 < argc) {
            config.ridge.maxIterations = std::stoi(argv[++i]);
        }
        else if ((arg == "--seed-density") && i + 1 < argc) {
            config.ridge.initialSeedDensity = std::stod(argv[++i]);
        }

        // ============================================================
        // ORIENTATION
        // ============================================================
        else if ((arg == "--orientation-method") && i + 1 < argc) {
            std::string m = argv[++i];
            if      (m == "poincare")          config.orientation.method = SFinGe::OrientationMethod::Poincare;
            else if (m == "fomfe")             config.orientation.method = SFinGe::OrientationMethod::FOMFE;
            else if (m == "poincare-smoothed") config.orientation.method = SFinGe::OrientationMethod::PoincareSmoothed;
            else { std::cerr << "Unknown orientation-method: " << m << "\n"; return 1; }
        }
        else if ((arg == "--smooth-sigma") && i + 1 < argc) {
            config.orientation.smoothingSigma = std::stod(argv[++i]);
        }
        else if ((arg == "--vertical-bias") && i + 1 < argc) {
            config.orientation.verticalBiasStrength = std::stod(argv[++i]);
        }
        else if ((arg == "--vertical-bias-radius") && i + 1 < argc) {
            config.orientation.verticalBiasRadius = std::stod(argv[++i]);
        }
        else if ((arg == "--core-convergence") && i + 1 < argc) {
            config.orientation.coreConvergenceStrength = std::stod(argv[++i]);
        }
        else if ((arg == "--core-convergence-radius") && i + 1 < argc) {
            config.orientation.coreConvergenceRadius = std::stod(argv[++i]);
        }
        else if ((arg == "--core-convergence-prob") && i + 1 < argc) {
            config.orientation.coreConvergenceProbability = std::stod(argv[++i]);
        }
        else if ((arg == "--anisotropy-x") && i + 1 < argc) {
            config.orientation.anisotropyFactorX = std::stod(argv[++i]);
        }
        else if ((arg == "--anisotropy-y") && i + 1 < argc) {
            config.orientation.anisotropyFactorY = std::stod(argv[++i]);
        }
        else if ((arg == "--arch-amplitude") && i + 1 < argc) {
            config.orientation.archAmplitude = std::stod(argv[++i]);
        }
        else if ((arg == "--tented-arch-decay") && i + 1 < argc) {
            config.orientation.tentedArchPeakInfluenceDecay = std::stod(argv[++i]);
        }
        else if ((arg == "--loop-bias") && i + 1 < argc) {
            config.orientation.loopVerticalBiasStrength = std::stod(argv[++i]);
        }
        else if ((arg == "--loop-edge-blend") && i + 1 < argc) {
            config.orientation.loopEdgeBlendFactor = std::stod(argv[++i]);
        }
        else if ((arg == "--loop-bias-radius") && i + 1 < argc) {
            config.orientation.loopVerticalBiasRadiusFactor = std::stod(argv[++i]);
        }
        else if ((arg == "--whorl-spiral") && i + 1 < argc) {
            config.orientation.whorlSpiralFactor = std::stod(argv[++i]);
        }
        else if ((arg == "--whorl-edge-decay") && i + 1 < argc) {
            config.orientation.whorlEdgeDecayFactor = std::stod(argv[++i]);
        }
        else if ((arg == "--twin-loop-smooth") && i + 1 < argc) {
            config.orientation.twinLoopSmoothing = std::stod(argv[++i]);
        }
        else if ((arg == "--central-pocket-conc") && i + 1 < argc) {
            config.orientation.centralPocketConcentration = std::stod(argv[++i]);
        }
        else if ((arg == "--accidental-irregularity") && i + 1 < argc) {
            config.orientation.accidentalIrregularity = std::stod(argv[++i]);
        }
        else if ((arg == "--fomfe-order-m") && i + 1 < argc) {
            config.orientation.fomfeOrderM = std::stoi(argv[++i]);
        }
        else if ((arg == "--fomfe-order-n") && i + 1 < argc) {
            config.orientation.fomfeOrderN = std::stoi(argv[++i]);
        }

        // ============================================================
        // RENDERING — TEXTURE NOISE
        // ============================================================
        else if ((arg == "--bg-noise-freq") && i + 1 < argc) {
            config.rendering.backgroundNoiseFrequency = std::stod(argv[++i]);
        }
        else if ((arg == "--bg-noise-amp") && i + 1 < argc) {
            config.rendering.backgroundNoiseAmplitude = std::stod(argv[++i]);
        }
        else if ((arg == "--ridge-noise-freq") && i + 1 < argc) {
            config.rendering.ridgeNoiseFrequency = std::stod(argv[++i]);
        }
        else if ((arg == "--ridge-noise") && i + 1 < argc) {
            config.rendering.ridgeNoiseAmplitude = std::stod(argv[++i]);
        }
        else if ((arg == "--valley-noise-freq") && i + 1 < argc) {
            config.rendering.valleyNoiseFrequency = std::stod(argv[++i]);
        }
        else if ((arg == "--valley-noise-amp") && i + 1 < argc) {
            config.rendering.valleyNoiseAmplitude = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — PORES
        // ============================================================
        else if (arg == "--enable-pores") {
            config.rendering.enablePores = true;
        }
        else if (arg == "--no-pores") {
            config.rendering.enablePores = false;
        }
        else if ((arg == "--pore-density") && i + 1 < argc) {
            config.rendering.poreDensity = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-min-size") && i + 1 < argc) {
            config.rendering.poreMinSize = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-max-size") && i + 1 < argc) {
            config.rendering.poreMaxSize = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-mean-size") && i + 1 < argc) {
            config.rendering.poreMeanSize = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-size-stddev") && i + 1 < argc) {
            config.rendering.poreSizeStdDev = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-circular-ratio") && i + 1 < argc) {
            config.rendering.poreCircularRatio = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-elliptical-ratio") && i + 1 < argc) {
            config.rendering.poreEllipticalRatio = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-irregular-ratio") && i + 1 < argc) {
            config.rendering.poreIrregularRatio = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-ellipse-aspect-min") && i + 1 < argc) {
            config.rendering.poreEllipseAspectMin = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-ellipse-aspect-max") && i + 1 < argc) {
            config.rendering.poreEllipseAspectMax = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-intensity-min") && i + 1 < argc) {
            config.rendering.minPoreIntensity = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-intensity-max") && i + 1 < argc) {
            config.rendering.maxPoreIntensity = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-opacity-variation") && i + 1 < argc) {
            config.rendering.poreOpacityVariation = std::stod(argv[++i]);
        }
        else if (arg == "--pore-clustering") {
            config.rendering.enablePoreClustering = true;
        }
        else if (arg == "--no-pore-clustering") {
            config.rendering.enablePoreClustering = false;
        }
        else if ((arg == "--pore-clustering-factor") && i + 1 < argc) {
            config.rendering.poreClusteringFactor = std::stod(argv[++i]);
        }
        else if ((arg == "--pore-cluster-size") && i + 1 < argc) {
            config.rendering.poreClusterSize = std::stoi(argv[++i]);
        }
        else if ((arg == "--incipient-ridge-ratio") && i + 1 < argc) {
            config.rendering.incipientRidgeRatio = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — FINAL
        // ============================================================
        else if ((arg == "--anti-alias-sigma") && i + 1 < argc) {
            config.rendering.antiAliasingSigma = std::stod(argv[++i]);
        }
        else if ((arg == "--ridge-transition-width") && i + 1 < argc) {
            config.rendering.ridgeTransitionWidth = std::stod(argv[++i]);
        }
        else if ((arg == "--final-blur-sigma") && i + 1 < argc) {
            config.rendering.finalBlurSigma = std::stod(argv[++i]);
        }
        else if ((arg == "--contrast-lower") && i + 1 < argc) {
            config.rendering.contrastPercentileLower = std::stod(argv[++i]);
        }
        else if ((arg == "--contrast-upper") && i + 1 < argc) {
            config.rendering.contrastPercentileUpper = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — SCARS
        // ============================================================
        else if (arg == "--enable-scars") {
            config.rendering.enableScars = true;
        }
        else if (arg == "--no-scars") {
            config.rendering.enableScars = false;
        }
        else if ((arg == "--num-scars") && i + 1 < argc) {
            config.rendering.numScars = std::stoi(argv[++i]);
        }
        else if ((arg == "--scar-min-length") && i + 1 < argc) {
            config.rendering.scarMinLength = std::stod(argv[++i]);
        }
        else if ((arg == "--scar-max-length") && i + 1 < argc) {
            config.rendering.scarMaxLength = std::stod(argv[++i]);
        }
        else if ((arg == "--scar-width") && i + 1 < argc) {
            config.rendering.scarWidth = std::stod(argv[++i]);
        }
        else if ((arg == "--scar-intensity") && i + 1 < argc) {
            config.rendering.scarIntensity = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — MOISTURE
        // ============================================================
        else if (arg == "--enable-moisture") {
            config.rendering.enableMoisture = true;
        }
        else if (arg == "--no-moisture") {
            config.rendering.enableMoisture = false;
        }
        else if ((arg == "--moisture-density") && i + 1 < argc) {
            config.rendering.moistureDensity = std::stod(argv[++i]);
        }
        else if ((arg == "--moisture-min-size") && i + 1 < argc) {
            config.rendering.moistureMinSize = std::stod(argv[++i]);
        }
        else if ((arg == "--moisture-max-size") && i + 1 < argc) {
            config.rendering.moistureMaxSize = std::stod(argv[++i]);
        }
        else if ((arg == "--moisture-intensity") && i + 1 < argc) {
            config.rendering.moistureIntensity = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — DIRT
        // ============================================================
        else if (arg == "--enable-dirt") {
            config.rendering.enableDirt = true;
        }
        else if (arg == "--no-dirt") {
            config.rendering.enableDirt = false;
        }
        else if ((arg == "--dirt-density") && i + 1 < argc) {
            config.rendering.dirtDensity = std::stod(argv[++i]);
        }
        else if ((arg == "--dirt-min-size") && i + 1 < argc) {
            config.rendering.dirtMinSize = std::stod(argv[++i]);
        }
        else if ((arg == "--dirt-max-size") && i + 1 < argc) {
            config.rendering.dirtMaxSize = std::stod(argv[++i]);
        }
        else if ((arg == "--dirt-intensity") && i + 1 < argc) {
            config.rendering.dirtIntensity = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — DRYNESS
        // ============================================================
        else if (arg == "--enable-dryness") {
            config.rendering.enableDryness = true;
        }
        else if (arg == "--no-dryness") {
            config.rendering.enableDryness = false;
        }
        else if ((arg == "--dry-cracks") && i + 1 < argc) {
            config.rendering.numDryCracks = std::stoi(argv[++i]);
        }
        else if ((arg == "--dry-crack-length") && i + 1 < argc) {
            config.rendering.dryCrackLength = std::stod(argv[++i]);
        }
        else if ((arg == "--dry-crack-width") && i + 1 < argc) {
            config.rendering.dryCrackWidth = std::stod(argv[++i]);
        }
        else if ((arg == "--dry-crack-intensity") && i + 1 < argc) {
            config.rendering.dryCrackIntensity = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — SMUDGES
        // ============================================================
        else if (arg == "--enable-smudges") {
            config.rendering.enableSmudges = true;
        }
        else if (arg == "--no-smudges") {
            config.rendering.enableSmudges = false;
        }
        else if ((arg == "--smudge-density") && i + 1 < argc) {
            config.rendering.smudgeDensity = std::stod(argv[++i]);
        }
        else if ((arg == "--smudge-min-size") && i + 1 < argc) {
            config.rendering.smudgeMinSize = std::stod(argv[++i]);
        }
        else if ((arg == "--smudge-max-size") && i + 1 < argc) {
            config.rendering.smudgeMaxSize = std::stod(argv[++i]);
        }
        else if ((arg == "--smudge-intensity") && i + 1 < argc) {
            config.rendering.smudgeIntensity = std::stod(argv[++i]);
        }

        // ============================================================
        // RENDERING — DIRECTIONAL LIGHTING
        // ============================================================
        else if (arg == "--enable-lighting") {
            config.rendering.enableDirectionalLighting = true;
        }
        else if (arg == "--no-lighting") {
            config.rendering.enableDirectionalLighting = false;
        }
        else if ((arg == "--light-azimuth") && i + 1 < argc) {
            config.rendering.lightAzimuth = std::stod(argv[++i]);
        }
        else if ((arg == "--light-elevation") && i + 1 < argc) {
            config.rendering.lightElevation = std::stod(argv[++i]);
        }
        else if ((arg == "--light-intensity") && i + 1 < argc) {
            config.rendering.lightIntensity = std::stod(argv[++i]);
        }
        else if ((arg == "--ridge-height") && i + 1 < argc) {
            config.rendering.ridgeHeight = std::stod(argv[++i]);
        }
        else if ((arg == "--ambient-light") && i + 1 < argc) {
            config.rendering.ambientLight = std::stod(argv[++i]);
        }
        else if (arg == "--enable-specular") {
            config.rendering.enableSpecular = true;
        }
        else if (arg == "--no-specular") {
            config.rendering.enableSpecular = false;
        }
        else if ((arg == "--specular-strength") && i + 1 < argc) {
            config.rendering.specularStrength = std::stod(argv[++i]);
        }
        else if ((arg == "--specular-shininess") && i + 1 < argc) {
            config.rendering.specularShininess = std::stod(argv[++i]);
        }

        // ============================================================
        // VARIATION / DISTORTION
        // ============================================================
        else if (arg == "--plastic-distortion") {
            config.variation.enablePlasticDistortion = true;
        }
        else if (arg == "--no-plastic-distortion") {
            config.variation.enablePlasticDistortion = false;
        }
        else if ((arg == "--plastic-strength") && i + 1 < argc) {
            config.variation.plasticDistortionStrength = std::stod(argv[++i]);
        }
        else if ((arg == "--plastic-bumps") && i + 1 < argc) {
            config.variation.plasticDistortionBumps = std::stoi(argv[++i]);
        }
        else if (arg == "--multi-scale") {
            config.variation.enableMultiScaleDistortion = true;
        }
        else if (arg == "--no-multi-scale") {
            config.variation.enableMultiScaleDistortion = false;
        }
        else if ((arg == "--distortion-octaves") && i + 1 < argc) {
            config.variation.distortionOctaves = std::stoi(argv[++i]);
        }
        else if ((arg == "--distortion-persistence") && i + 1 < argc) {
            config.variation.distortionPersistence = std::stod(argv[++i]);
        }
        else if (arg == "--radial-distortion") {
            config.variation.enableRadialDistortion = true;
        }
        else if (arg == "--no-radial-distortion") {
            config.variation.enableRadialDistortion = false;
        }
        else if ((arg == "--radial-center") && i + 1 < argc) {
            config.variation.radialDistortionCenter = std::stod(argv[++i]);
        }
        else if ((arg == "--radial-strength") && i + 1 < argc) {
            config.variation.radialDistortionStrength = std::stod(argv[++i]);
        }
        else if (arg == "--directional-distortion") {
            config.variation.enableDirectionalDistortion = true;
        }
        else if (arg == "--no-directional-distortion") {
            config.variation.enableDirectionalDistortion = false;
        }
        else if ((arg == "--directional-strength") && i + 1 < argc) {
            config.variation.directionalDistortionStrength = std::stod(argv[++i]);
        }
        else if (arg == "--enable-shear") {
            config.variation.enableShearDistortion = true;
        }
        else if (arg == "--no-shear") {
            config.variation.enableShearDistortion = false;
        }
        else if ((arg == "--shear-angle") && i + 1 < argc) {
            config.variation.shearAngle = std::stod(argv[++i]);
        }
        else if ((arg == "--shear-strength") && i + 1 < argc) {
            config.variation.shearStrength = std::stod(argv[++i]);
        }
        else if (arg == "--enable-lens") {
            config.variation.enableLensDistortion = true;
        }
        else if (arg == "--no-lens") {
            config.variation.enableLensDistortion = false;
        }
        else if ((arg == "--lens-k1") && i + 1 < argc) {
            config.variation.lensDistortionK1 = std::stod(argv[++i]);
        }
        else if ((arg == "--lens-k2") && i + 1 < argc) {
            config.variation.lensDistortionK2 = std::stod(argv[++i]);
        }
        else if (arg == "--enable-rotation") {
            config.variation.enableRotation = true;
        }
        else if (arg == "--no-rotation") {
            config.variation.enableRotation = false;
        }
        else if ((arg == "--max-rotation") && i + 1 < argc) {
            config.variation.maxRotationAngle = std::stod(argv[++i]);
        }
        else if (arg == "--enable-translation") {
            config.variation.enableTranslation = true;
        }
        else if (arg == "--no-translation") {
            config.variation.enableTranslation = false;
        }
        else if ((arg == "--max-translation-x") && i + 1 < argc) {
            config.variation.maxTranslationX = std::stod(argv[++i]);
        }
        else if ((arg == "--max-translation-y") && i + 1 < argc) {
            config.variation.maxTranslationY = std::stod(argv[++i]);
        }
        else if (arg == "--skin-condition") {
            config.variation.enableSkinCondition = true;
        }
        else if (arg == "--no-skin-condition") {
            config.variation.enableSkinCondition = false;
        }
        else if ((arg == "--skin-condition-factor") && i + 1 < argc) {
            config.variation.skinConditionFactor = std::stod(argv[++i]);
        }

        // ============================================================
        // MINUTIAE
        // ============================================================
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
        else if ((arg == "--min-minutiae") && i + 1 < argc) {
            config.minutiae.stats.minMinutiae = std::stoi(argv[++i]);
        }
        else if ((arg == "--max-minutiae") && i + 1 < argc) {
            config.minutiae.stats.maxMinutiae = std::stoi(argv[++i]);
        }
        else if ((arg == "--typical-minutiae") && i + 1 < argc) {
            config.minutiae.stats.typicalMinutiae = std::stoi(argv[++i]);
        }
        else if ((arg == "--bifurcation-ratio") && i + 1 < argc) {
            config.minutiae.stats.bifurcationRatio = std::stod(argv[++i]);
        }
        else if ((arg == "--core-concentration") && i + 1 < argc) {
            config.minutiae.stats.coreConcentration = std::stod(argv[++i]);
        }
        else if ((arg == "--core-radius-factor") && i + 1 < argc) {
            config.minutiae.stats.coreRadiusFactor = std::stod(argv[++i]);
        }
        else if ((arg == "--min-spacing") && i + 1 < argc) {
            config.minutiae.stats.minSpacing = std::stod(argv[++i]);
        }
        else if ((arg == "--insertion-prob") && i + 1 < argc) {
            config.minutiae.insertionProbability = std::stod(argv[++i]);
        }
        else if ((arg == "--removal-prob") && i + 1 < argc) {
            config.minutiae.removalProbability = std::stod(argv[++i]);
        }

        else if (arg[0] != '-') {
            // Argumento posicional: tratar como output directory (compatibilidade)
            config.outputDirectory = arg;
        }
        // Flags desconhecidas: ignorar silenciosamente (compatibilidade com versão anterior)

    }

    config.quietMode = quietMode;

    if (jobs < 1) jobs = 1;

    if (!quietMode) {
        std::cout << "=== SFINGE CLI Pure - Batch Generation ===\n";
        std::cout << "Fingerprints:       " << config.numFingerprints << "\n";
        std::cout << "Versions per FP:    " << config.versionsPerFingerprint << "\n";
        std::cout << "Skip original:      " << (config.skipOriginal ? "yes" : "no") << "\n";
        std::cout << "Output:             " << config.outputDirectory << "\n";
        std::cout << "Parallel jobs:      " << jobs << "\n";
        std::cout << "Global seed:        " << (config.globalSeed != 0
            ? std::to_string(config.globalSeed) : "random") << "\n";
        std::cout << "Fixed shape:        " << (config.useFixedShape ? "yes" : "no (random ~1000×1200)") << "\n";
        std::cout << "Fixed class:        " << (config.useFixedClass ? "yes" : "no (population distribution)") << "\n";
        std::cout << "==========================================\n\n";
    }

    SFinGe::BatchGenerator generator;
    generator.setBatchConfig(config);
    generator.setNumWorkers(jobs);

    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastUpdateTime = startTime;
    const auto updateInterval = std::chrono::seconds(5);

    generator.setProgressCallback([&](int fpCompleted, int totalFps, int imgCount) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

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
