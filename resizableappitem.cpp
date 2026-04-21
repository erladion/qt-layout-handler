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

#include "constants.h"
#include "layoutscene.h"
#include "settingsdialog.h"
#include "snappingutils.h"
#include "zoneitem.h"

ResizableAppItem::ResizableAppItem(const QString& appName, const QRectF& rect)
    : QGraphicsRectItem(rect), m_resizeHandle(None), m_name(appName), m_locked(false), m_currentScale(1.0) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges |
           QGraphicsItem::ItemSendsScenePositionChanges);
  setAcceptHoverEvents(true);
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  setBrush(QBrush(QColor::fromRgba(Constants::Color::AppItemFill)));
  setPen(QPen(QColor::fromRgba(Constants::Color::AppItemBorder), 1));

  m_baseFontSize = SettingsDialog::getAppFontSize();

  m_pTitleText = new QGraphicsTextItem(appName, this);
  m_pTitleText->setDefaultTextColor(QColor::fromRgba(Constants::Color::AppItemText));
  m_pTitleText->setPos(5, 5);

  m_pStatusText = new QGraphicsTextItem("", this);
  m_pStatusText->setDefaultTextColor(QColor::fromRgba(Constants::Color::AppItemStatus));

  setFontScale(1.0);
  updateStatusText();
}

QString ResizableAppItem::name() const {
  return m_name;
}

void ResizableAppItem::setBaseFontSize(int size) {
  m_baseFontSize = size;
  setFontScale(m_currentScale);
}

void ResizableAppItem::setFontScale(qreal scale) {
  if (scale <= 0) {
    return;
  }
  m_currentScale = scale;
  QFont titleFont = m_pTitleText->font();
  titleFont.setPointSizeF(static_cast<double>(m_baseFontSize) * scale);
  titleFont.setBold(true);
  m_pTitleText->setFont(titleFont);
  QFont statusFont = m_pStatusText->font();
  statusFont.setPointSizeF((static_cast<double>(m_baseFontSize) * 0.6) * scale);
  m_pStatusText->setFont(statusFont);
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
  QPointF p = scenePos();
  QString status = QString("%1, %2 (%3x%4)").arg((int)p.x()).arg((int)p.y()).arg((int)rect().width()).arg((int)rect().height());

  if (m_locked) {
    status += " [LOCKED]";
  }
  m_pStatusText->setPlainText(status);
  qreal h = m_pStatusText->boundingRect().height();
  m_pStatusText->setPos(5, rect().height() - h);
}

void ResizableAppItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
  m_pContextMenu.popup(QCursor::pos());
}

void ResizableAppItem::setupCustomActions() {
  QAction* lockAction = m_pContextMenu.addAction(m_locked ? "Unlock" : "Lock");
  connect(lockAction, &QAction::triggered, this, [&]() { setLocked(!m_locked); });
  m_pContextMenu.addSeparator();
  QAction* raiseAction = m_pContextMenu.addAction("Bring to Front");
  connect(raiseAction, &QAction::triggered, this, [&]() { setZValue(100); });
  QAction* lowerAction = m_pContextMenu.addAction("Send to Back");
  connect(lowerAction, &QAction::triggered, this, [&]() { setZValue(-1); });
  m_pContextMenu.addSeparator();
  QAction* removeAction = m_pContextMenu.addAction("Remove");
  connect(removeAction, &QAction::triggered, this, [&]() {
    if (scene()) {
      scene()->removeItem(this);
      this->deleteLater();
    }
  });
}

void ResizableAppItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  QGraphicsRectItem::paint(painter, option, widget);
  if (m_locked) {
    painter->save();
    QPen p(QColor::fromRgba(Constants::Color::SelectionLocked), 2, Qt::DashLine);
    painter->setPen(p);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect());
    painter->setPen(QColor::fromRgba(Constants::Color::SelectionLocked));
    painter->drawText(rect().adjusted(0, 5, -5, 0), Qt::AlignTop | Qt::AlignRight, "🔒");
    painter->restore();
  }
}

QVariant ResizableAppItem::itemChange(GraphicsItemChange change, const QVariant& value) {
  if (change == ItemSelectedHasChanged) {
    if (value.toBool()) {
      QColor selColor = m_locked ? QColor::fromRgba(Constants::Color::SelectionLocked) : QColor::fromRgba(Constants::Color::SelectionHighlight);
      setPen(QPen(selColor, 3));
      if (!m_locked) {
        setZValue(100);
      }
    } else {
      setPen(QPen(Qt::black, 1));
      if (!m_locked) {
        setZValue(0);
      }
    }
  }

  if (change == ItemScenePositionHasChanged) {
    updateStatusText();
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
      QList<QLineF> guides;
      QPointF newPos = SnappingUtils::snapPosition(layoutScene, this, value.toPointF(), rect(), &guides);
      layoutScene->setSnapGuides(guides);
      return newPos;
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
  if (handle == (Right | Bottom)) {
    cursor = Qt::SizeFDiagCursor;
  } else if (handle & Right) {
    cursor = Qt::SizeHorCursor;
  } else if (handle & Bottom) {
    cursor = Qt::SizeVerCursor;
  }
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

  if (scene()) {
    LayoutScene* ls = dynamic_cast<LayoutScene*>(scene());
    if (ls) {
      ls->clearSnapGuides();
    }

    if (m_resizeHandle == None && !m_locked && parentItem() == nullptr) {
      QList<QGraphicsItem*> itemsUnderMouse = scene()->items(event->scenePos());
      for (QGraphicsItem* item : std::as_const(itemsUnderMouse)) {
        if (item->type() == Constants::Item::ZoneItem) {
          ZoneItem* zone = static_cast<ZoneItem*>(item);
          setRect(zone->rect());
          setPos(zone->pos());
          break;
        }
      }
    }
  }
  updateStatusText();
}

void ResizableAppItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  if (m_locked) {
    return;
  }

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
      if (newW < 50) {
        newW = 50;
      }
      if (newH < 50) {
        newH = 50;
      }
      setRect(0, 0, newW, newH);
      return;
    }

    double proposedRight = mouseScenePos.x();
    double proposedBottom = mouseScenePos.y();

    bool snappedW = false;
    bool snappedH = false;

    QRectF queryRect(currentX, currentY, proposedRight - currentX, proposedBottom - currentY);
    QList<QGraphicsItem*> candidates = SnappingUtils::getSnappingCandidates(layoutScene, queryRect, this);

    if (m_resizeHandle & Right) {
      for (QGraphicsItem* item : std::as_const(candidates)) {
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
      for (QGraphicsItem* item : std::as_const(candidates)) {
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
      if (!snappedW && (m_resizeHandle & Right)) {
        proposedRight = SnappingUtils::snapToGridVal(proposedRight, gs);
      }
      if (!snappedH && (m_resizeHandle & Bottom)) {
        proposedBottom = SnappingUtils::snapToGridVal(proposedBottom, gs);
      }
    }

    double newW = proposedRight - currentX;
    double newH = proposedBottom - currentY;

    if (m_aspectRatioEnabled && m_targetAspectRatio > 0) {
      if (m_resizeHandle == Right || m_resizeHandle == (Right | Bottom)) {
        newH = newW / m_targetAspectRatio;
      } else if (m_resizeHandle == Bottom) {
        newW = newH * m_targetAspectRatio;
      }
      if (newW < 50 || newH < 50) {
        if (m_targetAspectRatio >= 1.0) {
          newH = 50;
          newW = 50 * m_targetAspectRatio;
        } else {
          newW = 50;
          newH = 50 / m_targetAspectRatio;
        }
      }
      if (currentX + newW > validArea.right()) {
        newW = validArea.right() - currentX;
        newH = newW / m_targetAspectRatio;
      }
      if (currentY + newH > validArea.bottom()) {
        newH = validArea.bottom() - currentY;
        newW = newH * m_targetAspectRatio;
      }
    } else {
      // Standard unconstrained logic
      if (currentX + newW > validArea.right())
        newW = validArea.right() - currentX;
      if (currentY + newH > validArea.bottom())
        newH = validArea.bottom() - currentY;
      if (newW < 50)
        newW = 50;
      if (newH < 50)
        newH = 50;
    }
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
  if (qAbs(pt.x() - r.right()) < margin) {
    handle |= Right;
  }
  if (qAbs(pt.y() - r.bottom()) < margin) {
    handle |= Bottom;
  }
  return handle;
}

void ResizableAppItem::initActions() {
  QAction* propAction = new QAction("Properties", this);
  QAction* removeAction = new QAction("Remove", this);

  connect(propAction, &QAction::triggered, this, [this]() {
    setSelected(true);
    emit propertiesRequested(this);  // Signal based!
  });

  connect(removeAction, &QAction::triggered, this, [this]() {
    if (scene())
      scene()->removeItem(this);
    this->deleteLater();  // Safe Deletion
  });

  m_pContextMenu.addAction(propAction);
  m_pContextMenu.addAction(removeAction);

  QAction* sep = new QAction(this);
  sep->setSeparator(true);
  m_pContextMenu.addAction(sep);

  setupCustomActions();  // Let derived classes inject actions
}
