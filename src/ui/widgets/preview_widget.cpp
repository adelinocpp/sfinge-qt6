#include "preview_widget.h"
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>

namespace SFinGe {

// ============================================================================
// InteractiveGraphicsView Implementation
// ============================================================================

InteractiveGraphicsView::InteractiveGraphicsView(QGraphicsScene* scene, PreviewWidget* parent)
    : QGraphicsView(scene, parent)
    , m_previewWidget(parent)
    , m_dragging(false)
    , m_dragPointType(-1)
    , m_dragPointIndex(-1)
{
    setMouseTracking(true);
}

int InteractiveGraphicsView::findPointAt(const QPointF& scenePos, int& pointType) {
    const double hitRadius = 10.0;
    
    // Check cores first
    const auto& cores = m_previewWidget->m_singularPoints.getCores();
    for (size_t i = 0; i < cores.size(); ++i) {
        double dx = scenePos.x() - cores[i].x;
        double dy = scenePos.y() - cores[i].y;
        if (dx*dx + dy*dy <= hitRadius * hitRadius) {
            pointType = 0;  // core
            return static_cast<int>(i);
        }
    }
    
    // Check deltas
    const auto& deltas = m_previewWidget->m_singularPoints.getDeltas();
    for (size_t i = 0; i < deltas.size(); ++i) {
        double dx = scenePos.x() - deltas[i].x;
        double dy = scenePos.y() - deltas[i].y;
        if (dx*dx + dy*dy <= hitRadius * hitRadius) {
            pointType = 1;  // delta
            return static_cast<int>(i);
        }
    }
    
    pointType = -1;
    return -1;
}

void InteractiveGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (!m_previewWidget->m_pointsEditable) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        int pointType;
        int index = findPointAt(scenePos, pointType);
        
        if (index >= 0) {
            m_dragging = true;
            m_dragPointType = pointType;
            m_dragPointIndex = index;
            m_dragStartPos = scenePos;
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void InteractiveGraphicsView::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && m_dragPointIndex >= 0) {
        QPointF scenePos = mapToScene(event->pos());
        m_previewWidget->handlePointDrag(m_dragPointType, m_dragPointIndex, scenePos);
        event->accept();
        return;
    }
    
    // Change cursor when hovering over points
    if (m_previewWidget->m_pointsEditable) {
        QPointF scenePos = mapToScene(event->pos());
        int pointType;
        int index = findPointAt(scenePos, pointType);
        if (index >= 0) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void InteractiveGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragging) {
        m_dragging = false;
        m_dragPointType = -1;
        m_dragPointIndex = -1;
        setCursor(Qt::ArrowCursor);
        
        emit m_previewWidget->singularPointsChanged();
        event->accept();
        return;
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

// ============================================================================
// PreviewWidget Implementation
// ============================================================================

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_zoomFactor(1.0)
    , m_pixmapItem(nullptr)
    , m_pointsEditable(true)
    , m_contextMenuPointType(-1)
    , m_contextMenuPointIndex(-1)
{
    setupUi();
}

void PreviewWidget::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_scene = new QGraphicsScene(this);
    m_view = new InteractiveGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(m_view, &QWidget::customContextMenuRequested, 
            this, &PreviewWidget::onContextMenuRequested);
    
    layout->addWidget(m_view);
}

void PreviewWidget::setImage(const QImage& image) {
    m_currentImage = image;
    
    if (m_pixmapItem) {
        m_scene->removeItem(m_pixmapItem);
        delete m_pixmapItem;
        m_pixmapItem = nullptr;
    }
    
    if (!image.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(image);
        m_pixmapItem = m_scene->addPixmap(pixmap);
        m_scene->setSceneRect(pixmap.rect());
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
    
    updatePointMarkers();
}

void PreviewWidget::setSingularPoints(const SingularPoints& points) {
    m_singularPoints = points;
    updatePointMarkers();
}

void PreviewWidget::updatePointMarkers() {
    // Remove existing markers
    for (auto* marker : m_coreMarkers) {
        m_scene->removeItem(marker);
        delete marker;
    }
    m_coreMarkers.clear();
    
    for (auto* marker : m_deltaMarkers) {
        m_scene->removeItem(marker);
        delete marker;
    }
    m_deltaMarkers.clear();
    
    if (m_currentImage.isNull()) {
        return;
    }
    
    const double markerRadius = 6.0;
    
    // Add core markers (red)
    const auto& cores = m_singularPoints.getCores();
    for (const auto& core : cores) {
        QGraphicsEllipseItem* marker = m_scene->addEllipse(
            core.x - markerRadius, core.y - markerRadius,
            markerRadius * 2, markerRadius * 2,
            QPen(Qt::red, 2),
            QBrush(QColor(255, 0, 0, 128))
        );
        marker->setZValue(100);
        m_coreMarkers.append(marker);
    }
    
    // Add delta markers (green triangles represented as ellipses for simplicity)
    const auto& deltas = m_singularPoints.getDeltas();
    for (const auto& delta : deltas) {
        QGraphicsEllipseItem* marker = m_scene->addEllipse(
            delta.x - markerRadius, delta.y - markerRadius,
            markerRadius * 2, markerRadius * 2,
            QPen(Qt::green, 2),
            QBrush(QColor(0, 255, 0, 128))
        );
        marker->setZValue(100);
        m_deltaMarkers.append(marker);
    }
}

void PreviewWidget::handlePointDrag(int pointType, int index, const QPointF& newPos) {
    // Clamp to image bounds
    double x = qBound(0.0, newPos.x(), static_cast<double>(m_currentImage.width() - 1));
    double y = qBound(0.0, newPos.y(), static_cast<double>(m_currentImage.height() - 1));
    
    if (pointType == 0) {  // core
        m_singularPoints.updateCore(index, x, y);
        if (index < m_coreMarkers.size()) {
            m_coreMarkers[index]->setRect(x - 6, y - 6, 12, 12);
        }
    } else if (pointType == 1) {  // delta
        m_singularPoints.updateDelta(index, x, y);
        if (index < m_deltaMarkers.size()) {
            m_deltaMarkers[index]->setRect(x - 6, y - 6, 12, 12);
        }
    }
    
    emit pointMoved(pointType, index, x, y);
}

void PreviewWidget::onContextMenuRequested(const QPoint& pos) {
    m_contextMenuPos = m_view->mapToScene(pos);
    
    // Check if click is within image bounds
    if (m_currentImage.isNull() || 
        m_contextMenuPos.x() < 0 || m_contextMenuPos.x() >= m_currentImage.width() ||
        m_contextMenuPos.y() < 0 || m_contextMenuPos.y() >= m_currentImage.height()) {
        return;
    }
    
    // Check if clicking on an existing point
    const double hitRadius = 10.0;
    m_contextMenuPointType = -1;
    m_contextMenuPointIndex = -1;
    
    // Check cores
    const auto& cores = m_singularPoints.getCores();
    for (size_t i = 0; i < cores.size(); ++i) {
        double dx = m_contextMenuPos.x() - cores[i].x;
        double dy = m_contextMenuPos.y() - cores[i].y;
        if (dx*dx + dy*dy <= hitRadius * hitRadius) {
            m_contextMenuPointType = 0;
            m_contextMenuPointIndex = static_cast<int>(i);
            break;
        }
    }
    
    // Check deltas if no core found
    if (m_contextMenuPointIndex < 0) {
        const auto& deltas = m_singularPoints.getDeltas();
        for (size_t i = 0; i < deltas.size(); ++i) {
            double dx = m_contextMenuPos.x() - deltas[i].x;
            double dy = m_contextMenuPos.y() - deltas[i].y;
            if (dx*dx + dy*dy <= hitRadius * hitRadius) {
                m_contextMenuPointType = 1;
                m_contextMenuPointIndex = static_cast<int>(i);
                break;
            }
        }
    }
    
    QMenu contextMenu(this);
    
    // If clicking on a point, show remove option
    if (m_contextMenuPointIndex >= 0) {
        QString pointName = (m_contextMenuPointType == 0) 
            ? tr("Core %1").arg(m_contextMenuPointIndex + 1)
            : tr("Delta %1").arg(m_contextMenuPointIndex + 1);
        
        QAction* removeAction = contextMenu.addAction(tr("Remove %1").arg(pointName));
        connect(removeAction, &QAction::triggered, this, &PreviewWidget::onRemovePointHere);
        contextMenu.addSeparator();
    }
    
    QAction* addCoreAction = contextMenu.addAction(tr("Add Core Point Here"));
    connect(addCoreAction, &QAction::triggered, this, &PreviewWidget::onAddCoreHere);
    
    QAction* addDeltaAction = contextMenu.addAction(tr("Add Delta Point Here"));
    connect(addDeltaAction, &QAction::triggered, this, &PreviewWidget::onAddDeltaHere);
    
    contextMenu.exec(m_view->mapToGlobal(pos));
}

void PreviewWidget::onAddCoreHere() {
    double x = m_contextMenuPos.x();
    double y = m_contextMenuPos.y();
    
    m_singularPoints.addCore(x, y);
    updatePointMarkers();
    
    qDebug() << "[PreviewWidget] Added core at (" << x << "," << y << ")";
    emit pointAdded(0, x, y);
    emit singularPointsChanged();
}

void PreviewWidget::onAddDeltaHere() {
    double x = m_contextMenuPos.x();
    double y = m_contextMenuPos.y();
    
    m_singularPoints.addDelta(x, y);
    updatePointMarkers();
    
    qDebug() << "[PreviewWidget] Added delta at (" << x << "," << y << ")";
    emit pointAdded(1, x, y);
    emit singularPointsChanged();
}

void PreviewWidget::onRemovePointHere() {
    if (m_contextMenuPointIndex < 0) {
        return;
    }
    
    if (m_contextMenuPointType == 0) {
        qDebug() << "[PreviewWidget] Removing core" << m_contextMenuPointIndex;
        m_singularPoints.removeCore(m_contextMenuPointIndex);
    } else if (m_contextMenuPointType == 1) {
        qDebug() << "[PreviewWidget] Removing delta" << m_contextMenuPointIndex;
        m_singularPoints.removeDelta(m_contextMenuPointIndex);
    }
    
    updatePointMarkers();
    emit singularPointsChanged();
    
    // Reset context menu state
    m_contextMenuPointType = -1;
    m_contextMenuPointIndex = -1;
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
