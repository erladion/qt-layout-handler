#ifndef SNAPPINGUTILS_H
#define SNAPPINGUTILS_H

#include <QList>
#include <QPointF>
#include <QRectF>
#include <Qt>  // For Qt::Orientation

class LayoutScene;
class QGraphicsItem;

class SnappingUtils {
public:
  static QPointF snapPosition(LayoutScene* scene, QGraphicsItem* item, const QPointF& proposedPos, const QRectF& logicalRect);

  static double snapToGridVal(double val, int gridSize);
  static bool rangesOverlap(double min1, double len1, double min2, double len2);

  static QList<QGraphicsItem*> getSnappingCandidates(LayoutScene* scene, const QRectF& queryRect, QGraphicsItem* ignoreItem);

  static bool isSnappableItem(QGraphicsItem* item);
  static bool isClose(double value, double target);

  // NEW: Centralized logic for snapping a single edge (for Resizing)
  static double snapValueToCandidates(double proposedValue,
                                      double orthoStart,
                                      double orthoLength,
                                      const QList<QGraphicsItem*>& candidates,
                                      Qt::Orientation snapAxis,
                                      bool& outSnapped);

  static constexpr double SNAP_DIST = 15.0;
};

#endif  // SNAPPINGUTILS_H
