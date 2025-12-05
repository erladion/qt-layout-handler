#ifndef SNAPPINGITEMGROUP_H
#define SNAPPINGITEMGROUP_H

#include <QGraphicsItemGroup>
#include <QPointF>

class LayoutScene;

class SnappingItemGroup : public QGraphicsItemGroup {
public:
  explicit SnappingItemGroup(LayoutScene* scene, QGraphicsItem* parent = nullptr);
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
  LayoutScene* m_layoutScene;
  // REMOVED: const double SNAP_DIST = 15.0; // Handled in SnappingUtils
  // REMOVED: Helper functions snapToGridVal, rangesOverlap // Handled in SnappingUtils
};

#endif  // SNAPPINGITEMGROUP_H
