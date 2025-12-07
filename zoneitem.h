#ifndef ZONEITEM_H
#define ZONEITEM_H

#include <QGraphicsRectItem>

#include "constants.h"

class QGraphicsTextItem;

class ZoneItem : public QGraphicsRectItem {
public:
  enum ResizeHandle { None = 0, Left = 1, Top = 2, Right = 4, Bottom = 8 };

  explicit ZoneItem(const QRectF& rect, QGraphicsItem* parent = nullptr);

  int type() const override { return Constants::Item::ZoneItem; }

protected:
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
  int getHandleAt(const QPointF& pt);
  void updateLabel();

  int m_resizeHandle;
  QGraphicsTextItem* m_pLabel;

  QPointF m_dragStartPos;
};

#endif  // ZONEITEM_H
