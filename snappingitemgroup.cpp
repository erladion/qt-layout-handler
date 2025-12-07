#include "snappingitemgroup.h"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>  // For mouseRelease
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>

#include "layoutscene.h"
#include "snappingutils.h"

SnappingItemGroup::SnappingItemGroup(LayoutScene *scene, QGraphicsItem *parent)
    : QGraphicsItemGroup(parent), m_layoutScene(scene) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
           QGraphicsItem::ItemSendsGeometryChanges);
}

QRectF SnappingItemGroup::boundingRect() const {
  return QGraphicsItemGroup::boundingRect().adjusted(-3, -3, 3, 3);
}

void SnappingItemGroup::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option,
                              QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);
  painter->save();

  QPen pen;
  pen.setWidth(2);
  pen.setJoinStyle(Qt::MiterJoin);

  if (isSelected()) {
    pen.setColor(QColor(0, 120, 215));
    pen.setStyle(Qt::DashLine);
  } else {
    pen.setColor(QColor(120, 120, 120, 180));
    pen.setStyle(Qt::DotLine);
  }

  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  painter->drawRect(QGraphicsItemGroup::boundingRect());
  painter->restore();
}

QVariant SnappingItemGroup::itemChange(GraphicsItemChange change,
                                       const QVariant &value) {
  if (change == ItemPositionChange && m_layoutScene) {
    // FIX: Capture and display snap guides
    QList<QLineF> guides;
    QPointF newPos = SnappingUtils::snapPosition(
        m_layoutScene, this, value.toPointF(), boundingRect(), &guides);
    m_layoutScene->setSnapGuides(guides);
    return newPos;
  }
  return QGraphicsItemGroup::itemChange(change, value);
}

// FIX: Override mouseRelease to clear guides
void SnappingItemGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  QGraphicsItemGroup::mouseReleaseEvent(event);
  if (m_layoutScene) m_layoutScene->clearSnapGuides();
}
