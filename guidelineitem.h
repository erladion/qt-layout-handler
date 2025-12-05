#ifndef GUIDELINEITEM_H
#define GUIDELINEITEM_H

#include <QGraphicsLineItem>
#include <QPen>

class GuideLineItem : public QGraphicsLineItem {
public:
  enum Orientation { Horizontal, Vertical };

  GuideLineItem(Orientation orientation, qreal pos, qreal length = 10000);

  int type() const override { return Type; }
  enum { Type = UserType + 10 };  // Custom Type ID for snapping checks

  Orientation orientation() const { return m_orientation; }

protected:
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  // update line length on change if needed, though usually fixed long length is fine
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
  Orientation m_orientation;
};

#endif  // GUIDELINEITEM_H
