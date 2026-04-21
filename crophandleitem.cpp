#include "crophandleitem.h"

#include <QBrush>
#include <QPainter>
#include <QPen>

#include "mirroredappitem.h"  // To cast parent and trigger updates

CropHandleItem::CropHandleItem(HandlePosition pos, QGraphicsItem* parent) : QGraphicsRectItem(-5, -5, 10, 10, parent), m_position(pos) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
  setBrush(QBrush(Qt::white));
  setPen(QPen(Qt::black, 1));

  if (pos == ApplyButton) {
    // --> CHANGED: Make it wider (80px instead of 30px) to fit the text
    setRect(0, 0, 80, 30);
    setBrush(QBrush(QColor(0x4CAF50)));  // A nicer Material Design green
    setPen(Qt::NoPen);                    // Remove the black border for a cleaner look
    setCursor(Qt::PointingHandCursor);    // Add a hand cursor so it feels like a button
  } else {
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

void CropHandleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  // 1. Let the base class draw the background color (the green rectangle)
  QGraphicsRectItem::paint(painter, option, widget);

  // 2. Draw our custom UI on top if it is the Apply Button
  if (m_position == ApplyButton) {
    painter->setRenderHint(QPainter::Antialiasing);

    // -- Draw the Checkmark --
    QPen checkPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(checkPen);

    QPainterPath checkPath;
    // Draw a checkmark using coordinates relative to the button's top-left
    checkPath.moveTo(10, 15);
    checkPath.lineTo(16, 21);
    checkPath.lineTo(26, 9);
    painter->drawPath(checkPath);

    // -- Draw the Text --
    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setBold(true);
    font.setPixelSize(12);
    painter->setFont(font);

    // Create a bounding box for the text, shifting it right to avoid the checkmark
    QRectF textRect(32, 0, rect().width() - 32, rect().height());

    // Draw the text vertically centered
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, "Apply");
  }
}
