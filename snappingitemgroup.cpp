#include "snappingitemgroup.h"
#include <QDebug>
#include "layoutscene.h"
#include "snappingutils.h"  // NEW INCLUDE

SnappingItemGroup::SnappingItemGroup(LayoutScene* scene, QGraphicsItem* parent) : QGraphicsItemGroup(parent), m_layoutScene(scene) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
}

QVariant SnappingItemGroup::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemPositionChange && m_layoutScene) {
    // REFACTORED: Use shared SnappingUtils
    // We pass boundingRect() which contains the content dimensions and offset
    return SnappingUtils::snapPosition(m_layoutScene, this, value.toPointF(), boundingRect());
  }
  return QGraphicsItemGroup::itemChange(change, value);
}
