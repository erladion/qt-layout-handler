#include "resizableappitem.h"
#include <QAction>
#include <QBrush>
#include <QCursor>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QMenu>
#include <QPen>
#include <cmath>
#include "layoutscene.h"
#include "zoneitem.h"

ResizableAppItem::ResizableAppItem(const QString& appName, const QRectF& rect) : QGraphicsRectItem(rect), m_resizeHandle(None), m_name(appName) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);

  setAcceptHoverEvents(true);
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  setBrush(QBrush(QColor(60, 60, 60, 200)));
  setPen(QPen(Qt::black, 1));

  // App Name Label (Top Left)
  QGraphicsTextItem* text = new QGraphicsTextItem(appName, this);
  text->setDefaultTextColor(Qt::lightGray);
  text->setPos(5, 5);

  // NEW: Status Label (Bottom Left, Smaller font)
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

void ResizableAppItem::updateStatusText() {
  // Format: X, Y (WxH)
  QString status = QString("%1, %2 (%3x%4)").arg((int)pos().x()).arg((int)pos().y()).arg((int)rect().width()).arg((int)rect().height());

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
  QAction* raiseAction = menu.addAction("Bring to Front");
  QAction* lowerAction = menu.addAction("Send to Back");
  menu.addSeparator();
  QAction* removeAction = menu.addAction("Remove");

  QAction* selectedAction = menu.exec(event->screenPos());

  if (selectedAction == raiseAction)
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

QVariant ResizableAppItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemSelectedHasChanged) {
    if (value.toBool()) {
      setPen(QPen(QColor(0, 120, 215), 3));
      setZValue(100);
    } else {
      setPen(QPen(Qt::black, 1));
      setZValue(0);
    }
  }

  // POSITION CHANGE
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

    // Update text whenever position changes
    // We defer this slightly or just call updateStatusText() but position is not yet committed to 'pos()'
    // So we might need to manually format string here or rely on the fact that pos() updates after return.
    // Actually, we can just use the proposed newPos for the text.

    // However, itemChange is called BEFORE pos() is updated.
    // We will just let the text update in the next paint cycle or force it here using the new coords.
    // But since we can't easily set the text based on "future" pos easily in a helper,
    // we'll update it but we need to pass the new pos.

    // Simpler approach: updateStatusText uses current pos(), which is old.
    // Let's rely on the fact that scene updates happen frequently.
    // Or better: Since we return a new point, the item WILL move there.
    // We can schedule an update.

    return QPointF(desiredLeft, desiredTop);
  }
  return QGraphicsRectItem::itemChange(change, value);
}

void ResizableAppItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
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
  m_resizeHandle = getHandleAt(event->pos());
  if (m_resizeHandle == None) {
    QGraphicsRectItem::mousePressEvent(event);
  }
}

void ResizableAppItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsRectItem::mouseReleaseEvent(event);

  if (scene() && m_resizeHandle == None) {
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

  // Always update text on release to be sure
  updateStatusText();
}

void ResizableAppItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  // If moving (not resizing), the base implementation calls itemChange,
  // but itemChange runs before the pos is updated.
  // So we manually call updateStatusText() after the base Move logic.
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
