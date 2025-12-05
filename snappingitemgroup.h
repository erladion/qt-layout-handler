#ifndef SNAPPINGITEMGROUP_H
#define SNAPPINGITEMGROUP_H

#include <QGraphicsItemGroup>
#include <QPointF>

class LayoutScene;

class SnappingItemGroup : public QGraphicsItemGroup {
public:
  // Constructor takes the scene pointer to access snapping rules (grid, bounds)
  explicit SnappingItemGroup(LayoutScene* scene, QGraphicsItem *parent = nullptr);

  // Override itemChange to implement custom snapping logic on the parent group
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
  LayoutScene* m_layoutScene;
  const double SNAP_DIST = 15.0;

  // Helper function to handle grid snap
  double snapToGridVal(double val, int gridSize);
  bool rangesOverlap(double min1, double len1, double min2, double len2);
};

#endif // SNAPPINGITEMGROUP_H
