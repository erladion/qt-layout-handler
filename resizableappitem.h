#ifndef RESIZABLEAPPITEM_H
#define RESIZABLEAPPITEM_H

#include <QGraphicsRectItem>
#include <QObject>

#include "constants.h"

class QGraphicsTextItem;

class QAction;

class ResizableAppItem : public QObject, public QGraphicsRectItem {
  Q_OBJECT
public:
  enum ResizeHandle { None = 0, Left = 1, Top = 2, Right = 4, Bottom = 8 };

  ResizableAppItem(const QString& appName, const QRectF& rect);
  QString name() const;
  int type() const override { return Constants::Item::Types::AppItem; }

  void setLocked(bool locked);
  bool isLocked() const;

  virtual void updateStatusText();

  void setFontScale(qreal scale);
  void setBaseFontSize(int size);

  void initActions();  // NVI Setup
  void setAspectRatioEnabled(bool enabled) { m_aspectRatioEnabled = enabled; }
  void setTargetAspectRatio(double ratio) { m_targetAspectRatio = ratio; }

signals:
  void propertiesRequested(QGraphicsItem* item);

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

  virtual void setupCustomActions() {}  // NVI Hook

  QList<QAction*> m_contextActions;
  double m_targetAspectRatio = -1.0;
  bool m_aspectRatioEnabled = false;
  QGraphicsTextItem* m_pStatusText;

private:
  int getHandleAt(const QPointF& pt);

  int m_resizeHandle;
  QString m_name;

  QGraphicsTextItem* m_pTitleText;

  bool m_locked;

  int m_baseFontSize;
  qreal m_currentScale;
};

#endif  // RESIZABLEAPPITEM_H
