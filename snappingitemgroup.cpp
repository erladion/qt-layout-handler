#include "snappingitemgroup.h"
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <limits>
#include "guidelineitem.h"  // NEW
#include "layoutscene.h"
#include "resizableappitem.h"
#include "zoneitem.h"

SnappingItemGroup::SnappingItemGroup(LayoutScene* scene, QGraphicsItem* parent) : QGraphicsItemGroup(parent), m_layoutScene(scene) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
}

double SnappingItemGroup::snapToGridVal(double val, int gridSize) {
  return std::round(val / gridSize) * gridSize;
}

bool SnappingItemGroup::rangesOverlap(double min1, double len1, double min2, double len2) {
  double max1 = min1 + len1;
  double max2 = min2 + len2;
  return max1 > min2 && min1 < max2;
}

QVariant SnappingItemGroup::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemPositionChange && m_layoutScene) {
    QPointF newPos = value.toPointF();

    QRectF localBounds = boundingRect();
    QPointF contentOffset = localBounds.topLeft();

    double visualLeft = newPos.x() + contentOffset.x();
    double visualTop = newPos.y() + contentOffset.y();
    double width = localBounds.width();
    double height = localBounds.height();

    double visualRight = visualLeft + width;
    double visualBottom = visualTop + height;

    QRectF validArea = m_layoutScene->getWorkingArea();
    bool snappedX = false;
    bool snappedY = false;

    // --- 1. Snap to Guide Lines (Priority) ---
    for (QGraphicsItem* item : m_layoutScene->items()) {
      if (GuideLineItem* guide = dynamic_cast<GuideLineItem*>(item)) {
        if (guide->orientation() == GuideLineItem::Vertical) {
          // Snap Left/Right edges to Vertical Line
          double guideX = guide->pos().x();
          if (qAbs(visualLeft - guideX) < SNAP_DIST) {
            visualLeft = guideX;
            snappedX = true;
          } else if (qAbs(visualRight - guideX) < SNAP_DIST) {
            visualLeft = guideX - width;
            snappedX = true;
          }
        } else {
          // Snap Top/Bottom edges to Horizontal Line
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

    // --- 2. Snap to Scene Bounds ---
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

    // --- 3. Snap to Other Items ---
    if (!snappedX || !snappedY) {
      for (QGraphicsItem* item : m_layoutScene->items()) {
        if (item == this)
          continue;
        if (item->parentItem() == this)
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

    // --- 4. Snap to Grid ---
    if (m_layoutScene->isGridEnabled()) {
      int gs = m_layoutScene->gridSize();
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
  return QGraphicsItemGroup::itemChange(change, value);
}
