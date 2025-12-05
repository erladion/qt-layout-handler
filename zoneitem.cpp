#include "zoneitem.h"
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPainter>
#include "layoutscene.h"

ZoneItem::ZoneItem(const QRectF& rect) : QGraphicsRectItem(rect), m_resizeHandle(None) {
  // Need ItemSendsGeometryChanges for itemChange to fire on move
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);
  setZValue(-500);
}

void ZoneItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  QPen pen(QColor(255, 165, 0), 2, Qt::DashLine);
  painter->setPen(pen);
  painter->setBrush(QColor(255, 165, 0, 40));
  painter->drawRect(rect());

  painter->setPen(QColor(255, 165, 0, 150));
  painter->drawText(rect(), Qt::AlignCenter, "ZONE");
}

// NEW: Constraint Logic (Same as Apps)
QVariant ZoneItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemPositionChange && scene()) {
    QPointF newPos = value.toPointF();
    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    QRectF validArea = layoutScene ? layoutScene->getWorkingArea() : scene()->sceneRect();
    QRectF myRect = rect();

    // Simple clamping to Working Area
    if (newPos.x() < validArea.left())
      newPos.setX(validArea.left());
    if (newPos.y() < validArea.top())
      newPos.setY(validArea.top());

    if (newPos.x() + myRect.width() > validArea.right())
      newPos.setX(validArea.right() - myRect.width());

    if (newPos.y() + myRect.height() > validArea.bottom())
      newPos.setY(validArea.bottom() - myRect.height());

    return newPos;
  }
  return QGraphicsRectItem::itemChange(change, value);
}

void ZoneItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
  int handle = getHandleAt(event->pos());
  QCursor cursor = Qt::ArrowCursor;
  if (handle == (Right | Bottom))
    cursor = Qt::SizeFDiagCursor;
  else if (handle & Right)
    cursor = Qt::SizeHorCursor;
  else if (handle & Bottom)
    cursor = Qt::SizeVerCursor;
  setCursor(cursor);
  QGraphicsRectItem::hoverMoveEvent(event);
}

void ZoneItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  m_resizeHandle = getHandleAt(event->pos());
  if (m_resizeHandle == None)
    QGraphicsRectItem::mousePressEvent(event);
}

void ZoneItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  if (m_resizeHandle != None) {
    QPointF mousePos = event->pos();
    QRectF r = rect();
    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    QRectF validArea = layoutScene ? layoutScene->getWorkingArea() : scene()->sceneRect();

    if (m_resizeHandle & Right) {
      r.setWidth(mousePos.x());
      // Constrain width
      if (pos().x() + r.width() > validArea.right())
        r.setWidth(validArea.right() - pos().x());
    }
    if (m_resizeHandle & Bottom) {
      r.setHeight(mousePos.y());
      // Constrain height
      if (pos().y() + r.height() > validArea.bottom())
        r.setHeight(validArea.bottom() - pos().y());
    }

    if (r.width() < 50)
      r.setWidth(50);
    if (r.height() < 50)
      r.setHeight(50);
    setRect(r);
  } else {
    QGraphicsRectItem::mouseMoveEvent(event);
  }
}

void ZoneItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
  QMenu menu;
  QAction* removeAction = menu.addAction("Remove Zone");
  if (menu.exec(event->screenPos()) == removeAction) {
    if (scene()) {
      scene()->removeItem(this);
      delete this;
    }
  }
}

int ZoneItem::getHandleAt(const QPointF& pt) {
  QRectF r = rect();
  int handle = None;
  qreal margin = 15;
  if (qAbs(pt.x() - r.right()) < margin)
    handle |= Right;
  if (qAbs(pt.y() - r.bottom()) < margin)
    handle |= Bottom;
  return handle;
}
