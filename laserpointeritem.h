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

private slots:
  void fadeTrail();

private:
  struct TrailPoint {
    QPointF pos;
    int age;
  };

  QPointF m_headPos;
  QList<TrailPoint> m_trail;
  QTimer* m_timer;
};

#endif // LASERPOINTERITEM_H
