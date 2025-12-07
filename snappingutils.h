#ifndef SNAPPINGUTILS_H
#define SNAPPINGUTILS_H

#include <QLineF>
#include <QList>
#include <QPointF>
#include <QRectF>

class LayoutScene;
class QGraphicsItem;

class SnappingUtils {
public:
  static QPointF snapPosition(LayoutScene* scene,
                              QGraphicsItem* item,
                              const QPointF& proposedPos,
                              const QRectF& logicalRect,
                              QList<QLineF>* outGuides = nullptr);

  static double snapToGridVal(double val, int gridSize);
  static bool rangesOverlap(double min1, double len1, double min2, double len2);

  static QList<QGraphicsItem*> getSnappingCandidates(LayoutScene* scene, const QRectF& queryRect, QGraphicsItem* ignoreItem);

  static bool isSnappableItem(QGraphicsItem* item);
  static bool isClose(double value, double target);

  static double snapValueToCandidates(double proposedValue,
                                      double orthoStart,
                                      double orthoLength,
                                      const QList<QGraphicsItem*>& candidates,
                                      Qt::Orientation snapAxis,
                                      bool& outSnapped);
};

#endif  // SNAPPINGUTILS_H
