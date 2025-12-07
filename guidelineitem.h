#ifndef GUIDELINEITEM_H
#define GUIDELINEITEM_H

#include <QGraphicsLineItem>
#include <QPen>
#include <QPointF>

class GuideLineItem : public QGraphicsLineItem {
public:
  enum Orientation { Horizontal, Vertical };

  GuideLineItem(Orientation orientation, qreal pos, qreal length = 10000);

  int type() const override { return Type; }
  enum { Type = UserType + 10 };

  Orientation orientation() const { return m_orientation; }

  QRectF boundingRect() const override;

protected:
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
  Orientation m_orientation;
};

#endif  // GUIDELINEITEM_H
