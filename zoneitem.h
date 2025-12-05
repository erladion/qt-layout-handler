#ifndef ZONEITEM_H
#define ZONEITEM_H

#include <QGraphicsRectItem>

class ZoneItem : public QGraphicsRectItem {
public:
  enum ResizeHandle { None = 0, Left = 1, Top = 2, Right = 4, Bottom = 8 };
  ZoneItem(const QRectF& rect);

protected:
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

  // NEW: Handle position constraints
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
  int getHandleAt(const QPointF& pt);
  int m_resizeHandle;
};

#endif  // ZONEITEM_H
