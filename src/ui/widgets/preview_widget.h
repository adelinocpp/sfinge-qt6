#ifndef PREVIEW_WIDGET_H
#define PREVIEW_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QMenu>
#include "models/singular_points.h"

namespace SFinGe {

class InteractiveGraphicsView;

class PreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    
    void setImage(const QImage& image);
    QImage getImage() const { return m_currentImage; }
    
    void setSingularPoints(const SingularPoints& points);
    SingularPoints getSingularPoints() const { return m_singularPoints; }
    void setPointsEditable(bool editable) { m_pointsEditable = editable; }
    
    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void pointMoved(int pointType, int index, double x, double y);
    void pointAdded(int pointType, double x, double y);
    void singularPointsChanged();

protected:
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onContextMenuRequested(const QPoint& pos);
    void onAddCoreHere();
    void onAddDeltaHere();
    void onRemovePointHere();

private:
    void setupUi();
    void updateView();
    void updatePointMarkers();
    void handlePointDrag(int pointType, int index, const QPointF& newPos);
    
    InteractiveGraphicsView* m_view;
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_pixmapItem;
    QImage m_currentImage;
    double m_zoomFactor;
    
    SingularPoints m_singularPoints;
    QList<QGraphicsEllipseItem*> m_coreMarkers;
    QList<QGraphicsEllipseItem*> m_deltaMarkers;
    bool m_pointsEditable;
    QPointF m_contextMenuPos;
    int m_contextMenuPointType;  // 0 = core, 1 = delta, -1 = none
    int m_contextMenuPointIndex;
    
    friend class InteractiveGraphicsView;
};

class InteractiveGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit InteractiveGraphicsView(QGraphicsScene* scene, PreviewWidget* parent);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int findPointAt(const QPointF& scenePos, int& pointType);
    
    PreviewWidget* m_previewWidget;
    bool m_dragging;
    int m_dragPointType;  // 0 = core, 1 = delta
    int m_dragPointIndex;
    QPointF m_dragStartPos;
};

}

#endif
