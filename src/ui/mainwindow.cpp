#include "mainwindow.h"
#include "widgets/shape_control_widget.h"
#include "widgets/density_control_widget.h"
#include "widgets/orientation_control_widget.h"
#include "widgets/preview_widget.h"
#include "dialogs/export_dialog.h"
#include "dialogs/batch_dialog.h"
#include "core/fingerprint_worker.h"
#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QScrollArea>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QLabel>

namespace SFinGe {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(nullptr)
    , m_generator(new FingerprintGenerator(this))
    , m_worker(new FingerprintWorker(this))
    , m_statusLabel(new QLabel(this))
    , m_progressBar(new QProgressBar(this)) {
    
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupDockWidgets();
    setupStatusBar();
    connectSignals();
    
    connect(m_worker, &FingerprintWorker::progressChanged,
            this, &MainWindow::onProgressChanged, Qt::QueuedConnection);
    connect(m_worker, &FingerprintWorker::fingerprintGenerated,
            this, &MainWindow::updatePreview, Qt::QueuedConnection);
    connect(m_worker, &FingerprintWorker::fingerprintGenerated,
            this, &MainWindow::onGenerationComplete, Qt::QueuedConnection);
    connect(m_worker, &FingerprintWorker::generationFailed,
            this, [this](const QString& error) {
                qDebug() << "[MainWindow] Error received:" << error;
                m_statusLabel->setText(tr("Error: %1").arg(error));
                m_progressBar->hide();
            }, Qt::QueuedConnection);
    
    m_singularPoints.addCore(250.0, 300.0);
    m_singularPoints.addDelta(250.0, 450.0);
    
    m_orientationControl->setSingularPoints(m_singularPoints);
    
    setWindowTitle("SFINGE-Qt6 - Synthetic Fingerprint Generator");
    resize(1200, 800);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    m_previewWidget = new PreviewWidget(this);
    setCentralWidget(m_previewWidget);
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(tr("Load Parameters..."), this, &MainWindow::onLoadParameters, QKeySequence::Open);
    fileMenu->addAction(tr("Save Parameters..."), this, &MainWindow::onSaveParameters, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Export Image..."), this, &MainWindow::onExportImage, QKeySequence("Ctrl+E"));
    fileMenu->addAction(tr("Batch Generation..."), this, &MainWindow::onBatchGeneration, QKeySequence("Ctrl+B"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit, QKeySequence::Quit);
    
    QMenu* editMenu = menuBar()->addMenu(tr("Edit"));
    editMenu->addAction(tr("Reset Parameters"), this, &MainWindow::onResetParameters);
    
    QMenu* generateMenu = menuBar()->addMenu(tr("Generate"));
    generateMenu->addAction(tr("Generate Shape"), this, &MainWindow::onGenerateShape, QKeySequence("F5"));
    generateMenu->addAction(tr("Generate Density"), this, &MainWindow::onGenerateDensity, QKeySequence("F6"));
    generateMenu->addAction(tr("Generate Orientation"), this, &MainWindow::onGenerateOrientation, QKeySequence("F7"));
    generateMenu->addSeparator();
    generateMenu->addAction(tr("Generate Fingerprint"), this, &MainWindow::onGenerateFingerprint, QKeySequence("F8"));
    
    QMenu* helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(tr("About"), this, &MainWindow::onAbout);
    helpMenu->addAction(tr("About Qt"), qApp, &QApplication::aboutQt);
}

void MainWindow::setupToolBar() {
    QToolBar* toolbar = new QToolBar(tr("Main Toolbar"), this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));
    
    QAction* shapeAction = toolbar->addAction(tr("Shape"), this, &MainWindow::onGenerateShape);
    shapeAction->setToolTip(tr("Generate Shape (F5)"));
    
    QAction* densityAction = toolbar->addAction(tr("Density"), this, &MainWindow::onGenerateDensity);
    densityAction->setToolTip(tr("Generate Density (F6)"));
    
    QAction* orientationAction = toolbar->addAction(tr("Orientation"), this, &MainWindow::onGenerateOrientation);
    orientationAction->setToolTip(tr("Generate Orientation (F7)"));
    
    toolbar->addSeparator();
    
    QAction* fingerprintAction = toolbar->addAction(tr("Fingerprint"), this, &MainWindow::onGenerateFingerprint);
    fingerprintAction->setToolTip(tr("Generate Fingerprint (F8)"));
    
    toolbar->addSeparator();
    
    QAction* exportAction = toolbar->addAction(tr("Export"), this, &MainWindow::onExportImage);
    exportAction->setToolTip(tr("Export Image (Ctrl+E)"));
    
    addToolBar(Qt::TopToolBarArea, toolbar);
}

void MainWindow::setupDockWidgets() {
    // Criar widgets internos apenas para manter valores padrão
    m_shapeControl = new ShapeControlWidget(nullptr);
    m_densityControl = new DensityControlWidget(nullptr);
    
    // Apenas Orientation control visível no dock
    m_orientationControl = new OrientationControlWidget(this);
    QDockWidget* orientationDock = new QDockWidget(tr("Orientation Parameters"), this);
    orientationDock->setWidget(m_orientationControl);
    orientationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, orientationDock);
}

void MainWindow::setupStatusBar() {
    m_ellipticalMaskCheckBox = new QCheckBox(tr("Elliptical mask"), this);
    m_ellipticalMaskCheckBox->setChecked(true);
    m_ellipticalMaskCheckBox->setToolTip(tr("Apply smooth elliptical mask with fade out to white at edges"));
    statusBar()->addPermanentWidget(m_ellipticalMaskCheckBox);
    
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_progressBar);
    m_progressBar->hide();
    m_statusLabel->setText(tr("Ready"));
}

void MainWindow::connectSignals() {
    connect(m_orientationControl, &OrientationControlWidget::pointsChanged,
            this, [this]() {
                qDebug() << "[MainWindow] Points changed, regenerating orientation map...";
                onGenerateOrientation();
            });
    
    connect(m_orientationControl, &OrientationControlWidget::parametersChanged,
            this, [this]() {
                qDebug() << "[MainWindow] Orientation parameters changed, regenerating with same points...";
                onRegenerateOrientationWithSamePoints();
            });
}

void MainWindow::onGenerateShape() {
    m_shapeControl->updateParameters(m_parameters.shape);
    m_generator->setParameters(m_parameters);
    
    QImage image = m_generator->generateShape();
    updatePreview(image);
}

void MainWindow::onGenerateDensity() {
    m_shapeControl->updateParameters(m_parameters.shape);
    m_densityControl->updateParameters(m_parameters.density);
    m_generator->setParameters(m_parameters);
    
    QImage image = m_generator->generateDensity();
    updatePreview(image);
}

void MainWindow::onGenerateOrientation() {
    m_shapeControl->updateParameters(m_parameters.shape);
    
    int width = m_parameters.shape.left + m_parameters.shape.right;
    int height = m_parameters.shape.top + m_parameters.shape.middle + m_parameters.shape.bottom;
    m_orientationControl->setImageDimensions(width, height);
    
    FingerprintClass currentClass = m_orientationControl->getCurrentClass();
    m_parameters.classification.fingerprintClass = currentClass;
    
    qDebug() << "[MainWindow] === GENERATE ORIENTATION ===";
    qDebug() << "[MainWindow] Selected class:" << static_cast<int>(currentClass);
    
    m_singularPoints.clearAll();
    m_singularPoints.generateRandomPoints(currentClass, width, height);
    m_orientationControl->setSingularPoints(m_singularPoints);
    
    qDebug() << "[MainWindow] Auto-generated points: cores=" << m_singularPoints.getCoreCount() << ", deltas=" << m_singularPoints.getDeltaCount();
    
    // Atualizar parâmetros de orientação da UI
    m_orientationControl->updateOrientationParameters(m_parameters.orientation);
    
    m_generator->setParameters(m_parameters);
    m_generator->setSingularPoints(m_singularPoints);
    
    QImage image = m_generator->generateOrientationVisualization();
    updatePreview(image);
}

void MainWindow::onRegenerateOrientationWithSamePoints() {
    m_shapeControl->updateParameters(m_parameters.shape);
    
    int width = m_parameters.shape.left + m_parameters.shape.right;
    int height = m_parameters.shape.top + m_parameters.shape.middle + m_parameters.shape.bottom;
    
    FingerprintClass currentClass = m_orientationControl->getCurrentClass();
    m_parameters.classification.fingerprintClass = currentClass;
    
    // Usar pontos existentes do orientation control
    m_singularPoints = m_orientationControl->getSingularPoints();
    
    qDebug() << "[MainWindow] Regenerating orientation with existing points: cores=" << m_singularPoints.getCoreCount() << ", deltas=" << m_singularPoints.getDeltaCount();
    
    // Atualizar parâmetros de orientação da UI
    m_orientationControl->updateOrientationParameters(m_parameters.orientation);
    
    m_generator->setParameters(m_parameters);
    m_generator->setSingularPoints(m_singularPoints);
    
    QImage image = m_generator->generateOrientationVisualization();
    updatePreview(image);
}

void MainWindow::onGenerateFingerprint() {
    m_shapeControl->updateParameters(m_parameters.shape);
    m_densityControl->updateParameters(m_parameters.density);
    m_orientationControl->updateOrientationParameters(m_parameters.orientation);
    
    int width = m_parameters.shape.left + m_parameters.shape.right;
    int height = m_parameters.shape.top + m_parameters.shape.middle + m_parameters.shape.bottom;
    m_orientationControl->setImageDimensions(width, height);
    
    FingerprintClass currentClass = m_orientationControl->getCurrentClass();
    m_parameters.classification.fingerprintClass = currentClass;
    
    qDebug() << "[MainWindow] === GENERATE FINGERPRINT ===";
    qDebug() << "[MainWindow] Selected class:" << static_cast<int>(currentClass);
    
    // Usar pontos que já foram visualizados no orientation control
    m_singularPoints = m_orientationControl->getSingularPoints();
    
    qDebug() << "[MainWindow] Using visualization points: cores=" << m_singularPoints.getCoreCount() << ", deltas=" << m_singularPoints.getDeltaCount();
    
    if (m_worker->isRunning()) {
        qDebug() << "[MainWindow] Worker already running, cancelling...";
        m_worker->cancelGeneration();
        m_worker->wait();
    }
    
    m_worker->setParameters(m_parameters);
    m_worker->setSingularPoints(m_singularPoints);
    m_worker->setApplyEllipticalMask(m_ellipticalMaskCheckBox->isChecked());
    
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Generating fingerprint..."));
    
    qDebug() << "[MainWindow] Starting worker thread...";
    m_worker->start();
    qDebug() << "[MainWindow] Worker started, isRunning:" << m_worker->isRunning();
}

void MainWindow::onExportImage() {
    QImage currentImage = m_previewWidget->getImage();
    if (currentImage.isNull()) {
        QMessageBox::warning(this, tr("Export"), tr("No image to export. Please generate a fingerprint first."));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Image"),
                                                    QString(),
                                                    tr("PNG Image (*.png);;BMP Image (*.bmp);;TIFF Image (*.tiff)"));
    
    if (!fileName.isEmpty()) {
        if (currentImage.save(fileName)) {
            m_statusLabel->setText(tr("Image exported successfully"));
        } else {
            QMessageBox::critical(this, tr("Export Error"), tr("Failed to export image"));
        }
    }
}

void MainWindow::onLoadParameters() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Parameters"),
                                                    QString(),
                                                    tr("SFINGE Parameters (*.json)"));
    
    if (!fileName.isEmpty()) {
        if (m_parameters.loadFromJson(fileName)) {
            m_shapeControl->setParameters(m_parameters.shape);
            m_densityControl->setParameters(m_parameters.density);
            m_statusLabel->setText(tr("Parameters loaded successfully"));
        } else {
            QMessageBox::critical(this, tr("Load Error"), tr("Failed to load parameters"));
        }
    }
}

void MainWindow::onSaveParameters() {
    m_shapeControl->updateParameters(m_parameters.shape);
    m_densityControl->updateParameters(m_parameters.density);
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Parameters"),
                                                    QString(),
                                                    tr("SFINGE Parameters (*.json)"));
    
    if (!fileName.isEmpty()) {
        if (m_parameters.saveToJson(fileName)) {
            m_statusLabel->setText(tr("Parameters saved successfully"));
        } else {
            QMessageBox::critical(this, tr("Save Error"), tr("Failed to save parameters"));
        }
    }
}

void MainWindow::onResetParameters() {
    m_parameters.reset();
    m_shapeControl->setParameters(m_parameters.shape);
    m_densityControl->setParameters(m_parameters.density);
    m_statusLabel->setText(tr("Parameters reset to defaults"));
}

void MainWindow::onBatchGeneration() {
    BatchDialog dialog(this);
    dialog.exec();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About SFINGE-Qt6"),
                      tr("SFINGE-Qt6 - Synthetic Fingerprint Generator\n\n"
                         "A modern Qt6-based implementation of the SFINGE fingerprint generator."));
}

void MainWindow::onProgressChanged(int percentage, const QString& message) {
    qDebug() << "[MainWindow] Progress:" << percentage << message;
    m_progressBar->setValue(percentage);
    m_statusLabel->setText(message);
}

void MainWindow::onGenerationComplete() {
    qDebug() << "[MainWindow] Generation complete";
    m_progressBar->hide();
    m_statusLabel->setText(tr("Generation complete"));
}

void MainWindow::updatePreview(const QImage& image) {
    qDebug() << "[MainWindow] updatePreview called, image size:" << image.width() << "x" << image.height();
    m_previewWidget->setImage(image);
    qDebug() << "[MainWindow] Preview updated";
}

}
