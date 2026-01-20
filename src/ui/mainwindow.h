#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include "core/fingerprint_generator.h"
#include "models/fingerprint_parameters.h"
#include "models/singular_points.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace SFinGe {

class ShapeControlWidget;
class DensityControlWidget;
class OrientationControlWidget;
class PreviewWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onGenerateShape();
    void onGenerateDensity();
    void onGenerateOrientation();
    void onRegenerateOrientationWithSamePoints();
    void onGenerateFingerprint();
    void onExportImage();
    void onLoadParameters();
    void onSaveParameters();
    void onResetParameters();
    void onBatchGeneration();
    void onAbout();
    
    void onProgressChanged(int percentage, const QString& message);
    void onGenerationComplete();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void setupStatusBar();
    void connectSignals();
    void updatePreview(const QImage& image);

    Ui::MainWindow *ui;
    
    FingerprintGenerator* m_generator;
    class FingerprintWorker* m_worker;
    FingerprintParameters m_parameters;
    SingularPoints m_singularPoints;
    
    ShapeControlWidget* m_shapeControl;
    DensityControlWidget* m_densityControl;
    OrientationControlWidget* m_orientationControl;
    PreviewWidget* m_previewWidget;
    
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QCheckBox* m_ellipticalMaskCheckBox;
};

}

#endif
