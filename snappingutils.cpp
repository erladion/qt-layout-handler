#include "snappingutils.h"
#include <QGraphicsItem>
#include <QtMath>
#include "guidelineitem.h"
#include "layoutscene.h"
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "zoneitem.h"

double SnappingUtils::snapToGridVal(double val, int gridSize) {
  return std::round(val / gridSize) * gridSize;
}

bool SnappingUtils::rangesOverlap(double min1, double len1, double min2, double len2) {
  double max1 = min1 + len1;
  double max2 = min2 + len2;
  return max1 > min2 && min1 < max2;
}

QPointF SnappingUtils::snapPosition(LayoutScene* scene, QGraphicsItem* selfItem, const QPointF& proposedPos, const QRectF& logicalRect) {
  if (!scene)
    return proposedPos;

  // 1. Calculate "Visual" coordinates (Future Scene Bounding Rect)
  // logicalRect is in local coordinates.
  // We assume the Item's origin (0,0) moves to proposedPos.
  // So the visual top-left in scene is proposedPos + logicalRect.topLeft()

  QPointF contentOffset = logicalRect.topLeft();

  double visualLeft = proposedPos.x() + contentOffset.x();
  double visualTop = proposedPos.y() + contentOffset.y();
  double width = logicalRect.width();
  double height = logicalRect.height();

  double visualRight = visualLeft + width;
  double visualBottom = visualTop + height;

  QRectF validArea = scene->getWorkingArea();
  bool snappedX = false;
  bool snappedY = false;

  // --- 2. Snap to Guide Lines ---
  for (QGraphicsItem* item : scene->items()) {
    if (GuideLineItem* guide = dynamic_cast<GuideLineItem*>(item)) {
      if (guide->orientation() == GuideLineItem::Vertical) {
        double guideX = guide->pos().x();
        if (qAbs(visualLeft - guideX) < SNAP_DIST) {
          visualLeft = guideX;
          snappedX = true;
        } else if (qAbs(visualRight - guideX) < SNAP_DIST) {
          visualLeft = guideX - width;
          snappedX = true;
        }
      } else {
        double guideY = guide->pos().y();
        if (qAbs(visualTop - guideY) < SNAP_DIST) {
          visualTop = guideY;
          snappedY = true;
        } else if (qAbs(visualBottom - guideY) < SNAP_DIST) {
          visualTop = guideY - height;
          snappedY = true;
        }
      }
    }
  }

  // --- 3. Snap to Scene Bounds ---
  if (!snappedX) {
    if (qAbs(visualLeft - validArea.left()) < SNAP_DIST) {
      visualLeft = validArea.left();
      snappedX = true;
    } else if (qAbs(visualRight - validArea.right()) < SNAP_DIST) {
      visualLeft = validArea.right() - width;
      snappedX = true;
    }
  }

  if (!snappedY) {
    if (qAbs(visualTop - validArea.top()) < SNAP_DIST) {
      visualTop = validArea.top();
      snappedY = true;
    } else if (qAbs(visualBottom - validArea.bottom()) < SNAP_DIST) {
      visualTop = validArea.bottom() - height;
      snappedY = true;
    }
  }

  // --- 4. Snap to Other Items ---
  if (!snappedX || !snappedY) {
    for (QGraphicsItem* item : scene->items()) {
      if (item == selfItem)
        continue;

      // Skip if 'item' is a child of 'selfItem' (e.g. inside the group moving)
      if (item->parentItem() == selfItem)
        continue;

      // Skip if 'selfItem' is a child of 'item' (should not happen in top-level check but good safety)
      if (selfItem->parentItem() == item)
        continue;

      if (!(dynamic_cast<ResizableAppItem*>(item) || dynamic_cast<ZoneItem*>(item) || dynamic_cast<SnappingItemGroup*>(item)))
        continue;

      QRectF otherRect = item->sceneBoundingRect();

      // Snap X
      if (!snappedX && rangesOverlap(visualTop, height, otherRect.top(), otherRect.height())) {
        if (qAbs(visualLeft - otherRect.right()) < SNAP_DIST) {
          visualLeft = otherRect.right();
          snappedX = true;
        } else if (qAbs(visualLeft - otherRect.left()) < SNAP_DIST) {
          visualLeft = otherRect.left();
          snappedX = true;
        } else if (qAbs(visualRight - otherRect.left()) < SNAP_DIST) {
          visualLeft = otherRect.left() - width;
          snappedX = true;
        } else if (qAbs(visualRight - otherRect.right()) < SNAP_DIST) {
          visualLeft = otherRect.right() - width;
          snappedX = true;
        }
      }

      // Snap Y
      if (!snappedY && rangesOverlap(visualLeft, width, otherRect.left(), otherRect.width())) {
        if (qAbs(visualTop - otherRect.bottom()) < SNAP_DIST) {
          visualTop = otherRect.bottom();
          snappedY = true;
        } else if (qAbs(visualTop - otherRect.top()) < SNAP_DIST) {
          visualTop = otherRect.top();
          snappedY = true;
        } else if (qAbs(visualBottom - otherRect.top()) < SNAP_DIST) {
          visualTop = otherRect.top() - height;
          snappedY = true;
        } else if (qAbs(visualBottom - otherRect.bottom()) < SNAP_DIST) {
          visualTop = otherRect.bottom() - height;
          snappedY = true;
        }
      }
    }
  }

  // --- 5. Snap to Grid ---
  if (scene->isGridEnabled()) {
    int gs = scene->gridSize();
    if (!snappedX)
      visualLeft = snapToGridVal(visualLeft, gs);
    if (!snappedY)
      visualTop = snapToGridVal(visualTop, gs);
  }

  // --- 6. Final Boundary Clamping ---
  if (visualLeft < validArea.left())
    visualLeft = validArea.left();
  if (visualLeft + width > validArea.right())
    visualLeft = validArea.right() - width;
  if (visualTop < validArea.top())
    visualTop = validArea.top();
  if (visualTop + height > validArea.bottom())
    visualTop = validArea.bottom() - height;

  // --- Convert Visual Position back to Item Position ---
  return QPointF(visualLeft - contentOffset.x(), visualTop - contentOffset.y());
}
