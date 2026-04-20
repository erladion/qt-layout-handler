#ifndef LASERPOINTERITEM_H
#define LASERPOINTERITEM_H

#include <QGraphicsObject>
#include <QList>
#include <QPointF>
#include <QTimer>

class LaserPointerItem : public QGraphicsObject {
  Q_OBJECT
public:
  explicit LaserPointerItem(QGraphicsItem* parent = nullptr);

  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  void updatePosition(const QPointF& pos);

  void setColor(const QColor& color);
  void setSize(qreal size);

private slots:
  void fadeTrail();

private:
  struct TrailPoint {
    QPointF pos;
    int age;
  };

  QPointF m_headPos;
  QList<TrailPoint> m_trail;
  QTimer m_timer;

  QColor m_color = Qt::red;
  qreal m_size = 10.0;  // Radius of the laser
};

#endif  // LASERPOINTERITEM_H
