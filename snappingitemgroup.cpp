#include "snappingitemgroup.h"
#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include "layoutscene.h"
#include "snappingutils.h"

SnappingItemGroup::SnappingItemGroup(LayoutScene* scene, QGraphicsItem* parent) : QGraphicsItemGroup(parent), m_layoutScene(scene) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
}

QRectF SnappingItemGroup::boundingRect() const {
  // Return the children's bounding rect expanded slightly for the border thickness
  // This ensures the drawing isn't clipped by the scene's update optimization
  return QGraphicsItemGroup::boundingRect().adjusted(-3, -3, 3, 3);
}

void SnappingItemGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  // We do NOT call QGraphicsItemGroup::paint because it has no default implementation
  // (it relies on children painting themselves). We just want to draw the frame on top.

  painter->save();

  QPen pen;
  pen.setWidth(2);
  pen.setJoinStyle(Qt::MiterJoin);

  if (isSelected()) {
    // Active Selection style
    pen.setColor(QColor(0, 120, 215));
    pen.setStyle(Qt::DashLine);
  } else {
    // Passive Group style (Always visible as requested)
    pen.setColor(QColor(120, 120, 120, 180));
    pen.setStyle(Qt::DotLine);
  }

  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);

  // Draw the rect around the children
  painter->drawRect(QGraphicsItemGroup::boundingRect());

  painter->restore();
}

QVariant SnappingItemGroup::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemPositionChange && m_layoutScene) {
    // Use shared SnappingUtils
    return SnappingUtils::snapPosition(m_layoutScene, this, value.toPointF(), boundingRect());
  }
  return QGraphicsItemGroup::itemChange(change, value);
}
