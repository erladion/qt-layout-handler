#include "snappingutils.h"
#include <QGraphicsItem>
#include <QtMath>
#include "guidelineitem.h"
#include "layoutscene.h"
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "zoneitem.h"

// --- Class Implementation ---

bool SnappingUtils::isSnappableItem(QGraphicsItem* item) {
  return (dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item) || dynamic_cast<SnappingItemGroup*>(item));
}

// Promoted to class method
bool SnappingUtils::isClose(double value, double target) {
  return qAbs(value - target) < SNAP_DIST;
}

double SnappingUtils::snapToGridVal(double val, int gridSize) {
  return std::round(val / gridSize) * gridSize;
}

bool SnappingUtils::rangesOverlap(double min1, double len1, double min2, double len2) {
  double max1 = min1 + len1;
  double max2 = min2 + len2;
  return max1 > min2 && min1 < max2;
}

QList<QGraphicsItem*> SnappingUtils::getSnappingCandidates(LayoutScene* scene, const QRectF& queryRect, QGraphicsItem* ignoreItem) {
  QRectF searchRect = queryRect.adjusted(-SNAP_DIST, -SNAP_DIST, SNAP_DIST, SNAP_DIST);
  QList<QGraphicsItem*> candidates = scene->items(searchRect, Qt::IntersectsItemBoundingRect);

  QList<QGraphicsItem*> valid;
  valid.reserve(candidates.size());

  for (QGraphicsItem* item : candidates) {
    if (item == ignoreItem)
      continue;
    if (item->parentItem() == ignoreItem)
      continue;
    if (ignoreItem->parentItem() == item)
      continue;

    if (isSnappableItem(item)) {
      valid.append(item);
    }
  }
  return valid;
}

QPointF SnappingUtils::snapPosition(LayoutScene* scene, QGraphicsItem* selfItem, const QPointF& proposedPos, const QRectF& logicalRect) {
  if (!scene)
    return proposedPos;

  QPointF contentOffset = logicalRect.topLeft();

  double visualLeft = proposedPos.x() + contentOffset.x();
  double visualTop = proposedPos.y() + contentOffset.y();
  double width = logicalRect.width();
  double height = logicalRect.height();

  double visualRight = visualLeft + width;
  double visualBottom = visualTop + height;

  QRectF visualRect(visualLeft, visualTop, width, height);
  QRectF validArea = scene->getWorkingArea();

  bool snappedX = false;
  bool snappedY = false;

  // --- 1. Snap to Guide Lines ---
  for (QGraphicsItem* item : scene->items()) {
    if (GuideLineItem* guide = dynamic_cast<GuideLineItem*>(item)) {
      if (guide->orientation() == GuideLineItem::Vertical) {
        double guideX = guide->pos().x();
        if (isClose(visualLeft, guideX)) {
          visualLeft = guideX;
          snappedX = true;
        } else if (isClose(visualRight, guideX)) {
          visualLeft = guideX - width;
          snappedX = true;
        }
      } else {
        double guideY = guide->pos().y();
        if (isClose(visualTop, guideY)) {
          visualTop = guideY;
          snappedY = true;
        } else if (isClose(visualBottom, guideY)) {
          visualTop = guideY - height;
          snappedY = true;
        }
      }
    }
  }

  // --- 2. Snap to Scene Bounds ---
  if (!snappedX) {
    if (isClose(visualLeft, validArea.left())) {
      visualLeft = validArea.left();
      snappedX = true;
    } else if (isClose(visualRight, validArea.right())) {
      visualLeft = validArea.right() - width;
      snappedX = true;
    }
  }

  if (!snappedY) {
    if (isClose(visualTop, validArea.top())) {
      visualTop = validArea.top();
      snappedY = true;
    } else if (isClose(visualBottom, validArea.bottom())) {
      visualTop = validArea.bottom() - height;
      snappedY = true;
    }
  }

  // --- 3. Snap to Other Items ---
  if (!snappedX || !snappedY) {
    QList<QGraphicsItem*> nearbyItems = getSnappingCandidates(scene, visualRect, selfItem);

    for (QGraphicsItem* item : nearbyItems) {
      QRectF otherRect = item->sceneBoundingRect();
      double otherL = otherRect.left();
      double otherR = otherRect.right();
      double otherT = otherRect.top();
      double otherB = otherRect.bottom();

      if (!snappedX && rangesOverlap(visualTop, height, otherT, otherRect.height())) {
        if (isClose(visualLeft, otherR)) {
          visualLeft = otherR;
          snappedX = true;
        } else if (isClose(visualLeft, otherL)) {
          visualLeft = otherL;
          snappedX = true;
        } else if (isClose(visualRight, otherL)) {
          visualLeft = otherL - width;
          snappedX = true;
        } else if (isClose(visualRight, otherR)) {
          visualLeft = otherR - width;
          snappedX = true;
        }
      }

      if (!snappedY && rangesOverlap(visualLeft, width, otherL, otherRect.width())) {
        if (isClose(visualTop, otherB)) {
          visualTop = otherB;
          snappedY = true;
        } else if (isClose(visualTop, otherT)) {
          visualTop = otherT;
          snappedY = true;
        } else if (isClose(visualBottom, otherT)) {
          visualTop = otherT - height;
          snappedY = true;
        } else if (isClose(visualBottom, otherB)) {
          visualTop = otherB - height;
          snappedY = true;
        }
      }
    }
  }

  // --- 4. Snap to Grid ---
  if (scene->isGridEnabled()) {
    int gs = scene->gridSize();
    if (!snappedX)
      visualLeft = snapToGridVal(visualLeft, gs);
    if (!snappedY)
      visualTop = snapToGridVal(visualTop, gs);
  }

  // --- 5. Final Boundary Clamping ---
  if (visualLeft < validArea.left())
    visualLeft = validArea.left();
  if (visualLeft + width > validArea.right())
    visualLeft = validArea.right() - width;
  if (visualTop < validArea.top())
    visualTop = validArea.top();
  if (visualTop + height > validArea.bottom())
    visualTop = validArea.bottom() - height;

  return QPointF(visualLeft - contentOffset.x(), visualTop - contentOffset.y());
}
