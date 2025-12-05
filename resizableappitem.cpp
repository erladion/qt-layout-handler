#include "resizableappitem.h"
#include <QAction>
#include <QBrush>
#include <QCursor>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <cmath>
#include "guidelineitem.h"  // NEW: Include for snapping
#include "layoutscene.h"
#include "zoneitem.h"

ResizableAppItem::ResizableAppItem(const QString& appName, const QRectF& rect)
    : QGraphicsRectItem(rect), m_resizeHandle(None), m_name(appName), m_locked(false) {
  // Default Flags
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);

  setAcceptHoverEvents(true);
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  setBrush(QBrush(QColor(60, 60, 60, 200)));
  setPen(QPen(Qt::black, 1));

  // App Name Label (Top Left)
  QGraphicsTextItem* text = new QGraphicsTextItem(appName, this);
  text->setDefaultTextColor(Qt::lightGray);
  text->setPos(5, 5);

  // Status Label (Bottom Left, Smaller font)
  m_statusText = new QGraphicsTextItem("", this);
  m_statusText->setDefaultTextColor(QColor(200, 200, 200));
  QFont font = m_statusText->font();
  font.setPointSize(8);
  m_statusText->setFont(font);

  updateStatusText();  // Initial update
}

QString ResizableAppItem::name() const {
  return m_name;
}

void ResizableAppItem::setLocked(bool locked) {
  m_locked = locked;
  // If locked, disable movement.
  // We keep Selectable so the user can select it to Unlock it.
  setFlag(QGraphicsItem::ItemIsMovable, !m_locked);

  // Visual feedback update
  update();
}

bool ResizableAppItem::isLocked() const {
  return m_locked;
}

void ResizableAppItem::updateStatusText() {
  // Format: X, Y (WxH)
  QString status = QString("%1, %2 (%3x%4)").arg((int)pos().x()).arg((int)pos().y()).arg((int)rect().width()).arg((int)rect().height());

  if (m_locked) {
    status += " [LOCKED]";
  }

  m_statusText->setPlainText(status);

  // Position it at the bottom-left of the rect
  m_statusText->setPos(5, rect().height() - 20);
}

double ResizableAppItem::snapToGridVal(double val, int gridSize) {
  return std::round(val / gridSize) * gridSize;
}

bool ResizableAppItem::rangesOverlap(double min1, double len1, double min2, double len2) {
  double max1 = min1 + len1;
  double max2 = min2 + len2;
  return max1 > min2 && min1 < max2;
}

void ResizableAppItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
  QMenu menu;

  // Feature 7: Lock/Unlock Action
  QAction* lockAction = menu.addAction(m_locked ? "Unlock" : "Lock");
  menu.addSeparator();

  QAction* raiseAction = menu.addAction("Bring to Front");
  QAction* lowerAction = menu.addAction("Send to Back");
  menu.addSeparator();
  QAction* removeAction = menu.addAction("Remove");

  QAction* selectedAction = menu.exec(event->screenPos());

  if (selectedAction == lockAction) {
    setLocked(!m_locked);
  } else if (selectedAction == raiseAction)
    setZValue(100);
  else if (selectedAction == lowerAction)
    setZValue(-1);
  else if (selectedAction == removeAction) {
    if (scene()) {
      scene()->removeItem(this);
      delete this;
    }
  }
}

void ResizableAppItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  QGraphicsRectItem::paint(painter, option, widget);

  // Feature 7: Visual Indicator for Locked state
  if (m_locked) {
    painter->save();
    QPen p(Qt::red, 2, Qt::DashLine);
    painter->setPen(p);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect());

    // Draw a small lock icon or text in top right
    painter->setPen(Qt::red);
    painter->drawText(rect().adjusted(0, 5, -5, 0), Qt::AlignTop | Qt::AlignRight, "🔒");
    painter->restore();
  }
}

QVariant ResizableAppItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemSelectedHasChanged) {
    if (value.toBool()) {
      // If locked, show distinct selection color (e.g., Orange instead of Blue)
      QColor selColor = m_locked ? QColor(255, 100, 100) : QColor(0, 120, 215);
      setPen(QPen(selColor, 3));
      if (!m_locked)
        setZValue(100);  // Don't auto-raise if locked
    } else {
      setPen(QPen(Qt::black, 1));
      if (!m_locked)
        setZValue(0);
    }
  }

  // POSITION CHANGE
  // If locked, we shouldn't be here because Movable flag is off, but safety check:
  if (change == ItemPositionChange && m_locked) {
    return pos();  // Reject change
  }

  // Feature 6: Grouping Support
  // If this item is part of a Group (has a parent item), we DISABLE the custom snapping logic.
  if (parentItem() != nullptr) {
    return QGraphicsRectItem::itemChange(change, value);
  }

  if (change == ItemPositionChange && scene()) {
    QPointF newPos = value.toPointF();
    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    QRectF validArea = layoutScene ? layoutScene->getWorkingArea() : scene()->sceneRect();
    QRectF myRect = rect();

    double desiredLeft = newPos.x();
    double desiredTop = newPos.y();
    double desiredRight = desiredLeft + myRect.width();
    double desiredBottom = desiredTop + myRect.height();

    bool snappedX = false;
    bool snappedY = false;

    // --- 0. NEW: Snap to Guide Lines (Priority) ---
    for (QGraphicsItem* item : scene()->items()) {
      if (GuideLineItem* guide = dynamic_cast<GuideLineItem*>(item)) {
        if (guide->orientation() == GuideLineItem::Vertical) {
          // Snap Left/Right edges to Vertical Line
          double guideX = guide->pos().x();
          if (qAbs(desiredLeft - guideX) < SNAP_DIST) {
            desiredLeft = guideX;
            snappedX = true;
          } else if (qAbs(desiredRight - guideX) < SNAP_DIST) {
            desiredLeft = guideX - myRect.width();
            snappedX = true;
          }
        } else {
          // Snap Top/Bottom edges to Horizontal Line
          double guideY = guide->pos().y();
          if (qAbs(desiredTop - guideY) < SNAP_DIST) {
            desiredTop = guideY;
            snappedY = true;
          } else if (qAbs(desiredBottom - guideY) < SNAP_DIST) {
            desiredTop = guideY - myRect.height();
            snappedY = true;
          }
        }
      }
    }

    if (qAbs(desiredLeft - validArea.left()) < SNAP_DIST) {
      desiredLeft = validArea.left();
      snappedX = true;
    } else if (qAbs(desiredRight - validArea.right()) < SNAP_DIST) {
      desiredLeft = validArea.right() - myRect.width();
      snappedX = true;
    }

    if (qAbs(desiredTop - validArea.top()) < SNAP_DIST) {
      desiredTop = validArea.top();
      snappedY = true;
    } else if (qAbs(desiredBottom - validArea.bottom()) < SNAP_DIST) {
      desiredTop = validArea.bottom() - myRect.height();
      snappedY = true;
    }

    if (!snappedX || !snappedY) {
      for (QGraphicsItem* item : scene()->items()) {
        if (item == this)
          continue;
        // Ignore parent group if checking collisions (though parent isn't ResizableAppItem usually)
        if (item == parentItem())
          continue;

        ResizableAppItem* otherItem = dynamic_cast<ResizableAppItem*>(item);
        if (!otherItem)
          continue;

        QRectF other = otherItem->mapRectToScene(otherItem->rect());

        if (!snappedX && rangesOverlap(desiredTop, myRect.height(), other.top(), other.height())) {
          if (qAbs(desiredLeft - other.right()) < SNAP_DIST) {
            desiredLeft = other.right();
            snappedX = true;
          } else if (qAbs(desiredLeft - other.left()) < SNAP_DIST) {
            desiredLeft = other.left();
            snappedX = true;
          } else if (qAbs(desiredRight - other.left()) < SNAP_DIST) {
            desiredLeft = other.left() - myRect.width();
            snappedX = true;
          } else if (qAbs(desiredRight - other.right()) < SNAP_DIST) {
            desiredLeft = other.right() - myRect.width();
            snappedX = true;
          }
        }

        if (!snappedY && rangesOverlap(desiredLeft, myRect.width(), other.left(), other.width())) {
          if (qAbs(desiredTop - other.bottom()) < SNAP_DIST) {
            desiredTop = other.bottom();
            snappedY = true;
          } else if (qAbs(desiredTop - other.top()) < SNAP_DIST) {
            desiredTop = other.top();
            snappedY = true;
          } else if (qAbs(desiredBottom - other.top()) < SNAP_DIST) {
            desiredTop = other.top() - myRect.height();
            snappedY = true;
          } else if (qAbs(desiredBottom - other.bottom()) < SNAP_DIST) {
            desiredTop = other.bottom() - myRect.height();
            snappedY = true;
          }
        }
      }
    }

    if (layoutScene && layoutScene->isGridEnabled()) {
      int gs = layoutScene->gridSize();
      if (!snappedX)
        desiredLeft = snapToGridVal(desiredLeft, gs);
      if (!snappedY)
        desiredTop = snapToGridVal(desiredTop, gs);
    }

    if (desiredLeft < validArea.left())
      desiredLeft = validArea.left();
    if (desiredLeft + myRect.width() > validArea.right())
      desiredLeft = validArea.right() - myRect.width();

    if (desiredTop < validArea.top())
      desiredTop = validArea.top();
    if (desiredTop + myRect.height() > validArea.bottom())
      desiredTop = validArea.bottom() - myRect.height();

    return QPointF(desiredLeft, desiredTop);
  }
  return QGraphicsRectItem::itemChange(change, value);
}

void ResizableAppItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
  // Feature 7: If locked, no resize cursor
  if (m_locked) {
    setCursor(Qt::ArrowCursor);
    return;
  }

  int handle = getHandleAt(event->pos());
  QCursor cursor = Qt::ArrowCursor;
  if (handle == (Right | Bottom))
    cursor = Qt::SizeFDiagCursor;
  else if (handle & Right)
    cursor = Qt::SizeHorCursor;
  else if (handle & Bottom)
    cursor = Qt::SizeVerCursor;
  setCursor(cursor);
  QGraphicsRectItem::hoverMoveEvent(event);
}

void ResizableAppItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  // Feature 7: If locked, ignore resize clicks
  if (m_locked) {
    QGraphicsRectItem::mousePressEvent(event);  // Pass through for selection
    return;
  }

  m_resizeHandle = getHandleAt(event->pos());
  if (m_resizeHandle == None) {
    QGraphicsRectItem::mousePressEvent(event);
  }
}

void ResizableAppItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsRectItem::mouseReleaseEvent(event);

  if (scene() && m_resizeHandle == None && !m_locked && parentItem() == nullptr) {
    QList<QGraphicsItem*> itemsUnderMouse = scene()->items(event->scenePos());

    for (QGraphicsItem* item : itemsUnderMouse) {
      ZoneItem* zone = dynamic_cast<ZoneItem*>(item);
      if (zone) {
        setRect(zone->rect());
        setPos(zone->pos());
        break;
      }
    }
  }

  updateStatusText();
}

void ResizableAppItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  if (m_locked)
    return;  // Should not happen due to flags, but safety first

  if (m_resizeHandle == None) {
    QGraphicsRectItem::mouseMoveEvent(event);
    updateStatusText();
    return;
  }

  if (m_resizeHandle != None) {
    QPointF mouseScenePos = event->scenePos();
    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    QRectF validArea = layoutScene ? layoutScene->getWorkingArea() : scene()->sceneRect();

    double currentX = pos().x();
    double currentY = pos().y();

    // Grouping Adjustment: If we are in a group, pos() is local.
    // Converting mouseScenePos (Scene) to Local logic is needed for accurate resizing inside groups.
    // However, for V1, we simply assume Resizing is done BEFORE grouping.
    if (parentItem()) {
      double newW = event->pos().x();
      double newH = event->pos().y();
      if (newW < 50)
        newW = 50;
      if (newH < 50)
        newH = 50;
      QRectF newRect = rect();
      newRect.setWidth(newW);
      newRect.setHeight(newH);
      setRect(newRect);
      return;
    }

    double proposedRight = mouseScenePos.x();
    double proposedBottom = mouseScenePos.y();

    bool snappedW = false;
    bool snappedH = false;

    if (m_resizeHandle & Right) {
      for (QGraphicsItem* item : scene()->items()) {
        if (item == this)
          continue;
        ResizableAppItem* otherItem = dynamic_cast<ResizableAppItem*>(item);
        if (!otherItem)
          continue;
        QRectF other = otherItem->mapRectToScene(otherItem->rect());

        if (rangesOverlap(currentY, rect().height(), other.top(), other.height())) {
          if (qAbs(proposedRight - other.left()) < SNAP_DIST) {
            proposedRight = other.left();
            snappedW = true;
            break;
          }
          if (qAbs(proposedRight - other.right()) < SNAP_DIST) {
            proposedRight = other.right();
            snappedW = true;
            break;
          }
        }
      }
      if (!snappedW && qAbs(proposedRight - validArea.right()) < SNAP_DIST) {
        proposedRight = validArea.right();
        snappedW = true;
      }
    }

    if (m_resizeHandle & Bottom) {
      for (QGraphicsItem* item : scene()->items()) {
        if (item == this)
          continue;
        ResizableAppItem* otherItem = dynamic_cast<ResizableAppItem*>(item);
        if (!otherItem)
          continue;
        QRectF other = otherItem->mapRectToScene(otherItem->rect());

        if (rangesOverlap(currentX, rect().width(), other.left(), other.width())) {
          if (qAbs(proposedBottom - other.top()) < SNAP_DIST) {
            proposedBottom = other.top();
            snappedH = true;
            break;
          }
          if (qAbs(proposedBottom - other.bottom()) < SNAP_DIST) {
            proposedBottom = other.bottom();
            snappedH = true;
            break;
          }
        }
      }
      if (!snappedH && qAbs(proposedBottom - validArea.bottom()) < SNAP_DIST) {
        proposedBottom = validArea.bottom();
        snappedH = true;
      }
    }

    if (layoutScene && layoutScene->isGridEnabled()) {
      int gs = layoutScene->gridSize();
      if (!snappedW && (m_resizeHandle & Right))
        proposedRight = snapToGridVal(proposedRight, gs);
      if (!snappedH && (m_resizeHandle & Bottom))
        proposedBottom = snapToGridVal(proposedBottom, gs);
    }

    double newW = proposedRight - currentX;
    double newH = proposedBottom - currentY;

    if (currentX + newW > validArea.right())
      newW = validArea.right() - currentX;
    if (currentY + newH > validArea.bottom())
      newH = validArea.bottom() - currentY;

    if (newW < 50)
      newW = 50;
    if (newH < 50)
      newH = 50;

    QRectF newRect = rect();
    newRect.setWidth(newW);
    newRect.setHeight(newH);
    setRect(newRect);

    // Update label during resize
    updateStatusText();

  } else {
    QGraphicsRectItem::mouseMoveEvent(event);
  }
}

int ResizableAppItem::getHandleAt(const QPointF& pt) {
  QRectF r = rect();
  int handle = None;
  qreal margin = 15;
  if (qAbs(pt.x() - r.right()) < margin)
    handle |= Right;
  if (qAbs(pt.y() - r.bottom()) < margin)
    handle |= Bottom;
  return handle;
}
