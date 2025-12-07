#include "zoneitem.h"
#include <QCursor>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QPainter>
#include "constants.h"
#include "layoutscene.h"
#include "snappingutils.h"

ZoneItem::ZoneItem(const QRectF& rect, QGraphicsItem* parent) : QGraphicsRectItem(rect, parent), m_resizeHandle(None) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);

  // FIX: Use fromRgba for transparency
  setBrush(QBrush(QColor::fromRgba(Constants::Color::ZoneItemFill)));
  setPen(QPen(QColor::fromRgba(Constants::Color::ZoneItemBorder), 2, Qt::DashLine));

  m_label = new QGraphicsTextItem("Zone", this);
  m_label->setDefaultTextColor(QColor::fromRgba(Constants::Color::ZoneItemText));
  QFont f = m_label->font();
  f.setBold(true);
  f.setPointSize(12);
  m_label->setFont(f);

  updateLabel();
}

void ZoneItem::updateLabel() {
  QRectF r = rect();
  QRectF tr = m_label->boundingRect();
  m_label->setPos(r.center().x() - tr.width() / 2, r.center().y() - tr.height() / 2);
}

void ZoneItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->setPen(pen());
  painter->setBrush(brush());
  painter->drawRect(rect());

  if (isSelected()) {
    painter->setPen(QPen(QColor::fromRgba(Constants::Color::SelectionHighlight), 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect());
  }
}

QVariant ZoneItem::itemChange(GraphicsItemChange change, const QVariant& value) {
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

void ZoneItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
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

void ZoneItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  m_resizeHandle = getHandleAt(event->pos());
  if (m_resizeHandle == None) {
    QGraphicsRectItem::mousePressEvent(event);
  }
}

void ZoneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsRectItem::mouseReleaseEvent(event);
  if (scene()) {
    LayoutScene* ls = dynamic_cast<LayoutScene*>(scene());
    if (ls)
      ls->clearSnapGuides();
  }
}

void ZoneItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
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

    QRectF queryRect(currentX, currentY, proposedRight - currentX, proposedBottom - currentY);
    QList<QGraphicsItem*> candidates = SnappingUtils::getSnappingCandidates(layoutScene, queryRect, this);

    if (m_resizeHandle & Right) {
      proposedRight = SnappingUtils::snapValueToCandidates(proposedRight, currentY, rect().height(), candidates, Qt::Horizontal, snappedW);
      if (!snappedW && SnappingUtils::isClose(proposedRight, validArea.right()))
        proposedRight = validArea.right();
    }

    if (m_resizeHandle & Bottom) {
      proposedBottom = SnappingUtils::snapValueToCandidates(proposedBottom, currentX, rect().width(), candidates, Qt::Vertical, snappedH);
      if (!snappedH && SnappingUtils::isClose(proposedBottom, validArea.bottom()))
        proposedBottom = validArea.bottom();
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

    if (newW < 50)
      newW = 50;
    if (newH < 50)
      newH = 50;

    setRect(0, 0, newW, newH);
    updateLabel();
  } else {
    QGraphicsRectItem::mouseMoveEvent(event);
  }
}

int ZoneItem::getHandleAt(const QPointF& pt) {
  QRectF r = rect();
  int handle = None;
  qreal margin = 15;
  if (qAbs(pt.x() - r.right()) < margin)
    handle |= Right;
  if (qAbs(pt.y() - r.bottom()) < margin)
    handle |= Bottom;
  return handle;
}
