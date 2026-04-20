#include "laserpointeritem.h"

#include <QLineF>
#include <QPainter>
#include <QRadialGradient>

LaserPointerItem::LaserPointerItem(QGraphicsItem* parent) : QGraphicsObject(parent) {
  setZValue(9999);

  // Run a 60 FPS timer to handle the fading animation smoothly
  connect(&m_timer, &QTimer::timeout, this, &LaserPointerItem::fadeTrail);
  m_timer.start(16);
}

void LaserPointerItem::updatePosition(const QPointF& pos) {
  // 1. WARNING TO QT: The bounding rect is about to change!
  // This forces Qt to clean up the OLD rectangle before we draw the NEW one.
  prepareGeometryChange();

  // 2. Now it is safe to update the variables
  if (m_trail.isEmpty() || QLineF(m_headPos, pos).length() > 2.0) {
    m_trail.prepend({m_headPos, 0});

    if (m_trail.size() > 15) {
      m_trail.removeLast();
    }
  }

  m_headPos = pos;
}

void LaserPointerItem::setColor(const QColor& color) {
  m_color = color;
  update();
}

void LaserPointerItem::setSize(qreal size) {
  prepareGeometryChange();  // CRITICAL: Tells Qt the bounding box is changing
  m_size = size;
  update();
}

void LaserPointerItem::fadeTrail() {
  if (!isVisible()) {
    if (!m_trail.isEmpty()) {
      prepareGeometryChange();
      m_trail.clear();
    }
    return;
  }

  bool geometryChanged = false;
  bool needsUpdate = false;

  for (int i = 0; i < m_trail.size(); ++i) {
    m_trail[i].age += 1;

    // If a point is about to die, the bounding box shrinks.
    if (m_trail[i].age > 15) {
      if (!geometryChanged) {
        prepareGeometryChange();  // Warn Qt!
        geometryChanged = true;
      }
      m_trail.removeAt(i);
      i--;  // Adjust index after removal
    } else {
      // Colors are fading, so we definitely need to redraw
      needsUpdate = true;
    }
  }

  // Only force a manual update if we just faded colors.
  // If geometryChanged is true, prepareGeometryChange() already requested an update.
  if (needsUpdate && !geometryChanged) {
    update();
  }
}

QRectF LaserPointerItem::boundingRect() const {
  if (!isVisible())
    return QRectF();

  qreal minX = m_headPos.x();
  qreal maxX = m_headPos.x();
  qreal minY = m_headPos.y();
  qreal maxY = m_headPos.y();

  for (const auto& pt : m_trail) {
    minX = qMin(minX, pt.pos.x());
    maxX = qMax(maxX, pt.pos.x());
    minY = qMin(minY, pt.pos.y());
    maxY = qMax(maxY, pt.pos.y());
  }

  // Add padding to account for the glow radius and line thickness
  return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).adjusted(-20, -20, 20, 20);
}

void LaserPointerItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  painter->setRenderHint(QPainter::Antialiasing);

  // 1. Draw the Fading Trail
  if (!m_trail.isEmpty()) {
    QPointF prev = m_headPos;
    for (int i = 0; i < m_trail.size(); ++i) {
      int age = m_trail[i].age;

      // Fade out opacity and shrink thickness as it gets older
      int alpha = qMax(0, 180 - (age * 12));
      qreal thickness = qMax(1.0, m_size - (age / 2.0));

      QColor c(m_color);
      c.setAlpha(alpha);

      QPen pen(c);
      pen.setWidthF(thickness);
      pen.setCapStyle(Qt::RoundCap);

      painter->setPen(pen);
      painter->drawLine(prev, m_trail[i].pos);

      prev = m_trail[i].pos;
    }
  }

  // 2. Draw the Glowing Head
  QRadialGradient grad(m_headPos, 12);
  grad.setColorAt(0.0, QColor(255, 255, 255, 255));  // Hot white core
  grad.setColorAt(0.3, m_color);                     // Red laser diode
  QColor fadeColor(m_color);
  fadeColor.setAlpha(0);
  grad.setColorAt(1.0, fadeColor);  // Soft edge fade

  painter->setPen(Qt::NoPen);
  painter->setBrush(grad);
  painter->drawEllipse(m_headPos, 12, 12);
}
