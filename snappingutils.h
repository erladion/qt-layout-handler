#ifndef SNAPPINGUTILS_H
#define SNAPPINGUTILS_H

#include <QPointF>
#include <QRectF>

class LayoutScene;
class QGraphicsItem;

class SnappingUtils {
public:
  // Main function to calculate snapped position
  // logicalRect: The local rectangle of the item (e.g., item->rect() or item->boundingRect())
  static QPointF snapPosition(LayoutScene* scene, QGraphicsItem* item, const QPointF& proposedPos, const QRectF& logicalRect);

  // Helpers (exposed for use in Resizing logic if needed)
  static double snapToGridVal(double val, int gridSize);
  static bool rangesOverlap(double min1, double len1, double min2, double len2);

private:
  static constexpr double SNAP_DIST = 15.0;
};

#endif  // SNAPPINGUTILS_H
