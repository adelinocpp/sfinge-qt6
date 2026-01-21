#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include "core/batch_generator.h"

static bool g_quietMode = false;

void quietMessageHandler(QtMsgType type, const QMessageLogContext&, const QString&) {
    if (g_quietMode && type == QtDebugMsg) {
        return;
    }
}

void printUsage() {
    std::cout << "SFINGE-Qt6 CLI - Synthetic Fingerprint Generator\n\n";
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
    std::cout << "  -q, --quiet             Suppress debug, show only elapsed time\n";
    std::cout << "  -h, --help              Show this help\n";
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("SFINGE-Qt6-CLI");
    app.setApplicationVersion("1.0.0");
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Synthetic Fingerprint Generator - CLI Only");
    
    parser.addOption(QCommandLineOption({"n", "num"}, "Number of fingerprints", "count", "10"));
    parser.addOption(QCommandLineOption({"v", "versions"}, "Versions per fingerprint", "count", "3"));
    parser.addOption(QCommandLineOption({"o", "output"}, "Output directory", "dir", "./output"));
    parser.addOption(QCommandLineOption({"p", "prefix"}, "Filename prefix", "name", "fingerprint"));
    parser.addOption(QCommandLineOption({"s", "start"}, "Start index", "index", "0"));
    parser.addOption(QCommandLineOption({"j", "jobs"}, "Parallel jobs", "count", QString::number(QThread::idealThreadCount())));
    parser.addOption(QCommandLineOption("skip-original", "Skip v0 (original) images"));
    parser.addOption(QCommandLineOption("no-mask", "Disable elliptical mask"));
    parser.addOption(QCommandLineOption("save-params", "Save parameters JSON"));
    parser.addOption(QCommandLineOption({"q", "quiet"}, "Suppress debug output, show only elapsed time"));
    parser.addOption(QCommandLineOption({"h", "help"}, "Show help"));
    
    parser.process(app);
    
    if (parser.isSet("help")) {
        printUsage();
        return 0;
    }
    
    SFinGe::BatchConfig config;
    config.numFingerprints = parser.value("num").toInt();
    config.versionsPerFingerprint = parser.value("versions").toInt();
    config.outputDirectory = parser.value("output");
    config.filenamePrefix = parser.value("prefix");
    config.startIndex = parser.value("start").toInt();
    config.skipOriginal = parser.isSet("skip-original");
    config.applyEllipticalMask = !parser.isSet("no-mask");
    config.saveParameters = parser.isSet("save-params");
    
    int jobs = parser.value("jobs").toInt();
    if (jobs < 1) jobs = QThread::idealThreadCount();
    
    bool quietMode = parser.isSet("quiet");
    g_quietMode = quietMode;
    config.quietMode = quietMode;
    
    if (quietMode) {
        qInstallMessageHandler(quietMessageHandler);
    }
    
    std::cout << "=== SFINGE-Qt6 CLI Batch Generation ===\n";
    std::cout << "Fingerprints: " << config.numFingerprints << "\n";
    std::cout << "Versions per FP: " << config.versionsPerFingerprint << "\n";
    std::cout << "Skip original: " << (config.skipOriginal ? "yes" : "no") << "\n";
    std::cout << "Output: " << config.outputDirectory.toStdString() << "\n";
    std::cout << "Parallel jobs: " << jobs << "\n";
    std::cout << "===================================\n\n";
    
    SFinGe::BatchGenerator generator;
    generator.setBatchConfig(config);
    generator.setNumWorkers(jobs);
    
    QElapsedTimer timer;
    timer.start();
    
    QObject::connect(&generator, &SFinGe::BatchGenerator::progressUpdated,
        [&timer](int fpCompleted, int totalFps, const QString& imgCount) {
            if (fpCompleted > 0) {
                qint64 elapsed = timer.elapsed();
                qint64 avgTimePerFp = elapsed / fpCompleted;
                qint64 remaining = avgTimePerFp * (totalFps - fpCompleted);
                int remainingSec = remaining / 1000;
                int elapsedSec = elapsed / 1000;
                std::cout << "\rFP [" << fpCompleted << "/" << totalFps << "] "
                          << "Images: " << imgCount.toStdString() << " | "
                          << "Elapsed: " << elapsedSec << "s, ETA: " 
                          << remainingSec / 60 << ":" << std::setfill('0') << std::setw(2) << remainingSec % 60
                          << "          " << std::flush;
            }
        });
    
    QObject::connect(&generator, &SFinGe::BatchGenerator::batchCompleted,
        [&timer](int generated) {
            qint64 elapsed = timer.elapsed();
            int seconds = elapsed / 1000;
            int ms = elapsed % 1000;
            std::cout << "\n\nBatch completed! Generated " << generated << " images.\n";
            std::cout << "Elapsed time: " << seconds << "." << ms << " seconds\n";
        });
    
    QObject::connect(&generator, &SFinGe::BatchGenerator::error,
        [](const QString& message) {
            std::cerr << "\nError: " << message.toStdString() << "\n";
        });
    
    bool success = generator.generateBatchParallel();
    
    return success ? 0 : 1;
}
