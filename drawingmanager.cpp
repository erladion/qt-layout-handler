#include "drawingmanager.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPen>

DrawingManager::DrawingManager(QGraphicsScene* scene, QObject* parent)
    : QObject(parent), m_pScene(scene), m_drawingLayer(new QGraphicsPathItem()), m_activeDrawItem(nullptr), m_currentShape(Shape::Freehand),
      m_drawColor(Qt::blue), m_drawSize(4) {
  QPen drawPen(Qt::yellow);
  drawPen.setWidth(4);
  drawPen.setCapStyle(Qt::RoundCap);
  drawPen.setJoinStyle(Qt::RoundJoin);
  m_drawingLayer->setPen(drawPen);
  m_drawingLayer->setZValue(9998);
  m_pScene->addItem(m_drawingLayer);
}

DrawingManager::~DrawingManager() {
  clearDrawings();
}

void DrawingManager::setShape(Shape shape) {
  m_currentShape = shape;
}
void DrawingManager::setColor(const QColor& color) {
  m_drawColor = color;
}
void DrawingManager::setSize(int size) {
  m_drawSize = size;
}

void DrawingManager::clearDrawings() {
  for (QGraphicsItem* item : std::as_const(m_drawnItems)) {
    if (m_pScene && item->scene() == m_pScene) {
      m_pScene->removeItem(item);
    }
    delete item;
  }
  m_drawnItems.clear();
  m_currentPath.clear();
  if (m_drawingLayer) {
    m_drawingLayer->setPath(m_currentPath);
  }
  m_activeDrawItem = nullptr;
}

bool DrawingManager::handleViewportEvent(QEvent* event, QGraphicsView* view) {
  if (!m_pScene) {
    return false;
  }

  if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
    const QPointF scenePos = view->mapToScene(mouseEvent->pos());

    if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton) {
      m_drawStartPos = scenePos;
      QPen drawPen(m_drawColor, m_drawSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

      QAbstractGraphicsShapeItem* item = nullptr;
      if (m_currentShape == Shape::Freehand || m_currentShape == Shape::Marker) {
        if (m_currentShape == Shape::Marker) {
          QColor markerColor = m_drawColor;
          markerColor.setAlpha(90);
          drawPen.setWidth(m_drawSize * 2);
          drawPen.setColor(markerColor);
          drawPen.setCapStyle(Qt::SquareCap);
          drawPen.setJoinStyle(Qt::MiterJoin);
        }
        QGraphicsPathItem* pathItem = new QGraphicsPathItem();
        m_currentPath.clear();
        m_currentPath.moveTo(scenePos);
        pathItem->setPath(m_currentPath);
        item = pathItem;
      } else if (m_currentShape == Shape::Rectangle) {
        QGraphicsRectItem* rectItem = new QGraphicsRectItem();
        rectItem->setRect(QRectF(scenePos, scenePos));
        item = rectItem;
      } else if (m_currentShape == Shape::Ellipse) {
        QGraphicsEllipseItem* ellipseItem = new QGraphicsEllipseItem();
        ellipseItem->setRect(QRectF(scenePos, scenePos));
        item = ellipseItem;
      }

      if (item != nullptr) {
        item->setPen(drawPen);
        item->setZValue(900);

        m_pScene->addItem(item);
        m_activeDrawItem = item;
      }

      if (m_activeDrawItem) {
        m_drawnItems.append(m_activeDrawItem);
      }
      return true;
    } else if (event->type() == QEvent::MouseMove && (mouseEvent->buttons() & Qt::LeftButton)) {
      if (m_activeDrawItem) {
        if (m_currentShape == Shape::Freehand || m_currentShape == Shape::Marker) {
          m_currentPath.lineTo(scenePos);
          static_cast<QGraphicsPathItem*>(m_activeDrawItem)->setPath(m_currentPath);
        } else if (m_currentShape == Shape::Rectangle) {
          QRectF newRect = QRectF(m_drawStartPos, scenePos).normalized();
          static_cast<QGraphicsRectItem*>(m_activeDrawItem)->setRect(newRect);
        } else if (m_currentShape == Shape::Ellipse) {
          QRectF newRect = QRectF(m_drawStartPos, scenePos).normalized();
          static_cast<QGraphicsEllipseItem*>(m_activeDrawItem)->setRect(newRect);
        }
      }
      return true;
    } else if (event->type() == QEvent::MouseButtonRelease && mouseEvent->button() == Qt::LeftButton) {
      m_activeDrawItem = nullptr;
      return true;
    }
  }
  return false;
}
