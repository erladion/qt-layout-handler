#include "guidelineitem.h"

#include <QCursor>
#include <QGraphicsScene>
#include <QPainter>

GuideLineItem::GuideLineItem(Orientation orientation, qreal pos, qreal length)
    : m_orientation(orientation) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
           QGraphicsItem::ItemSendsScenePositionChanges);

  QPen pen(QColor(0, 255, 255));
  pen.setStyle(Qt::DashLine);
  pen.setWidth(0);
  setPen(pen);

  if (m_orientation == Horizontal) {
    setLine(-length, 0, length, 0);
    setPos(0, pos);
    setCursor(Qt::SizeVerCursor);
  } else {
    setLine(0, -length, 0, length);
    setPos(pos, 0);
    setCursor(Qt::SizeHorCursor);
  }

  setZValue(1000);
}

QRectF GuideLineItem::boundingRect() const {
  QRectF rect = QGraphicsLineItem::boundingRect();
  qreal handleSize = 20.0;
  QRectF handleRect(-handleSize / 2, -handleSize / 2, handleSize, handleSize);
  return rect.united(handleRect);
}

void GuideLineItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->setPen(pen());
  painter->drawLine(line());

  painter->setBrush(QColor(0, 255, 255));
  painter->setPen(Qt::NoPen);

  if (isSelected()) {
    painter->setBrush(QColor(255, 100, 100));
  }

  if (m_orientation == Horizontal) {
    QPolygonF arrow;
    arrow << QPointF(-10, 0) << QPointF(0, -5) << QPointF(0, 5);
    painter->drawPolygon(arrow);
  } else {
    QPolygonF arrow;
    arrow << QPointF(0, -10) << QPointF(-5, 0) << QPointF(5, 0);
    painter->drawPolygon(arrow);
  }
}

QVariant GuideLineItem::itemChange(GraphicsItemChange change,
                                   const QVariant &value) {
  if (change == ItemPositionChange && scene()) {
    // FIX: Prevent guide from moving if it is selected but not being directly
    // dragged (i.e., part of a rubber band selection move) We check if a mouse
    // grabber exists and if it is NOT us.
    if (isSelected() && scene()->mouseGrabberItem() &&
        scene()->mouseGrabberItem() != this) {
      return pos();  // Reject the new position, stay put
    }

    QPointF newPos = value.toPointF();
    if (m_orientation == Horizontal) {
      newPos.setX(0);
    } else {
      newPos.setY(0);
    }
    return newPos;
  }
  return QGraphicsItem::itemChange(change, value);
}
