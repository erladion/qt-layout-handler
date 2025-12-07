#ifndef SNAPPINGITEMGROUP_H
#define SNAPPINGITEMGROUP_H

#include <QGraphicsItemGroup>
#include <QPointF>

#include "constants.h"

class LayoutScene;
class QGraphicsSceneMouseEvent;

class SnappingItemGroup : public QGraphicsItemGroup {
public:
  explicit SnappingItemGroup(LayoutScene* scene, QGraphicsItem* parent = nullptr);
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  int type() const override { return Constants::Item::GroupItem; }

  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
  LayoutScene* m_pLayoutScene;
};

#endif  // SNAPPINGITEMGROUP_H
