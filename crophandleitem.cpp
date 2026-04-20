#include "crophandleitem.h"
#include <QBrush>
#include <QPen>
#include "mirroredappitem.h"  // To cast parent and trigger updates

CropHandleItem::CropHandleItem(HandlePosition pos, QGraphicsItem* parent) : QGraphicsRectItem(-5, -5, 10, 10, parent), m_position(pos) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
  setBrush(QBrush(Qt::white));
  setPen(QPen(Qt::black, 1));

  if (pos == ApplyButton) {
    setRect(0, 0, 30, 30);        // Make the button larger
    setBrush(QBrush(Qt::green));  // Green for apply
  } else {
    // Set appropriate cursors for corners
    if (pos == TopLeft || pos == BottomRight)
      setCursor(Qt::SizeFDiagCursor);
    if (pos == TopRight || pos == BottomLeft)
      setCursor(Qt::SizeBDiagCursor);
  }
}

void CropHandleItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  if (m_position == ApplyButton) {
    // If they click the green box, apply the crop
    if (auto parentApp = dynamic_cast<MirroredAppItem*>(parentItem())) {
      event->accept();
      parentApp->applyInteractiveCrop();
      return;
    }
  }
  QGraphicsRectItem::mousePressEvent(event);
}

void CropHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsRectItem::mouseMoveEvent(event);
  if (m_position != ApplyButton) {
    // Notify the parent to redraw the overlay and adjust sibling handles
    if (auto parentApp = dynamic_cast<MirroredAppItem*>(parentItem())) {
      parentApp->updateCropHandles(this, m_position);
    }
  }
}

void CropHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsRectItem::mouseReleaseEvent(event);
}
