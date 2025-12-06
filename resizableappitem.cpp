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
#include "settingsdialog.h"  // NEW: Read settings
#include "snappingutils.h"
#include "zoneitem.h"

ResizableAppItem::ResizableAppItem(const QString& appName, const QRectF& rect)
    : QGraphicsRectItem(rect), m_resizeHandle(None), m_name(appName), m_locked(false), m_currentScale(1.0) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  setBrush(QBrush(QColor(60, 60, 60, 200)));
  setPen(QPen(Qt::black, 1));

  // Initialize Base Font Size from Settings
  m_baseFontSize = SettingsDialog::getAppFontSize();

  // Title Text
  m_titleText = new QGraphicsTextItem(appName, this);
  m_titleText->setDefaultTextColor(Qt::lightGray);
  m_titleText->setPos(5, 5);

  // Status Text
  m_statusText = new QGraphicsTextItem("", this);
  m_statusText->setDefaultTextColor(QColor(200, 200, 200));

  // Apply Initial Fonts
  setFontScale(1.0);

  updateStatusText();
}

QString ResizableAppItem::name() const {
  return m_name;
}

// NEW: Helper to update base size and re-apply scale
void ResizableAppItem::setBaseFontSize(int size) {
  m_baseFontSize = size;
  setFontScale(m_currentScale);  // Re-apply with new base
}

void ResizableAppItem::setFontScale(qreal scale) {
  if (scale <= 0)
    return;
  m_currentScale = scale;

  // Scale Title (Base from settings)
  QFont titleFont = m_titleText->font();
  titleFont.setPointSizeF(static_cast<double>(m_baseFontSize) * scale);
  titleFont.setBold(true);
  m_titleText->setFont(titleFont);

  // Scale Status/Location Text (Base ~60% of title)
  QFont statusFont = m_statusText->font();
  statusFont.setPointSizeF((static_cast<double>(m_baseFontSize) * 0.6) * scale);
  m_statusText->setFont(statusFont);

  // Force re-calculation of text position
  updateStatusText();
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

  // Adjust position based on new font height
  qreal h = m_statusText->boundingRect().height();
  m_statusText->setPos(5, rect().height() - h);
}

// ... Context Menu ...
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

// ... Paint ...
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

// ... Item Change ...
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
    QRectF queryRect(currentX, currentY, proposedRight - currentX, proposedBottom - currentY);
    QList<QGraphicsItem*> candidates = SnappingUtils::getSnappingCandidates(layoutScene, queryRect, this);

    if (m_resizeHandle & Right) {
      for (QGraphicsItem* item : candidates) {
        QRectF other = item->mapRectToScene(item->boundingRect());

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
      for (QGraphicsItem* item : candidates) {
        QRectF other = item->mapRectToScene(item->boundingRect());

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

    setRect(0, 0, newW, newH);
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
