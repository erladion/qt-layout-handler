#ifndef CROPHANDLEITEM_H
#define CROPHANDLEITEM_H

#include <QCursor>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>

class CropHandleItem : public QGraphicsRectItem {
public:
  enum HandlePosition { TopLeft, TopRight, BottomLeft, BottomRight, ApplyButton };

  CropHandleItem(HandlePosition pos, QGraphicsItem* parent = nullptr);

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  HandlePosition m_position;
};

#endif  // CROPHANDLEITEM_H
