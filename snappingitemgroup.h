#ifndef SNAPPINGITEMGROUP_H
#define SNAPPINGITEMGROUP_H

#include <QGraphicsItemGroup>
#include <QPointF>

class LayoutScene;

class SnappingItemGroup : public QGraphicsItemGroup {
public:
  explicit SnappingItemGroup(LayoutScene* scene, QGraphicsItem* parent = nullptr);
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  // Override to draw the group border
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  LayoutScene* m_layoutScene;
};

#endif  // SNAPPINGITEMGROUP_H
