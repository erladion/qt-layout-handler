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
#include "layoutscene.h"
#include "snappingutils.h"
#include "zoneitem.h"

// REMOVED local constant, now using SnappingUtils::isClose / SNAP_DIST

ResizableAppItem::ResizableAppItem(const QString& appName, const QRectF& rect)
    : QGraphicsRectItem(rect), m_resizeHandle(None), m_name(appName), m_locked(false) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  setBrush(QBrush(QColor(60, 60, 60, 200)));
  setPen(QPen(Qt::black, 1));

  QGraphicsTextItem* text = new QGraphicsTextItem(appName, this);
  text->setDefaultTextColor(Qt::lightGray);
  text->setPos(5, 5);

  m_statusText = new QGraphicsTextItem("", this);
  m_statusText->setDefaultTextColor(QColor(200, 200, 200));
  QFont font = m_statusText->font();
  font.setPointSize(8);
  m_statusText->setFont(font);

  updateStatusText();
}

QString ResizableAppItem::name() const {
  return m_name;
}

void ResizableAppItem::setLocked(bool locked) {
  m_locked = locked;
  setFlag(QGraphicsItem::ItemIsMovable, !m_locked);
  update();
}

bool ResizableAppItem::isLocked() const {
  return m_locked;
}

void ResizableAppItem::updateStatusText() {
  QString status = QString("%1, %2 (%3x%4)").arg((int)pos().x()).arg((int)pos().y()).arg((int)rect().width()).arg((int)rect().height());
  if (m_locked)
    status += " [LOCKED]";
  m_statusText->setPlainText(status);
  m_statusText->setPos(5, rect().height() - 20);
}

void ResizableAppItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
  QMenu menu;
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
  if (m_locked) {
    painter->save();
    QPen p(Qt::red, 2, Qt::DashLine);
    painter->setPen(p);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect());
    painter->setPen(Qt::red);
    painter->drawText(rect().adjusted(0, 5, -5, 0), Qt::AlignTop | Qt::AlignRight, "🔒");
    painter->restore();
  }
}

QVariant ResizableAppItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemSelectedHasChanged) {
    if (value.toBool()) {
      QColor selColor = m_locked ? QColor(255, 100, 100) : QColor(0, 120, 215);
      setPen(QPen(selColor, 3));
      if (!m_locked)
        setZValue(100);
    } else {
      setPen(QPen(Qt::black, 1));
      if (!m_locked)
        setZValue(0);
    }
  }

  if (change == ItemPositionChange && m_locked) {
    return pos();
  }

  // Disable internal snapping if grouped (parent handles it)
  if (parentItem() != nullptr) {
    return QGraphicsRectItem::itemChange(change, value);
  }

  if (change == ItemPositionChange && scene()) {
    LayoutScene* layoutScene = dynamic_cast<LayoutScene*>(scene());
    if (layoutScene) {
      return SnappingUtils::snapPosition(layoutScene, this, value.toPointF(), rect());
    }
  }
  return QGraphicsRectItem::itemChange(change, value);
}

void ResizableAppItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
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
  if (m_locked) {
    QGraphicsRectItem::mousePressEvent(event);
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
    return;

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

    if (parentItem()) {
      double newW = event->pos().x();
      double newH = event->pos().y();
      if (newW < 50)
        newW = 50;
      if (newH < 50)
        newH = 50;
      setRect(0, 0, newW, newH);
      return;
    }

    double proposedRight = mouseScenePos.x();
    double proposedBottom = mouseScenePos.y();

    bool snappedW = false;
    bool snappedH = false;

    // Use SnappingUtils helper
    if (m_resizeHandle & Right) {
      for (QGraphicsItem* item : scene()->items()) {
        if (item == this)
          continue;
        ResizableAppItem* otherItem = dynamic_cast<ResizableAppItem*>(item);
        if (!otherItem)
          continue;
        QRectF other = otherItem->mapRectToScene(otherItem->rect());

        if (SnappingUtils::rangesOverlap(currentY, rect().height(), other.top(), other.height())) {
          if (SnappingUtils::isClose(proposedRight, other.left())) {
            proposedRight = other.left();
            snappedW = true;
            break;
          }
          if (SnappingUtils::isClose(proposedRight, other.right())) {
            proposedRight = other.right();
            snappedW = true;
            break;
          }
        }
      }
      if (!snappedW && SnappingUtils::isClose(proposedRight, validArea.right())) {
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

        if (SnappingUtils::rangesOverlap(currentX, rect().width(), other.left(), other.width())) {
          if (SnappingUtils::isClose(proposedBottom, other.top())) {
            proposedBottom = other.top();
            snappedH = true;
            break;
          }
          if (SnappingUtils::isClose(proposedBottom, other.bottom())) {
            proposedBottom = other.bottom();
            snappedH = true;
            break;
          }
        }
      }
      if (!snappedH && SnappingUtils::isClose(proposedBottom, validArea.bottom())) {
        proposedBottom = validArea.bottom();
        snappedH = true;
      }
    }

    if (layoutScene && layoutScene->isGridEnabled()) {
      int gs = layoutScene->gridSize();
      if (!snappedW && (m_resizeHandle & Right))
        proposedRight = SnappingUtils::snapToGridVal(proposedRight, gs);
      if (!snappedH && (m_resizeHandle & Bottom))
        proposedBottom = SnappingUtils::snapToGridVal(proposedBottom, gs);
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

    setRect(0, 0, newW, newH);  // Assuming rect starts at 0,0
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
