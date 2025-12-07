#include "snappingutils.h"

#include <QGraphicsItem>
#include <QtMath>

#include "constants.h"
#include "guidelineitem.h"
#include "layoutscene.h"
#include "resizableappitem.h"
#include "snappingitemgroup.h"
#include "zoneitem.h"

bool SnappingUtils::isSnappableItem(QGraphicsItem* item) {
  return (item->type() == Constants::Item::AppItem || item->type() == Constants::Item::ZoneItem || item->type() == Constants::Item::GroupItem);
}

bool SnappingUtils::isClose(double value, double target) {
  return qAbs(value - target) < Constants::SnapDistance;
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
  QRectF searchRect = queryRect.adjusted(-Constants::SnapDistance, -Constants::SnapDistance, Constants::SnapDistance, Constants::SnapDistance);
  QList<QGraphicsItem*> candidates = scene->items(searchRect, Qt::IntersectsItemBoundingRect);

  QList<QGraphicsItem*> valid;
  valid.reserve(candidates.size());

  for (QGraphicsItem* item : candidates) {
    if (item == ignoreItem) {
      continue;
    }

    if (item->parentItem() == ignoreItem) {
      continue;
    }

    if (ignoreItem->parentItem() == item) {
      continue;
    }

    if (isSnappableItem(item)) {
      valid.append(item);
    }
  }
  return valid;
}

double SnappingUtils::snapValueToCandidates(double proposedValue,
                                            double orthoStart,
                                            double orthoLength,
                                            const QList<QGraphicsItem*>& candidates,
                                            Qt::Orientation snapAxis,
                                            bool& outSnapped) {
  outSnapped = false;

  for (QGraphicsItem* item : candidates) {
    QRectF other = item->mapRectToScene(item->boundingRect());

    double oStart, oLen;
    if (snapAxis == Qt::Horizontal) {  // Snapping X (checking Y overlap)
      oStart = other.top();
      oLen = other.height();
    } else {  // Snapping Y (checking X overlap)
      oStart = other.left();
      oLen = other.width();
    }

    if (rangesOverlap(orthoStart, orthoLength, oStart, oLen)) {
      double target1, target2;
      if (snapAxis == Qt::Horizontal) {
        target1 = other.left();
        target2 = other.right();
      } else {
        target1 = other.top();
        target2 = other.bottom();
      }

      if (isClose(proposedValue, target1)) {
        outSnapped = true;
        return target1;
      }

      if (isClose(proposedValue, target2)) {
        outSnapped = true;
        return target2;
      }
    }
  }
  return proposedValue;
}

QPointF SnappingUtils::snapPosition(LayoutScene* scene,
                                    QGraphicsItem* selfItem,
                                    const QPointF& proposedPos,
                                    const QRectF& logicalRect,
                                    QList<QLineF>* outGuides) {
  if (!scene) {
    return proposedPos;
  }

  if (outGuides) {
    outGuides->clear();
  }

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

  auto addVGuide = [&](double x) {
    if (outGuides) {
      outGuides->append(QLineF(x, -10000, x, 10000));
    }
  };
  auto addHGuide = [&](double y) {
    if (outGuides) {
      outGuides->append(QLineF(-10000, y, 10000, y));
    }
  };

  // 1. Guide Lines
  for (QGraphicsItem* item : scene->items()) {
    if (item->type() == Constants::Item::GuideItem) {
      GuideLineItem* guide = static_cast<GuideLineItem*>(item);
      if (guide->orientation() == GuideLineItem::Vertical) {
        double guideX = guide->pos().x();
        if (isClose(visualLeft, guideX)) {
          visualLeft = guideX;
          snappedX = true;
          addVGuide(guideX);
        } else if (isClose(visualRight, guideX)) {
          visualLeft = guideX - width;
          snappedX = true;
          addVGuide(guideX);
        }
      } else {
        double guideY = guide->pos().y();
        if (isClose(visualTop, guideY)) {
          visualTop = guideY;
          snappedY = true;
          addHGuide(guideY);
        } else if (isClose(visualBottom, guideY)) {
          visualTop = guideY - height;
          snappedY = true;
          addHGuide(guideY);
        }
      }
    }
  }

  // 2. Scene Bounds
  if (!snappedX) {
    if (isClose(visualLeft, validArea.left())) {
      visualLeft = validArea.left();
      snappedX = true;
      addVGuide(validArea.left());
    } else if (isClose(visualRight, validArea.right())) {
      visualLeft = validArea.right() - width;
      snappedX = true;
      addVGuide(validArea.right());
    }
  }
  if (!snappedY) {
    if (isClose(visualTop, validArea.top())) {
      visualTop = validArea.top();
      snappedY = true;
      addHGuide(validArea.top());
    } else if (isClose(visualBottom, validArea.bottom())) {
      visualTop = validArea.bottom() - height;
      snappedY = true;
      addHGuide(validArea.bottom());
    }
  }

  // 3. Other Items
  if (!snappedX || !snappedY) {
    QList<QGraphicsItem*> nearbyItems = getSnappingCandidates(scene, visualRect, selfItem);

    for (QGraphicsItem* item : nearbyItems) {
      QRectF otherRect = item->sceneBoundingRect();

      // Snap X
      if (!snappedX && rangesOverlap(visualTop, height, otherRect.top(), otherRect.height())) {
        if (isClose(visualLeft, otherRect.right())) {
          visualLeft = otherRect.right();
          snappedX = true;
          addVGuide(otherRect.right());
        } else if (isClose(visualLeft, otherRect.left())) {
          visualLeft = otherRect.left();
          snappedX = true;
          addVGuide(otherRect.left());
        } else if (isClose(visualRight, otherRect.left())) {
          visualLeft = otherRect.left() - width;
          snappedX = true;
          addVGuide(otherRect.left());
        } else if (isClose(visualRight, otherRect.right())) {
          visualLeft = otherRect.right() - width;
          snappedX = true;
          addVGuide(otherRect.right());
        }
      }

      // Snap Y
      if (!snappedY && rangesOverlap(visualLeft, width, otherRect.left(), otherRect.width())) {
        if (isClose(visualTop, otherRect.bottom())) {
          visualTop = otherRect.bottom();
          snappedY = true;
          addHGuide(otherRect.bottom());
        } else if (isClose(visualTop, otherRect.top())) {
          visualTop = otherRect.top();
          snappedY = true;
          addHGuide(otherRect.top());
        } else if (isClose(visualBottom, otherRect.top())) {
          visualTop = otherRect.top() - height;
          snappedY = true;
          addHGuide(otherRect.top());
        } else if (isClose(visualBottom, otherRect.bottom())) {
          visualTop = otherRect.bottom() - height;
          snappedY = true;
          addHGuide(otherRect.bottom());
        }
      }
    }
  }

  // 4. Grid
  if (scene->isGridEnabled()) {
    int gs = scene->gridSize();
    if (!snappedX) {
      visualLeft = snapToGridVal(visualLeft, gs);
    }
    if (!snappedY) {
      visualTop = snapToGridVal(visualTop, gs);
    }
  }

  // 5. Clamping
  if (visualLeft < validArea.left()) {
    visualLeft = validArea.left();
  }
  if (visualLeft + width > validArea.right()) {
    visualLeft = validArea.right() - width;
  }
  if (visualTop < validArea.top()) {
    visualTop = validArea.top();
  }
  if (visualTop + height > validArea.bottom()) {
    visualTop = validArea.bottom() - height;
  }

  return QPointF(visualLeft - contentOffset.x(), visualTop - contentOffset.y());
}
