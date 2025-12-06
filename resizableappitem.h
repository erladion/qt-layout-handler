#ifndef RESIZABLEAPPITEM_H
#define RESIZABLEAPPITEM_H

#include <QGraphicsRectItem>

class QGraphicsTextItem;

class ResizableAppItem : public QGraphicsRectItem {
public:
  enum ResizeHandle { None = 0, Left = 1, Top = 2, Right = 4, Bottom = 8 };

  ResizableAppItem(const QString& appName, const QRectF& rect);
  QString name() const;

  void setLocked(bool locked);
  bool isLocked() const;

  void updateStatusText();

  // NEW: Scale fonts based on layout size
  void setFontScale(qreal scale);

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
  int getHandleAt(const QPointF& pt);

  int m_resizeHandle;
  QString m_name;

  // NEW: Track title text to update font
  QGraphicsTextItem* m_titleText;
  QGraphicsTextItem* m_statusText;
  bool m_locked;
};

#endif  // RESIZABLEAPPITEM_H
