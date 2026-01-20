#include "preview_widget.h"
#include <QVBoxLayout>
#include <QWheelEvent>

namespace SFinGe {

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_zoomFactor(1.0) {
    setupUi();
}

void PreviewWidget::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene, this);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform);
    m_pixmapItem = nullptr;
    
    layout->addWidget(m_view);
}

void PreviewWidget::setImage(const QImage& image) {
    m_currentImage = image;
    
    if (m_pixmapItem) {
        m_scene->removeItem(m_pixmapItem);
        delete m_pixmapItem;
    }
    
    if (!image.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(image);
        m_pixmapItem = m_scene->addPixmap(pixmap);
        m_scene->setSceneRect(pixmap.rect());
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void PreviewWidget::zoomIn() {
    m_zoomFactor *= 1.2;
    m_view->scale(1.2, 1.2);
}

void PreviewWidget::zoomOut() {
    m_zoomFactor /= 1.2;
    m_view->scale(1.0 / 1.2, 1.0 / 1.2);
}

void PreviewWidget::resetZoom() {
    m_view->resetTransform();
    m_zoomFactor = 1.0;
    if (!m_currentImage.isNull()) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void PreviewWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void PreviewWidget::updateView() {
    if (!m_currentImage.isNull()) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

}
