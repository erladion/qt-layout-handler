#ifndef SNAPPINGUTILS_H
#define SNAPPINGUTILS_H

#include <QList>
#include <QPointF>
#include <QRectF>

class LayoutScene;
class QGraphicsItem;

class SnappingUtils {
public:
  static QPointF snapPosition(LayoutScene* scene, QGraphicsItem* item, const QPointF& proposedPos, const QRectF& logicalRect);

  static double snapToGridVal(double val, int gridSize);
  static bool rangesOverlap(double min1, double len1, double min2, double len2);

  static QList<QGraphicsItem*> getSnappingCandidates(LayoutScene* scene, const QRectF& queryRect, QGraphicsItem* ignoreItem);

  static bool isSnappableItem(QGraphicsItem* item);

  // NEW: Exposed helper for distance checking
  static bool isClose(double value, double target);

  static constexpr double SNAP_DIST = 15.0;
};

#endif  // SNAPPINGUTILS_H
