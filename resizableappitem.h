#ifndef RESIZABLEAPPITEM_H
#define RESIZABLEAPPITEM_H

#include <QGraphicsRectItem>

class QGraphicsTextItem;  // Forward declaration

class ResizableAppItem : public QGraphicsRectItem {
public:
  enum ResizeHandle { None = 0, Left = 1, Top = 2, Right = 4, Bottom = 8 };

  ResizableAppItem(const QString& appName, const QRectF& rect);
  QString name() const;

  // Feature 7: Locking
  void setLocked(bool locked);
  bool isLocked() const;

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;  // Custom paint for lock visual

  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
  double snapToGridVal(double val, int gridSize);
  bool rangesOverlap(double min1, double len1, double min2, double len2);
  int getHandleAt(const QPointF& pt);

  // Updates the coordinate/size label
  void updateStatusText();

  int m_resizeHandle;
  QString m_name;
  const double SNAP_DIST = 15.0;

  // NEW: Text item for stats
  QGraphicsTextItem* m_statusText;

  // Feature 7: Lock State
  bool m_locked;
};

#endif  // RESIZABLEAPPITEM_H
