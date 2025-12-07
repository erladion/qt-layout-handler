#ifndef SNAPPINGITEMGROUP_H
#define SNAPPINGITEMGROUP_H

#include <QGraphicsItemGroup>
#include <QPointF>

class LayoutScene;
class QGraphicsSceneMouseEvent;  // Forward declaration

class SnappingItemGroup : public QGraphicsItemGroup {
 public:
  explicit SnappingItemGroup(LayoutScene *scene,
                             QGraphicsItem *parent = nullptr);
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

  // Override to draw the group border
  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

 protected:
  // FIX: Declare the mouse event handler
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

 private:
  LayoutScene *m_layoutScene;
};

#endif  // SNAPPINGITEMGROUP_H
