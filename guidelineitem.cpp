#include "guidelineitem.h"
#include <QCursor>
#include <QGraphicsScene>
#include <QPainter>

GuideLineItem::GuideLineItem(Orientation orientation, qreal pos, qreal length) : m_orientation(orientation) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsScenePositionChanges);

  // Cyan color, dashed line
  QPen pen(QColor(0, 255, 255));
  pen.setStyle(Qt::DashLine);
  pen.setWidth(0);  // Cosmetic
  setPen(pen);

  if (m_orientation == Horizontal) {
    setLine(-length, 0, length, 0);
    setPos(0, pos);
    // Horizontal line moves Vertically (Up/Down)
    setCursor(Qt::SizeVerCursor);
  } else {
    setLine(0, -length, 0, length);
    setPos(pos, 0);
    // Vertical line moves Horizontally (Left/Right)
    setCursor(Qt::SizeHorCursor);
  }

  setZValue(1000);  // Always on top
}

// FIX: Define the visual bounds to include the arrows
QRectF GuideLineItem::boundingRect() const {
  // Get the bounds of the infinite line
  QRectF rect = QGraphicsLineItem::boundingRect();

  // Define the area covered by the arrow handles at (0,0)
  // The arrows are roughly 10px wide/tall, so a 20x20 box is safe
  qreal handleSize = 20.0;
  QRectF handleRect(-handleSize / 2, -handleSize / 2, handleSize, handleSize);

  // Return the combined area so the scene knows to redraw it all
  return rect.united(handleRect);
}

void GuideLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->setPen(pen());

  // Draw the infinite line
  painter->drawLine(line());

  // Draw a small handle or tag to indicate it's selectable/draggable
  painter->setBrush(QColor(0, 255, 255));
  painter->setPen(Qt::NoPen);

  // Draw a small triangle indicator at the "origin" of the line for easy grabbing
  if (isSelected()) {
    painter->setBrush(QColor(255, 100, 100));  // Red when selected
  }

  if (m_orientation == Horizontal) {
    // Left arrow
    QPolygonF arrow;
    arrow << QPointF(-10, 0) << QPointF(0, -5) << QPointF(0, 5);
    painter->drawPolygon(arrow);
  } else {
    // Up arrow
    QPolygonF arrow;
    arrow << QPointF(0, -10) << QPointF(-5, 0) << QPointF(5, 0);
    painter->drawPolygon(arrow);
  }
}

QVariant GuideLineItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemPositionChange && scene()) {
    // Lock axis movement
    QPointF newPos = value.toPointF();
    if (m_orientation == Horizontal) {
      newPos.setX(0);  // Lock X, can only move Y
    } else {
      newPos.setY(0);  // Lock Y, can only move X
    }
    return newPos;
  }
  return QGraphicsItem::itemChange(change, value);
}
