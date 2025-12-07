#ifndef RESIZABLEAPPITEM_H
#define RESIZABLEAPPITEM_H

#include <QGraphicsRectItem>

#include "constants.h"

class QGraphicsTextItem;

class ResizableAppItem : public QGraphicsRectItem {
public:
  enum ResizeHandle { None = 0, Left = 1, Top = 2, Right = 4, Bottom = 8 };

  ResizableAppItem(const QString& appName, const QRectF& rect);
  QString name() const;
  int type() const override { return Constants::Item::Types::AppItem; }

  void setLocked(bool locked);
  bool isLocked() const;

  void updateStatusText();

  void setFontScale(qreal scale);
  void setBaseFontSize(int size);

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

  QGraphicsTextItem* m_pTitleText;
  QGraphicsTextItem* m_pStatusText;
  bool m_locked;

  int m_baseFontSize;
  qreal m_currentScale;
};

#endif  // RESIZABLEAPPITEM_H
