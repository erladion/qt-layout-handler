#include "snappingitemgroup.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include "constants.h"
#include "layoutscene.h"
#include "snappingutils.h"

SnappingItemGroup::SnappingItemGroup(LayoutScene* scene, QGraphicsItem* parent) : QGraphicsItemGroup(parent), m_pLayoutScene(scene) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
}

QRectF SnappingItemGroup::boundingRect() const {
  return QGraphicsItemGroup::boundingRect().adjusted(-3, -3, 3, 3);
}

void SnappingItemGroup::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);
  painter->save();

  QPen pen;
  pen.setWidth(2);
  pen.setJoinStyle(Qt::MiterJoin);

  if (isSelected()) {
    pen.setColor(QColor::fromRgba(Constants::Color::SelectionHighlight));
    pen.setStyle(Qt::DashLine);
  } else {
    pen.setColor(QColor::fromRgba(Constants::Color::GroupBorderPassive));
    pen.setStyle(Qt::DotLine);
  }

  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  painter->drawRect(QGraphicsItemGroup::boundingRect());
  painter->restore();
}

QVariant SnappingItemGroup::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemPositionChange && m_pLayoutScene) {
    QList<QLineF> guides;
    QPointF newPos = SnappingUtils::snapPosition(m_pLayoutScene, this, value.toPointF(), boundingRect(), &guides);
    m_pLayoutScene->setSnapGuides(guides);
    return newPos;
  }
  return QGraphicsItemGroup::itemChange(change, value);
}

void SnappingItemGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsItemGroup::mouseReleaseEvent(event);
  if (m_pLayoutScene) {
    m_pLayoutScene->clearSnapGuides();
  }
}
